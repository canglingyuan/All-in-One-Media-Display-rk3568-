#ifndef __MENU_H
#define __MENU_H

#include "../lvgl/lvgl.h"

void log_Init(void);
void desktop_Init(void);
void app_Init(void);
void set_cn_font(lv_obj_t *label);
void set_cn_font_red(lv_obj_t *label);
void set_cn_font_white(lv_obj_t *label);
void back_event(lv_obj_t* e);
void back_prev(lv_obj_t* parent);

#endif