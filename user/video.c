#include "menu.h"
#include "../lvgl/lvgl.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>

#define FIFO_PATH "/workpace/fifo_1"
static char* mp3[] = {"./mp3/huluwa.mp3", "./mp3/lanjingling.mp3"};
#define MP3_COUNT (sizeof(mp3) / sizeof(mp3[0]))
static char* mp4[] = {"./mp3/04.flv","./mp3/04.flv4"};
#define VIDEO_COUNT (sizeof(mp4) / sizeof(mp4[0]))


extern lv_obj_t* page_menu;
extern lv_font_t Ch_make;
extern int page_flag;

// 界面
static lv_obj_t* page_video=NULL;
static lv_obj_t  *content_panels[2];
static lv_obj_t* tab_btns[2];
static lv_obj_t* user_btn[3];
static int current_tab= 0;


// 播放器文件
static pthread_t pid1;
static pthread_t pid2;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  //静态初始化互斥锁
pthread_cond_t cond = PTHREAD_COND_INITIALIZER; //静态初始化条件变量
static int k=0;
static int playing = 0; 
static int fd=0;

// 音乐播放
static lv_obj_t *label_song  = NULL;   
static lv_obj_t *bar_progress = NULL;  // 进度条
static lv_obj_t *label_time   = NULL;
static int       total_sec    = 0; 
static int       paused_sec   = 0;  //存已播时间
static time_t    music_start  = 0;  //当前时间

/* ================================================================
 *  Tab 切换
 * ================================================================ */

// 更新左侧按钮高光
static void update_tab_highlight()
{
    for (int i = 0; i < 2; i++) {
        lv_obj_set_style_bg_color(tab_btns[i],
            (i == current_tab) ? lv_color_hex(0x3377FF) : lv_color_hex(0x444444),
            LV_STATE_DEFAULT);
    }
}

