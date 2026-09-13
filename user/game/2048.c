#include "../menu.h"
#include "../../lvgl/lvgl.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

extern lv_obj_t* page_game;
extern lv_font_t Ch_make;
extern int page_flag;

static int build=0;
static lv_obj_t* page_2048=NULL;
static lv_obj_t* panel_2048=NULL;

int score_max=0;
static int score=0;

static void entry_game(void)
{
     
}

static void event_start(lv_event_t* e)
{
    lv_obj_t* label=lv_event_get_user_data(e);
    char* str=lv_label_get_text(label);
    if(strcmp(str,"开始游戏")==0)
    {
        entry_game();
    }
}

static void build_left(lv_obj_t* parent)
{
    lv_obj_t * btn_start = lv_btn_create(parent);
    lv_obj_set_size(btn_start, 160, 80);
    lv_obj_set_pos(btn_start, 26, 260);

    lv_obj_t * label = lv_label_create(btn_start);
    lv_label_set_text(label, "开始游戏");
    lv_obj_center(label);
    set_cn_font_red(label);

    lv_obj_add_event_cb(btn_start,event_start,LV_EVENT_CLICKED,label);
    }

/*========构建总界面==========*/
static void build_page(void)
{
    // 主界面
    page_2048=lv_obj_create(lv_screen_active());
    lv_obj_set_align(page_2048,LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(page_2048, lv_color_hex(0x222222), LV_STATE_DEFAULT);
    lv_obj_add_flag(page_2048,LV_OBJ_FLAG_HIDDEN);

    // 滑动界面（4*4）
    panel_2048=lv_obj_create(page_game);
    lv_obj_set_size(panel_2048,600,600);
    lv_obj_set_align(page_2048,LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(page_2048, lv_color_hex(0x333333), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(page_2048, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(page_2048, 0, LV_STATE_DEFAULT);
    lv_obj_add_flag(page_2048,LV_OBJ_FLAG_HIDDEN);

    // 信息界面：最高分、得分
    lv_obj_t *panel_right = lv_obj_create(page_2048);
    lv_obj_set_size(panel_right, 212, 600);
    lv_obj_set_pos(panel_right, 0, 0);
    lv_obj_set_style_bg_color(panel_right, lv_color_hex(0x333333), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_right, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel_right, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel_right, 15, LV_STATE_DEFAULT);
    lv_obj_add_flag(panel_right, LV_OBJ_FLAG_CLICKABLE); /* 拦截点击，防止穿透到内容区 */

    // 操作界面：退出、开始游戏
    lv_obj_t *panel_left = lv_obj_create(page_2048);
    lv_obj_set_size(panel_left, 212, 600);
    lv_obj_set_pos(panel_left, 0, 0);
    lv_obj_set_style_bg_color(panel_left, lv_color_hex(0x333333), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(panel_left, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_radius(panel_left, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(panel_left, 15, LV_STATE_DEFAULT);
    lv_obj_add_flag(panel_left, LV_OBJ_FLAG_CLICKABLE); /* 拦截点击，防止穿透到内容区 */

    page_flag=-1;
    back_prev(page_2048);
}

/*==========进入============*/
static void entry_cb(lv_event_t* e)
{
    (void)e;
    build=0;
    if(!build)
    {
        build_page();
        build=1;
    }
    lv_obj_add_flag(page_game,LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(page_2048,LV_OBJ_FLAG_HIDDEN);
}

void icon_2048(void)
{
    lv_obj_t* btn=lv_button_create(page_game);
    lv_obj_set_size(btn,120,120);
    lv_obj_align(btn,LV_ALIGN_TOP_LEFT,30,0);

    lv_obj_t* label=lv_label_create(btn);
    lv_label_set_text(label,"2048");
    lv_obj_set_parent(label,lv_obj_get_parent(btn));
    lv_obj_align_to(label,btn,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
    set_cn_font(label);

    lv_obj_add_event_cb(btn,entry_cb,LV_EVENT_CLICKED,NULL);
}