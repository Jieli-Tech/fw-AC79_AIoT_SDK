#include "app_config.h"
#include "system/includes.h"
#include "os/os_api.h"
#include "event/net_event.h"
#include "wifi/wifi_connect.h"
#include "net/config_network.h"
#include "event/key_event.h"
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
    {"init",                30,     512,   256   },
    {"app_core",            15,     1024,   256   },
    {"sys_event",           29,      512,   0     },
    {"systimer",            14,      256,   0     },
    {"sys_timer",            9,      512,   64    },
    {"mp3_encoder",         13,      768,   0     },
    {"audio_server",        16,      512,   64    },
    {"audio_mix",           28,      512,   0     },
    {"audio_decoder",       30,     1024,   64    },
    {"audio_encoder",       12,      384,   64    },
    {"speex_encoder",       10,     1024,   0     },
    {"opus_encoder",        10,     1536,   0     },
    {"vir_dev_task",        13,      256,   0     },
    {"wechat_task",         18,     2048,   64    },
    {"amr_encoder",         16,     1024,   0     },
    {"cvsd_encoder",        13,      512,   0     },
    {"vad_encoder",         16,     1024,   0     },
    {"aec_encoder",         13,     1024,   0     },
    {"dns_encoder",         13,      512,   0     },
    {"adpcm_encoder",       13,      512,   0     },
    {"msbc_encoder",        13,      256,   0     },
    {"echo_deal",           11,     1024,   32    },
    {"sbc_encoder",         13,      512,   0     },
    {"usb_server",          20,     1024,   64    },
    {"usb_msd",             25,     1024,   32    },
    {"uac_sync",            20,      512,   0     },
    {"uac_play",             7,      768,   0     },
    {"uac_record",           7,      768,   32    },
    {"uda_main",             2,     7000,   0     },
#ifdef CONFIG_BT_ENABLE
#if CPU_CORE_NUM > 1
    {"#C0btencry",          16,      512,   128   },
    {"#C0btctrler",         19,      512,   384   },
    {"#C0btstack",          18,      1024,  384   },
#else
    {"btencry",             14,      512,   128   },
    {"btctrler",            19,      512,   384   },
    {"btstack",             18,      768,   384   },
#endif
#endif

#ifdef CONFIG_WIFI_ENABLE
    {"tasklet",             10,     1400,    0},//通过调节任务优先级平衡WIFI收发占据总CPU的比重
    {"RtmpMlmeTask",        17,     700,  	 0},
    {"RtmpCmdQTask",        17,     300,   	 0},
    {"wl_rx_irq_thread",     5,     256,  	 0},
#endif

#ifdef CONFIG_UI_ENABLE
    {"ui",           	    6,     768,   256  },
#endif
    {"update",              21,      512,   32    },
    {"dw_update",           21,      512,   32    },
    {"iperf_test",          15,     1024,   0     },
    {"ai_server",			15,		1024,	64    },
    {"asr_server",			15,		1024,	64    },
    {"wake-on-voice",		7,		1024,	0     },
    {"resample_task",		8,		1024,	0     },
    {"video_server",        26,      800,   1024  },
    {"vpkg_server",         26,      512,   512   },
    {"jpg_dec",             27,     1024,   32    },
    {"jpg_spec_enc",        27,     1024,   32    },
    {"dynamic_huffman0",    15,      256,   32    },
    {"video0_rec0",         25,      512,   512   },
    {"video0_rec1",         24,      512,   512   },
    {"video1_rec0",         25,      512,   512   },
    {"video1_rec1",         24,      512,   512   },
    {"video2_rec0",         25,      512,   512   },
    {"video2_rec1",         24,      512,   512   },
    {"audio_rec0",          22,      256,   256   },
    {"audio_rec1",          19,      256,   256   },
    {"avi0",                29,      320,   64    },
    {"avi1",                29,      320,   64    },
    {"avi2",                29,      320,   64    },

    {"ctp_server",          26,      600,   64    },
    {"net_video_server",    16,      256,   64    },
    {"net_avi0",            24,      600,   0     },
    {"net_avi1",            24,      600,   0     },
    {"net_avi2",            24,      600,   0     },
    {"stream_avi0",         18,      600,   0     },
    {"stream_avi1",         18,      600,   0     },
    {"stream_avi2",         18,      600,   0     },
    {"video_dec_server",    27,     1024,   1024  },
    {"vunpkg_server",       23,     1024,   128   },
    {"yuv_task",            22,     1024,   128   },
    {"wyuv_task",           22,     1024,   128   },

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
#ifdef USE_DEMO_WIFI_TEST
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
#endif //USE_DEMO_WIFI_TEST
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
    printf("\r\n\r\n\r\n\r\n --------------- app_main run %s ---------------\r\n\r\n\r\n\r\n\r\n", __TIME__);

    struct intent it;
    init_intent(&it);
    it.name = "app_demo";
    it.action = ACTION_DO_NOTHING;
    start_app(&it);
}

