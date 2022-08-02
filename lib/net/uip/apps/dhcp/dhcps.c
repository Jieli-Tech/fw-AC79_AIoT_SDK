#include "string.h"
#include "printf.h"
#include <stdlib.h>
#include "src/clock.h"
#include "pt.h"
#include "os/os_api.h"
#include "dhcps.h"

static struct lan_setting lan_setting_info = {
    .WIRELESS_IP_ADDR0  = 192,
    .WIRELESS_IP_ADDR1  = 168,
    .WIRELESS_IP_ADDR2  = 1,
    .WIRELESS_IP_ADDR3  = 1,

    .WIRELESS_NETMASK0  = 255,
    .WIRELESS_NETMASK1  = 255,
    .WIRELESS_NETMASK2  = 255,
    .WIRELESS_NETMASK3  = 0,

    .WIRELESS_GATEWAY0  = 192,
    .WIRELESS_GATEWAY1  = 168,
    .WIRELESS_GATEWAY2  = 1,
    .WIRELESS_GATEWAY3  = 1,

    .SERVER_IPADDR1  = 192,
    .SERVER_IPADDR2  = 168,
    .SERVER_IPADDR3  = 1,
    .SERVER_IPADDR4  = 1,

    .CLIENT_IPADDR1  = 192,
    .CLIENT_IPADDR2  = 168,
    .CLIENT_IPADDR3  = 1,
    .CLIENT_IPADDR4  = 2,

    .SUB_NET_MASK1   = 255,
    .SUB_NET_MASK2   = 255,
    .SUB_NET_MASK3   = 255,
    .SUB_NET_MASK4   = 0,
};

struct lan_setting *net_get_lan_info(void)
{
    return &lan_setting_info;
}

