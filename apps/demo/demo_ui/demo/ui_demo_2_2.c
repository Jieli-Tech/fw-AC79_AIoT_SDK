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


#ifdef UI_DEMO_2_2

#define STYLE_NAME  JL
REGISTER_UI_STYLE(STYLE_NAME)

static void double_ui_change_handler(void *priv)
{

    struct element *elm = ui_core_get_element_by_id(BASEFORM_1);
    if (elm == NULL) {
        printf(">>>>>>>>>>>>elm = NULL");
    }

    ui_core_redraw(elm);
    struct element *p;

    list_for_each_child_element(p, elm) {
        struct rect rect;
        ui_core_get_element_abs_rect(p, &rect);
    }
}

static int double_ui_onchange(void *ctr, enum element_change_event e, void *arg)
{
    static u16 timeid = 0;
    static u32 timer = 0;

    switch (e) {
    case ON_CHANGE_INIT: //这种情况去显示插入控件是最好的 这种是在界面没有显示的时候准备好数据然后跟主界面一起显示
        //init后才回去加载主界面
        create_control_by_id(RES_PATH"prj2.tab", PRJ2_PAGE_0, PRJ2_BASEFORM_26, BASEFORM_1);//加载第二UI工程控件
        timeid =  sys_timer_add(NULL, double_ui_change_handler, 500); //每隔半秒刷新一次
        break;

    case ON_CHANGE_RELEASE:
        delete_control_by_id(PRJ2_BASEFORM_26);//删除控件
        sys_timer_del(timeid);//删除定时器
        break;
    default:
        return false;
    }

    return false;
}
REGISTER_UI_EVENT_HANDLER(BASEFORM_1) //ui工程1 布局ID
.onchange = double_ui_onchange,
};



static void double_ui_project_task(void *priv)
{
    user_ui_lcd_init();

    while (1) {
        ui_show_main(PAGE_0);
        os_time_dly(300);
        ui_hide_curr_main();

        ui_show_main(PAGE_1);
        create_control_by_id(RES_PATH"prj2.tab", PRJ2_PAGE_0, PRJ2_BASEFORM_26, BASEFORM_37); //这种加载是显示之后加载需要重新刷新显示
        struct element *parent = ui_core_get_element_by_id(BASEFORM_37);
        ui_core_redraw(parent);//重新刷新显示
        os_time_dly(300);
        ui_hide_curr_main();
    }
}


static int double_ui_project_task_init(void)
{
    puts("double_ui_project_task_init \n\n");
    return thread_fork("double_ui_project_task", 11, 1024, 0, 0, double_ui_project_task, NULL);
}
late_initcall(double_ui_project_task_init);

#endif //DOUBLE_PROJECT_TEST

