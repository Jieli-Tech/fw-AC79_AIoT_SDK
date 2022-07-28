
#include "uip.h"
#include "uip_arp.h"
#include "os/os_api.h"
#include "uip_timer.h"
#include "dhcpc.h"
#include "tcp_demo.h"
#include "udp_demo.h"



static void uip_task(void *prive)
{
    static u8 init_flog = 0;
    u8_t tcp_server_tsta = 0xff;
    u8_t tcp_client_tsta = 0xff;
    u8_t udp_server_tsta = 0xff;
    u8_t udp_client_tsta = 0xff;
    static struct timer  arp_timer;
    DEBUG_PRINTF(">>>>>>>>>>>>uip_test");

    while (1) {


        if (DHCP_ok_flog) { //DHCP已经获取到了动态分配的IP
            if (!init_flog) { //初始化配置进行一次
                init_flog = 1;
                tcp_server_connect();
                /*tcp_client_connect();*/
                /*udp_server_connect();*/ //需要开关宏定义USE_UDP_CLIENT USE_UDP_SERVER
                /*udp_client_connect();*/
            }
        }

        if (tcp_server_tsta != tcp_server_sta) { //TCP Server状态改变
            if (tcp_server_sta & (1 << 7)) {
                printf("TCP Server Connected\r\n");
            } else {
                printf("TCP Server Disconnected\r\n");
            }
            if (tcp_server_sta & (1 << 6)) {	//收到新数据
                printf("TCP Server RX:%s\r\n", tcp_server_rx_databuf); //打印数据
                tcp_send_data(" TCP Server TX Successfully!\r\n");
                tcp_server_sta &= ~(1 << 6);		//标记数据已经被处理
            }
            tcp_server_tsta = tcp_server_sta;
        }

        if (tcp_client_tsta != tcp_client_sta) { //TCP Client状态改变
            if (tcp_client_sta & (1 << 7)) {
                printf("TCP Client Connected\r\n");
            } else {
                printf("TCP Client Disconnected\r\n");
            }
            if (tcp_client_sta & (1 << 6)) {	//收到新数据
                printf("TCP Client RX:%s\r\n", tcp_client_rx_databuf); //打印数据
                tcp_send_data("TCP Client TX Successfully!\r\n");
                tcp_client_sta &= ~(1 << 6);		//标记数据已经被处理
            }
            tcp_client_tsta = tcp_client_sta;
        }

        if (udp_server_tsta != udp_server_sta) { //UDP Server状态改变
            if (udp_server_sta & (1 << 7)) {
                printf("UDP Server Connected\r\n");
            } else {
                printf("UDP Server Disconnected\r\n");
            }
            if (udp_server_sta & (1 << 6)) {	//收到新数据
                printf("UDP Server RX:%s\r\n", udp_server_rx_databuf); //打印数据
                udp_send_data(" UDP Server TX Successfully!\r\n");
                udp_server_sta &= ~(1 << 6);		//标记数据已经被处理
            }
            udp_server_tsta = udp_server_sta;
        }

        if (udp_client_tsta != udp_client_sta) { //UDP Client状态改变
            if (udp_client_sta & (1 << 7)) {
                printf("UDP Client Connected\r\n");
            } else {
                printf("UDP Client Disconnected\r\n");
            }
            if (udp_client_sta & (1 << 6)) {	//收到新数据
                printf("UDP Client RX:%s\r\n", udp_client_rx_databuf); //打印数据
                udp_send_data(" UDP Client TX Successfully!\r\n");
                udp_client_sta &= ~(1 << 6);		//标记数据已经被处理
            }
            udp_client_tsta = udp_client_sta;
        }

        os_time_dly(50);
    }
}

/*测试程序入口*/
void uip_test(void)
{
    thread_fork("uip_task", 20, 512, 0, NULL, uip_task, NULL);
}
/*调用该函数进行测试*/
int uip_test_main()
{
    uip_test();
    return 0 ;
}

