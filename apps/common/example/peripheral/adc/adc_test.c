#include "generic/typedef.h"
#include "asm/clock.h"
#include "asm/adc_api.h"
#include "system/timer.h"
#include "asm/efuse.h"
#include "asm/p33.h"
#include "asm/power_interface.h"
#include "device/gpio.h"
#include "system/includes.h"
#include "app_config.h"
#include "demo_config.h"

#ifdef USE_ADC_TEST_DEMO

void c_main(void)
{

    /*测量AD通道*/
    void adc_init(void);
    adc_init();
    u32 adc_get_value(u32 ch);
    u32 adc_get_voltage(u32 ch);

    u32 adc_io = IO_PORTA_07;

    gpio_set_die(adc_io, 0);        //将io设为模拟功能
    //浮空输入

    gpio_set_direction(adc_io, 1);  //方向设为输入
    gpio_set_pull_up(adc_io, 0);    //关上拉10K
    gpio_set_pull_down(adc_io, 0);  //关下拉10K

    adc_add_sample_ch(AD_CH_PA07);//添加通道

    os_time_dly(100);
    u16 adc_val = 0;
    u16 io_vol = 0;

    adc_val = adc_get_value(AD_CH_PA07);
    while (1) {
        delay(10000000);
        adc_val = adc_get_value(AD_CH_PA07);
        io_vol = adc_get_voltage(AD_CH_PA07);
        printf("adc_val = %d  >>>  %d mv\n", adc_val, io_vol);
    }

}
late_initcall(c_main);

#endif
