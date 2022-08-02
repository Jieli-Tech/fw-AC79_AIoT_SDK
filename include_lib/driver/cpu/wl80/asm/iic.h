/*******************************************************************************************
  File Name: iic.h

Version: 1.00

Discription:  IIC 驱动

Author:yulin deng

Email :flowingfeeze@163.com

Date:星期五, 四月 19 2013

Copyright:(c)JIELI  2011  @ , All Rights Reserved.
 *******************************************************************************************/
#ifndef ASM_IIC_H
#define ASM_IIC_H

#include "typedef.h"
#include "asm/cpu.h"


#define IIC_OUTPORT_NUM 	4

struct iic_outport {
    u8  clk_pin;
    u8  dat_pin;
    u32 value;
};

struct hardware_iic {
    u8 clk_pin;
    u8 dat_pin;
    u32 baudrate;
    u32 occupy_reg;
    u32 occupy_io_mask;
    u32 occupy_io_value;
    JL_IIC_TypeDef *reg;
    struct iic_outport outport_map[IIC_OUTPORT_NUM];
};

#define iic_pnd(iic)             	(iic->reg->CON0 & BIT(15))//读取中断标志
#define iic_clr_pnd(iic)         	iic->reg->CON0 |= BIT(14)//清空普通pending
#define iic_epnd(iic)               (iic->reg->CON0 & BIT(13))//读取结束位中断标记
#define iic_clr_epnd(iic)           iic->reg->CON0 |= BIT(12)//清空结束位pending
#define iic_ack_in(iic)             (iic->reg->CON0 & BIT(7))//读取ACK
#define iic_set_output(iic)         iic->reg->CON0 &=~BIT(3)//方向设置为输出
#define iic_set_input(iic)          iic->reg->CON0 |= BIT(3)//方向设置为输入
#define iic_cfg_done(iic)           iic->reg->CON0 |= BIT(2)//配置完成
#define iic_add_start_bit(iic)      iic->reg->CON0 |= BIT(5)//加起始位
#define iic_add_end_bit(iic)        iic->reg->CON0 |= BIT(4)//加结束位
#define iic_set_ack(iic)         	iic->reg->CON0 &=~BIT(6)//设置ACK位
#define iic_set_unack(iic)         	iic->reg->CON0 |= BIT(6)//设置ACK位


#define HW_IIC0_PLATFORM_DATA_BEGIN(data) \
	extern int hw_iic_ops_link(void);module_initcall(hw_iic_ops_link);\
	static struct iic_device _iic_device_##data;\
	static const struct hw_iic_platform_data data = { \
		.head = { \
			.type = IIC_TYPE_HW, \
			.p_iic_device = &_iic_device_##data,\
		}, \
		.iic = {



#define HW_IIC0_PLATFORM_DATA_END() \
			.occupy_reg = (u32)&JL_IOMAP->CON1, \
			.occupy_io_mask = ~(BIT(18)|BIT(19)), \
			.reg = JL_IIC, \
			.outport_map = { \
				{IO_PORT_USB_DPA, IO_PORT_USB_DMA, 0}, \
				{IO_PORTA_07, IO_PORTA_08, BIT(18)}, \
				{IO_PORTC_01, IO_PORTC_02, BIT(19)}, \
				{IO_PORTH_00, IO_PORTH_01, (BIT(18) | BIT(19))}, \
			}, \
		}, \
	};



#endif

