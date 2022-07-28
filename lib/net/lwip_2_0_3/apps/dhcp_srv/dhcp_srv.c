#include "lwip/inet.h"
#include "lwip/pbuf.h"
#include "lwip/mem.h"
#include "lwip/ip.h"
#include "lwip/prot/dhcp.h"
#include "dhcp_srv.h"
#include "string.h"
#include "printf.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "lwip.h"

#define DHCP_DBG_PRINTF             //printf
#define DHCP_DBG_FUNC_IN()          //printf("\n\n+In func:%s \n",__FUNCTION__)
#define DHCP_DBG_SEND(p,len)        //do{printf("\n\n%s send:",__FUNCTION__);put_buf((p),(len));puts("\n");}while(0)
#define DHCP_DBG_RECV(p,len)        //do{printf("\n\n%s recv:",__FUNCTION__);put_buf((p),(len));puts("\n");}while(0)

#define LEASE_TIME   86400U   //sec

#define LEASE_TMR_INTERVAL  60*60*1000U       //mill_sec


/** DHCP client states */
enum _dhcps_state {
    DHCPS_STATE_OFFER,
    DHCPS_STATE_DECLINE,
    DHCPS_STATE_ACK,
    DHCPS_STATE_NAK,
    DHCPS_STATE_IDLE,
};

struct _dhcps_cli {
    u8_t cli_mac[6];
    struct ip4_addr ipaddr;
    s32_t timeout;
    struct _dhcps_cli *next;
};

static struct udp_pcb *pcb_dhcps;
static struct ip4_addr broadcast_dhcps;
static struct ip4_addr server_address;
static struct ip4_addr y_address;//just for test
static struct ip4_addr client_address;
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


    memcpy_aligne((char *) &m->yiaddr.addr, (char *) &client_address.addr, sizeof(m->yiaddr.addr));
    memcpy_aligne((char *) &m->siaddr.addr, (char *) &server_address.addr, sizeof(m->siaddr.addr));
    memset((char *) &m->giaddr.addr, 0, sizeof(m->giaddr.addr));
    memset((char *) &m->sname[0], 0, sizeof(m->sname));
    memset((char *) &m->file[0], 0, sizeof(m->file));
    m->cookie = PP_HTONL(DHCP_MAGIC_COOKIE);
    memset((char *) &m->options[0], 0, sizeof(m->options));
}


static void send_offer(struct pbuf *p)
{
    DHCP_DBG_FUNC_IN();
    struct lan_setting *lan_setting_info;
    lan_setting_info = net_get_lan_info();
    struct _dhcps_cli *cli, *last_cli;
    struct dhcp_msg *m = (struct dhcp_msg *) p->payload;
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
            DHCP_DBG_PRINTF("no avail ipaddr \n!");
        }

        IP4_ADDR(&client_address, lan_setting_info->CLIENT_IPADDR1, lan_setting_info->CLIENT_IPADDR2, lan_setting_info->CLIENT_IPADDR3, ipaddr_cnt + lan_setting_info->CLIENT_IPADDR4);
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

    DHCP_DBG_SEND(p->payload, p->len);

    udp_sendto(pcb_dhcps, p, &broadcast_dhcps, DHCP_CLIENT_PORT);
    /*udp_sendto(pcb_dhcps, p, &broadcast_dhcps, DHCP_CLIENT_PORT);//减缓wifi信号不好导致DHCP分配不到的情况*/

}


static void send_nak(struct pbuf *p)
{
    DHCP_DBG_FUNC_IN();

    struct dhcp_msg *m = (struct dhcp_msg *) p->payload;
    u8_t *end;

    create_msg(m);

    end = add_msg_type(m->options, DHCP_NAK);
    end = add_end(end);

    DHCP_DBG_SEND(p->payload, p->len);

    udp_sendto(pcb_dhcps, p, &broadcast_dhcps, DHCP_CLIENT_PORT);
}

