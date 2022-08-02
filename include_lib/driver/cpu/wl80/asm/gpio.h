#ifndef ASM_PORT_IO_H
#define ASM_PORT_IO_H


#define IO_GROUP_NUM 		16

#define IO_PORTA_BASE 		(ls_io_base + 0x00*4)

#define IO_PORTA_00 				(IO_GROUP_NUM * 0 + 0)
#define IO_PORTA_01 				(IO_GROUP_NUM * 0 + 1)
#define IO_PORTA_02 				(IO_GROUP_NUM * 0 + 2)
#define IO_PORTA_03 				(IO_GROUP_NUM * 0 + 3)
#define IO_PORTA_04 				(IO_GROUP_NUM * 0 + 4)
#define IO_PORTA_05 				(IO_GROUP_NUM * 0 + 5)
#define IO_PORTA_06 				(IO_GROUP_NUM * 0 + 6)
#define IO_PORTA_07 				(IO_GROUP_NUM * 0 + 7)
#define IO_PORTA_08 				(IO_GROUP_NUM * 0 + 8)
#define IO_PORTA_09 				(IO_GROUP_NUM * 0 + 9)
#define IO_PORTA_10 				(IO_GROUP_NUM * 0 + 10)
#define IO_PORTA_11 				(IO_GROUP_NUM * 0 + 11)
#define IO_PORTA_12 				(IO_GROUP_NUM * 0 + 12)
#define IO_PORTA_13 				(IO_GROUP_NUM * 0 + 13)
#define IO_PORTA_14 				(IO_GROUP_NUM * 0 + 14)
#define IO_PORTA_15 				(IO_GROUP_NUM * 0 + 15)

#define IO_PORTB_00 				(IO_GROUP_NUM * 1 + 0)
#define IO_PORTB_01 				(IO_GROUP_NUM * 1 + 1)
#define IO_PORTB_02 				(IO_GROUP_NUM * 1 + 2)
#define IO_PORTB_03 				(IO_GROUP_NUM * 1 + 3)
#define IO_PORTB_04 				(IO_GROUP_NUM * 1 + 4)
#define IO_PORTB_05 				(IO_GROUP_NUM * 1 + 5)
#define IO_PORTB_06 				(IO_GROUP_NUM * 1 + 6)
#define IO_PORTB_07 				(IO_GROUP_NUM * 1 + 7)
#define IO_PORTB_08 				(IO_GROUP_NUM * 1 + 8)
#define IO_PORTB_09 				(IO_GROUP_NUM * 1 + 9)
#define IO_PORTB_10 				(IO_GROUP_NUM * 1 + 10)
#define IO_PORTB_11 				(IO_GROUP_NUM * 1 + 11)
#define IO_PORTB_12 				(IO_GROUP_NUM * 1 + 12)
#define IO_PORTB_13 				(IO_GROUP_NUM * 1 + 13)
#define IO_PORTB_14 				(IO_GROUP_NUM * 1 + 14)
#define IO_PORTB_15 				(IO_GROUP_NUM * 1 + 15)

#define IO_PORTC_00 				(IO_GROUP_NUM * 2 + 0)
#define IO_PORTC_01 				(IO_GROUP_NUM * 2 + 1)
#define IO_PORTC_02 				(IO_GROUP_NUM * 2 + 2)
#define IO_PORTC_03 				(IO_GROUP_NUM * 2 + 3)
#define IO_PORTC_04 				(IO_GROUP_NUM * 2 + 4)
#define IO_PORTC_05 				(IO_GROUP_NUM * 2 + 5)
#define IO_PORTC_06 				(IO_GROUP_NUM * 2 + 6)
#define IO_PORTC_07 				(IO_GROUP_NUM * 2 + 7)
#define IO_PORTC_08 				(IO_GROUP_NUM * 2 + 8)
#define IO_PORTC_09 				(IO_GROUP_NUM * 2 + 9)
#define IO_PORTC_10 				(IO_GROUP_NUM * 2 + 10)
#define IO_PORTC_11 				(IO_GROUP_NUM * 2 + 11)
#define IO_PORTC_12 				(IO_GROUP_NUM * 2 + 12)
#define IO_PORTC_13 				(IO_GROUP_NUM * 2 + 13)
#define IO_PORTC_14 				(IO_GROUP_NUM * 2 + 14)
#define IO_PORTC_15 				(IO_GROUP_NUM * 2 + 15)

