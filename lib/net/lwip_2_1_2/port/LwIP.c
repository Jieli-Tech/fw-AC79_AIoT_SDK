#define __LW_IP_C


#include "system/includes.h"
#include "stdlib.h"


/* Includes ------------------------------------------------------------------*/

#include "lwip/inet.h"
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/dhcp6.h"
#include "lwip/prot/dhcp.h"
#include "lwip/autoip.h"
#include "lwip/dns.h"
#include "lwip/etharp.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "lwip.h"
#include "lwip/sys.h"
#include "printf.h"
#include "sys_arch.h"
#include "eth/eth_phy.h"
#include "wifi/wifi_connect.h"

/*#define HAVE_ETH_WIRE_NETIF*/
/*#define HAVE_LTE_NETIF*/
#ifdef LIWP_USE_BT
#define HAVE_BT_NETIF
#endif

extern const u8  IPV4_ADDR_CONFLICT_DETECT;
extern char *itoa(int num, char *str, int radix);
extern err_t wireless_ethernetif_init(struct netif *netif);
extern err_t wired_ethernetif_init(struct netif *netif);
extern err_t bt_ethernetif_init(struct netif *netif);
extern err_t lte_ethernetif_init(struct netif *netif);
extern void ntp_client_get_time(const char *host);
int set_phy_stats_cb(u8 id, void (*f)(enum phy_state));

int  __attribute__((weak)) lwip_event_cb(void *lwip_ctx, enum LWIP_EVENT event)
{
    return 0;
}

void __attribute__((weak)) dns_set_server(unsigned int *dnsserver)
{
    *dnsserver = 0x23415679;
}
int __attribute__((weak)) set_phy_stats_cb(u8 id, void (*f)(enum phy_state))
{
    return 0;
}

void __attribute__((weak)) lwip_netflow(int in_out, int proto_type)
{
}


static struct lan_setting lan_setting_info = {

    .WIRELESS_IP_ADDR0  = 192,
    .WIRELESS_IP_ADDR1  = 168,
    .WIRELESS_IP_ADDR2  = 0,
    .WIRELESS_IP_ADDR3  = 1,

    .WIRELESS_NETMASK0  = 255,
    .WIRELESS_NETMASK1  = 255,
    .WIRELESS_NETMASK2  = 255,
    .WIRELESS_NETMASK3  = 0,

    .WIRELESS_GATEWAY0  = 192,
    .WIRELESS_GATEWAY1  = 168,
    .WIRELESS_GATEWAY2  = 0,
    .WIRELESS_GATEWAY3  = 1,

    .SERVER_IPADDR1  = 192,
    .SERVER_IPADDR2  = 168,
    .SERVER_IPADDR3  = 0,
    .SERVER_IPADDR4  = 1,

    .CLIENT_IPADDR1  = 192,
    .CLIENT_IPADDR2  = 168,
    .CLIENT_IPADDR3  = 0,
    .CLIENT_IPADDR4  = 101,

    .SUB_NET_MASK1   = 255,
    .SUB_NET_MASK2   = 255,
    .SUB_NET_MASK3   = 255,
    .SUB_NET_MASK4   = 0,
};

static struct netif wireless_netif;
static u8 lwip_static_ip_renew;

#ifdef HAVE_LTE_NETIF
static struct netif lte_netif;
static u32 lte_dhcp_timeout_cnt;
#endif

#ifdef HAVE_ETH_WIRE_NETIF
static struct netif  wire_netif;
static int use_dhcp = 1;
static u32 wire_dhcp_timeout_cnt;
#endif

#ifdef HAVE_BT_NETIF
static struct netif  bt_netif;
static u32 bt_dhcp_timeout_cnt;
#endif

#define DHCP_TMR_INTERVAL 100  //mill_sec
static int dhcp_timeout_msec = 15 * 1000;
static u32 wireless_dhcp_timeout_cnt;
static u8 get_time_init;
extern const u8 ntp_get_time_init;

struct lan_setting *net_get_lan_info(void)
{
    return &lan_setting_info;
}

