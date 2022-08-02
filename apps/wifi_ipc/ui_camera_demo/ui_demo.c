#ifdef CONFIG_UI_ENABLE //上电执行则打开app_config.h TCFG_DEMO_UI_RUN = 1

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


extern void camera_to_lcd_init(void);//带TE处理无切线

static void open_animation(char speed)//开机图片以及开机音乐播放
{
    set_lcd_show_data_mode(ui);

    ui_show_main(PAGE_2);

    for (u8 i = 0; i < 17; i++) {
        ui_pic_show_image_by_id(BASEFORM_19, i);
        os_time_dly(speed);
    }

    ui_hide_main(PAGE_2);
}

static void ui_demo(void *priv)
{
    user_ui_lcd_init();//初始化ui服务和lcd
    open_animation(1);//开机动画 20为延时 200ms每帧

    set_lcd_show_data_mode(camera);
    camera_to_lcd_init();
}

static int ui_demo_task_init(void)
{
    puts("ui_demo_task_init \n\n");
    return thread_fork("ui_demo", 11, 1024 * 2, 32, 0, ui_demo, NULL);
}
late_initcall(ui_demo_task_init);
#endif
