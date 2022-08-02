/**
 *
 * @file usb_video.c
 * @brief usb host video driver for jpeg stream
 * @author chenrixin@zh-jieli.com
 * @version 1.0
 * @date 2016-11-25
 */
#include <string.h>
#include "usb_ctrl_transfer.h"
#include "usb_host.h"
#include "usb_ctrl_transfer.h"
#include "host_uvc.h"
#include "usb_stack.h"
#include "asm/device_drive.h"
#include "app_config.h"
#include "usb/host/usb_host.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_HOST_UVC_ENABLE

/*static	USB_DEV uvc_ep3;*/
/*static	USB_DEV uvc_ep4;*/
#define     USB_DT_INTERFACE_HEADER_DESCRIPTOR_SIZE	0x0d
#define     STOP_TRANSFER   1

#define time_get(t,ms)	t = jiffies + msecs_to_jiffies(ms)
#define time_out(a,b)  (((long)(a) - (long)(b)) > 0)

#define  check_ret(ret)\
    if ((int)(ret) < DEV_ERR_NONE) {\
        log_error("err ret = %d\n",ret);\
        return ret;\
    }

struct usb_video_manager {
    u8 online;
    u8 open;
    u8 in_irq;
    u8 itf_base;
    const char *name;
    u8 *buffer;
    u32 frame_cnt;
    struct device dev;
};

static struct usb_video_manager uvc_manager[USB_MAX_HW_NUM];

int uvc_host_online(void)
{
    int id = 0;
    for (id = 0; id < 2; id++) {
        if (uvc_manager[id].online) {
            return id;
        }
    }
    return -EINVAL;
}
u32 uvc_host_get_frame(usb_dev usb_id)
{
    return uvc_manager[usb_id].frame_cnt;
}
static int uvc_set_power(struct usb_host_device *host_dev, u32 value)
{
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    hdl->online = value == 1 ? 1 : 0;
    return 0;
}
static int uvc_get_power(struct usb_host_device *host_dev, u32 value)
{
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    return hdl->online;
}
static int uvc_ioctl(struct usb_host_device *host_dev, u32 cmd, u32 arg)
{
    return 0;
}
static const struct interface_ctrl uvc_host_ops = {
    .interface_class = USB_CLASS_VIDEO,
    .set_power = uvc_set_power,
    .get_power = uvc_get_power,
    .ioctl = uvc_ioctl,
};
static struct usb_interface_info uvc_host_inf[USB_MAX_HW_NUM] = {
    {
        .ctrl = (struct interface_ctrl *) &uvc_host_ops,
        .dev.uvc = NULL,
    },
    {
        .ctrl = (struct interface_ctrl *) &uvc_host_ops,
        .dev.uvc = NULL,
    },
};

#define device_to_uvc(device)     (uvc_host_inf[usb_id].dev.uvc)

int usb_host_video_init(const usb_dev usb_id)
{
    int ret = 0;
    struct device *device;
    struct usb_host_device *host_dev;
    struct usb_uvc *uvc;
    u32 maxpsize;
    struct usb_video_manager *hdl = &uvc_manager[usb_id];

    device = &hdl->dev;
    if (!device) {
        return -DEV_ERR_OFFLINE;
    }
    host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -DEV_ERR_OFFLINE;
    }
    uvc = device_to_uvc(device);
    if (!uvc) {
        return -DEV_ERR_OFFLINE;
    }
    maxpsize = (uvc->wMaxPacketSize & 0x7ff) * (((uvc->wMaxPacketSize >> 11) & 0x3) + 1);
    if (uvc->xfer_type == USB_ENDPOINT_XFER_ISOC) {
        hdl->buffer = malloc(maxpsize);
        if (!hdl->buffer) {
            log_error("err no mem in hdl->buffer");
            ret = -DEV_ERR_INVALID_BUF;
            goto __exit_fail;
        }
        uvc->host_ep = usb_get_ep_num(usb_id, USB_DIR_IN, USB_ENDPOINT_XFER_ISOC);
        if ((int)uvc->host_ep < 0) {
            log_error("uvc get ep_num fail");
            ret = -DEV_ERR_INUSE;
            goto __exit_fail;
        }
        uvc->ep_buffer = usb_h_alloc_ep_buffer(usb_id, uvc->host_ep | USB_DIR_IN, maxpsize);
        if (!uvc->ep_buffer) {
            log_error("uvc alloc ep buffer fail");
            ret = -DEV_ERR_INVALID_BUF;
            goto __exit_fail;
        }
    } else if (uvc->xfer_type == USB_ENDPOINT_XFER_BULK) {
        hdl->buffer = malloc(maxpsize * 2);  //第二份用于保存bulk的上一包
        if (!hdl->buffer) {
            log_error("err no mem in hdl->buffer");
            ret = -DEV_ERR_INVALID_BUF;
            goto __exit_fail;
        }
        uvc->host_ep = usb_get_ep_num(usb_id, USB_DIR_IN, USB_ENDPOINT_XFER_BULK);
        if ((int)uvc->host_ep < 0) {
            log_error("uvc get ep_num fail");
            ret = -DEV_ERR_INUSE;
            goto __exit_fail;
        }
        uvc->ep_buffer = usb_h_alloc_ep_buffer(usb_id, uvc->host_ep | USB_DIR_IN, maxpsize * 2);  //bulk传输dma使用double buffer
        if (!uvc->ep_buffer) {
            log_error("uvc alloc ep buffer fail");
            ret = -DEV_ERR_INVALID_BUF;
            goto __exit_fail;
        }
    }
    ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, 0, hdl->itf_base + 1,  NULL, 0);
    /* check_ret(ret); */
    if (ret < DEV_ERR_NONE) {
        log_error("USB_REQ_SET_INTERFACE %d to 0 fail", 0);
        goto __exit_fail;
    }
    return DEV_ERR_NONE;

__exit_fail:
    if (uvc->ep_buffer) {
        usb_h_free_ep_buffer(usb_id, uvc->ep_buffer);
        uvc->ep_buffer = NULL;
    }
    if (hdl->buffer) {
        free(hdl->buffer);
        hdl->buffer = NULL;
    }
    return ret;
}

static struct device *uvc_host_device_find(const char *name)
{
    usb_dev usb_id = name[3] - '0';
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    if (usb_id >= 0 && usb_id <= 1 && name && hdl->name) {
        if (!strcmp(hdl->name, name)) {
            return &hdl->dev;
        }
    }
    return NULL;
}

