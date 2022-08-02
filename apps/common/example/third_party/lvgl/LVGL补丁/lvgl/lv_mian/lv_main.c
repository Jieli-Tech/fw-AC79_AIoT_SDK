#include "app_config.h"
#include "device/device.h"//u8
#include "system/includes.h"//GPIO
#include "lcd_drive.h"
#include "sys_common.h"
#include "lcd_te_driver.h"
#include "get_yuv_data.h"
#include "lvgl/lvgl.h"
//#ifdef USE_LVGL_UI_DEMO

extern void lv_init(void);
extern void lv_port_disp_init(void);
extern void lv_port_indev_init(void);
extern void lv_demo_stress(void);
extern void lv_demo_benchmark(void);
extern void lv_demo_music(void);
extern void lv_demo_widgets(void);

static void lv_header_timer_run(void)
{
    lv_tick_inc(1);
}

static void lvgl_main_task(void *priv)
{
    lv_init();
    sys_timer_add(NULL, lv_header_timer_run, 1); //LVGL心跳提供
    lv_port_disp_init(); //显示接口对接
    lv_port_indev_init(); //touch dev

    //lv_demo_stress();
    //lv_demo_benchmark();
    //lv_demo_music();
    lv_demo_widgets();
    while (1) {
        lv_task_handler();
        //os_time_dly(1);
    }
}

static int lvgl_main_task_init(void)
{
    puts("lvgl_main_task_init \n\n");
    return thread_fork("lvgl_main_task", 11, 1024, 0, 0, lvgl_main_task, NULL);
}
late_initcall(lvgl_main_task_init);

//#endif


