/**
 * @file task_pc.c
 * @brief 从机模式
 * @author chenrixin@zh-jieli.com
 * @version 1.0.0
 * @date 2020-02-29
 */

#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "os/os_api.h"
#include "event/device_event.h"
#include "sdio/sdmmc.h"

#if TCFG_USB_SLAVE_ENABLE
#if USB_PC_NO_APP_MODE == 1
#include "app_task.h"
#include "app_charge.h"
#include "asm/charge.h"
#endif
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"

#if TCFG_USB_SLAVE_HID_ENABLE
#include "usb/device/hid.h"
#endif

#if TCFG_USB_SLAVE_MSD_ENABLE
#include "usb/device/msd.h"
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
#include "usb/device/cdc.h"
#endif

#if (TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0)
#include "dev_multiplex_api.h"
#endif

#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB_TASK]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define USB_TASK_NAME0   "usb_msd0"
#define USB_TASK_NAME1   "usb_msd1"

#define USBSTACK_EVENT		    0x80
#define USBSTACK_MSD_RUN		0x81
#define USBSTACK_MSD_RELASE		0x82
#define USBSTACK_HID		    0x83
#define USBSTACK_MSD_RESET      0x84

extern int usb_audio_demo_init(void);
extern void usb_audio_demo_exit(void);

static OS_MUTEX msd_mutex[USB_MAX_HW_NUM];//SEC(.usb_g_bss);
static u8 msd_in_task[USB_MAX_HW_NUM];
static u8 msd_run_reset[USB_MAX_HW_NUM];


static void usb_task(void *p)
{
    usb_dev usbfd = (usb_dev)p;
    int ret = 0;
    int msg[16];

    while (1) {
        ret = os_task_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        if (msg[0] != Q_MSG) {
            continue;
        }
        switch (msg[1]) {
#if TCFG_USB_SLAVE_MSD_ENABLE
        case USBSTACK_MSD_RUN:
            os_mutex_pend(&msd_mutex[usbfd], 0);
            msd_in_task[usbfd] = 1;
#if TCFG_USB_APPLE_DOCK_EN
            apple_mfi_link((void *)msg[2]);
#else
            USB_MassStorage((void *)msg[2]);
#endif
            if (msd_run_reset[usbfd]) {
                msd_reset((struct usb_device_t *)msg[2], 0);
                msd_run_reset[usbfd] = 0;
            }
            msd_in_task[usbfd] = 0;
            os_mutex_post(&msd_mutex[usbfd]);
            break;
        case USBSTACK_MSD_RELASE:
            os_sem_post((OS_SEM *)msg[2]);
            while (1) {
                os_time_dly(10000);
            }
            break;
//        case USBSTACK_MSD_RESET:
//            os_mutex_pend(&msd_mutex[usbfd], 0);
//            msd_reset((struct usb_device_t *)msg[2], (u32)msg[3]);
//            os_mutex_post(&msd_mutex[usbfd]);
//            break;
#endif
        default:
            break;
        }
    }
}

static void usb_msd_wakeup(struct usb_device_t *usb_device)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    if (!usb_id) {
        os_taskq_post_msg(USB_TASK_NAME0, 2, USBSTACK_MSD_RUN, usb_device);
    } else {
        os_taskq_post_msg(USB_TASK_NAME1, 2, USBSTACK_MSD_RUN, usb_device);
    }
}
static void usb_msd_reset_wakeup(struct usb_device_t *usb_device, u32 itf_num)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    /* os_taskq_post_msg(USB_TASK_NAME, 3, USBSTACK_MSD_RESET, usb_device, itf_num); */
    if (msd_in_task[usb_id]) {
        msd_run_reset[usb_id] = 1;
    } else {
#if TCFG_USB_SLAVE_MSD_ENABLE
        msd_reset(usb_device, 0);
#endif
    }
}
static void usb_msd_init(const usb_dev usbfd)
{
    /* r_printf("%s()", __func__); */
    int err;
    os_mutex_create(&msd_mutex[usbfd]);
    if (!usbfd) {
        err = task_create(usb_task, (void *)usbfd, USB_TASK_NAME0);
    } else {
        err = task_create(usb_task, (void *)usbfd, USB_TASK_NAME1);
    }
    if (err != OS_NO_ERR) {
        r_printf("usb_msd task creat fail %x\n", err);
    }
}
static void usb_msd_free(const usb_dev usbfd)
{
    /* r_printf("%s()", __func__); */

    os_mutex_del(&msd_mutex[usbfd], 0);

    int err;
    OS_SEM sem;
    os_sem_create(&sem, 0);
    if (!usbfd) {
        os_taskq_post_msg(USB_TASK_NAME0, 2, USBSTACK_MSD_RELASE, (int)&sem);
    } else {
        os_taskq_post_msg(USB_TASK_NAME1, 2, USBSTACK_MSD_RELASE, (int)&sem);
    }
    os_sem_pend(&sem, 0);

    if (!usbfd) {
        err = task_kill(USB_TASK_NAME0);
    } else {
        err = task_kill(USB_TASK_NAME1);
    }
    if (!err) {
        r_printf("usb_msd_uninit succ!!\n");
    } else {
        r_printf("usb_msd_uninit fail!!\n");
    }
}