int net_set_lan_info(struct lan_setting *__lan_setting_info)
{
    lan_setting_info.WIRELESS_IP_ADDR0  = __lan_setting_info->WIRELESS_IP_ADDR0;
    lan_setting_info.WIRELESS_IP_ADDR1  = __lan_setting_info->WIRELESS_IP_ADDR1;
    lan_setting_info.WIRELESS_IP_ADDR2  = __lan_setting_info->WIRELESS_IP_ADDR2;
    lan_setting_info.WIRELESS_IP_ADDR3  = __lan_setting_info->WIRELESS_IP_ADDR3;

    lan_setting_info.WIRELESS_NETMASK0  = __lan_setting_info->WIRELESS_NETMASK0;
    lan_setting_info.WIRELESS_NETMASK1  = __lan_setting_info->WIRELESS_NETMASK1;
    lan_setting_info.WIRELESS_NETMASK2  = __lan_setting_info->WIRELESS_NETMASK2;
    lan_setting_info.WIRELESS_NETMASK3  = __lan_setting_info->WIRELESS_NETMASK3;

    lan_setting_info.WIRELESS_GATEWAY0  = __lan_setting_info->WIRELESS_GATEWAY0;
    lan_setting_info.WIRELESS_GATEWAY1  = __lan_setting_info->WIRELESS_GATEWAY1;
    lan_setting_info.WIRELESS_GATEWAY2  = __lan_setting_info->WIRELESS_GATEWAY2;
    lan_setting_info.WIRELESS_GATEWAY3  = __lan_setting_info->WIRELESS_GATEWAY3;

    lan_setting_info.SERVER_IPADDR1     = __lan_setting_info->SERVER_IPADDR1;
    lan_setting_info.SERVER_IPADDR2     = __lan_setting_info->SERVER_IPADDR2;
    lan_setting_info.SERVER_IPADDR3     = __lan_setting_info->SERVER_IPADDR3;
    lan_setting_info.SERVER_IPADDR4     = __lan_setting_info->SERVER_IPADDR4;

    lan_setting_info.CLIENT_IPADDR1     = __lan_setting_info->CLIENT_IPADDR1;
    lan_setting_info.CLIENT_IPADDR2     = __lan_setting_info->CLIENT_IPADDR2;
    lan_setting_info.CLIENT_IPADDR3     = __lan_setting_info->CLIENT_IPADDR3;
    lan_setting_info.CLIENT_IPADDR4     = __lan_setting_info->CLIENT_IPADDR4;

    lan_setting_info.SUB_NET_MASK1      = __lan_setting_info->SUB_NET_MASK1;
    lan_setting_info.SUB_NET_MASK2      = __lan_setting_info->SUB_NET_MASK2;
    lan_setting_info.SUB_NET_MASK3      = __lan_setting_info->SUB_NET_MASK3;
    lan_setting_info.SUB_NET_MASK4      = __lan_setting_info->SUB_NET_MASK4;

    return 0;
}

/**
 * @brief Display_IPAddress Display IP Address
 *
 */
void Display_IPAddress(void)
{
    struct netif *netif = netif_list;

    while (netif != NULL) {
        printf("Default network interface: %c%c\n", netif->name[0], netif->name[1]);
        printf("ip4 address: %s\n", inet_ntoa(*((ip4_addr_t *) & (netif->ip_addr))));
        printf("gw address: %s\n", inet_ntoa(*((ip4_addr_t *) & (netif->gw))));
        printf("net mask  : %s\n\n", inet_ntoa(*((ip4_addr_t *) & (netif->netmask))));

#if LWIP_IPV6
        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            char *ip6_addr_state;
            if (ip6_addr_isinvalid(netif->ip6_addr_state[i])) {
                ip6_addr_state = "INVALID";
            } else if (ip6_addr_istentative(netif->ip6_addr_state[i])) {
                ip6_addr_state = "TENTATIVE";
            } else if (ip6_addr_isvalid(netif->ip6_addr_state[i])) {
                ip6_addr_state = "VALID";
            } else if (ip6_addr_ispreferred(netif->ip6_addr_state[i])) {
                ip6_addr_state = "PREFERRED";
            } else if (ip6_addr_isdeprecated(netif->ip6_addr_state[i])) {
                ip6_addr_state = "DEPRECATED";
            } else if (ip6_addr_isduplicated(netif->ip6_addr_state[i])) {
                ip6_addr_state = "DUPLICATE";
            } else {
                ip6_addr_state = "UNKNOW";
            }

            if (strcmp(ip6addr_ntoa(&netif->ip6_addr[i].u_addr.ip6), "::"))
                printf("ip6 address: %s, state = %s, valid_life_time = %d sec,  ip6_addr_pref_life = %d sec\n",
                       ip6addr_ntoa(&netif->ip6_addr[i].u_addr.ip6), ip6_addr_state, netif->ip6_addr_valid_life[i], netif->ip6_addr_pref_life[i]);
        }
#endif

        netif = netif->next;
    }
}

void lwip_set_dhcp_timeout(int sec)
{
    dhcp_timeout_msec = sec * 1000;
}

