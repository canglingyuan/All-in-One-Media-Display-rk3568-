#include "../lvgl/lvgl.h"
// #include "../lvgl/demos/lv_demos.h"
// #include <unistd.h>
// #include <pthread.h>
#include <time.h>
#include "Ch_make.h"
#include <stdio.h>
#include <string.h>

lv_obj_t* page_menu;
lv_obj_t* log_image;
int key_flag=0;
int page_flag=0;

// 中文样式
void set_cn_font(lv_obj_t *label)   //默认
{
    static lv_style_t s;
    lv_style_init(&s);
    lv_style_set_text_font(&s, &Ch_make);
    lv_obj_add_style(label, &s, LV_STATE_DEFAULT);
}
void set_cn_font_red(lv_obj_t *label)   //红色
{
    static lv_style_t s;
    lv_style_init(&s);
    lv_style_set_text_font(&s, &Ch_make);
    lv_style_set_text_color(&s, lv_color_hex(0xFF0000));
    lv_obj_add_style(label, &s, LV_STATE_DEFAULT);
}
void set_cn_font_white(lv_obj_t *label) //白色
{
    static lv_style_t s;
    lv_style_init(&s);
    lv_style_set_text_font(&s, &Ch_make);
    lv_style_set_text_color(&s, lv_color_hex(0xFFFFFF));
    lv_obj_add_style(label, &s, LV_STATE_DEFAULT);
}

// 返回上一级
void back_event(lv_event_t* e)
{
    lv_obj_t* label_back=lv_event_get_user_data(e);
    char* str=lv_label_get_text(label_back);
    lv_obj_t* btn_back=lv_event_get_target(e);
    lv_obj_t* parent=lv_obj_get_parent(btn_back);

    if(strcmp(str,"退回")==0)
    {
        // lv_obj_add_flag(parent,LV_OBJ_FLAG_HIDDEN);
        if(page_flag==0)
        {
            lv_obj_add_flag(parent,LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(page_menu,LV_OBJ_FLAG_HIDDEN);
        }
        else if(page_flag==-1)
        {
            lv_obj_add_flag(parent,LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(page_menu,LV_OBJ_FLAG_HIDDEN);
            lv_obj_delete(parent);
        }
    }
}
void back_prev(lv_obj_t* parent)
{
    lv_obj_t* btn_back = lv_button_create(parent);
    lv_obj_set_size(btn_back, 80, 40);
    lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT,5,5);

    lv_obj_t* label_back = lv_label_create(btn_back);
    lv_obj_set_align(label_back, LV_ALIGN_CENTER);
    lv_label_set_text(label_back,"退回");
    set_cn_font_red(label_back);
    lv_obj_move_foreground(label_back);

    lv_obj_add_event_cb(btn_back,back_event, LV_EVENT_CLICKED, label_back);
}


static void password_event(lv_event_t* e)
{
    lv_event_code_t code=lv_event_get_code(e);
    lv_obj_t* ta=lv_event_get_target(e);
    lv_obj_t* kb=lv_event_get_user_data(e);

    if(code==LV_EVENT_FOCUSED)
    {
        lv_keyboard_set_textarea(kb,ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(kb);  // 将键盘提到最顶层，防止被图片遮挡
        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    }
    else if(code==LV_EVENT_DEFOCUSED)
    {
        lv_keyboard_set_textarea(kb,NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }

    if(code == LV_EVENT_READY)
    {
        const char* key = lv_textarea_get_text(ta);
        printf("%s\n", key);

        if(strcmp(key, "123456") == 0)
        {
            key_flag = 1;
            // lv_obj_add_flag(log_image, LV_OBJ_FLAG_HIDDEN);
            // lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
            lv_obj_remove_flag(page_menu, LV_OBJ_FLAG_HIDDEN);
            lv_obj_delete(log_image);
            lv_obj_delete(kb);          
        }
        else
        {
            printf("密码错误\n");
        }
    }
}

void log_Init()
{
    lv_obj_t* kb = lv_keyboard_create(lv_screen_active());
    lv_obj_set_align(kb, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_size(kb, 400, 300);
    lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);

    log_image=lv_image_create(lv_screen_active());
    lv_image_set_src(log_image,"A:/workpace/pic/log_image.bmp");
    lv_obj_set_align(log_image,LV_ALIGN_CENTER);

    lv_obj_t* log_ta=lv_textarea_create(log_image);
    lv_obj_set_size(log_ta,400,40);
    lv_obj_align(log_ta,LV_ALIGN_CENTER,0,40);
    lv_obj_add_event_cb(log_ta,password_event,LV_EVENT_ALL,kb);
    lv_textarea_set_placeholder_text(log_ta, "password:");

    page_menu=lv_image_create(lv_screen_active());
    lv_image_set_src(page_menu,"A:/workpace/pic/page_menu.bmp");
    lv_obj_set_align(page_menu,LV_ALIGN_CENTER);
    lv_obj_add_flag(page_menu, LV_OBJ_FLAG_HIDDEN);
}

static lv_obj_t *label_Time = NULL;

static void clock_timer_cb(lv_timer_t *t)
{
    (void)t;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    lv_label_set_text_fmt(label_Time, "%02d:%02d",tm->tm_hour, tm->tm_min);
}

void time_Init()
{
    label_Time=lv_label_create(page_menu);
    lv_obj_align(label_Time,LV_ALIGN_TOP_RIGHT,-300,40);

    static lv_style_t style_Time;
    lv_style_init(&style_Time);
    lv_style_set_text_font(&style_Time,&lv_font_montserrat_48);
    lv_obj_set_style_transform_scale_x(label_Time,800,LV_STATE_DEFAULT);
    lv_obj_set_style_transform_scale_y(label_Time, 800, LV_STATE_DEFAULT);

    // 设置缩放中心为标签中心，避免偏移
    lv_obj_set_style_transform_pivot_x(label_Time, lv_obj_get_width(label_Time) / 2, LV_STATE_DEFAULT);
    lv_obj_set_style_transform_pivot_y(label_Time, lv_obj_get_height(label_Time) / 2, LV_STATE_DEFAULT);
    lv_style_set_text_color(&style_Time,lv_color_hex(0xFFFF));
    lv_obj_add_style(label_Time,&style_Time,LV_STATE_DEFAULT);

    lv_timer_create(clock_timer_cb,1000,NULL);
}

void app_Init()
{
    // 相册
    pic_icon();
    // 闹钟时钟
    clock_icon();
    // 游戏
    // game_icon();
    // 音频播放器
    video_icon();

}

void desktop_Init()
{
    time_Init();
    app_Init();
}
