#include "os/os_api.h"
#include "usb/device/usb_stack.h"
#include "usb/device/hid.h"
#include "usb_config.h"
#include "app_config.h"
#include "circular_buf.h"

#ifdef TCFG_USB_SLAVE_USER_HID
#undef TCFG_USB_SLAVE_HID_ENABLE
#define TCFG_USB_SLAVE_HID_ENABLE           0
#endif

#if TCFG_USB_SLAVE_HID_ENABLE
#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#define TMPBUFLEN           (64 + 2)
#define CBUF_SIZE           (TMPBUFLEN * 50)

struct usb_hid_info {
    u8 *stream;
    cbuffer_t cbuf;
    //OS_SEM sem;
    //u8 tmpbuf[TMPBUFLEN];
    int (*output)(u8 *buf, u32 len);
    u8 cust_rp_desc[512];
    u32 desc_len;
    //int timer;
    int cfg_done : 1;
    int trig : 1;
    int dummy : 30;
    u8 *hid_tx_buf;
    u8 *hid_rx_buf;
};

static struct usb_hid_info *hid_info[USB_MAX_HW_NUM];

#if (USB_MALLOC_ENABLE == 0)
static struct usb_hid_info _hid_info[USB_MAX_HW_NUM];
static u8 hid_g_stream[USB_MAX_HW_NUM][CBUF_SIZE];
//static u8 hid_g_tx_buf[MAXP_SIZE_HIDIN] __attribute__((align(64)));
//static u8 hid_g_rx_buf[MAXP_SIZE_HIDOUT] __attribute__((align(64)));
#endif

static void hid_intrtx(struct usb_device_t *usb_device, u32 ep);
static void hid_intrrx(struct usb_device_t *usb_device, u32 ep);
static void hid_output_handle(usb_dev usb_id, u8 *buf, u32 len);

static const u8 sHIDDescriptor[] = {
//HID
    //InterfaceDeszcriptor:
    USB_DT_INTERFACE_SIZE,     // Length
    USB_DT_INTERFACE,          // DescriptorType
    /* 0x04,                      // bInterface number */
    0x00,                       // bInterface number
    0x00,                      // AlternateSetting
    0x02,                        // NumEndpoint
    USB_CLASS_HID,             // Class = Human Interface Device
    0x00,                      // Subclass, 0 No subclass, 1 Boot Interface subclass
    0x00,                      // Procotol, 0 None, 1 Keyboard, 2 Mouse
    0x00,                      // Interface Name

    //HIDDescriptor:
    0x09,                      // bLength
    USB_HID_DT_HID,            // bDescriptorType, HID Descriptor
    0x00, 0x01,                // bcdHID, HID Class Specification release NO.
    0x00,                      // bCuntryCode, Country localization (=none)
    0x01,                       // bNumDescriptors, Number of descriptors to follow
    0x22,                       // bDescriptorType, Report Desc. 0x22, Physical Desc. 0x23
    0,//LOW(ReportLength)
    0, //HIGH(ReportLength)

    //EndpointDescriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_IN | HID_EP_IN,     // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_HIDIN), HIBYTE(MAXP_SIZE_HIDIN),// Maximum packet size
    0x01,     // Poll every 1msec seconds

//@Endpoint Descriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_OUT | HID_EP_OUT,   // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_HIDOUT), HIBYTE(MAXP_SIZE_HIDOUT),// Maximum packet size
    0x01,                       // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms */
};

static const u8 sHIDReportDesc[] = {
    USAGE_PAGE(1, CONSUMER_PAGE),
    USAGE(1, CONSUMER_CONTROL),
    COLLECTION(1, APPLICATION),

    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(1, 0x01),

    USAGE(1, VOLUME_INC),
    USAGE(1, VOLUME_DEC),
    USAGE(1, MUTE),
    USAGE(1, PLAY_PAUSE),
    USAGE(1, SCAN_NEXT_TRACK),
    USAGE(1, SCAN_PREV_TRACK),
    USAGE(1, FAST_FORWARD),
    USAGE(1, STOP),

    USAGE(1, TRACKING_INC),
    USAGE(1, TRACKING_DEC),
    USAGE(1, STOP_EJECT),
    USAGE(1, VOLUME),
    USAGE(2, BALANCE_LEFT),
    USAGE(2, BALANCE_RIGHT),
    USAGE(1, PLAY),
    USAGE(1, PAUSE),

    REPORT_SIZE(1, 0x01),
    REPORT_COUNT(1, 0x10),
    INPUT(1, 0x42),
    END_COLLECTION,

};

