#include "app_config.h"
#include "typedef.h"
#include "usb_stack.h"
#include "device/hid.h"
#include "system/includes.h"
//#include "generic/jiffies.h"

#if USB_HID_POS_ENABLE

#define HID_POS_STA_BM_NORMAL                                    0x00
#define HID_POS_STA_BM_POWER_ON_RESET_SCANNER                    0x01
#define HID_POS_STA_BM_PREVENT_READ_OF_BARCODES                  0x02
#define HID_POS_STA_BM_INITIATE_BARCODE_READ                     0x04

enum {
    HID_POS_MSG_BEEP_GOOD = 0x10,
    HID_POS_MSG_BEEP_ERROR,
    HID_POS_MSG_TRIG_START,
    HID_POS_MSG_TRIG_STOP,
    HID_POS_MSG_RECV_AT_CMD,
};

struct hid_pos_msg {
    u8 *buf;
    u32 len;
    u32 in_pos;
    u32 out_pos;
    spinlock_t spinlock;
};

struct hid_pos_handle {
    /* void *usb_slave; */
    usb_dev usb_id;
    int state;
    struct hid_pos_msg msg;
    u8 at_cmd[62];
    int timer;
    spinlock_t spinlock;
    u8 trigger_sta;
    u8 trigger_last_sta;
    u8 trigger_on_filter;
    u8 trigger_off_filter;
    u8 send_trig;
};

static struct hid_pos_handle *__this;

int hid_pos_send_report(usb_dev usb_id, u32 symbology, u32 vendor_code, const u8 *buf, u32 len, u8 force);