// 切换内容面板
static void panel_sw(int tab)
{
    for(int i=0;i<2;i++)
    {
        lv_obj_add_flag(content_panels[i],LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_remove_flag(content_panels[tab], LV_OBJ_FLAG_HIDDEN);
    current_tab = tab;
    system("killall -9 mplayer64");
    k=0;
    playing=0;
    update_tab_highlight();
}

static void tab_cb_0(lv_event_t *e) { (void)e; panel_sw(0); }
static void tab_cb_1(lv_event_t *e) { (void)e; panel_sw(1); }

void* board_music(void* arg)
{
    while(1)
    {
        //加锁
        pthread_mutex_lock(&mutex);
        while(!playing)
        {
            printf("sleep....\n");
            pthread_cond_wait(&cond, &mutex);
        }

        char cmd[100] = {0};
        if(current_tab==0)
            snprintf(cmd, sizeof(cmd),  "mplayer64 -vo null -slave -input file=/workpace/fifo_1 \"%s\" ", mp3[k]);
        else
            snprintf(cmd, sizeof(cmd), "mplayer64 -vo fbdev2 -slave -quiet -input file=/workpace/fifo_1 %s",mp4[k]);
        system(cmd);
        sleep(1);
        //解锁
        pthread_mutex_unlock(&mutex);
    }
}

/* 用 mplayer -identify 获取时长 */
static int get_mp3_len(const char *path)
{
    char cmd_bar[512], line[256];
    int  len = 0;
    snprintf(cmd_bar, sizeof(cmd_bar),"mplayer64 -vo null -ao null -frames 0 -identify '%s' 2>/dev/null | grep ID_LENGTH",path);
    FILE *fp = popen(cmd_bar, "r");
    if (!fp) return 0;
    while (fgets(line, sizeof(line), fp)) 
    {
        if (strncmp(line, "ID_LENGTH=", 10) == 0) 
        {
            len = (int)atof(line + 10);
            break;
        }
    }
    pclose(fp);
    return len;
}

// 显示进度条时间
static void progress_timer_cb(lv_timer_t *t)
{
    (void)t;
    if (!playing || label_time == NULL) return;

    int now;
    if (music_start == 0) 
    {
        now = paused_sec;           // 暂停中，显示冻结值
    } 
    else 
    {
        now = paused_sec + (int)(time(NULL) - music_start);
    }
    if (total_sec > 0 && now > total_sec) now = total_sec;

    if (total_sec > 0) {
        lv_label_set_text_fmt(label_time, "%02d:%02d / %02d:%02d",
            now / 60, now % 60, total_sec / 60, total_sec % 60);
        lv_bar_set_value(bar_progress, now * 100 / total_sec, LV_ANIM_ON);
    } else {
        lv_label_set_text_fmt(label_time, "%02d:%02d", now / 60, now % 60);
    }
}

void* tran_music(void* arg)
{
    while(1)
    {
        //加锁
        pthread_mutex_lock(&mutex);
        if(playing==1)
        {
            pthread_cond_signal(&cond);
        }
        //解锁
        pthread_mutex_unlock(&mutex);
    }
}

// 如何切换
static void user_event(lv_event_t* e)
{
    int mplayer_exit = 0;
    lv_obj_t* label=lv_event_get_user_data(e);
    char* str=lv_label_get_text(label);

    if(str == NULL) return;
    
    if(strcmp(str,"|<<")==0)
    {
        k=(k-1+2)%2;
        mplayer_exit = 1;

        total_sec    = get_mp3_len(mp3[k]);
        paused_sec  = 0;
        music_start  = time(NULL);
        lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);
        lv_label_set_text(label_time, "00:00 / 00:00");
        lv_label_set_text(label_song, mp3[k]);
    }
    else if(strcmp(str,">>|")==0)
    {
        k=(k+1)%2;
        mplayer_exit = 1;

        total_sec    = get_mp3_len(mp3[k]);
        paused_sec  = 0;
        music_start  = time(NULL);
        lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);
        lv_label_set_text(label_time, "00:00 / 00:00");
        lv_label_set_text(label_song, mp3[k]);
    }
    else if(strcmp(str,"|>")==0) //暂停/继续
    {
        if(playing==0)
        {
            playing=1;

            total_sec    = get_mp3_len(mp3[k]);
            paused_sec   = 0;

            const char* cmd = "pause\n";
            write(fd, cmd, strlen(cmd));
        }
        music_start  = time(NULL);

        const char* cmd = "pause\n";
        write(fd, cmd, strlen(cmd));
        lv_label_set_text(label,"||");
    }
    else if(strcmp(str,"||")==0) //暂停/继续
    {
        if(music_start != 0)                // ← 防止 time(NULL)-0 炸掉
            paused_sec += (int)(time(NULL) - music_start);
        music_start = 0;

        const char* cmd = "pause\n";
        write(fd, cmd, strlen(cmd));
        lv_label_set_text(label,"|>");
    }
    if(mplayer_exit == 1)
    {
        //切歌之前需要把原来的音乐播放器停止
        //kill掉正在播放的歌曲
        system("killall -9 mplayer64");
        //wait回收子进程资源
        // wait(NULL);
    }
}
// 下侧按钮(|<<、|> / ||、>>|)
void btn_user(lv_obj_t* parent,int i)
{
    user_btn[i]=lv_button_create(parent);
    lv_obj_set_size(user_btn[i],120,120);
    lv_obj_align(user_btn[i],LV_ALIGN_BOTTOM_MID, (i - 1) * 180, -30);
    lv_obj_t* label=lv_label_create(user_btn[i]);
    switch(i)
    {
        case 0:
            lv_label_set_text(label,"|<<");
            lv_obj_set_align(label,LV_ALIGN_CENTER);
            break;
        case 1:
            lv_label_set_text(label,"|>");
            lv_obj_set_align(label,LV_ALIGN_CENTER);
            break;
        case 2:
            lv_label_set_text(label,">>|");
            lv_obj_set_align(label,LV_ALIGN_CENTER);
            break;
    }
    lv_obj_add_event_cb(user_btn[i],user_event,LV_EVENT_CLICKED,label);
}

void create_music_panel(lv_obj_t* parent)
{
    for(int i=0;i<3;i++)
    {
        btn_user(parent,i);
    }

    label_song = lv_label_create(parent);
    lv_obj_align(label_song, LV_ALIGN_TOP_MID, 0, 40);
    lv_label_set_text(label_song, mp3[k]);

    static lv_style_t style_music;
    lv_style_init(&style_music);
    lv_style_set_text_font(&style_music, &lv_font_montserrat_32);
    lv_style_set_text_color(&style_music, lv_color_hex(0xFFFF));
    lv_obj_add_style(label_song, &style_music, LV_STATE_DEFAULT);

    /* 进度条 */
    bar_progress = lv_bar_create(parent);
    lv_obj_set_size(bar_progress, 500, 20);
    lv_obj_align(bar_progress, LV_ALIGN_BOTTOM_MID, 0, -160);
    lv_bar_set_range(bar_progress, 0, 100);
    lv_bar_set_value(bar_progress, 0, LV_ANIM_OFF);

    // 进度条时间
    label_time = lv_label_create(parent);
    lv_obj_align(label_time, LV_ALIGN_BOTTOM_MID, 0, -130);
    static lv_style_t st_time;
    lv_style_init(&st_time);
    lv_style_set_text_font(&st_time, &lv_font_montserrat_24);
    lv_style_set_text_color(&st_time, lv_color_hex(0x00FF00));
    lv_obj_add_style(label_time, &st_time, LV_STATE_DEFAULT);
    lv_label_set_text(label_time, "00:00 / 00:00");
    }

