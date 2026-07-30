#include "menu.h"
#include "../lvgl/lvgl.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

extern lv_obj_t* page_menu;
extern lv_font_t Ch_make;

/* ================================================================
 *  模块级静态变量
 * ================================================================ */
static lv_obj_t *clk_page = NULL;  
static lv_obj_t *tab_btns[4];
static lv_obj_t *content_panels[4];

static int      current_tab   = 0;         /* 0=时钟 1=闹钟 2=秒表 3=倒计时 */
static int      clk_create = 0;

/* ── 定时器句柄 ── */
static lv_timer_t *clock_timer       = NULL;
static lv_timer_t *alarm_check_timer = NULL;
static lv_timer_t *stopwatch_timer   = NULL;
static lv_timer_t *countdown_timer   = NULL;

/* ── 闹钟数据 ── */
static int alarm_hour          = 0;
static int alarm_min           = 0;
static int alarm_enabled       = 0;
static lv_obj_t *alarm_status  = NULL;

/* ── 秒表数据 ── */
static int      sw_running    = 0;
static uint32_t sw_start_tick = 0;
static uint32_t sw_elapsed    = 0;
static lv_obj_t *sw_label     = NULL;

/* ── 倒计时数据 ── */
static int      cd_running      = 0;
static int      cd_remaining_sec = 0;
static lv_obj_t *cd_label       = NULL;
static lv_obj_t *cd_roller_min  = NULL;
static lv_obj_t *cd_roller_sec  = NULL;


// 构建roller字符串
static void build_options(char* buf,int max_val)
{
    buf[0]='\0';
    for(int i=0;i<max_val;i++)
    {
        char tmp[8];
        snprintf(tmp,sizeof(tmp),"%02d\n",i);
        strcat(buf,tmp);
    }
}

// 更新左侧按钮高光
static void update_tab_highlight()
{
    for (int i = 0; i < 4; i++) {
        lv_obj_set_style_bg_color(tab_btns[i],
            (i == current_tab) ? lv_color_hex(0x3377FF) : lv_color_hex(0x444444),
            LV_STATE_DEFAULT);
    }
}

