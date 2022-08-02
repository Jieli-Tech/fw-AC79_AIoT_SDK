#include "app_config.h"
#include "typedef.h"
#include "usb_stack.h"
#include "device/hid.h"
#include "generic/jiffies.h"
#include "le_common.h"

#if USB_HID_KEYBOARD_ENABLE

#define KB_LED_NUM_LOCK             0x01
#define KB_LED_CAPS_LOCK            0x02
#define KB_LED_SCROLL_LOCK          0x04
#define KB_LED_COMPOSE              0x08
#define KB_LED_KANA                 0x10

struct usb_hid_keyboard {
    usb_dev usb_id;
    u8 kb_leds_sta;
};

static struct usb_hid_keyboard hid_kb = {
    .usb_id = -1,
};

static struct usb_hid_keyboard *const __this = &hid_kb;


static const u8 report_keyboard[] = {
    0x05, 0x01,
    0x09, 0x06,
    0xa1, 0x01,
    0x05, 0x07,
    0x19, 0xe0,
    0x29, 0xe7,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x08,
    0x81, 0x02,
    0x95, 0x01,
    0x75, 0x08,
    0x81, 0x01,
    0x95, 0x06,
    0x75, 0x08,
    0x15, 0x00,
    0x25, 0x65,
    0x05, 0x07,
    0x19, 0x00,
    0x29, 0x65,
    0x81, 0x00,
#if 1  //OUTPUT
    0x95, 0x05,
    0x75, 0x01,
    0x05, 0x08,
    0x19, 0x01,
    0x29, 0x05,
    0x91, 0x02,
    0x95, 0x01,
    0x75, 0x03,
    0x91, 0x03,
#endif
    0xc0,
};
static const u16 ascii2kb_remap[128] = {
    0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x002a,  0x002b,  0x0028,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  // 0 ~ 15
    0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  // 16 ~ 31
    0x002c,  0x011e,  0x0134,  0x0120,  0x0121,  0x0122,  0x0124,  0x0034,  0x0126,  0x0127,  0x0125,  0x012e,  0x0036,  0x002d,  0x0037,  0x0038,  // 32 ~ 47
    0x0027,  0x001e,  0x001f,  0x0020,  0x0021,  0x0022,  0x0023,  0x0024,  0x0025,  0x0026,  0x0133,  0x0033,  0x0136,  0x002e,  0x0137,  0x0138,  // 48 ~ 63
    0x011f,  0x0104,  0x0105,  0x0106,  0x0107,  0x0108,  0x0109,  0x010a,  0x010b,  0x010c,  0x010d,  0x010e,  0x010f,  0x0110,  0x0111,  0x0112,  // 64 ~ 79
    0x0113,  0x0114,  0x0115,  0x0116,  0x0117,  0x0118,  0x0119,  0x011a,  0x011b,  0x011c,  0x011d,  0x002f,  0x0031,  0x0030,  0x0123,  0x012d,  // 80 ~ 95
    0x0035,  0x0004,  0x0005,  0x0006,  0x0007,  0x0008,  0x0009,  0x000a,  0x000b,  0x000c,  0x000d,  0x000e,  0x000f,  0x0010,  0x0011,  0x0012,  // 96 ~ 111
    0x0013,  0x0014,  0x0015,  0x0016,  0x0017,  0x0018,  0x0019,  0x001a,  0x001b,  0x001c,  0x001d,  0x012f,  0x0131,  0x0130,  0x0135,  0x0000,  // 112 ~ 127
};
static const u16 ascii2kp_remap[128] = {
    0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0058,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,   // 0 ~ 15
    0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,   // 16 ~ 31
    0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0055,  0x0057,  0x0000,  0x0056,  0x0063,  0x0054,   // 32 ~ 47
    0x0062,  0x0059,  0x005A,  0x005B,  0x005C,  0x005D,  0x005E,  0x005F,  0x0060,  0x0061,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,   // 48 ~ 63
    0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,   // 64 ~ 79
    0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,   // 80 ~ 95
    0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,   // 96 ~ 111
    0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,  0x0000,   // 112 ~ 127
};


static u32 hid_write_data(usb_dev usb_id, const u8 *buf, u32 len)
{
    int ret = -1;

    /* printf("\n usb_id = %d\n",usb_id); */
    if (usb_id == 0 || usb_id == 1) {
#if 1
        ret = hid_send_buf_async(usb_id, buf, len);
#else
        ret = hid_tx_data(__this->usb_id, buf, len);
#endif
        return ret;
    }
#ifdef CONFIG_BT_ENABLE
    extern int bt_tx_data(void *priv, u8 * data, u16 len);
    extern int bt_is_connect(void);
    if (bt_is_connect()) {
        ret = bt_tx_data(NULL, buf, len);
        delay(0x10); //根据情况延时一点,否则容易出现BUF满
        /* os_time_dly(1); */
        if (ret == BLE_BUFFER_FULL) {
            log_w("BLE_BUFFER_FULL"); //增大le_hogp.c中 ATT_SEND_CBUF_SIZE的值
        }
    }
#endif
    return ret;
}

