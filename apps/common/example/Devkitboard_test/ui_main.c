#include "app_config.h"

#ifdef USE_DevKitBoard_TEST_DEMO

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
#include "wifi/wifi_connect.h"

#include "ename.h"
#include "res_config.h"
#include "ui/ui.h"
#include "ui/ui_core.h"
#include "ui_api.h"
#include "ui/includes.h"
#include "font/font_all.h"
#include "font/font_textout.h"
#include "font/language_list.h"
#include "lcd_te_driver.h"//set_lcd_show_data_mode


#define POST_TASK_DevKitBoard "DevKitBoard_task"
extern void post_msg_play_flash_mp3(char *file_name, u8 dec_volume);
static u8 ui_show_text_flog = 0;
/*==================开机播提示音已经开机动画==================*/
static void open_animation(char speed)//开机图片以及开机音乐播放
{
    set_lcd_show_data_mode(ui);

    post_msg_play_flash_mp3("poweron.mp3", 100); //开机提示音
    ui_show_main(PAGE_0);

    for (u8 i = 0; i < 4; i++) {
        printf("i=%d.\n", i);
        ui_pic_show_image_by_id(BASEFORM_2, i);
        os_time_dly(speed);
    }

    ui_hide_main(PAGE_0);
}
/*************************END**********************************/

/*======================PAGE_1================================*/
static lise_show_text(void *priv)
{
    static char str1[] = "开源板综合测试";
    static char str2[] = "按键测试";
    static char str3[] = "DVP摄像头测试";
    static char str4[] = "SPI摄像头测试";
    static char str5[] = "MIC测试";
    static char str6[] = "音频输入测试(linein)";
    static char str7[] = "网络播放测试";
    static char str8[] = "蓝牙播放测试";
    ui_text_set_textu_by_id(BASEFORM_5, str1, strlen(str1), FONT_DEFAULT);
    ui_text_set_textu_by_id(BASEFORM_14, str2, strlen(str2), FONT_DEFAULT);
    ui_text_set_textu_by_id(BASEFORM_15, str3, strlen(str3), FONT_DEFAULT);
    ui_text_set_textu_by_id(BASEFORM_16, str4, strlen(str4), FONT_DEFAULT);
    ui_text_set_textu_by_id(BASEFORM_17, str5, strlen(str5), FONT_DEFAULT);
    ui_text_set_textu_by_id(BASEFORM_18, str6, strlen(str6), FONT_DEFAULT);
    ui_text_set_textu_by_id(BASEFORM_19, str7, strlen(str7), FONT_DEFAULT);
    ui_text_set_textu_by_id(BASEFORM_20, str8, strlen(str8), FONT_DEFAULT);
}
/*page1 垂直列表初始化*/
static int list_onchange(void *ctr, enum element_change_event event, void *arg)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    int row, col;
    switch (event) {
    case ON_CHANGE_INIT:
        row = 7;
        col = 1;
        ui_grid_init_dynamic(grid, &row, &col);//初始化垂直列表的个数用于判断触摸行号使用
        ui_grid_set_slide_direction(grid, SCROLL_DIRECTION_UD);//限制控件滑动方向
        sys_timeout_add(NULL, lise_show_text, 50);

        break;
    }

    return false;
}
/*page1 触摸事件响应*/
static int list_ontouch(void *ctr, struct element_touch_event *e)
{
    int indx;
    int hi_indx;
    static u8 list_touch_index;
    static u8 move_flog;
    struct ui_grid *grid = (struct ui_grid *)ctr;
    struct element *elm;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        move_flog = 0;
        indx = ui_grid_cur_item_dynamic(grid);
        list_touch_index = indx;
        /*printf(">>>>>>>>>ELM_EVENT_TOUCH_DOWN_index = %d", list_touch_index);*/
        break;
    case ELM_EVENT_TOUCH_UP:
        indx = ui_grid_cur_item_dynamic(grid);
        if (!move_flog && list_touch_index == indx) { //如果没有移动并且按下和抬起都是同一个地方表示按键按下
            hi_indx = ui_grid_get_hindex(grid);//获取底层控制的高亮行号
            printf("<<<<<<<<<<touch hi_indx=%d", hi_indx);
            switch (hi_indx) { //取消系统高亮
            case 0:
                elm = ui_core_get_element_by_id(LIST_1);
                break;
            case 1:
                elm = ui_core_get_element_by_id(LIST_2);
                break;
            case 2:
                elm = ui_core_get_element_by_id(LIST_3);
                break;
            case 3:
                elm = ui_core_get_element_by_id(LIST_4);
                break;
            case 4:
                elm = ui_core_get_element_by_id(LIST_5);
                break;
            case 5:
                elm = ui_core_get_element_by_id(LIST_6);
                break;
            case 6:
                elm = ui_core_get_element_by_id(LIST_7);
                break;
            }
            ui_core_highlight_element(elm, 0);//改变控件高亮状态
            /*printf("<<<<<<<<<<touch ok indx=%d", list_touch_index);	*/
            switch (list_touch_index) { //高亮选择
            case 0:
                elm = ui_core_get_element_by_id(LIST_1);
                break;
            case 1:
                elm = ui_core_get_element_by_id(LIST_2);
                break;
            case 2:
                elm = ui_core_get_element_by_id(LIST_3);
                break;
            case 3:
                elm = ui_core_get_element_by_id(LIST_4);
                break;
            case 4:
                elm = ui_core_get_element_by_id(LIST_5);
                break;
            case 5:
                elm = ui_core_get_element_by_id(LIST_6);
                break;
            case 6:
                elm = ui_core_get_element_by_id(LIST_7);
                break;
            }
            ui_core_highlight_element(elm, 1);
            ui_core_redraw(elm->parent);
            os_time_dly(10);

            ui_hide_main(PAGE_1);
            switch (list_touch_index) { //根据选择切界面
            case 0://按键测试
                ui_show_main(PAGE_2);
                break;
            case 1://DVP测试
                ui_show_main(PAGE_3);
                break;
            case 2://spi测试
                ui_show_main(PAGE_10);
                ui_show_text_flog = 1;
                break;
            case 3://MIC测试
                ui_show_main(PAGE_4);
                break;
            case 4://linein测试
                ui_show_main(PAGE_5);
                break;
            case 5://wifi播歌测试
                ui_show_main(PAGE_9); //进入wifi信息显示
                break;
            case 6://蓝牙播歌测试
                ui_show_main(PAGE_10);
                ui_show_text_flog = 0;
                break;
            }
        }
        /*printf(">>>>>>>>>ELM_EVENT_TOUCH_UP_index = %d", list_touch_index);*/
        break;
    case ELM_EVENT_TOUCH_MOVE:
        if (!move_flog) {
            printf("<<<<<<<<<<<<<<<<<ELM_EVENT_TOUCH_MOVE");
        }
        move_flog = 1;
        break;
    case ELM_EVENT_TOUCH_R_MOVE:
        break;
    }
    return false;//不接管消息
}
REGISTER_UI_EVENT_HANDLER(LIST)
.onchange = list_onchange,
 .ontouch = list_ontouch,
};
/***********************PAGE_1_END**************************/