// 切换内容面板
static void panel_sw(int tab)
{
    for(int i=0;i<4;i++)
    {
        lv_obj_add_flag(content_panels[i],LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_remove_flag(content_panels[tab], LV_OBJ_FLAG_HIDDEN);
    current_tab = tab;
    update_tab_highlight();
}

/* 关闭 msgbox 回调 */
static void mbox_close_cb(lv_event_t *e)
{
    lv_obj_t *mbox = lv_event_get_user_data(e);
    lv_obj_add_flag(mbox, LV_OBJ_FLAG_HIDDEN);
    // lv_obj_delete(lv_event_get_user_data(e));
    lv_msgbox_close(mbox); 
}

// 离开
static void leave_clk(void)
{
    if(clock_timer) {lv_timer_delete(clock_timer);    clock_timer=NULL;} 
    if(stopwatch_timer) {lv_timer_delete(stopwatch_timer);    stopwatch_timer=NULL;} 
    if(countdown_timer) {lv_timer_delete(countdown_timer);    countdown_timer=NULL;} 

    sw_running=0;
    cd_running=0;

    lv_obj_add_flag(page_menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(clk_page, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(page_menu, LV_OBJ_FLAG_HIDDEN);
}

/* ================================================================
 *  左侧按钮回调
 * ================================================================ */

static void tab_cb_0(lv_event_t *e) { (void)e; panel_sw(0); }
static void tab_cb_1(lv_event_t *e) { (void)e; panel_sw(1); }
static void tab_cb_2(lv_event_t *e) { (void)e; panel_sw(2); }
static void tab_cb_3(lv_event_t *e) { (void)e; panel_sw(3); }

static void back_cb(lv_event_t *e)
{
    (void)e;
    leave_clk();
}


/* ================================================================
 *  时钟 —— content_panels[0]
 * ================================================================ */

static lv_obj_t *clock_label = NULL;

static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    lv_label_set_text_fmt(clock_label, "%02d:%02d:%02d",
                          tm->tm_hour, tm->tm_min, tm->tm_sec);
}

// 进入
static void enter_clk(void)
{
    lv_obj_add_flag(page_menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(clk_page, LV_OBJ_FLAG_HIDDEN);

    /* 默认显示时钟面板，启动时钟刷新 */
    panel_sw(0);
    if (!clock_timer) {
        clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
    }
}

static void create_clock_panel(lv_obj_t *parent)
{
    // 标题
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "实时时钟");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    set_cn_font_white(title);

    // 时间
    clock_label = lv_label_create(parent);
    lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, -30);
    static lv_style_t s;
    lv_style_init(&s);
    lv_style_set_text_font(&s, &lv_font_montserrat_48);
    lv_style_set_text_color(&s, lv_color_hex(0xFFFF));
    lv_obj_add_style(clock_label, &s, LV_STATE_DEFAULT);
    lv_label_set_text(clock_label, "--:--:--");
}

/* ================================================================
 *  闹钟 —— content_panels[1]
 * ================================================================ */
static lv_obj_t* alarm_roller_hour=NULL;
static lv_obj_t* alarm_roller_min=NULL;

static void alarm_check_timer_cb(lv_timer_t *t)
{
    if(!alarm_enabled)  return;

    time_t now=time(NULL);
    struct tm* tm=localtime(&now);
    if(tm->tm_hour==alarm_hour && tm->tm_min==alarm_min)
    {
        alarm_enabled=0;
        if(alarm_check_timer)   {lv_timer_delete(alarm_check_timer);  alarm_check_timer=NULL;}
        if(alarm_status) lv_label_set_text(alarm_status, "闹钟未开启");

        lv_obj_t *mbox = lv_msgbox_create(NULL);
        lv_obj_t *t = lv_msgbox_add_title(mbox, "闹钟");
        set_cn_font(t);
        lv_obj_t *txt=lv_msgbox_add_text(mbox, "时间到！");
        set_cn_font(txt);
        lv_obj_t *btn = lv_msgbox_add_footer_button(mbox, "关闭");
        set_cn_font(btn);
        lv_obj_add_event_cb(btn, mbox_close_cb, LV_EVENT_CLICKED, mbox);
    }
}

static void alarm_set_cb(lv_event_t *e)
{
    (void)e;
    char buf[4];
    lv_roller_get_selected_str(alarm_roller_hour,buf,sizeof(buf));
    alarm_hour=atoi(buf);
    lv_roller_get_selected_str(alarm_roller_min,buf,sizeof(buf));
    alarm_min=atoi(buf);
    alarm_enabled=1;

    if (alarm_check_timer) lv_timer_delete(alarm_check_timer);
    alarm_check_timer = lv_timer_create(alarm_check_timer_cb, 1000, NULL);

    lv_label_set_text_fmt(alarm_status, "已开启: %02d:%02d", alarm_hour, alarm_min);
}

static void create_alarm_panel(lv_obj_t *parent)
{
    // 标题
    lv_obj_t* title=lv_label_create(parent);
    lv_label_set_text(title,"闹钟设计");
    lv_obj_align(title,LV_ALIGN_TOP_MID,0,30);
    set_cn_font_white(title);
    // 小时
    alarm_roller_hour=lv_roller_create(parent);
    lv_obj_set_size(alarm_roller_hour,130,200);
    lv_obj_align(alarm_roller_hour,LV_ALIGN_CENTER,-100,-20);
    char buf[256];
    build_options(buf,24);
    lv_roller_set_options(alarm_roller_hour,buf,LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(alarm_roller_hour,5);
    // 冒号
    lv_obj_t* colon=lv_label_create(parent);
    lv_label_set_text(colon,":");
    lv_obj_align(colon, LV_ALIGN_CENTER, 0, -20);
    static lv_style_t sc;
    lv_style_init(&sc);
    lv_style_set_text_font(&sc, &lv_font_montserrat_32);
    lv_obj_add_style(colon, &sc, LV_STATE_DEFAULT);
    // 分钟
    alarm_roller_min=lv_roller_create(parent);
    lv_obj_set_size(alarm_roller_min,130,200);
    lv_obj_align(alarm_roller_min,LV_ALIGN_CENTER,100,-20);
    build_options(buf,60);
    lv_roller_set_options(alarm_roller_min,buf,LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(alarm_roller_min,5);
    // 开启按钮
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 160, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 130);
    lv_obj_t *bl = lv_label_create(btn);
    lv_label_set_text(bl, "开启闹钟");
    lv_obj_set_align(bl, LV_ALIGN_CENTER);
    set_cn_font_white(bl);
    lv_obj_add_event_cb(btn, alarm_set_cb, LV_EVENT_CLICKED, NULL);
    // 状态标签
    alarm_status = lv_label_create(parent);
    lv_label_set_text(alarm_status, "闹钟未开启");
    lv_obj_align(alarm_status, LV_ALIGN_CENTER, 0, 190);
    set_cn_font_white(alarm_status);

//     lv_obj_move_foreground(btn);
//     lv_obj_move_foreground(alarm_status);
}

/* ================================================================
 *  秒表 —— content_panels[2]
 * ================================================================ */
static void sw_update_label(void)
{
    uint32_t ms  = sw_elapsed;
    uint32_t sec = ms / 1000;
    uint32_t min = sec / 60;
    lv_label_set_text_fmt(sw_label, "%02d:%02d.%01d", min, sec % 60, (ms % 1000) / 100);
}

static void sw_timer_cb(lv_timer_t *t)
{
    (void)t;
    sw_elapsed = lv_tick_get() - sw_start_tick;
    sw_update_label();
}

static void sw_start_cb(lv_event_t *e)
{
    (void)e;
    if (sw_running == 1) return;
    if (sw_running == 0) { sw_start_tick = lv_tick_get(); sw_elapsed = 0; }
    if (sw_running == 2) { sw_start_tick = lv_tick_get() - sw_elapsed; }
    sw_running = 1;
    if (!stopwatch_timer) stopwatch_timer = lv_timer_create(sw_timer_cb, 100, NULL);
}

static void sw_pause_cb(lv_event_t *e)
{
    (void)e;
    if (sw_running != 1) return;
    sw_running = 2;
    sw_elapsed = lv_tick_get() - sw_start_tick;
    if (stopwatch_timer) { lv_timer_delete(stopwatch_timer); stopwatch_timer = NULL; }
    sw_update_label();
}

static void sw_reset_cb(lv_event_t *e)
{
    (void)e;
    sw_running = 0; sw_elapsed = 0;
    if (stopwatch_timer) { lv_timer_delete(stopwatch_timer); stopwatch_timer = NULL; }
    lv_label_set_text(sw_label, "00:00.0");
}

static void create_stopwatch_panel(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "秒表");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    set_cn_font_white(title);

    sw_label = lv_label_create(parent);
    lv_obj_align(sw_label, LV_ALIGN_CENTER, 0, -60);
    static lv_style_t s;
    lv_style_init(&s);
    lv_style_set_text_font(&s, &lv_font_montserrat_48);
    lv_style_set_text_color(&s, lv_color_hex(0x00FF00));
    lv_obj_add_style(sw_label, &s, LV_STATE_DEFAULT);
    lv_label_set_text(sw_label, "00:00.0");

    const char *txt[] = {"开始", "暂停", "复位"};
    lv_event_cb_t cb[] = {sw_start_cb, sw_pause_cb, sw_reset_cb};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(parent);
        lv_obj_set_size(btn, 140, 55);
        lv_obj_align(btn, LV_ALIGN_CENTER, (i - 1) * 160, 70);
        lv_obj_t *l  = lv_label_create(btn);
        lv_label_set_text(l, txt[i]);
        lv_obj_set_align(l, LV_ALIGN_CENTER);
        set_cn_font_white(l);
        lv_obj_add_event_cb(btn, cb[i], LV_EVENT_CLICKED, NULL);
    }
}

/* ================================================================
*  倒计时 —— content_panels[3]
* ================================================================ */
static void cd_update_label(void)
{
    lv_label_set_text_fmt(cd_label, "%02d:%02d",
                          cd_remaining_sec / 60, cd_remaining_sec % 60);
}

static void cd_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (cd_remaining_sec > 0) {
        cd_remaining_sec--;
        cd_update_label();
    }
    if (cd_remaining_sec == 0) {
        cd_running = 0;
        if (countdown_timer) { lv_timer_delete(countdown_timer); countdown_timer = NULL; }

        lv_obj_t *mbox = lv_msgbox_create(NULL);
        lv_obj_t *tt   = lv_msgbox_add_title(mbox, "倒计时");
        set_cn_font_white(tt);
        lv_msgbox_add_text(mbox, "时间到！");
        lv_obj_t *btn  = lv_msgbox_add_footer_button(mbox, "关闭");
        set_cn_font_white(btn);
        lv_obj_add_event_cb(btn, mbox_close_cb, LV_EVENT_CLICKED, mbox);
    }
}