u8 *hid_keyboard_get_report_desc()
{
    return (u8 *)report_keyboard;
}

u32 hid_keyboard_get_report_desc_size()
{
    return sizeof(report_keyboard);
}

int inter2string(int value, char *s, int len)
{
    char tmp;
    int count = 0;
    char symbol = 0;

    if (s == NULL) {
        return -1;
    }
    if (value < 0) {
        value = -value;
        symbol = 1;
    }
    do {
        s[count++] = value % 10 + '0';
        value = value / 10;
    } while (value != 0 && count < len);
    if (count > len - 1 - symbol) {
        return -2;
    }
    if (symbol) {
        s[count] = '-';
        s[count + 1] = 0;
        count++;
    } else {
        s[count] = 0;
    }
    len = count;
    for (count = 0; count < len / 2; count++) {
        tmp = s[count];
        s[count] = s[len - 1 - count];
        s[len - 1 - count] = tmp;
    }
    return len;
}

int string2inter(const char *s, int len, int *val)
{
    char *p = (char *)s;
    char symbol = 0;
    char radix = 10;  //进制，取值为16,10,8
    int err = 0;

    if (!p) {
        return -1;
    }

    if (*p == '-') {
        symbol = 1;
        p++;
        radix = 10;
    } else if (*p == '+') {
        symbol = 0;
        p++;
        radix = 10;
    } else if (*p == '0' && len >= 2 && (*(p + 1) == 'x' || *(p + 1) == 'X')) {
        radix = 16;
        while (*p != 0) {
            //upper to lower
            if (*p >= 'A' && *p <= 'Z') {
                *p += 'a' - 'A';
            }
            p++;
        }
        p = (char *)s + 2;
    } else if (*p == '0' && len >= 2 && *(p + 1) != 'x' && *(p + 1) != 'X') {
        radix = 8;
        p++;
    }
    *val = 0;
    while (*p && len--) {
        if (radix == 16) {
            if (*p >= '0' && *p <= '9') {
                *val *= 16;
                *val += *p - '0';
            } else if (*p >= 'a' && *p <= 'f') {
                *val *= 16;
                *val += *p - 'a' + 10;
            } else {
                return -2;
            }
        } else if (radix == 10) {
            if (*p < '0' || *p > '9') {
                return -2;
            }
            *val *= 10;
            *val += *p - '0';
        } else if (radix == 8) {
            if (*p < '0' || *p > '7') {
                return -2;
            }
            *val *= 8;
            *val += *p - '0';
        }
        p++;
    }
    *val = symbol ? -(*val) : *val;
    return err;
}



/* --------------------------------------------------------------------------*/
/**
 * @brief  hid keyboard send one key
 * @param  key - ascii code
 * @return NULL
 *
 * @additional infomation
 * modifier key bitmap
 * -  bit0  left control
 * -  bit1  left shift
 * -  bit2  left alt
 * -  bit3  left gui
 * -  bit4  right control
 * -  bit5  right shift
 * -  bit6  right alt
 * -  bit7  right gui
 */
/* --------------------------------------------------------------------------*/
static void hid_keyboard_send_onekey(u8 key)
{
    static u8 last_key = 0;
    u8 buf[8] = {0};
    u8 err = 0;

    if (key > sizeof(ascii2kb_remap) / sizeof(ascii2kb_remap[0])) {
        printf("ERR KEY %x", key);
        return;
    }

#if 1
    if ((ascii2kb_remap[key] & 0xff) == (ascii2kb_remap[last_key] & 0xff)) {
        buf[0] = (ascii2kb_remap[last_key] & 0x0100) ? 0x02 : 0;
        if (__this->kb_leds_sta & KB_LED_CAPS_LOCK) {
            if ((last_key >= 'a' && last_key <= 'z') || (last_key >= 'A' && last_key <= 'Z')) {
                buf[0] ^= 0x02;
            }
        }
        if ((buf[0] & 0x02) == 0) {
            buf[0] = 0x00;  //shift key
            buf[2] = 0x00;
            hid_write_data(__this->usb_id, buf, 8);
        }
    }

    last_key = key;
    buf[0] = ascii2kb_remap[key] & 0x0100 ? 0x02 : 0x00;  //shift key
    if (__this->kb_leds_sta & KB_LED_CAPS_LOCK) {
        if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) {
            buf[0] ^= 0x02;
        }
    }
    buf[2] = (u8)(ascii2kb_remap[key] & 0xff);
    hid_write_data(__this->usb_id, buf, 8);
    if (buf[2] & 0x02) {
        buf[0] = 0;
        buf[2] = 0;
        hid_write_data(__this->usb_id, buf, 8);
    }
