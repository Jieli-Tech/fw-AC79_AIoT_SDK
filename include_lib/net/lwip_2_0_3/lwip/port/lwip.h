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
};

enum LWIP_NETIF {
    ETH_NETIF,
    WIFI_NETIF,
    BT_NETIF,
};

#ifndef __LW_IP_C
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
void Init_LwIP(enum LWIP_NETIF lwip_netif, u8_t dhcp);
void lwip_netif_set_up(enum LWIP_NETIF lwip_netif);
void lwip_renew(enum LWIP_NETIF lwip_netif, u8_t dhcp);
int gethostname(char *name, int namelen);
void Get_IPAddress(enum LWIP_NETIF lwip_netif, char *ipaddr);
int lwip_get_dest_hwaddr(enum LWIP_NETIF lwip_netif, ip4_addr_t *ipaddr, struct eth_addr *dhwaddr);
void LwIP_Pkt_Handle(void);
void lwip_set_cb(int (*cb)(void *, enum LWIP_EVENT, void *), void *priv);
void lwip_set_dhcp_timeout(int sec);
int get_netif_macaddr_and_ipaddr(ip4_addr_t *ipaddr, unsigned char *mac_addr, int count);
struct netif *ip4_route2(const ip4_addr_t *src, const ip4_addr_t *dest);
struct lan_setting *get_lan_setting_info(void);
int set_lan_setting_info(void *priv);
void lwip_get_netif_info(u8 is_wireless, struct netif_info *info);
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

