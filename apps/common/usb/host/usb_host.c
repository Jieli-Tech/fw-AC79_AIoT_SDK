#include "includes.h"
#include "app_config.h"
#include "device_drive.h"
#include "event/device_event.h"
#if TCFG_USB_HOST_ENABLE
#include "usb_config.h"
#include "usb/host/usb_host.h"
#include "usb/usb_phy.h"
#include "usb_ctrl_transfer.h"
#include "usb_storage.h"
#include "adb.h"
#include "aoa.h"
#include "host_uvc.h"
#include "hid.h"
#include "audio.h"
#include "usbnet.h"

#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#include "apple_mfi.h"
#endif

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[mount]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


static struct usb_host_device host_devices[USB_MAX_HW_NUM];// SEC(.usb_h_bss);
static u8 *h_ep0_dmabuf[USB_MAX_HW_NUM];


int host_dev_status(const struct usb_host_device *host_dev)
{
    return ((host_dev)->private_data.status);
}

u32 host_device2id(const struct usb_host_device *host_dev)
{
#if USB_MAX_HW_NUM > 1
    return ((host_dev)->private_data.usb_id);
#else
    return 0;
#endif
}
const struct usb_host_device *host_id2device(const usb_dev id)
{
#if USB_MAX_HW_NUM > 1
    return &host_devices[id];
#else
    return &host_devices[0];
#endif
}
int usb_sem_init(struct usb_host_device  *host_dev)
{
    usb_dev usb_id = host_device2id(host_dev);

    usb_host_config(usb_id);

    OS_SEM *sem = zalloc(sizeof(OS_SEM));
    ASSERT(sem, "usb alloc sem error");
    host_dev->sem = sem;
    g_printf("%s %x %x ", __func__, host_dev, sem);
    os_sem_create(host_dev->sem, 0);
    return 0;
}
int usb_sem_pend(struct usb_host_device  *host_dev, u32 timeout)
{
    if (host_dev->sem == NULL) {
        return 1;
    }
    int ret = os_sem_pend(host_dev->sem, timeout);
    if (ret) {
        r_printf("%s %d ", __func__, ret);
    }
    return ret;
}
int usb_sem_post(struct usb_host_device  *host_dev)
{
    if (host_dev->sem == NULL) {
        return 1;
    }
    int ret = os_sem_post(host_dev->sem);
    if (ret) {
        r_printf("%s %d ", __func__, ret);
    }
    return 0;
}
int usb_sem_del(struct usb_host_device *host_dev)
{
    usb_dev usb_id = host_device2id(host_dev);

    r_printf("1");
    if (host_dev->sem == NULL) {
        return 0;
    }
    r_printf("2");
    r_printf("3");
#if USB_HUB
    if (host_dev && host_dev->sem && host_dev->father == NULL) {
        os_sem_del(host_dev->sem, 0);
    }
#else
    if (host_dev && host_dev->sem) {
        os_sem_del(host_dev->sem, 0);
    }
#endif
    r_printf("4");
    g_printf("%s %x %x ", __func__, host_dev, host_dev->sem);
    free(host_dev->sem);
    r_printf("5");
    host_dev->sem = NULL;
    r_printf("6");
    usb_host_free(usb_id);
    r_printf("7");
    return 0;
}

static int _usb_msd_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find udisk @ interface %d", interface_num);
#if TCFG_UDISK_ENABLE
    return   usb_msd_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_apple_mfi_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find udisk @ interface %d", interface_num);
#if TCFG_USB_APPLE_DOCK_EN
    return   usb_apple_mfi_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_adb_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find adb @ interface %d", interface_num);
#if TCFG_ADB_ENABLE
    return usb_adb_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_aoa_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find aoa @ interface %d", interface_num);
#if TCFG_AOA_ENABLE
    return usb_aoa_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif

}
static int _usb_hid_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find hid @ interface %d", interface_num);
#if TCFG_HID_HOST_ENABLE
    return usb_hid_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_audio_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find audio @ interface %d", interface_num);
