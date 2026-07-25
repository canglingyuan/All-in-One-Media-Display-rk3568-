#include "menu.h"
#include "../lvgl/lvgl.h"
#include <stdio.h>
#include <string.h>

extern lv_font_t Ch_make;
extern int page_flag;

static int i=0;
static lv_obj_t * page_pic ;
static lv_obj_t * image ;
#define IMG_MAX_NUM (sizeof(pics)/sizeof(pics[0]))

void album(void);

char* pics[]= {
    "A:/workpace/1.bmp",
    "A:/workpace/2.bmp",
    "A:/workpace/3.bmp",
    "A:/workpace/4.bmp",
    "A:/workpace/5.bmp",
    };

static void pic_event_handle(lv_event_t* e)
{
    lv_event_code_t code=lv_event_get_code(e);
    lv_obj_t* label_pic = lv_event_get_user_data(e);
    if(code==LV_EVENT_PRESSED)
    {
        extern lv_obj_t* page_menu;
        lv_obj_add_flag(page_menu, LV_OBJ_FLAG_HIDDEN);
        album();
    }
}

// 相册图标
void pic_icon(void)
{
    if(page_pic != NULL) 
    {
        lv_obj_add_flag(page_pic, LV_OBJ_FLAG_HIDDEN);
    }

    extern lv_obj_t* page_menu;
    lv_obj_t* btn_pic = lv_button_create(page_menu);
    lv_obj_set_size(btn_pic, 120, 120);
    lv_obj_align(btn_pic, LV_ALIGN_LEFT_MID,30,0);

    lv_obj_t* label_pic = lv_label_create(btn_pic);
    lv_label_set_text(label_pic, "相册");
    lv_obj_set_parent(label_pic, lv_obj_get_parent(btn_pic)); // 移到同一父容器
    lv_obj_align_to(label_pic, btn_pic, LV_ALIGN_OUT_BOTTOM_MID, -15, 0);
    // lv_obj_set_align(label_pic, LV_ALIGN_OUT_BOTTOM_MID);

    static lv_style_t style1;
    lv_style_init(&style1);
    lv_style_set_text_font(&style1, &Ch_make);
    lv_obj_add_style(label_pic, &style1, LV_STATE_DEFAULT);

    lv_obj_add_event_cb(btn_pic, pic_event_handle, LV_EVENT_ALL, label_pic);
}

static void pacture_event(lv_event_t* e)
{
    lv_obj_t* label=lv_event_get_user_data(e);
    char* str=lv_label_get_text(label);
    if(strcmp(str, "上一张") == 0)
    {
        if(i>0)
        {
            i--;
        }
        else
        i=IMG_MAX_NUM -1;
    }
    else if(strcmp(str, "下一张") == 0)
    {
        if(i<IMG_MAX_NUM -1)
        {
            i++;
        }
        else
        i=0;
    }

    lv_image_set_src(image, pics[i]);

    if(strcmp(str,"退回")==0)
    {
        extern lv_obj_t* page_menu;
        lv_obj_add_flag(page_pic, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(page_menu, LV_OBJ_FLAG_HIDDEN);
    }
}

// 相册
void album(void)
{
    if(page_pic != NULL) 
    {
        lv_obj_delete(page_pic);   // 先删旧的
    }

    page_pic=lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_pic,1024,600);
    lv_obj_set_align(page_pic,LV_ALIGN_CENTER);

    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_text_font(&style, &Ch_make);
    lv_style_set_text_color(&style, lv_color_hex(0xFF0000));

    lv_obj_t* btn1 = lv_button_create(page_pic);
    lv_obj_set_size(btn1, 160, 80);
    lv_obj_set_align(btn1, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_t* label1 = lv_label_create(btn1);
    lv_obj_set_align(label1, LV_ALIGN_CENTER);
    lv_label_set_text(label1,"上一张");
    lv_obj_add_style(label1, &style, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn1,pacture_event, LV_EVENT_CLICKED, label1);


    lv_obj_t* btn2 = lv_button_create(page_pic);
    lv_obj_set_size(btn2, 160, 80);
    lv_obj_t* label2 = lv_label_create(btn2);
    lv_obj_set_align(btn2, LV_ALIGN_BOTTOM_RIGHT);
    lv_obj_set_align(label2, LV_ALIGN_CENTER);
    lv_label_set_text(label2,"下一张");
    lv_obj_add_style(label2, &style, LV_STATE_DEFAULT);
    lv_obj_add_event_cb(btn2,pacture_event, LV_EVENT_CLICKED, label2);

    back_prev(page_pic);

    //在活动屏幕上创建一个图片控件 --- 静态控件
	image = lv_image_create(page_pic);
	lv_image_set_src(image, pics[i]);
    lv_obj_set_align(image, LV_ALIGN_CENTER);
}