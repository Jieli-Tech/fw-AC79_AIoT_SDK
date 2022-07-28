#ifndef __LW_IP_H
#define __LW_IP_H
#include "lwip/netif.h"
#include "lwip/prot/ethernet.h"
#ifdef __cplusplus
extern "C" { /* Make sure we have C-declarations in C++ programs */
#endif

/* Includes ------------------------------------------------------------------*/
/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/

struct netif_info {
    u32 ip;
    u32 gw;
    u32 netmask;
};

struct lan_setting {

    u8 WIRELESS_IP_ADDR0;//无线IP地址
    u8 WIRELESS_IP_ADDR1;
    u8 WIRELESS_IP_ADDR2;
    u8 WIRELESS_IP_ADDR3;

    u8 WIRELESS_NETMASK0;//无线掩码
    u8 WIRELESS_NETMASK1;
    u8 WIRELESS_NETMASK2;
    u8 WIRELESS_NETMASK3;

    u8 WIRELESS_GATEWAY0;//无线网关
    u8 WIRELESS_GATEWAY1;
    u8 WIRELESS_GATEWAY2;
    u8 WIRELESS_GATEWAY3;

    u8 SERVER_IPADDR1;//DHCP服务器地址
    u8 SERVER_IPADDR2;
    u8 SERVER_IPADDR3;
    u8 SERVER_IPADDR4;

    u8 CLIENT_IPADDR1;//起始IP地址
    u8 CLIENT_IPADDR2;
    u8 CLIENT_IPADDR3;
    u8 CLIENT_IPADDR4;

    u8 SUB_NET_MASK1;//子网掩码
    u8 SUB_NET_MASK2;
    u8 SUB_NET_MASK3;
    u8 SUB_NET_MASK4;
};

enum LWIP_EVENT {
    LWIP_WIRELESS_DHCP_BOUND_TIMEOUT,
    LWIP_WIRELESS_DHCP_BOUND_SUCC,

    LWIP_WIRE_DHCP_BOUND_TIMEOUT,
    LWIP_WIRE_DHCP_BOUND_SUCC,

    LWIP_LTE_DHCP_BOUND_TIMEOUT,
    LWIP_LTE_DHCP_BOUND_SUCC,
};

#define  ETH_NETIF      (0)
#define  WIFI_NETIF     (1)
#define  BT_NETIF       (2)
#define  LTE_NETIF      (3)

#ifndef __LW_IP_C
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Init_LwIP(u8_t lwip_netif, u8_t dhcp);
void lwip_netif_set_up(u8_t lwip_netif);
void lwip_renew(u8_t lwip_netif, u8_t dhcp);
int lwip_dhcp_bound(void);
void lwip_dhcp_release_and_stop(u8_t lwip_netif);
int gethostname(char *name, int namelen);
void Get_IPAddress(u8_t lwip_netif, char *ipaddr);
int lwip_get_dest_hwaddr(u8_t lwip_netif, ip4_addr_t *ipaddr, struct eth_addr *dhwaddr);
void LwIP_Pkt_Handle(void);
int  lwip_event_cb(void *lwip_ctx, enum LWIP_EVENT event);
void lwip_set_dhcp_timeout(int sec);
int get_netif_macaddr_and_ipaddr(ip4_addr_t *ipaddr, unsigned char *mac_addr, int count);
struct netif *ip4_route2(const ip4_addr_t *src, const ip4_addr_t *dest);
struct lan_setting *net_get_lan_info(void);
int net_set_lan_info(struct lan_setting *__lan_setting_info);
void lwip_get_netif_info(u8 is_wireless, struct netif_info *info);
void set_wireless_netif_macaddr(const char *mac_addr);
#endif /* !defined(__LW_IP_C) */

/*
#error section
-- The standard C preprocessor directive #error should be used to notify the
programmer when #define constants or macros are not present and to indicate
that a #define value is out of range. These statements are normally found in
a module's .H file. The #error directive will display the message within the
double quotes when the condition is not met.
*/


#ifdef __cplusplus
}
#endif

#define NI_NUMERICHOST (1<<0)
#define NI_NUMERICSERV (1<<1)

#endif /* #ifndef __LW_IP_H */
/*-- File end --*/