#else
    buf[0] = ascii2kb_remap[key] & 0x0100 ? 0x02 : 0x00;  //shift key
    if (__this->kb_leds_sta & KB_LED_CAPS_LOCK) {
        if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z')) {
            buf[0] ^= 0x02;
        }
    }
    buf[2] = (u8)(ascii2kb_remap[key] & 0xff);
    if ((buf[0] & 0x02) && buf[2] != 0) {
        u8 tmpbuf[8] = {0x02};
        hid_write_data(__this->usb_id, tmpbuf, 8);
    }
    hid_write_data(__this->usb_id, buf, 8);

    if ((buf[0] & 0x02) && buf[2] != 0) {
        u8 tmpbuf[8] = {0x02};
        hid_write_data(__this->usb_id, tmpbuf, 8);
    }
    buf[0] = 0x00;  //shift key
    buf[2] = 0x00;
    hid_write_data(__this->usb_id, buf, 8);
#endif
}



static void hid_keyboard_send_wc_gbk(u16 wchar)
{
    char numeric_str[6] = {0};
    u8 buf[8] = {0};
    int err;
    char *p;

    buf[0] = BIT(2);  //left alt
    hid_write_data(__this->usb_id, buf, 8);
    inter2string(wchar, numeric_str, sizeof(numeric_str));
    p = numeric_str;
    while (*p != 0) {
        buf[0] = BIT(2);  //left alt
        buf[2] = ascii2kp_remap[*p] & 0xff;
        hid_write_data(__this->usb_id, buf, 8);
        buf[0] = BIT(2);  //left alt
        buf[2] = 0;
        hid_write_data(__this->usb_id, buf, 8);
        p++;
    }
    buf[0] = 0;
    buf[2] = 0;
    hid_write_data(__this->usb_id, buf, 8);
}

int hid_keyboard_send(const u8 *data, u32 len)
{
    int i;
    u32 start_time = jiffies;
    u16 wchar_gbk;
    u8 is_tax_code = 0;
    u32 len2;
    u8 lf = 0;
    u8 *ptr = (u8 *)data;

    //国税总局发票助手二维码格式
    if (len > 11 && ptr[0] == '$' && ptr[1] == '0' && ptr[2] == '1') {
        len2 = len;
        while (len2 > 11) {
            if (ptr[len2 - 1] == '$') {
                is_tax_code = 1;
                len = len2 - 3; //begin: $01
                len -= 1;  //end: $
                ptr += 3;
                u8 *tmpbuf = zalloc(len);
                if (!tmpbuf) {
                    printf("malloc tmpbuf fail\n");
                    return -1;
                }
                // << need function prototype>>
                //base64_decode(tmpbuf, ptr, len);
                len -= 4;  //CRC16
                if (!memcmp(&tmpbuf[len - 3], "</>", 3)) {
                    len -= 3;  //</>
                }
                ptr = tmpbuf;
                break;
            } else if (ptr[len2 - 1] == '\n') {
                lf = 1;
            }
            len2--;
        }
    }

    if (is_tax_code != 0) {
        printf("================ is tax qrcode ================\n");
        i = 0;
        while (i < len) {
            if (ptr[i] >= 0x81 && ptr[i] <= 0xfe) {
                if ((i + 1 < len) && ptr[i + 1] >= 0x40 && ptr[i + 1] <= 0xfe) {
                    wchar_gbk = (ptr[i] << 8) + ptr[i + 1];
                    hid_keyboard_send_wc_gbk(wchar_gbk);
                    i++;
                }
                i++;
            } else if ((i + 2 < len) && !memcmp(&ptr[i], "</>", 3)) {
                hid_keyboard_send_onekey('\t');
                i += 3;
            } else {
                hid_keyboard_send_onekey(ptr[i]);
                i++;
            }
        }
        if (lf != 0) {
            hid_keyboard_send_onekey('\n');
        }
    } else {
        i = 0;
        while (i < len) {
            if (ptr[i] < 0x80) {
                hid_keyboard_send_onekey(ptr[i]);
                i++;
            } else {
                //utf-8 code
                i += 2;
            }
        }
    }
    hid_keyboard_send_onekey(0);

    /* printf("usb send ptr %3d[in %d ms]: %s\n", len, jiffies_to_msecs(jiffies - start_time), ptr); */

    if (ptr != data) {
        free(ptr);
    }
    return 0;
}


//usb相关
int hid_keyboard_output(u8 *buf, u32 len)
{
    puts("hid output report\n");
    put_buf(buf, len);
    __this->kb_leds_sta = buf[0];
    return len;
}

void hid_keyboard_set_usb_dev(usb_dev usb_id)
{
    __this->usb_id = usb_id;
}

#endif