static int uvc_dummy_stream_out(void *fd, int cnt, void *stream_list, int eof)
{
    return 0;
}

int usb_uvc_parser(struct usb_host_device *host_dev, u8 interface_num, u8 *pBuf)
{
    int len = 0;
    int cur_len;
    int cur_type;
    int cur_sub_type;
    int loop = 1;
    int bControlSize;
    int formart_num;
    u32 frame_num;
    u16 wTotalLength = 0;
    u32 wMaxPacketSize = 0;
    u32 AlternateSetting = 0;
    struct usb_endpoint_descriptor end_desc;
    struct usb_uvc *real_uvc = NULL;
    struct usb_private_data *private_data = &host_dev->private_data;
    usb_dev usb_id = private_data->usb_id;
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    u8 int_ep = 0;
    u8 ItfSubClass = 0;
    u8 *pformat;

    struct usb_uvc *uvc = (struct usb_uvc *)zalloc(sizeof(struct usb_uvc));
    if (!uvc) {
        log_error("err no mem in usb_uvc_parser\n\n");
        len = -DEV_ERR_INVALID_BUF;
        goto __err;
    }
    uvc->bfh = 0;
    uvc->cur_frame = 1;
    uvc->stream_out = uvc_dummy_stream_out;

    while (loop) {
        cur_len = *(pBuf + len);
        cur_type = *(pBuf + len + 1);
        cur_sub_type = *(pBuf + len + 2);

        if (!cur_len) {
            break;
        }

        if (cur_type == USB_DT_INTERFACE_ASSOCIATION) {
            if (*(pBuf + len + 4) != USB_CLASS_VIDEO ||
                *(pBuf + len + 5) != UVC_SC_VIDEO_INTERFACE_COLLECTION) {
                break;
            }
            //	composite device
            log_debug("USB_DT_INTERFACE_ASSOCIATION");
            log_debug_hexdump(pBuf + len, cur_len);
        } else if (cur_type == USB_DT_INTERFACE) {
            if (cur_len != USB_DT_INTERFACE_SIZE) {
                len = -USB_DT_INTERFACE;
                goto __err;
            }
            if (*(pBuf + len + 5) != USB_CLASS_VIDEO) {
                break;
            }
            log_debug("USB_DT_INTERFACE");
            log_debug_hexdump(pBuf + len, cur_len);
            AlternateSetting = *(pBuf + len + 3);
            log_debug("AlternateSetting:%d", AlternateSetting);

            if (*(pBuf + len + 6) == UVC_SC_VIDEOCONTROL) {
                ItfSubClass = UVC_SC_VIDEOCONTROL;
                hdl->itf_base = *(pBuf + len + 2);
            } else if (*(pBuf + len + 6) == UVC_SC_VIDEOSTREAMING) {
                ItfSubClass = UVC_SC_VIDEOSTREAMING;
            } else {
                log_error("invalid video subclass");
                len = -USB_DT_INTERFACE;
                goto __err;
            }
        } else if (cur_type == USB_DT_CS_INTERFACE) {
            if (ItfSubClass == UVC_SC_VIDEOCONTROL) {
                switch (cur_sub_type) {
                case UVC_VC_HEADER:
                    log_debug("VC_HEADER:%d %d, uvc ver %02x%02x", len, *(pBuf + len), *(pBuf + len + 4), *(pBuf + len + 3));
                    log_debug_hexdump(pBuf + len, cur_len);
                    break;

                case UVC_VC_INPUT_TERMINAL:
                    bControlSize = *(pBuf + len + 14);
                    log_debug("VC_INPUT_TERMINAL:%d %d %d", len, *(pBuf + len), bControlSize);
                    log_debug_hexdump(pBuf + len, cur_len);
                    break;

                case UVC_VC_PROCESSING_UNIT:
                    bControlSize = *(pBuf + len + 7);
                    log_debug("VC_PROCESSING_UNIT:%d %d %d", len, *(pBuf + len), bControlSize);
                    log_debug_hexdump(pBuf + len, cur_len);
                    break;

                case UVC_VC_OUTPUT_TERMINAL:
                    bControlSize = *(pBuf + len + 8);
                    log_debug("VC_OUTPUT_TERMINAL:%d %d %d", len, *(pBuf + len), bControlSize);
                    log_debug_hexdump(pBuf + len, cur_len);
                    break;

                case UVC_VC_SELECTOR_UNIT:
                    break;

                case UVC_VC_EXTENSION_UNIT:
                    bControlSize = *(pBuf + len + 20);
                    log_debug("VC_EXTENSION_UNIT:%d %d %d %d",
                              len,
                              *(pBuf + len),
                              bControlSize,
                              *(pBuf + len + 22 + bControlSize));
                    log_debug_hexdump(pBuf + len, cur_len);
                    break;

                default :
                    log_debug("Unrecognized VC UNIT:%d %d", len, *(pBuf + len));
                    log_debug_hexdump(pBuf + len, cur_len);
                    break;
                }
            } else if (ItfSubClass == UVC_SC_VIDEOSTREAMING) {
                switch (cur_sub_type) {
                case UVC_VS_INPUT_HEADER:
                    formart_num = *(pBuf + len + 3);
                    bControlSize = *(pBuf + len + 12);
                    memcpy(&wTotalLength, pBuf + len + 4, 2);
                    log_debug("VS_INPUT_HEADER:%d %d %d",
                              wTotalLength,
                              formart_num,
                              bControlSize);
                    log_debug_hexdump(pBuf + len, *(pBuf + len));
                    wTotalLength -= *(pBuf + len);
                    break;

                case UVC_VS_FORMAT_MJPEG:
                    pformat = pBuf + len;
                    frame_num = pformat[4];
                    uvc->mjpeg_format_index = pformat[3];
                    wTotalLength -= pformat[0];
                    log_debug("find mjpeg format descriptor %d %d %d",
                              frame_num,
                              pformat[3],
                              wTotalLength);
                    log_debug_hexdump(pformat, pformat[0]);
                    uvc->cur_frame = pformat[6];
                    log_debug("default frame id %d", uvc->cur_frame);
                    pformat += pformat[0];
                    uvc->reso_num = frame_num;
                    real_uvc = (struct usb_uvc *)zalloc(sizeof(struct usb_uvc) + sizeof(struct uvc_frame_info) * (1 + frame_num));
                    if (!real_uvc) {
                        log_error("err no mem in usb_uvc_parser\n\n");
                        len = -DEV_ERR_INVALID_BUF;
                        goto __err;
                    }
                    for (int i = 0; i < frame_num; i++) {
                        memcpy(&real_uvc->pframe[i].width, &pformat[5], 2);
                        memcpy(&real_uvc->pframe[i].height, &pformat[7], 2);
                        log_debug("frame info %d %d x %d",
                                  1 + i,
                                  real_uvc->pframe[i].width,
                                  real_uvc->pframe[i].height);
                        log_debug_hexdump(pformat, pformat[0]);
                        u32 f_interval = 0;
                        u32 f_interval_step = 0;
                        memcpy(&f_interval, pformat + 21, 4);
                        log_debug("default frame interval %d, fps %d", f_interval, 10000000 / f_interval);
                        uvc->fps = (f_interval ? (10000000 / f_interval) : 0);
                        if (pformat[25] == 0) {
                            memcpy(&f_interval, pformat + 26, 4);
                            log_debug("min frame interval %d, fps %d", f_interval, 10000000 / f_interval);
                            memcpy(&f_interval, pformat + 30, 4);
                            log_debug("max frame interval %d, fps %d", f_interval, 10000000 / f_interval);
                            memcpy(&f_interval_step, pformat + 34, 4);
                            log_debug("frame interval step %d, fps step %d", f_interval_step,
                                      f_interval_step ? (10000000 / f_interval_step) : 0);
                        } else {
                            for (int icnt = 0; icnt < pformat[25]; icnt++) {
                                memcpy(&f_interval, pformat + 26 + 4 * icnt, 4);
                                log_debug("frame interval %d %d, fps %d", icnt, f_interval, 10000000 / f_interval);
                            }
                        }
                        wTotalLength -= pformat[0];
                        cur_len += pformat[0];
                        pformat += pformat[0];
                    }
                    break;

                case UVC_VS_FORMAT_UNCOMPRESSED:
                    pformat = pBuf + len;
                    frame_num = pformat[4];
                    wTotalLength -= pformat[0];
                    log_debug("find yuy2 format descriptor %d %d %d",
                              frame_num,
                              pformat[3],
                              wTotalLength);
                    log_debug_hexdump(pformat, pformat[0]);
                    pformat += pformat[0];
                    for (int i = 0; i < frame_num; i++) {
                        u16 width, heigth;
                        memcpy(&width, &pformat[5], 2);
                        memcpy(&heigth, &pformat[7], 2);
                        log_debug("frame info %d %d x %d", 1 + i, width,
                                  heigth);
                        log_debug_hexdump(pformat, pformat[0]);
                        wTotalLength -= pformat[0];
                        cur_len += pformat[0];
                        pformat += pformat[0];
                    }
                    break;

                case UVC_VS_STILL_IMAGE_FRAME:
                    wTotalLength -= cur_len;
                    log_debug("find still image format descriptor %d", wTotalLength);
                    log_debug_hexdump(pBuf + len, cur_len);
                    break;

                case UVC_VS_COLORFORMAT:
                    wTotalLength -= cur_len;
                    log_debug("find colorformat descriptor %d", wTotalLength);
                    log_debug_hexdump(pBuf + len, cur_len);
                    break;

                default:
                    wTotalLength -= cur_len;
                    log_debug("unrecognized vs descriptor %d", wTotalLength);
                    log_debug_hexdump(pBuf + len, cur_len);
                    break;
                }
            }
        } else if (cur_type == USB_DT_CS_ENDPOINT) {
            if (cur_len != 5) {
                len = -USB_DT_CS_ENDPOINT;
                goto __err;
            }

            log_debug("USB_DT_CS_ENDPOINT");
            log_debug_hexdump(pBuf + len, cur_len);
        } else if (cur_type == USB_DT_ENDPOINT) {
            if (cur_len != USB_DT_ENDPOINT_SIZE) {
                len = -USB_DT_ENDPOINT;
                goto __err;
            }

            log_debug("USB_DT_ENDPOINT");
            log_debug_hexdump(pBuf + len, cur_len);
            memcpy(&end_desc, pBuf + len, USB_DT_ENDPOINT_SIZE);

            if (end_desc.bEndpointAddress & USB_DIR_IN) {
                log_debug("Endpoint IN %x", end_desc.bEndpointAddress);
            } else {
                log_debug("Endpoint OUT %x", end_desc.bEndpointAddress);
            }

            log_debug("wMaxPacketSize  = %d, %d additional transaction", end_desc.wMaxPacketSize & 0x7ff, end_desc.wMaxPacketSize >> 11);

            if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
                log_debug(">>>>>XFER_BULK\n");
            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
                log_debug(">>>>>XFER_INT = %d\n", end_desc.bInterval);
                int_ep = 1;
            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) {
                log_debug(">>>>>XFER_ISOC = %d\n", end_desc.bInterval);
            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_CONTROL) {
                log_debug(">>>>>XFER_CONTROL\n");
            }

            if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
                uvc->int_ep.epin = end_desc.bEndpointAddress & 0x0f;
                uvc->int_ep.interval = end_desc.bInterval;
                uvc->int_ep.wMaxPacketSize = end_desc.wMaxPacketSize;
                uvc->int_ep.enable = 1;
            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) {
                if (end_desc.wMaxPacketSize > wMaxPacketSize) {
                    wMaxPacketSize = end_desc.wMaxPacketSize;
                    uvc->ep = end_desc.bEndpointAddress & 0x0f;
                    uvc->wMaxPacketSize = end_desc.wMaxPacketSize;
                    uvc->interval = end_desc.bInterval;
                    uvc->if_alt_num = AlternateSetting;
                    uvc->xfer_type = USB_ENDPOINT_XFER_ISOC;
                }
            } else if ((end_desc.bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
                if (end_desc.wMaxPacketSize > wMaxPacketSize) {
                    wMaxPacketSize = end_desc.wMaxPacketSize;
                    uvc->ep = end_desc.bEndpointAddress & 0x0f;
                    uvc->wMaxPacketSize = end_desc.wMaxPacketSize;
                    uvc->interval = end_desc.bInterval;
                    uvc->if_alt_num = AlternateSetting;
                    uvc->xfer_type = USB_ENDPOINT_XFER_BULK;
                }
            }
            /*log_d("=======[%s - %d] >>>> \nif_alt_num : %d\ninterval : %d\nwMaxPacketSize : %d\n", __FUNCTION__,*/
            /*__LINE__, AlternateSetting, end_desc.bInterval, end_desc.wMaxPacketSize);*/
        } else {
            log_error("error %x", len);
            log_debug_hexdump(pBuf + len, 16);
            loop = 0;
            cur_len = 0;
        }
        len += cur_len;
    }
    if (real_uvc) {
        memcpy(real_uvc, uvc, sizeof(*uvc));
    }
    free(uvc);
    uvc = NULL;
    uvc_host_inf[usb_id].dev.uvc = real_uvc;
    host_dev->interface_info[interface_num] = &uvc_host_inf[usb_id];
    hdl->dev.private_data = host_dev;
    hdl->name = usb_id ? "uvc1" : "uvc0";
    hdl->frame_cnt = 0;
    return len;