static const char Hidpos_report_descr[] = {
    0x05, 0x8c,
    0x09, 0x02,
    0xa1, 0x01,

    0x09, 0x12,
    0xa1, 0x02,
    0x85, 0x02,
    0x15, 0x00,
    0x26, 0xff, 0x00,
    0x75, 0x08,
    0x95, 0x01,
    0x05, 0x01,
    0x09, 0x3b,
    0x81, 0x02,
    0x95, 0x03,
    0x05, 0x8c,
    0x09, 0xfb,
    0x09, 0xfc,
    0x09, 0xfd,
    0x81, 0x02,
    0x95, 0x38,
    0x09, 0xfe,
    0x82, 0x02, 0x01,
    0x06, 0x66, 0xff,
    0x95, 0x02,
    0x09, 0x04,
    0x09, 0x00,
    0x81, 0x02,
    0x05, 0x8c,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x08,
    0x09, 0xff,
    0x81, 0x02,
    0xc0,

    0x09, 0x14,
    0xa1, 0x02,
    0x85, 0x04,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x08,
    0x09, 0x5e,
    0x09, 0x5f,
    0x09, 0x60,
    0x09, 0x00,
    0x09, 0x00,
    0x09, 0x85,
    0x09, 0x86,
    //0x09, 0x00,
    0x91, 0x86,
    0xc0,
#if 0
    0x09, 0x16,
    0xa1, 0x02,
    0x85, 0x06,

    0x09, 0xA9,
    0xa1, 0x02,
    0x75, 0x02,
    0x95, 0x01,
    0x15, 0x01,
    0x25, 0x03,
    0x19, 0xAA,
    0x29, 0xAC,
    0x91, 0x00,
    0xc0,

    0x09, 0xA9,
    0xa1, 0x02,
    0x75, 0x02,
    0x95, 0x01,
    0x15, 0x01,
    0x25, 0x03,
    0x19, 0xAD,
    0x29, 0xAF,
    0x91, 0x00,
    0xc0,

    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 12,
    0x09, 0x91,
    0x09, 0x92,
    0x09, 0x93,
    0x09, 0x94,
    0x09, 0x95,
    0x09, 0x96,
    0x09, 0x9b,
    0x09, 0x9d,
    0x09, 0xa1,
    0x09, 0xa2,

    0x91, 0x02,
    0xc0,

    0x09, 0x18,
    0xa1, 0x02,
    0x85, 0x07,

    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x01,
    0x09, 0xc7,
    0x91, 0x02,
    0x0a, 0x0a, 0x01,
    0xa1, 0x02,
    0x75, 0x02,
    0x15, 0x01,
    0x25, 0x02,
    0x95, 0x01,
    0x1a, 0x0b, 0x01,
    0x2a, 0x0c, 0x01,
    0x91, 0x00,
    0xc0,

    0x09, 0xf0,
    0xa1, 0x02,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x01,
    0x19, 0xf1,
    0x29, 0xf2,
    0x91, 0x00,
    0xc0,
    0x09, 0xD6,
    0xa1, 0x02,
    0x15, 0x01,
    0x25, 0x09,
    0x75, 0x04,
    0x95, 0x01,
    0x19, 0xd7,
    0x29, 0xdf,
    0x91, 0x00,
    0xc0,

    0x15, 0x00,
    0x25, 48,
    0x75, 0x08,
    0x95, 0x02,
    0x0a, 0x06, 0x01,
    0x0a, 0x07, 0x01,
    0x91, 0x02,
    0x75, 0x01,
    0x95, 0x08,
    0x09, 0xc9,
    0x09, 0xd4,
    0x91, 0x02,
    0xc0,

    0x09, 0x19,
    0xa1, 0x02,
    0x85, 0x08,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x01,
    0x09, 0xca,
    0x91, 0x02,
    0x0a, 0x0a, 0x01,
    0xa1, 0x02,
    0x75, 0x02,
    0x15, 0x01,
    0x25, 0x02,
    0x95, 0x01,
    0x1a, 0x0b, 0x01,
    0x2a, 0x0c, 0x01,
    0x91, 0x00,
    0xc0,

    0x09, 0xf0,
    0xa1, 0x02,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x01,
    0x19, 0xf1,
    0x29, 0xf2,
    0x91, 0x00,
    0xc0,
    0x09, 0xD6,
    0xa1, 0x02,
    0x15, 0x01,
    0x25, 0x03,
    0x75, 0x04,
    0x95, 0x01,
    0x19, 0xd7,
    0x29, 0xd9,
    0x91, 0x00,
    0xc0,

    0x15, 2,
    0x25, 80,
    0x75, 0x08,
    0x95, 0x02,
    0x0a, 0x06, 0x01,
    0x0a, 0x07, 0x01,
    0x91, 0x02,
    0xc0,
#endif
    0x06, 0x66, 0xff,
    0x75, 0x08,
    0x26, 0xff, 0x00,
    0x09, 0x03,
    0xa1, 0x02,
    0x85, 0xfe,
    0x95, 0x01,
    0x09, 0x03,
    0xb2, 0x82, 0x01,
    0xc0,
    0x09, 0x01,
    0xa1, 0x02,
    0x85, 0xfd,
    0x95, 0x01,
    0x09, 0x21,
    0x91, 0x02,
    0x95, 0x3e,
    0x09, 0x22,
    0x92, 0x82, 0x01,
    0xc0,
    0xc0
};

u8 *hid_pos_get_report_desc()
{
    return (u8 *)Hidpos_report_descr;
}

u32 hid_pos_get_report_desc_size()
{
    return sizeof(Hidpos_report_descr);
}

static int hid_pos_msg_init(struct hid_pos_msg *msg, u8 *buf, u32 len)
{
    if (!msg || !buf || !len) {
        return -EINVAL;
    }
    msg->buf = buf;
    msg->len = len;
    msg->in_pos = 0;
    msg->out_pos = 0;
    spin_lock_init(&msg->spinlock);
    return 0;
}