static void network_is_dhcp_bound(struct netif *netif)
{
    u32 *p_to = NULL;
    struct dhcp *dhcp = ((struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP));

    if (netif == &wireless_netif) {
        p_to = &wireless_dhcp_timeout_cnt;

    }
#ifdef HAVE_LTE_NETIF
    else if (netif == &lte_netif) {
        p_to = &lte_dhcp_timeout_cnt;
    }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
    else {
        p_to = &wire_dhcp_timeout_cnt;
    }
#elif defined HAVE_BT_NETIF
    else {
        p_to = &bt_dhcp_timeout_cnt;
    }
#endif

    if ((++(*p_to))*DHCP_TMR_INTERVAL >  dhcp_timeout_msec) {
        (*p_to) = 0;

        unsigned int parm;
        if (netif == &wireless_netif) {
            lwip_event_cb(NULL, LWIP_WIRELESS_DHCP_BOUND_TIMEOUT);
            parm = WIFI_NETIF;
        }
#ifdef HAVE_LTE_NETIF
        else if (netif == &lte_netif) {
            lwip_event_cb(NULL, LWIP_LTE_DHCP_BOUND_TIMEOUT);
            parm = LTE_NETIF;
        }
#endif
        else {
            lwip_event_cb(NULL, LWIP_WIRE_DHCP_BOUND_TIMEOUT);
            parm = ETH_NETIF;
        }

#if 1
        parm <<= 8;
        parm |= 1;
        void __lwip_renew(unsigned short parm);
        __lwip_renew(parm);
#else
        tcpip_untimeout((sys_timeout_handler)network_is_dhcp_bound, netif);//减缓network_is_dhcp_bound 刚好被lwip_renew装载的情况
        if (tcpip_timeout(DHCP_TMR_INTERVAL, (sys_timeout_handler)network_is_dhcp_bound, netif) != ERR_OK) {
            LWIP_ASSERT("failed to create timeout network_is_dhcp_bound", 0);
        }
#endif
    } else {
        if (dhcp->state == DHCP_STATE_BOUND) {
            (*p_to) = 0;
            Display_IPAddress();

#if DNS_LOCAL_HOSTLIST
            if (netif == &wireless_netif) {
                dns_local_removehost(LOCAL_WIRELESS_HOST_NAME, &wireless_netif.ip_addr);

                if (dns_local_addhost(LOCAL_WIRELESS_HOST_NAME, &wireless_netif.ip_addr) != ERR_OK) {
                    puts("dns_local_addhost err`.\n");
                }
            }
#ifdef HAVE_LTE_NETIF
            else if (netif == &lte_netif) {
                dns_local_removehost(LOCAL_WIRELESS_HOST_NAME, &lte_netif.ip_addr);

                if (dns_local_addhost(LOCAL_WIRELESS_HOST_NAME, &lte_netif.ip_addr) != ERR_OK) {
                    puts("dns_local_addhost err`.\n");
                }
            }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
            else {
                dns_local_removehost(LOCAL_WIRE_HOST_NAME, &wire_netif.ip_addr);

                if (dns_local_addhost(LOCAL_WIRE_HOST_NAME, &wire_netif.ip_addr) != ERR_OK) {
                    puts("dns_local_addhost err`.\n");
                }
            }
#elif defined HAVE_BT_NETIF
            else {
                dns_local_removehost(LOCAL_WIRE_HOST_NAME, &bt_netif.ip_addr);

                if (dns_local_addhost(LOCAL_WIRE_HOST_NAME, &bt_netif.ip_addr) != ERR_OK) {
                    puts("dns_local_addhost err`.\n");
                }
            }
#endif
#endif

            if (netif == &wireless_netif) {
                lwip_event_cb(NULL, LWIP_WIRELESS_DHCP_BOUND_SUCC);
            }
#ifdef HAVE_LTE_NETIF
            else if (netif == &lte_netif) {
                lwip_event_cb(NULL, LWIP_LTE_DHCP_BOUND_SUCC);
            }
#endif
            else {
                lwip_event_cb(NULL, LWIP_WIRE_DHCP_BOUND_SUCC);
            }
            if (ntp_get_time_init && !get_time_init) {
                get_time_init = 1;
                thread_fork("ntp_client_get_time", 10, 1024, 0, 0, ntp_client_get_time, NULL);
            }
        } else {
            tcpip_untimeout((sys_timeout_handler)network_is_dhcp_bound, netif);//减缓network_is_dhcp_bound 刚好被lwip_renew装载的情况
            if (tcpip_timeout(DHCP_TMR_INTERVAL, (sys_timeout_handler)network_is_dhcp_bound, netif) != ERR_OK) {
                LWIP_ASSERT("failed to create timeout network_is_dhcp_bound", 0);
            }
        }
    }
}

/**
 * @brief Get_IPAddress
 *
 */
void Get_IPAddress(u8_t lwip_netif, char *ipaddr)
{
    if (lwip_netif == WIFI_NETIF) {
        inet_ntoa_r(*((ip4_addr_t *) & (wireless_netif.ip_addr)), ipaddr, IP4ADDR_STRLEN_MAX);
    }
#ifdef HAVE_LTE_NETIF
    else if (lwip_netif == LTE_NETIF) {
        inet_ntoa_r(*((ip4_addr_t *) & (lte_netif.ip_addr)), ipaddr, IP4ADDR_STRLEN_MAX);
    }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
    else {
        inet_ntoa_r(*((ip4_addr_t *) & (wire_netif.ip_addr)), ipaddr, IP4ADDR_STRLEN_MAX);
    }
#elif defined HAVE_BT_NETIF
    else {
        inet_ntoa_r(*((ip4_addr_t *) & (bt_netif.ip_addr)), ipaddr, IP4ADDR_STRLEN_MAX);
    }
#endif

    /* printf("Get_IPAddress: %s\n", ipaddr); */
}