#define IO_PORTD_00 				(IO_GROUP_NUM * 3 + 0)
#define IO_PORTD_01 				(IO_GROUP_NUM * 3 + 1)
#define IO_PORTD_02 				(IO_GROUP_NUM * 3 + 2)
#define IO_PORTD_03 				(IO_GROUP_NUM * 3 + 3)
#define IO_PORTD_04 				(IO_GROUP_NUM * 3 + 4)
#define IO_PORTD_05 				(IO_GROUP_NUM * 3 + 5)
#define IO_PORTD_06 				(IO_GROUP_NUM * 3 + 6)
#define IO_PORTD_07 				(IO_GROUP_NUM * 3 + 7)
#define IO_PORTD_08 				(IO_GROUP_NUM * 3 + 8)
#define IO_PORTD_09 				(IO_GROUP_NUM * 3 + 9)
#define IO_PORTD_10 				(IO_GROUP_NUM * 3 + 10)
#define IO_PORTD_11 				(IO_GROUP_NUM * 3 + 11)
#define IO_PORTD_12 				(IO_GROUP_NUM * 3 + 12)
#define IO_PORTD_13 				(IO_GROUP_NUM * 3 + 13)
#define IO_PORTD_14 				(IO_GROUP_NUM * 3 + 14)
#define IO_PORTD_15 				(IO_GROUP_NUM * 3 + 15)

#define IO_PORTE_00 				(IO_GROUP_NUM * 4 + 0)
#define IO_PORTE_01 				(IO_GROUP_NUM * 4 + 1)
#define IO_PORTE_02 				(IO_GROUP_NUM * 4 + 2)
#define IO_PORTE_03 				(IO_GROUP_NUM * 4 + 3)
#define IO_PORTE_04 				(IO_GROUP_NUM * 4 + 4)
#define IO_PORTE_05 				(IO_GROUP_NUM * 4 + 5)
#define IO_PORTE_06 				(IO_GROUP_NUM * 4 + 6)
#define IO_PORTE_07 				(IO_GROUP_NUM * 4 + 7)
#define IO_PORTE_08 				(IO_GROUP_NUM * 4 + 8)
#define IO_PORTE_09 				(IO_GROUP_NUM * 4 + 9)
#define IO_PORTE_10 				(IO_GROUP_NUM * 4 + 10)
#define IO_PORTE_11 				(IO_GROUP_NUM * 4 + 11)
#define IO_PORTE_12 				(IO_GROUP_NUM * 4 + 12)
#define IO_PORTE_13 				(IO_GROUP_NUM * 4 + 13)
#define IO_PORTE_14 				(IO_GROUP_NUM * 4 + 14)
#define IO_PORTE_15 				(IO_GROUP_NUM * 4 + 15)

#define IO_PORTF_00 				(IO_GROUP_NUM * 5 + 0)
#define IO_PORTF_01 				(IO_GROUP_NUM * 5 + 1)
#define IO_PORTF_02 				(IO_GROUP_NUM * 5 + 2)
#define IO_PORTF_03 				(IO_GROUP_NUM * 5 + 3)
#define IO_PORTF_04 				(IO_GROUP_NUM * 5 + 4)
#define IO_PORTF_05 				(IO_GROUP_NUM * 5 + 5)
#define IO_PORTF_06 				(IO_GROUP_NUM * 5 + 6)
#define IO_PORTF_07 				(IO_GROUP_NUM * 5 + 7)
#define IO_PORTF_08 				(IO_GROUP_NUM * 5 + 8)
#define IO_PORTF_09 				(IO_GROUP_NUM * 5 + 9)
#define IO_PORTF_10 				(IO_GROUP_NUM * 5 + 10)
#define IO_PORTF_11 				(IO_GROUP_NUM * 5 + 11)
#define IO_PORTF_12 				(IO_GROUP_NUM * 5 + 12)
#define IO_PORTF_13 				(IO_GROUP_NUM * 5 + 13)
#define IO_PORTF_14 				(IO_GROUP_NUM * 5 + 14)
#define IO_PORTF_15 				(IO_GROUP_NUM * 5 + 15)

#define IO_PORTG_00 				(IO_GROUP_NUM * 6 + 0)
#define IO_PORTG_01 				(IO_GROUP_NUM * 6 + 1)
#define IO_PORTG_02 				(IO_GROUP_NUM * 6 + 2)
#define IO_PORTG_03 				(IO_GROUP_NUM * 6 + 3)
#define IO_PORTG_04 				(IO_GROUP_NUM * 6 + 4)
#define IO_PORTG_05 				(IO_GROUP_NUM * 6 + 5)
#define IO_PORTG_06 				(IO_GROUP_NUM * 6 + 6)
#define IO_PORTG_07 				(IO_GROUP_NUM * 6 + 7)
#define IO_PORTG_08 				(IO_GROUP_NUM * 6 + 8)
#define IO_PORTG_09 				(IO_GROUP_NUM * 6 + 9)
#define IO_PORTG_10 				(IO_GROUP_NUM * 6 + 10)
#define IO_PORTG_11 				(IO_GROUP_NUM * 6 + 11)
#define IO_PORTG_12 				(IO_GROUP_NUM * 6 + 12)
#define IO_PORTG_13 				(IO_GROUP_NUM * 6 + 13)
#define IO_PORTG_14 				(IO_GROUP_NUM * 6 + 14)
#define IO_PORTG_15 				(IO_GROUP_NUM * 6 + 15)

