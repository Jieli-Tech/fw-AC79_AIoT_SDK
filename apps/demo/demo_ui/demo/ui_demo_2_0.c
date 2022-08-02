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
#include "ui/ui.h"
#include "ui_api.h"
#include "ui/includes.h"
#include "font/font_all.h"
#include "font/font_textout.h"
#include "font/language_list.h"

#ifdef UI_DEMO_2_0


#define STYLE_NAME  JL
REGISTER_UI_STYLE(STYLE_NAME)


static u32 time_1 = 0;

static void battery_change_handler(void *priv)
{
    static u8 percent = 0;
    struct unumber timer;
    static u16 show_nub = 0;
    putchar('B');
    ui_battery_level_change(percent, 0);
    percent += 10;

    if (percent >= 90) {
        percent = 0;
    }
    show_nub++;
    timer.numbs = 2;
    timer.type = TYPE_NUM;
    timer.number[0] = percent;
    ui_number_update_by_id(BASEFORM_3, &timer);

    static char str1[] = "%";
    ui_text_set_textu_by_id(BASEFORM_4, str1, strlen(str1), FONT_DEFAULT | FONT_SHOW_MULTI_LINE);
}

static int battery_onchange(void *ctr, enum element_change_event e, void *arg)
{
    struct ui_battery *battery = (struct ui_battery *)ctr;
    static u32 timer = 0;

    switch (e) {
    case ON_CHANGE_INIT:
        if (!time_1) {
            time_1 = sys_timer_add(NULL, battery_change_handler, 1000); //开启进入动态电池电量显示测试
        }

        break;

    case ON_CHANGE_RELEASE:
        if (time_1) {
            sys_timer_del(time_1);
            time_1 = 0;
        }

        break;

    default:
        return false;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(BASEFORM_2)
.onchange = battery_onchange,
};

static void dot_control(char mub)
{
    switch (mub) {
    case 0:
        ui_pic_show_image_by_id(BASEFORM_9, 0);
        ui_pic_show_image_by_id(BASEFORM_5, 1);
        ui_pic_show_image_by_id(BASEFORM_6, 0);
        break;

    case 1:
        ui_pic_show_image_by_id(BASEFORM_5, 0);
        ui_pic_show_image_by_id(BASEFORM_6, 1);
        break;

    case 2:
        ui_pic_show_image_by_id(BASEFORM_6, 0);
        ui_pic_show_image_by_id(BASEFORM_7, 1);
        break;

    case 3:
        ui_pic_show_image_by_id(BASEFORM_7, 0);
        ui_pic_show_image_by_id(BASEFORM_8, 1);
        break;

    case 4:
        ui_pic_show_image_by_id(BASEFORM_8, 0);
        ui_pic_show_image_by_id(BASEFORM_9, 1);
        break;
    }
}

static void main_page_test_task(void *priv)
{
    user_ui_lcd_init();
    ui_show_main(PAGE_0);
    os_time_dly(100);

    u8 mub = 0;
    static s32 x = 0;
    static s32 y = 0;

    dot_control(0);
    struct ui_touch_event e = {0};
    e.event = ELM_EVENT_TOUCH_DOWN;
    e.x = 0;
    e.y = 50;
    ui_touch_msg_post(&e);
    printf(">>>>>>>>>>>>>>>touch_msg_post");

    while (1) {

        //模拟触摸动作
        x -= 1;
        e.event = ELM_EVENT_TOUCH_MOVE;
        e.x = 0 + x;
        e.y = 50;

        ui_touch_msg_post(&e);

        if (!(-x % 128)) {
            mub++	;
            dot_control(mub);
            os_time_dly(100);

            if (mub == 4) {
                break;
            }
        }
        os_time_dly(2);
    }
}

static int main_page_test_task_init(void)
{
    puts("main_page_test_task_init \n\n");
    return thread_fork("main_page_test_task", 11, 1024, 32, 0, main_page_test_task, NULL);
}
late_initcall(main_page_test_task_init);

#endif
