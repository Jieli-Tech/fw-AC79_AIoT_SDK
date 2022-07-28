#ifndef _EMI_H_
#define _EMI_H_

#include "device.h"
#include "generic/typedef.h"
#include "generic/list.h"
#include "device/gpio.h"
#include "generic/atomic.h"
#include "generic/ioctl.h"
#include "device/ioctl_cmds.h"
#include "os/os_api.h"


//bits_mode:8bit/16bit/32bit模式配置
#define EMI_8BITS_MODE		0
#define EMI_16BITS_MODE		1
#define EMI_32BITS_MODE		2


//emi_wr colection:写极性边沿
#define EMI_RISING_COLT		0 //上升沿
#define EMI_FALLING_COLT	1 //下降沿

//th:是否使用减半周期
#define EMI_TWIDTH_NO_HALF	0
#define EMI_TWIDTH_HALF		1

/*
timing_setup(ts) timing_half(th) timing_width(tw):数据传输配置
写时钟信号:
                    __________               _________
wr =0: ____________|          |_____________|         |__________
wr =1: ____________            _____________           __________
                   |__________|             |_________|
数据信号:
             ________________________                       ______
data:  _____|                        |_____________________|
		           |          |
ts,tw:      |      |          |      |
		    |<-ts->|<---tw--->|      |
		    |      |          |      |
			|                        |
			|<---hsb/(baudrate+1)--->|
			|                        |
			|                        |
			|                        |
ts:0-255,ts个hsb时钟宽度
tw:0-255,tw个hsb时钟宽度
th:0-1,0-->tw:wr不减少半个hsb时钟周期，1-->tw:wr减少半个hsb时钟周期
*/

#define EMI_MAGIC                        'E'
#define EMI_SET_ISR_CB                   _IOW(EMI_MAGIC,1,u32)
#define EMI_USE_SEND_SEM                 _IO(EMI_MAGIC,2)
#define EMI_SET_WRITE_BLOCK              _IOW(EMI_MAGIC,3,u32)
#define EMI_SET_WRITE_TIMER_OUT          _IOW(EMI_MAGIC,4,u32)

//baudrate:EMI波特率，hsb的分频
enum EMI_BAUD {
    EMI_BAUD_DIV2 = 1,
    EMI_BAUD_DIV3 = 2,
    EMI_BAUD_DIV4 = 3,
    EMI_BAUD_DIV5 = 4,
    EMI_BAUD_DIV6 = 5,
    EMI_BAUD_DIV7 = 6,
    EMI_BAUD_DIV8 = 7,
    EMI_BAUD_DIV10 = 9,
    EMI_BAUD_DIV16 = 15,
    EMI_BAUD_DIV32 = 31,
    EMI_BAUD_DIV64 = 63,
    EMI_BAUD_DIV128 = 127,
    EMI_BAUD_DIV256 = 255,
};

struct emi_platform_data {
    u8 baudrate;//emi_clk = hsb/(baudrate+1)
    u8 bits_mode: 4;
    u8 colection: 2;
    u8 th: 2; //timing_half:0-->timing_width不减少半个hsb系统时钟周期，1-->timing_width减少半个hsb系统时钟周期
    u8 ts;//timing_setup:ts个hsb系统时钟宽度
    u8 tw;//timing_width:tw个hsb系统时钟宽度
    u32 time_out;//emi发送超时时间，单位ms
    u32 data_bit_en;//32位(4字节)对应IO的输出使能(0禁止输出,1输出)
};

struct emi_device {
    char *name;
    struct device dev;
    void (*emi_isr_cb)(void *priv);
    OS_SEM send_sem;
    char use_send_sem;
    char write_block;
    volatile char wait_send_flg;
    u32 time_out;
    void *priv;
};

#define REGISTER_EMI_DEVICE(pdev) \
	static struct emi_device pdev = {\
		.name = "emi",\
	}

extern const struct device_operations emi_dev_ops;

#endif

