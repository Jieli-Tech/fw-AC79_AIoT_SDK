#include "app_config.h"
#include "includes.h"
#include "ui/ui_api.h"
#include "clock_cfg.h"

#if (TCFG_UI_ENABLE&&(CONFIG_UI_STYLE == STYLE_JL_LED7))
#define UI_DEBUG_ENABLE
#ifdef UI_DEBUG_ENABLE
#define ui_debug(fmt, ...) 	printf("[UI] "fmt, ##__VA_ARGS__)
#define ui_log(fmt, ...) 	printf("[UI] "fmt, ##__VA_ARGS__)
#define ui_error(fmt, ...) 	printf("[UI error] "fmt, ##__VA_ARGS__)
#else
#define ui_debug(...)
#define ui_log(...)
#define ui_error(...)
#endif

#define UI_NO_ARG (-1)
#define UI_TASK_NAME "ui"



struct ui_display_env {
    u8 init;
    OS_SEM sem;//用来做模式初始化的信号量
    u16 tmp_menu_ret_cnt;//子页面显示计数
    u16 auto_reflash_time;//自动刷新计数
    u16 time_count;//计数器
    void (*timeout_cb)(int);//回调函数
    int main_menu;//当前主页
    int this_menu;//当前子页面
    LCD_API *ui_api;//显示api
    const struct ui_dis_api *ui;//指向具体模式的处理函数
    void *private;//页面open时候传入的函数
};

enum {
    UI_MSG_REFLASH,//刷新主页
    UI_MSG_OTHER,//显示子页面
    UI_MSG_STRICK,//定时器事件
    UI_MSG_MENU_SW,//页面切换
    UI_MSG_MENU_CLOSE,//页面关闭
    UI_MSG_EXIT,//模式退出
};




static const struct ui_dis_api *ui_dis_main[] = {
#if TCFG_APP_BT_EN
    &bt_main,
#endif
#if TCFG_APP_MUSIC_EN
    &music_main,
#endif
#if TCFG_APP_FM_EN
    &fm_main,
#endif
#if TCFG_APP_RECORD_EN
    &record_main,
#endif
#if TCFG_APP_LINEIN_EN
    &linein_main,
#endif
#if TCFG_APP_RTC_EN
    &rtc_main,
#endif
#if TCFG_APP_PC_EN
    &pc_main,
#endif
#if TCFG_APP_SPDIF_EN
    /* &idle_main, */
#endif
    &idle_main,
};




#define list_for_each_ui_main(c) \
    int ui_list = 0;\
	for (c = ui_dis_main[ui_list]; ui_list <sizeof(ui_dis_main)/sizeof(ui_dis_main[0]); ui_list++,c = ui_dis_main[ui_list])



static struct ui_display_env __ui_display_env = {0};

#define __ui_display (&__ui_display_env)

static int post_ui_msg(int *msg, u8 len)
{
    int count = 0;
    int err = 0;
    if (!__ui_display->init) {
        return -1;
    }

__retry:
    err =  os_taskq_post_type(UI_TASK_NAME, msg[0], len - 1, &msg[1]);
    if (cpu_in_irq() || cpu_irq_disabled()) {
        return err;
    }
    if (err) {

        if (!strcmp(os_current_task(), UI_TASK_NAME)) {
            return err;
        }

        if (count > 10) {
            return -1;
        }
        count++;
        os_time_dly(1);
        goto __retry;
    }
    return err;
}
//=================================================================================//
//                                 API 函数
//=================================================================================//

//=================================================================================//
// @brief: 非主界面显示
// @input:
// 	1)tmp_menu: 要显示的非主界面
// 	2)ret_time: 持续时间ms, 返回主界面
//  3)arg: 显示参数
//  4)子菜单被打断或者时间到了
//=================================================================================//
void ui_set_tmp_menu(u8 app_menu, u16 ret_time, s32 arg, void (*timeout_cb)(u8 menu))
{

    int msg[6];
    msg[0] = UI_MSG_OTHER;
    msg[1] = app_menu;
    msg[2] = arg;
    msg[3] = (int)((ret_time + 99) / 100);
    msg[4] = (int)timeout_cb;
    post_ui_msg(msg, 5);
}

//进入app时设置一次, 设置主界面
void ui_set_main_menu(enum ui_menu_main menu)
{
    int msg[2];
    msg[0] = UI_MSG_MENU_SW;
    msg[1] = menu;
    post_ui_msg(msg, 2);
}
//进入app时设置一次, 设置主界面
void ui_close_main_menu()
{
    int msg[1];
    msg[0] = UI_MSG_MENU_CLOSE;
    post_ui_msg(msg, 1);
}