static void send_ack(struct pbuf *p)
{
    DHCP_DBG_FUNC_IN();

    struct dhcp_msg *m = (struct dhcp_msg *) p->payload;
    u8_t *end;

    create_msg(m);

    end = add_msg_type(m->options, DHCP_ACK);
    end = add_offer_options(end);
    end = add_end(end);

    DHCP_DBG_SEND(p->payload, p->len);

    udp_sendto(pcb_dhcps, p, &broadcast_dhcps, DHCP_CLIENT_PORT);

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
                    DHCP_DBG_PRINTF("+parse_options: cli != NULL -> dhcps_state = DHCPS_STATE_ACK\n");
                    memcpy(&client_address, &cli->ipaddr, sizeof(client_address));
                    cli->timeout = LEASE_TIME;
                    dhcps_state = DHCPS_STATE_ACK;
                    break;
                }
            }
            if (cli == NULL) {
                DHCP_DBG_PRINTF("+parse_options: cli == NULL -> dhcps_state = DHCPS_STATE_NAK\n");
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
                if (!memcmp(&cli->ipaddr, &m->ciaddr, sizeof(struct ip4_addr))) {
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


static s16_t parse_msg(struct pbuf *p)
{
    DHCP_DBG_FUNC_IN();

    struct dhcp_msg *m = (struct dhcp_msg *) p->payload;

    if (m->cookie == PP_HTONL(DHCP_MAGIC_COOKIE)) {
        return parse_options(m, p->len);
    }
    DHCP_DBG_PRINTF("\n m->cookie == 0x%x---------------parse_msg wrong! dhcp flame error!\n", m->cookie);
    return 0;
}


static void handle_dhcp(void *arg, struct udp_pcb *pcb, struct pbuf *p, const struct ip4_addr *addr, u16_t port)
{
    DHCP_DBG_FUNC_IN();

    struct pbuf *q;
    u16_t tlen;

    if (p == NULL) {
        return;
    }

    tlen = p->tot_len;
    if (p->next != NULL) {
        q = pbuf_coalesce(p, PBUF_TRANSPORT);
        if (q->tot_len != tlen) {
            pbuf_free(p);
            return;
        }
    } else {
        q = p;
    }

    DHCP_DBG_RECV(q->payload, q->tot_len);
    os_mutex_pend(&dhcps_mtx, 0);
    //如果源地址不为0，说明是请求续约的，此时需回复ACK,地址需为请求续约的客户端IP，反之源地址为0，说明是请求IP地址的，此时需回复分配的IP.
    if (addr->addr) {
        memcpy(&client_address, &addr->addr, sizeof(client_address));
    }
    switch (parse_msg(q)) {
    case	DHCPS_STATE_OFFER:
        send_offer(q);
        break;

    case	DHCPS_STATE_ACK	:
        send_ack(q);
        break;

    case	DHCPS_STATE_NAK	:
        send_nak(q);
        break;

    default :
        break;
    }
    os_mutex_post(&dhcps_mtx);
    pbuf_free(q);
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
    LWIP_DEBUGF(TIMERS_DEBUG, ("dhcps: dhcps_lease_timer()\n"));
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

    tcpip_timeout(LEASE_TMR_INTERVAL, dhcps_lease_timer, NULL);
}


void dhcps_init(void)
{
//    static char dhcps_init_flag;
//
//    if(dhcps_init_flag)
//        return;
//    dhcps_init_flag = 1;

    DHCP_DBG_FUNC_IN();

    struct lan_setting *lan_setting_info;
    lan_setting_info = net_get_lan_info();

    dhcps_cli_head = NULL;
    memset(ipaddr_tab, 0, sizeof(ipaddr_tab));

    pcb_dhcps = (struct udp_pcb *)udp_new();

    IP4_ADDR(&broadcast_dhcps, 255, 255, 255, 255);
    IP4_ADDR(&server_address, lan_setting_info->SERVER_IPADDR1, lan_setting_info->SERVER_IPADDR2, lan_setting_info->SERVER_IPADDR3, lan_setting_info->SERVER_IPADDR4);
    IP4_ADDR(&y_address, lan_setting_info->WIRELESS_IP_ADDR0, lan_setting_info->WIRELESS_IP_ADDR1, lan_setting_info->WIRELESS_IP_ADDR2, lan_setting_info->WIRELESS_IP_ADDR3); //绑定自己的IP
//   udp_bind(pcb_dhcps, IP_ADDR_ANY, DHCP_SERVER_PORT );
    udp_bind(pcb_dhcps, &y_address, DHCP_SERVER_PORT);
    udp_recv(pcb_dhcps, handle_dhcp, NULL);

//    tcpip_timeout(LEASE_TMR_INTERVAL, dhcps_lease_timer, NULL);
    os_mutex_create(&dhcps_mtx);
}

void dhcps_uninit(void)
{
    struct _dhcps_cli *cli;
    struct _dhcps_cli *n;

    os_mutex_pend(&dhcps_mtx, 0);

    DHCP_DBG_FUNC_IN();

    if (pcb_dhcps) {
        udp_remove(pcb_dhcps);
        pcb_dhcps = NULL;

        for (cli = dhcps_cli_head; cli != NULL; cli = n) {
            printf("dhcps remove mac_addr = 0x%x,0x%x,0x%x,0x%x,0x%x,0x%x\n",
                   cli->cli_mac[0], cli->cli_mac[1], cli->cli_mac[2],
                   cli->cli_mac[3], cli->cli_mac[4], cli->cli_mac[5]);

            printf("dhcps remove ipaddr :%d.%d.%d.%d\n",
                   ip4_addr1(&cli->ipaddr), ip4_addr2(&cli->ipaddr),
                   ip4_addr3(&cli->ipaddr), ip4_addr4(&cli->ipaddr));

            n = cli->next;

            free(cli);
        }
        dhcps_cli_head = NULL;
    }

    os_mutex_post(&dhcps_mtx);

    os_mutex_del(&dhcps_mtx, 1);
}


void dhcps_offer_dns(void)
{
    is_offer_dns = 1;
}


int dhcps_get_ipaddr(u8 hwaddr[6], struct ip4_addr *ipaddr)
{
    int ret = -1;
    os_mutex_pend(&dhcps_mtx, 0);
    for (struct _dhcps_cli *cli = dhcps_cli_head; cli != NULL; cli = cli->next) {
        if (memcmp(cli->cli_mac, hwaddr, 6) == 0) {
            memcpy(ipaddr, &cli->ipaddr, sizeof(struct ip4_addr));
            DHCP_DBG_PRINTF("dhcps_get_ipaddr[%d.%d.%d.%d] : 0x%x.0x%x.0x%x.0x%x.0x%x.0x%x\n",
                            ip4_addr1(&cli->ipaddr), ip4_addr2(&cli->ipaddr),
                            ip4_addr3(&cli->ipaddr), ip4_addr4(&cli->ipaddr),
                            cli->cli_mac[0], cli->cli_mac[1], cli->cli_mac[2],
                            cli->cli_mac[3], cli->cli_mac[4], cli->cli_mac[5]);
            ret = 0;
        }
    }
    os_mutex_post(&dhcps_mtx);
    return ret;
}