void create_movie_panel(lv_obj_t* parent)
{
    for(int i=0;i<3;i++)
    {
        btn_user(parent,i);
    }
}



/* ================================================================
 *  构建总页面
 * ================================================================ */
void build_page(void)
{
    // 总界面容器
    page_video = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_video, 1024, 600);
    lv_obj_set_align(page_video, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(page_video, lv_color_hex(0x222222), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(page_video, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(page_video, 0, LV_STATE_DEFAULT);
    lv_obj_add_flag(page_video, LV_OBJ_FLAG_HIDDEN);

    /* ── 左侧按钮栏背景 ── */
    lv_obj_t *sidebar = lv_obj_create(page_video);
    lv_obj_set_size(sidebar, 200, 600);
    lv_obj_set_pos(sidebar, 0, 0);
    lv_obj_set_style_bg_color(sidebar, lv_color_hex(0x333333), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(sidebar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(sidebar, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(sidebar, 15, LV_STATE_DEFAULT);
    lv_obj_add_flag(sidebar, LV_OBJ_FLAG_CLICKABLE); /* 拦截点击，防止穿透到内容区 */

    // 左侧按钮（音乐+视频）
    const char* tab_names[]={"音乐","视频"};
    lv_event_cb_t tab_cbs[]={tab_cb_0,tab_cb_1};

    for (int i = 0; i < 2; i++) 
    {
        tab_btns[i] = lv_button_create(sidebar);
        lv_obj_set_size(tab_btns[i], 170, 60);
        lv_obj_align(tab_btns[i], LV_ALIGN_TOP_MID, 0, i * 200 + 100);
        lv_obj_set_style_bg_color(tab_btns[i], lv_color_hex(0x444444), LV_STATE_DEFAULT);

        lv_obj_t *l = lv_label_create(tab_btns[i]);
        lv_label_set_text(l, tab_names[i]);
        lv_obj_set_align(l, LV_ALIGN_CENTER);
        set_cn_font(l);
        lv_obj_add_event_cb(tab_btns[i], tab_cbs[i], LV_EVENT_CLICKED, NULL);
    }

    // 右侧界面设计
    lv_obj_t *content_area = lv_obj_create(page_video);
    lv_obj_set_size(content_area, 824, 600);
    lv_obj_set_pos(content_area, 200, 0);
    lv_obj_set_style_bg_color(content_area, lv_color_hex(0x222222), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(content_area, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(content_area, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(content_area, 20, LV_STATE_DEFAULT);

    /* ── 2 个内容面板（全部隐藏，后面填充） ── */
    for (int i = 0; i < 2; i++) {
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
    create_music_panel(content_panels[0]);
    create_movie_panel(content_panels[1]);

    // 退回
    back_prev(page_video);

    panel_sw(0);
}


/* =======================================
* 进入
* ========================================*/
static void entry_cb(lv_event_t* e)
{
    (void)e;
    static int built = 0;

    if (!built) 
    {
        /* 初始化播放器线程 */
        mkfifo(FIFO_PATH,0664);
        playing=0;
        fd = open(FIFO_PATH, O_RDWR);
        build_page();

        pthread_create(&pid1, NULL, board_music, NULL);
        pthread_create(&pid2, NULL, tran_music, NULL);

        lv_timer_create(progress_timer_cb, 200, NULL);

        pthread_detach(pid1);
        pthread_detach(pid2);
        
        built = 1;
    }

    lv_obj_add_flag(page_menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(page_video, LV_OBJ_FLAG_HIDDEN);
}

void video_icon(void)
{
    lv_obj_t* btn=lv_button_create(page_menu);
    lv_obj_set_size(btn,120,120);
    lv_obj_align(btn,LV_ALIGN_LEFT_MID,350,0);

    lv_obj_t* label=lv_label_create(btn);
    lv_label_set_text(label,"音频");
    lv_obj_set_parent(label,lv_obj_get_parent(btn));
    lv_obj_align_to(label,btn,LV_ALIGN_OUT_BOTTOM_MID,-20,0);
    set_cn_font(label);
    
    lv_obj_add_event_cb(btn,entry_cb,LV_EVENT_CLICKED,NULL);
}