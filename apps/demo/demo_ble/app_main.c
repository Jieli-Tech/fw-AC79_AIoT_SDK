#include "system/includes.h"
#include "app_config.h"
#include "generic/log.h"
#include "os/os_api.h"
#include "event/key_event.h"
#include "event/bt_event.h"
#include "event/device_event.h"

/*中断列表 */
const struct irq_info irq_info_table[] = {
#if CPU_CORE_NUM == 1
    { IRQ_SOFT5_IDX,      7,   0    }, //此中断强制注册到cpu0
    { IRQ_SOFT4_IDX,      7,   1    }, //此中断强制注册到cpu1
    { -2,     			-2,   -2   },//如果加入了该行, 那么只有该行之前的中断注册到对应核, 其他所有中断强制注册到CPU0
#endif

    { -1,     -1,   -1    },
};

/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core",            15,     1024,   256   },
    {"sys_event",           29,      512,   0     },
    {"systimer",            14,      256,   0     },
    {"sys_timer",            9,      512,   64    },
#ifdef CONFIG_BT_ENABLE
#if CPU_CORE_NUM > 1
    {"#C0btctrler",         19,      512,   384   },
    {"#C0btstack",          18,      1024,  384   },
#else
    {"btctrler",            19,      512,   384   },
    {"btstack",             18,      768,   384   },
#endif
#endif
    {0, 0},
};

static int main_key_event_handler(struct key_event *key)
{
#if CONFIG_BLE_MESH_ENABLE
    extern void input_key_handler(u8 key_status, u8 key_number);
    input_key_handler(key->action, key->value);
#endif
    return false;
}

static int main_dev_event_handler(struct device_event *event)
{
    switch (event->event) {
    case DEVICE_EVENT_IN:
        break;
    case DEVICE_EVENT_OUT:
        break;
    case DEVICE_EVENT_CHANGE:
        break;
    }
    return false;
}

/*
 * 默认的系统事件处理函数
 * 当所有活动的app的事件处理函数都返回false时此函数会被调用
 */
void app_default_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        main_key_event_handler((struct key_event *)event->payload);
        break;
    case SYS_DEVICE_EVENT:
        main_dev_event_handler((struct device_event *)event->payload);
        break;
    case SYS_BT_EVENT:
        extern int ble_demo_bt_event_handler(struct sys_event * event);
        ble_demo_bt_event_handler(event);
        break;
    default:
        ASSERT(0, "unknow event type: %s\n", __func__);
        break;
    }
}

/*
 * 应用程序主函数
 */
void app_main()
{
    puts("------------- demo_ble app main-------------\n");

    extern void bt_ble_module_init(void);
    bt_ble_module_init();
}

