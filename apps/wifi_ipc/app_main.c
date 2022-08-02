#include "system/includes.h"
#include "action.h"
#include "app_core.h"
#include "event/key_event.h"
#include "event/device_event.h"
#include "app_config.h"
#include "storage_device.h"
#include "generic/log.h"
#include "os/os_api.h"
#include "event/net_event.h"
#include "lwip/netdb.h"


/*中断列表 */
const struct irq_info irq_info_table[] = {
    //中断号   //优先级0-7   //注册的cpu(0或1)
#ifdef CONFIG_IPMASK_ENABLE
    //不可屏蔽中断方法：支持写flash，但中断函数和调用函数和const要全部放在内部ram
    { IRQ_SOFT5_IDX,      6,   0    }, //此中断强制注册到cpu0
    { IRQ_SOFT4_IDX,      6,   1    }, //此中断强制注册到cpu1
#if 0 //如下，SPI1使用不可屏蔽中断设置
    { IRQ_SPI1_IDX,      7,   1    },//中断强制注册到cpu0/1
#endif
#endif
#if CPU_CORE_NUM == 1
    { IRQ_SOFT5_IDX,      7,   0    }, //此中断强制注册到cpu0
    { IRQ_SOFT4_IDX,      7,   1    }, //此中断强制注册到cpu1
    { -2,     			-2,   -2   },//如果加入了该行, 那么只有该行之前的中断注册到对应核, 其他所有中断强制注册到CPU0
#endif

    { IRQ_SPI1_IDX,      7,   1    },//中断强制注册到cpu0/1
    { IRQ_PAP_IDX,       7,   1    },//中断强制注册到cpu0/1
    { IRQ_EMI_IDX,       7,   1    },//中断强制注册到cpu0/1


    { -1,     			-1,   -1   },
};

/*创建使用 os_task_create_static 或者task_create 接口的 静态任务堆栈*/
#define SYS_TIMER_STK_SIZE 512
#define SYS_TIMER_Q_SIZE 128
static u8 sys_timer_tcb_stk_q[sizeof(StaticTask_t) + SYS_TIMER_STK_SIZE * 4 + sizeof(struct task_queue) + SYS_TIMER_Q_SIZE] ALIGNE(4);

#define SYSTIMER_STK_SIZE 256
static u8 systimer_tcb_stk_q[sizeof(StaticTask_t) + SYSTIMER_STK_SIZE * 4] ALIGNE(4);

#define SYS_EVENT_STK_SIZE 512
static u8 sys_event_tcb_stk_q[sizeof(StaticTask_t) + SYS_EVENT_STK_SIZE * 4] ALIGNE(4);

#define APP_CORE_STK_SIZE 2048
#define APP_CORE_Q_SIZE 2048

static u8 app_core_tcb_stk_q[sizeof(StaticTask_t) + APP_CORE_STK_SIZE * 4 + sizeof(struct task_queue) + APP_CORE_Q_SIZE] ALIGNE(4);

/*创建使用  thread_fork 接口的 静态任务堆栈*/
#define WIFI_TASKLET_STK_SIZE 1400
static u8 wifi_tasklet_tcb_stk_q[sizeof(struct thread_parm) + WIFI_TASKLET_STK_SIZE * 4] ALIGNE(4);

#define WIFI_CMDQ_STK_SIZE 300
static u8 wifi_cmdq_tcb_stk_q[sizeof(struct thread_parm) + WIFI_CMDQ_STK_SIZE * 4] ALIGNE(4);

#define WIFI_MLME_STK_SIZE 700
static u8 wifi_mlme_tcb_stk_q[sizeof(struct thread_parm) + WIFI_MLME_STK_SIZE * 4] ALIGNE(4);

#define WIFI_RX_STK_SIZE 256
static u8 wifi_rx_tcb_stk_q[sizeof(struct thread_parm) + WIFI_RX_STK_SIZE * 4] ALIGNE(4);