static u32 get_hid_report_desc_len(usb_dev usb_id, u32 index)
{
    u32 len = 0;
    if (hid_info[usb_id] && hid_info[usb_id]->desc_len) {
        len = hid_info[usb_id]->desc_len;
    } else {
        len = sizeof(sHIDReportDesc);
    }
    return len;
}

static void *get_hid_report_desc(usb_dev usb_id, u32 index)
{
    u8 *ptr  = NULL;
    if (hid_info[usb_id] && hid_info[usb_id]->desc_len) {
        ptr = hid_info[usb_id]->cust_rp_desc;
    } else {
        ptr = (u8 *)sHIDReportDesc;
    }
    return ptr;
}

void hid_set_report_desc(const usb_dev usb_id, const u8 *report_desc, u32 len)
{
    if (hid_info[usb_id] == NULL) {
        return;
    }
    if (len > sizeof(hid_info[usb_id]->cust_rp_desc)) {
        len = sizeof(hid_info[usb_id]->cust_rp_desc);
    }
    if (report_desc == NULL || len == 0) {
        memset(hid_info[usb_id]->cust_rp_desc, 0, sizeof(hid_info[usb_id]->cust_rp_desc));
        hid_info[usb_id]->desc_len = 0;
    } else {
        memcpy(hid_info[usb_id]->cust_rp_desc, report_desc, len);
        hid_info[usb_id]->desc_len = len;
    }
}

static u8 *hid_ep_in_dma[USB_MAX_HW_NUM];
static u8 *hid_ep_out_dma[USB_MAX_HW_NUM];

static void hid_rx_data(struct usb_device_t *usb_device, u32 ep)
{
    /* const usb_dev usb_id = usb_device2id(usb_device); */
    /* u32 rx_len = usb_g_intr_read(usb_id, ep, NULL, 64, 0); */
    /* hid_tx_data(usb_id, hid_ep_out_dma[usb_id], rx_len); */
}

u32 hid_tx_data(const usb_dev usb_id, const u8 *buffer, u32 len)
{
    bool cfg_done;

    if (hid_info[usb_id] == NULL) {
        return 0;
    }
    while (hid_info[usb_id]->trig) {
        os_time_dly(1);
    }
    cfg_done = hid_info[usb_id]->cfg_done;
    hid_info[usb_id]->cfg_done = 0;
    if (cfg_done != 0) {
        len = usb_g_intr_write(usb_id, HID_EP_IN, buffer, len);
    }
    hid_info[usb_id]->cfg_done = cfg_done;
    return len;
}

static void hid_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    usb_g_ep_config(usb_id, HID_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_INT, 0, hid_ep_in_dma[usb_id], MAXP_SIZE_HIDIN);
    usb_g_set_intr_hander(usb_id, HID_EP_IN | USB_DIR_IN, hid_intrtx);

    usb_g_ep_config(usb_id, HID_EP_OUT, USB_ENDPOINT_XFER_INT, 1, hid_ep_out_dma[usb_id], MAXP_SIZE_HIDOUT);
    /* usb_g_set_intr_hander(usb_id, HID_EP_OUT, hid_rx_data); */
    usb_g_set_intr_hander(usb_id, HID_EP_OUT, hid_intrrx);
    usb_enable_ep(usb_id, HID_EP_IN);
    hid_info[usb_id]->trig = 0;
}

static void hid_reset(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    log_debug("%s\n", __func__);
#if USB_ROOT2
    usb_disable_ep(usb_id, HID_EP_IN);
#else
    hid_endpoint_init(usb_device, itf);
#endif
}

static u32 hid_recv_output_report(struct usb_device_t *usb_device, struct usb_ctrlrequest *setup)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 ret = 0;
    u8 read_ep[MAXP_SIZE_HIDOUT];
    u8 mute;
    u16 volume = 0;
    usb_read_ep0(usb_id, read_ep, MIN(sizeof(read_ep), setup->wLength));
    put_buf(read_ep, 8);
    hid_output_handle(usb_id, read_ep, MIN(sizeof(read_ep), setup->wLength));

    ret = USB_EP0_STAGE_SETUP;
    return ret;
}

