/**
 * @file tuya_os_init.c
 * @author sunkz@tuya.com
 * @brief
 * @version 0.1
 * @date 2020-05-15
 *
 * @copyright Copyright (c) tuya.inc 2019
 *
 */

#include "tuya_os_adapter.h"
#include "tuya_os_init.h"

extern int platform_uart_init(void);
extern int platform_pin_init(void);
extern int platform_pwm_init(void);
extern int platform_i2c_init(void);
extern int platform_timer_init(void);
extern int platform_adc_init(void);
extern int platform_rtc_init(void);

extern int tuya_os_adapt_reg_memory_intf(void);
extern int tuya_os_adapt_reg_mutex_intf(void);
extern int tuya_os_adapt_reg_semaphore_intf(void);
extern int tuya_os_adapt_reg_thread_intf(void);
extern int tuya_os_adapt_reg_network_intf(void);
extern int tuya_os_adapt_reg_output_intf(void);
extern int tuya_os_adapt_reg_system_intf(void);
extern int tuya_os_adapt_reg_queue_intf(void);
extern int tuya_os_adapt_reg_wifi_intf(void);
extern int tuya_os_adapt_reg_bt_intf(void);
extern int tuya_os_adapt_reg_storage_intf(void);
extern int tuya_os_adapt_reg_ota_intf(void);

void tuya_os_init(void)
{
    tuya_os_adapt_reg_memory_intf();
    tuya_os_adapt_reg_mutex_intf();
    tuya_os_adapt_reg_semaphore_intf();
    tuya_os_adapt_reg_thread_intf();
    tuya_os_adapt_reg_network_intf();
    tuya_os_adapt_reg_output_intf();
    tuya_os_adapt_reg_system_intf();
    tuya_os_adapt_reg_queue_intf();
    tuya_os_adapt_reg_wifi_intf();
    tuya_os_adapt_reg_bt_intf();
    tuya_os_adapt_reg_storage_intf();
    tuya_os_adapt_reg_ota_intf();

    platform_uart_init();
    platform_pin_init();
//    platform_pwm_init();
//    platform_i2c_init();
//    platform_timer_init();
//    platform_adc_init();
//    platform_rtc_init();

}

char *tuya_os_adapt_get_ver(void)
{
    return TUYA_OS_ADAPT_PLATFORM_VERSION;
}