static int hid_pos_msg_put(struct hid_pos_msg *msg, u8 value)
{
    if (!msg) {
        return -EINVAL;
    }
    spin_lock(&msg->spinlock);
    if ((msg->in_pos + msg->len - msg->out_pos) % msg->len == msg->len - 1) {
        //msg full
        spin_unlock(&msg->spinlock);
        return -ENOMEM;
    }
    msg->buf[msg->in_pos] = value;
    msg->in_pos = (msg->in_pos + 1) % msg->len;
    spin_unlock(&msg->spinlock);
    return 0;
}

static int hid_pos_msg_get(struct hid_pos_msg *msg, u8 *value)
{
    if (!msg || !value) {
        return -EINVAL;
    }
    spin_lock(&msg->spinlock);
    if ((msg->in_pos + msg->len - msg->out_pos) % msg->len == 0) {
        //msg empty
        spin_unlock(&msg->spinlock);
        return -ENOMEM;
    }
    *value = msg->buf[msg->out_pos];
    msg->out_pos = (msg->out_pos + 1) % msg->len;
    spin_unlock(&msg->spinlock);
    return 0;
}

static u32 hid_pos_msg_query_size(struct hid_pos_msg *msg)
{
    u32 len;
    if (!msg) {
        return 0;
    }
    spin_lock(&msg->spinlock);
    len = (msg->in_pos + msg->len - msg->out_pos) % msg->len;
    spin_unlock(&msg->spinlock);
    return len;
}

static int get_trigger_state()
{
    //获取触发按键状态
#if 0
    static int init = 0;
    if (!init) {
        gpio_set_pull_up(IO_PORTX_XX);
        init = 1;
    }
    return gpio_direction_input(IO_PORTX_XX);
#else
    return 0;
#endif
}

static const char *const hid_pos_at_cmd[] = {
    "\x16\x07\r",                                                           //BEEP
    "\x16M\rP_INFO.",                                                       //GET INFO
    "\x16M\rTERMID?.",                                                      //TERMID
};

static void hid_pos_msg_handle(void *arg)
{
    int err;
    struct hid_pos_handle *hdl = arg;
    u8 msg_val;
    u32 idx;
    const char *at_cmd_ack;

    if (get_trigger_state()) {
        hdl->trigger_on_filter++;
        hdl->trigger_off_filter = 0;
    } else {
        hdl->trigger_on_filter = 0;
        hdl->trigger_off_filter++;
    }
    if (hdl->trigger_on_filter >= 2) {
        hdl->trigger_sta = 1;
    } else if (hdl->trigger_off_filter >= 2) {
        hdl->trigger_sta = 0;
    }
    if (hdl->trigger_sta != hdl->trigger_last_sta) {
        if (!(hdl->state & HID_POS_STA_BM_INITIATE_BARCODE_READ)) {
            if (hdl->trigger_sta) {
                msg_val = HID_POS_MSG_TRIG_START;
            } else {
                msg_val = HID_POS_MSG_TRIG_STOP;
            }
            err = hid_pos_msg_put(&hdl->msg, msg_val);
            if (err < 0) {
                printf("hid pos put msg fail, %d, line %d\n", err, __LINE__);
            }
        }
    }
    hdl->trigger_last_sta = hdl->trigger_sta;

    if (0 == hid_pos_msg_get(&hdl->msg, &msg_val)) {
        switch (msg_val) {
        case HID_POS_MSG_BEEP_GOOD:
            printf("beep good\n");
            /* tick(2); */
            break;
        case HID_POS_MSG_BEEP_ERROR:
            printf("beep error\n");
            /* tick(3); */
            break;
        case HID_POS_MSG_TRIG_START:
            printf("trig start\n");
            //add codes of LED turning on
            hdl->send_trig = 1;
            break;
        case HID_POS_MSG_TRIG_STOP:
            printf("trig stop\n");
            //add codes of LED turning off
            hdl->send_trig = 0;
            break;
        case HID_POS_MSG_RECV_AT_CMD:
            printf("recv at-cmd\n");
            spin_lock(&hdl->spinlock);
            for (idx = 0; idx < ARRAY_SIZE(hid_pos_at_cmd); idx++) {
                if (memcmp(hdl->at_cmd, hid_pos_at_cmd[idx], strlen(hid_pos_at_cmd[idx])) == 0) {
                    break;
                }
            }
            spin_unlock(&hdl->spinlock);
            printf("idx %d\n", idx);
            if (idx < ARRAY_SIZE(hid_pos_at_cmd)) {
                switch (idx) {
                case 0:
                    /* tick(2); */
                    break;
                case 1:
                    at_cmd_ack = "INFO: JL_HID_POS\r\nVERSION: 1.0.0\r\n\x06.";
                    hid_pos_send_report(hdl->usb_id, 0x365a5d, 0x36, (const u8 *)at_cmd_ack, strlen(at_cmd_ack), 1);
                    break;
                case 2:
                    at_cmd_ack = "TERMID131\x06.";
                    hid_pos_send_report(hdl->usb_id, 0x365a5d, 0x36, (const u8 *)at_cmd_ack, strlen(at_cmd_ack), 1);
                    break;
                }
            }
            break;
        }
    }
}

