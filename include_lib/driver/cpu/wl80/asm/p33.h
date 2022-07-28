/*********************************************************************************************
    *   Filename        : p33.h

    *   Description     :

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2018-06-04 15:47

    *   Copyright:(c)JIELI  2011-2017  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef _P33_HW_H_
#define _P33_HW_H_

#include "typedef.h"


extern void p33_lock(void);
extern void p33_unlock(void);
#define p33_cs_h(x)        do{p33_lock();if(x&BIT(15)){JL_P33->SPI_CON |= BIT(0)|BIT(8);}else{JL_P33->SPI_CON  &= ~BIT(8);JL_P33->SPI_CON  |= BIT(0);}}while(0)
#define p33_cs_l           do{JL_P33->SPI_CON  &= ~(BIT(0)|BIT(8));p33_unlock();}while(0)

#define P33_OR              0b001
#define P33_AND             0b010
#define P33_XOR             0b011



//===============================================================================//
//
//
//
//===============================================================================//
//............. 0x0000 - 0x000f............ for analog control
#define P3_ANA_CON0                       0x00
#define P3_ANA_CON1                       0x01
#define P3_ANA_CON2                       0x02
#define P3_ANA_CON3                       0x03
#define P3_ANA_CON4                       0x04
#define P3_ANA_CON5                       0x05
#define P3_ANA_CON6                       0x06
#define P3_ANA_CON7                       0x07
#define P3_ANA_CON8                       0x08
#define P3_ANA_CON9                       0x09

#define P3_DRPG_CON0                      0x0c
#define P3_DRPG_CON1                      0x0d
#define P3_SDPG_CON                       0x0e
#define P3_FSPG_CON                       0x0f

//............. 0x0010 - 0x001f............ for analog others
#define P3_VLVD_CON                       0x11
#define P3_RST_SRC                        0x12
#define P3_LRC_CON0                       0x13
#define P3_LRC_CON1                       0x14
#define P3_RST_CON0                       0x15
#define P3_ANA_KEEP                       0x16
#define P3_VLD_KEEP                       0x17
#define P3_CHG_WKUP                       0x18
#define P3_CHG_READ                       0x19
#define P3_CHG_CON0                       0x1a
#define P3_CHG_CON1                       0x1b
#define P3_CHG_CON2                       0x1c
#define P3_CHG_CON3                       0x1d

//............. 0x0020 - 0x002f............ for PWM LED
#define P3_PWM_CON0                       0x20
#define P3_PWM_CON1                       0x21
#define P3_PWM_CON2                       0x22
#define P3_PWM_CON3                       0x23
#define P3_PWM_BRI_PRDL                   0x24
#define P3_PWM_BRI_PRDH                   0x25
#define P3_PWM_BRI_DUTY0L                 0x26
#define P3_PWM_BRI_DUTY0H                 0x27
#define P3_PWM_BRI_DUTY1L                 0x28
#define P3_PWM_BRI_DUTY1H                 0x29
#define P3_PWM_PRD_DIVL                   0x2a
#define P3_PWM_DUTY0                      0x2b
#define P3_PWM_DUTY1                      0x2c
#define P3_PWM_DUTY2                      0x2d
#define P3_PWM_DUTY3                      0x2e
#define P3_PWM_CNT_RD                     0x2f

//............. 0x0030 - 0x003f............ for PMU manager
#define P3_PMU_CON0                       0x30
#define P3_PMU_CON1                       0x31
#define P3_PMU_CON2                       0x32
#define P3_PMU_CON3                       0x33
#define P3_PMU_CON4                       0x34
#define P3_PMU_CON5                       0x35
#define P3_LP_PRP0                        0x36
#define P3_LP_PRP1                        0x37
#define P3_LP_STB0_STB1                   0x38
#define P3_LP_STB2_STB3                   0x39
#define P3_LP_STB4_STB5                   0x3a
#define P3_LP_STB6                        0x3b

//............. 0x0040 - 0x005f............ for PMU timer
#define P3_LP_RSC00                       0x40
#define P3_LP_RSC01                       0x41
#define P3_LP_RSC02                       0x42
#define P3_LP_RSC03                       0x43
#define P3_LP_PRD00                       0x44
#define P3_LP_PRD01                       0x45
#define P3_LP_PRD02                       0x46
#define P3_LP_PRD03                       0x47
#define P3_LP_RSC10                       0x48
#define P3_LP_RSC11                       0x49
#define P3_LP_RSC12                       0x4a
#define P3_LP_RSC13                       0x4b
#define P3_LP_PRD10                       0x4c
#define P3_LP_PRD11                       0x4d
#define P3_LP_PRD12                       0x4e
#define P3_LP_PRD13                       0x4f
//
#define P3_LP_TMR0_CLK                    0x54

#define P3_LP_TMR0_CON                    0x58
#define P3_LP_TMR1_CON                    0x59

//............. 0x0060 - 0x006f............ for PMU others
#define P3_LP_CNTRD0                      0x60
#define P3_LP_CNTRD1                      0x61
#define P3_LP_CNT0                        0x62
#define P3_LP_CNT1                        0x63
#define P3_LP_CNT2                        0x64
#define P3_LP_CNT3                        0x65

//............. 0x0070 - 0x007f............ for PMU others
#define P3_MSTM_RD                        0x70
#define P3_IVS_RD                         0x71
#define P3_IVS_SET                        0x72
#define P3_IVS_CLR                        0x73
#define P3_WLDO12_AUTO                    0x74
#define P3_WLDO06_AUTO                    0x75

//............. 0x0080 - 0x008f............ for reserved
#define P3_WDT_CON                        0x80

//............. 0x0090 - 0x009f............ for PORT and clock reset power control
#define P3_WKUP_EN                        0x90
#define P3_WKUP_EDGE                      0x91
#define P3_WKUP_CPND                      0x92
#define P3_WKUP_PND                       0x93
#define P3_PINR_CON                       0x94
#define P3_WKUP_FLEN                      0x95
#define P3_PCNT_CON                       0x96
#define P3_PCNT_VLUE                      0x97
#define P3_PR_IN                          0x98
#define P3_PR_OUT                         0x99
#define P3_PR_DIR                         0x9a
#define P3_PR_DIE                         0x9b
#define P3_PR_PU                          0x9c
#define P3_PR_PD                          0x9d
#define P3_PR_HD                          0x9e

//............. 0x00a0 - 0x00af............
#define P3_PR_PWR                         0xa0
#define P3_LDO5V_CON                      0xa1

#define P3_WKUP_SRC                       0xa8

#define P3_PORT_FLT                       0xaa

#define P3_P33_LAT                        0xad
#define P3_OTH_LAT                        0xae
#define P3_USB_LAT                        0xaf

//............. 0x00b0 - 0x00bf............ for EFUSE
#define P3_EFUSE_CON0                     0xb0
#define P3_EFUSE_CON1                     0xb1
#define P3_EFUSE_RDAT                     0xb2

//............. 0x00c0 - 0x00cf............ for rtc
#define P3_PORT_SEL10                     0xc0
#define P3_PORT_SEL11                     0xc1
#define P3_PORT_SEL12                     0xc2
#define P3_PORT_SEL13                     0xc3
#define P3_PORT_SEL14                     0xc4
#define P3_PORT_SEL15                     0xc5
#define P3_PORT_SEL16                     0xc6
#define P3_PORT_SEL17                     0xc7


//............. 0x00d0 - 0x00df............ for port latch
#define P3_PORTA_LAT0                     0xd0
#define P3_PORTB_LAT0                     0xd1
#define P3_PORTC_LAT0                     0xd2
#define P3_PORTD_LAT0                     0xd3
#define P3_PORTE_LAT0                     0xd4
#define P3_PORTF_LAT0                     0xd5
#define P3_PORTG_LAT0                     0xd6
#define P3_PORTH_LAT0                     0xd7
#define P3_PORTA_LAT1                     0xd8
#define P3_PORTB_LAT1                     0xd9
#define P3_PORTC_LAT1                     0xda
#define P3_PORTD_LAT1                     0xdb
#define P3_PORTE_LAT1                     0xdc
#define P3_PORTF_LAT1                     0xdd
#define P3_PORTG_LAT1                     0xde
#define P3_PORTH_LAT1                     0xdf

//............. others ............
#define P3_SOFT_FLAG                      P3_PMU_CON2


//Soft flag1
//[0]   : 0-open wdt
//      : 1-none
//
//[1]   : 0-usb None
//      : 1-usb dp close pd
//
//[2]   : 0-None
//      : 1-usb dp open pu
//
//[3]   : 0-usb None
//      : 1-usb dm close pd
//
//[4]   : 0-None
//      : 1-usb dm open pu

//[5]   : 0-None
//      : 1-enter power off flow
//
//[6]   : 0-None
//      : 1- btosc fast boot en
//
#define     SOFT_FLAG_WDT_NONE            BIT(0)
#define     SOFT_FLAG_USB_DP_PD_DISABLE   BIT(1)
#define     SOFT_FLAG_USB_DP_PU_ENABLE    BIT(2)
#define     SOFT_FLAG_USB_DM_PD_DISABLE   BIT(3)
#define     SOFT_FLAG_USB_DM_PU_ENABLE    BIT(4)
#define     SOFT_FLAG_POWEROFF_ENABLE     BIT(5)
#define     SOFT_FLAG_BTOSC_FT_ENABLE     BIT(6)

#define P3_SOFT_FLAG1                     P3_PMU_CON3


//===============================================================================//
//
//      p33 rtcvdd
//
//===============================================================================//
#define RTC_SFR_BEGIN                     0x8000



//............. 0x0080 - 0x008f............ for RTC
#define R3_ALM_CON                        (0x80 | RTC_SFR_BEGIN)

#define R3_RTC_CON0                       (0x84 | RTC_SFR_BEGIN)
#define R3_RTC_CON1                       (0x85 | RTC_SFR_BEGIN)
#define R3_RTC_DAT0                       (0x86 | RTC_SFR_BEGIN)
#define R3_RTC_DAT1                       (0x87 | RTC_SFR_BEGIN)
#define R3_RTC_DAT2                       (0x88 | RTC_SFR_BEGIN)
#define R3_RTC_DAT3                       (0x89 | RTC_SFR_BEGIN)
#define R3_RTC_DAT4                       (0x8a | RTC_SFR_BEGIN)

//............. 0x0090 - 0x009f............ for PORT control
#define R3_WKUP_EN                        (0x90 | RTC_SFR_BEGIN)
#define R3_WKUP_EDGE                      (0x91 | RTC_SFR_BEGIN)
#define R3_WKUP_CPND                      (0x92 | RTC_SFR_BEGIN)
#define R3_WKUP_PND                       (0x93 | RTC_SFR_BEGIN)
#define R3_WKUP_FLEN                      (0x94 | RTC_SFR_BEGIN)
#define R3_PORT_FLT                       (0x95 | RTC_SFR_BEGIN)

#define R3_PR_IN                          (0x98 | RTC_SFR_BEGIN)
#define R3_PR_OUT                         (0x99 | RTC_SFR_BEGIN)
#define R3_PR_DIR                         (0x9a | RTC_SFR_BEGIN)
#define R3_PR_DIE                         (0x9b | RTC_SFR_BEGIN)
#define R3_PR_PU                          (0x9c | RTC_SFR_BEGIN)
#define R3_PR_PD                          (0x9d | RTC_SFR_BEGIN)
#define R3_PR_HD                          (0x9e | RTC_SFR_BEGIN)

//............. 0x00a0 - 0x00af............ for system
#define R3_TIME_CON                       (0xa0 | RTC_SFR_BEGIN)
#define R3_TIME_CPND                      (0xa1 | RTC_SFR_BEGIN)
#define R3_TIME_PND                       (0xa2 | RTC_SFR_BEGIN)

#define R3_ADC_CON                        (0xa4 | RTC_SFR_BEGIN)
#define R3_OSL_CON                        (0xa5 | RTC_SFR_BEGIN)

#define R3_WKUP_SRC                       (0xa8 | RTC_SFR_BEGIN)
#define R3_RST_SRC                        (0xa9 | RTC_SFR_BEGIN)

#define R3_RST_CON                        (0xab | RTC_SFR_BEGIN)
#define R3_CLK_CON                        (0xac | RTC_SFR_BEGIN)


//others
#define P3_SOFT_FLAT                      P3_PMU_CON2
//Soft flag
//[0]   : 0-None
//      : 1-BT_OSC init ensable
//
//[1]   : 0-None
//      : 1-RTC_OSC init ensable
//
//[2:3] : 0-none
//      : 1-boot SPI PORTD_A
//      : 2-boot SPI PORTD_B
//      : 3-boot SPI PORTD_A & B
//
//[4]   : 0-none
//      : 1-switch to OSC_CLK
//
//

enum {
    SOFT_FLAG_BT_OSC_ENABLE     = BIT(0),
    SOFT_FLAG_RTC_OSC_ENABLE    = BIT(1),
    SOFT_FLAG_BOOT_SPI_PORTD_A  = BIT(2),
    SOFT_FLAG_BOOT_SPI_PORTD_B  = BIT(3),
    SOFT_FLAG_BOOT_SPI_ALL      = BIT(2) | BIT(3),
    SOFT_FLAG_SWITCH_OSC_CLK    = BIT(4),
    SOFT_FLAG_BOOT_OTP          = BIT(5),
};

#define P3_SOFT_FLAT1                     P3_PMU_CON3
//Soft flag1
//[0]   : 0-open wdt
//      : 1-none
//
//[1]   : 0-usb None
//      : 1-usb dp close pd
//
//[2]   : 0-None
//      : 1-usb dp open pu
//
//[3]   : 0-usb None
//      : 1-usb dm close pd
//
//[4]   : 0-None
//      : 1-usb dm open pu

//[5]   : 0-None
//      : 1-enter power off flow
//
//[6]   : 0-None
//      : 1- btosc fast boot en
//
#define     SOFT_FLAG_WDT_NONE            BIT(0)
#define     SOFT_FLAG_USB_DP_PD_DISABLE   BIT(1)
#define     SOFT_FLAG_USB_DP_PU_ENABLE    BIT(2)
#define     SOFT_FLAG_USB_DM_PD_DISABLE   BIT(3)
#define     SOFT_FLAG_USB_DM_PU_ENABLE    BIT(4)
#define     SOFT_FLAG_POWEROFF_ENABLE     BIT(5)
#define     SOFT_FLAG_BTOSC_FT_ENABLE     BIT(6)

//ROM
u8 p33_buf(u8 buf);

void p33_xor_1byte(u16 addr, u8 data0);

void p33_and_1byte(u16 addr, u8 data0);

void p33_or_1byte(u16 addr, u8 data0);

void p33_tx_1byte(u16 addr, u8 data0);

u8 p33_rx_1byte(u16 addr);

void p33_io_latch_init(void);

void P33_CON_SET(u16 addr, u8 start, u8 len, u8 data);

void SET_WVDD_LEV(u8 lev);

void RESET_MASK_SW(u8 sw);

void close_32K(u8 keep_osci_flag);

void P33_SYSTEM_RESET(void);
void P33_WKUP_CPND(u8 index);
void P33_WKUP_EN(u8 index);
void P33_WKUP_FLEN(u8 index);
void P33_WKUP_EDGE(u8 index, u8 edge);

u8 P33_CON_GET(u16 addr);
void P33_TX_NBIT(u16 addr, u8 data0, u8 en);
void P33_CON_DEBUG(void);
/*
 *-------------------P3_ANA_CON0
 */