#if TCFG_USB_SLAVE_CDC_ENABLE
static void usb_cdc_wakeup(struct usb_device_t *usb_device)
{
    //回调函数在中断里，正式使用不要在这里加太多东西阻塞中断，
    //或者先post到任务，由任务调用cdc_read_data()读取再执行后续工作
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 buf[64] = {0};
    u32 rlen;

    log_debug("cdc rx hook");
    rlen = cdc_read_data(usb_id, buf, 64);
    put_buf(buf, rlen);
    cdc_write_data(usb_id, buf, rlen);
}
#endif

void usb_start(const usb_dev usbfd)
{
#if TCFG_USB_SLAVE_AUDIO_ENABLE
    usb_audio_demo_init();
#endif

#ifdef USB_DEVICE_CLASS_CONFIG
#ifdef USB_DEVICE_CLASS_CONFIG_2_0
    if (usbfd != 0) {
        g_printf("USB_DEVICE_CLASS_CONFIG:%x", USB_DEVICE_CLASS_CONFIG_2_0);
        usb_device_mode(usbfd, USB_DEVICE_CLASS_CONFIG_2_0);
    } else {
        g_printf("USB_DEVICE_CLASS_CONFIG:%x", USB_DEVICE_CLASS_CONFIG);
        usb_device_mode(usbfd, USB_DEVICE_CLASS_CONFIG);
    }
#else
    g_printf("USB_DEVICE_CLASS_CONFIG:%x", USB_DEVICE_CLASS_CONFIG);
    usb_device_mode(usbfd, USB_DEVICE_CLASS_CONFIG);
#endif
#endif

#if TCFG_USB_SLAVE_MSD_ENABLE
    //没有复用时候判断 sd开关
    //复用时候判断是否参与复用
#if (!TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_SD0_ENABLE)\
     ||(TCFG_SD0_ENABLE && TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_DM_MULTIPLEX_WITH_SD_PORT != 0)
    msd_register_disk(usbfd, "sd0", NULL);
#endif

#if (!TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_SD1_ENABLE)\
     ||(TCFG_SD1_ENABLE && TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0 && TCFG_DM_MULTIPLEX_WITH_SD_PORT != 1)
    msd_register_disk(usbfd, "sd1", NULL);
#endif

#if TCFG_NOR_FAT
    msd_register_disk(usbfd, "fat_nor", NULL);
#endif

#if TCFG_VIR_UDISK_ENABLE
    msd_register_disk(usbfd, "vir_udisk", NULL);
#endif

    msd_set_wakeup_handle(usbfd, usb_msd_wakeup);
    msd_set_reset_wakeup_handle(usbfd, usb_msd_reset_wakeup);
    usb_msd_init(usbfd);
#endif

#if TCFG_USB_SLAVE_CDC_ENABLE
    cdc_set_wakeup_handler(usbfd, usb_cdc_wakeup);
#endif

#if TCFG_USB_SLAVE_HID_ENABLE
#if USB_HID_KEYBOARD_ENABLE
    extern u8 *hid_keyboard_get_report_desc();
    extern u32 hid_keyboard_get_report_desc_size();
    extern int hid_keyboard_output(u8 * buf, u32 len);
    extern void hid_keyboard_set_usb_dev(usb_dev usb_id);
    hid_set_report_desc(usbfd, hid_keyboard_get_report_desc(), hid_keyboard_get_report_desc_size());
    hid_set_output_handle(usbfd, hid_keyboard_output);
    hid_keyboard_set_usb_dev(usbfd);
#elif USB_HID_POS_ENABLE
    extern u8 *hid_pos_get_report_desc();
    extern u32 hid_pos_get_report_desc_size();
    extern int hid_pos_receive(u8 * buf, u32 len);
    extern int hid_pos_open(usb_dev usb_id);
    hid_set_report_desc(usbfd, hid_pos_get_report_desc(), hid_pos_get_report_desc_size());
    hid_set_output_handle(usbfd, hid_pos_receive);
    hid_pos_open(usbfd);
#endif
    hid_control(usbfd, 1);
#endif
}

