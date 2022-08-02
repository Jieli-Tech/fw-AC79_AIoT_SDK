#include "app_config.h"
#include "device/device.h"//u8
#include "server/video_dec_server.h"//fopen
#include "system/includes.h"//GPIO
#include "asm/gpio.h"

#ifdef USE_HC_SR04_DEMO

//边缘捕获使用定时器 2 5
//采用定时器一个捕获高电平一个捕获低电平
// trig _________----______________________________  触发信号一个大于10us的高电平
//
// echo _________________--------------------______  触发后会返回一个高电平时间为测量结果
#define SENSOR_ECHO   IO_PORTA_05
#define SENSOR_TRIG   IO_PORTA_06

static int timer2_cnt;
static int timer5_cnt;


___interrupt static void timer2_isr()
{
    JL_TIMER2->CON |= BIT(14);
    timer2_cnt = JL_TIMER2->PRD;
}

//rise_edge: 1 上升沿 0下降沿
static void timer2_init(u8 rise_edge, int gpio)
{
    JL_TIMER2->CNT = 0;
    request_irq(IRQ_TIMER2_IDX, 1, timer2_isr, 1);
    JL_TIMER2->CON = BIT(7) | BIT(6);	//lsb_clk 512分频

    //select timer2 IOMAP
    JL_IOMAP->CON1 |= BIT(28);
    JL_IOMAP->CON2 &= ~(0x3f << 16);
    JL_IOMAP->CON2 |= (gpio / 16 * 10 + gpio % 16) << 16;

    if (rise_edge) {	//上升沿捕获
        JL_TIMER2->CON |= BIT(1);
    } else {			//下降沿捕获
        JL_TIMER2->CON |= BIT(0) | BIT(1);
    }
}

static void timer2_uninit(void)
{
    JL_TIMER2->CON = 0;
    unrequest_irq(IRQ_TIMER2_IDX, 1);
}

static void timer2_start(void)
{
    JL_TIMER2->CNT = 0;
}

___interrupt static void timer5_isr()
{
    JL_TIMER5->CON |= BIT(14);
    timer5_cnt = JL_TIMER5->PRD;
}

//rise_edge: 1 上升沿 0下降沿
static void timer5_init(u8 rise_edge, int gpio)
{
    JL_TIMER5->CNT = 0;
    request_irq(IRQ_TIMER5_IDX, 1, timer5_isr, 1);
    JL_TIMER5->CON = BIT(7) | BIT(6);	//lsb_clk 512分频

    //select timer5 IOMAP
    JL_IOMAP->CON1 |= BIT(31);
    JL_IOMAP->CON2 &= ~(0x3f << 16);
    JL_IOMAP->CON2 |= (gpio / 16 * 10 + gpio % 16) << 16;

    if (rise_edge) {	//上升沿捕获
        JL_TIMER5->CON |= BIT(1);
    } else {			//下降沿捕获
        JL_TIMER5->CON |= BIT(0) | BIT(1);
    }
}

static void timer5_uninit(void)
{
    JL_TIMER5->CON = 0;
    unrequest_irq(IRQ_TIMER5_IDX, 1);
}

static void timer5_start(void)
{
    JL_TIMER5->CNT = 0;
}


static void timer_cap_init(int gpio)
{
    timer2_init(1, gpio);
    timer5_init(0, gpio);
}

static void timer_cap_uninit()
{
    timer2_uninit();
    timer5_uninit();
}

static void timer_cap_start()
{
    timer2_start();
    timer5_start();
}

static u8 timer_cap_interrupt()
{
    return timer5_interrupt();
}

static int timer_cap_get_cnt()
{
    return (timer5_cnt - timer2_cnt) ;
}

static void hc_sr04_music_test_init(void *priv)
{
    int cnt;
    float time_us;
    float aim_cm;

    timer_cap_init(SENSOR_ECHO);//初始化定时器
    while (1) {
        gpio_direction_output(SENSOR_TRIG, 0);
        os_time_dly(1);
        gpio_direction_output(SENSOR_TRIG, 1); //触发信号10ms高电平
        os_time_dly(1);
        gpio_direction_output(SENSOR_TRIG, 0);
        timer_cap_start(); //清0定时器计数器
        os_time_dly(10);
        cnt = timer_cap_get_cnt(); //得到计数次数
        time_us = cnt / (533 / 512); //得到搞电平时间
        aim_cm = time_us / 5.8;    //换算结果
        if (aim_cm > 0) {
            printf(">>>>>>>>>>>>>>>>>>>>>>get_sensor = %fcm", aim_cm);
        } else {
            printf("[ERROR]not have sensor please connect  ECHO->IO_PORTA_05 TRIG->IO_PORTA_06");
        }
    }
}

static int hc_sr04_test_task_init(void)
{
    puts("hc_sr04_test_task_init \n\n");
    return thread_fork("hc_sr04_music_test_init", 11, 1024, 0, 0, hc_sr04_music_test_init, NULL);
}
late_initcall(hc_sr04_test_task_init);

#endif
