#include "app_config.h"
#include "system/includes.h"
#include "event/device_event.h"
#include "event/key_event.h"
#include "key/key_driver.h"

/* 一个事件递交到当前APP处理,实际上最常用*/
static int app1_event_handler(struct application *app, struct sys_event *event)
{
    if (event->type == SYS_KEY_EVENT) {
        struct key_event *key_e = (struct key_event *)event->payload;
        printf("app1_event_handler[in %s task]  SYS_KEY_EVENT %d \r\n", os_current_task(), key_e->value);
    }

    return true;    //如果返回true则不再调用 app_default_event_handler
    //return false;
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


        os_time_dly(200);
        puts("\n\n\n\n");

        struct key_event key = {0};
        key.action = KEY_EVENT_CLICK;
        // key.value = NO_KEY;
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
    // .prob_handler   = static_event_prob_handler,
    // .post_handler   = static_event_post_handler,
};

static int c_main(void)
{
    os_task_create(app_event_test_task, NULL, 10, 1000, 128, "app_event_test_task");
    return 0;
}
late_initcall(c_main);
