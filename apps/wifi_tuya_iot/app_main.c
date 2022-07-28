#include "app_config.h"
#include "system/includes.h"
#include "os/os_api.h"
#include "event/net_event.h"
#include "wifi/wifi_connect.h"
#include "net/config_network.h"
#include "event/key_event.h"


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
    {"app_core",            15,     2048,	  1024 },
    {"sys_event",           29,     512,	   0 },
    {"systimer",            14,     256, 	   0 },
    {"sys_timer",            9,     512,	  128 },

//添加wifi
#ifdef CONFIG_WIFI_ENABLE
    {"tasklet",             10,     1400,    0},//通过调节任务优先级平衡WIFI收发占据总CPU的比重
    {"RtmpMlmeTask",        17,     700,  	 0},
    {"RtmpCmdQTask",        17,     300,   	 0},
    {"wl_rx_irq_thread",     5,     256,  	 0},
#endif

//添加蓝牙
#ifdef CONFIG_BT_ENABLE
#if CPU_CORE_NUM > 1
    {"#C0btctrler",         19,      512,   384   },
    {"#C0btstack",          18,      1024,  384   },
#else
    {"btctrler",            19,      512,   384   },
    {"btstack",             18,      768,   384   },
#endif
#endif

//添加升级任务
    {"update",              21,      512,   32    },
    {"dw_update",           21,      512,   32    },


    {0, 0, 0, 0, 0},
};

extern void vTaskDeleteRaw(TaskHandle_t xTaskToDelete);
void tuya_app_main()
{
    void user_main(void);
    user_main();

    vTaskDelay(100);
    printf("\r\n user main  over");
    vTaskDeleteRaw(NULL);
}

static int tuya_key_event_handler(struct key_event *key)
{
    printf("tuya_key_event_handler : (key value %d)\n", key->value);
    switch (key->action) {
    case KEY_EVENT_CLICK:
        switch (key->value) {
        case KEY_MODE:
            extern void reset_tuya(void);
            reset_tuya();
            break;
        default:
            return false;
        }
        break;
    case KEY_EVENT_LONG:
        break;
    default:
        return false;
    }

    return true;
}

/*
 * 默认的系统事件处理函数
 * 当所有活动的app的事件处理函数都返回false时此函数会被调用
 */
void app_default_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        tuya_key_event_handler((struct key_event *)event->payload);
        break;
    case SYS_DEVICE_EVENT:
        break;
    case SYS_BT_EVENT:
        extern int ble_demo_bt_event_handler(struct sys_event * event);
        ble_demo_bt_event_handler(event);
        break;
    case SYS_NET_EVENT:
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
    printf("\r\n\r\n\r\n\r\n --------- tuya app_main run %s -----------\r\n\r\n\r\n\r\n\r\n", __TIME__);

    TaskHandle_t xHandle;
    xTaskCreate(tuya_app_main, "tuya_app_main", 4 * 1024, NULL, 5, &xHandle);
}
