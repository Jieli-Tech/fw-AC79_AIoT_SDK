#include "app_config.h"
#include "system/includes.h"
#include "event/device_event.h"

#ifdef USE_USB_HID_SLAVE_TEST_DEMO

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

void usb_hid_slave_test(void)
{
    sys_timer_add(NULL, hid_send_test, 1000);
}

late_initcall(usb_hid_slave_test);

#endif