#define IO_PORTH_00 				(IO_GROUP_NUM * 7 + 0)
#define IO_PORTH_01 				(IO_GROUP_NUM * 7 + 1)
#define IO_PORTH_02 				(IO_GROUP_NUM * 7 + 2)
#define IO_PORTH_03 				(IO_GROUP_NUM * 7 + 3)
#define IO_PORTH_04 				(IO_GROUP_NUM * 7 + 4)
#define IO_PORTH_05 				(IO_GROUP_NUM * 7 + 5)
#define IO_PORTH_06 				(IO_GROUP_NUM * 7 + 6)
#define IO_PORTH_07 				(IO_GROUP_NUM * 7 + 7)
#define IO_PORTH_08 				(IO_GROUP_NUM * 7 + 8)
#define IO_PORTH_09 				(IO_GROUP_NUM * 7 + 9)
#define IO_PORTH_10 				(IO_GROUP_NUM * 7 + 10)
#define IO_PORTH_11 				(IO_GROUP_NUM * 7 + 11)
#define IO_PORTH_12 				(IO_GROUP_NUM * 7 + 12)
#define IO_PORTH_13 				(IO_GROUP_NUM * 7 + 13)
#define IO_PORTH_14 				(IO_GROUP_NUM * 7 + 14)
#define IO_PORTH_15 				(IO_GROUP_NUM * 7 + 15)

#define IO_PORTI_00 				(IO_GROUP_NUM * 8 + 0)
#define IO_PORTI_01 				(IO_GROUP_NUM * 8 + 1)
#define IO_PORTI_02 				(IO_GROUP_NUM * 8 + 2)
#define IO_PORTI_03 				(IO_GROUP_NUM * 8 + 3)
#define IO_PORTI_04 				(IO_GROUP_NUM * 8 + 4)
#define IO_PORTI_05 				(IO_GROUP_NUM * 8 + 5)
#define IO_PORTI_06 				(IO_GROUP_NUM * 8 + 6)
#define IO_PORTI_07 				(IO_GROUP_NUM * 8 + 7)
#define IO_PORTI_08 				(IO_GROUP_NUM * 8 + 8)
#define IO_PORTI_09 				(IO_GROUP_NUM * 8 + 9)
#define IO_PORTI_10 				(IO_GROUP_NUM * 8 + 10)
#define IO_PORTI_11 				(IO_GROUP_NUM * 8 + 11)
#define IO_PORTI_12 				(IO_GROUP_NUM * 8 + 12)
#define IO_PORTI_13 				(IO_GROUP_NUM * 8 + 13)
#define IO_PORTI_14 				(IO_GROUP_NUM * 8 + 14)
#define IO_PORTI_15 				(IO_GROUP_NUM * 8 + 15)

#define IO_MAX_NUM 					(IO_PORTI_15+1)

#define IO_PORT_PR_00               (IO_MAX_NUM + 0)
#define IO_PORT_PR_01               (IO_MAX_NUM + 1)
#define IO_PORT_PR_02               (IO_MAX_NUM + 2)
#define IO_PORT_PR_03               (IO_MAX_NUM + 3)

#define IO_PR_MAX                   (IO_PORT_PR_03 + 1)

#define IO_PORT_USB_DPA              (IO_PR_MAX + 0)
#define IO_PORT_USB_DMA              (IO_PR_MAX + 1)
#define IO_PORT_USB_DPB              (IO_PR_MAX + 2)
#define IO_PORT_USB_DMB              (IO_PR_MAX + 3)

#define IO_PORTA					IO_PORTA_00
#define IO_PORTB					IO_PORTB_00
#define IO_PORTC					IO_PORTC_00
#define IO_PORTD					IO_PORTD_00
#define IO_PORTE					IO_PORTE_00
#define IO_PORTF					IO_PORTF_00
#define IO_PORTG					IO_PORTG_00
#define IO_PORTH					IO_PORTH_00

