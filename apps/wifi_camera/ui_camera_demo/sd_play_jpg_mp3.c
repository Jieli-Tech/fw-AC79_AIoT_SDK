#include "ui/ui.h"
#include "ui_api.h"
#include "system/timer.h"
#include "server/server_core.h"
#include "os/os_api.h"
#include "asm/gpio.h"
#include "system/includes.h"
#include "server/audio_server.h"
#include "storage_device.h"
#include "app_config.h"
#include "font/font_textout.h"
#include "ui/includes.h"
#include "ui_action_video.h"
#include "font/font_all.h"
#include "font/language_list.h"
#include "ename.h"
#include "asm/rtc.h"
#include "lcd_drive.h"
#include "video_rec.h"
#include "yuv_soft_scalling.h"
#include "event/key_event.h"
#include "net_video_rec.h"
#include "asm/jpeg_codec.h"
#include "lcd_te_driver.h"
#include "res_config.h"
#include "lcd_config.h"

/*
 * 该文件用于播放SD卡文件中JPG图片测试
 */

#if CONFIG_PLAY_JPG_ENABLE

#define TEST_JPG_MUN  149

static void camera_show_lcd(u8 *buf, u32 size, int width, int height)
{
    printf(">>>>>>>>>>>>>>camera_show_lcd");

    lcd_show_frame(buf, LCD_YUV420_DATA_SIZE); //240*320*2=153600
}

static u8 jpg_save[1024 * 100];
static void play_jpg_task(void *priv)
{

    user_ui_lcd_init();
    //1.打开jpeg解码YUV

    set_lcd_show_data_mode(camera);
    u8 ret = jpeg2yuv_open();
    if (ret) {
        jpeg2yuv_close();
        printf("[ERROR]  jpeg2yuv_open fail");
        return;
    }
    //2.注册YUV数据回调函数
    jpeg2yuv_yuv_callback_register(camera_show_lcd);

    FILE *fd;
    u8 file_name[64];
    u32 file_len = 0;

    while (1) {
        for (u8 i = 0; i < TEST_JPG_MUN; i++) {
            printf(">>>>>>>>>>i=%d", i);
            sprintf(file_name, RES_PATH"%d.jpg", i);
            printf(">>>>>>>>>file_name=%s", file_name);

            fd = fopen(file_name, "r");
            if (fd == NULL) {
                printf("[error]>>>>>fd= NULL");
                return;
            }
            file_len = flen(fd);
            int rlen = fread(jpg_save, 1, file_len, fd);
            if (file_len != rlen) {
                printf("[error]>>>>>file_len != rlen");
                fclose(fd);
                return;
            }
            jpeg2yuv_jpeg_frame_write(jpg_save, file_len);
            fclose(fd);
        }
    }
}

static int play_jpg_task_init(void)
{
    puts("plplay_jpg_task_initay_jpg_task_init \n\n");
    return thread_fork("play_jpg_task", 11, 1024, 32, 0, play_jpg_task, NULL);
}
late_initcall(play_jpg_task_init);
#endif