__err:
    log_error("error in usb_uvc_parser");
    if (real_uvc) {
        free(real_uvc);
        real_uvc = NULL;
    }
    if (uvc) {
        free(uvc);
        uvc = NULL;
    }
    return len;
}

#define     USB_GET_REQ		(USB_DIR_IN|USB_TYPE_CLASS|USB_RECIP_INTERFACE)
#define     USB_SET_REQ		(USB_DIR_OUT|USB_TYPE_CLASS|USB_RECIP_INTERFACE)

int uvc_host_get_backlight_compensation(void *fd, u32 *iostatus)
{
    struct device *device = (struct device *)fd;
    s32 ret;
    u32 tmp_buf = 0;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    usb_dev usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    //PU_BACKLIGHT_COMPENSATION_CONTROL
    ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, 0x0100, (0x02 << 8) | hdl->itf_base, (u8 *)&tmp_buf, 2);
    memcpy(iostatus, &tmp_buf, 4);
    /* log_debug("%x", *iostatus); */
    check_ret(ret);
    return DEV_ERR_NONE;
}

int uvc_host_req_processing_unit(void *fd, struct uvc_processing_unit *pu)
{
    struct device *device = (struct device *)fd;
    int err = 0;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    usb_dev usb_id = host_device2id(host_dev);

    err = usb_control_msg(host_dev, pu->request, pu->type, pu->value, pu->index, (u8 *)pu->buf, pu->len);

    return err;
}

