#ifndef ASM_GPIO_H
#define ASM_GPIO_H


#include "asm/gpio.h"

///  \cond DO_NOT_DOCUMENT
#define GPIO2PORT(gpio)    (gpio / IO_GROUP_NUM)
/// \endcond

/**
 * @brief gpio_port_lock，自旋锁，即不能被另一个cpu打断，也不能被中断打断
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 */
void gpio_port_lock(unsigned int port);


/**
 * @brief gpio_port_unlock，解除自旋锁
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 */
void gpio_port_unlock(unsigned int port);


/**
 * @brief gpio_direction_input，将引脚直接设为输入模式
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return 0：成功  非0：失败
 */
///  \cond DO_NOT_DOCUMENT
int __gpio_direction_input(unsigned int gpio);
/// \endcond
int gpio_direction_input(unsigned int gpio);


/**
 * @brief gpio_direction_output，设置引脚的方向为输出，并设置一下电平
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，输出1,  0，输出0
 *
 * @return 0：成功  非0：失败
 */
///  \cond DO_NOT_DOCUMENT
int __gpio_direction_output(unsigned int gpio, int value);
/// \endcond
int gpio_direction_output(unsigned int gpio, int value);


/**
 * @brief gpio_set_pull_up，设置引脚的上拉，上拉电阻10K，当引脚为输入模式时才有效
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，上拉；0，不上拉
 *
 * @return 0：成功  非0：失败
 */
///  \cond DO_NOT_DOCUMENT
int __gpio_set_pull_up(unsigned int gpio, int value);
/// \endcond
int gpio_set_pull_up(unsigned int gpio, int value);


/**
 * @brief gpio_set_pull_down，设置引脚的下拉，下拉电阻10K，当引脚为输入模式时才有效
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，下拉；0，不下拉
 *
 * @return 0：成功  非0：失败
 */
///  \cond DO_NOT_DOCUMENT
int __gpio_set_pull_down(unsigned int gpio, int value);
/// \endcond
int gpio_set_pull_down(unsigned int gpio, int value);


/**
 * @brief gpio_set_hd, 设置引脚的内阻，当引脚为输出模式时才有效
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，关闭内阻增强输出  0，存在内阻，芯片默认的
 *
 * @return 0：成功  非0：失败
 */
///  \cond DO_NOT_DOCUMENT
int __gpio_set_hd(unsigned int gpio, int value);
/// \endcond
int gpio_set_hd(unsigned int gpio, int value);


/**
 * @brief gpio_set_hd1, 设置引脚的输出电流，当引脚为输出模式时才有效
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，电流变大增强输出； 0，默认电流
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_hd1(unsigned int gpio, int value);


/**
 * @brief gpio_set_die，设置引脚为数字功能还是模拟功能，比如引脚作为ADC的模拟输入，则die要置0
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，数字功能，即01信号；  0，跟电压先关的模拟功能
 *
 * @return 0：成功  非0：失败
 */
///  \cond DO_NOT_DOCUMENT
int __gpio_set_die(unsigned int gpio, int value);
/// \endcond
int gpio_set_die(unsigned int gpio, int value);

///  \cond DO_NOT_DOCUMENT
int __gpio_set_output_clk(unsigned int gpio, int clk);
/// \endcond


/**
 * @brief gpio_set_output_clk，将引脚设为时钟输出
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param clk 12,24分别是12M时钟和24M时钟，0是默认24M时钟
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_output_clk(unsigned int gpio, int clk);


/**
 * @brief gpio_clear_output_clk，清除引脚时钟输出
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param clk 12,24分别是12M时钟和24M时钟，0是默认24M时钟
 *
 * @return 0：成功  非0：失败
 */
int gpio_clear_output_clk(unsigned int gpio, int clk);


/**
 * @brief gpio_read，读取引脚的输入电平，引脚为输入模式时才有效
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return 电平值
 */
///  \cond DO_NOT_DOCUMENT
int __gpio_read(unsigned int gpio);
/// \endcond
int gpio_read(unsigned int gpio);

/**
 * @brief gpio_set_direction，设置引脚方向
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param dir 1，输入；0，输出
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_direction(unsigned int gpio, unsigned int dir);


/**
 * @brief gpio_latch_en，保持引脚电平，软硬件无法改变它，如需取消则再调用一次并把en设置为0
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param en 1，使能；0，取消使能
 */