void get_gateway(u8_t lwip_netif, char *ipaddr)
{
    if (lwip_netif == WIFI_NETIF) {
        inet_ntoa_r(*((ip4_addr_t *) & (wireless_netif.gw)), ipaddr, IP4ADDR_STRLEN_MAX);
    }
#ifdef HAVE_LTE_NETIF
    else if (lwip_netif == LTE_NETIF) {
        inet_ntoa_r(*((ip4_addr_t *) & (lte_netif.gw)), ipaddr, IP4ADDR_STRLEN_MAX);
    }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
    else {
        inet_ntoa_r(*((ip4_addr_t *) & (wire_netif.gw)), ipaddr, IP4ADDR_STRLEN_MAX);
    }
#elif defined HAVE_BT_NETIF
    else {
        inet_ntoa_r(*((ip4_addr_t *) & (bt_netif.gw)), ipaddr, IP4ADDR_STRLEN_MAX);
    }
#endif
}

int get_netif_macaddr_and_ipaddr(ip4_addr_t *ipaddr, unsigned char *mac_addr, int count)
{
    struct netif *netif = netif_list;
    int i;
    unsigned char (*hwaddr)[6] = (unsigned char (*)[6])mac_addr;
    int p_ipaddr;

    for (i = 0; i < count && netif != NULL; i++, netif = netif->next) {
        memcpy(ipaddr, &netif->ip_addr, sizeof(ip4_addr_t));
        memcpy(hwaddr, netif->hwaddr, NETIF_MAX_HWADDR_LEN);
        p_ipaddr = ipaddr->addr;
        printf("|get_netif_macaddr_and_ipaddr-> [0x%x],[%x%x%x%x%x%x]\n",
               p_ipaddr, netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2], netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);

        ++ipaddr;
        ++hwaddr;
    }

    return i;
}

void set_wireless_netif_macaddr(const char *mac_addr)
{
    memcpy(wireless_netif.hwaddr, mac_addr, NETIF_MAX_HWADDR_LEN);
}

static void dhcp_renew_ipaddr(struct netif *netif)
{
    struct dhcp *dhcp = ((struct dhcp *)netif_get_client_data(netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP));

    ip_addr_set_zero(&netif->ip_addr);
    ip_addr_set_zero(&netif->netmask);
    ip_addr_set_zero(&netif->gw);

    //dhcp发送和接收需要放在同一线程里做
    if (tcpip_callback((tcpip_callback_fn)dhcp_start, netif) != ERR_OK) {
        LWIP_ASSERT("failed to create timeout dhcp_start", 0);
    }
#if 0
    if (tcpip_callback((tcpip_callback_fn)dhcp_network_changed, netif) != ERR_OK) {
        LWIP_ASSERT("failed to create timeout dhcp_network_changed", 0);
    }
#endif
}

/**
 * @brief TcpipInitDone wait for tcpip init being done
 *
 * @param arg the semaphore to be signaled
 */
static void TcpipInitDone(void *arg)
{
    sys_sem_t *sem;

    sem = arg;
    sys_sem_signal(sem);
}


void lwip_netif_set_up(u8_t lwip_netif)
{
    if (lwip_netif == WIFI_NETIF) {
        netif_set_up(&wireless_netif); //使能网卡
    }
#ifdef HAVE_LTE_NETIF
    else if (lwip_netif == LTE_NETIF) {
        netif_set_up(&lte_netif);      //使能网卡
    }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
    else {
        netif_set_up(&wire_netif);     //使能网卡
    }
#elif defined HAVE_BT_NETIF
    else {
        netif_set_up(&bt_netif);       //使能网卡
    }
#endif
}

