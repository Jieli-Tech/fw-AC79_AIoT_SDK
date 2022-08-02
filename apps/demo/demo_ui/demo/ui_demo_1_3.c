#include "app_config.h"
#include "device/device.h"//u8
#include "server/video_dec_server.h"//fopen
#include "system/includes.h"//GPIO
#include "lcd_drive.h"
#include "sys_common.h"
#include "yuv_soft_scalling.h"
#include "asm/jpeg_codec.h"
#include "lcd_te_driver.h"
#include "system/timer.h"
#include "get_yuv_data.h"

#include "ename.h"
#include "res_config.h"
#include "ui/ui.h"
#include "ui_api.h"
#include "ui/includes.h"
#include "font/font_all.h"
#include "font/font_textout.h"
#include "font/language_list.h"
#include "GUI_GIF_Private.h"


#ifdef UI_DEMO_1_3


extern void play_gif_to_lcd(char *gif_path, char play_speed);

static void double_ui_project_task(void *priv)
{
    user_lcd_init(); //单纯屏幕初始化 不带UI
    while (1) {
        play_gif_to_lcd(CONFIG_UI_PATH_FLASH"gif.gif", 0); //0使用默认GIF延时  >1为延时为10*i ms 每张动图的延时间隔
    }
}


static int double_ui_project_task_init(void)
{
    puts("double_ui_project_task_init \n\n");
    return thread_fork("double_ui_project_task", 11, 1024, 0, 0, double_ui_project_task, NULL);
}
late_initcall(double_ui_project_task_init);

#endif
