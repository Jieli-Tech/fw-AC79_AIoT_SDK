#ifndef __DHCPS_H__
#define __DHCPS_H__

#include "uip.h"

typedef unsigned char u8_t;
typedef int32_t   s32_t;
typedef uint32_t  u32_t;
typedef int16_t   s16_t;
typedef unsigned short u16_t;

#define DHCP_DBG_PRINTF             //printf
#define DHCP_DBG_FUNC_IN()          //printf("\n\n+In func:%s \n",__FUNCTION__)
#define DHCP_DBG_SEND(p,len)        //do{printf("\n\n%s send:",__FUNCTION__);put_buf((p),(len));puts("\n");}while(0)
#define DHCP_DBG_RECV(p,len)        //do{printf("\n\n%s recv:",__FUNCTION__);put_buf((p),(len));puts("\n");}while(0)

#define LEASE_TIME   86400U   //sec

#define LEASE_TMR_INTERVAL  60*60*1000U       //mill_sec

enum _dhcps_state {
    DHCPS_STATE_OFFER,
    DHCPS_STATE_DECLINE,
    DHCPS_STATE_ACK,
    DHCPS_STATE_NAK,
    DHCPS_STATE_IDLE,
};

struct _dhcps_cli {
    u8_t cli_mac[6];
    uip_ipaddr_t ipaddr;
    s32_t timeout;
    struct _dhcps_cli *next;
};

#define  ETH_NETIF 0
#define  WIFI_NETIF 1
#define  BT_NETIF 2

#define LEASE_TIME   86400U   //sec
#define LEASE_TMR_INTERVAL  60*60*1000U       //mill_sec

#define TEMP_BUF_SIZE 2*1024
#define DHCP_MAGIC_COOKIE           0x63825363UL
#define DHCP_OPTION_SUBNET_MASK     1 /* RFC 2132 3.3 */
#define DHCP_HTYPE_ETH    1
#define DHCP_OPTION_END             255
#define DHCP_OPTION_ROUTER          3
#define DHCP_OPTION_LEASE_TIME      51 /* RFC 2132 9.2, time in seconds, in 4 bytes */
#define DHCP_OPTION_MESSAGE_TYPE    53 /* RFC 2132 9.6, important for DHCP */
#define DHCP_OPTION_SERVER_ID       54 /* RFC 2132 9.7, server IP address */
#define DHCP_OPTION_DNS_SERVER      6
#define DHCP_BOOTREPLY              2
#define DHCP_OFFER                  2
#define DHCP_NAK                    6
#define DHCP_CLIENT_PORT  68        //dhcp客户端端口
#define DHCP_SERVER_PORT  67       	//dhcp服务端端口
#define DHCP_ACK                    5
#define DHCP_DECLINE                4
#define DHCP_DISCOVER               1
#define DHCP_REQUEST                3
#define DHCP_RELEASE                7
#define DHCP_OPTION_REQUESTED_IP    50 /* RFC 2132 9.1, requested IP address */
#define mem_malloc  malloc
#define mem_free    free
#define LWIP_UNUSED_ARG(x) (void)x

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

#define ip4_addr_get_byte(ipaddr, idx) ((u8_t *)ipaddr)[idx]

#define ip4_addr1(ipaddr) ip4_addr_get_byte(ipaddr, 0)
#define ip4_addr2(ipaddr) ip4_addr_get_byte(ipaddr, 1)
#define ip4_addr3(ipaddr) ip4_addr_get_byte(ipaddr, 2)
#define ip4_addr4(ipaddr) ip4_addr_get_byte(ipaddr, 3)

struct ip4_addr_packed {
    u32_t addr;
};

typedef struct ip4_addr_packed ip4_addr_p_t;

#define DHCP_CHADDR_LEN   16U
#define DHCP_SNAME_LEN    64U
#define DHCP_FILE_LEN     128U

#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_FLD_8(x) PACK_STRUCT_FIELD(x)
#define PACK_STRUCT_FLD_S(x) PACK_STRUCT_FIELD(x)

#define DHCP_OPTIONS_LEN DHCP_MIN_OPTIONS_LEN
struct dhcp_msg {
    PACK_STRUCT_FLD_8(u8_t op);
    PACK_STRUCT_FLD_8(u8_t htype);
    PACK_STRUCT_FLD_8(u8_t hlen);
    PACK_STRUCT_FLD_8(u8_t hops);
    PACK_STRUCT_FIELD(u32_t xid);
    PACK_STRUCT_FIELD(u16_t secs);
    PACK_STRUCT_FIELD(u16_t flags);
    PACK_STRUCT_FLD_S(ip4_addr_p_t ciaddr);
    PACK_STRUCT_FLD_S(ip4_addr_p_t yiaddr);
    PACK_STRUCT_FLD_S(ip4_addr_p_t siaddr);
    PACK_STRUCT_FLD_S(ip4_addr_p_t giaddr);
    PACK_STRUCT_FLD_8(u8_t chaddr[DHCP_CHADDR_LEN]);
    PACK_STRUCT_FLD_8(u8_t sname[DHCP_SNAME_LEN]);
    PACK_STRUCT_FLD_8(u8_t file[DHCP_FILE_LEN]);
    PACK_STRUCT_FIELD(u32_t cookie);
    PACK_STRUCT_FLD_8(u8_t options[68]);
    /* PACK_STRUCT_FLD_8(u8_t options[312]); */
};

#define PP_HTONL(x) ((((x) & (u32_t)0x000000ffUL) << 24) | \
                     (((x) & (u32_t)0x0000ff00UL) <<  8) | \
                     (((x) & (u32_t)0x00ff0000UL) >>  8) | \
                     (((x) & (u32_t)0xff000000UL) >> 24))

void dhcps_init(void);
void dhcps_uninit(void);
void display_IPAddress(void);
void dhcps_appcall(void);
int net_set_lan_info(struct lan_setting *__lan_setting_info);

#endif
