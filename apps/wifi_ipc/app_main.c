#include "system/includes.h"
#include "action.h"
#include "app_core.h"
#include "event/key_event.h"
#include "event/device_event.h"
#include "app_config.h"
#include "storage_device.h"
#include "generic/log.h"
#include "os/os_api.h"


/*中断列表 */
const struct irq_info irq_info_table[] = {
    //中断号   //优先级0-7   //注册的cpu(0或1)
#if 0
    //不可屏蔽中断方法：支持写flash，但中断函数和调用函数和const要全部放在内部ram
    { IRQ_SOFT5_IDX,      6,   0    }, //此中断强制注册到cpu0
    { IRQ_SOFT4_IDX,      6,   1    }, //此中断强制注册到cpu1
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

    {"ai_server",			15,		1024,	64    },
    {"asr_server",			15,		1024,	64    },
    {"wake-on-voice",		7,		1024,	0     },
    {"resample_task",		8,		1024,	0     },
    {"vad_encoder",         16,     1024,   0     },
    {"video_server",        26,     800,   1024  },
    {"vpkg_server",         26,     1024,   512   },
    {"jpg_dec",             27,     1024,   32    },
    {"jpg_wl80_spec_enc",   27,     1024,   32    },
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



/*
 * 默认的系统事件处理函数
 * 当所有活动的app的事件处理函数都返回false时此函数会被调用
 */
void app_default_event_handler(struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        break;
    case SYS_TOUCH_EVENT:
        break;
    case SYS_DEVICE_EVENT:
        break;
    case SYS_BT_EVENT:
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
    struct intent it;

    puts("------------- wifi_camera app main-------------\n");

    init_intent(&it);
    it.name	= "net_video_rec";//APP状态机在：net_video_rec.c
    /*it.name	= "video_rec";//APP状态机在：video_rec.c*/
    it.action = ACTION_VIDEO_REC_MAIN;
    start_app(&it);
}