static u8 Video_Probe_Commit_Control[26] = {
    0x00, 0x00,                     //bmHint
    0x01,                           //FormatIndex
    0x01,                           //FrameIndex
    DW1BYTE(FRAME_FPS_0), DW2BYTE(FRAME_FPS_0), DW3BYTE(FRAME_FPS_0), DW4BYTE(FRAME_FPS_0), //dwFrameInterval Frame interval in 100 ns units.
    0x00, 0x00,                     //KeyFrameRate
    0x00, 0x00,                     //PFrameRate
    0x00, 0x27,                     //CompQuanlity
    0x00, 0x00,                     //CompWindowsSize
    0x00, 0x00, //0x32,0x00,                      //Delay
    DW1BYTE(300 * 1024), DW2BYTE(300 * 1024), DW3BYTE(300 * 1024), DW4BYTE(300 * 1024), //MaxVideoFrameSize
    LOBYTE(512), HIBYTE(512), 0x00, 0x00, //MaxPayLoadTransferSize
};

static u32 StreamInterfaceNum_get_cur(struct device *device, u16 id)
{
    u32 ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    usb_dev usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    id = cpu_to_be16(id);
    ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, id, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    //log_debug("%s:%x\n",__FUNCTION__,id);
    check_ret(ret);
    return DEV_ERR_NONE;
}
static u32 StreamInterfaceNum_set_cur(struct device *device, u16 id)
{
    u32 ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    usb_dev usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    struct usb_uvc *uvc = device_to_uvc(device);
    if (!uvc) {
        return -1;
    }
    id = cpu_to_be16(id);
    Video_Probe_Commit_Control[2] = uvc->mjpeg_format_index;
    Video_Probe_Commit_Control[3] = uvc->cur_frame;
    log_debug("%s:%x\n", __func__, uvc->cur_frame);
    ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, id, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    //log_debug("%s:%x\n",__FUNCTION__,usb_id);
    check_ret(ret);
    return DEV_ERR_NONE;
}
static void Stream_head_analysis(u8 *rx_addr)
{
#if 0
    u32 frame_cnt = 0, scr = 0;
    u16 sof_count = 0;
    /// Stream Head
    rx_addr[0x00] = 0x0C;//Head Length
    rx_addr[0x01] = hdr->bfh_val; //BFH[0] [7:0]={EOH,ERR,STI,RES,SCR,PTS,EOF,FID}
    /* rx_addr[0x1] &= ~(BIT(2) | BIT(3) | BIT(5) | BIT(6)); */
    frame_cnt = hdr->frame_cnt;
    rx_addr[0x02] = frame_cnt ;//PTS[7:0]
    rx_addr[0x03] = (frame_cnt >> 8); //PTS[15:8]
    rx_addr[0x04] = (frame_cnt >> 16) ; //PTS[23:16]
    rx_addr[0x05] = (frame_cnt >> 24) ; //PTS[31:24]
    scr = hdr->scr;
    rx_addr[0x06] = DW1BYTE(scr);//SCR[7:0]
    rx_addr[0x07] = DW2BYTE(scr);//SCR[15:8]
    rx_addr[0x08] = DW3BYTE(scr);//SCR[23:16]
    rx_addr[0x09] = DW4BYTE(scr);//SCR[31:24]
    sof_count = hdr->sof_cnt;
    rx_addr[0x0A] = LOBYTE(sof_count); //SCR[42:32] 1KHz SOF token counter
    rx_addr[0x0B] = HIBYTE(sof_count); //SCR[47:42] res set to 0.
#endif
}

static struct uvc_stream_list uvc_global_l[USB_MAX_HW_NUM][ISO_INTR_CNT];

