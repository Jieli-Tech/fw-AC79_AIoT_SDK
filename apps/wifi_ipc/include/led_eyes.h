#ifndef _LED_EYES_
#define _LED_EYES_

#include "system/includes.h"
#include "app_config.h"

typedef enum {
    LED_CLOSE,
    LED_UP_LIGHT,
    LED_DOWN_LIGHT,
    LED_ALL_LIGHT,
    LED_TWINKLING,
    LED_TWINKLING_CIRCLE,
} _led_status;

typedef struct __gr202_led_ctl {
    volatile u8 led_status;
    int led_timer;
    void (*led_scan)(void);
    void (*led_switch)(_led_status status);
    void (*led_init)(_led_status status);
    void (*led_uninit)(void);
} _gr202_led_ctl;

extern _gr202_led_ctl gr202_led_ctl;
#endif