/*======================PAGE_2===按键测试==========================*/
static key_show_text(void *priv)
{
    static char str1[] = "返回主界面";
    static char str2[] = "按键测试-请按按键";
    ui_text_set_textu_by_id(BASEFORM_24, str1, strlen(str1), FONT_DEFAULT);
    ui_text_set_textu_by_id(BASEFORM_7, str2, strlen(str2), FONT_DEFAULT);
}
/*page2 返回按钮初始化*/
static int key_test_onchange(void *ctr, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        sys_timeout_add(NULL, key_show_text, 50);
        break;
    }
    return false;
}
/*page2 触摸事件响应*/
static int key_test_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        ui_hide_main(PAGE_2);
        ui_show_main(PAGE_1);
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_24)//返回按钮
.onchange = key_test_onchange,
 .ontouch = key_test_ontouch,
};
/***********************PAGE_2_END*按键测试*************************/

/*======================PAGE_3===DVP摄像头出图测试=================*/
/*page3 初始化*/
static int dvp_test_onchange(void *ctr, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        set_lcd_show_data_mode(ui_camera);//图像合成模式
        os_taskq_post(POST_TASK_DevKitBoard, 1, ACTION_MSG_DEP_CAMERA_OPEN);
        break;
    }
    return false;
}
/*page3 触摸事件响应*/
static int dvp_test_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        set_lcd_show_data_mode(ui);//ui模式
        os_taskq_post(POST_TASK_DevKitBoard, 1, ACTION_MSG_DEP_CAMERA_CLOSE);
        ui_hide_main(PAGE_3);
        ui_show_main(PAGE_1);
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_8)//返回按钮
.onchange = dvp_test_onchange,
 .ontouch = dvp_test_ontouch,
};