static void vs_iso_handler(struct usb_host_device *host_dev, u32 ep)
{
    usb_dev usb_id;
    int rx_len;
    u8 *start_addr;
    u8 error_flag;
    u8 stream_state = 0;
    u32 maxpsize;
    static u32 trans_err_cnt[USB_MAX_HW_NUM] = {0};
    struct device *device;
    struct usb_uvc *uvc;
    if (!host_dev) {
        return;
    }
    usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    device = &hdl->dev;
    if (!device) {
        return;
    }
    uvc = device_to_uvc(device);
    if (!host_dev) {
        return;
    }
    hdl->in_irq = 1;
    maxpsize = (uvc->wMaxPacketSize & 0x7ff) * (((uvc->wMaxPacketSize >> 11) & 0x3) + 1);
    rx_len = usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, hdl->buffer, maxpsize, USB_ENDPOINT_XFER_ISOC, 0);
    if (rx_len < 0) {
        uvc->error_frame = 1;
        if (uvc->stream_out) {
            uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
        }
        trans_err_cnt[usb_id]++;
        if (trans_err_cnt[usb_id] >= 1000) {
            trans_err_cnt[usb_id] = 0;
            usb_host_force_reset(usb_id);
        }
    }
    usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_ISOC, 1);
    start_addr = hdl->buffer;
    struct uvc_stream_list *uvc_list = uvc_global_l[usb_id];
    if (rx_len > 0 && rx_len > start_addr[0]) {
        rx_len -= start_addr[0];
        error_flag = start_addr[1];
        /* if (!uvc->error_frame) { */
        uvc_list->addr = start_addr + start_addr[0];
        uvc_list->length = rx_len;
        /* } */
        if (start_addr[0] != 0x0c && start_addr[0] != 0x02) {
            log_error("start_addr = %x", start_addr[0]);
            uvc->error_frame = 1;
            if (uvc->stream_out) {
                uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
            }
        }
        //BFH[0] [7:0]={EOH,ERR,STI,RES,SCR,PTS,EOF,FID}
        if ((error_flag & BIT(6)) && uvc->error_frame == 0) {
            log_error("stream_err");
            uvc->error_frame = 1;
            //send_error_info
            if (uvc->stream_out) {
                uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
                stream_state = STREAM_ERR;
            }
        }
        if (error_flag & BIT(1)) { //endof frame
            if (!uvc->error_frame) {
                if (uvc->stream_out) {
                    uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_EOF);
                    stream_state = STREAM_EOF;
                    hdl->frame_cnt++;
                }
            }
            uvc->error_frame = 0;//clear error_frame pending
        } else {
            if (!uvc->error_frame) {
                if (uvc->bfh != (error_flag & BIT(0))) { //new frame
                    uvc->bfh = error_flag & BIT(0);//clear error_frame pending
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                        stream_state = STREAM_SOF;
                    }
                } else {
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                        stream_state = STREAM_NO_ERR;
                    }
                }
            }
        }
    }
    hdl->in_irq = 0;
}
static void vs_bulk_handler(struct usb_host_device *host_dev, u32 ep)
{
    usb_dev usb_id;
    u8 _header = 0;
    int rx_len;
    u8 *start_addr;
    u8 error_flag;
    u8 stream_state = 0;
    u8 *last_buf;
    static u32 trans_err_cnt[USB_MAX_HW_NUM] = {0};
    static u32 last_len[USB_MAX_HW_NUM];
    static u32 frame_len[USB_MAX_HW_NUM];
    u32 maxpsize;
    struct device *device;
    struct usb_uvc *uvc;
    if (!host_dev) {
        return;
    }
    usb_id = host_device2id(host_dev);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    device = &hdl->dev;
    if (!device) {
        return;
    }
    uvc = device_to_uvc(device);
    if (!host_dev) {
        return;
    }
    hdl->in_irq = 1;
    maxpsize = (uvc->wMaxPacketSize & 0x7ff) * (((uvc->wMaxPacketSize >> 11) & 0x3) + 1);
    rx_len = usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, hdl->buffer, maxpsize, USB_ENDPOINT_XFER_BULK, 0);
    if (rx_len < 0) {
        uvc->error_frame = 1;
        if (uvc->stream_out) {
            uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
        }
        trans_err_cnt[usb_id]++;
        if (trans_err_cnt[usb_id] >= 1000) {
            trans_err_cnt[usb_id] = 0;
            usb_host_force_reset(usb_id);
        }
    }
    usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_BULK, 1);
    start_addr = hdl->buffer;
    struct uvc_stream_list *uvc_list = uvc_global_l[usb_id];
    if (rx_len > 0) {
        _header = 0;
        last_buf = &hdl->buffer[maxpsize];
        //BFH[0] [7:0]={EOH,ERR,STI,RES,SCR,PTS,EOF,FID}
        if (start_addr[0] == 0x0c) {
            if ((start_addr[1] & 0x8c) == 0x8c) {
                if (start_addr[0x0c] == 0xff && start_addr[0x0d] == 0xd8) {
                    _header = 1;
                }
            }
        } else if (start_addr[0] == 0x02) {
            if ((start_addr[1] & 0x8c) == 0x80) {
                if (start_addr[0x02] == 0xff && start_addr[0x03] == 0xd8) {
                    _header = 1;
                }
            }
        }
        if (_header) {
            if (rx_len >= start_addr[0]) {
                rx_len -= start_addr[0];
            }
            /* log_debug_hexdump(start_addr, rx_len + start_addr[0]); */
            uvc_list->addr = start_addr + start_addr[0];
            uvc_list->length = rx_len;
            error_flag = start_addr[1];
            if ((error_flag & BIT(6)) && (uvc->error_frame == 0)) {
                frame_len[usb_id] = 0;
                uvc->error_frame = 1;
                if (uvc->stream_out) {
                    uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
                    stream_state = STREAM_ERR;
                }
            }
            if (uvc->bfh != (error_flag & BIT(0))) { //new frame
                uvc->bfh = error_flag & BIT(0);//clear error_frame pending
                if (last_len[usb_id] >= 2 &&
                    last_buf[last_len[usb_id] - 2] == 0xff &&
                    last_buf[last_len[usb_id] - 1] == 0xd9) {
                    if (!uvc->error_frame) {
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 0, uvc_list, STREAM_EOF);
                            stream_state = STREAM_EOF;
                            hdl->frame_cnt++;
                        }
                        //log_info("frame_len[usb_id] %u", frame_len[usb_id]);
                    }

                    /* puts(" new\n"); */
                    if (uvc_list->length) {
                        if (uvc->stream_out) {
                            uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_SOF);
                            stream_state = STREAM_SOF;
                        }
                        uvc->error_frame = 0;
                        frame_len[usb_id] = rx_len;
                    }
                } else {
                    uvc->error_frame = 1;
                    if (uvc->stream_out) {
                        uvc->stream_out(uvc->priv, -1, NULL, STREAM_ERR);
                        stream_state = STREAM_ERR;
                    }
                    frame_len[usb_id] = 0;
                }
            }
            last_len[usb_id] = rx_len;
            memcpy(last_buf, start_addr + start_addr[0], rx_len);
        } else {
            uvc_list->addr = start_addr;
            uvc_list->length = rx_len;
            if (!uvc->error_frame) {
                /* log_debug_hexdump(start_addr, rx_len); */
                if (uvc->stream_out) {
                    uvc->stream_out(uvc->priv, 1, uvc_list, STREAM_NO_ERR);
                    stream_state = STREAM_NO_ERR;
                }
                frame_len[usb_id] += rx_len;
            }
            last_len[usb_id] = rx_len;
            memcpy(last_buf, start_addr, rx_len);
        }
    }
    hdl->in_irq = 0;
}
static void usb_intr_config(struct device *device, u8 ep, u8 enable)
{
    u8 id = usbpriv_to_usbid(usbdev_to_usbpriv(device_to_usbdev(device)));
    if (enable) {
        usb_set_intr_rxe(id, ep);
    } else {
        usb_clr_intr_rxe(id, ep);
        usb_write_rxcsr(id, ep, RXCSRH_ClrDataTog | RXCSRH_FlushFIFO);
        usb_write_rxinterval(id, ep, 0);
    }
}
static void iso_ep_rx_init(struct device *device)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_uvc *uvc = device_to_uvc(device);
    u8 devnum = host_dev->private_data.devnum;

    usb_write_rxfuncaddr(usb_id, uvc->host_ep, devnum);
    usb_h_set_ep_isr(host_dev, uvc->host_ep | USB_DIR_IN, vs_iso_handler, host_dev);
    usb_h_ep_config(usb_id, uvc->host_ep | USB_DIR_IN, USB_ENDPOINT_XFER_ISOC, 1, uvc->interval, uvc->ep_buffer, uvc->wMaxPacketSize);
    usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_ISOC, 1);
}
static void bulk_ep_rx_init(struct device *device)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_uvc *uvc = device_to_uvc(device);
    u8 devnum = host_dev->private_data.devnum;

    usb_write_rxfuncaddr(usb_id, uvc->host_ep, devnum);
    usb_h_set_ep_isr(host_dev, uvc->host_ep | USB_DIR_IN, vs_bulk_handler, host_dev);
    usb_h_ep_config(usb_id, uvc->host_ep | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 1, uvc->interval, uvc->ep_buffer, uvc->wMaxPacketSize);
    usb_h_ep_read_async(usb_id, uvc->host_ep, uvc->ep, NULL, 0, USB_ENDPOINT_XFER_BULK, 1);
}