static void usb_remove_disk(const usb_dev usbfd)
{
#if TCFG_USB_SLAVE_MSD_ENABLE
    os_mutex_pend(&msd_mutex[usbfd], 0);
    msd_unregister_all(usbfd);
    os_mutex_post(&msd_mutex[usbfd]);
#endif
}

void usb_pause(const usb_dev usbfd)
{
    log_info("usb pause");

    usb_sie_disable(usbfd);

#if TCFG_USB_SLAVE_MSD_ENABLE
    if (msd_set_wakeup_handle(usbfd, NULL)) {
        usb_remove_disk(usbfd);
        usb_msd_free(usbfd);
    }
#endif

#if TCFG_USB_SLAVE_AUDIO_ENABLE
    usb_audio_demo_exit();
#endif

#if TCFG_USB_SLAVE_HID_ENABLE
    hid_control(usbfd, 0);
    hid_set_report_desc(usbfd, NULL, 0);
    hid_set_output_handle(usbfd, NULL);
#if USB_HID_POS_ENABLE
    extern void hid_pos_close(usb_dev usb_id);
    hid_pos_close(usbfd);
#endif
#endif

    usb_device_mode(usbfd, 0);
}

void usb_stop(const usb_dev usbfd)
{
    log_info("App Stop - usb");

    usb_pause(usbfd);

    usb_sie_close(usbfd);
}

int pc_device_event_handler(struct sys_event *e)
{
    if (e->from != DEVICE_EVENT_FROM_OTG) {
        return false;
    }
    struct device_event *event = (struct device_event *)e->payload;
    int switch_app_case = false;
    const char *usb_msg = (const char *)event->value;
    log_debug("usb event : %d DEVICE_EVENT_FROM_OTG %s", event->event, usb_msg);
    usb_dev usbfd = 0;

    if (usb_msg[0] == 's') {
        if (event->event == DEVICE_EVENT_IN) {
            log_info("usb %c online", usb_msg[2]);
            usbfd = usb_msg[2] - '0';
#if USB_PC_NO_APP_MODE
            usb_start(usbfd);
#elif TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
            usb_otg_suspend(0, OTG_KEEP_STATE);
            mult_sdio_suspend();
            usb_pause(usbfd);
            mult_sdio_resume();
#else
            usb_pause(usbfd);
#endif
            switch_app_case = 1;
        } else if (event->event == DEVICE_EVENT_OUT) {
            log_info("usb %c offline", usb_msg[2]);
            usbfd = usb_msg[2] - '0';
            switch_app_case = 2;
#if USB_PC_NO_APP_MODE
            usb_stop(usbfd);
#else

#ifdef CONFIG_SOUNDBOX
            if (!app_check_curr_task(APP_PC_TASK)) {
#else
            if (!app_cur_task_check(APP_NAME_PC)) {
#endif
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
                mult_sdio_suspend();
#endif
                usb_stop(usbfd);
#if TCFG_USB_DM_MULTIPLEX_WITH_SD_DAT0
                mult_sdio_resume();
#endif
            }
#endif
        }
    }

    return switch_app_case;
}

#endif
