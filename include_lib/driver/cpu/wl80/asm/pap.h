#ifndef _PAP_H_
#define _PAP_H_

#include "typedef.h"
#include "asm/cpu.h"
#include "os/os_api.h"

#define PAP_DMA_MAX	(64 * 1024 - 4)

#define PAPDEN  14  //PAP接口数据信号引脚(PGx)使能
#define PAPREN  13  //PAP接口读信号引脚(PH4)使能
#define PAPWEN  12  //PAP接口写信号引脚(PH3)使能

#define PAP_PORT_EN()   JL_IOMAP->CON0 |= ((1L<<PAPDEN)|(1L<<PAPREN)|(1L<<PAPWEN))   //使能PAP接口
#define PAP_PORT_DIS()  JL_IOMAP->CON0 &=~((1L<<PAPDEN)|(1L<<PAPREN)|(1L<<PAPWEN))   //使能PAP接口
#define PAP_PORT_SEL()

#define PAP_EXT_EN()    JL_PAP->CON |= BIT(16)      //使能PAP扩展模式
#define PAP_EXT_DIS()   JL_PAP->CON &= ~BIT(16)     //禁止PAP扩展模式
#define PAP_EXT_M2L()   JL_PAP->CON |= BIT(17)  //扩展模式顺序MSB到LSB
#define PAP_EXT_L2M()   JL_PAP->CON &= ~BIT(17) //扩展模式顺序LSB到MSB
#define PAP_IE(x)       JL_PAP->CON = (JL_PAP->CON & ~BIT(18)) | ((x & 0x1)<<18)

#define PAP_USE_SEM         2

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~PAP参数配置~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
//datawidth:8bit/16bit模式配置
#define PAP_PORT_8BITS      0
#define PAP_PORT_16BITS     1

//endian:数据大小端配置
#define PAP_LE              0 //8bit时，数据在低8位IO输出；16bit时，低位数据在低端口IO输出
#define PAP_BE              1 //8bit时，数据在高8位IO输出；16bit时，低位数据在高端口IO输出

//cycle:1-2字节的循环发送和接收次数,3字节以上不进行多次发送
#define PAP_CYCLE_ONE       0  //ReadWrite one times
#define PAP_CYCLE_TWO       1  //ReadWrite two times

//pre:读极性
#define PAP_READ_LOW 		0 //读信号：空闲1电平，有效0电平
#define PAP_READ_HIGH 		1 //读信号：空闲0电平，有效1电平

//pwe:写极性
#define PAP_WRITE_LOW 		0 //写信号：空闲1电平，有效0电平
#define PAP_WRITE_HIGH 		1 //写信号：空闲0电平，有效1电平

//port_sel:pap IO端口选择
#define PAP_PORT_A			0 //PAP_D0-->PH0,PAP_D7-->PH8,PAP_D8-->PC1,PAP_D15-->PC8, PAP_WR-->PC9,PAP_RD-->PC10
#define PAP_PORT_B			1 //PAP_D0-->PH0,PAP_D7-->PH8,PAP_D8-->PE2,PAP_D15-->PE9, PAP_WR-->PE0,PAP_RD-->PC0

//timing_setup(ts) timing_hold(th) timing_width(tw):数据传输配置
/*
写时钟信号:
                    __________               _________
pwe=H: ____________|          |_____________|         |__________
pwe=L: ____________            _____________           __________
                   |__________|             |_________|
数据信号:
             ________________________                       ______
data:  _____|                        |_____________________|
		           |          |
ts,tw,th:   |      |          |      |
		    |<-ts->|<---tw--->|<-th->|
		    |      |          |      |
ts:0-3,ts个lsb系统时钟宽度
tw:0-15,0-->16个lsb系统时钟宽度，1-15：tw个lsb系统时钟宽度
th:0-3,th个lsb系统时钟宽度
*/
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

struct pap_info {
    u8 timing_setup: 2; //ts:数据建立时间(2bits)
    u8 timing_hold: 2;	 //th:数据保持时间(2bits)
    u8 timing_width: 4; //tw:读/写使能信号宽度  系统时钟 0:16个 1:1个 2:2个 依此类推(4bits)
    u8 datawidth: 1; //PAP_PORT_8BITS or PAP_PORT_16BITS
    u8 endian: 1; //PAP_LE or PAP_BE
    u8 cycle: 1; //PAP_CYCLE_ONE or PAP_CYCLE_TWO
    u8 port_sel: 1; //PAP_PORT_A or PAP_PORT_B
    u8 use_sem: 1;
    u8 port_status: 2;
    u8 rd_en: 1;//是否使用rd引脚读取数据
    u8 pre: 1;//读极性
    u8 pwe: 1;//写极性
    u8 dma_no_waite: 1;//发送DMA不用等待发送完成(下次发送再检查是否发送完成)
    OS_SEM sem;
    void (*interrupt_cb)(void *priv);
    void *priv;
};

extern const struct device_operations pap_dev_ops;

#endif