void __lwip_renew(unsigned short parm)
{
    struct ip4_addr ipaddr;
    struct ip4_addr netmask;
    struct ip4_addr gw;
    u8_t lwip_netif = parm >> 8;
    u8_t dhcp = (u8_t)parm;

    printf("|lwip_renew [%d][%d].\n", lwip_netif, dhcp);

    if (lwip_netif == WIFI_NETIF) {
        int wifi_get_mac(u8 * mac);
        wifi_get_mac(wireless_netif.hwaddr);
        if (dhcp) {
            dhcp_renew_ipaddr(&wireless_netif);

            tcpip_untimeout((sys_timeout_handler)network_is_dhcp_bound, &wireless_netif);

            wireless_dhcp_timeout_cnt = 0;

            if (tcpip_timeout(DHCP_TMR_INTERVAL, (sys_timeout_handler)network_is_dhcp_bound, &wireless_netif) != ERR_OK) {
                LWIP_ASSERT("failed to create timeout network_is_dhcp_bound", 0);
            }
        } else {
            tcpip_untimeout((sys_timeout_handler)network_is_dhcp_bound, &wireless_netif);
            IP4_ADDR(&ipaddr, lan_setting_info.WIRELESS_IP_ADDR0, lan_setting_info.WIRELESS_IP_ADDR1, lan_setting_info.WIRELESS_IP_ADDR2, lan_setting_info.WIRELESS_IP_ADDR3);
            IP4_ADDR(&netmask, lan_setting_info.WIRELESS_NETMASK0, lan_setting_info.WIRELESS_NETMASK1, lan_setting_info.WIRELESS_NETMASK2, lan_setting_info.WIRELESS_NETMASK3);
            IP4_ADDR(&gw, lan_setting_info.WIRELESS_GATEWAY0, lan_setting_info.WIRELESS_GATEWAY1, lan_setting_info.WIRELESS_GATEWAY2, lan_setting_info.WIRELESS_GATEWAY3);
            netif_set_addr(&wireless_netif, &ipaddr, &netmask, &gw);

            lwip_event_cb(NULL, LWIP_WIRELESS_DHCP_BOUND_SUCC);
            Display_IPAddress();

            if (IPV4_ADDR_CONFLICT_DETECT) {
                if ((etharp_request(&wireless_netif, &ipaddr)) != ERR_OK) {
                    printf("etharp_request failed!");
                }
            }
            struct wifi_mode_info info;
            info.mode = NONE_MODE;
            wifi_get_mode_cur_info(&info);
            if (info.mode == STA_MODE) {
                lwip_static_ip_renew = 1;
            }
            if (ntp_get_time_init && !get_time_init && info.mode == STA_MODE) {
                get_time_init = 1;
                thread_fork("ntp_client_get_time", 10, 1024, 0, 0, ntp_client_get_time, NULL);
            }
        }
    }
#ifdef HAVE_LTE_NETIF
    else if (lwip_netif == LTE_NETIF) {
        u8 *lte_module_get_mac_addr(void);
        memcpy(lte_netif.hwaddr, lte_module_get_mac_addr, 6);
        if (dhcp) {
            dhcp_renew_ipaddr(&lte_netif);

            tcpip_untimeout((sys_timeout_handler)network_is_dhcp_bound, &lte_netif);

            lte_dhcp_timeout_cnt = 0;

            if (tcpip_timeout(DHCP_TMR_INTERVAL, (sys_timeout_handler)network_is_dhcp_bound, &lte_netif) != ERR_OK) {
                LWIP_ASSERT("failed to create timeout network_is_dhcp_bound", 0);
            }
        }
    }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
    else if (lwip_netif == ETH_NETIF) {
        dhcp_renew_ipaddr(&wire_netif);
        wire_dhcp_timeout_cnt = 0;
        tcpip_untimeout((sys_timeout_handler)network_is_dhcp_bound, &wire_netif);

        if (tcpip_timeout(DHCP_TMR_INTERVAL, (sys_timeout_handler)network_is_dhcp_bound, &wire_netif) != ERR_OK) {
            LWIP_ASSERT("failed to create timeout network_is_dhcp_bound", 0);
        }

    }
#elif defined HAVE_BT_NETIF
    else if (lwip_netif == BT_NETIF) {
        dhcp_renew_ipaddr(&bt_netif);
        bt_dhcp_timeout_cnt = 0;
        tcpip_untimeout((sys_timeout_handler)network_is_dhcp_bound, &bt_netif);

        if (tcpip_timeout(DHCP_TMR_INTERVAL, (sys_timeout_handler)network_is_dhcp_bound, &bt_netif) != ERR_OK) {
            LWIP_ASSERT("failed to create timeout network_is_dhcp_bound", 0);
        }

    }
#endif
}

void lwip_renew(u8_t lwip_netif, u8_t dhcp)
{
    int err;
    unsigned int parm;
    parm = lwip_netif;
    parm <<= 8;
    parm |= dhcp;
    err = tcpip_callback((tcpip_callback_fn)__lwip_renew, (void *)parm);
    LWIP_ASSERT("failed to lwip_renew tcpip_callback", err == 0);
}

void lwip_dhcp_release_and_stop(u8_t lwip_netif)
{
    lwip_static_ip_renew = 0;
    int err;
    err = tcpip_callback((tcpip_callback_fn)dhcp_release_and_stop, (void *)&wireless_netif);
    LWIP_ASSERT("failed to dhcp_release_and_stop tcpip_callback", err == 0);
}

/**
* @brief Init_LwIP initialize the LwIP
*/
/*
 * 函数名：LWIP_Init
 * 描述  ：初始化LWIP协议栈，主要是把网卡与LWIP连接起来。
 			包括IP、MAC地址，接口函数
 * 输入  ：是否使用DHCP获得IP地址
 * 输出  : 无
 * 调用  ：外部调用
 */
