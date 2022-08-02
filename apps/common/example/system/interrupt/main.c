#include "app_config.h"
#include "system/includes.h"
#include "device/device.h"
#include "asm/clock.h"

//用户只能选择：JL_TIMER2、JL_TIMER3、JL_TIMER4、JL_TIMER5
//PWM使用定时器2或者3映射PWM时候，不能用于定时

#ifdef  USE_INTERRUPT_TEST_DEMO

static JL_TIMER_TypeDef *TMR = JL_TIMER4;//选择定时器4
static u8 timer_irq = IRQ_TIMER4_IDX;//选择定时器4

static const u8 timer_index[16] =  {0, 4, 1, 5, 2,  6,  3,  7,   8,   12,  9,    13,   10,   14,   11,    15};
static const u32 timer_table[16] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
static u32 timer_clk = 0;

#define AWINLINE   __attribute__((always_inline))

//#define TSEC SEC_USED(.volatile_ram_code) //当中断1ms一下，建议使用函数放在内部ram
#define TSEC

static AWINLINE TSEC void timer_cfg(u32 freq, u32 us)
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
#else
    //timer_freq_set(10000);//设置10K频率进中断
#endif
    return 0;
}
late_initcall(c_main);
#endif