static u32 hid_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;
    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:
            switch (HIBYTE(req->wValue)) {
            case USB_HID_DT_HID:
                tx_payload = (u8 *)sHIDDescriptor + USB_DT_INTERFACE_SIZE;
                tx_len = 9;
                tx_payload = usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                tx_payload[7] = LOBYTE(get_hid_report_desc_len(usb_id, req->wIndex));
                tx_payload[8] = HIBYTE(get_hid_report_desc_len(usb_id, req->wIndex));
                break;
            case USB_HID_DT_REPORT:
                /* hid_endpoint_init(usb_device, req->wIndex); */
                tx_len = get_hid_report_desc_len(usb_id, req->wIndex);
                tx_payload = get_hid_report_desc(usb_id, req->wIndex);
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                break;
            }// USB_REQ_GET_DESCRIPTOR
            break;
        case USB_REQ_SET_DESCRIPTOR:
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        case USB_REQ_SET_INTERFACE:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                //只有一个interface 没有Alternate
                if (req->wValue == 0) { //alt 0
                    usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
                } else {
                    usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
                }
            }
            break;
        case USB_REQ_GET_INTERFACE:
            if (req->wValue || (req->wLength != 1)) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                tx_payload[0] = 0x00;
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            }
            break;
        case USB_REQ_GET_STATUS:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;
        }//bRequest @ USB_TYPE_STANDARD
        break;

    case USB_TYPE_CLASS: {
        switch (req->bRequest) {
        case USB_REQ_SET_IDLE:
            if (hid_info[usb_id] && !hid_info[usb_id]->cfg_done) {
                hid_endpoint_init(usb_device, -1);
                hid_info[usb_id]->cfg_done = 1;
            }
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            break;
        case USB_REQ_GET_IDLE:
            tx_len = 1;
            tx_payload[0] = 0;
            usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            break;
        case USB_REQ_SET_REPORT:
            usb_set_setup_recv(usb_device, hid_recv_output_report);
            break;
        }//bRequest @ USB_TYPE_CLASS
    }
    break;
    }
    return 0;
}

u32 hid_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    log_debug("hid interface num:%d\n", *cur_itf_num);
    u8 *_ptr = ptr;
    memcpy(ptr, sHIDDescriptor, sizeof(sHIDDescriptor));
    ptr[2] = *cur_itf_num;
    if (usb_set_interface_hander(usb_id, *cur_itf_num, hid_itf_hander) != *cur_itf_num) {
        ASSERT(0, "hid set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, hid_reset) != *cur_itf_num) {
        ASSERT(0, "hid set interface_reset_hander fail");
    }

    ptr[USB_DT_INTERFACE_SIZE + 7] = LOBYTE(get_hid_report_desc_len(usb_id, 0));
    ptr[USB_DT_INTERFACE_SIZE + 8] = HIBYTE(get_hid_report_desc_len(usb_id, 0));
    *cur_itf_num = *cur_itf_num + 1;
    return sizeof(sHIDDescriptor) ;
}

u32 hid_send_buf_async(const usb_dev usb_id, const u8 *buf, u32 len)
{
    u32 txlen;
    u8 tmpbuf[TMPBUFLEN] = {0};
    u32 len1;
    u8 *addr;

    if (hid_info[usb_id] == NULL) {
        return 0;
    }
    txlen = len > (TMPBUFLEN - 2) ? (TMPBUFLEN - 2) : len;
    tmpbuf[0] = txlen & 0xff;
    tmpbuf[1] = (txlen >> 8) & 0xff;
    memcpy(&tmpbuf[2], buf, txlen);
    do {
        if (cbuf_write(&hid_info[usb_id]->cbuf, tmpbuf, txlen + 2) > 0) {
            break;
        }
        /* log_debug("cbuf full\n"); */
        putchar('.');
        if (hid_info[usb_id]->cfg_done == 0) {
            return 0;
        } else if (hid_info[usb_id]->trig == 0) {
            return 0;
        }
        os_time_dly(1);
    } while (1);

    if (hid_info[usb_id]->cfg_done) {
        if (!hid_info[usb_id]->trig) {
            len1 = cbuf_read(&hid_info[usb_id]->cbuf, &tmpbuf[0], 2);
            if (len1 == 0) {
                goto __exit;
            }
            len1 = ((u16)tmpbuf[1] << 8) | tmpbuf[0];
            len1 = cbuf_read(&hid_info[usb_id]->cbuf, &tmpbuf[2], len1);
            if (len1 == 0) {
                goto __exit;
            }
            if (len1 > MAXP_SIZE_HIDIN) {
                len1 = MAXP_SIZE_HIDIN;
            }
            addr = usb_get_dma_taddr(usb_id, HID_EP_IN);
            memcpy(addr, &tmpbuf[2], len1);
            usb_set_intr_txe(usb_id, HID_EP_IN);
            usb_g_intr_write(usb_id, HID_EP_IN, NULL, len1);
            hid_info[usb_id]->trig = 1;
        }
    }

__exit:
    return txlen;
}