#define VDD13TO12_SYS_EN(en)    P33_TX_NBIT(P3_ANA_CON0, BIT(0), en)

#define LDO13_EN(en)            P33_TX_NBIT(P3_ANA_CON0, BIT(2), en)

#define DCDC13_EN(en)           P33_TX_NBIT(P3_ANA_CON0, BIT(3), en)

#define PW_GATE_EN(en)          P33_TX_NBIT(P3_ANA_CON0, BIT(5), en)

#define RC_250k_EN(en)          P33_TX_NBIT(P3_ANA_CON0, BIT(7), en)
/*******************************************************************/

/*
 *-------------------P3_ANA_CON1
 */
#define BTDCDC_V17_PDEN(en)     P33_TX_NBIT(P3_ANA_CON1, BIT(0), en)

#define BTDCDC_RAMP_EN(en)      P33_TX_NBIT(P3_ANA_CON1, BIT(1), en)

#define BTDCDC_PFM_MODE(en)     P33_TX_NBIT(P3_ANA_CON1, BIT(2), en)

#define WLDO06_EN(en)           P33_TX_NBIT(P3_ANA_CON1, BIT(3), en)

#define VDC13_LOAD_EN(en)       P33_TX_NBIT(P3_ANA_CON1, BIT(4), en)

#define SYSVDD_RES_SHORT_EN(en) P33_TX_NBIT(P3_ANA_CON1, BIT(6), en)