void Init_LwIP(u8_t lwip_netif)
{
    err_t err = -1;
    sys_sem_t sem;
    struct ip4_addr ipaddr;
    struct ip4_addr netmask;
    struct ip4_addr gw;

    char host_name[32] = {0};
    struct netif *netif = NULL;
    netif_init_fn ethernetif_init = NULL;

    switch (lwip_netif) {
#ifdef HAVE_ETH_WIRE_NETIF
    case ETH_NETIF:
        netif = &wire_netif;
        ethernetif_init = wired_ethernetif_init;
        sprintf(host_name, "%s", LOCAL_WIRE_HOST_NAME);
        break;
#endif

    case WIFI_NETIF:
        netif = &wireless_netif;
        ethernetif_init = wireless_ethernetif_init;
        sprintf(host_name, "%s", LOCAL_WIRELESS_HOST_NAME);
        break;

#ifdef HAVE_BT_NETIF
    case BT_NETIF:
        netif = &bt_netif;
        ethernetif_init = bt_ethernetif_init;
        sprintf(host_name, "%s", LOCAL_WIRE_HOST_NAME);
        break;
#endif

#ifdef HAVE_LTE_NETIF
    case LTE_NETIF:
        netif = &lte_netif;
        ethernetif_init = lte_ethernetif_init;
        sprintf(host_name, "%s", LOCAL_WIRELESS_HOST_NAME);
        break;
#endif

    default:
        printf("no support netif = %d\n", lwip_netif);
        return;
    }

    //lwip_spin_init();
    if (netif->input) {
        return;
    }

    printf("|Init_LwIP [%d]\n", lwip_netif);

    if (sys_sem_new(&sem, 0) != ERR_OK) {
        LWIP_ASSERT("failed to create sem", 0);
    }

    tcpip_init(TcpipInitDone, &sem);
    sys_sem_wait(&sem);
    sys_sem_free(&sem);
    LWIP_DEBUGF(TCPIP_DEBUG, ("tcpip_init: initialized\n"));

    IP4_ADDR(&ipaddr, lan_setting_info.WIRELESS_IP_ADDR0, lan_setting_info.WIRELESS_IP_ADDR1, lan_setting_info.WIRELESS_IP_ADDR2, lan_setting_info.WIRELESS_IP_ADDR3);
    IP4_ADDR(&netmask, lan_setting_info.WIRELESS_NETMASK0, lan_setting_info.WIRELESS_NETMASK1, lan_setting_info.WIRELESS_NETMASK2, lan_setting_info.WIRELESS_NETMASK3);
    IP4_ADDR(&gw, lan_setting_info.WIRELESS_GATEWAY0, lan_setting_info.WIRELESS_GATEWAY1, lan_setting_info.WIRELESS_GATEWAY2, lan_setting_info.WIRELESS_GATEWAY3);

    /*初始化网卡与LWIP的接口，参数为网络接口结构体、ip地址、
    子网掩码、网关、网卡信息指针、初始化函数、输入函数*/
    netif_add(netif, &ipaddr, &netmask, &gw, NULL, ethernetif_init, (netif_input_fn)&tcpip_input);

#if DNS_LOCAL_HOSTLIST
    err = dns_local_addhost(host_name, &netif->ip_addr);
    if (err != ERR_OK) {
        printf("dns_local_addhost err = %d\n", err);
    }
#endif
    /*  When the dev_netif is fully configured this function must be called.*/

    /*设置默认网卡 .*/
    netif_set_default(netif);

    /*设置多播网卡*/
    //ip4_set_default_multicast_netif(&wire_netif);
    /*-------------------------------------------------------------------------------------------------*/


    Display_IPAddress();
}


#ifdef HAVE_ETH_WIRE_NETIF
/*
 * 有线网络事件处理
 */
static void wired_event_handler(struct sys_event *event)
{
    switch (event->u.dev.event) {
    case DEVICE_EVENT_IN:
        if (use_dhcp) {
            Init_LwIP(ETH_NETIF, 1);
        }
        break;
    case DEVICE_EVENT_OUT:
        break;
    }
}

static void device_event_handler3(struct sys_event *e)
{
    struct device_event *event = (struct device_event *)e->payload;
    if (!ASCII_StrCmp(event->arg, "eth0", 4)) {
        wired_event_handler(event);
    }
}
/*
 * 静态注册设备事件回调函数，优先级为0
 */
SYS_EVENT_STATIC_HANDLER_REGISTER(net_event) = {
    .event_type     = SYS_DEVICE_EVENT,
    .prob_handler   = device_event_handler3,
    .post_handler   = NULL,
};
#endif


/*The gethostname function retrieves the standard host name for the local computer.*/
int gethostname(char *name, int namelen)
{
    if (wireless_netif.flags) {
        strncpy(name, LOCAL_WIRELESS_HOST_NAME, namelen);
    }
#ifdef HAVE_LTE_NETIF
    else if (lte_netif.flags) {
        strncpy(name, LOCAL_WIRELESS_HOST_NAME, namelen);
    }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
    else if (wire_netif.flags) {
        strncpy(name, LOCAL_WIRE_HOST_NAME, namelen);
    }
#elif defined HAVE_BT_NETIF
    else if (bt_netif.flags) {
        strncpy(name, LOCAL_WIRE_HOST_NAME, namelen);
    }
#endif

    name[namelen - 1] = 0;

    return 0;
}

