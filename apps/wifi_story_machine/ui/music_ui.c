#include "ui/ui.h"
#include "ui_api.h"
#include "system/timer.h"
#include "server/server_core.h"
#include "os/os_api.h"
#include "asm/gpio.h"
#include "system/includes.h"
#include "server/audio_server.h"
#include "storage_device.h"
#include "font/font_textout.h"
#include "ui/includes.h"
#include "ui_action_video.h"
#include "font/font_all.h"
#include "font/language_list.h"
#include "ename.h"
#include "asm/rtc.h"
#include "lcd_drive.h"
#include "yuv_soft_scalling.h"
#include "event/key_event.h"
#include "lcd_te_driver.h"
#include "app_music.h"

/********************************文件说明************************************
 *  功能说明：
 *      该文件实现了 app_music.c中 SD卡播放时的ui显示
 *      需要通过按键切换到SD卡播歌模式
 */

#define UI_ID_SD_LOG             BASEFORM_3
#define UI_ID_ALL_TIME           BASEFORM_5
#define UI_ID_CURRENT_TIME       BASEFORM_6
#define UI_ID_LAST_SONG          BASEFORM_9
#define UI_ID_NEXT_SONG          BASEFORM_8
#define UI_ID_PAUSE              BASEFORM_10
#define UI_ID_VOLUME_NUMB        BASEFORM_12
#define UI_ID_SONG_NAME          BASEFORM_15
#define UI_ID_STARTUP_LOG        BASEFORM_19
#define UI_ID_PROGRESS_BAR       NEWLAYOUT_21

#define UI_UPDATA_TEXT           BASEFORM_21
#define UI_UPDATA_PROGRESS_BAR   NEWLAYOUT_22
#define UI_UPDATA_NUMB           BASEFORM_20
#define UI_UPDATA_PER            BASEFORM_22


struct ui_play_mp3_hdl {
    int play_new_time;
    int play_all_time;
    u8 progress_bar_num;
};

static struct ui_play_mp3_hdl ui_play_mp3_handler;

#define __this 	(&ui_play_mp3_handler)

char ui_show_mp3_name[64] = {0};


/*==========ui_MP3播放歌名显示==========*/
static void ui_play_mp3_name(u8 *name)
{
    ui_text_set_textu_by_id(UI_ID_SONG_NAME, name, strlen(name), FONT_DEFAULT);//utf8显示 不能显示中文 会乱码
    /*ui_text_set_textw_by_id(UI_ID_SONG_NAME, name, strlen(name), FONT_ENDIAN_SMALL, FONT_DEFAULT);*///utf16显示 //不能显示英文会乱码
}

/*==========ui_高亮控制函数=============*/
static void form_highlight(u32 id, int status)
{
    struct element *elm = ui_core_get_element_by_id(id);
    ui_core_highlight_element(elm, status);
    ui_core_redraw(elm->parent);
}

/*==========ui_MP3播放上一首============*/
static void ui_play_last_mp3(void)
{
    form_highlight(UI_ID_LAST_SONG, 1);
    os_time_dly(50);
    form_highlight(UI_ID_LAST_SONG, 0);
}

/*==========ui_MP3播放开关控制==============*/
static void ui_play_pause_mp3(int status)
{
    form_highlight(UI_ID_PAUSE, status);
}

/*==========ui_MP3播放下一首============*/
static void ui_play_next_mp3(void)
{
    form_highlight(UI_ID_NEXT_SONG, 1);
    os_time_dly(50);
    form_highlight(UI_ID_NEXT_SONG, 0);
}

/*==========ui_MP3播放进度条============*/
static void ui_play_mp3_progress_bar(int num)
{
    ui_slider_set_persent_by_id(UI_ID_PROGRESS_BAR, num);
}

/*==========ui_MP3播放总时间显示========*/
static void ui_play_mp3_all_time(int time)
{
    struct utime time_r;

    time_r.min = time / 60;
    time_r.sec = time % 60;
    ui_time_update_by_id(UI_ID_ALL_TIME, &time_r);
}

/*==========ui_MP3播放当前时间显示======*/
static void ui_play_mp3_current_time(int time)
{
    struct utime time_r;

    time_r.min = time / 60;
    time_r.sec = time % 60;
    ui_time_update_by_id(UI_ID_CURRENT_TIME, &time_r);
}

/*==========ui_MP3播放音量显示==========*/
static void ui_play_volume_show(char numb)
{
    struct unumber timer;

    timer.type = TYPE_NUM;
    timer.numbs = 3;
    timer.number[0] =  numb;
    ui_number_update_by_id(UI_ID_VOLUME_NUMB, &timer);
}

/*==========ui_MP3播放SD卡显示==========*/
static void ui_play_SD_show(char status)
{
    ui_pic_show_image_by_id(UI_ID_SD_LOG, status);
}