int uvc_host_close_camera(void *fd)
{
    int ret;
    struct device *device;
    struct usb_uvc *uvc;
    struct usb_host_device *host_dev;
    u8 status;
    u32 timeout;
    device = (struct device *)fd;
    if (!device) {
        return DEV_ERR_NONE;
    }
    host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return DEV_ERR_NONE;
    }
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    uvc = device_to_uvc(device);
    if (!uvc) {
        return DEV_ERR_NONE;
    }
    struct usb_video_manager *hdl = &uvc_manager[usb_id];
    status = host_dev_status(host_dev);
    time_get(timeout, 1000);

    if (uvc->xfer_type == USB_ENDPOINT_XFER_ISOC) {
#if (STOP_TRANSFER == 0)
#if CAMERA_CLOSE_BY_IRQ
        while (hdl->in_irq) {
            if (time_out(jiffies, timeout)) {
                log_error("wait uvc exiting isr timeout");
                break;
            }
        }
        usb_intr_config(device, uvc->host_ep, 0);
#endif
        if (uvc->camera_open) {
#if !CAMERA_CLOSE_BY_IRQ
            uvc->stream_out = uvc_dummy_stream_out;
#endif
            return DEV_ERR_NONE;
        }
        if (status) {
            ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, 0, hdl->itf_base + 1, NULL, 0);
            log_debug("USB_REQ_SET_INTERFACE %d to %d", 0x01, 0);
        }

#else  /*STOP_TRANSFER*/
        if (uvc->camera_open) {
#if !CAMERA_CLOSE_BY_IRQ
            uvc->stream_out = uvc_dummy_stream_out;
#endif
#if CAMERA_CLOSE_BY_IRQ
#if ISO_INTR_MODE == 2
            //TODO
#elif ISO_INTR_MODE == 1
            //TODO
#else
            //TODO
#endif
            while (hdl->in_irq) {
                if (time_out(jiffies, timeout)) {
                    log_error("wait uvc exiting isr timeout");
                    break;
                }
            }
            usb_intr_config(device, uvc->host_ep, 0);
#endif
            if (status) {
                ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, 0, hdl->itf_base + 1, NULL, 0);
                log_debug("USB_REQ_SET_INTERFACE %d to %d", 0x01, 0);
            }
            uvc->camera_open = 0;
        }
