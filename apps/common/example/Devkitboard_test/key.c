#include "app_config.h"

#ifdef USE_DevKitBoard_TEST_DEMO
#include "device/device.h"//u8
#include "server/video_dec_server.h"//fopen
#include "system/includes.h"//GPIO
#include "ename.h"
#include "event/key_event.h"


#define POST_TASK_NAME  "ui_main_task"

int main_key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK:
        switch (key->value) {
        case KEY_K1:
            printf("k1 down");
            os_taskq_post(POST_TASK_NAME, 2, UI_MSG_KEY_TEST, 1);
            break;
        case KEY_K2:
            printf("k2 down");
            os_taskq_post(POST_TASK_NAME, 2, UI_MSG_KEY_TEST, 2);
            break;
        case KEY_K3:
            printf("k3 down");
            os_taskq_post(POST_TASK_NAME, 2, UI_MSG_KEY_TEST, 3);
            break;
        case KEY_K4:
            printf("k4 down");
            os_taskq_post(POST_TASK_NAME, 2, UI_MSG_KEY_TEST, 4);
            break;
        case KEY_K5:
            printf("k5 down");
            os_taskq_post(POST_TASK_NAME, 2, UI_MSG_KEY_TEST, 5);
            break;
        case KEY_K6:
            printf("k6 down");
            os_taskq_post(POST_TASK_NAME, 2, UI_MSG_KEY_TEST, 6);
            break;
        case KEY_K7:
            printf("k7 down");
            os_taskq_post(POST_TASK_NAME, 2, UI_MSG_KEY_TEST, 7);
            break;
        case KEY_K8:
            printf("k8 down");
            os_taskq_post(POST_TASK_NAME, 2, UI_MSG_KEY_TEST, 8);
            break;
        default:
            return false;
        }
        break;
    case KEY_EVENT_LONG:
        switch (key->value) {
        case KEY_K1:
            printf("k1 long down");
            break;
        case KEY_K2:
            printf("k2 long down");
            break;
        case KEY_K3:
            printf("k3 long down");
            break;
        case KEY_K4:
            printf("k4 long down");
            break;
        case KEY_K5:
            printf("k5 long down");
            break;
        case KEY_K6:
            printf("k6 long down");
            break;
        case KEY_K7:
            printf("k7 long down");
            break;
        case KEY_K8:
            printf("k8 long down");
            break;
        default:
            return false;
        }
        break;
    default:
        return false;
    }

    return true;
}

#if 0  //该函数放到蓝牙播歌例子下面了

void app_default_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        main_key_event_handler((struct key_event *)event->payload);
        break;
    case SYS_TOUCH_EVENT:
        break;
    case SYS_DEVICE_EVENT:
        /*main_dev_event_handler((struct device_event *)event->payload);*/
        break;
    /*#ifdef CONFIG_NET_ENABLE*/
    case SYS_NET_EVENT:
        /*main_net_event_hander((struct net_event *)event->payload);*/
        break;
    /*#endif*/
    default:
        break;
    }
}
#endif

#endif