int getdomainname(char *name, int namelen)
{
    if (wireless_netif.flags) {
        strncpy(name, "www."LOCAL_WIRELESS_HOST_NAME".com", namelen);
    }
#ifdef HAVE_LTE_NETIF
    else if (lte_netif.flags) {
        strncpy(name, "www."LOCAL_WIRELESS_HOST_NAME".com", namelen);
    }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
    else if (wire_netif.flags) {
        strncpy(name, "www."LOCAL_WIRE_HOST_NAME".com", namelen);
    }
#elif defined HAVE_BT_NETIF
    else if (bt_netif.flags) {
        strncpy(name, "www."LOCAL_WIRE_HOST_NAME".com", namelen);
    }
#endif

    name[namelen - 1] = 0;

    return 0;
}


u32_t sys_now(void)
{
    return OSGetTime() * 10;
}
/**
  * @brief  Called when a frame is received
  * @param  None
  * @retval None
  */
void LwIP_Pkt_Handle(void)
{
}

int getnameinfo(const struct sockaddr *sa, int salen, char *host, int hostlen, char *serv, int servlen, int flags)
{
    struct sockaddr_in *sa_in = (struct sockaddr_in *)sa;

    if ((flags & NI_NUMERICHOST) && host) {
        if (sa_in->sin_addr.s_addr == 0) {
            Get_IPAddress(1, host);
        } else {
            inet_ntoa_r(sa_in->sin_addr.s_addr, host, hostlen);
        }
    }

    if ((flags & NI_NUMERICSERV) && serv) {
        itoa(htons(sa_in->sin_port), serv, 10);
        /* sprintf(serv, "%s", htons(sa_in->sin_port)); */
        serv[servlen - 1] = '\0';
    }

    return 0;
}

char *strerror(int errnum)
{
    return "not_define_strerror";
}

#ifdef LWIP_HOOK_IP4_ROUTE_SRC
struct netif *
ip4_route2(const ip4_addr_t *src, const ip4_addr_t *dest)
{

//    printf("222  src 0x%x  dest 0x%x \n\n",src,dest);
    if (src == NULL) {
        printf("src is NULL\n\n");
        return NULL;
    }

    if (dest == NULL) {
        printf("dest is NULl \n\n");
        return NULL;
    }

    struct netif *netif;

#if LWIP_MULTICAST_TX_OPTIONS

    /* Use administratively selected interface for multicast by default */
    if (ip4_addr_ismulticast(dest) && ip4_default_multicast_netif) {
        return ip4_default_multicast_netif;
    }

#endif /* LWIP_MULTICAST_TX_OPTIONS */

    /* iterate through netifs */


    for (netif = netif_list; netif != NULL; netif = netif->next) {
//      printf("%s   ::::: %d \n\n", __func__, __LINE__);

        /* is the netif up, does it have a link and a valid address? */
        if (netif_is_up(netif) && netif_is_link_up(netif) && !ip4_addr_isany_val(*netif_ip4_addr(netif))) {
            //          printf("%s   ::::: %d \n\n", __func__, __LINE__);
            /* network mask matches? */
            /*           printf("src %s \n\n", inet_ntoa(*src));                     */
            /* printf("dest %s \n\n", inet_ntoa(*dest));                           */
            /* printf("netif %s \n\n", inet_ntoa(*netif_ip4_addr(netif)));         */


            //这里拿源来匹配
            //在之前的函数中，目标地址如果不在同一网关中时，往下走，会使用默认网卡。
            if (ip_addr_isany_val(*src)) {
                if (ip4_addr_netcmp(dest, netif_ip4_addr(netif), netif_ip4_netmask(netif))) {
                    /*         printf("src %s \n\n", inet_ntoa(*src));                     */
                    /* printf("dest %s \n\n", inet_ntoa(*dest));                           */
                    /* printf("netif %s \n\n", inet_ntoa(*netif_ip4_addr(netif)));         */

                    //                printf("%s   ::::: %d \n\n", __func__, __LINE__);
                    /* return netif on which to forward IP packet                        */
                    return netif;
                }

            } else {
                if (ip4_addr_cmp(src, netif_ip4_addr(netif))) {
                    //              printf("%s   ::::: %d \n\n", __func__, __LINE__);
                    /* return netif on which to forward IP packet */
                    return netif;

                }

            }

            /* gateway matches on a non broadcast interface? (i.e. peer in a point to point interface) */
            if (((netif->flags & NETIF_FLAG_BROADCAST) == 0) && ip4_addr_cmp(dest, netif_ip4_gw(netif))) {
                /* return netif on which to forward IP packet */
                return netif;
            }
        }
    }

#if LWIP_NETIF_LOOPBACK && !LWIP_HAVE_LOOPIF

    /* loopif is disabled, looopback traffic is passed through any netif */
    if (ip4_addr_isloopback(dest)) {
        /* don't check for link on loopback traffic */
        if (netif_is_up(netif_default)) {
            return netif_default;
        }

        /* default netif is not up, just use any netif for loopback traffic */
        for (netif = netif_list; netif != NULL; netif = netif->next) {
            if (netif_is_up(netif)) {
                return netif;
            }
        }

        return NULL;
    }

#endif /* LWIP_NETIF_LOOPBACK && !LWIP_HAVE_LOOPIF */

//造成递归
    /* #if 0                                            */
    /* #ifdef LWIP_HOOK_IP4_ROUTE_SRC                   */

    /*     netif = LWIP_HOOK_IP4_ROUTE_SRC(dest, NULL); */

    /*     if(netif != NULL) {                          */
    /*         return netif;                            */
    /*     }                                            */

    /* #elif defined(LWIP_HOOK_IP4_ROUTE)               */
    /*     netif = LWIP_HOOK_IP4_ROUTE(dest);           */

    /*     if(netif != NULL) {                          */
    /*         return netif;                            */
    /*     }                                            */

    /* #endif                                           */
    /* #endif                                           */

    if ((netif_default == NULL) || !netif_is_up(netif_default) || !netif_is_link_up(netif_default) ||
        ip4_addr_isany_val(*netif_ip4_addr(netif_default))) {
        /* No matching netif found and default netif is not usable.
           If this is not good enough for you, use LWIP_HOOK_IP4_ROUTE() */
        LWIP_DEBUGF(IP_DEBUG | LWIP_DBG_LEVEL_SERIOUS, ("ip4_route: No route to %"U16_F".%"U16_F".%"U16_F".%"U16_F"\n",
                    ip4_addr1_16(dest), ip4_addr2_16(dest), ip4_addr3_16(dest), ip4_addr4_16(dest)));
        IP_STATS_INC(ip.rterr);
        MIB2_STATS_INC(mib2.ipoutnoroutes);
        return NULL;
    }

    return netif_default;
}
#endif


