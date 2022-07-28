#include "uip.h"
#include "tcp_demo.h"
#include <string.h>
#include "stdio.h"


void tcp_send_data(char *data)
{
    struct tcp_demo_appstate *s = (struct tcp_demo_appstate *)&uip_conn->appstate;

    s->textptr = (u8_t *)data;
    s->textlen = strlen((char *)s->textptr);
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
        DEBUG_PRINTF("tcp_client_demo_appcall 1200");
        tcp_client_demo_appcall();
        break;

    default:
        break;
    }
}
//打印日志用
void uip_log(char *m)
{
    printf("uIP log:%s\r\n", m);
}


























