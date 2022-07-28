#include "app_config.h"
#include "wifi/wifi_connect.h"
#include "system/includes.h"
#include "asm/power_interface.h"

#ifdef USE_LOW_POWER_TEST

static void sta_low_power_test_task(void *p)
{
    while (1) {
        puts(" WIFI STA low_power DISABLE ... \r\n");
        low_power_hw_unsleep_lock();//调用这句话以后开始不允许WIFI进入休眠,通常需要WIFI全力工作的工况下启用


        os_time_dly(10 * 100);

        low_power_hw_unsleep_unlock(); //调用这句话以后开始允许WIFI进入休眠,通常WIFI开始进入空闲工况下启用
        puts(" WIFI STA low_power ENABLE ... \r\n");

        os_time_dly(60 * 100);
    }
}

static void c_main(void *priv)
{
    thread_fork("sta_low_power_test_task", 10, 512, 0, NULL, sta_low_power_test_task, NULL);
}

late_initcall(c_main);

#endif //USE_LOW_POWER_TEST

