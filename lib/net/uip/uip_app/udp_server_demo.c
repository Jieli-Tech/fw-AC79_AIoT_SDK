
#include "udp_demo.h"
#include "uip.h"
#include <string.h>
#include <stdio.h>

u8 udp_server_databuf[200];   	//发送数据缓存
u8 udp_server_rx_databuf[200];   	//发送数据缓存
u8 udp_server_sta;				//服务端状态
//[7]:0,无连接;1,已经连接;
//[6]:0,无数据;1,收到客户端数据
//[5]:0,无数据;1,有数据需要发送


//这是一个udp 服务器应用回调函数。
//该函数通过UIP_APPCALL(udp_demo_appcall)调用,实现Web Server的功能.
//当uip事件发生时，UIP_APPCALL函数会被调用,根据所属端口(1200),确定是否执行该函数。
//例如 : 当一个udp连接被创建时、有新的数据到达、数据已经被应答、数据需要重发等事件
void udp_server_demo_appcall(void)
{
    char *data;
    struct udp_demo_appstate *s = (struct udp_demo_appstate *)&uip_udp_conn->appstate;

    DEBUG_PRINTF("udp_server uip_poll!");//打印log

    if (uip_newdata()) { //收到客户端发过来的数据
        //接收到一个新的udp数据包
        DEBUG_PRINTF("udp_server have data!");//打印log

        if ((udp_server_sta & (1 << 6)) == 0) { //还未收到数据
            if (uip_len > 199) {
                ((u8_t *)uip_appdata)[199] = 0;
            }

            memset(udp_server_rx_databuf, 0, sizeof(udp_server_rx_databuf));
            memcpy(udp_server_rx_databuf, uip_appdata, uip_len);
            udp_server_sta |= 1 << 6; //表示收到客户端数据
        }
    }

    if (uip_poll()) {
        if (udp_server_sta & (1 << 5)) { //有数据需要发送
            s->textptr = udp_server_databuf;
            s->textlen = strlen((const char *)s->textptr);
            udp_server_sta &= ~(1 << 5); //清除标记
            uip_send(s->textptr, s->textlen);//发送udp数据包
        }
    }
}

//建立一个udp_server连接
void udp_server_connect(void)
{
    uip_ipaddr_t ipaddr;
    static struct uip_udp_conn *conn = 0;

    if (conn != 0) {	//已经建立连接则删除连接
        uip_udp_remove(conn);
    }

    uip_ipaddr(&ipaddr, 0, 0, 0, 0);	//将远程IP设置为 255.255.255.255 具体原理见uip.c的源码
    conn = uip_udp_new(&ipaddr, 0); 	//远程端口为0

    if (conn) {
        uip_udp_bind(conn, HTONS(UDP_SERVER_PORT));
    }

    udp_server_sta |= 1 << 7;		//标志连接成功
}
//发送数据给客户端
void udp_server_senddata(void)
{
    struct udp_demo_appstate *s = (struct udp_demo_appstate *)&uip_conn->appstate;

    //s->textptr : 发送的数据包缓冲区指针
    //s->textlen ：数据包的大小（单位字节）
    if (s->textlen > 0) {
        uip_send(s->textptr, s->textlen);    //发送udp数据包
    }
}










