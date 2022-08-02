#ifndef ASM_UART_H
#define ASM_UART_H


#define UART_NUM 			3
#define UART_OUTPORT_NUM 	4


#include "device/uart.h"
#include "device/device.h"

void put_bin(char *str, u32 bin);
#define putbin(x) 	put_bin(#x,x)
#define putline()   printf("-> %s %d \r\n",__func__,__LINE__)
#define putint(x)   printf("-> %s = 0x%x \r\n",#x,x)

/**************勿改***************/
#define PORT_BASE_VALUE	0x10000 //该值需要大于16 * 8

//	    PORT_TX_RX , UART0
#define PORTA_5_6		(PORT_BASE_VALUE + 0)
#define PORTB_1_2		(PORT_BASE_VALUE + 1)
#define PORTA_3_4		(PORT_BASE_VALUE + 2)
#define PORTH_0_1		(PORT_BASE_VALUE + 3)

//	    PORT_TX_RX , UART1
#define PORTH_6_7		(PORT_BASE_VALUE + 0)
#define PORTC_3_4		(PORT_BASE_VALUE + 1)
#define PORTUSB_B		(PORT_BASE_VALUE + 2)
#define PORTB_3_4		(PORT_BASE_VALUE + 2)
#define PORTUSB_A		(PORT_BASE_VALUE + 3)

//	    PORT_TX_RX , UART2
#define PORTH_2_3		(PORT_BASE_VALUE + 0)
#define PORTC_9_10		(PORT_BASE_VALUE + 1)
#define PORTB_6_7		(PORT_BASE_VALUE + 2)
#define PORTE_0_1		(PORT_BASE_VALUE + 3)

#define PORT_MIN_NUM	(PORT_BASE_VALUE + 0)
#define PORT_MAX_NUM	(PORT_BASE_VALUE + 4)
#define PORT_REMAP		(PORT_BASE_VALUE + 5)

#define OUTPUT_CHANNEL0	(PORT_BASE_VALUE + 0)
#define OUTPUT_CHANNEL1	(PORT_BASE_VALUE + 1)
#define OUTPUT_CHANNEL2	(PORT_BASE_VALUE + 2)
#define OUTPUT_CHANNEL3	(PORT_BASE_VALUE + 3)

#define INPUT_CHANNEL0	(PORT_BASE_VALUE + 0)
#define INPUT_CHANNEL1	(PORT_BASE_VALUE + 1)
#define INPUT_CHANNEL2	(PORT_BASE_VALUE + 2)
#define INPUT_CHANNEL3	(PORT_BASE_VALUE + 3)

#define PORT_GET_VALUE(port)	(port - PORT_BASE_VALUE)
#define CHANNEL_GET_VALUE(port)	(port - PORT_BASE_VALUE)
/***********************************/


#define UART0_PLATFORM_DATA_BEGIN(data) \
	static const struct uart_platform_data data = {


#define UART0_PLATFORM_DATA_END()  \
	.irq 					= IRQ_UART0_IDX, \
};


#define UART1_PLATFORM_DATA_BEGIN(data) \
	static const struct uart_platform_data data = {


#define UART1_PLATFORM_DATA_END()  \
	.irq 					= IRQ_UART1_IDX, \
};


#define UART2_PLATFORM_DATA_BEGIN(data) \
	static const struct uart_platform_data data = {

#define UART2_PLATFORM_DATA_END()  \
	.irq 					= IRQ_UART2_IDX, \
};


extern const struct device_operations uart_dev_ops;


extern int uart_init(const struct uart_platform_data *);

extern void uart_debug_suspend(void);

extern void uart_debug_resume(void);

#endif