#define SYSVDD_BYPASS_EN(en)    P33_TX_NBIT(P3_ANA_CON1, BIT(7), en)
/*******************************************************************/

/*
 *-------------------P3_ANA_CON2
 */
#define VCM_DET_EN(en)          P33_TX_NBIT(P3_ANA_CON2, BIT(3), en)

#define D2SH_EN_SW(en)          P33_TX_NBIT(P3_ANA_CON2, BIT(4), en)

#define VDDOK_DIS(dis)          P33_TX_NBIT(P3_ANA_CON2, BIT(6), dis)

#define SWVLD_AUTO_EN(en)       P33_TX_NBIT(P3_ANA_CON2, BIT(7), en)
/*******************************************************************/

/*
 *-------------------P3_ANA_CON3
 */
#define FORCE_BYPASS_EN(en)     P33_TX_NBIT(P3_ANA_CON3, BIT(2), en)

#define IVBATL_EN(en)           P33_TX_NBIT(P3_ANA_CON3, BIT(1), en)
/*******************************************************************/

/*
 *-------------------P3_ANA_CON4
 */
#define PMU_DET_EN(en)          P33_TX_NBIT(P3_ANA_CON4, BIT(0), en)

#define ADC_CHANNEL_SEL(ch)     P33_CON_SET(P3_ANA_CON4, 1, 3, ch)
//Macro for ADC_CHANNEL_SEL
enum {
    ADC_CHANNEL_SEL_VBG = 0,
    ADC_CHANNEL_SEL_VDC14,
    ADC_CHANNEL_SEL_SYSVDD,
    ADC_CHANNEL_SEL_VTEMP,
    ADC_CHANNEL_SEL_PROGF,
    ADC_CHANNEL_SEL_VBAT1_4,
};
/*******************************************************************/


