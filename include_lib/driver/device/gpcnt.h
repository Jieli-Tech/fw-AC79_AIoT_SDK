
#ifndef ADAPTER_GPCNT_H
#define ADAPTER_GPCNT_H
/*
 * 该模块使用说明
 * 选择一个时钟源进行计算另一个时钟源输入的脉冲数量
 * 使用方法如gpnt_test();
 */
#include "generic/typedef.h"

struct gpcnt_platform_data {
    u8 gpcnt_gpio;
    u8 gss_clk_source;//采样时钟源480M
    u8 css_clk_source;//
    u8 ch_source;
    u8 cycle;
};

enum {
    GPCNT_GSS_CSS_LSB_CLK = 0,
    GPCNT_GSS_CSS_OSC_CLK,
    GPCNT_GSS_CSS_INPUT_CHANNEL2,//测试异常
    GPCNT_GSS_CSS_INPUT_CHANNEL4,
    GPCNT_GSS_CSS_CLK,//时钟系统输入
    GPCNT_GSS_CSS_RING_OSC,
    GPCNT_GSS_CSS_PLL480M,
    GPCNT_GSS_CSS_INPUT_CHANNEL1,
};

typedef enum { //采样时钟源
    GPCNT_LCB_CLK,//
    GPCNT_OSC_CLK,//外部晶振时钟
    GPCNT_SRC_CLK,//
    GPCNT_HCO_CLK,//
    GPCNT_HSB_CLK,//
    GPCNT_AUD_CLK,//音频时钟
    GPCNT_WL_CLK,//wifi时钟
    GPCNT_USB_CLK,//USB时钟
    GPCNT_PLL_CLK,//锁相环时钟
} CLK_GPCNT;

typedef enum { //gpio输出通道
    GPCNT_INPUT_CHANNEL1 = 1,
    GPCNT_INPUT_CHANNEL2,
    GPCNT_INPUT_CHANNEL4,
} CHANNEL_GPCNT;

typedef enum { //采样时钟的多少周期  周期 = 32*2^CYCLE_GPCNT
    CYCLE_1 = 1,
    CYCLE_2,
    CYCLE_3,
    CYCLE_4,
    CYCLE_5,
    CYCLE_6,
    CYCLE_7,
    CYCLE_8,
    CYCLE_9,
    CYCLE_10,
    CYCLE_11,
    CYCLE_12,
    CYCLE_13,
    CYCLE_14,
    CYCLE_15,
} CYCLE_GPCNT;

typedef enum {
    IOCTL_GET_GPCNT,
    IOCTL_SET_CYCLE,
    IOCTL_SET_GPIO,
    IOCTL_SET_GSS_CLK_SOURCE,//采样时钟
    IOCTL_SET_CSS_CLK_SOURCE,//需要计算的时钟
    IOCTL_SET_CH_SOURCE,
} IOCTL_GPCNT;

extern const struct device_operations gpcnt_dev_ops;
extern const struct gpcnt_platform_data gpcnt_data;

#define GPCNT_GSS(x)            SFR(JL_GPCNT->CON,  12, 3,  x)//选择主输入时钟源
#define GPCNT_CLR_PEND(x)       SFR(JL_GPCNT->CON,  6,  1,  x)//清楚中断标志
#define GPCNT_GTS(x)            SFR(JL_GPCNT->CON,  8,  4,  x)//主时钟周期选择
#define GPCNT_PND(x)            SFR(JL_GPCNT->CON,  7,  1,  x)//中断请求标志
#define GPCNT_CSS(x)            SFR(JL_GPCNT->CON,  1,  3,  x)//输入源时钟选择即想要量的时钟源
#define GPCNT_EN(x)             SFR(JL_GPCNT->CON,  0,  1,  x)//模块使能 0关 1开    u8 cycle;

#endif