#endif

    } else if (uvc->xfer_type == USB_ENDPOINT_XFER_BULK) {
#if !CAMERA_CLOSE_BY_IRQ
        uvc->stream_out = uvc_dummy_stream_out;
#endif
        while (hdl->in_irq) {
            if (time_out(jiffies, timeout)) {
                log_error("wait uvc exiting isr timeout");
                break;
            }
        }
        usb_intr_config(device, uvc->host_ep, 0);
        uvc->camera_open = 0;
        if (status) {
            //clear endpoint feature, halt endpoint
            ret = usb_control_msg(host_dev, USB_REQ_CLEAR_FEATURE, 0x02, 0, uvc->ep | USB_DIR_IN, NULL, 0);
            log_debug("USB_REQ_CLEAR_FEATURE");
            check_ret(ret);

            Video_Probe_Commit_Control[0]  = 0;
            Video_Probe_Commit_Control[1]  = 0;
            Video_Probe_Commit_Control[2]  = uvc->mjpeg_format_index;
            Video_Probe_Commit_Control[3]  = uvc->cur_frame;
            ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x0100, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
            check_ret(ret);
            log_debug("Video Probe Control SET_CUR");
            log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

            ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, 0x0100, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
            check_ret(ret);
            log_debug("Video Probe Control GET_CUR");
            log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

            ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x0100, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
            check_ret(ret);
            log_debug("Video Probe Control SET_CUR");
            log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

            ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, 0x0100, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
            check_ret(ret);
            log_debug("Video Probe Control GET_CUR");
            log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

            u8 tmp_buf[10];
            ret = usb_control_msg(host_dev, UVC_GET_DEF, USB_GET_REQ, 0x1400, (0x01 << 8) | hdl->itf_base, tmp_buf, 0x0a);
            check_ret(ret);
            log_debug("CameraTerminal GET_DEF");
            log_debug_hexdump(tmp_buf, ret);

            memset(tmp_buf, 0, sizeof(tmp_buf));
            ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x1400, (0x01 << 8) | hdl->itf_base, tmp_buf, 0x0a);
            check_ret(ret);
            log_debug("CameraTerminal SET_CUR");
            log_debug_hexdump(tmp_buf, ret);
        }
    }
    return DEV_ERR_NONE;
}

static int usb_uvc_info(struct device *device);

int uvc_host_open_camera(void *fd)
{
    u32 ret;
    struct device *device;
    struct usb_uvc *uvc;
    struct usb_host_device *host_dev;
    device = (struct device *)fd;
    if (!device) {
        return -DEV_ERR_OFFLINE;
    }
    /*ret = uvc_host_close_camera(device);*/
    host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -DEV_ERR_OFFLINE;
    }
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    uvc = device_to_uvc(device);
    if (!uvc) {
        return -DEV_ERR_OFFLINE;
    }
    struct usb_video_manager *hdl = &uvc_manager[usb_id];

    uvc->bfh = 0;
    uvc->error_frame = 1;
    if (uvc->xfer_type == USB_ENDPOINT_XFER_ISOC) {
        log_debug("USB_ENDPOINT_XFER_ISOC\n");
#if (STOP_TRANSFER == 0)
        if (uvc->camera_open) {
#if !CAMERA_CLOSE_BY_IRQ
            uvc->stream_out = uvc->bak_stream_out;
#endif
            log_debug("warning: UVC ALREADY OPEN\n\n\n");
            return 0;
        }
        usb_uvc_info(device);
        ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, uvc->if_alt_num, hdl->itf_base + 1, NULL, 0);
        log_debug("USB_REQ_SET_INTERFACE %d to %d", hdl->itf_base + 1, uvc->if_alt_num);
        check_ret(ret);
        iso_ep_rx_init(device);
        uvc->camera_open = 1;
#else  /*STOP_TRANSFER*/
        if (!uvc->camera_open) {
#if !CAMERA_CLOSE_BY_IRQ
            uvc->stream_out = uvc->bak_stream_out;
#endif
            usb_uvc_info(device);
            ret = usb_control_msg(host_dev, USB_REQ_SET_INTERFACE, 0x01, uvc->if_alt_num, hdl->itf_base + 1, NULL, 0);
            log_debug("USB_REQ_SET_INTERFACE %d to %d", hdl->itf_base + 1, uvc->if_alt_num);
            check_ret(ret);
            iso_ep_rx_init(device);
            uvc->camera_open = 1;
        }
#endif

    } else if (uvc->xfer_type == USB_ENDPOINT_XFER_BULK) {
        log_debug("USB_ENDPOINT_XFER_BULK\n");
        usb_uvc_info(device);
        bulk_ep_rx_init(device);
#if !CAMERA_CLOSE_BY_IRQ
        uvc->stream_out = uvc->bak_stream_out;
#endif
        uvc->camera_open = 1;
    }
    log_info("uvc usb open ok");
    return DEV_ERR_NONE;
}

#define USE_HUSB    0
static int usb_uvc_info(struct device *device)
{
    struct uvc_parm p;
#if USE_HUSB
    u8 tmp_Video_Probe_Commit_Control[26] = {
        0x00, 0x00,
        0x02,//1 yuv 2 mjpg
        0x01,//6 1280*720 5 640*360 1 640*480
        0x15, 0x16, 0x05, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x00, 0x00,
        0x20, 0x00,
        0x00, 0x60, 0x09, 0x00,
        0x00, 0x0c, 0x00, 0x00
    };
    StreamInterfaceNum_get_cur(device, 1);
    memcpy(Video_Probe_Commit_Control, tmp_Video_Probe_Commit_Control, 26);
    StreamInterfaceNum_set_cur(device, 1);

    StreamInterfaceNum_get_cur(device, 1);
    log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));

    memcpy(Video_Probe_Commit_Control, tmp_Video_Probe_Commit_Control, 26);
    StreamInterfaceNum_set_cur(device, 1);
    memcpy(Video_Probe_Commit_Control, tmp_Video_Probe_Commit_Control, 26);
    StreamInterfaceNum_set_cur(device, 2);


    //StreamInterfaceNum_get_cur(device,1);
    log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