void hid_control(const usb_dev usb_id, u8 state)
{
    //for API compatiblity, do not delete this function
}

void hid_set_output_handle(const usb_dev usb_id, int (*output)(u8 *, u32))
{
    if (hid_info[usb_id] == NULL) {
        return;
    }
    hid_info[usb_id]->output = output;
}

static void hid_output_handle(usb_dev usb_id, u8 *buf, u32 len)
{
    if (hid_info[usb_id] && hid_info[usb_id]->output) {
        hid_info[usb_id]->output(buf, len);
    }
}

static void hid_intrtx(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 len = 0;
    u8 *addr;
    int err;
    u8 tmpbuf[TMPBUFLEN] = {0};

    if (hid_info[usb_id] == NULL) {
        return;
    }
    ep = ep & 0x0f;
    if (hid_info[usb_id]->trig) {
        len = cbuf_read(&hid_info[usb_id]->cbuf, &tmpbuf[0], 2);
        if (len == 0) {
            goto __write_end;
        }
        len = ((u16)tmpbuf[1] << 8) | tmpbuf[0];
        len = cbuf_read(&hid_info[usb_id]->cbuf, &tmpbuf[2], len);
        if (len == 0) {
            goto __write_end;
        }
        if (len > MAXP_SIZE_HIDIN) {
            len = MAXP_SIZE_HIDIN;
        }
        addr = usb_get_dma_taddr(usb_id, HID_EP_IN);
        memcpy(addr, &tmpbuf[2], len);
        usb_g_intr_write(usb_id, ep, NULL, len);
    }
    return;

__write_end:
    usb_clr_intr_txe(usb_id, ep);
    hid_info[usb_id]->trig = 0;
}

static void hid_intrrx(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 len = 0;
    int err;
    u8 read_ep[MAXP_SIZE_HIDOUT];

    if (hid_info[usb_id] == NULL) {
        return;
    }
    ep &= 0x0f;
    len = usb_g_intr_read(usb_id, ep, read_ep, MAXP_SIZE_HIDOUT, 0);
    /* USB_DEBUG_PRINTF("rx len %d reg :%x", len, usb_read_rxcsr(usb_id, ep)); */
    //printf_buf(radr, len);
    hid_output_handle(usb_id, read_ep, len);
}

