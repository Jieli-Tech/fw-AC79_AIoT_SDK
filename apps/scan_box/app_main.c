#include "system/includes.h"
#include "event/key_event.h"
#include "event/bt_event.h"
#include "event/device_event.h"
#include "app_config.h"
#include "generic/log.h"
#include "qrcode.h"
#include "syscfg/syscfg_id.h"


/*中断列表 */
const struct irq_info irq_info_table[] = {
#if CPU_CORE_NUM == 1
    { IRQ_SOFT5_IDX,      7,   0    }, //此中断强制注册到cpu0
    { IRQ_SOFT4_IDX,      7,   1    }, //此中断强制注册到cpu1
    { -2,     			-2,   -2   },//如果加入了该行, 那么只有该行之前的中断注册到对应核, 其他所有中断强制注册到CPU0
#endif

    { -1,     -1,   -1    },
};


/*创建使用 os_task_create_static 或者task_create 接口的 静态任务堆栈*/
#define SYS_TIMER_STK_SIZE 512
#define SYS_TIMER_Q_SIZE 128
static u8 sys_timer_tcb_stk_q[sizeof(StaticTask_t) + SYS_TIMER_STK_SIZE * 4 + sizeof(struct task_queue) + SYS_TIMER_Q_SIZE] ALIGNE(4);

#define SYSTIMER_STK_SIZE 256
static u8 systimer_tcb_stk_q[sizeof(StaticTask_t) + SYSTIMER_STK_SIZE * 4] ALIGNE(4);

#define SYS_EVENT_STK_SIZE 512
static u8 sys_event_tcb_stk_q[sizeof(StaticTask_t) + SYS_EVENT_STK_SIZE * 4] ALIGNE(4);

#define APP_CORE_STK_SIZE 1536
#define APP_CORE_Q_SIZE 1024
static u8 app_core_tcb_stk_q[sizeof(StaticTask_t) + APP_CORE_STK_SIZE * 4 + sizeof(struct task_queue) + APP_CORE_Q_SIZE] ALIGNE(4);


/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core",            15,     APP_CORE_STK_SIZE,	  APP_CORE_Q_SIZE,		 app_core_tcb_stk_q },
    {"sys_event",           29,     SYS_EVENT_STK_SIZE,	   0, 					 sys_event_tcb_stk_q },
    {"systimer",            14,     SYSTIMER_STK_SIZE, 	   0,					 systimer_tcb_stk_q },
    {"sys_timer",            9,     SYS_TIMER_STK_SIZE,	  SYS_TIMER_Q_SIZE,		 sys_timer_tcb_stk_q },
    {"audio_server",        16,     512,    64    },
    {"audio_encoder",       12,     384,    64    },
    {"video_server",        26,     512,   128    },
    {"update",              21,     512,    32    },
    {"dw_update",           21,     512,    32    },
    {"jpg_spec_enc",        27,     1024,   32    },
    {"dynamic_huffman0",    15,     256,    32    },
    {"video0_rec0",         19,     256,    64    },
    {"video0_rec1",         19,     256,    64    },
    {"vpkg_server",         26,     512,   128    },
#if CPU_CORE_NUM > 1
    {"#C0btctrler",         19,     512,   384    },
    {"#C0btstack",          18,     1024,  384    },
#else
    {"btctrler",            19,     512,   384    },
    {"btstack",             18,     768,   384    },
#endif
    {0, 0},
};


static int main_key_event_handler(struct key_event *key)
{
    switch (key->action) {
    case KEY_EVENT_CLICK:
        switch (key->value) {
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
    return 0;
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
    case SYS_TOUCH_EVENT:
        break;
    case SYS_DEVICE_EVENT:
        main_dev_event_handler((struct device_event *)event->payload);
        break;
#ifdef CONFIG_BT_ENABLE
    case SYS_BT_EVENT:
        struct bt_event *bt = (struct bt_event *)event->payload;
        if (event->from == BT_EVENT_FROM_CON) {
            extern void bt_ble_init(void);
            bt_ble_init();
        }
        break;
#endif
    default:
        ASSERT(0, "unknow event type: %s\n", __func__);
        break;
    }
}


static void hid_send_test(void *priv)
{
    int ret = 0;
    char buf[] = "{123!@#$%^&*()_+ABC}\r\n";
#if USB_HID_KEYBOARD_ENABLE
    extern int hid_keyboard_send(const u8 * data, u32 len);
    ret = hid_keyboard_send((u8 *)buf, strlen(buf));
    printf("hid_keyboard, ret = %d\n", ret);
#elif USB_HID_POS_ENABLE
    int hid_pos_send(u8 * data, u32 len, u8 force);
    ret = hid_pos_send((u8 *)buf, strlen(buf), 0);
    printf("hid_pos, ret = %d\n", ret);
#endif
}

static void *qr_decoder;
extern u32 timer_get_ms(void);

void qrcode_process(u8 *data, u32 len, int width, int height)
{
    char *buf;
    int buf_size;
    int enc_type;
    int md_detected = 0, ret = 0, type = 0; //是否检测到运动物体
    u32 time = timer_get_ms();
    if (!qr_decoder) {
        qr_decoder = qrcode_init(QR_FRAME_W, QR_FRAME_H, QR_FRAME_W, QRCODE_MODE_NORMAL, 150, SYM_QRCODE, 0);
    }
    qrcode_detectAndDecode(qr_decoder, data, &md_detected);
    ret = qrcode_get_result(qr_decoder, &buf, &buf_size, &enc_type);
    type = qrcode_get_symbol_type(qr_decoder);
    if (buf_size > 0 && ret == 0) {
        buf[buf_size] = 0;
        printf("qr code type = %d decode: %s\n", type, buf);
        /* printf("\n @@@@@@ time = %d \n", timer_get_ms() - time); */
#if USB_HID_KEYBOARD_ENABLE
        extern int hid_keyboard_send(const u8 * data, u32 len);
        hid_keyboard_send((u8 *)buf, strlen(buf));
        /* hid_send_test(NULL); */
#elif USB_HID_POS_ENABLE
        int hid_pos_send(u8 * data, u32 len, u8 force);
        hid_pos_send((u8 *)buf, strlen(buf), 0);
        /* hid_send_test(NULL); */
#endif
    }
}

/*
 * 应用程序主函数
 */
void app_main()
{
    puts("-------------scan_box app main-------------\n");

#ifdef CONFIG_VIDEO_ENABLE
    extern void get_yuv_init(void (*cb)(u8 * data, u32 len, int width, int height));
    get_yuv_init(qrcode_process);
#endif

#ifdef CONFIG_BT_ENABLE
    extern void bt_ble_module_init(void);
    bt_ble_module_init();
#endif
}

