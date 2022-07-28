
#include "app_config.h"
#include "system/includes.h"
#include "event/device_event.h"
#include "event/key_event.h"

#ifdef USE_EVENT_TEST_DEMO

int sys_event_recode(u16 type, u8 from, void *event, u8 len)/* event_nitify调用的库内弱函数,可以转义事件或者对特定事件处理*/
{
    printf("sys_event_recode[in %s task] run  %d\r\n", os_current_task(), type);
    return 0;//返回0不做任何事情,继续发送事件
    return 1;//返回1不再发送事件
}

/* 一个事件到来,首先遍历所有static_event_handler的prob_handler函数处理
一般用不到*/
static int static_event_prob_handler(struct sys_event *event)
{
    printf("static_event_prob_handler[in %s task] run  %d\r\n", os_current_task(), event->type);

    return 0;
    return -EINVAL;  //如果返回-EINVAL则不再递交给其他所有地方处理这个事件
}

/* 一个事件递交到当前APP处理之前的钩子函数,库内弱函数,一般用不到*/
void  app_event_prepare_handler(struct sys_event *event)
{
    printf("app_event_prepare_handler[in %s task] run  %d\r\n", os_current_task(), event->type);
}

/* 一个事件递交到当前APP处理,实际上最常用*/
static int app1_event_handler(struct application *app, struct sys_event *event)
{
    if (event->type == SYS_DEVICE_EVENT) {
        struct device_event *device_e = (struct device_event *)event->payload;
        printf("app1_event_handler[in %s task]  SYS_DEVICE_EVENT %s \r\n", os_current_task(), device_e->arg);
    } else if (event->type == SYS_KEY_EVENT) {
        struct key_event *key_e = (struct key_event *)event->payload;
        printf("app1_event_handler[in %s task]  SYS_KEY_EVENT %d \r\n", os_current_task(), key_e->value);
    }

    return true;    //如果返回true则不再调用 app_default_event_handler
    return false;
}

/*
 * 库内弱函数,默认的系统事件处理函数
 * 当所有活动的app的事件处理函数都返回false时此函数会被调用
 * 通常用作多个APP的统一事件处理函数
 */
void app_default_event_handler(struct sys_event *event)
{
    printf("app_default_event_handler[in %s task] run  %d\r\n", os_current_task(), event->type);
}

/* 当前APP处理完事件之后,遍历所有static_event_handler的post_handler函数处理,一般用不到*/
static void static_event_post_handler(struct sys_event *event)
{
    printf("static_event_post_handler[in %s task] run  %d\r\n", os_current_task(), event->type);
}

static void app_event_test_task(void *p)
{
    os_time_dly(100); //app_core_init初始化完成后才可以测试

    struct intent it;
    init_intent(&it);

    it.name = "APP1";
    it.action = ACTION_DO_NOTHING;
    start_app(&it);

    key_event_enable(); //针对按键事件,需要先使能开关

    while (1) {
        struct device_event event = {0};
        event.arg = "event_arg";
        event.event = DEVICE_EVENT_IN;
        event.value = 0x12345678;
        device_event_notify(DEVICE_EVENT_FROM_PC, &event);

        os_time_dly(200);
        puts("\n\n\n\n");

        struct key_event key = {0};
        key.action = KEY_EVENT_CLICK;
        key.value = KEY_DOWN;
        key.type = KEY_EVENT_USER;
        key_event_notify(KEY_EVENT_FROM_USER, &key);

        os_time_dly(200);
        puts("\n\n\n\n");

    }
}

static int app1_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    return 0;
}
static const struct application_operation app1_ops = {
    .state_machine  = app1_state_machine,
    .event_handler  = app1_event_handler,
};
REGISTER_APPLICATION(app1) = {
    .name   = "APP1",
    .ops    = &app1_ops,
    .state  = APP_STA_DESTROY,
};


//注意一个静态事件句柄
SYS_EVENT_STATIC_HANDLER_REGISTER(static_event_handler) = {
    .event_type     = SYS_ALL_EVENT,
    .prob_handler   = static_event_prob_handler,
    .post_handler   = static_event_post_handler,
};

static int c_main(void)
{
    os_task_create(app_event_test_task, NULL, 10, 1000, 128, "app_event_test_task");
    return 0;
}
late_initcall(c_main);
#endif
