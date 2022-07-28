#include "app_config.h"
#include "system/includes.h"
#include "os/os_api.h"
#include "event/net_event.h"
#include "wifi/wifi_connect.h"
#include "net/config_network.h"

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

#ifdef CONFIG_WIFI_ENABLE
    {"tasklet",             10,     1400,    0},//通过调节任务优先级平衡WIFI收发占据总CPU的比重
    {"RtmpMlmeTask",        17,     700,  	 0},
    {"RtmpCmdQTask",        17,     300,   	 0},
    {"wl_rx_irq_thread",     5,     256,  	 0},
#endif

    {0, 0, 0, 0, 0},
};


static int app_demo_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    return 0;
}
static int app_demo_event_handler(struct application *app, struct sys_event *sys_event)
{
    switch (sys_event->type) {
    case SYS_NET_EVENT:
        struct net_event *net_event = (struct net_event *)sys_event->payload;
        if (!ASCII_StrCmp(net_event->arg, "net", 4)) {
            switch (net_event->event) {
            case NET_CONNECT_TIMEOUT_NOT_FOUND_SSID:
                puts("app_demo_event_handler recv NET_CONNECT_TIMEOUT_NOT_FOUND_SSID \r\n");
                break;
            case NET_CONNECT_TIMEOUT_ASSOCIAT_FAIL:
                puts("app_demo_event_handler recv NET_CONNECT_TIMEOUT_ASSOCIAT_FAIL \r\n");
                break;
            case NET_EVENT_SMP_CFG_FIRST:
                puts("app_demo_event_handler recv NET_EVENT_SMP_CFG_FIRST \r\n");
                break;
            case NET_EVENT_SMP_CFG_FINISH:
                puts("app_demo_event_handler recv NET_EVENT_SMP_CFG_FINISH \r\n");
                if (is_in_config_network_state()) {
                    config_network_stop();
                }
                config_network_connect();
                break;
            case NET_EVENT_CONNECTED:
                puts("app_demo_event_handler recv NET_EVENT_CONNECTED \r\n");
                extern void config_network_broadcast(void);
                config_network_broadcast();
                break;
            case NET_EVENT_DISCONNECTED:
                puts("app_demo_event_handler recv NET_EVENT_DISCONNECTED \r\n");
                break;
            case NET_EVENT_SMP_CFG_TIMEOUT:
                puts("app_demo_event_handler recv NET_EVENT_SMP_CFG_TIMEOUT \r\n");
                break;
            case NET_SMP_CFG_COMPLETED:
                puts("app_demo_event_handler recv NET_SMP_CFG_COMPLETED \r\n");
#ifdef CONFIG_AIRKISS_NET_CFG
                wifi_smp_set_ssid_pwd();
#endif
                break;
            case NET_EVENT_DISCONNECTED_AND_REQ_CONNECT:
                puts("app_demo_event_handler recv NET_EVENT_DISCONNECTED_AND_REQ_CONNECT \r\n");
                extern void wifi_return_sta_mode(void);
                wifi_return_sta_mode();
                break;
            default:
                break;
            }
        }
    default:
        return false;
    }
}

static const struct application_operation app_demo_ops = {
    .state_machine  = app_demo_state_machine,
    .event_handler 	= app_demo_event_handler,
};

REGISTER_APPLICATION(app_demo) = {
    .name 	= "app_demo",
    .ops 	= &app_demo_ops,
    .state  = APP_STA_DESTROY,
};

/*
 * 应用程序主函数
 */
void app_main()
{
    printf("\r\n\r\n\r\n\r\n ------------------- demo_wifi app_main run %s ---------------\r\n\r\n\r\n\r\n\r\n", __TIME__);

    struct intent it;
    init_intent(&it);
    it.name = "app_demo";
    it.action = ACTION_DO_NOTHING;
    start_app(&it);

}

