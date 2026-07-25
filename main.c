#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "./user/menu.h"

extern int key_flag;

int main(void)
{
    lv_init();

    /*Linux frame buffer device init*/
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    /*Create a Demo*/
    lv_indev_t * indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event6");
    // lv_demo_widgets();
    // lv_demo_widgets_start_slideshow();
    
    log_Init();
    // if(key_flag==1)
    // {
    //     desktop_Init();
    // }

    /*Handle LVGL tasks*/
    while(1) {
        lv_timer_handler();
        usleep(5000);

        if(key_flag == 1)
        {
            desktop_Init();
            key_flag = 2;   // 标记已初始化
        }
    }

    return 0;
}
