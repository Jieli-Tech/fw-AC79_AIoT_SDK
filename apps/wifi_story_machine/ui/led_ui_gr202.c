#include "app_config.h"

#if (defined CONFIG_BOARD_7901A_DEMO_STORY)

#include "server/led_ui_server.h"
#include "system/init.h"
#include "device/gpio.h"
#include "led_eyes.h"
#include "action.h"

#if (defined CONFIG_BOARD_7901A_DEMO_STORY)

#define LED0_IO IO_PORTB_07
#define LED1_IO IO_PORTC_08
#define LED2_IO IO_PORTC_09
#define LED3_IO IO_PORTC_10

#endif


#define  LED_INIT()  do{\
    gpio_set_hd(LED1_IO,0);\
    gpio_set_hd(LED2_IO,0);\
    gpio_set_hd(LED3_IO,0);\
}while(0)
#define  LED_ALL_CLOSE()  do{\
    gpio_direction_output(LED1_IO,0);\
    gpio_direction_output(LED2_IO,0);\
    gpio_direction_output(LED3_IO,0);\
}while(0)
#define LED_LEFT_UP()   do{\
    gpio_direction_input(LED1_IO);\
    gpio_set_pull_up(LED1_IO,0);\
    gpio_set_pull_down(LED1_IO,0);\
    gpio_direction_output(LED2_IO,1);\
    gpio_direction_output(LED3_IO,0);\
}while(0)
#define LED_RIGHT_UP()   do{\
    gpio_direction_input(LED1_IO);\
    gpio_set_pull_up(LED1_IO,0);\
    gpio_set_pull_down(LED1_IO,0);\
    gpio_direction_output(LED2_IO,0);\
    gpio_direction_output(LED3_IO,1);\
}while(0)
#define LED_LEFT_DOWN()   do{\
    gpio_direction_input(LED2_IO);\
    gpio_set_pull_up(LED2_IO,0);\
    gpio_set_pull_down(LED2_IO,0);\
    gpio_direction_output(LED1_IO,0);\
    gpio_direction_output(LED3_IO,1);\
}while(0)
#define LED_RIGHT_DOWN()   do{\
    gpio_direction_input(LED2_IO);\
    gpio_set_pull_up(LED2_IO,0);\
    gpio_set_pull_down(LED2_IO,0);\
    gpio_direction_output(LED1_IO,1);\
    gpio_direction_output(LED3_IO,0);\
}while(0)


#define TWINKLING_CIRCLE_ONCE_TIMES   2000

static u8 led_on = 0;
static u8 eye_led_status;
static u8 status_flg = 0;
static u16 led_cnt = 0;
static u16 eye_timer;