//=================================================================================//
// @brief: 主界面显示刷新
// @input:
// 	1)break_in : 1:打断显示 0：不打断显示
// 	打断显示可以理解为当前正在显示临时页面并非主页，例如设置页面，打断显示可以打断当前显示，恢复主页刷新
// 	实例化函数：ui_menu_reflash_action
//=================================================================================//
void ui_menu_reflash(u8 break_in)//break_in 是否打断显示,例如显示设置过程中需要刷新新界面。是是否打断设置界面显示
{
    int msg[2];
    msg[0] = UI_MSG_REFLASH;
    msg[1] = !!break_in;
    post_ui_msg(msg, 2);
}

//=================================================================================//
// @brief: 自动刷新时间
// @input: 自动刷新主页
//=================================================================================//
void ui_set_auto_reflash(u32 msec)//自动刷新主页
{
    if (__ui_display->init) {
        __ui_display->auto_reflash_time = (msec + 99) / 100;
    }

}


//=================================================================================//
// @brief: led7 复用io 推灯  api
// // @input:
// 	1)app_menu 需要显示的灯id
// 	2)on  开关灯
// 	3)phase 相位,即先低电平 后高电平 或者先高电平后低电平
// 	4)高电平时间 单位2ms
// 	5)周期时间 单位2ms
//=================================================================================//

void ui_set_led(u8 app_menu, u8 on, u8 phase, u16 highlight, u16 period)//
{
#if TCFG_UI_LED1888_ENABLE
    extern void LED1888_show_led0(u32 arg);
    extern void LED1888_show_led1(u32 arg);
    u32 arg = 0;
    arg = (!!on) << 31 | (!!phase) << 30 | highlight << 15 | (period - highlight);

    if (period >= highlight) {
        if (app_menu == MENU_LED0) {
            if (arg) {
                LED1888_show_led0(arg);
            } else {
                LED1888_show_led0(0);
            }
            return;
        } else if (app_menu == MENU_LED1) {
            if (arg) {
                LED1888_show_led1(arg);
            } else {
                LED1888_show_led1(0);
            }
            return;
        }

    }
#endif
}

u8 ui_get_app_menu(u8 type)
{
    if (!__ui_display->ui_api) {
        return 0;
    }
    if (type) {
        return __ui_display->main_menu;
    } else {
        return __ui_display->this_menu;
    }
    return 0;
}


static void ui_strick_loop()
{
    int msg[1];
    if (__ui_display->init) {
        if (__ui_display->auto_reflash_time && ((++__ui_display->time_count) >= __ui_display->auto_reflash_time)) {
            ui_menu_reflash(0);//自动刷新主页
            __ui_display->time_count = 0;
        }

        if (__ui_display->tmp_menu_ret_cnt && ((--__ui_display->tmp_menu_ret_cnt) == 0)) {
            msg[0] = UI_MSG_STRICK;
            post_ui_msg(msg, 1);
        }
    }
}

//=================================================================================//
//                                 实例化函数
//=================================================================================//

//=================================================================================//
//=================================================================================//
// @brief: 主界面显示实例化函数
// api 对应 ui_menu_reflash
//=================================================================================//
static void __ui_menu_reflash_action(u8 break_in)
{
    if (break_in && (__ui_display->this_menu != MENU_MAIN)) {//如果需要打断显示，先判断当前是否主页，不是主页
        if (__ui_display->timeout_cb) {//判断是否有打断显示回调函数，有则告诉应用，当前页面已经被打断
            __ui_display->timeout_cb(__ui_display->this_menu);
            __ui_display->timeout_cb = NULL;
        }
    } else if (__ui_display->this_menu != MENU_MAIN) {//不支持打断显示
        return ;
    }
    if (__ui_display->ui && __ui_display->ui->ui_main) {
        __ui_display->ui->ui_main(__ui_display->ui_api, __ui_display->private); //刷新主页
    }
    __ui_display->this_menu = MENU_MAIN;
}


//=================================================================================//
// @brief: 子界面显示实例化函数
// api 对应 ui_set_tmp_menu
//=================================================================================//
static void __ui_user_action(int menu, u16 ret_time, u32 arg, void (*timeout_cb)(u8 menu))
{
    int ret = false;
    if (menu != __ui_display->this_menu) {//如果显示的子页面和当前不一样，则回调注册的回调函数
        if (__ui_display->timeout_cb) {
            __ui_display->timeout_cb(__ui_display->this_menu);
            __ui_display->timeout_cb = NULL;
        }
    }
    __ui_display->tmp_menu_ret_cnt = ret_time;
    __ui_display->timeout_cb = timeout_cb;
    __ui_display->this_menu = menu;
    if (__ui_display->ui && __ui_display->ui->ui_user) {//判断使用有指向的主页，有的则优先主页处理
        ret =  __ui_display->ui->ui_user(__ui_display->ui_api, __ui_display->private, menu, arg);
    }
    if (ret != true) {
        ui_common(__ui_display->ui_api, __ui_display->private, menu, arg);   //公共显示
    }
}

