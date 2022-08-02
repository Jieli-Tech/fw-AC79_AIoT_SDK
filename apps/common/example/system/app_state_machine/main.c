#include "app_config.h"
#include "system/includes.h"
#include "app_core.h"
#include "event/key_event.h"
#include "event/device_event.h"
/* #include "init.h" */

#ifdef USE_APP_STATE_MACHINE_TEST_DEMO

#define ACTION_APP_RUN1           0x00009001
#define ACTION_APP_RUN2           0x00009002


static void app_state_machine_test_task(void *p)
{
    os_time_dly(100); //app_core_init初始化完成后才可以测试

    struct intent it;
    init_intent(&it);

    while (1) {
        printf("TEST APP1 RUN  ACTION_APP_RUN1 \r\n");
        it.name = "APP1";
        it.action = ACTION_APP_RUN1;
        it.data = "action.data";
        start_app(&it);

        printf("TEST APP1 RUN  ACTION_APP_RUN2 \r\n");
        it.name = "APP1";
        it.action = ACTION_APP_RUN2;
        it.exdata = 0x12345678;
        start_app(&it);

        printf("TEST APP2 RUN  ACTION_APP_RUN1 \r\n");
        it.name = "APP2";
        it.action = ACTION_APP_RUN1;
        start_app(&it);

        printf("TEST APP2 RUN  ACTION_APP_RUN2 \r\n");
        it.name = "APP2";
        it.action = ACTION_APP_RUN2;
        start_app(&it);

        printf("TEST RUN ACTION_BACK \r\n");
        it.action = ACTION_BACK;//退出当前状态机,回退到前一个上一个状态机,即退出APP2, 迁移到APP1
        start_app(&it);

        printf("TEST APP1 RUN  ACTION_APP_RUN1 \r\n");
        it.name = "APP1";
        it.action = ACTION_APP_RUN1;
        start_app(&it);

        printf("TEST RUN ACTION_BACK \r\n");
        it.action = ACTION_BACK;//退出当前状态机,回退到前一个上一个状态机,即退出APP1, 迁移到APP2
        start_app(&it);

        printf("TEST APP2 RUN  ACTION_APP_RUN2 \r\n");
        it.name = "APP2";
        it.action = ACTION_APP_RUN1;
        start_app(&it);

        os_time_dly(300);
        break;
    }

    printf("TEST APP1 RUN  ACTION_STOP \r\n");
    it.name = "APP1";
    it.action = ACTION_STOP;
    start_app(&it);

    printf("TEST APP2 RUN  ACTION_STOP \r\n");
    it.name = "APP2";
    it.action = ACTION_STOP;
    start_app(&it);



    while (1) {
        os_time_dly(300);
    }
}
static void c_main(void *priv)
{
    os_task_create(app_state_machine_test_task, NULL, 10, 1000, 0, "app_state_machine_test_task");
}
late_initcall(c_main);

static int app1_state_machine(struct application *app, enum app_state state,
                              struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        printf("        app1_state_machine[in %s task] run APP_STA_CREATE \r\n", os_current_task());
        break;
    case APP_STA_START:
        switch (it->action) {
        case ACTION_APP_RUN1:
            printf("        app1_state_machine[in %s task] run ACTION_APP_RUN1 %s\r\n", os_current_task(), it->data);
            break;
        case ACTION_APP_RUN2:
            printf("        app1_state_machine[in %s task] run ACTION_APP_RUN2 0x%x \r\n", os_current_task(), it->exdata);
            break;
        default :
            break;
        }
        break;
    case APP_STA_PAUSE:
        printf("        app1_state_machine[in %s task] run APP_STA_PAUSE \r\n", os_current_task());
        break;
    case APP_STA_RESUME:
        printf("        app1_state_machine[in %s task] run APP_STA_RESUME \r\n", os_current_task());
        break;
    case APP_STA_STOP:
        printf("        app1_state_machine[in %s task] run APP_STA_STOP \r\n", os_current_task());
        break;
    case APP_STA_DESTROY:
        printf("        app1_state_machine[in %s task] run APP_STA_DESTROY \r\n", os_current_task());
        break;
    default :
        break;
    }
    return 0;
}
//app的按键事件响应函数
static int app1_key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK://单击事件（当然还有长按、双击等事件）
        switch (key->value) {
        case KEY_OK:
            break;
        case KEY_MENU:
            break;
        case KEY_MODE:
            break;
        case KEY_UP:
            break;
        case KEY_DOWN:
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return false;//当事件是在该APP处理则返回true，否则返回false，才能使得没有处理的事件其他APP状态机也可以接收到
}
//app的设备事件响应函数
static int app1_device_event_handler(struct device_event *event)
{
    printf("---> event->arg = %s ,event->event = %d\n", event->arg, event->event);
    if (!ASCII_StrCmp(event->arg, "sd*", 4)) {//如SD卡事件判断
        switch (event->event) {
        case DEVICE_EVENT_IN:
            break;
        case DEVICE_EVENT_OUT:
            break;
        default :
            break;
        }
        return true;//当事件是在该APP处理则返回true，否则返回false，才能使得没有处理的事件其他APP状态机也可以接收到
    }
    return false;//当事件是在该APP处理则返回true，否则返回false，才能使得没有处理的事件其他APP状态机也可以接收到
}
//app事件处理函数
static int app1_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT://按键事件
        return app1_key_event_handler((struct key_event *)event->payload);
    case SYS_DEVICE_EVENT://设备事件
        return app1_device_event_handler((struct device_event *)event->payload);
    default:
        return false;
    }
}
static const struct application_operation app1_ops = {
    .state_machine  = app1_state_machine,//app状态处理
    .event_handler  = app1_event_handler,//app事件处理
};
REGISTER_APPLICATION(app1) = {
    .name   = "APP1",
    .ops    = &app1_ops,
    .state  = APP_STA_DESTROY,
};









static int app2_state_machine(struct application *app, enum app_state state,
                              struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        printf("        app2_state_machine[in %s task] run APP_STA_CREATE \r\n", os_current_task());
        break;
    case APP_STA_START:
        switch (it->action) {
        case ACTION_APP_RUN1:
            printf("        app2_state_machine[in %s task] run ACTION_APP_RUN1 \r\n", os_current_task());
            break;
        case ACTION_APP_RUN2:
            printf("        app2_state_machine[in %s task] run ACTION_APP_RUN2 \r\n", os_current_task());
            break;
        default :
            break;
        }
        break;
    case APP_STA_PAUSE:
        printf("        app2_state_machine[in %s task] run APP_STA_PAUSE \r\n", os_current_task());
        break;
    case APP_STA_RESUME:
        printf("        app2_state_machine[in %s task] run APP_STA_RESUME \r\n", os_current_task());
        break;
    case APP_STA_STOP:
        printf("        app2_state_machine[in %s task] run APP_STA_STOP \r\n", os_current_task());
        break;
    case APP_STA_DESTROY:
        printf("        app2_state_machine[in %s task] run APP_STA_DESTROY \r\n", os_current_task());
        break;
    default :
        break;
    }

    return 0;
}

static const struct application_operation app2_ops = {
    .state_machine  = app2_state_machine,
    .event_handler  = NULL,//事件处理类似app1
};
REGISTER_APPLICATION(app2) = {
    .name   = "APP2",
    .ops    = &app2_ops,
    .state  = APP_STA_DESTROY,
};

#endif