int net_set_lan_info(struct lan_setting *__lan_setting_info)
{
    printf("\n  value %d  \n", __lan_setting_info->WIRELESS_IP_ADDR0);

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

static uip_ipaddr_t server_address;
static uip_ipaddr_t y_address;//just for test
static uip_ipaddr_t client_address;
static struct _dhcps_cli *dhcps_cli_head;

static u8_t ipaddr_tab[254];

static u8_t is_offer_dns;
static OS_MUTEX dhcps_mtx;

static u8_t *add_msg_type(u8_t *optptr, u8_t type)
{
    DHCP_DBG_FUNC_IN();

    *optptr++ = DHCP_OPTION_MESSAGE_TYPE;
    *optptr++ = 1;
    *optptr++ = type;

    return optptr;
}

static u8_t *add_offer_options(u8_t *optptr)
{
    DHCP_DBG_FUNC_IN();
    struct lan_setting *lan_setting_info;
    lan_setting_info = net_get_lan_info();

    *optptr++ = DHCP_OPTION_SERVER_ID;
    *optptr++ = 4;  //len
    *optptr++ = ip4_addr1(&server_address);
    *optptr++ = ip4_addr2(&server_address);
    *optptr++ = ip4_addr3(&server_address);
    *optptr++ = ip4_addr4(&server_address);

    *optptr++ = DHCP_OPTION_LEASE_TIME;
    *optptr++ = 4;  //len
    *optptr++ = (u8_t)(LEASE_TIME >> 24);
    *optptr++ = (u8_t)(LEASE_TIME >> 16);
    *optptr++ = (u8_t)(LEASE_TIME >> 8);
    *optptr++ = (u8_t)LEASE_TIME;

    *optptr++ = DHCP_OPTION_SUBNET_MASK;
    *optptr++ = 4;  //len
    *optptr++ = lan_setting_info->SUB_NET_MASK1;
    *optptr++ = lan_setting_info->SUB_NET_MASK2;	//note this is different from interface netmask and broadcast address
    *optptr++ = lan_setting_info->SUB_NET_MASK3;
    *optptr++ = lan_setting_info->SUB_NET_MASK4;

    *optptr++ = DHCP_OPTION_ROUTER;
    *optptr++ = 4;  //len
    *optptr++ = ip4_addr1(&server_address);
    *optptr++ = ip4_addr2(&server_address);
    *optptr++ = ip4_addr3(&server_address);
    *optptr++ = ip4_addr4(&server_address);

    if (is_offer_dns) {
//ios 上网
//ios 上网
        *optptr++ = DHCP_OPTION_DNS_SERVER;
        *optptr++ = 4;  //len
        *optptr++ = ip4_addr1(&server_address);
        *optptr++ = ip4_addr2(&server_address);
        *optptr++ = ip4_addr3(&server_address);
        *optptr++ = ip4_addr4(&server_address);
    }

    return optptr;
}

static u8_t *add_end(u8_t *optptr)
{
    DHCP_DBG_FUNC_IN();

    *optptr++ = DHCP_OPTION_END;

    return optptr;
}

static void memcpy_aligne(char *dst0, const char *src0, u32_t len0)
{
    while (len0--) {
        *dst0++ = *src0++;
    }
}

static void create_msg(struct dhcp_msg *m)
{
    DHCP_DBG_FUNC_IN();

    m->op = DHCP_BOOTREPLY;
    m->htype = DHCP_HTYPE_ETH;
    m->hlen = 6;  //mac id length
    m->hops = 0;
    m->secs = 0;

    memset((char *) &m->ciaddr.addr, 0, sizeof(m->ciaddr.addr));

    memcpy_aligne((char *) &m->yiaddr.addr, (char *) &client_address, sizeof(m->yiaddr.addr));

    memcpy_aligne((char *) &m->siaddr.addr, (char *) &server_address, sizeof(m->siaddr.addr));
    memset((char *) &m->giaddr.addr, 0, sizeof(m->giaddr.addr));
    memset((char *) &m->sname[0], 0, sizeof(m->sname));
    memset((char *) &m->file[0], 0, sizeof(m->file));
    m->cookie = PP_HTONL(DHCP_MAGIC_COOKIE);
    memset((char *) &m->options[0], 0, sizeof(m->options));
}

static void send_offer(void *p)
{
    DHCP_DBG_FUNC_IN();

    struct lan_setting *lan_setting_info;
    lan_setting_info = net_get_lan_info();
    struct _dhcps_cli *cli, *last_cli;
    struct dhcp_msg *m = (struct dhcp_msg *) p;
    u8_t *end;
    u8_t *ptr;
    u8_t ipaddr_cnt;

    for (last_cli = cli = dhcps_cli_head; cli != NULL; cli = cli->next) {
        last_cli = cli;
        if (!memcmp(cli->cli_mac, m->chaddr, 6)) {
            break;
        }
    }

    if (cli) {
        memcpy(&client_address, &cli->ipaddr, sizeof(client_address));
        cli->timeout = LEASE_TIME;

        printf("old client find ,mac_addr = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
               cli->cli_mac[0], cli->cli_mac[1], cli->cli_mac[2],
               cli->cli_mac[3], cli->cli_mac[4], cli->cli_mac[5]);

        printf("alloc ipaddr :%d.%d.%d.%d\n",
               ip4_addr1(&cli->ipaddr), ip4_addr2(&cli->ipaddr),
               ip4_addr3(&cli->ipaddr), ip4_addr4(&cli->ipaddr));
    } else {
        cli = (struct _dhcps_cli *)malloc(sizeof(struct _dhcps_cli));
        if (cli == NULL) {
            printf("_dhcps_cli malloc err!!!");
            return;
        }

        memset(cli, 0, sizeof(struct _dhcps_cli));
        memcpy(cli->cli_mac, m->chaddr, 6);

        cli->timeout = LEASE_TIME;
        cli->next = NULL;

        for (ptr = ipaddr_tab, ipaddr_cnt = 0; ipaddr_cnt < sizeof(ipaddr_tab) - lan_setting_info->CLIENT_IPADDR4; ++ptr, ++ipaddr_cnt) {
            if (*ptr == 0) {
                *ptr = 1;
                break;
            }
        }

        if (ipaddr_cnt >= sizeof(ipaddr_tab) - lan_setting_info->CLIENT_IPADDR4) {
            ipaddr_cnt = 0;
            printf("no avail ipaddr \n!");
        }

        uip_ipaddr(&client_address, lan_setting_info->CLIENT_IPADDR1, lan_setting_info->CLIENT_IPADDR2, lan_setting_info->CLIENT_IPADDR3, ipaddr_cnt + lan_setting_info->CLIENT_IPADDR4);
        memcpy(&cli->ipaddr, &client_address, sizeof(client_address));

        if (dhcps_cli_head == NULL) {
            dhcps_cli_head = cli;
        } else {
            last_cli->next = cli;
        }

        printf("client not find, new mac_addr = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
               cli->cli_mac[0], cli->cli_mac[1], cli->cli_mac[2],
               cli->cli_mac[3], cli->cli_mac[4], cli->cli_mac[5]);

        printf("alloc new ipaddr :%d.%d.%d.%d\n",
               ip4_addr1(&cli->ipaddr), ip4_addr2(&cli->ipaddr),
               ip4_addr3(&cli->ipaddr), ip4_addr4(&cli->ipaddr));
    }

    create_msg(m);

    end = add_msg_type(m->options, DHCP_OFFER);
    end = add_offer_options(end);
    add_end(end);

    /*fixme: (end - (u8_t *)p)采用这种方式计算数据包大小*/
    uip_send(p, end - (u8_t *)p);
}

static void send_nak(void *p)
{
    DHCP_DBG_FUNC_IN();

    struct dhcp_msg *m = (struct dhcp_msg *) p;
    u8_t *end;

    create_msg(m);

    end = add_msg_type(m->options, DHCP_NAK);
    end = add_end(end);

    /*fixme: (end - (u8_t *)p)采用这种方式计算数据包大小*/
    uip_send(p, end - (u8_t *)p);
}

static void send_ack(void *p)
{
    DHCP_DBG_FUNC_IN();

    struct dhcp_msg *m = (struct dhcp_msg *) p;
    u8_t *end;

    create_msg(m);

    end = add_msg_type(m->options, DHCP_ACK);
    end = add_offer_options(end);
    end = add_end(end);

    /*fixme: (end - (u8_t *)p)采用这种方式计算数据包大小*/
    uip_send(p, end - (u8_t *)p);
}

static u8_t parse_options(struct dhcp_msg *m, s16_t len)
{
    DHCP_DBG_FUNC_IN();

    u8_t dhcps_state;
    u8_t *optptr = m->options;
    u8_t *end = optptr + len;
    s16_t type = 0;
    struct _dhcps_cli *cli;

    dhcps_state = DHCPS_STATE_IDLE;

    while (optptr < end) {
        switch ((s16_t) *optptr) {

        case DHCP_OPTION_MESSAGE_TYPE:
            type = *(optptr + 2);
            break;

        case DHCP_OPTION_REQUESTED_IP	:

            DHCP_DBG_PRINTF("+parse_options: DHCP_OPTION_REQUESTED_IP:\n"\
                            "m->chaddr = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n", \
                            m->chaddr[0], m->chaddr[1], m->chaddr[2], m->chaddr[3], m->chaddr[4], m->chaddr[5]);

            for (cli = dhcps_cli_head; cli != NULL; cli = cli->next) {
                if (!memcmp(&cli->cli_mac, m->chaddr, 6)) {
                    printf("+parse_options: cli != NULL -> dhcps_state = DHCPS_STATE_ACK\n");
                    memcpy(&client_address, &cli->ipaddr, sizeof(client_address));
                    cli->timeout = LEASE_TIME;
                    dhcps_state = DHCPS_STATE_ACK;
                    break;
                }
            }
            if (cli == NULL) {
                printf("+parse_options: cli == NULL -> dhcps_state = DHCPS_STATE_NAK\n");
                dhcps_state = DHCPS_STATE_NAK;
            }

            break;

        case DHCP_OPTION_END:
            break;

        default:
            break;
        }
        optptr += optptr[1] + 2;
    }

    switch (type) {
    case	DHCP_DECLINE	:
        dhcps_state = DHCPS_STATE_IDLE;
        break;

    case	DHCP_DISCOVER	:
        dhcps_state = DHCPS_STATE_OFFER;
        break;

    case	DHCP_REQUEST	:
        if (!(dhcps_state == DHCPS_STATE_ACK || dhcps_state == DHCPS_STATE_NAK)) {
            for (cli = dhcps_cli_head; cli != NULL; cli = cli->next) {
                if (!memcmp(&cli->ipaddr, &m->ciaddr, sizeof(uip_ipaddr_t))) {
                    cli->timeout = LEASE_TIME;
                    dhcps_state = DHCPS_STATE_ACK;
                    break;
                }
            }
            if (cli == NULL) {
                dhcps_state = DHCPS_STATE_NAK;
            }
        }
        break;

    case	DHCP_RELEASE	:
        dhcps_state = DHCPS_STATE_IDLE;
        break;

    default:
        break;
    }

    return dhcps_state;
}

static s16_t parse_msg(void *p)
{
    DHCP_DBG_FUNC_IN();

    int len = uip_len;

    struct dhcp_msg *m = (struct dhcp_msg *) p;

    if (m->cookie == PP_HTONL(DHCP_MAGIC_COOKIE)) {
        return parse_options(m, len);
    }

    printf("\n m->cookie == 0x%x---------------parse_msg wrong! dhcp flame error!\n", m->cookie);
    return 0;
}

static void handle_dhcp(void *q)
{
    DHCP_DBG_FUNC_IN();

    if (NULL == q) {
        printf("handle_dhcp err!!");
        return;
    }

    DHCP_DBG_RECV(q, uip_len);

    os_mutex_pend(&dhcps_mtx, 0);

    /*fixme: 解决字节不对齐*/
    char *udp_rx_buf = NULL;
    udp_rx_buf = (char *) malloc(TEMP_BUF_SIZE);
    if (udp_rx_buf == NULL) {
        printf("udp_rx_buf malloc err!!");
        return;
    }

    memset(udp_rx_buf, 0, TEMP_BUF_SIZE);
    memcpy(udp_rx_buf, q, uip_len);

    switch (parse_msg(udp_rx_buf)) {
    case	DHCPS_STATE_OFFER:
        send_offer(udp_rx_buf);
        break;

    case	DHCPS_STATE_ACK	:
        send_ack(udp_rx_buf);
        break;

    case	DHCPS_STATE_NAK	:
        send_nak(udp_rx_buf);
        break;

    default :
        break;
    }

    if (udp_rx_buf) {
        free(udp_rx_buf);
        udp_rx_buf = NULL;
    }

    os_mutex_post(&dhcps_mtx);
}

void dhcps_release_ipaddr(void *hwaddr)
{
    os_mutex_pend(&dhcps_mtx, 0);
    struct lan_setting *lan_setting_info;
    lan_setting_info = net_get_lan_info();
    struct _dhcps_cli *cli, *pre_cli, *next_cli;

    for (cli = dhcps_cli_head; cli != NULL; cli = next_cli) {
        if (memcmp(cli->cli_mac, hwaddr, 6) == 0) {
            DHCP_DBG_PRINTF("delete client mac_addr : 0x%x.0x%x.0x%x.0x%x.0x%x.0x%x\n",
                            cli->cli_mac[0], cli->cli_mac[1], cli->cli_mac[2],
                            cli->cli_mac[3], cli->cli_mac[4], cli->cli_mac[5]);
            DHCP_DBG_PRINTF("delete client_ipaddr :%d.%d.%d.%d\n",
                            ip4_addr1(&cli->ipaddr), ip4_addr2(&cli->ipaddr),
                            ip4_addr3(&cli->ipaddr), ip4_addr4(&cli->ipaddr));

            ipaddr_tab[ip4_addr4(&cli->ipaddr) - lan_setting_info->CLIENT_IPADDR4] = 0;

            if (cli == dhcps_cli_head) {
                dhcps_cli_head = cli->next;
                next_cli = cli->next;
                mem_free(cli);
                cli = NULL;
            } else {
                next_cli = cli->next;
                pre_cli->next = next_cli;
                mem_free(cli);
                cli = pre_cli;
            }
            pre_cli = cli;
        }
    }
    os_mutex_post(&dhcps_mtx);
}

static void dhcps_lease_timer(void *arg)
{
    LWIP_UNUSED_ARG(arg);

    printf("dhcps: dhcps_lease_timer()\n");

    struct lan_setting *lan_setting_info;
    lan_setting_info = net_get_lan_info();

    struct _dhcps_cli *cli, *pre_cli, *next_cli;

    for (cli = dhcps_cli_head; cli != NULL; cli = next_cli) {
        cli->timeout -= LEASE_TMR_INTERVAL / 1000;
        if (cli->timeout < 0) {
            DHCP_DBG_PRINTF("delete client mac_addr : 0x%x.0x%x.0x%x.0x%x.0x%x.0x%x\n",
                            cli->cli_mac[0], cli->cli_mac[1], cli->cli_mac[2],
                            cli->cli_mac[3], cli->cli_mac[4], cli->cli_mac[5]);
            DHCP_DBG_PRINTF("delete client_ipaddr :%d.%d.%d.%d\n",
                            ip4_addr1(&cli->ipaddr), ip4_addr2(&cli->ipaddr),
                            ip4_addr3(&cli->ipaddr), ip4_addr4(&cli->ipaddr));

            ipaddr_tab[ip4_addr4(&cli->ipaddr) - lan_setting_info->CLIENT_IPADDR4] = 0;

            if (cli == dhcps_cli_head) {
                dhcps_cli_head = cli->next;
                next_cli = cli->next;
                free(cli);
                cli = NULL;
            } else {
                next_cli = cli->next;
                pre_cli->next = next_cli;
                free(cli);
                cli = pre_cli;
            }
            pre_cli = cli;
        }
    }

    /* tcpip_timeout(LEASE_TMR_INTERVAL, dhcps_lease_timer, NULL); */
}

void dhcps_appcall(void)
{
    if (uip_newdata()) { //接收到数据
        handle_dhcp(uip_appdata);
    }
}

void dhcps_init(void)
{
    DHCP_DBG_FUNC_IN();

    struct lan_setting *lan_setting_info;
    lan_setting_info = net_get_lan_info();

    dhcps_cli_head = NULL;
    memset(ipaddr_tab, 0, sizeof(ipaddr_tab));

    uip_ipaddr(&server_address, lan_setting_info->SERVER_IPADDR1, lan_setting_info->SERVER_IPADDR2, lan_setting_info->SERVER_IPADDR3, lan_setting_info->SERVER_IPADDR4);
    uip_ipaddr(&y_address, lan_setting_info->WIRELESS_IP_ADDR0, lan_setting_info->WIRELESS_IP_ADDR1, lan_setting_info->WIRELESS_IP_ADDR2, lan_setting_info->WIRELESS_IP_ADDR3); //绑定自己的IP

    void uip_set_host_ip(void);
    uip_set_host_ip();

    uip_ipaddr_t ipaddr;
    static struct uip_udp_conn *conn = 0;

    if (conn != 0) {
        uip_udp_remove(conn);
    }

    uip_ipaddr(&ipaddr, 255, 255, 255, 255);
    conn = uip_udp_new(&ipaddr, HTONS(DHCP_CLIENT_PORT));

    if (conn) {
        uip_udp_bind(conn, HTONS(DHCP_SERVER_PORT));
    }

    /*监听本地67端口*/
    uip_listen(HTONS(DHCP_SERVER_PORT));

    display_IPAddress();

    os_mutex_create(&dhcps_mtx);
}

void dhcps_uninit(void)
{
    puts("dhcps_uninit bug not fix...\n");

    struct _dhcps_cli *cli;

    DHCP_DBG_FUNC_IN();

    for (cli = dhcps_cli_head; cli != NULL; cli = cli->next) {
        printf("dhcps remove mac_addr = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
               cli->cli_mac[0], cli->cli_mac[1], cli->cli_mac[2],
               cli->cli_mac[3], cli->cli_mac[4], cli->cli_mac[5]);

        printf("dhcps remove ipaddr :%d.%d.%d.%d\n",
               ip4_addr1(&cli->ipaddr), ip4_addr2(&cli->ipaddr),
               ip4_addr3(&cli->ipaddr), ip4_addr4(&cli->ipaddr));
        free(cli);
    }

    dhcps_cli_head = NULL;

    os_mutex_del(&dhcps_mtx, 1);
}

void dhcps_offer_dns(void)
{
    is_offer_dns = 1;
}

void uip_set_host_ip(void)
{
    uip_ipaddr_t ipaddr;

    uip_ipaddr(ipaddr, 192, 168, 1, 1);      //配置Ip
    uip_sethostaddr(ipaddr);
    uip_ipaddr(ipaddr, 192, 168, 1, 1);      //配置网关
    uip_setdraddr(ipaddr);
    uip_ipaddr(ipaddr, 255, 255, 255, 0);      //配置子网掩码
    uip_setnetmask(ipaddr);
}

void display_IPAddress(void)
{
    uip_ipaddr_t ipaddr;
    memset(&ipaddr, 0, sizeof(ipaddr));
    uip_gethostaddr(ipaddr);
    printf("ip4 address: %d.%d.%d.%d\n",
           ip4_addr1(&ipaddr), ip4_addr2(&ipaddr),
           ip4_addr3(&ipaddr), ip4_addr4(&ipaddr));

    memset(&ipaddr, 0, sizeof(ipaddr));
    uip_getdraddr(ipaddr);
    printf("gw address: %d.%d.%d.%d\n",
           ip4_addr1(&ipaddr), ip4_addr2(&ipaddr),
           ip4_addr3(&ipaddr), ip4_addr4(&ipaddr));

    memset(&ipaddr, 0, sizeof(ipaddr));
    uip_getnetmask(ipaddr);
    printf("net mask: %d.%d.%d.%d\n",
           ip4_addr1(&ipaddr), ip4_addr2(&ipaddr),
           ip4_addr3(&ipaddr), ip4_addr4(&ipaddr));
}

