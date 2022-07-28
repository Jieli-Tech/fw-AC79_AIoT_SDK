#include "key/key_driver.h"
#include "device/gpio.h"
#include "system/timer.h"
#include "asm/clock.h"
#include "app_config.h"

#if TCFG_IRKEY_ENABLE

#define ir_log 	log_d

//红外定时器定义
#define IR_TIMER                    5
#define IR_IRQ_TIME_IDX             IRQ_TIMER5_IDX
#define IR_TIME_REG                 JL_TIMER5

typedef struct _IR_CODE {
    u16 wData;          //<键值
    u16 wUserCode;      //<用户码
    u16 timer_pad;
    u8  bState;         //<接收状态
    u8  boverflow;      //<红外信号超时
} IR_CODE;

static IR_CODE ir_code;       ///<红外遥控信息

static const u16 timer_div[] = {
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

#define IRFLT_OUTPUT_TIMER_SEL(x)		SFR(JL_IOMAP->CON0, 8, 3, (x+2))


___interrupt
static void timer_ir_isr(void)
{
    u32 bCap1;
    u8 cap = 0;

    IR_TIME_REG->CON |= BIT(14);

    bCap1 = IR_TIME_REG->PRD;
    IR_TIME_REG->CNT = 0;
    cap = bCap1 / ir_code.timer_pad;

    /* static u8 cnt = 0; */
    /* ir_log("cnt = %d, cap = 0x%x", cnt++, cap); */
    if (cap <= 1) {
        ir_code.wData >>= 1;
        ir_code.bState++;
        ir_code.boverflow = 0;
    } else if (cap == 2) {
        ir_code.wData >>= 1;
        ir_code.bState++;
        ir_code.wData |= 0x8000;
        ir_code.boverflow = 0;
    }
    /*13ms-Sync*/
    /*
    else if ((cap == 13) || (cap < 8) || (cap > 110))
    {
        ir_code.bState = 0;
    }
    else
    {
        ir_code.boverflow = 0;
    }
    */
    else if ((cap == 13) && (ir_code.boverflow < 8)) {
        ir_code.bState = 0;
    } else if ((cap < 8) && (ir_code.boverflow < 5)) {
        ir_code.bState = 0;
    } else if ((cap > 110) && (ir_code.boverflow > 53)) {
        ir_code.bState = 0;
    } else if ((cap > 20) && (ir_code.boverflow > 53)) { //溢出情况下 （12M 48M）
        ir_code.bState = 0;
    } else {
        ir_code.boverflow = 0;
    }
    if (ir_code.bState == 16) {
        ir_code.wUserCode = ir_code.wData;
    }
    if (ir_code.bState == 32) {
        /* printf("[0x%X]",ir_code.wData); */
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   ir按键初始化
   @param   void
   @param   void
   @return  void
   @note    void set_ir_clk(void)

   ((cnt - 1)* 分频数)/lsb_clk = 1ms
*/
/*----------------------------------------------------------------------------*/
#define APP_TIMER_CLK           clk_get("osc")
#define TIMER_UNIT_MS           1
#define MAX_TIME_CNT 			0x07ff //分频准确范围，更具实际情况调整
#define MIN_TIME_CNT 			0x0030

static void set_ir_clk(void)
{
    u32 clk;
    u32 prd_cnt;
    u8 index = 0;

    for (index = 0; index < (sizeof(timer_div) / sizeof(timer_div[0])); ++index) {
        prd_cnt = TIMER_UNIT_MS * (APP_TIMER_CLK / 1000) / timer_div[index];
        if (prd_cnt > MIN_TIME_CNT && prd_cnt < MAX_TIME_CNT) {
            break;
        }
    }

    ir_code.timer_pad = prd_cnt;
    IR_TIME_REG->CON = ((index << 4) | BIT(3) | BIT(1) | BIT(0));
}

u8 get_irflt_value(void)
{
    u8 tkey = 0xff;

    if (ir_code.bState != 32) {
        return tkey;
    }

    /* printf("(0x%X_%x)",ir_code.wUserCode,ir_code.wData); */
    if ((((u8 *)&ir_code.wData)[0] ^ ((u8 *)&ir_code.wData)[1]) == 0xff) {
        /* if (ir_code.wUserCode == 0xFF00) */
        {
            /* printf("<%d>",(u8)ir_code.wData); */
            tkey = (u8)ir_code.wData;
        }
    } else {
        ir_code.bState = 0;
    }

    return tkey;
}

void ir_input_io_sel(u8 port)
{
    if (port > IO_PORT_USB_DMB) {
        return;
    }

    if (port >= IO_PORTA_00 && port <= IO_PORTA_10) {
        port = port - IO_PORTA_00;
    } else if (port >= IO_PORTB_00 && port <= IO_PORTB_08) {
        port = port - IO_PORTB_00 + 11;
    } else if (port >= IO_PORTC_00 && port <= IO_PORTC_10) {
        port = port - IO_PORTC_00 + 20;
    } else if (port >= IO_PORTE_00 && port <= IO_PORTE_09) {
        port = port - IO_PORTE_00 + 31;
    } else if (port >= IO_PORTH_00 && port <= IO_PORTH_09) {
        port = port - IO_PORTH_00 + 41;
    } else if (port >= IO_PORTG_08 && port <= IO_PORTG_15) {
        port = port - IO_PORTG_08 + 51;
    } else if (port >= IO_PORT_USB_DPA && port <= IO_PORT_USB_DMB) {
        port = port - IO_PORT_USB_DPA + 60;
    }

    JL_IOMAP->CON2 &= ~(0b111111 << 8);
    JL_IOMAP->CON2 |= port << 8;

    gpio_set_direction(port, 1);
    gpio_set_die(port, 1);
}

void ir_output_timer_sel(void)
{
    IRFLT_OUTPUT_TIMER_SEL(IR_TIMER);
    request_irq(IR_IRQ_TIME_IDX, 5, timer_ir_isr, 0);
}

static void ir_timeout(void *priv)
{
    if (++ir_code.boverflow > 56) { //56*2ms ~= 112ms
        ir_code.bState = 0;
    }
}

void ir_timeout_set(void)
{
    sys_s_hi_timer_add(NULL, ir_timeout, 2); //2ms
}

void irflt_config(void)
{
    JL_IR->RFLT_CON = 0;
    JL_IR->RFLT_CON |= BIT(7);		//256 div
    JL_IR->RFLT_CON |= BIT(3);		//osc
    JL_IR->RFLT_CON |= BIT(0);		//irflt enable

    set_ir_clk();
}

void log_irflt_info(void)
{
    ir_log("IOMC0 = 0x%x", JL_IOMAP->CON0);
    ir_log("IOMC2 = 0x%x", JL_IOMAP->CON2);
    ir_log("RFLT_CON = 0x%x", JL_IR->RFLT_CON);
    ir_log("IR_TIME_REG = 0x%x", IR_TIME_REG->CON);
}

#endif