int hid_pos_receive(u8 *buf, u32 len)
{
    int err = 0;
    u8 msg_val;

    printf("%s() buf size %d\n", __func__, len);
    put_buf(buf, len);
    //report id
    switch (buf[0]) {
    case 4:
        puts("trigger report\n");
        if (__this) {
            if (buf[1] & 0x40) {  //Sound Good Read Beep
                msg_val = HID_POS_MSG_BEEP_GOOD;
                err = hid_pos_msg_put(&__this->msg, msg_val);
                if (err < 0) {
                    printf("hid pos put msg fail, %d, line %d\n", err, __LINE__);
                }
            }
            if (buf[1] & 0x20) {  //Sound Error Beep
                msg_val = HID_POS_MSG_BEEP_ERROR;
                err = hid_pos_msg_put(&__this->msg, msg_val);
                if (err < 0) {
                    printf("hid pos put msg fail, %d, line %d\n", err, __LINE__);
                }
            }
            if (buf[1] & 0x02) {  //Prevent Read of Barcodes
                __this->state |= HID_POS_STA_BM_PREVENT_READ_OF_BARCODES;
            } else {
                __this->state &= ~HID_POS_STA_BM_PREVENT_READ_OF_BARCODES;
            }
            if (buf[1] & 0x04) {  //Initiate Barcode Read
                if (!(__this->state & HID_POS_STA_BM_INITIATE_BARCODE_READ)) {
                    __this->state |= HID_POS_STA_BM_INITIATE_BARCODE_READ;
                    msg_val = HID_POS_MSG_TRIG_START;
                    err = hid_pos_msg_put(&__this->msg, msg_val);
                    if (err < 0) {
                        printf("hid pos put msg fail, %d, line %d\n", err, __LINE__);
                    }
                }
            } else {
                if ((__this->state & HID_POS_STA_BM_INITIATE_BARCODE_READ)) {
                    __this->state &= ~HID_POS_STA_BM_INITIATE_BARCODE_READ;
                    msg_val = HID_POS_MSG_TRIG_STOP;
                    err = hid_pos_msg_put(&__this->msg, msg_val);
                    if (err < 0) {
                        printf("hid pos put msg fail, %d, line %d\n", err, __LINE__);
                    }
                }
            }
        }
        break;
    case 254:
        puts("Vendor-Defined 3\n");
        break;
    case 253:
        puts("Vendor-Defined 1\n");
        //receive at_cmd from pc
        spin_lock(&__this->spinlock);
        memcpy(__this->at_cmd, &buf[2], buf[1]);
        spin_unlock(&__this->spinlock);
        msg_val = HID_POS_MSG_RECV_AT_CMD;
        err = hid_pos_msg_put(&__this->msg, msg_val);
        if (err < 0) {
            printf("hid pos put msg fail, %d, line %d\n", err, __LINE__);
        }
        break;
    default:
        printf("unsupported report id %d\n", buf[0]);
    }
    return 0;
}