/*
 *-------------------P3_ANA_CON5
 */
#define VDDIOM_VOL_SEL(lev)     P33_CON_SET(P3_ANA_CON5, 0, 3, lev)

#define GET_VDDIOM_VOL()        (P33_CON_GET(P3_ANA_CON5) & 0xf)

#define VDDIOW_VOL_SEL(lev)     P33_CON_SET(P3_ANA_CON5, 4, 2, lev)

#define GET_VDDIOW_VOL()        (P33_CON_GET(P3_ANA_CON5)>>4 & 0x3)

#define VDDIO_HD_SEL(cur)       P33_CON_SET(P3_ANA_CON5, 6, 2, cur)

/*******************************************************************/

/*
 *-------------------P3_ANA_CON6
 */
#define VDC14_VOL_SEL(sel)      P33_CON_SET(P3_ANA_CON6, 0, 3, sel)
#define VDC14_VOL_GET()        (P33_CON_GET(P3_ANA_CON6) & 0x7)
//Macro for VDC14_VOL_SEL
enum {
    VDC14_VOL_SEL_120V = 0,
    VDC14_VOL_SEL_125V,
    VDC14_VOL_SEL_130V,
    VDC14_VOL_SEL_135V,
    VDC14_VOL_SEL_140V,
    VDC14_VOL_SEL_145V,
    VDC14_VOL_SEL_150V,
    VDC14_VOL_SEL_155V,
};

