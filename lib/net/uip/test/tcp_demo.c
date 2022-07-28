#include "uip.h"
#include "tcp_demo.h"
#include <string.h>
#include "stdio.h"


void tcp_send_data(char *data)
{
    tcp_server_sta |= 1 << 5;		//标志连接成功
    memset(tcp_server_databuf, 0, sizeof(tcp_server_databuf));
    memcpy(tcp_server_databuf, data, strlen(data));
}
//TCP应用接口函数(UIP_APPCALL)
//完成TCP服务(包括server和client)和HTTP服务
void tcp_demo_appcall(void)
{
    switch (uip_conn->lport) { //本地监听端口TCP_SERVER_PORT
    case HTONS(TCP_SERVER_PORT):
        DEBUG_PRINTF("tcp_server_demo_appcall 1200");
        tcp_server_demo_appcall();
        break;

    default:
        break;
    }

    switch (uip_conn->rport) {	//远程连接TCP_CLIENT_PORT端口
    case HTONS(TCP_CLIENT_PORT):
        DEBUG_PRINTF("tcp_client_demo_appcall 8000");
        tcp_client_demo_appcall();
        break;

    default:
        break;
    }
}


