//=================================================================================//
// @brief: 主界面显示实例化函数
//=================================================================================//
static void __ui_main_close_action()
{
    __ui_display->auto_reflash_time = 0;//停止自动计数
    __ui_display->tmp_menu_ret_cnt = 0;
    __ui_display->time_count        = 0;

    if (__ui_display->timeout_cb) {//检测回调
        __ui_display->timeout_cb(__ui_display->this_menu);
        __ui_display->timeout_cb = NULL;
    }
    if (__ui_display->ui && __ui_display->ui->close) {//掉用关闭
        __ui_display->ui->close(__ui_display->ui_api, __ui_display->private);
        __ui_display->ui = NULL;
        __ui_display->private = NULL;
    }
    __ui_display->main_menu = 0;
    __ui_display->this_menu = MENU_MAIN;
}





//=================================================================================//
// @brief: 主界面显示实例化函数
// api 对应 ui_set_tmp_menu
//=================================================================================//
static void __ui_main_open_action(int cur_main)
{
    const struct ui_dis_api *ui;
    __ui_main_close_action();
    list_for_each_ui_main(ui) {
        if (ui->ui == cur_main) {
            __ui_display->ui = ui;
            if (ui->open) {
                __ui_display->private = ui->open((void *)__ui_display->ui_api);
            }
            if (ui->ui_main) {
                ui->ui_main((void *)__ui_display->ui_api, __ui_display->private);
            }
            break;
        }
    }
    __ui_display->main_menu = cur_main;
    __ui_display->this_menu = MENU_MAIN;
}




static void __ui_strick_action()
{
    if (!__ui_display->tmp_menu_ret_cnt && __ui_display->main_menu) {
        if (__ui_display->timeout_cb) {
            __ui_display->timeout_cb(__ui_display->this_menu);
            __ui_display->timeout_cb = NULL;
        }
        if (__ui_display->ui && __ui_display->ui->ui_main) {
            __ui_display->ui->ui_main(__ui_display->ui_api, __ui_display->private);
        }
        __ui_display->this_menu = MENU_MAIN;
    }
}


static void ui_task(void *p)
{
    int msg[32];
    int ret;
    __ui_display->init = 1;
    os_sem_post(&__ui_display->sem);
    sys_timer_add(NULL, ui_strick_loop, 100); //500ms
    while (1) {
        ret = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)); //500ms_reflash
        if (ret != OS_TASKQ) {
            continue;
        }
        switch (msg[0]) { //action
        case UI_MSG_EXIT:
            os_sem_post((OS_SEM *)msg[1]);
            os_time_dly(10000);
            break;
        case UI_MSG_STRICK:
            __ui_strick_action();
            break;
        case UI_MSG_REFLASH:
            __ui_menu_reflash_action(!!msg[1]);
            break;
        case UI_MSG_OTHER:
            __ui_user_action(msg[1], msg[3], msg[2], (void (*)(u8))msg[4]);
            break;
        case UI_MSG_MENU_SW:
            __ui_main_open_action(msg[1]);
            break;
        case UI_MSG_MENU_CLOSE:
            __ui_main_close_action();
            break;

        default:
            break;
        }
    }
}





int led7_ui_init(const struct ui_devices_cfg *ui_cfg)
{

    int err = 0;
    clock_add(DEC_UI_CLK);
#if TCFG_UI_LED1888_ENABLE
    if (ui_cfg->type == LED_7) {
        void *led_1888_init(const struct led7_platform_data * _data);
        __ui_display->ui_api = (LCD_API *)led_1888_init(ui_cfg->private_data);
    }
#endif

#if TCFG_UI_LED7_ENABLE
    void *led7_init(const struct led7_platform_data * _data);
    __ui_display->ui_api = (LCD_API *)led7_init(ui_cfg->private_data);
#endif

    if (__ui_display->ui_api == NULL) {
        return -ENODEV;
    }
    os_sem_create(&__ui_display->sem, 0);
    err = task_create(ui_task, NULL, UI_TASK_NAME);
    os_sem_pend(&__ui_display->sem, 0);
    return 0;
}



#endif /* #if TCFG_UI_ENABLE */