#define GET_VD14_HD_SEL()       (P33_CON_GET(P3_ANA_CON6) & 0x7)

#define VD14_HD_SEL(sel)        P33_CON_SET(P3_ANA_CON6, 3, 2, sel)

/*******************************************************************/

/*
 *-------------------P3_ANA_CON7
 */
#define BTDCDC_BIAS_HD_SEL(sel) P33_CON_SET(P3_ANA_CON7, 0, 2, sel)
//Macro for BTDCDC_BIAS_HD_SEL
enum {
    BTDCDC_BIAS_HD_SEL_1uA = 0,
    BTDCDC_BIAS_HD_SEL_1P5uA,
    BTDCDC_BIAS_HD_SEL_2uA,
    BTDCDC_BIAS_HD_SEL_2P5uA,
};

#define BTDCDC_DUTY_SEL(sel)    P33_CON_SET(P3_ANA_CON7, 2, 2, sel)

#define BTDCDC_OSC_SEL(sel)     P33_CON_SET(P3_ANA_CON7, 4, 2, sel)
//Macro for BTDCDC_OSC_SEL
enum {
    BTDCDC_OSC_SEL0537MHz = 0,
    BTDCDC_OSC_SEL0789MHz,
    BTDCDC_OSC_SEL1030MHz,
    BTDCDC_OSC_SEL1270MHz,
    BTDCDC_OSC_SEL1720MHz,
    BTDCDC_OSC_SEL1940MHz,
    BTDCDC_OSC_SEL2150MHz,
    BTDCDC_OSC_SEL2360MHz,
};
/*******************************************************************/