/*任务列表 */
const struct task_info task_info_table[] = {
    {"init",                30,     512,   256   },
    {"app_core",            22,     APP_CORE_STK_SIZE,	APP_CORE_Q_SIZE,  app_core_tcb_stk_q },
    {"sys_event",           30,     SYS_EVENT_STK_SIZE,  			  0,  sys_event_tcb_stk_q},
    {"systimer",            16,     SYSTIMER_STK_SIZE,  			  0,  systimer_tcb_stk_q },
    {"sys_timer",           10,     SYS_TIMER_STK_SIZE, SYS_TIMER_Q_SIZE, sys_timer_tcb_stk_q},
    {"audio_server",        16,     1024,   256   },
    {"audio_mix",           27,      512,   64    },
    {"audio_decoder",       30,     1024,   64    },
    {"audio_encoder",       14,     1024,   64    },
    {"speex_encoder",       10,     1024,   0     },
    {"opus_encoder",        10,     1536,   0     },
    {"aec_encoder",         13,     1024,   0     },
    {"dns_encoder",         13,      512,   0     },
    {"vir_dev_task",         9,     1024,   0     },
    {"wechat_task",         18,     2048,   64    },
    {"amr_encoder",         16,     1024,   0     },
    {"usb_server",          20,     1024,   64    },
#if CPU_CORE_NUM > 1
    {"#C0usb_msd0",          1,      512,   128   },
#else
    {"usb_msd0",             1,      512,   128   },
#endif
    {"usb_msd1",             1,      512,   128   },
    {"update",      		21,     512,   32    },
    {"dw_update",      		21,     512,   32    },
#ifdef CONFIG_WIFI_ENABLE
    {"tasklet",             10,     WIFI_TASKLET_STK_SIZE,   0,		 wifi_tasklet_tcb_stk_q	 },//通过调节任务优先级平衡WIFI收发占据总CPU的比重
    {"RtmpMlmeTask",        16,     WIFI_MLME_STK_SIZE,  	 0, 	 wifi_mlme_tcb_stk_q	 },
    {"RtmpCmdQTask",        16,     WIFI_CMDQ_STK_SIZE,   	 0,  	 wifi_cmdq_tcb_stk_q	 },
    {"wl_rx_irq_thread",    5,      WIFI_RX_STK_SIZE,  		 0,		 wifi_rx_tcb_stk_q		 },
#endif

#ifdef CONFIG_BT_ENABLE
#if CPU_CORE_NUM > 1
    {"#C0btencry",          14,      512,   128   },
#else
    {"btencry",             14,      512,   128   },
#endif
#if CPU_CORE_NUM > 1
    {"#C0btctrler",         19,      512,   384   },
    {"#C0btstack",          18,      1024,  384   },
#else
    {"btctrler",            19,      512,   384   },
    {"btstack",             18,      768,   384   },
#endif
#endif
    {"ai_server",			15,		1024,	64    },
    {"asr_server",			15,		1024,	64    },
    {"wake-on-voice",		7,		1024,	0     },
    {"resample_task",		8,		1024,	0     },
    {"vad_encoder",         16,     1024,   0     },
    {"video_server",        26,     800,   1024  },
    {"vpkg_server",         26,     1024,   512   },
    {"jpg_dec",             27,     1024,   32    },
    {"jpg_spec_enc",        27,     1024,   32    },
    {"dynamic_huffman0",    15,     256,    32    },
    {"video0_rec0",         25,     512,   512   },
    {"video0_rec1",         24,     512,   512   },
    {"video1_rec0",         25,     512,   512   },
    {"video1_rec1",         24,     512,   512   },
    {"video2_rec0",         25,     512,   512   },
    {"video2_rec1",         24,     512,   512   },
    {"audio_rec0",          22,     256,   256   },
    {"audio_rec1",          19,     256,   256   },
    {"avi0",                29,     320,   64    },
    {"avi1",                29,     1024,   64    },

    {"ctp_server",          26,     600,   64  },
    {"net_video_server",    16,     2048,   64  },
    {"net_avi0",            24,     600,   0    },
    {"net_avi1",            24,     2400,   0    },
    {"net_avi2",            24,     2400,   0    },
    {"stream_avi0",         18,     600,   0   },
    {"stream_avi1",         18,     600,   0   },
    {"stream_avi2",         18,     600,   0   },
    {"video_dec_server",    27,     1024,   1024  },
    {"vunpkg_server",       23,     1024,   128   },
    {"yuv_task",            22,     1024,   128   },
    {"iotc_thread_resolve_master_name_new",         18,     2024,   0   },
    {"iotc_thread_resolve_master_name_new2",         18,     2024,   0   },
#ifdef CONFIG_UI_ENABLE
    {"ui",           	    6,     768,   256  },
#endif
    {0, 0},
};



