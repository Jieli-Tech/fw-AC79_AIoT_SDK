#include "includes.h"
#include "os/os_api.h"
#include "asm/clock.h"
#include "asm/irq.h"
//#include "jiffies.h"
#include "system/timer.h"
#include "system/init.h"
#include "gpio.h"
//#include "asm/power_interface.h"
#include "generic/atomic.h"
#include "asm/cpu.h"
#include "demo_config.h"


#ifdef  USE_TIMER_TEST2_DEMO

//用户只能选择：JL_TIMER2、JL_TIMER3、JL_TIMER4、JL_TIMER5
//PWM使用定时器2或者3映射PWM时候，不能用于定时
static JL_TIMER_TypeDef *TMR = JL_TIMER4;//选择定时器4
static u8 timer_irq = IRQ_TIMER4_IDX;//选择定时器4
//#define JL_TIMER_B    JL_TIMER5

//u32 timer_b_cnt;

static const u8 timer_index[16] =  {0, 4, 1, 5, 2,  6,  3,  7,   8,   12,  9,    13,   10,   14,   11,    15};
static const u32 timer_table[16] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
static u32 timer_clk = 0;
void timer4_stop(void);
void timer4_run(u32 precesion);
// timer_b_test(void);
void test(void);


#define AWINLINE   __attribute__((always_inline))

//#define TSEC SEC_USED(.volatile_ram_code) //当中断1ms一下，建议使用函数放在内部ram
#define TSEC
#define TIMER_CON               JL_TIMER4->CON      //定时器4的控制寄存器
#define TIMER_CNT               JL_TIMER4->CNT      //定时器4的计数寄存器
#define TIMER_PRD               JL_TIMER4->PRD      //定时器4的周期寄存器
//#define TIMER_VETOR             IRQ_TIMER4_IDX

struct timer_hdl {
    u32 ticks;
    int index;
    int prd;
    u32 fine_cnt;
    u8 run;
    u32 precesion;
    void *power_ctrl;
    struct list_head head;
};

static struct timer_hdl hdl;

#define __this  (&hdl)

static const u32 timer_div[] = {
    /*0000*/    1,
    /*0001*/    4,
    /*0010*/    16,
    /*0011*/    64,
    /*0100*/    2,
    /*0101*/    8,
    /*0110*/    32,
    /*0111*/    128,
    /*1000*/    256,
    /*1001*/    4 * 256,
    /*1010*/    16 * 256,
    /*1011*/    64 * 256,
    /*1100*/    2 * 256,
    /*1101*/    8 * 256,
    /*1110*/    32 * 256,
    /*1111*/    128 * 256,
};

#define APP_TIMER_CLK           clk_get("osc")   //获得时钟源
#define MAX_TIME_CNT            0x7fff          //最大计数值
#define MIN_TIME_CNT            0x100           //最小计数值

static AWINLINE TSEC void timer_cfg(u32 freq, u32 us) //定时中断配置
{
    u32 clock = timer_clk;
    u8 psc = 0;
    u8 tmp = 0;
    u32 prd = 0;
    u32 ts = us / (1000 * 1000);//计算秒
    u32 tu = us % (1000 * 1000);//计算秒剩余us
    u8 i;
    float tp = 0;

    if (freq >= clock) {
        freq = clock;
    } else if (freq <= 1) {
        freq = 1;
        if (ts) {
            tp = (float)tu / (1000 * 1000);
        }
    }
    /*求prd值*/
    prd = clock / freq;
    if (prd > 65535) {
        for (psc = 0; psc < 16; psc++) {
            prd = (u32)(clock / (timer_table[psc]) / freq);
            if (prd <= 65535) {
                break;
            } else if (psc == 15) {
                prd = 65535;
                break;
            }
        }
    }
    prd = ts ? (prd * ts + tp * prd) : prd;
    psc = timer_index[psc];
    TMR->CON = 0;
    TMR->CNT = 0;
    TMR->CON |= BIT(14);
    TMR->PRD = prd;
    TMR->CON |= psc << 4; //lsb_clk分频
    TMR->CON |= BIT(0);
}
static void TSEC timer_set(u32 us)//设置时间，该函数可以在中断调用重新设置定时器值
{
    u32 freq = 1000000 / us;
    timer_cfg(freq, us);
}
static void TSEC timer_freq_set(u32 freq)//设置频率，该函数可以在中断调用重新设置定时器值
{
    timer_cfg(freq, 0);
}
static ___interrupt TSEC void timer_isr(void)//中断
{
    if (TMR->CON & BIT(15)) {
        TMR->CON |= BIT(14);
        putchar('@');
        //todo，中断函数执行程序...
    }
}

