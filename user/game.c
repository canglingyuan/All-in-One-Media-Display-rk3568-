#include "menu.h"
#include "../lvgl/lvgl.h"
#include <time.h>
#include <stdio.h>
#include <string.h>

extern lv_obj_t* page_menu;
extern lv_font_t Ch_make;
extern int page_flag;

static lv_obj_t* page_game=NULL;

static void entry_game(lv_event_t* e)
{
    lv_obj_t* label=lv_event_get_user_data(e);
    char* s=lv_label_get_text(label);
}
/*----------------------游戏图标设计---------------------*/
// 1.2048
void icon_2048(void)
{
    lv_obj_t* btn=lv_button_create(page_menu);
    lv_obj_set_size(btn,120,120);
    lv_obj_align(btn,LV_ALIGN_TOP_LEFT,30,0);

    lv_obj_t* label=lv_label_create(btn);
    lv_label_set_text(label,"2048");
    lv_obj_set_parent(label,lv_obj_get_parent(btn));
    lv_obj_align_to(label,btn,LV_ALIGN_OUT_BOTTOM_LEFT,0,0);
    set_cn_font(label);

    lv_obj_add_event_cb(btn,entry_game,LV_EVENT_CLICKED,NULL);
}

static create_game()
{
    if(page_game!=NULL)
    {
        lv_obj_delete(page_game);
    }
    back_prev(page_game);

    icon_2048();

}

static void entry_cb(lv_event_t* e)
{
    (void)e;
    lv_obj_add_flag(page_menu,LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(page_game,LV_OBJ_FLAG_HIDDEN);
    create_game();
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