void gpio_latch_en(unsigned int gpio, unsigned char en);

/**
 * @brief gpio_set_dieh, 设置引脚为数字功能还是模拟功能，但模拟功能时，跟die的不一样而已
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，数字功能，即01信号；  0，跟电压先关的模拟功能
 *
 * @return 0：成功  非0：失败
 */
int gpio_set_dieh(unsigned int gpio, int value);


/**
 * @brief gpio_set_output_channle, 将IO映射成特殊引脚
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param clk 参考枚举CHx_UTx_TX，如CH0_UT0_TX
 *
 * @return 0：成功  非0：失败
 */
unsigned int gpio_output_channle(unsigned int gpio, enum gpio_out_channle clk);


/**
 * @brief gpio_clear_output_channle，取消将IO映射成特殊引脚
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param clk 参考枚举CHx_UTx_TX，如CH0_UT0_TX
 *
 * @return 0：成功  非0：失败
 */
unsigned int gpio_clear_output_channle(unsigned int gpio, enum gpio_out_channle clk);

/**
 * @brief gpio_uart_rx_input，将引脚映射成RX脚
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param ut 0,1,2，分别代表串口0，1，2
 * @param ch 0,1,2,3，分别代表input_channel0，1，2，3
 *
 * @return 0：成功  非0：失败
 */
unsigned int gpio_uart_rx_input(unsigned int gpio, unsigned int ut, unsigned int ch);


/**
 * @brief gpio_close_uart0，关闭串口0
 *
 * @return 0：成功  非0：失败
 */
unsigned int gpio_close_uart0(void);


/**
 * @brief gpio_close_uart1，关闭串口1
 *
 * @return 0：成功  非0：失败
 */
unsigned int gpio_close_uart1(void);


/**
 * @brief gpio_close_uart2，关闭串口2
 *
 * @return 0：成功  非0：失败
 */
unsigned int gpio_close_uart2(void);


/**
 * @brief gpio_set_uart0
 *
 * @param ch 0:3 选择对应IO wl82
 *         |ch|tx|rx|
 *         |- |- |- |
 *         |0|PA5_TX|PA6_RX|
 *         |1|PB1_TX|PB2_RX|
 *         |2|PA3_TX|PA4_RX|
 *         |3|PH0_TX|PH1_RX|
 *         |-1|关闭对应的IO口串口功能|no|
 *
 * @return
 */
unsigned int gpio_set_uart0(unsigned int ch);


/**
 * @brief gpio_set_uart1
 *
 * @param ch 0:3 选择对应IO  wl82
 *         |ch|tx|rx|
 *         |- |- |- |
 *         |0|PH6_TX|PH7_RX|
 *         |1|PC3_TX|PC4_RX|
 *         |2|PB3_TX|PB4_RX|
 *         |3|USBDP |USBDM |
 *         |-1|关闭对应的IO口串口功能|no|
 *
 * @return
 */
unsigned int gpio_set_uart1(unsigned int ch);


/**
* @brief gpio_set_uart2
*
* @param ch 0:3 选择对应IO  wl82
*         |ch|tx|rx|
*         |- |- |- |
*         |0|PH2_TX|PH3_RX|
*         |1|PC9_TX|PC10_RX|
*         |2|PB6_TX|PB7_RX|
*         |3|PE0_TX|PE1_RX|
*         |-1|关闭对应的IO口串口功能|no|
*
* @return
*/
unsigned int gpio_set_uart2(unsigned int ch);


///  \cond DO_NOT_DOCUMENT
/**
 * @brief usb_iomode, usb引脚设为普通IO
 *
 * @param usb_id usb名称号，0是usb0，1是usb1
 * @param enable 1，使能；0，关闭
 */
void usb_iomode(unsigned char usb_id, unsigned int enable);

unsigned int usb_set_direction(unsigned int gpio, unsigned int value);
unsigned int usb_output(unsigned int gpio, unsigned int value);
unsigned int usb_set_pull_up(unsigned int gpio, unsigned int value);
unsigned int usb_set_pull_down(unsigned int gpio, unsigned int value);
unsigned int usb_set_dieh(unsigned int gpio, unsigned int value);
unsigned int usb_set_die(unsigned int gpio, unsigned int value);
unsigned int usb_read(unsigned int gpio);
/// \endcond
#endif