//example test
static int c_main(void)
{
    timer_clk = clk_get("timer");//先获取定时器的时钟源
    request_irq(timer_irq, 3, timer_isr, 0);//注册中断函数定和中断优先级
#if 1
    timer_set(50 * 1000); //初始化50ms进一次中断
    os_time_dly(500);
    timer4_stop();        //定时器停止
    os_time_dly(500);
    timer4_run(2);        //定时器开始
    os_time_dly(500);
    test();               //输入捕获功能测试
#else
    //timer_freq_set(10000);//设置10K频率进中断
#endif
    return 0;
}

static void timer4_suspend(void)  //定时器暂停
{
    if (__this->run) {
        __this->ticks = JL_TIMER4->CNT;
        JL_TIMER4->CON = 0;
    }
}



static void timer4_resume(void) //定时器继续
{
    if (__this->run == 0) {
        return;
    }
    u32 prd_cnt;
    u8 index;
    //precesion = PRD * index / CLK
    for (index = 0; index < (sizeof(timer_div) / sizeof(timer_div[0])); index++) {
        prd_cnt = __this->precesion * (APP_TIMER_CLK / 1000) / timer_div[index];
        if (prd_cnt > MIN_TIME_CNT && prd_cnt < MAX_TIME_CNT) {
            break;
        }
    }

    __this->index = index;
    __this->prd = prd_cnt;
    JL_TIMER4->CNT = __this->ticks;
    JL_TIMER4->PRD = prd_cnt; //2ms
    JL_TIMER4->CON = (index << 4) | BIT(0) | BIT(3);
}

void timer4_stop(void)   //定时器停止
{
    printf("--->Stop");
    JL_TIMER4->CON = BIT(14);
    __this->run = 0;
    __this->precesion = 0;
}



void timer4_run(u32 precesion) //定时器开始

{
    precesion = 2;//br23 br25 周期不变化  为减少占用cpu资源，固定收集2ms内的计数值 可去掉
    if (__this->precesion == precesion) {
        return;
    }
    printf("-->Run %d", precesion);
    JL_TIMER4->CON = BIT(14);
    __this->precesion = precesion;
    __this->run = 1;
    __this->ticks = 0;
    timer4_resume();
}

//////输入捕获
static bool isr_flag = false;   //设立中断标志位
enum edge_mode {
    RISE_EDGE = 0,
    FALL_EDGE,
};

bool timer5_interrupt(void)
{
    return isr_flag;
}


void timer5_start(void)     //定时器5开始，清空计数器和标志位
{
    JL_TIMER5->CNT = 0;
    isr_flag = false;

}

___interrupt
static void timer5_isr(void)
{
    JL_TIMER5->CON |= BIT(14);
    isr_flag = true;
}

int timer5_get_cnt(void)	//10us
{
    u32 a = JL_TIMER5->PRD;
    u32 b = a * 96 / 100;  //53Mhz/512k 再转化为10us

    return b   ;
}

void timer5_init(int mode)     //定时器5初始化
{
    JL_TIMER5->CNT = 0;
#if CPU_CORE_NUM > 1
    request_irq(IRQ_TIMER5_IDX, 1, timer5_isr, 1);
#else
    request_irq(IRQ_TIMER5_IDX, 1, timer5_isr, 0);
#endif
    JL_TIMER5->CON = BIT(7) | BIT(6);//lsb_clk的512分频
    if (mode == FALL_EDGE) {
        JL_TIMER5->CON |= BIT(0) | BIT(1);//下降沿捕获
        gpio_direction_input(IO_PORTC_08);//输入引脚为PC8
        gpio_set_pull_up(IO_PORTC_08, 1);
    } else {
        JL_TIMER5->CON |= BIT(1);//上升沿捕获
        gpio_direction_input(IO_PORTC_08);
        gpio_set_pull_down(IO_PORTC_08, 1);
    }
}

void timer5_uninit(void)
{
    JL_TIMER5->CON = 0;
}
void test(void)
{
    extern bool timer5_interrupt(void);
    extern void timer5_start(void);
    extern void timer5_init(int mode);	//mode： 0 上升沿  1 下降沿
    extern void timer5_uninit(void);
    extern int timer5_get_cnt(void);


    timer5_init(1);
    printf("-------%s-------%d\r\n", __func__, __LINE__);

    while (1) {
        timer5_start();	//先清空下数据
        printf("-------%s-------%d\r\n", __func__, __LINE__);

        while (!timer5_interrupt()) {	//检测到有下降沿则为true,采用轮询方式每200ms检测一次
            os_time_dly(20);
        }
        printf("-------%s-------%d get cnt = %d\r\n", __func__, __LINE__, timer5_get_cnt());	//返回出来的是单位是10us 的cnt  就是 cnt* 100 000就是秒
        /* timer5_get_cnt(); */


    }
    timer5_uninit();	//用完就释放
}
late_initcall(c_main);


#endif
