#ifndef DEVICE_UART_H
#define DEVICE_UART_H

#include "generic/typedef.h"
#include "device/device.h"
#include "generic/ioctl.h"
#include "system/task.h"

/// \cond DO_NOT_DOCUMENT
#define UART_DISABLE        0x00000000
#define UART_DMA_SUPPORT 	0x00000001
#define UART_TX_USE_DMA 	0x00000003
#define UART_RX_USE_DMA 	0x00000005
#define UART_DEBUG 			0x00000008

struct uart_outport {
    u8  tx_pin;
    u8  rx_pin;
    u16 value;
};
/// \endcond


/**
 * \name UART_CLK selest
 * \{
 */
enum uart_clk_src {
    LSB_CLK,			/*!< 低速时钟  */
    OSC_CLK,			/*!< 晶振时钟  */
    PLL_48M,			/*!< PLL48M时钟  */
};
/* \} name */

struct uart_platform_data {
    u8 *name;					/*!<  串口名称，在注册的时候已经配好*/

    u8  irq;       		        /*!<  中断号，在注册的时候已经对应设置好了*/
    u8  disable_tx_irq;			/*!<  1:不使用发送中断*/
    u8  disable_rx_irq;			/*!<  1:不使用中断接收*/
    u8  disable_ot_irq; 	 	/*!<  1:不使用超时中断*/
    u8  tx_pin_hd;        		/*!<  1:tx io开强驱*/

    int tx_pin;					/*!<  发送引脚,不配置需设置-1*/
    int rx_pin;					/*!<  接收引脚,不配置需设置-1*/
    int flags;					/*!<  串口用作打印*/
    u32 baudrate;				/*!<  波特率设置*/

    int port;					/*!<  enum _uart_port0-3的值*/
    int input_channel;			/*!<  输入通道*/
    int output_channel;			/*!<  输出通道*/
    u32 max_continue_recv_cnt;	/*!<  连续接收最大字节*/
    u32 idle_sys_clk_cnt;		/*!<  超时计数器，如果在指定的时间里没有收到任何数据，则产生超时中断*/
    enum uart_clk_src clk_src;	/*!<  选择时钟源*/
};

/**
 * \name UART error
 * \{
 */
enum {
    UART_CIRCULAR_BUFFER_WRITE_OVERLAY = -1, /*!<  循环buf写满*/
    UART_RECV_TIMEOUT = -2,				  	 /*!<  接收超时*/
    UART_RECV_EXIT = -3,				     /*!<  接收终止退出*/
};
/* \} name */

/**
 * \name UART dev_ioctl funciton selest
 * \{
 */
#define UART_MAGIC                          'U'
#define UART_FLUSH                          _IO(UART_MAGIC,1)					/*!<  串口重载*/
#define UART_SET_RECV_ALL                   _IOW(UART_MAGIC,2,bool)				/*!<  设置串口等待接收满才退出，需先调用下面的阻塞指令*/
#define UART_SET_RECV_BLOCK                 _IOW(UART_MAGIC,3,bool)				/*!<  设置串口接收阻塞*/
#define UART_SET_RECV_TIMEOUT               _IOW(UART_MAGIC,4,u32)				/*!<  设置串口接收超时*/
#define UART_SET_RECV_TIMEOUT_CB            _IOW(UART_MAGIC,5,int (*)(void))	/*!<  设置串口超时之后回调的函数*/
#define UART_GET_RECV_CNT                   _IOR(UART_MAGIC,6,u32)				/*!<  获得串口当前接收到的计数值*/
#define UART_START                          _IO(UART_MAGIC,7)					/*!<  开启串口*/
#define UART_SET_CIRCULAR_BUFF_ADDR         _IOW(UART_MAGIC,8,void *)			/*!<  设置串口循环buf地址*/
#define UART_SET_CIRCULAR_BUFF_LENTH        _IOW(UART_MAGIC,9,u32)				/*!<  设置串口循环buf长度*/
#define UART_SET_BAUDRATE                   _IOW(UART_MAGIC,10,u32)				/*!<串口运行过程中更换波特率，初始化串口时不需要调用这*/
/* \} name */


/// \cond DO_NOT_DOCUMENT
#define UART_PLATFORM_DATA_BEGIN(data) \
    static const struct uart_platform_data data = {


#define UART_PLATFORM_DATA_END() \
    };


struct uart_device {
    char *name;
    const struct uart_operations *ops;
    struct device dev;
    const struct uart_platform_data *priv;
    OS_MUTEX mutex;
};

struct uart_operations {
    int (*init)(struct uart_device *);
    int (*read)(struct uart_device *, void *buf, u32 len);
    int (*write)(struct uart_device *, void *buf, u16 len);
    int (*ioctl)(struct uart_device *, u32 cmd, u32 arg);
    int (*close)(struct uart_device *);
};


#define REGISTER_UART_DEVICE(dev) \
    static struct uart_device dev //SEC_USED(.uart)

extern struct uart_device uart_device_begin[], uart_device_end[];

#define list_for_each_uart_device(p) \
    for (p=uart_device_begin; p<uart_device_end; p++)

extern const struct device_operations uart_dev_ops;
/// \endcond
#endif