/***********************PAGE_3_END*DVP摄像头出图测试****************/

/*======================PAGE_4===mic测试================*/
/*page4 初始化*/
static int mic_test_onchange(void *ctr, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        os_taskq_post(POST_TASK_DevKitBoard, 1, ACTION_MSG_MIC_OPEN);
        break;
    }
    return false;
}
/*page4 触摸事件响应*/
static int mic_test_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        os_taskq_post(POST_TASK_DevKitBoard, 1, ACTION_MSG_MIC_CLOSE);
        ui_hide_main(PAGE_4);
        ui_show_main(PAGE_1);
        break;
    }
    return false;
}

REGISTER_UI_EVENT_HANDLER(BASEFORM_11)//返回按钮
.onchange = mic_test_onchange,
 .ontouch = mic_test_ontouch,
};
/***********************PAGE_4_END*mic测试****************/

/*======================PAGE_5===linein测试===============*/
/*page5 初始化*/
static int linein_test_onchange(void *ctr, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        os_taskq_post(POST_TASK_DevKitBoard, 1, ACTION_MSG_LINEIN_OPEN);
        break;
    }
    return false;
}
/*page5 触摸事件响应*/
static int linein_test_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        os_taskq_post(POST_TASK_DevKitBoard, 1, ACTION_MSG_LINEIN_CLOSE);
        ui_hide_main(PAGE_5);
        ui_show_main(PAGE_1);
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(BASEFORM_23)//返回按钮
.onchange = linein_test_onchange,
 .ontouch = linein_test_ontouch,
};
/***********************PAGE_5_END*linein测试****************/

/*======================PAGE_9===wifi连接信息显示=================*/
static wifi_list_text(void *priv)
{
    struct wifi_mode_info info;
    static char str1[] = "请使用手机开热点配置连接";
    static char wifi_ssid[64];
    static char wifi_pwd[64];
    struct wifi_store_info *show_info;
    show_info = get_cur_wifi_info();
    printf("<<<<<<<<<<wifi_ssid=%s, wifi_pwd=%s", show_info->ssid, show_info->pwd);

    sprintf((char *)wifi_ssid, "wifi名字: ");
    sprintf((char *)wifi_pwd, "wifi密码: ");

    strncat((char *)wifi_ssid, (const char *)show_info->ssid, sizeof(show_info->ssid));
    strncat((char *)wifi_pwd, (const char *)show_info->pwd, sizeof(show_info->pwd));

    ui_text_set_textu_by_id(BASEFORM_48, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);

    ui_text_set_textu_by_id(BASEFORM_49, wifi_ssid, strlen(wifi_ssid), FONT_DEFAULT);
    ui_text_set_textu_by_id(BASEFORM_50, wifi_pwd, strlen(wifi_pwd), FONT_DEFAULT);

}
/*page9 初始化*/
static int wifi_get_ssid_onchange(void *ctr, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        os_taskq_post(POST_TASK_DevKitBoard, 1, ACTION_MSG_GET_WIFI_SSID);
        sys_timeout_add(NULL, wifi_list_text, 50);
        break;
    }
    return false;
}
/*page9 触摸事件响应*/
static int wifi_get_ssid_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        os_taskq_post(POST_TASK_DevKitBoard, 1, ACTION_MSG_SCAN_WIFI);
        ui_hide_main(PAGE_9);
        ui_show_main(PAGE_1);
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(BASEFORM_47)//返回按钮
.onchange = wifi_get_ssid_onchange,
 .ontouch = wifi_get_ssid_ontouch,
};

/***********************PAGE_9_END*连接wifi****************/

//网络播歌需要先扫描wifi连接有网络的wifi输入正确得的密码进行连接wifi 然后会自动播歌

/*======================PAGE_8===播歌界面=================*/
/*page9 触摸事件响应*/
extern void wifi_music_end(void);
static int music_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;

    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        ui_hide_main(PAGE_8);
        ui_show_main(PAGE_1);
        post_msg_play_flash_mp3("NetDisc.mp3", 100);
        wifi_music_end();
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(BASEFORM_43)//返回按钮
.ontouch = music_ontouch,
};

/***********************PAGE_8_END*播歌界面****************/

/*======================PAGE_10====================*/
const char *bt_get_local_name(void);

