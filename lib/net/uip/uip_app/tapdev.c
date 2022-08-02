#include "uip.h"
#include "string.h"
#include "tapdev.h"
#include "device/device.h"
#include "uip_arp.h"
#include "uip_timer.h"

extern void netdev_rx_register(void (*fun)(void *priv, void *data, unsigned int len), void *priv);
extern void *netdev_alloc_output_buf(unsigned char **buf, unsigned int len);
extern int netdev_send_data(void *priv, void *skb);
extern void UipPro(void);
extern int wifi_get_mac(u8 *mac_addr);
extern int uip_test_main(char *mac);



static void uip_rx_cb(void *param, void *data, int len)//接收数据
{
    memcpy(uip_buf, data, len); //拷贝网卡接收到的数据到uip全局变量
    uip_len = len ;//将接收到的数据长度传入uip全局变量
    put_buf(uip_buf, len);
    DEBUG_PRINTF(">>>>>>>>>>>>>>RX len = %d \r\n", len);
}
void tapdev_init(void)
{
    char mac[6];
    char i;

    uip_init();	 //UIP协议栈初始化
    DEBUG_PRINTF(">>>>>>>>>>uip_init<<<<<<<<<");

    wifi_get_mac(mac);//获取网卡MAC地址传入uip全局变量

    for (i = 0; i < 6; i++) {
        uip_ethaddr.addr[i] = mac[i];
    }

    netdev_rx_register((void (*)(void *, void *, unsigned int))uip_rx_cb, NULL);//注册回调函数接收网卡数据

    uip_test_main(mac);//uip测试入口
}

void tapdev_send(void)
{
    char *buffer, *pos;

    buffer = (char *)netdev_alloc_output_buf((unsigned char **)(&pos), uip_len);//申请发送buf内存 申请的buf 为 ops buffer为结构体地址

    if (buffer == NULL) {
        DEBUG_PRINTF("alloc_uip_buf [erro]!!!!");
    }

    put_buf(uip_buf, uip_len);
    DEBUG_PRINTF("uip_send_buf len = %d \r\n", uip_len);
    memcpy(pos, uip_buf, uip_len);//将发送的数据拷贝到申请的buf内存中

    netdev_send_data(NULL, buffer);//网卡发送数据
}