#if TCFG_HOST_AUDIO_ENABLE
    return usb_audio_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_adb_interface_ptp_mtp_parse(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find adbmtp @ interface %d", interface_num);
#if TCFG_ADB_ENABLE
    return usb_adb_interface_ptp_mtp_parse(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}
static int _usb_uvc_parse(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find uvc @ interface %d", interface_num);
#if TCFG_HOST_UVC_ENABLE
    return usb_uvc_parser(host_dev, interface_num, (u8 *)pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}


static int _usb_wireless_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find wireless @ interface %d", interface_num);
#if TCFG_HOST_WIRELESS_ENABLE
    return usbnet_generic_cdc_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}


static int _usb_wireless_at_port_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    log_info("find wireless at_port @ interface %d", interface_num);
#if TCFG_HOST_WIRELESS_ENABLE
    return usbnet_at_port_parser(host_dev, interface_num, pBuf);
#else
    return USB_DT_INTERFACE_SIZE;
#endif
}


static int usb_descriptor_parser(struct usb_host_device *host_dev, const u8 *pBuf, u32 total_len, struct usb_device_descriptor *device_desc)
{
    int len = 0;
    u8 interface_num = 0;
    struct usb_private_data *private_data = &host_dev->private_data;

    struct usb_config_descriptor *cfg_desc = (struct usb_config_descriptor *)pBuf;

    if (cfg_desc->bDescriptorType != USB_DT_CONFIG ||
        cfg_desc->bLength < USB_DT_CONFIG_SIZE) {
        log_error("invalid descriptor for config bDescriptorType = %d bLength= %d",
                  cfg_desc->bDescriptorType, cfg_desc->bLength);
        return -USB_DT_CONFIG;
    }

    log_info("idVendor %x idProduct %x", device_desc->idVendor, device_desc->idProduct);

    len += USB_DT_CONFIG_SIZE;
    pBuf += USB_DT_CONFIG_SIZE;
    int i = 0;
    u32 have_find_valid_class = 0;
    while (len < total_len) {
        if (interface_num > MAX_HOST_INTERFACE) {
            log_error("interface_num too much");
            break;
        }

        struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)pBuf;
        if (interface->bDescriptorType == USB_DT_INTERFACE) {

            printf("inf class %x subclass %x ep %d",
                   interface->bInterfaceClass, interface->bInterfaceSubClass, interface->bNumEndpoints);

            if (interface->bInterfaceClass == USB_CLASS_MASS_STORAGE) {
                i = _usb_msd_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((device_desc->idVendor == 0x05AC) &&
                       ((device_desc->idProduct & 0xff00) == 0x1200)) {
                i = _usb_apple_mfi_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if (device_desc->idVendor == 0x2c7c &&
                       (device_desc->idProduct == 0x0191 || device_desc->idProduct == 0x0125) &&
                       interface->bInterfaceClass == 0xff &&
                       interface->bInterfaceSubClass == 0x00) {
                i = _usb_wireless_at_port_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---, i = %d", __func__, __LINE__, i);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if (interface->bInterfaceClass == USB_CLASS_AUDIO) {
                i = _usb_audio_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((interface->bInterfaceClass == 0xff)  &&
                       (interface->bInterfaceSubClass == USB_CLASS_ADB)) {
                i = _usb_adb_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((device_desc->idVendor == 0x18d1) &&
                       ((device_desc->idProduct & 0x2d00) == 0x2d00)) {
                i = _usb_aoa_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if (interface->bInterfaceClass == USB_CLASS_HID) {
                i = _usb_hid_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                    have_find_valid_class = true;
                }
            } else if ((interface->bNumEndpoints == 3) &&
                       (interface->bInterfaceClass == 0xff || interface->bInterfaceClass == 0x06)) {
                i = _usb_adb_interface_ptp_mtp_parse(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if ((interface->bInterfaceClass == 0xff) &&
                       (interface->bInterfaceSubClass == 0xff)) {
                i = _usb_aoa_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if (interface->bInterfaceClass == USB_CLASS_VIDEO) {
                i = _usb_uvc_parse(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---", __func__, __LINE__);
                    len = total_len;
                } else {
                    interface_num += 2;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else if (interface->bInterfaceClass == USB_CLASS_WIRELESS_CONTROLLER) {
                i = _usb_wireless_parser(host_dev, interface_num, pBuf);
                if (i < 0) {
                    log_error("---%s %d---, i = %d", __func__, __LINE__, i);
                    len = total_len;
                } else {
                    interface_num++;
                    len += i;
                    pBuf += i;
                }
                have_find_valid_class = true;
            } else {
                log_info("find unsupport [class %x subClass %x] @ interface %d",
                         interface->bInterfaceClass,
                         interface->bInterfaceSubClass,
                         interface_num);

                len += USB_DT_INTERFACE_SIZE;
                pBuf += USB_DT_INTERFACE_SIZE;
            }
        } else {
            /* log_error("unknown section %d %d", len, pBuf[0]); */
            if (pBuf[0]) {
                len += pBuf[0];
                pBuf += pBuf[0];
            } else {
                len = total_len;
            }
        }
    }


    log_debug("len %d total_len %d", len, total_len);
    return !have_find_valid_class;
}


/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_suspend
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
void usb_host_suspend(const usb_dev usb_id)
{
    usb_h_entry_suspend(usb_id);
}

void usb_host_resume(const usb_dev usb_id)
{
    usb_h_resume(usb_id);
}

static u32 _usb_host_mount(const usb_dev usb_id, u32 retry, u32 reset_delay, u32 mount_timeout)
{
    u32 ret = DEV_ERR_NONE;
    struct usb_host_device *host_dev = &host_devices[usb_id];
    struct usb_private_data *private_data = &host_dev->private_data;


    for (int i = 0; i < retry; i++) {
        usb_h_sie_init(usb_id);
        ret = usb_host_init(usb_id, reset_delay, mount_timeout);
        if (ret) {
            reset_delay += 10;
            continue;
        }

        if (!h_ep0_dmabuf[usb_id]) {
            h_ep0_dmabuf[usb_id] = usb_h_alloc_ep_buffer(usb_id, 0, 64);
        }
        usb_set_dma_taddr(usb_id, 0, h_ep0_dmabuf[usb_id]);

        usb_sie_enable(usb_id);//enable sie intr
        usb_mdelay(reset_delay);

        /**********get device descriptor*********/
        struct usb_device_descriptor device_desc;
        private_data->usb_id = usb_id;
        private_data->status = 0;
        private_data->devnum = 0;
        private_data->ep0_max_packet_size = 8;
        ret = usb_get_device_descriptor(host_dev, &device_desc);

        /**********set address*********/
        usb_mdelay(20);
        u8 devnum = rand32() % 16 + 1;
        ret = set_address(host_dev, devnum);
        check_usb_mount(ret);
        private_data->devnum = devnum ;

        /**********get device descriptor*********/
        usb_mdelay(20);
        ret = usb_get_device_descriptor(host_dev, &device_desc);
        check_usb_mount(ret);
        private_data->ep0_max_packet_size = device_desc.bMaxPacketSize0;

        /**********get config descriptor*********/
        struct usb_config_descriptor cfg_desc;
        ret = get_config_descriptor(host_dev, &cfg_desc, USB_DT_CONFIG_SIZE);
        check_usb_mount(ret);

#if USB_H_MALLOC_ENABLE
        u8 *desc_buf = zalloc(cfg_desc.wTotalLength + 16);
        ASSERT(desc_buf, "desc_buf");
#else
        u8 desc_buf[128] = {0};
        cfg_desc.wTotalLength = min(sizeof(desc_buf), cfg_desc.wTotalLength);
#endif

        ret = get_config_descriptor(host_dev, desc_buf, cfg_desc.wTotalLength);
        check_usb_mount(ret);

        /**********set configuration*********/
        ret = set_configuration(host_dev);
        /* printf_buf(desc_buf, cfg_desc.wTotalLength); */
        ret |= usb_descriptor_parser(host_dev, desc_buf, cfg_desc.wTotalLength, &device_desc);
#if USB_H_MALLOC_ENABLE
        log_info("free:desc_buf= %x\n", desc_buf);
        free(desc_buf);
#endif
        check_usb_mount(ret);

        for (int itf = 0; itf < MAX_INTERFACE_NUM; itf++) {
            if (host_dev->interface_info[itf]) {
                host_dev->interface_info[itf]->ctrl->set_power(host_dev, 1);
            }
        }

        break;//succ
    }

    if (ret) {
        goto __exit_fail;
    }
    private_data->status = 1;
    return DEV_ERR_NONE;

__exit_fail:
    printf("usb_probe fail");
    private_data->status = 0;
    usb_sie_close(usb_id);
    return ret;
}
static int usb_event_notify(const struct usb_host_device *host_dev, u32 ev)
{
    const usb_dev id = host_device2id(host_dev);
    struct device_event event = {0};
    static u32 bmUsbEvent[USB_MAX_HW_NUM];
    u8 have_post_event = 0;
    u8 no_send_event = 0;
    if (ev == 0) {
        event.event = DEVICE_EVENT_IN;
    } else if (ev == 1) {
        event.event = DEVICE_EVENT_CHANGE;
    } else {
        event.event = DEVICE_EVENT_OUT;
        goto __usb_event_out;
    }
    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        no_send_event = 0;
        event.value = 0;
        if (host_dev->interface_info[i]) {
            switch (host_dev->interface_info[i]->ctrl->interface_class) {
#if TCFG_UDISK_ENABLE
            case USB_CLASS_MASS_STORAGE:
                if (have_post_event & BIT(0)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(0);
                }
                if (id == 0) {
                    event.value = (int)"udisk0";
                } else {
                    event.value = (int)"udisk1";
                }
                bmUsbEvent[id] |= BIT(0);
                break;
#endif
#if TCFG_ADB_ENABLE
            case USB_CLASS_ADB:
                if (have_post_event & BIT(1)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(1);
                }
                if (id == 0) {
                    event.value = (int)"adb0";
                } else {
                    event.value = (int)"adb1";
                }
                bmUsbEvent[id] |= BIT(1);
                break;
#endif
#if TCFG_AOA_ENABLE
            case USB_CLASS_AOA:
                if (have_post_event & BIT(2)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(2);
                }
                if (id == 0) {
                    event.value = (int)"aoa0";
                } else {
                    event.value = (int)"aoa1";
                }
                bmUsbEvent[id] |= BIT(2);
                break;
#endif
#if TCFG_HID_HOST_ENABLE
            case USB_CLASS_HID:
                if (have_post_event & BIT(3)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(3);
                }
                if (id == 0) {
                    event.value = (int)"hid0";
                } else {
                    event.value = (int)"hid1";
                }
                bmUsbEvent[id] |= BIT(3);
                break;
#endif
#if TCFG_HOST_AUDIO_ENABLE
            case USB_CLASS_AUDIO:
                if (have_post_event & BIT(4)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(4);
                }
                if (id == 0) {
                    event.value = (int)"audio0";
                } else {
                    event.value = (int)"audio1";
                }
                bmUsbEvent[id] |= BIT(4);
                break;
#endif
#if TCFG_HOST_UVC_ENABLE
            case USB_CLASS_VIDEO:
                if (have_post_event & BIT(5)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(5);
                }
                if (id == 0) {
                    event.value = (int)"uvc0";
                } else {
                    event.value = (int)"uvc1";
                }
                bmUsbEvent[id] |= BIT(5);
                break;
#endif
#if TCFG_HOST_WIRELESS_ENABLE
            case USB_CLASS_WIRELESS_CONTROLLER:
                if (have_post_event & BIT(6)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(6);
                }
                if (id == 0) {
                    event.value = (int)"wireless0";
                } else {
                    event.value = (int)"wireless1";
                }
                bmUsbEvent[id] |= BIT(6);
                break;

            case USB_CLASS_VENDOR_SPEC:
                if (have_post_event & BIT(7)) {
                    no_send_event = 1;
                } else {
                    have_post_event |= BIT(7);
                }
                if (id == 0) {
                    event.value = (int)"at_port0";
                } else {
                    event.value = (int)"at_port1";
                }
                bmUsbEvent[id] |= BIT(7);
                break;
#endif
            }

            if (!no_send_event && event.value) {
                log_info("event %x interface %x class %x %s",
                         event.event, i,
                         host_dev->interface_info[i]->ctrl->interface_class,
                         (const char *)event.value);

                /* printf("usb_host_mount notify >>>>>>>>>>>\n"); */
                event.arg = id == 0 ? (void *)"usb_host0" : (void *)"usb_host1";
                device_event_notify(DEVICE_EVENT_FROM_USB_HOST, &event);
            }
        }
    }

__usb_event_out:
    if (event.event == DEVICE_EVENT_OUT) {
        for (int i = 0; i < 32; i++) {
            if (bmUsbEvent[id] & BIT(i)) {
                switch (i) {
#if TCFG_UDISK_ENABLE
                case 0:
                    if (id == 0) {
                        event.value = (int)"udisk0";
                    } else {
                        event.value = (int)"udisk1";
                    }
                    break;
#endif
#if TCFG_ADB_ENABLE
                case 1:
                    if (id == 0) {
                        event.value = (int)"adb0";
                    } else {
                        event.value = (int)"adb1";
                    }
                    break;
#endif
#if TCFG_AOA_ENABLE
                case 2:
                    if (id == 0) {
                        event.value = (int)"aoa0";
                    } else {
                        event.value = (int)"aoa1";
                    }
                    break;
#endif
#if TCFG_HID_HOST_ENABLE
                case 3:
                    if (id == 0) {
                        event.value = (int)"hid0";
                    } else {
                        event.value = (int)"hid1";
                    }
                    break;
#endif
#if TCFG_HOST_AUDIO_ENABLE
                case 4:
                    if (id == 0) {
                        event.value = (int)"audio0";
                    } else {
                        event.value = (int)"audio1";
                    }
                    break;
#endif
#if TCFG_HOST_UVC_ENABLE
                case 5:
                    if (id == 0) {
                        event.value = (int)"uvc0";
                    } else {
                        event.value = (int)"uvc1";
                    }
                    break;
#endif
#if TCFG_HOST_WIRELESS_ENABLE
                case 6:
                    if (id == 0) {
                        event.value = (int)"wireless0";
                    } else {
                        event.value = (int)"wireless1";
                    }
                    break;

                case 7:
                    if (id == 0) {
                        event.value = (int)"at_port0";
                    } else {
                        event.value = (int)"at_port1";
                    }
                    break;
#endif
                default:
                    event.value = 0;
                    break;
                }
                bmUsbEvent[id] &= ~BIT(i);
                if (event.value) {
                    event.arg  = (id == 0) ? (void *)"usb_host0" : (void *)"usb_host1";
                    have_post_event = 1;
                    device_event_notify(DEVICE_EVENT_FROM_USB_HOST, &event);
                }
            }
        }
    }

    if (have_post_event) {
        return DEV_ERR_NONE;
    } else {
        return DEV_ERR_UNKNOW_CLASS;
    }

}

const char *usb_host_valid_class_to_dev(const usb_dev id, u32 usbclass)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    struct usb_host_device *host_dev = &host_devices[usb_id];
    u32 itf_class;

    for (int i = 0; i < MAX_HOST_INTERFACE; i++) {
        if (host_dev->interface_info[i] &&
            host_dev->interface_info[i]->ctrl) {
            itf_class = host_dev->interface_info[i]->ctrl->interface_class;
            if (itf_class == usbclass) {
                switch (itf_class) {
                case USB_CLASS_MASS_STORAGE:
                    if (usb_id == 0) {
                        return "udisk0";
                    } else if (usb_id == 1) {
                        return "udisk1";
                    }
                    break;
                case USB_CLASS_ADB:
                    if (usb_id == 0) {
                        return "adb0";
                    } else if (usb_id == 1) {
                        return "adb1";
                    }
                    break;
                case USB_CLASS_AOA:
                    if (usb_id == 0) {
                        return "aoa0";
                    } else if (usb_id == 1) {
                        return "aoa1";
                    }
                    break;
                case USB_CLASS_HID:
                    if (usb_id == 0) {
                        return "hid0";
                    } else if (usb_id == 1) {
                        return "hid1";
                    }
                    break;
                case USB_CLASS_VIDEO:
                    if (usb_id == 0) {
                        return "uvc0";
                    } else if (usb_id == 1) {
                        return "uvc1";
                    }
                    break;
                }
            }
        }
    }
    return NULL;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_mount
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
u32 usb_host_mount(const usb_dev id, u32 retry, u32 reset_delay, u32 mount_timeout)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif

    u32 ret;
    struct usb_host_device *host_dev = &host_devices[usb_id];
    memset(host_dev, 0, sizeof(*host_dev));

    host_dev->private_data.usb_id = id;

    usb_sem_init(host_dev);
    usb_h_isr_reg(usb_id, 1, 0);

    ret = _usb_host_mount(usb_id, retry, reset_delay, mount_timeout);

    usb_otg_resume(usb_id);  //打开usb host之后恢复otg检测，需要在host_mount之后
    if (ret) {
        goto __exit_fail;
    }
    return usb_event_notify(host_dev, 0);

__exit_fail:
    usb_sie_disable(usb_id);
    usb_sem_del(host_dev);
    return ret;
}

static u32 _usb_host_unmount(const usb_dev usb_id)
{
    struct usb_host_device *host_dev = &host_devices[usb_id];

    struct usb_private_data *private_data = &host_dev->private_data;
    private_data->status = 0;

    usb_sem_post(host_dev);//拔掉设备时，让读写线程快速释放

    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        if (host_dev->interface_info[i] && host_dev->interface_info[i]->ctrl->set_power) {
            host_dev->interface_info[i]->ctrl->set_power(host_dev, 0);
            host_dev->interface_info[i] = NULL;
        }
    }

    usb_sie_close(usb_id);
    return DEV_ERR_NONE;
}

/* --------------------------------------------------------------------------*/
/**
 * @brief usb_host_unmount
 *
 * @param usb
 *
 * @return
 */
/* --------------------------------------------------------------------------*/
/* u32 usb_host_unmount(const usb_dev usb_id, char *device_name) */
u32 usb_host_unmount(const usb_dev id)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    u32 ret;
    struct usb_host_device *host_dev = &host_devices[usb_id];
    struct device_event event = {0};

#if (TCFG_UDISK_ENABLE && UDISK_READ_512_ASYNC_ENABLE)
    _usb_stor_async_wait_sem(host_dev);
#endif
    ret = _usb_host_unmount(usb_id);
    if (ret) {
        goto __exit_fail;
    }
    usb_sem_del(host_dev);

    /* printf("usb_host_unmount notify >>>>>>>>>>>\n"); */
    usb_event_notify(host_dev, 2);
    return DEV_ERR_NONE;

__exit_fail:
    return ret;
}

u32 usb_host_remount(const usb_dev id, u32 retry, u32 delay, u32 ot, u8 notify)
{
#if USB_MAX_HW_NUM > 1
    const usb_dev usb_id = id;
#else
    const usb_dev usb_id = 0;
#endif
    u32 ret;
    struct device_event event;

    ret = _usb_host_unmount(usb_id);
    if (ret) {
        goto __exit_fail;
    }

    struct usb_host_device *host_dev = &host_devices[usb_id];
    os_sem_set(host_dev->sem, 0);

    ret = _usb_host_mount(usb_id, retry, delay, ot);
    if (ret) {
        goto __exit_fail;
    }

    if (notify) {
        struct usb_host_device *host_dev = &host_devices[usb_id];
        usb_event_notify(host_dev, 1);
    }
    return DEV_ERR_NONE;

__exit_fail:
    return ret;
}

int usb_host_force_reset(const usb_dev usb_id)
{
    //预留以后加上hub的处理
    usb_h_force_reset(usb_id);
    return 0;
}
#endif
