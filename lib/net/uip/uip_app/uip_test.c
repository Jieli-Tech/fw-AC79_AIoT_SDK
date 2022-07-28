
#include "uip.h"
#include "uip_arp.h"
#include "tapdev.h"
#include "os/os_compat.h"
#include "uip_timer.h"
#include "dhcpc.h"
#include "tcp_demo.h"
#include "udp_demo.h"


#define BUF ((struct uip_eth_hdr *)&uip_buf[0])

void UipPro(void)
{
    u8 i;
    static struct timer periodic_timer, arp_timer;
    static u8 timer_ok = 0;

    if (timer_ok == 0) { //仅初始化一次
        timer_ok = 1;
        timer_set(&periodic_timer, CLOCK_SECOND / 2); //创建1个0.5秒的定时器
        timer_set(&arp_timer, CLOCK_SECOND * 10);	   	//创建1个10秒的定时器
    }

    if (uip_len > 0) {		//有数据
        //处理IP数据包(只有校验通过的IP包才会被接收)
        if (BUF->type == htons(UIP_ETHTYPE_IP)) { //是否是IP包?
            uip_arp_ipin();	//去除以太网头结构，更新ARP表
            uip_input();   	//IP包处理
            //当上面的函数执行后，如果需要发送数据，则全局变量 uip_len > 0
            //需要发送的数据在uip_buf, 长度是uip_len  (这是2个全局变量)
            if (uip_len > 0) { //需要回应数据
                uip_arp_out();//加以太网头结构，在主动连接时可能要构造ARP请求
                tapdev_send();//发送数据到以太网
            }
        } else if (BUF->type == htons(UIP_ETHTYPE_ARP)) { //处理arp报文,是否是ARP请求包?
            uip_arp_arpin();

            //当上面的函数执行后，如果需要发送数据，则全局变量uip_len>0
            //需要发送的数据在uip_buf, 长度是uip_len(这是2个全局变量)
            if (uip_len > 0) {
                tapdev_send();    //需要发送数据,则通过tapdev_send发送
            }
        }
    } else if (timer_expired(&periodic_timer)) {	//0.5秒定时器超时
        timer_reset(&periodic_timer);		//复位0.5秒定时器

        //轮流处理每个TCP连接, UIP_CONNS缺省是40个
        for (i = 0; i < UIP_CONNS; i++) {
            uip_periodic(i);	//处理TCP通信事件

            //当上面的函数执行后，如果需要发送数据，则全局变量uip_len>0
            //需要发送的数据在uip_buf, 长度是uip_len (这是2个全局变量)
            if (uip_len > 0) {
                uip_arp_out();//加以太网头结构，在主动连接时可能要构造ARP请求
                tapdev_send();//发送数据到以太网
            }
        }

#if UIP_UDP	//UIP_UDP

        //轮流处理每个UDP连接, UIP_UDP_CONNS缺省是10个
        for (i = 0; i < UIP_UDP_CONNS; i++) {
            uip_udp_periodic(i);	//处理UDP通信事件

            //当上面的函数执行后，如果需要发送数据，则全局变量uip_len>0
            //需要发送的数据在uip_buf, 长度是uip_len (这是2个全局变量)
            if (uip_len > 0) {
                uip_arp_out();//加以太网头结构，在主动连接时可能要构造ARP请求
                tapdev_send();//发送数据到以太网
            }
        }

#endif

        //每隔10秒调用1次ARP定时器函数 用于定期ARP处理,ARP表10秒更新一次，旧的条目会被抛弃
        if (timer_expired(&arp_timer)) {
            timer_reset(&arp_timer);
            uip_arp_timer();
        }
    }
}
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

        UipPro();//该函数需要放在循环里面处理数据

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

        os_time_dly(1);
    }
}

/*测试程序入口*/
void uip_test(void)
{
    thread_fork("uip_task", 20, 512, 0, NULL, uip_task, NULL);
}

int uip_test_main(char *mac)
{
    uip_ipaddr_t ipaddr;

    uip_ipaddr(ipaddr, 0, 0, 0, 0); //初始化IP让DHCP去获取IP
    uip_sethostaddr(ipaddr);
    uip_ipaddr(ipaddr, 0, 0, 0, 0);
    uip_setnetmask(ipaddr);

    dhcpc_init(mac, 6);
    printf("dhcp init ok");
    dhcpc_request();

    uip_test();

    return 0 ;

}