u32 hid_register(const usb_dev usb_id)
{
    u32 err = 0;
    if (hid_info[usb_id] == NULL) {
#if USB_MALLOC_ENABLE
        hid_info[usb_id] = (struct usb_hid_info *)zalloc(sizeof(struct usb_hid_info));
        if (!hid_info[usb_id]) {
            printf("hid malloc hid_info[%d] fail\n", usb_id);
            return (u32) - 1;
        }
        hid_info[usb_id]->stream = (u8 *)malloc(CBUF_SIZE);
        if (!hid_info[usb_id]->stream) {
            printf("hid malloc cbuffer space fail\n");
            err = (u32) - 1;
            goto __exit;
        }
        //hid_info[usb_id]->hid_tx_buf = (u8 *)malloc(MAXP_SIZE_HIDIN);
        //hid_info[usb_id]->hid_rx_buf = (u8 *)malloc(MAXP_SIZE_HIDOUT);
        //if (!hid_info[usb_id]->hid_tx_buf || !hid_info[usb_id]->hid_rx_buf) {
        //    printf("hid malloc tx/rx space fail\n");
        //    err = (u32) - 1;
        //    goto __exit;
        //}
#else
        hid_info[usb_id] = &_hid_info[usb_id];
        hid_info[usb_id]->stream = hid_g_stream[usb_id];
        //hid_info[usb_id]->hid_tx_buf = hid_g_tx_buf;
        //hid_info[usb_id]->hid_rx_buf = hid_g_rx_buf;
#endif
        cbuf_init(&hid_info[usb_id]->cbuf, hid_info[usb_id]->stream, CBUF_SIZE);

        hid_ep_in_dma[usb_id] = usb_alloc_ep_dmabuffer(usb_id, HID_EP_IN | USB_DIR_IN, MAXP_SIZE_HIDIN);
        hid_ep_out_dma[usb_id] = usb_alloc_ep_dmabuffer(usb_id, HID_EP_OUT, MAXP_SIZE_HIDOUT);
    }
    return 0;
__exit:
#if USB_MALLOC_ENABLE
    //if (hid_info[usb_id]->hid_tx_buf) {
    //    free(hid_info[usb_id]->hid_tx_buf);
    //    hid_info[usb_id]->hid_tx_buf = NULL;
    //}
    //if (hid_info[usb_id]->hid_rx_buf) {
    //    free(hid_info[usb_id]->hid_rx_buf);
    //    hid_info[usb_id]->hid_rx_buf = NULL;
    //}
    if (hid_info[usb_id]->stream) {
        free(hid_info[usb_id]->stream);
        hid_info[usb_id]->stream = NULL;
    }
    free(hid_info[usb_id]);
    hid_info[usb_id] = NULL;
#else
    //hid_info[usb_id]->hid_tx_buf = NULL;
    //hid_info[usb_id]->hid_rx_buf = NULL;
    hid_info[usb_id]->stream = NULL;
    hid_info[usb_id] = NULL;
#endif
    return err;
}

void hid_release(const usb_dev usb_id)
{
    if (hid_info[usb_id]) {
        //hid_set_output_handle(usb_id, NULL);
        //hid_set_report_desc(usb_id, NULL, 0);
        hid_info[usb_id]->cfg_done = 0;
        if (hid_ep_in_dma[usb_id]) {
            usb_free_ep_dmabuffer(usb_id, hid_ep_in_dma[usb_id]);
            hid_ep_in_dma[usb_id] = NULL;
        }
        if (hid_ep_out_dma[usb_id]) {
            usb_free_ep_dmabuffer(usb_id, hid_ep_out_dma[usb_id]);
            hid_ep_out_dma[usb_id] = NULL;
        }
#if USB_MALLOC_ENABLE
        //if (hid_info[usb_id]->hid_tx_buf) {
        //    free(hid_info[usb_id]->hid_tx_buf);
        //    hid_info[usb_id]->hid_tx_buf = NULL;
        //}
        //if (hid_info[usb_id]->hid_rx_buf) {
        //    free(hid_info[usb_id]->hid_rx_buf);
        //    hid_info[usb_id]->hid_rx_buf = NULL;
        //}
        if (hid_info[usb_id]->stream) {
            free(hid_info[usb_id]->stream);
            hid_info[usb_id]->stream = NULL;
        }
        free(hid_info[usb_id]);
#else
        //hid_info[usb_id]->hid_tx_buf = NULL;
        //hid_info[usb_id]->hid_rx_buf = NULL;
        hid_info[usb_id]->stream = NULL;
#endif
        hid_info[usb_id] = NULL;
    }
}

void hid_key_handler(struct usb_device_t *usb_device, u32 hid_key)
{
    const usb_dev usb_id = usb_device2id(usb_device);

    u16 key_buf = hid_key;
    hid_tx_data(usb_id, (const u8 *)&key_buf, 2);
    os_time_dly(2);
    key_buf = 0;
    hid_tx_data(usb_id, (const u8 *)&key_buf, 2);
}


struct hid_button {
    u8 report_id;
    u8 button1: 1;
    u8 button2: 1;
    u8 button3: 1;
    u8 no_button: 5;
    u8 X_axis;
    u8 Y_axis;
};
struct hid_button hid_key;
void hid_test(struct usb_device_t *usb_device)
{
    static u32 tx_count = 0;

    hid_key_handler(usb_device, BIT(tx_count));
    tx_count ++;
    if (BIT(tx_count) > USB_AUDIO_PAUSE) {
        tx_count = 0;
    }
}
#else
void hid_key_handler(struct usb_device_t *usb_device, u32 hid_key)
{

}
#endif
