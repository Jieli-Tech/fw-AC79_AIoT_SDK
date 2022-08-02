#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "lcd_drive.h"

#ifdef USE_GIF_PLAY_DEMO

extern void play_gif_to_lcd(char *gif_path, char play_speed);

static void gif_play_test_init(void *priv)
{
    user_lcd_init();

    while (1) {
        play_gif_to_lcd(CONFIG_UI_PATH_FLASH"gif.gif", 0); //0使用默认GIF延时  >1为延时为10*i ms 每张动图的延时间隔
        os_time_dly(200);
    }
}

static int gif_play_test_task_init(void)
{
    puts("gif_play_test_task_init \n\n");
    return thread_fork("gif_play_test_init", 11, 1024, 0, 0, gif_play_test_init, NULL);
}

late_initcall(gif_play_test_task_init);

#endif