int hid_pos_open(usb_dev usb_id)
{
    struct hid_pos_handle *hdl;
    u8 *ptr = NULL;
    int err;

    hdl = (struct hid_pos_handle *)malloc(sizeof(*hdl));
    if (!hdl) {
        return -ENOMEM;
    }
    memset(hdl, 0, sizeof(*hdl));
    hdl->usb_id = usb_id;
    ptr = (u8 *)malloc(10);
    if (!ptr) {
        goto __err_exit;
    }
    hid_pos_msg_init(&hdl->msg, ptr, 10);
    spin_lock_init(&hdl->spinlock);
    hdl->timer = sys_timer_add(hdl, hid_pos_msg_handle, 50);
    hdl->state = HID_POS_STA_BM_NORMAL;
    __this = hdl;
    return 0;

__err_exit:
    if (hdl->timer) {
        sys_timer_del(hdl->timer);
    }
    if (ptr) {
        free(ptr);
    }
    free(hdl);

    return -EFAULT;
}

void hid_pos_close(usb_dev usb_id)
{
    struct hid_pos_handle *hdl;
    int err;

    if (__this == NULL) {
        return;
    }
    hdl = __this;
    if (hdl->timer) {
        sys_timer_del(hdl->timer);
    }
    free(hdl->msg.buf);
    free(hdl);
    __this = NULL;
}

int hid_pos_send_report(usb_dev usb_id, u32 symbology, u32 vendor_code, const u8 *buf, u32 len, u8 force)
{
    struct hid_pos_handle *hdl;
    u32 offset = 0;
    u32 fit_len;
    u8 decoded_data[64] = {0};
    int err = 0;

    if (__this == NULL) {
        return -EFAULT;
    }
    hdl = __this;
    if (((hdl->state & HID_POS_STA_BM_PREVENT_READ_OF_BARCODES) == 0) &&
        (hdl->send_trig || force)) {
        decoded_data[0] = 2;  //report id: 2
        //decoded_data[2] = 0x5d;  //Symbology Identifier1
        //decoded_data[3] = 0x43;  //Symbology Identifier2
        //decoded_data[4] = 0x30;  //Symbology Identifier3
        //decoded_data[61] = 0x6a;
        decoded_data[2] = symbology & 0xff;
        decoded_data[3] = (symbology >> 8) & 0xff;
        decoded_data[4] = (symbology >> 16) & 0xff;
        decoded_data[61] = vendor_code & 0xff;
        decoded_data[62] = (vendor_code >> 8) & 0xff;
        while (offset < len) {
            fit_len = len - offset > 56 ? 56 : len - offset;
            memset(&decoded_data[5], 0, 56);
            memcpy(&decoded_data[5], buf + offset, fit_len);
            if (fit_len < 56) {
                decoded_data[1] = fit_len;
                decoded_data[63] &= ~BIT(0);
            } else {
                decoded_data[1] = 56;
                decoded_data[63] |= BIT(0);  //Decode Data Continued
            }
            hid_send_buf_async(hdl->usb_id, decoded_data, 64);
            //hid_tx_data(hdl->usb_id, decode_data, 64);
            offset += fit_len;
        }
        if (!force) {
            hdl->send_trig = 0;
        }
        err = hid_pos_msg_put(&hdl->msg, HID_POS_MSG_TRIG_STOP);
        if (err < 0) {
            printf("hid pos put msg fail, %d, line %d\n", err, __LINE__);
        }
    }
    if (offset < len) {
        err = -EFAULT;
    }
    return err;
}

int hid_pos_send(u8 *data, u32 len, u8 force)
{
    if (data[len - 1] == '\n') {
        data[len - 1] = '\r';
    }
    return hid_pos_send_report(__this->usb_id, 0x30435d, 0x6a, data, len, force);
}

#endif


