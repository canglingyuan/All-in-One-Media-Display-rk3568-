#include "menu.h"
#include "../lvgl/lvgl.h"
#include <stdio.h>
#include <string.h>

extern lv_font_t Ch_make;
extern int page_flag;
extern lv_obj_t* page_menu;

static int i=0;
static lv_obj_t * page_pic ;
static lv_obj_t * image ;
static int build=0;
#define IMG_MAX (sizeof(pics)/sizeof(pics[0]))

char* pics[]= {
    "A:/workpace/pic/1.bmp",
    "A:/workpace/pic/2.bmp",
    "A:/workpace/pic/3.bmp",
    "A:/workpace/pic/4.bmp",
    "A:/workpace/pic/5.bmp",
    };

/* ── 记录按下坐标 ── */
static int start_x = 0, start_y = 0;

static void on_press(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_active();
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    start_x = p.x;
    start_y = p.y;
    printf("press at %d, %d\n", start_x, start_y);
}

static void on_release(lv_event_t *e)
{
    lv_indev_t *indev = lv_indev_active();
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    int dx = p.x - start_x;
    printf("release at %d, %d  dx=%d\n", p.x, p.y, dx);

    if (dx < -50)
        i = (i - 1 + IMG_MAX) % IMG_MAX;    // 右滑 → 上一张
    else if (dx > 50)
        i = (i + 1) % IMG_MAX;              // 左滑 → 下一张
    else return;

    lv_image_set_src(image, pics[i]);
}

/* ================================================================
 *  构建总页面
 * ================================================================ */
void album(void)
{
    page_pic=lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_pic,1024,600);
    lv_obj_set_align(page_pic,LV_ALIGN_CENTER);

    page_flag=-1;
    back_prev(page_pic);

    /* 图片控件 */
    image = lv_image_create(page_pic);
    lv_image_set_src(image, pics[i]);
    lv_obj_set_align(image, LV_ALIGN_CENTER);

    /* ── 图片不参与触摸 ── */
    lv_obj_add_flag(image, LV_OBJ_FLAG_EVENT_BUBBLE);   // 只冒泡，不 CLICKABLE

    /* ── page_pic 收所有事件 ── */
    lv_obj_add_flag(page_pic, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(page_pic, LV_OPA_COVER, 0);
    lv_obj_add_event_cb(page_pic, on_press,   LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(page_pic, on_release, LV_EVENT_RELEASED, NULL);
}

/* =======================================
* 进入
* ========================================*/
static void pic_event_handle(lv_event_t* e)
{
    build=0;
    if(!build)
    {
        album();
        build=1;
    }
    lv_obj_add_flag(page_menu, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(page_pic, LV_OBJ_FLAG_HIDDEN);
}

// 相册图标
void pic_icon(void)
{
     if(page_pic) lv_obj_add_flag(page_pic, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* btn_pic = lv_button_create(page_menu);
    lv_obj_set_size(btn_pic, 120, 120);
    lv_obj_align(btn_pic, LV_ALIGN_LEFT_MID,30,0);

    lv_obj_t* label_pic = lv_label_create(btn_pic);
    lv_label_set_text(label_pic, "相册");
    lv_obj_set_parent(label_pic, lv_obj_get_parent(btn_pic)); // 移到同一父容器
    lv_obj_align_to(label_pic, btn_pic, LV_ALIGN_OUT_BOTTOM_MID, -20, 0);
    // lv_obj_set_align(label_pic, LV_ALIGN_OUT_BOTTOM_MID);
    set_cn_font(label_pic);

    lv_obj_add_event_cb(btn_pic, pic_event_handle, LV_EVENT_CLICKED, NULL);
}