static void cd_start_cb(lv_event_t *e)
{
    (void)e;
    if (cd_running == 1) return;
    if (cd_running == 0) {
        char buf[4];
        lv_roller_get_selected_str(cd_roller_min, buf, sizeof(buf));
        int m = atoi(buf);
        lv_roller_get_selected_str(cd_roller_sec, buf, sizeof(buf));
        int s = atoi(buf);
        cd_remaining_sec = m * 60 + s;
        if (cd_remaining_sec <= 0) return;
        cd_update_label();
    }
    cd_running = 1;
    if (!countdown_timer) countdown_timer = lv_timer_create(cd_timer_cb, 1000, NULL);
}

static void cd_pause_cb(lv_event_t *e)
{
    (void)e;
    if (cd_running != 1) return;
    cd_running = 2;
    if (countdown_timer) { lv_timer_delete(countdown_timer); countdown_timer = NULL; }
}

static void cd_reset_cb(lv_event_t *e)
{
    (void)e;
    cd_running = 0; cd_remaining_sec = 0;
    if (countdown_timer) { lv_timer_delete(countdown_timer); countdown_timer = NULL; }
    lv_label_set_text(cd_label, "--:--");
}

static void create_countdown_panel(lv_obj_t *parent)
{
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "倒计时");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    set_cn_font_white(title);

    /* 分钟 roller */
    cd_roller_min = lv_roller_create(parent);
    lv_obj_set_size(cd_roller_min, 130, 180);
    lv_obj_align(cd_roller_min, LV_ALIGN_CENTER, -100, -10);
    char buf[512];
    build_options(buf, 100);
    lv_roller_set_options(cd_roller_min, buf, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(cd_roller_min, 5);
    lv_obj_t *lm = lv_label_create(parent);
    lv_label_set_text(lm, "分");
    lv_obj_align_to(lm, cd_roller_min, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    set_cn_font_white(lm);

    /* 秒 roller */
    cd_roller_sec = lv_roller_create(parent);
    lv_obj_set_size(cd_roller_sec, 130, 180);
    lv_obj_align(cd_roller_sec, LV_ALIGN_CENTER, 100, -10);
    build_options(buf, 60);
    lv_roller_set_options(cd_roller_sec, buf, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(cd_roller_sec, 5);
    lv_obj_t *ls = lv_label_create(parent);
    lv_label_set_text(ls, "秒");
    lv_obj_align_to(ls, cd_roller_sec, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    set_cn_font_white(ls);

    /* 倒计时显示 */
    cd_label = lv_label_create(parent);
    lv_obj_align(cd_label, LV_ALIGN_CENTER, 0, -150);
    static lv_style_t s;
    lv_style_init(&s);
    lv_style_set_text_font(&s, &lv_font_montserrat_36);
    lv_style_set_text_color(&s, lv_color_hex(0xFF6600));
    lv_obj_add_style(cd_label, &s, LV_STATE_DEFAULT);
    lv_label_set_text(cd_label, "--:--");

    /* 按钮 */
    const char *txt[] = {"开始", "暂停", "复位"};
    lv_event_cb_t cb[] = {cd_start_cb, cd_pause_cb, cd_reset_cb};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *btn = lv_button_create(parent);
        lv_obj_set_size(btn, 140, 55);
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, (i - 1) * 160, -60);
        lv_obj_t *l = lv_label_create(btn);
        lv_label_set_text(l, txt[i]);
        lv_obj_set_align(l, LV_ALIGN_CENTER);
        set_cn_font_white(l);
        lv_obj_add_event_cb(btn, cb[i], LV_EVENT_CLICKED, NULL);
    }
}





/* ================================================================
 *  构建页面：左侧按钮栏 + 右侧内容区
 * ================================================================ */
static void build_clk_page(void)
{
    /* ── 总页面容器 ── */
    clk_page = lv_obj_create(lv_screen_active());
    lv_obj_set_size(clk_page, 1024, 600);
    lv_obj_set_align(clk_page, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(clk_page, lv_color_hex(0x222222), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(clk_page, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(clk_page, 0, LV_STATE_DEFAULT);
    lv_obj_add_flag(clk_page, LV_OBJ_FLAG_HIDDEN);

    /* ── 左侧按钮栏背景 ── */
    lv_obj_t *sidebar = lv_obj_create(clk_page);
    lv_obj_set_size(sidebar, 200, 600);
    lv_obj_set_pos(sidebar, 0, 0);
    lv_obj_set_style_bg_color(sidebar, lv_color_hex(0x333333), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(sidebar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(sidebar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(sidebar, 15, LV_STATE_DEFAULT);
    lv_obj_add_flag(sidebar, LV_OBJ_FLAG_CLICKABLE); /* 拦截点击，防止穿透到内容区 */

    /* ── 右侧内容区 ── */
    lv_obj_t *content_area = lv_obj_create(clk_page);
    lv_obj_set_size(content_area, 824, 600);
    lv_obj_set_pos(content_area, 200, 0);
    lv_obj_set_style_bg_color(content_area, lv_color_hex(0x222222), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(content_area, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(content_area, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(content_area, 20, LV_STATE_DEFAULT);

    /* ── 4 个内容面板（全部隐藏，后面填充） ── */
    for (int i = 0; i < 4; i++) {
        content_panels[i] = lv_obj_create(content_area);
        lv_obj_set_size(content_panels[i], 824 - 40, 600 - 40); /* 扣除 content_area 的 pad */
        lv_obj_set_align(content_panels[i],LV_ALIGN_CENTER);
        lv_obj_set_style_bg_color(content_panels[i], lv_color_hex(0x222222), LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(content_panels[i], 0, LV_STATE_DEFAULT);
        lv_obj_set_style_radius(content_panels[i], 0, LV_STATE_DEFAULT);
        lv_obj_set_style_pad_all(content_panels[i], 10, LV_STATE_DEFAULT);
        lv_obj_add_flag(content_panels[i], LV_OBJ_FLAG_HIDDEN);
    }

    /* 填充各面板 */
    create_clock_panel(content_panels[0]);
    create_alarm_panel(content_panels[1]);
    create_stopwatch_panel(content_panels[2]);
    create_countdown_panel(content_panels[3]);

    /* ── 左侧 5 个按钮（4 tab + 返回） ── */
    const char *tab_names[] = {"时钟", "闹钟", "秒表", "倒计时"};
    lv_event_cb_t tab_cbs[] = {tab_cb_0, tab_cb_1, tab_cb_2, tab_cb_3};

    for (int i = 0; i < 4; i++) {
        tab_btns[i] = lv_button_create(sidebar);
        lv_obj_set_size(tab_btns[i], 170, 60);
        lv_obj_align(tab_btns[i], LV_ALIGN_TOP_MID, 0, i * 80 + 20);
        lv_obj_set_style_bg_color(tab_btns[i], lv_color_hex(0x444444), LV_STATE_DEFAULT);

        lv_obj_t *l = lv_label_create(tab_btns[i]);
        lv_label_set_text(l, tab_names[i]);
        lv_obj_set_align(l, LV_ALIGN_CENTER);
        set_cn_font_white(l);
        lv_obj_add_event_cb(tab_btns[i], tab_cbs[i], LV_EVENT_CLICKED, NULL);
    }

    /* 返回按钮 */
    lv_obj_t *btn_back = lv_button_create(sidebar);
    lv_obj_set_size(btn_back, 170, 60);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x555555), LV_STATE_DEFAULT);
    lv_obj_t *bl = lv_label_create(btn_back);
    lv_label_set_text(bl, "返回");
    lv_obj_set_align(bl, LV_ALIGN_CENTER);
    set_cn_font_white(bl);
    lv_obj_add_event_cb(btn_back, back_cb, LV_EVENT_CLICKED, NULL);
}

static void entry_cb(lv_event_t *e)
{
    (void)e;
    if (!clk_create) 
    {
        build_clk_page();
        clk_create = 1;
    }
    enter_clk();
}

// 图标
void clock_icon(void)
{
    lv_obj_t* btn=lv_button_create(page_menu);
    lv_obj_set_size(btn,120,120);
    lv_obj_align(btn,LV_ALIGN_LEFT_MID,190,0);

    lv_obj_t* label=lv_label_create(btn);
    lv_label_set_text(label,"时钟闹钟");
    lv_obj_set_parent(label,lv_obj_get_parent(btn));
    lv_obj_align_to(label,btn,LV_ALIGN_OUT_BOTTOM_MID,-42,0);
    set_cn_font(label);
    
    lv_obj_add_event_cb(btn,entry_cb,LV_EVENT_CLICKED,NULL);
}