int lwip_get_dest_hwaddr(u8_t lwip_netif, ip4_addr_t *ipaddr, struct eth_addr *dhwaddr)
{
    int ret = -1;
    struct eth_addr *eth_ret;
    ip4_addr_t *ipaddr_ret;

    if (lwip_netif == WIFI_NETIF) {
        ret = etharp_find_addr(&wireless_netif, ipaddr, &eth_ret, (const ip4_addr_t **)(&ipaddr_ret));
    }
#ifdef HAVE_LTE_NETIF
    else if (lwip_netif == LTE_NETIF) {
        ret = etharp_find_addr(&lte_netif, ipaddr, &eth_ret, (const ip4_addr_t **)(&ipaddr_ret));
    }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
    else {
        ret = etharp_find_addr(&wire_netif, ipaddr, &eth_ret, (const ip4_addr_t **)(&ipaddr_ret));
    }
#elif defined  HAVE_BT_NETIF
    else {
        ret = etharp_find_addr(&bt_netif, ipaddr, &eth_ret, (const ip4_addr_t **)(&ipaddr_ret));
    }
#endif

    if (ret != -1) {
        memcpy(dhwaddr, eth_ret, sizeof(struct eth_addr));
    } else {
        puts("lwip_get_dest_hwaddr fail!\r\n");
    }
    return (ret == -1) ? -1 : 0;
}


void lwip_get_netif_info(u8_t lwip_netif, struct netif_info *info)
{
    if (lwip_netif == WIFI_NETIF) {
#if LWIP_IPV4 && LWIP_IPV6
        info->ip = (u32)wireless_netif.ip_addr.u_addr.ip4.addr;
        info->gw = (u32)wireless_netif.gw.u_addr.ip4.addr;
        info->netmask = (u32)wireless_netif.netmask.u_addr.ip4.addr;
#elif LWIP_IPV4
        info->ip = (u32)wireless_netif.ip_addr.addr;
        info->gw = (u32)wireless_netif.gw.addr;
        info->netmask = (u32)wireless_netif.netmask.addr;
#elif LWIP_IPV6
#endif
    }
#ifdef HAVE_LTE_NETIF
    else if (lwip_netif == LTE_NETIF) {
        info->ip = (u32)lte_netif.ip_addr.addr;
        info->gw = (u32)lte_netif.gw.addr;
        info->netmask = (u32)lte_netif.netmask.addr;
    }
#endif
#ifdef HAVE_ETH_WIRE_NETIF
    else {
        info->ip = (u32)wire_netif.ip_addr.addr;
        info->gw = (u32)wire_netif.gw.addr;
        info->netmask = (u32)wire_netif.netmask.addr;
    }
#elif defined HAVE_BT_NETIF
    else {
        info->ip = (u32)bt_netif.ip_addr.addr;
        info->gw = (u32)bt_netif.gw.addr;
        info->netmask = (u32)bt_netif.netmask.addr;
    }
#endif
}

int lwip_dhcp_bound(void)
{
    struct netif *wireless_netif = netif_find("wl0");
    if (wireless_netif == NULL) { //lwip 未初始化
        return 0;
    }

    if (lwip_static_ip_renew) { //静态IP情况下,可以直接通信
        return 1;
    }

    struct dhcp *dhcp = (struct dhcp *)netif_get_client_data(wireless_netif, LWIP_NETIF_CLIENT_DATA_INDEX_DHCP);
    if (dhcp == NULL) { //dhcp 未初始化
        return 0;
    }

    if (dhcp->state == DHCP_STATE_BOUND) {
        return 1;
    }

    return 0;
}