static int app_music_net_event_handler(struct net_event *event)
{
    if (!ASCII_StrCmp(event->arg, "net", 4)) {
        switch (event->event) {
        case NET_CONNECT_TIMEOUT_NOT_FOUND_SSID:
        case NET_CONNECT_TIMEOUT_ASSOCIAT_FAIL:
            ble_cfg_net_result_notify(event->event);
            break;
        case NET_EVENT_SMP_CFG_FIRST:
            break;
        case NET_EVENT_SMP_CFG_FINISH:
            printf(">>>>>>>>>>NET_EVENT_SMP_CFG_FINISH\n");
            if (is_in_config_network_state()) {
                config_network_stop();
                config_network_connect();
                sys_power_auto_shutdown_resume();
            }
            break;
        case NET_EVENT_CONNECTED:
            puts("NET_EVENT_CONNECTED\n");

            config_network_broadcast();

            if (!is_in_config_network_state()) {
#if BT_NET_CFG_EN
                ble_cfg_net_result_notify(event->event);
#endif
            }
            break;
        case NET_EVENT_DISCONNECTED:
            puts("NET_EVENT_DISCONNECTED\n");
            canceladdrinfo();
            return false;
        case NET_EVENT_SMP_CFG_TIMEOUT:
            if (is_in_config_network_state()) {
                printf("NET_EVENT_SMP_CFG_TIMEOUT\n");
            }
            break;

        case NET_SMP_CFG_COMPLETED:
#ifdef CONFIG_AIRKISS_NET_CFG
            wifi_smp_set_ssid_pwd();
#endif
            break;
        case NET_EVENT_DISCONNECTED_AND_REQ_CONNECT:
            wifi_return_sta_mode();
            break;
        case NET_NTP_GET_TIME_SUCC:	//NTP获取成功事件返回
            break;
        default:
            break;
        }
    }

    return false;
}

/*
 * 默认的系统事件处理函数
 * 当所有活动的app的事件处理函数都返回false时此函数会被调用
 */


static void bt_net_config(void)
{
    printf("%s   %d\n", __func__, __LINE__);

#ifdef CONFIG_ASSIGN_MACADDR_ENABLE
    cancel_server_assign_macaddr();
#endif


    if (!is_in_config_network_state()) {
        sys_power_auto_shutdown_pause();

        config_network_start();
    } else {

        config_network_stop();
        wifi_return_sta_mode();
        sys_power_auto_shutdown_resume();
    }

}



static int bt_net_config_key_click(struct key_event *key)
{
    /* printf("key->value%d\n",key->value); */
    switch (key->value) {
    case KEY_OK:
#ifdef CONFIG_NET_ENABLE
        puts("switch_net_config\n");
        bt_net_config();
#endif
        break;
    default:
        break;
    }

    return false;
}


static int bt_config_net_key_event_handler(struct key_event *key)
{
    int ret = false;

    switch (key->action) {

    case KEY_EVENT_CLICK:
        ret = bt_net_config_key_click(key);
        break;
    default:
        break;
    }

    return ret;
}

void app_default_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        printf("%s   %d\n", __func__, __LINE__);
        bt_config_net_key_event_handler((struct key_event *)event->payload);
        break;
    case SYS_TOUCH_EVENT:
        break;
    case SYS_DEVICE_EVENT:
        break;
    case SYS_BT_EVENT:
        extern int ble_demo_bt_event_handler(struct sys_event * event);
        ble_demo_bt_event_handler(event);
        break;
#ifdef CONFIG_NET_ENABLE
    case SYS_NET_EVENT:
        app_music_net_event_handler((struct net_event *)event->payload);
        break;
#endif


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
    struct intent it;

    key_event_enable(); //针对按键事件,需要先使能开关

    puts("------------- wifi_camera app main-------------\n");

    init_intent(&it);
    /* it.name	= "net_video_rec";//APP状态机在：net_video_rec.c */
    it.name	= "video_rec";//APP状态机在：video_rec.c
    it.action = ACTION_VIDEO_REC_MAIN;
    start_app(&it);

    extern void bt_ble_module_init(void);
    bt_ble_module_init();

}

