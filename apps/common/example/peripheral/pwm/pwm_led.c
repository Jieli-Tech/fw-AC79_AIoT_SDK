#include "app_config.h"
#include "system/includes.h"
#include "asm/pwm.h"
#include "asm/pwm_led.h"
#include "device/device.h"

#ifdef USE_PWM_LED_TEST_DEMO
static int c_main(void)
{
    pwm_led_test();
    sys_timer_add(NULL, pwm_led_test, 4000);
    return 0;
}
late_initcall(c_main);

#endif
