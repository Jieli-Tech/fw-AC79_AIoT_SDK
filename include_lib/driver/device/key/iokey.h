#ifndef DEVICE_IOKEY_H
#define DEVICE_IOKEY_H

#include "generic/typedef.h"
#include "device/device.h"


///  \cond DO_NOT_DOCUMENT
#define ONE_PORT_TO_LOW 		0 		//按键一个端口接低电平, 另一个端口接IO
#define ONE_PORT_TO_HIGH		1 		//按键一个端口接高电平, 另一个端口接IO
#define DOUBLE_PORT_TO_IO		2		//按键两个端口接IO
#define CUST_DOUBLE_PORT_TO_IO	3
/// \endcond


struct one_io_key {
    u8 port;						/*!<  io按键引脚*/
};

struct two_io_key {
    u8 in_port;						/*!<  io按键输入引脚*/
    u8 out_port;					/*!<  io按键输出引脚*/
};

union key_type {
    struct one_io_key one_io;		/*!<  单io按键*/
    struct two_io_key two_io;		/*!<  双io按键*/
};

struct iokey_port {
    union key_type key_type;		/*!<  io按键类型*/
    u8 connect_way;					/*!<  io按键连接方式*/
    u8 key_value;					/*!<  io按键值*/
};

struct iokey_platform_data {
    u8 enable;						/*!<  io按键使能，使能为1，不使能为0*/
    u8 num;							/*!<  io按键数量*/
    const struct iokey_port *port;  /*!<  io按键参数*/
};

/**
 * @brief iokey_init：io按键初始化
 *
 * @param iokey_data io按键句柄
 */
extern int iokey_init(const struct iokey_platform_data *iokey_data);

/**
 * @brief io_get_key_value：获取io按键值
 */
extern u8 io_get_key_value(void);

#endif