/*
 *-------------------P3_ANA_CON9
 */
#define SYSVDD_VOL_SEL(sel)     P33_CON_SET(P3_ANA_CON9, 0, 4, sel)
//Macro for SYSVDD_VOL_SEL
enum {
    SYSVDD_VOL_SEL_087V = 0,
    SYSVDD_VOL_SEL_090V,
    SYSVDD_VOL_SEL_093V,
    SYSVDD_VOL_SEL_096V,
    SYSVDD_VOL_SEL_099V,
    SYSVDD_VOL_SEL_102V,
    SYSVDD_VOL_SEL_105V,
    SYSVDD_VOL_SEL_108V,
    SYSVDD_VOL_SEL_111V,
    SYSVDD_VOL_SEL_114V,
    SYSVDD_VOL_SEL_117V,
    SYSVDD_VOL_SEL_120V,
    SYSVDD_VOL_SEL_123V,
    SYSVDD_VOL_SEL_126V,
    SYSVDD_VOL_SEL_129V,
    SYSVDD_VOL_SEL_132V,
};

#define GET_SYSVDD_VOL_SEL()     (P33_CON_GET(P3_ANA_CON9) & 0xf)

#define SYSVDD_VOL_HD_SEL(sel)  P33_CON_SET(P3_ANA_CON9, 4, 2, sel)
/*******************************************************************/


/*
 *-------------------P3_VLVD_CON
 */
#define P33_VLVD_EN(en)         P33_TX_NBIT(P3_VLVD_CON, BIT(0), en)

#define P33_VLVD_PS(en)         P33_TX_NBIT(P3_VLVD_CON, BIT(1), en)

#define P33_VLVD_OE(en)         P33_TX_NBIT(P3_VLVD_CON, BIT(2), en)

#define VLVD_SEL(lev)           P33_CON_SET(P3_VLVD_CON, 3, 3, lev)
//Macro for VLVD_SEL
enum {
    VLVD_SEL_19V = 0,
    VLVD_SEL_20V,
    VLVD_SEL_21V,
    VLVD_SEL_22V,
    VLVD_SEL_23V,
    VLVD_SEL_24V,
    VLVD_SEL_25V,
    VLVD_SEL_26V,
};

#define VLVD_PND_CLR(clr)       P33_TX_NBIT(P3_VLVD_CON, BIT(6), clr)

#define VLVD_PND(pend)          P33_TX_NBIT(P3_VLVD_CON, BIT(7), pend)
/*******************************************************************/

/*
 *-------------------P3_RST_CON0
 */
#define VLVD_RST_EN(en)         P33_TX_NBIT(P3_RST_SRC, BIT(2), en)

#define VLVD_WKUP_EN(en)        P33_TX_NBIT(P3_RST_SRC, BIT(3), en)
/*******************************************************************/

/*
 *-------------------P3_LRC_CON0
 */
#define RC32K_EN(en)            P33_TX_NBIT(P3_LRC_CON0, BIT(0), en)

#define RC32K_RN_TRIM(en)       P33_TX_NBIT(P3_LRC_CON0, BIT(1), en)

#define RC32K_RPPS_SEL(sel)     P33_CON_SET(P3_LRC_CON0, 4, 2, sel)

#define RC32K_RNPS_SEL(sel)     P33_TX_NBIT(P3_LRC_CON0, BIT(7), (sel & BIT(0))); \
                                P33_CON_SET(P3_LRC_CON1, 0, 2, (sel>>1))
