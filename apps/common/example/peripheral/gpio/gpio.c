#include "asm/gpio.h"
#include "system/includes.h"
#include "app_config.h"
#include "asm/port_waked_up.h"
#include "device/gpio.h"
#include "demo_config.h"

#ifdef USE_GPIO_TEST_DEMO

static void *PA0IntHandler(void)   //外部中断处理

{
    printf("hello,jl");
}

static void c_main(void *priv)
{

    __gpio_direction_input(IO_PORTH_12);       //GPIO输入   同一组引脚之间没有互斥, 针对用户有高速度要求, 并且不需要考虑同一组引脚之间操作互斥才允许使用
    __gpio_set_pull_up(IO_PORTH_12, 1);        //上拉配置

    gpio_direction_output(IO_PORTC_01, 1);  //GPIO输出
    gpio_set_hd(IO_PORTC_01, 1);            //强驱配置

    //gpio_direction_input(IO_PORTA_01);         //GPIO输入
    //gpio_set_pull_down(IO_PORTA_01,1);
    os_time_dly(300);


    port_wakeup_reg(EVENT_IO_0, IO_PORTA_01, EDGE_POSITIVE, PA0IntHandler);//gpio中断模式使用（i/o口作外部中断，外部中断引脚，上升沿触发，中断响应函数）event_io_0可以映射到任意引脚



    gpio_direction_output(IO_PORT_USB_DPA, 1);
    gpio_direction_output(IO_PORT_USB_DMA, 1);  //特殊引脚如USB/配置为GPIO使用方法

    gpio_direction_input(IO_PORTH_04);         //GPIO输入,引脚PH4和PH6是双重绑定IO
    gpio_set_pull_down(IO_PORTH_04, 1);        //下拉配置

    os_time_dly(3000);
    gpio_direction_output(IO_PORTH_06, 1);

    //gpio_output_channle(IO_PORTE_00,00);
    //gpio_read(IO_PORTE_00);
    //ret=gpio_read(IO_PORTE_00);
    //printf("ret:",ret);



}

late_initcall(c_main);

#endif