#else
    int ret = 0;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    usb_dev usb_id = host_device2id(host_dev);
    struct usb_uvc *uvc = device_to_uvc(device);
    struct usb_video_manager *hdl = &uvc_manager[usb_id];

    ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, 0x0100, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Probe Control GET_CUR");
    log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x0100, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Probe Control SET_CUR");
    log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    ret = usb_control_msg(host_dev, UVC_GET_MIN, USB_GET_REQ, 0x0100, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Probe Control GET_MIN");
    log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    ret = usb_control_msg(host_dev, UVC_GET_MAX, USB_GET_REQ, 0x0100, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Probe Control GET_MAX");
    log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    ret = usb_control_msg(host_dev, UVC_GET_CUR, USB_GET_REQ, 0x0100, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Probe Control GET_CUR");
    log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

    Video_Probe_Commit_Control[2] = uvc->mjpeg_format_index;
    Video_Probe_Commit_Control[3] = uvc->cur_frame;
    ret = usb_control_msg(host_dev, UVC_SET_CUR, USB_SET_REQ, 0x0200, hdl->itf_base + 1, Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    log_debug("Video Commit Control SET_CUR");
    log_debug_hexdump(Video_Probe_Commit_Control, sizeof(Video_Probe_Commit_Control));
    check_ret(ret);

#endif
    return DEV_ERR_NONE;
}

static void toogle_port()
{

}

struct status_packet_format {
    u8 bStatusType;//bit0-3 : 0=Reserved, 1=VideoControl interface, 2 = VideoStreaming interface
    u8 bOriginator;//ID of Teriminal, Unit or Interface
};

struct status_vc_packet {
    u8 bEvent;//0x0 : Control Change , 0x1 - 0xFF : Reserved
    u8 bSelector;//Report the Control Selector of th control that issued the interrupt
    u8 bAttribute;//0x0:Control value change, 0x1:Control info change, 0x2:Control failure change, 0x3-0xff : Reserved
    u8 bValue;//0x0:Equivalent to the result of a GET_CUR request, 0x1:GET_INFO request, 0x2:GET_CUR request on VC_REQUEST_ERROR_CODE_CONTROL
};

struct status_vs_packet {
    u8 bEvent;//0x0:Button Press, 0x1-0xff:Stream Error
    u8 bValue;//Button Press--0x0:button released, 0x1:button pressed
};

void *uvc_host_open(void *arg)
{
    s32 ret;
    struct uvc_host_param *info = (struct uvc_host_param *)arg;
    struct usb_uvc *uvc;
    struct device *device;
    struct usb_host_device *host_dev;
    struct usb_video_manager *hdl;
    usb_dev usb_id;

    device = uvc_host_device_find(info->name);
    if (device) {
        atomic_inc(&(device->ref));
        host_dev = device_to_usbdev(device);
        if (!host_dev) {
            return NULL;
        }
        usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
        hdl = &uvc_manager[usb_id];
        if (hdl->open) {
            log_error("uvc host reopen !!!");
            return device;
        }
        uvc = device_to_uvc(device);
        if (uvc) {
            uvc->priv = info->priv;
#if !CAMERA_CLOSE_BY_IRQ
            if (!uvc->camera_open) {
                uvc->stream_out = info->uvc_stream_out;
            }
            uvc->bak_stream_out = info->uvc_stream_out;
#else
            uvc->stream_out = info->uvc_stream_out;
#endif
            uvc->offline = info->uvc_out;
        }
        hdl->open = 1;
        return device;
    } else {
        log_debug("err uvc_host_open device NULL\n");
        return NULL;
    }
}

int uvc_host_close(void *fd)
{
    struct device *device = (struct device *)fd;
    struct usb_uvc *uvc;
    struct usb_host_device *host_dev;

    if (device && (device->private_data != NULL)) {
        if (atomic_dec_and_test(&device->ref)) {
            host_dev = device_to_usbdev(device);
            if (!host_dev) {
                return DEV_ERR_NONE;
            }
            usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
            uvc = device_to_uvc(device);
            if (uvc) {
                /* usb_intr_config(device, uvc->host_ep, 0); */
                uvc->stream_out = uvc_dummy_stream_out;
#if !CAMERA_CLOSE_BY_IRQ
                uvc->bak_stream_out = uvc_dummy_stream_out;
#endif
                uvc->priv = NULL;
                uvc->offline = NULL;
            }
            struct usb_video_manager *hdl = &uvc_manager[usb_id];
            hdl->open = 0;
        }
    }
    return DEV_ERR_NONE;
}

int uvc_host_get_pix_table(void *fd, struct uvc_frame_info **pix_table)
{
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_uvc *uvc = device_to_uvc(device);

    if (uvc && uvc->reso_num) {
        *pix_table = uvc->pframe;
        return uvc->reso_num;
    }
    return 0;
}

int uvc_host_get_fps(void *fd)
{
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_uvc *uvc = device_to_uvc(device);
    if (uvc) {
        return uvc->fps;
    }
    return 25;
}

int uvc_host_set_pix(void *fd, int index)
{
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_uvc *uvc = device_to_uvc(device);

    if (uvc) {
        uvc->cur_frame = index;
    }
    return 0;
}

int uvc_force_reset(void *fd)
{
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -1;
    }
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    uvc_host_close_camera(fd);
    return usb_host_force_reset(usb_id);
}

int uvc_host_camera_out(const usb_dev usb_id)
{
    struct device *device;
    struct usb_uvc *uvc;
    struct usb_host_device *host_dev;
    struct usb_video_manager *hdl = &uvc_manager[usb_id];

    device = &hdl->dev;
    if (!device) {
        return 0;
    }
    uvc = device_to_uvc(device);
    if (!uvc) {
        return 0;
    }
    host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return 0;
    }
    if (uvc->offline) {
        uvc->offline(uvc->priv);
    }
#if (STOP_TRANSFER == 0)
    uvc->camera_open = 0;
#endif
    uvc_host_close_camera(device);
    if (uvc->ep_buffer) {
        usb_h_free_ep_buffer(usb_id, uvc->ep_buffer);
        uvc->ep_buffer = NULL;
    }
    free(uvc);
    if (hdl->buffer) {
        free(hdl->buffer);
        hdl->buffer = NULL;
    }
#if !CAMERA_CLOSE_BY_IRQ
    uvc->stream_out = uvc_dummy_stream_out;
    uvc->bak_stream_out = uvc_dummy_stream_out;
#else
    uvc->stream_out = uvc_dummy_stream_out;
#endif
    uvc->priv = NULL;
    uvc->offline = NULL;
    uvc_host_inf[usb_id].dev.uvc = NULL;
    hdl->open = 0;
    atomic_set(&(device->ref), 0);
    device->private_data = NULL;
    device->driver_data = NULL;
    return 0;
}

int uvc2usb_ioctl(void *fd, u32 cmd, void *arg)
{
    struct device *device = (struct device *)fd;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    if (!host_dev) {
        return -ENODEV;
    }
    usb_dev usb_id = usbpriv_to_usbid(usbdev_to_usbpriv(host_dev));
    struct usb_uvc *uvc = device_to_uvc(device);

    if (!uvc) {
        return -ENODEV;
    }
    switch (cmd) {
    case UVCIOC_SET_EVENT_LISTENER:
        struct uvc_event_listener *listener = (struct uvc_event_listener *)arg;
        uvc->event_priv = listener->priv;
        uvc->event_handler = listener->handler;
        break;
    default:
        log_error("err in uvc2usb_ioctl");
        return -EINVAL;
        /*return host_device_ioctl(device, cmd, arg);*/
    }
    return 0;
}

#endif // USB_HOST_UVC