/*==========ui_MP3播放电池电量显示======*/
static void ui_play_power_show(char percent)
{
    ui_battery_level_change(percent, 0);
}

/*==========ui_MP3播放开机画面==========*/
static void open_animation(char speed)//开机图片以及开机音乐播放
{
    set_lcd_show_data_mode(ui);
    ui_show_main(PAGE_1);

    for (u8 i = 0; i < 4; i++) {
        printf("i=%d.\n", i);
        ui_pic_show_image_by_id(UI_ID_STARTUP_LOG, i);
        os_time_dly(speed);
    }

    ui_hide_main(PAGE_1);
}

/*==========ui_MP3 SD卡升级UI==========*/
static void ui_update_play(void)
{
    ui_hide_curr_main();
    ui_show_main(PAGE_2);

    char str1[] = "升级中请稍后";
    char str2[] = "%";

    ui_text_set_textu_by_id(UI_UPDATA_TEXT, str1, strlen(str1), FONT_DEFAULT);
    ui_text_set_textu_by_id(UI_UPDATA_PER, str2, strlen(str2), FONT_DEFAULT);

    struct unumber timer;
    int numb = 55;
    timer.type = TYPE_NUM;
    timer.numbs = 3;
    timer.number[0] =  0;
    ui_number_update_by_id(UI_UPDATA_NUMB, &timer);

    int num = 0;
    ui_slider_set_persent_by_id(UI_UPDATA_PROGRESS_BAR, num);
}
static void ui_update_persent_show(u8 numb)
{
    struct unumber timer;
    timer.type = TYPE_NUM;
    timer.numbs = 3;
    timer.number[0] =  numb;
    ui_number_update_by_id(UI_UPDATA_NUMB, &timer);

    ui_slider_set_persent_by_id(UI_UPDATA_PROGRESS_BAR, numb);
}
static void ui_update_fail(void)
{
    char str1[] = "升级失败请检查错误原因";
    ui_text_set_textu_by_id(UI_UPDATA_TEXT, str1, strlen(str1), FONT_DEFAULT);
}

/*==========ui_应用初始化部分===========*/
static void ui_server_init(void)
{
    extern const struct ui_devices_cfg ui_cfg_data;
    int lcd_ui_init(void *arg);
    lcd_ui_init(&ui_cfg_data);
}

static void ui_demo(void *priv)
{
    int msg[32];

    set_lcd_show_data_mode(ui);
    ui_server_init();
    open_animation(10);
    ui_show_main(PAGE_0);

    while (1) {
        os_taskq_pend("taskq", msg, ARRAY_SIZE(msg)); //接收app_music.c中发来的消息 没有消息在这行等待

        switch (msg[1]) {
        case UI_MSG_LAST_SONG://上一首
            ui_play_pause_mp3(1);
            ui_play_last_mp3();
            break;

        case UI_MSG_PAUSE_START://播放开始
            ui_play_pause_mp3(1);
            break;

        case UI_MSG_PAUSE_STOP://播放停止
            ui_play_pause_mp3(0);
            break;

        case UI_MSG_NEXT_SONG://下一首
            ui_play_pause_mp3(1);
            ui_play_next_mp3();
            break;

        case UI_MSG_SET_VOLUME://设置音量
            ui_play_volume_show(msg[2]);
            break;

        case UI_MSG_ALL_TIME://播放总时间
            __this->play_all_time =  msg[2];
            ui_play_mp3_all_time(msg[2]);
            break;

        case UI_MSG_CURRENT_TIME://当前播放时间
            __this->play_new_time =  msg[2];
            ui_play_mp3_current_time(msg[2]);
            __this->progress_bar_num = (__this->play_new_time * 100) / __this->play_all_time;
            ui_play_mp3_progress_bar(__this->progress_bar_num);//显示进度条
            break;

        case UI_MSG_SONG_NAME://显示歌曲名字
            ui_play_mp3_name(ui_show_mp3_name);
            break;

        case UI_MSG_SD_LOG://SD状态
            ui_play_SD_show(msg[2]);
            break;

        case UI_MSG_POWER_LOG://电池电量
            ui_play_power_show(msg[2]);
            break;

        case UI_MSG_UPDATE://显示升级界面
            ui_update_play();
            break;

        case UI_MSG_SHOW_PERSENT://升级过程中的百分比显示
            ui_update_persent_show(msg[2]);
            break;

        case UI_MSG_UPGRADE_FAIL://升级过程中的百分比显示
            ui_update_persent_show(msg[2]);
            break;
        }
    }
}

int ui_demo_task_init(void)
{
    puts("ui_demo_task_init \n\n");
    return thread_fork("ui_demo", 11, 1024, 32, 0, ui_demo, NULL);
}
late_initcall(ui_demo_task_init);