#define USB_IO_OFFSET               4
#define IO_PORT_DP                  (IO_MAX_NUM + USB_IO_OFFSET)
#define IO_PORT_DM                  (IO_MAX_NUM + USB_IO_OFFSET + 1)

#define GPIOA                       (IO_GROUP_NUM * 0)
#define GPIOB                       (IO_GROUP_NUM * 1)
#define GPIOC                       (IO_GROUP_NUM * 2)
#define GPIOD                       (IO_GROUP_NUM * 3)
#define GPIOE                       (IO_GROUP_NUM * 4)
#define GPIOF                       (IO_GROUP_NUM * 5)
#define GPIOG                       (IO_GROUP_NUM * 6)
#define GPIOH                       (IO_GROUP_NUM * 7)
#define GPIOR                       (IO_MAX_NUM)
#define GPIOUSB                     (IO_MAX_NUM + USB_IO_OFFSET)

enum {
    INPUT_CH0,
    INPUT_CH1,
    INPUT_CH2,
    INPUT_CH3,
};

enum gpio_op_mode {
    GPIO_SET = 1,
    GPIO_AND,
    GPIO_OR,
    GPIO_XOR,
};

enum gpio_direction {
    GPIO_OUT = 0,
    GPIO_IN = 1,
};

enum gpio_out_channle {
    CH0_UT0_TX,
    CH0_UT1_TX,
    CH0_T0_PWM_OUT,
    CH0_T1_PWM_OUT,
    CH0_RTOSL_CLK,
    CH0_BTOSC_CLK,
    CH0_PLL_12M,
    CH0_UT2_TX,
    CH0_CH0_PWM_H,
    CH0_CH0_PWM_L,
    CH0_CH1_PWM_H,
    CH0_CH1_PWM_L,
    CH0_CH3_PWM_H,
    CH0_CH3_PWM_L,
    CH0_PLNK0_SCLK_OUT,
    CH0_T5_PWM_OUT,

    CH1_UT0_TX = 0x10,
    CH1_UT1_TX,
    CH1_T0_PWM_OUT,
    CH1_T1_PWM_OUT,
    CH1_RTOSL_CLK,
    CH1_BTOSC_CLK,
    CH1_PLL_24M,
    CH1_UT2_TX,
    CH1_CH0_PWM_H,
    CH1_CH0_PWM_L,
    CH1_CH1_PWM_H,
    CH1_CH1_PWM_L,
    CH1_CH3_PWM_H,
    CH1_CH3_PWM_L,
    CH1_PLNK1_SCLK_OUT,
    CH1_T4_PWM_OUT,

    CH2_UT1_RTS = 0x20,
    CH2_UT1_TX,
    CH2_T0_PWM_OUT,
    CH2_T1_PWM_OUT,
    CH2_RTOSL_CLK,
    CH2_BTOSC_CLK,
    CH2_PLL_24M,
    CH2_UT2_TX,
    CH2_CH0_PWM_H,
    CH2_CH0_PWM_L,
    CH2_CH1_PWM_H,
    CH2_CH1_PWM_L,
    CH2_CH2_PWM_H,
    CH2_CH2_PWM_L,
    CH2_T2_PWM_OUT,
    CH2_T3_PWM_OUT,

    CH3_UT1_RTS = 0x30,
    CH3_UT1_TX,
    CH3_T0_PWM_OUT,
    CH3_T1_PWM_OUT,
    CH3_RTOSL_CLK,
    CH3_BTOSC_CLK,
    CH3_PLL_24M,
    CH3_UT2_TX,
    CH3_CH0_PWM_H,
    CH3_CH0_PWM_L,
    CH3_CH1_PWM_H,
    CH3_CH1_PWM_L,
    CH3_CH2_PWM_H,
    CH3_CH2_PWM_L,
    CH3_T2_PWM_OUT,
    CH3_T3_PWM_OUT,
};

struct gpio_reg {
    volatile unsigned long out;
    volatile unsigned long in;
    volatile unsigned long dir;
    volatile unsigned long die;
    volatile unsigned long pu;
    volatile unsigned long pd;
    volatile unsigned long hd0;
    volatile unsigned long hd;
    volatile unsigned long dieh;
};

#if 0
#define     IO_DEBUG_0(i,x)       {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     IO_DEBUG_1(i,x)       {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}
#define     IO_DEBUG_TOGGLE(i,x)  {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT ^= BIT(x);}
#else
#define     IO_DEBUG_0(i,x)         {}
#define     IO_DEBUG_1(i,x)         {}
#define     IO_DEBUG_TOGGLE(i,x)    {}
#endif


#endif