/*******************************************************************/

/*
 *-------------------P3_LRC_CON1
 */
//#define RC32K_PNPS_SEL(sel)     P33_CON_SET(P3_LRC_CON1, 0, 2, sel)

#define RC32K_CAP_SEL(sel)      P33_CON_SET(P3_LRC_CON1, 2, 3, sel)

#define RC32K_LDO_SEL(sel)      P33_CON_SET(P3_LRC_CON1, 5, 2, sel)
/*******************************************************************/

/*
 *-------------------P3_VLD_KEEP
 */
#define CLOCK_KEEP(a)           P33_TX_NBIT(P3_VLD_KEEP, BIT(0), a)

#define WKUP_KEEP(a)            P33_TX_NBIT(P3_VLD_KEEP, BIT(1), a)

#define FLASH_KEEP(a)           P33_TX_NBIT(P3_VLD_KEEP, BIT(2), a)

#define PWM_LED_KEEP(a)         P33_TX_NBIT(P3_VLD_KEEP, BIT(3), a)

#define SYS_RST_KEEP(a)         P33_TX_NBIT(P3_VLD_KEEP, BIT(4), a)

#define SYS_VDD_KEEP(a)         P33_TX_NBIT(P3_VLD_KEEP, BIT(5), a)

#define WDT_EXPT_EN(a)          P33_TX_NBIT(P3_VLD_KEEP, BIT(6), a)
/*******************************************************************/

/*
 *-------------------P3_CLK_CON0
 */
#define PWM_LED_SEL(a)          P33_CON_SET(P3_CLK_CON0, 0, 2, a)

#define SYSPLL_DPLL_REF(a)      P33_TX_NBIT(P3_VLVD_KEEP, BIT(2), a)

/*******************************************************************/


/*
 *-------------------P3_CHG_WKUP
 */
#define CHARGE_LEVEL_DETECT_EN(a)	P33_CON_SET(P3_CHG_WKUP, 0, 1, a)

#define CHARGE_EDGE_DETECT_EN(a)	P33_CON_SET(P3_CHG_WKUP, 1, 1, a)

#define CHARGE_WKUP_SOURCE_SEL(a)	P33_CON_SET(P3_CHG_WKUP, 2, 2, a)

#define CHARGE_WKUP_EN(a)			P33_CON_SET(P3_CHG_WKUP, 4, 1, a)

#define CHARGE_WKUP_EDGE_SEL(a)		P33_CON_SET(P3_CHG_WKUP, 5, 1, a)

#define CHARGE_WKUP_PND_CLR()		P33_CON_SET(P3_CHG_WKUP, 6, 1, 1)

/*
 *-------------------P3_CHG_READ
 */
#define CHARGE_FULL_FLAG_GET()		((P33_CON_GET(P3_CHG_READ) & BIT(0)) ? 1: 0 )

/*
 *-------------------P3_CHG_CON0
 */
#define CHARGE_EN(en)           P33_CON_SET(P3_CHG_CON0, 0, 1, en)

#define CHGBG_EN(en)            P33_CON_SET(P3_CHG_CON0, 1, 1, en)

#define IS_CHARGE_EN()			((P33_CON_GET(P3_CHG_CON0) & BIT(0)) ? 1: 0 )

/*
 *-------------------P3_CHG_CON1
 */
#define CHARGE_FULL_V_SEL(a)	P33_CON_SET(P3_CHG_CON1, 0, 4, a)

#define CHARGE_mA_SEL(a)		P33_CON_SET(P3_CHG_CON1, 4, 4, a)

/*
 *-------------------P3_CHG_CON2
 */
#define CHARGE_FULL_mA_SEL(a)	P33_CON_SET(P3_CHG_CON2, 4, 3, a)

/*
 *-------------------P3_LDO5V_CON
 */
#define LDO5V_EN(a)				P33_CON_SET(P3_LDO5V_CON, 0, 1, a)

#define LDO5V_EDGE_SEL(a)		P33_CON_SET(P3_LDO5V_CON, 1, 1, a)

#define LDO5V_LEVEL_WKUP_EN(a)	P33_CON_SET(P3_LDO5V_CON, 2, 1, a)

