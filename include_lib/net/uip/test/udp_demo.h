#ifndef __UDP_DEMO_H
#define __UDP_DEMO_H

#include "uip-conf.h"

/*#define USE_UDP_SERVER */
#define USE_UDP_CLIENT

struct udp_demo_appstate {
    unsigned char state;
    unsigned char *textptr;
    int textlen;
};
typedef struct udp_demo_appstate uip_udp_appstate_t;

#ifndef UIP_UDP_APPCALL
#define UIP_UDP_APPCALL udp_demo_appcall
#endif

#ifndef UIP_UDP_CONNS
#define UIP_UDP_CONNS 10
#endif

#ifndef UIP_CONF_BROADCAST
#define UIP_CONF_BROADCAST 1
#endif

/////////////////////////////////////udp DEMO/////////////////////////////////////
void udp_demo_appcall(void);
void udp_server_init(void);
void udp_send_data(char *data);
/////////////////////////////////////udp SERVER/////////////////////////////////////
#define UDP_SERVER_PORT	1800
extern u8_t udp_server_databuf[200];   		//发送数据缓存
extern u8_t udp_server_rx_databuf[200];
extern u8_t udp_server_sta;				//服务端状态
//udp server 函数
void udp_server_senddata(void);
void udp_server_connect(void);
void udp_server_demo_appcall(void);
/////////////////////////////////////udp CLIENT/////////////////////////////////////
#define UDP_CLIENT_PORT	1900
extern u8_t udp_client_databuf[200];   		//发送数据缓存
extern u8_t udp_client_rx_databuf[200];
extern u8_t udp_client_sta;				//客户端状态
//udp client 函数
void udp_client_connect(void);
void udp_client_demo_appcall(void);
////////////////////////////////////////////////////////////////////////////////////

#endif

