#include "system/init.h"
#include "lwip.h"
#include "app_config.h"
#include "event/event.h"
#include "event/device_event.h"
#include "lte_module.h"
#include "log.h"

#define     __LOG_LEVEL     __LOG_DEBUG

/*
 *
 *******************************************多网卡使用说明*******************************************
 *
 *      1.需要配置：
 *          a)使能LwIP.c文件中的宏，HAVE_LTE_NETIF;
 *          b)使能LWIP配置文件(lwipopts_sfc.h/lwipopts.h)中的宏，LWIP_HOOK_IP4_ROUTE_SRC;
 *
 *      2.IP层根据目标IP地址在网卡列表中选择一个网卡。匹配条件：目标IP地址与网卡在同一个
 *      网段内，即匹配;否则将使用默认网卡;
 *
 *      3.可通过 lwip_set_default_netif函数切换默认网卡。
 *
 ****************************************************************************************************
 *
 * */


static char ip_addr[32];
static char gw_addr[32];
static void *dev = NULL;
static int lte_app_task_pid;


int lte_lwip_event_cb(void *lwip_ctx, enum LWIP_EVENT event)
{
    switch (event) {
    case LWIP_LTE_DHCP_BOUND_TIMEOUT:
        break;

    case LWIP_LTE_DHCP_BOUND_SUCC:
        Get_IPAddress(LTE_NETIF, ip_addr);
        get_gateway(LTE_NETIF, gw_addr);
        log_i("LTE DHCP SUCC, IP:[%s] \r\n, GW:[%s]", ip_addr, gw_addr);

        //此处4G网络已连通

        lwip_set_default_netif(LTE_NETIF);   //设置4G网卡为默认模块
        void lte_network_test(void);         //4G网络测试
        lte_network_test();
        break;

    default:
        break;
    }

    return 0;
}


char *get_lte_ip(void)
{
    return &ip_addr[0];
}


static void at_cmd_test(void *priv)
{
    usbnet_host_at_data_send("AT\r\n", strlen("AT\r\n"));
}


static void at_port_rx_handler(u8 *buf, u32 len)
{
    printf("%s, %s\n", __func__, buf);
}


static int lte_state_cb(void *priv, int on)
{
    if (!strncmp((u8 *)priv, "wireless", strlen("wireless"))) {
        if (on) {
            log_i("lte on\r\n");
            dev_ioctl(dev, LTE_NETWORK_START, NULL);
        } else {
            log_i("lte off\r\n");
            dev_ioctl(dev, LTE_NETWORK_STOP, NULL);
        }
    } else if (!strncmp((u8 *)priv, "at_port", strlen("at_port"))) {
        if (on) {
            log_i("lte at_port on\r\n");
            usbnet_at_port_rx_handler_register(at_port_rx_handler);
        } else {
            log_i("lte at_port off\r\n");
        }
    }

    return 0;
}


static int lte_net_init(void)
{
    dev = dev_open("lte", NULL);
    dev_ioctl(dev, LTE_DEV_SET_CB, (u32)lte_state_cb);

    if (dev) {
        log_i("lte early init succ\n\n\n\n\n\n");
    } else {
        log_e("lte early init fail\n\n\n\n\n\n");
    }

    return 0;
}

#ifdef CONFIG_LTE_PHY_ENABLE
late_initcall(lte_net_init);
#endif