#define LDO5V_EDGE_WKUP_EN(a)	P33_CON_SET(P3_LDO5V_CON, 3, 1, a)

#define LDO5V_DET_GET()			((P33_CON_GET(P3_LDO5V_CON) & BIT(5)) ? 1: 0 )

#define LDO5V_PND_CLR()			P33_CON_SET(P3_LDO5V_CON, 6, 1, 1)

/*
 *-------------------P3_LVCMP_CON
 */
#define LVCMP_EN(a)				P33_CON_SET(P3_LVCMP_CON, 0, 1, a)

#define LVCMP_EDGE_SEL(a)		P33_CON_SET(P3_LVCMP_CON, 1, 1, a)

#define LVCMP_LEVEL_WKUP_EN(a)	P33_CON_SET(P3_LVCMP_CON, 2, 1, a)

#define LVCMP_EDGE_WKUP_EN(a)	P33_CON_SET(P3_LVCMP_CON, 3, 1, a)

#define LVCMP_CMP_SEL(a)		P33_CON_SET(P3_LVCMP_CON, 4, 1, a)

#define LVCMP_DET_GET()			((P33_CON_GET(P3_LVCMP_CON) & BIT(5)) ? 1: 0 )

#define LVCMP_PND_CLR()			P33_CON_SET(P3_LVCMP_CON, 6, 1, 1)

/*
 *-------------------P3_L5DEM_CON
 */
#define L5DEM_EN(a)				P33_CON_SET(P3_L5DEM_CON, 0, 1, a)

#define L5DEM_EDGE_SEL(a)		P33_CON_SET(P3_L5DEM_CON, 1, 1, a)

#define L5DEM_LEVEL_WKUP_EN(a)	P33_CON_SET(P3_L5DEM_CON, 2, 1, a)

#define L5DEM_EDGE_WKUP_EN(a)	P33_CON_SET(P3_L5DEM_CON, 3, 1, a)

#define L5DEM_DET_GET()			((P33_CON_GET(P3_L5DEM_CON) & BIT(5)) ? 1: 0 )

#define L5DEM_PND_CLR()			P33_CON_SET(P3_L5DEM_CON, 6, 1, 1)

/*
 *-------------------P3_L5DEM_FLT
 */
#define L5DEM_FIT_SEL(a)		P33_CON_SET(P3_L5DEM_FLT, 0, 3, a)

/*
 *-------------------P3_WKUP_SUB
 */
#define IS_LVCMP_WKUP()			((P33_CON_GET(P3_WKUP_SUB) & BIT(0)) ? 1: 0 )

#define IS_LDO5V_WKUP()			((P33_CON_GET(P3_WKUP_SUB) & BIT(1)) ? 1: 0 )

#define IS_L5DEM_WKUP()			((P33_CON_GET(P3_WKUP_SUB) & BIT(2)) ? 1: 0 )

/*
 *-------------------P3_L5V_CON0
 */
#define L5V_LOAD_EN(a)				P33_CON_SET(P3_L5V_CON0, 0, 1, a)

#define L5V_RES_DET_EN(a)			P33_CON_SET(P3_L5V_CON0, 1, 1, a)

#define L5V_VM_EN(a)			P33_CON_SET(P3_L5V_CON0, 2, 1, a)

#define L5V_IM_EN(a)			P33_CON_SET(P3_L5V_CON0, 3, 1, a)

#define L5V_IM_S_SEL(a)			P33_CON_SET(P3_L5V_CON0, 4, 2, a)

/*
 *-------------------P3_L5V_CON1
 */
#define L5V_RES_DET_S_SEL(a)				P33_CON_SET(P3_L5V_CON1, 0, 2, a)

#define L5V_VM_S_SEL(a)			P33_CON_SET(P3_L5V_CON1, 4, 2, a)



/*******************************************************************/

//Macro for P3_WLDO06_AUTO
enum {
    WLDO_LEVEL_050V = 0,
    WLDO_LEVEL_054V,
    WLDO_LEVEL_058V,
    WLDO_LEVEL_062V,
    WLDO_LEVEL_066V,
    WLDO_LEVEL_070V,
    WLDO_LEVEL_085V,
    WLDO_LEVEL_133V,
};



#endif