static void gr202_led_scan(void)
{
    if (!led_on) {
        return;
    }

    switch (eye_led_status) {
    case LED_CLOSE:
        LED_ALL_CLOSE();
        break;
    case LED_UP_LIGHT:
        if (status_flg == 0) {
            status_flg++;
            LED_LEFT_UP();
        } else {
            status_flg = 0;
            LED_RIGHT_UP();
        }
        break;
    case LED_DOWN_LIGHT:
        if (status_flg == 0) {
            status_flg++;
            LED_LEFT_DOWN();
        } else {
            status_flg = 0;
            LED_RIGHT_DOWN();
        }
        break;
    case LED_ALL_LIGHT:
        if (status_flg == 0) {
            status_flg++;
            LED_LEFT_UP();
        } else if (status_flg == 1) {
            status_flg ++;
            LED_RIGHT_UP();
        } else if (status_flg == 2) {
            status_flg ++;
            LED_LEFT_DOWN();
        } else {
            status_flg  = 0;
            LED_RIGHT_DOWN();
        }
        break;
    case LED_TWINKLING:
        if (led_cnt < 500) {
            if (status_flg == 0) {
                status_flg++;
                LED_LEFT_UP();
            } else if (status_flg == 1) {
                status_flg ++;
                LED_RIGHT_UP();
            } else if (status_flg == 2) {
                status_flg ++;
                LED_LEFT_DOWN();
            } else {
                status_flg  = 0;
                LED_RIGHT_DOWN();
            }
        } else {
            LED_ALL_CLOSE();
        }

        led_cnt++;
        if (led_cnt >= 1000) {
            led_cnt = 0;
        }
        break;
    case LED_TWINKLING_CIRCLE:
        if (led_cnt < (TWINKLING_CIRCLE_ONCE_TIMES / 5 * 1)) {
            if (status_flg == 0) {
                status_flg++;
                LED_LEFT_UP();
            } else if (status_flg == 1) {
                status_flg ++;
                LED_RIGHT_UP();
            } else if (status_flg == 2) {
                status_flg ++;
                LED_LEFT_DOWN();
            } else {
                status_flg  = 0;
                LED_RIGHT_DOWN();
            }
        } else if (led_cnt < (TWINKLING_CIRCLE_ONCE_TIMES / 5 * 2)) {
            LED_ALL_CLOSE();
        } else if (led_cnt < (TWINKLING_CIRCLE_ONCE_TIMES / 5 * 3)) {
            if (status_flg == 0) {
                status_flg++;
                LED_LEFT_DOWN();
            } else {
                status_flg = 0;
                LED_RIGHT_DOWN();
            }
        } else if (led_cnt < (TWINKLING_CIRCLE_ONCE_TIMES / 5 * 4)) {
            if (status_flg == 0) {
                status_flg++;
                LED_LEFT_UP();
            } else if (status_flg == 1) {
                status_flg ++;
                LED_RIGHT_UP();
            } else if (status_flg == 2) {
                status_flg ++;
                LED_LEFT_DOWN();
            } else {
                status_flg  = 0;
                LED_RIGHT_DOWN();

            }
        } else if (led_cnt < (TWINKLING_CIRCLE_ONCE_TIMES)) {
            if (status_flg == 0) {
                status_flg++;
                LED_LEFT_UP();
            } else {
                status_flg = 0;
                LED_RIGHT_UP();
            }
        } else {
            led_cnt = 0;
        }
        led_cnt ++;
        break;

    default:
        break;
    }
}

static int dec_pause_handler(const char *type, u32 arg)
{
    eye_led_status = LED_ALL_LIGHT;

    return 0;
}

static int dec_start_handler(const char *type, u32 arg)
{
    led_cnt = 0;
    status_flg = 0;
    eye_led_status = LED_TWINKLING_CIRCLE;

    return 0;
}

static int init_handler(const char *type, u32 arg)
{
    LED_INIT();

    led_on = 1;
    eye_led_status = LED_ALL_LIGHT;
    eye_timer = sys_hi_timer_add(NULL, (void(*)(void *))gr202_led_scan, 1);

    return 0;
}

static int uninit_handler(const char *type, u32 arg)
{
    sys_hi_timer_del(eye_timer);
    eye_timer = 0;

    return 0;
}

static const struct uimsg_handl led_msg_handler[] = {
    { "dec_start",  dec_start_handler        },
    { "dec_pause",  dec_pause_handler         },
    { "dec_stop",   dec_pause_handler        },
    { "dec_end",    dec_pause_handler        },
    { "init",       init_handler       },
    { "uninit",     uninit_handler     },
    { NULL, NULL},      /* 必须以此结尾！ */
};

void led_ui_event_handler(struct sys_event *e, void *priv)
{
    struct intent it;
    struct key_event *event = (struct key_event *)e->payload;

    if (event->action == KEY_EVENT_CLICK && event->value == KEY_RIGHT) {
        led_on = !led_on;

        if (!led_on) {
            LED_ALL_CLOSE();
            gpio_direction_output(LED0_IO, 0);
        } else {
            led_cnt = 0;
            status_flg = 0;
            LED_INIT();
            gpio_direction_output(LED0_IO, 1);
        }

        it.name = "app_music";
        it.action = ACTION_MUSIC_PLAY_VOICE_PROMPT;
        it.data = led_on ? "LightOpen.mp3" : "LightClose011.mp3";
        it.exdata = 1;
        start_app(&it);
    }
}

static int gr202_led_ui_init(void)
{
    gpio_direction_output(LED0_IO, 1);
    led_ui_register_msg_handler("gr202_led", led_msg_handler);
    return 0;
}
late_initcall(gr202_led_ui_init);

#endif

