#include "includes.h"
#include "app_config.h"
#include "device_drive.h"
/* #include "os/os_compat.h" */
#include "usb_config.h"
#include "usb/host/usb_host.h"
#include "usb/usb_phy.h"
#include "usb_ctrl_transfer.h"
#include "usb_storage.h"
#include "usb/host/usb_host.h"
#include "usb/usb_phy.h"
#include "device_drive.h"
#include "usb_ctrl_transfer.h"
#include "usb_bulk_transfer.h"
#include "usb_storage.h"
#include "usb_hid_keys.h"
/* #include "gamebox.h" */

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[MFI]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USB_APPLE_DOCK_EN
#include "apple_dock/iAP.h"
#include "apple_mfi.h"

static u8 usb_apple_descriptor_parser(struct usb_host_device *host_dev)
{
    u8 have_audio = 0;
    for (u8 i = 0; i < MAX_HOST_INTERFACE; i++) {
        if (host_dev->interface_info[i]) {
            switch (host_dev->interface_info[i]->ctrl->interface_class) {
#if TCFG_ADB_ENABLE
            case USB_CLASS_ADB:
                have_audio = 1;
                break;
#endif
#if TCFG_AOA_ENABLE
            case USB_CLASS_AOA:
                have_audio = 1;
                break;
#endif
            default:
                break;
            }
        }
    }
    return have_audio;
}


int usb_apple_mfi_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    u32 ret = DEV_ERR_NONE;
    struct usb_device_descriptor device_desc;
    ret = usb_get_device_descriptor(host_dev, &device_desc);

    if (apple_mfi_chip_online_lib()) {
        log_info("idV:0x%x, idP:0x%x \n", device_desc.idVendor, device_desc.idProduct);
        if ((device_desc.idVendor == 0x05AC) && ((device_desc.idProduct & 0xff00) == 0x1200)) {
            u8 cnt;
            u8 bDescriptorNum = 0;
            for (cnt = 0; cnt <= 3; cnt++) {
                struct usb_config_descriptor cfg_desc;
                ret = get_config_descriptor_add_value_l(host_dev, &cfg_desc, USB_DT_CONFIG_SIZE, cnt);
                /* check_usb_mount(ret); */

#if USB_H_MALLOC_ENABLE
                u8 *desc_buf = zalloc(cfg_desc.wTotalLength + 16);
                ASSERT(desc_buf, "desc_buf");
#else
                u8 desc_buf[128] = {0};
                cfg_desc.wTotalLength = min(sizeof(desc_buf), cfg_desc.wTotalLength);
#endif

                ret = get_config_descriptor_add_value_l(host_dev, desc_buf, cfg_desc.wTotalLength, cnt);
                /* check_usb_mount(ret); */

                /**********set configuration*********/
                /* ret = set_configuration(host_dev); */

                /* printf_buf(desc_buf, cfg_desc.wTotalLength); */
                /* ret |= usb_descriptor_parser(host_dev, desc_buf, cfg_desc.wTotalLength, &device_desc); */

                bDescriptorNum = desc_buf[5];

                u8 have_audio = 0;
                have_audio = usb_apple_descriptor_parser(host_dev);

#if USB_H_MALLOC_ENABLE
                log_info("free:desc_buf= %x\n", desc_buf);
                free(desc_buf);
#endif

                if (have_audio) {
                    break;
                }
            }
            ret = set_configuration_add_value(host_dev, bDescriptorNum);
            /* check_usb_mount(ret); */

            usb_mdelay(20);

            ret = usb_switch2slave(host_dev);
            /* check_usb_mount(ret); */

            usb_mdelay(20);

            ret = DEV_ERR_OFFLINE;
            /* break; */
        }
    }

    return 0;
}
#endif
