#include "menu.h"
#include "../lvgl/lvgl.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

extern lv_obj_t* page_menu;
extern lv_font_t Ch_make;
extern int page_flag;

lv_obj_t* page_game=NULL;
static int build=0;


/*========构建总界面==========*/
static void build_page()
{
    if(page_game!=NULL)
    {
        lv_obj_delete(page_game);
    }
    // 主界面
    page_game = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_game, 1024, 600);
    lv_obj_set_align(page_game, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(page_game, lv_color_hex(0x222222), LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(page_game, 0, LV_STATE_DEFAULT);
    lv_obj_set_style_pad_all(page_game, 0, LV_STATE_DEFAULT);
    lv_obj_add_flag(page_game, LV_OBJ_FLAG_HIDDEN);

    // 游戏类型
    icon_2048();

    // 退出
    page_flag=-1;
    back_prev(page_game);

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
    lv_obj_add_flag(page_menu,LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(page_game,LV_OBJ_FLAG_HIDDEN);
}

void game_icon()
{
    lv_obj_t* btn=lv_button_create(page_menu);
    lv_obj_set_size(btn,120,120);
    lv_obj_align(btn,LV_ALIGN_LEFT_MID,510,0);

    lv_obj_t* label=lv_label_create(btn);
    lv_label_set_text(label,"游戏");
    lv_obj_set_parent(label,lv_obj_get_parent(btn));
    lv_obj_align_to(label,btn,LV_ALIGN_OUT_BOTTOM_MID,-15,0);
    set_cn_font(label);
    
    lv_obj_add_event_cb(btn,entry_cb,LV_EVENT_CLICKED,NULL);
}