static void bt_name_text(void *priv)
{
    char *bt_name;
    static u8 cpy_flog = 0;
    static char str1[] = "连接蓝牙";
    static char str2[] = "SPI_camera开发中敬请期待";

    if (ui_show_text_flog) {
        ui_text_set_textu_by_id(BASEFORM_53, str2, strlen(str2), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
    } else {
        if (!cpy_flog) {
            cpy_flog = 1;
            bt_name = bt_get_local_name();
            printf(">>>>>>>>>bt_name=%s", bt_name);
            strncat((char *)str1, (const char *)bt_name, 64);
        }
        ui_text_set_textu_by_id(BASEFORM_53, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
    }
}
/*page10 初始化*/
static int bt_music_onchange(void *ctr, enum element_change_event event, void *arg)
{
    switch (event) {
    case ON_CHANGE_INIT:
        sys_timeout_add(NULL, bt_name_text, 50);
        break;
    }
    return false;
}
/*page10 触摸事件响应*/
static int bt_music_ontouch(void *ctr, struct element_touch_event *e)
{
    struct ui_grid *grid = (struct ui_grid *)ctr;
    switch (e->event) {
    case ELM_EVENT_TOUCH_DOWN:
        ui_hide_main(PAGE_10);
        ui_show_main(PAGE_1);
        break;
    }
    return false;
}
REGISTER_UI_EVENT_HANDLER(BASEFORM_53)//返回按钮
.onchange = bt_music_onchange,
 .ontouch = bt_music_ontouch,
};
/***********************PAGE_10_END*****************/

/*======================PAGE_====================*/

/***********************PAGE__END*****************/
/*======================测试线程接收消息处理UI==========================*/
static void ui_main_task(void *priv)
{
    int msg[32];
    int play_all_time;
    int play_time;
    int per;
    int time[32];
    static char str1[] = "网络连接成功";
    user_ui_lcd_init();
    open_animation(100); //执行开机图片
    ui_show_main(PAGE_1);
    extern void wifi_music_start(void);
    while (1) {
        os_taskq_pend("taskq", msg, ARRAY_SIZE(msg)); //接收app_music.c中发来的消息 没有消息在这行等待
        printf(">>>>>>>>>>>>>>ui_main_task have msg=%d", msg[1]);
        switch (msg[1]) {
        case UI_MSG_KEY_TEST://显示按键按中的图片
            if (ui_get_current_window_id() == PAGE_2) {
                ui_pic_show_image_by_id(BASEFORM_6, msg[2]);
            }
            break;
        case UI_MSG_WIFI_STA_CONNT_OK://sta连接成功
            post_msg_play_flash_mp3("NetMusic.mp3", 100);
            ui_text_set_textu_by_id(BASEFORM_48, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
            os_time_dly(150);
            ui_hide_main(PAGE_9);
            ui_show_main(PAGE_8);
            wifi_music_start();
            break;
        case UI_MSG_PLEAY_ALL_TIME://获取播放总时间
            play_all_time = msg[2];
            sprintf((char *)time, "%d%d:%d%d", play_all_time / 60 / 10, play_all_time / 60 % 10, play_all_time % 60 / 10, play_all_time % 10);
            printf(">>>>>>>>all_time = %s", time);
            ui_text_set_textu_by_id(BASEFORM_45, time, strlen(time), FONT_DEFAULT);
            break;
        case UI_MSG_PLEAY_TIME://获取播放时间
            play_time = msg[2];
            sprintf((char *)time, "%d%d:%d%d", play_time / 60 / 10, play_time / 60 % 10, play_time % 60 / 10, play_time % 10);
            ui_text_set_textu_by_id(BASEFORM_44, time, strlen(time), FONT_DEFAULT);

            per = play_time * 100 / play_all_time;

            ui_slider_set_persent_by_id(NEWLAYOUT_21, per);
            if (play_time == (play_all_time - 1)) {
                os_time_dly(120);
                wifi_music_end();
            }
            break;
        case UI_MSG_BT_CONNT_OK://蓝牙连接成功

            break;
        case UI_MSG_BT_DISCONNECT://蓝牙断开连接

            break;
        }
    }
}

static int ui_main_task_init(void)
{
    puts("ui_main_task_init \n\n");
    return thread_fork("ui_main_task", 11, 1024, 32, 0, ui_main_task, NULL);
}
late_initcall(ui_main_task_init);

#endif

