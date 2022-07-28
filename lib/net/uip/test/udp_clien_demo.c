
#include "udp_demo.h"
#include "uip.h"
#include <string.h>
#include <stdio.h>

u8 udp_client_databuf[200];   	//发送数据缓存
u8 udp_client_rx_databuf[200];
u8 udp_client_sta;				//客户端状态
//[7]:0,无连接;1,已经连接;
//[6]:0,无数据;1,收到客户端数据
//[5]:0,无数据;1,有数据需要发送

//这是一个udp 客户端应用回调函数。
//该函数通过UIP_APPCALL(udp_demo_appcall)调用,实现Web Client的功能.
//当uip事件发生时，UIP_APPCALL函数会被调用,根据所属端口(1400),确定是否执行该函数。
//例如 : 当一个udp连接被创建时、有新的数据到达、数据已经被应答、数据需要重发等事件
void udp_client_demo_appcall(void)
{
    struct udp_demo_appstate *s = (struct udp_demo_appstate *)&uip_udp_conn->appstate;

    if (uip_newdata()) { //接收新的udp数据包
        if ((udp_client_sta & (1 << 6)) == 0) { //还未收到数据
            if (uip_len > 199) {
                ((u8 *)uip_appdata)[199] = 0;
            }

            memset(udp_client_rx_databuf, 0, sizeof(udp_client_rx_databuf));
            memcpy(udp_client_rx_databuf, uip_appdata, uip_len);
            udp_client_sta |= 1 << 6; //表示收到客户端数据
        }
    }

    if (uip_poll()) {	//当前连接空闲轮训
        if (udp_client_sta & (1 << 5)) { //有数据需要发送
            s->textptr = udp_client_databuf;
            s->textlen = strlen((const char *)s->textptr);
            udp_client_sta &= ~(1 << 5); //清除标记
            uip_send(s->textptr, s->textlen);//发送udp数据包
        }
    }

}

//建立一个udp_client的连接
void udp_client_connect(void)
{
    uip_ipaddr_t ipaddr;
    static struct uip_udp_conn *c = 0;
    uip_ipaddr(&ipaddr, 192, 168, 31, 228);	//设置目标服务器UIP为

    if (c != 0) {							//已经建立连接则删除连接
        uip_udp_remove(c);
    }
    c = uip_udp_new(&ipaddr, htons(UDP_CLIENT_PORT)); 	//远程端口为23

    udp_client_sta |= 1 << 7;		//标志连接成功
}




