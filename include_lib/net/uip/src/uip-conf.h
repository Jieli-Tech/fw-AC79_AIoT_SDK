#ifndef __UIP_CONF_H__
#define __UIP_CONF_H__

#if 0
#define DEBUG_PRINTF(x, ...)    printf("\n>>>>>>>>>>>>>[uip]##" x " ", ## __VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

#include <inttypes.h>
typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint8_t u8;
typedef uint16_t u16;
typedef unsigned short uip_stats_t;



//使用DHCP获取路由器分配的IP信息
#define USE_DHCP_GET_IP          1
//最大TCP连接数
#define UIP_CONF_MAX_CONNECTIONS 40

//最大TCP端口监听数
#define UIP_CONF_MAX_LISTENPORTS 40

//uIP缓存大小
#define UIP_CONF_BUFFER_SIZE     4096

//CPU大小端模式
//STM32是小端模式的
#define UIP_CONF_BYTE_ORDER  UIP_LITTLE_ENDIAN

//日志开关
#define UIP_CONF_LOGGING         1

//UDP支持开关
#define UIP_CONF_UDP             1

//UDP校验和开关
#define UIP_CONF_UDP_CHECKSUMS   1


//uIP统计开关
#define UIP_CONF_STATISTICS      1


#include "tcp_demo.h"
#include "udp_demo.h"

#endif
























