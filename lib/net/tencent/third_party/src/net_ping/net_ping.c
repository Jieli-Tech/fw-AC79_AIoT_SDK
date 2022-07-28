//#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lwip/tcpip.h>
#include <lwip/inet.h>
#include "lwip/sockets.h"
#include <lwip/ip.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <lwip/raw.h>
#include "net_ping.h"
#include "os_wrapper.h"

#define TVS_LOG_DEBUG_MODULE  "netping"
#include "tvs_log.h"

static u16_t PING_IDs = 0x1234;
#define PING_TO		3000    /* timeout to wait every reponse(ms) */
#define PING_ID		0xABCD
#define PING_DATA_SIZE	100     /* size of send frame buff, not include ICMP frma head */
#define PING_IP_HDR_SIZE	40
#define GET_TICKS	os_wrapper_get_time_ms

#if LWIP_RAW

struct ping_t {
    struct raw_pcb *pcb;
    void (*cb)(void *, u32_t succ_cnt);
    void *priv;
    ip_addr_t ping_target;
    u32_t delayms;
    u32_t ping_time;
    u32_t ping_size;
    u16_t ping_seq_num;
    u16_t ping_cnt;
    u16_t ping_total_cnt;
    u8_t succ_cnt;
};

struct ping_res_t {
    OS_SEM sem;
    u8_t succ_cnt;
};

static void ping_prepare_echo(u8_t *buf, u32_t len, u16_t seq)
{
    u32_t i;
    u32_t data_len = len - sizeof(struct icmp_echo_hdr);
    struct icmp_echo_hdr *pecho;

    pecho = (struct icmp_echo_hdr *)buf;

    ICMPH_TYPE_SET(pecho, ICMP_ECHO);
    ICMPH_CODE_SET(pecho, 0);

    pecho->chksum = 0;
    pecho->id = PING_IDs;
    pecho->seqno = htons(seq);

    /* fill the additional data buffer with some data */
    for (i = 0; i < data_len; i++) {
        buf[sizeof(struct icmp_echo_hdr) + i] = (unsigned char)i;
    }
    /* Checksum of icmp header and data */
    pecho->chksum = inet_chksum(buf, len);
}

static void ping_send(struct raw_pcb *raw, ip_addr_t *addr, void *arg)
{
    struct ping_t *ping = (struct ping_t *)arg;
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + ping->ping_size;

    p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if (!p) {
        return;
    }
    if ((p->len == p->tot_len) && (p->next == NULL)) {
        iecho = (struct icmp_echo_hdr *)p->payload;

        ping_prepare_echo((u8_t *)iecho, (u16_t)ping_size, ping->ping_seq_num);

        raw_sendto(raw, p, addr);
        ping->ping_time = GET_TICKS();
    }
    pbuf_free(p);
}

static void ping_timeout(void *arg)
{
    struct ping_t *ping = (struct ping_t *)arg;
    struct raw_pcb *pcb = (struct raw_pcb *)ping->pcb;

    if (++ping->ping_cnt > ping->ping_total_cnt) {
        if (ping->cb) {
            ping->cb(ping->priv, ping->succ_cnt);
        }
        raw_remove(pcb);
        if (ping->priv) {
            ((struct ping_res_t *)ping->priv)->succ_cnt = ping->succ_cnt;
            os_sem_post(&((struct ping_res_t *)ping->priv)->sem);
        }
        free(ping);
        return;
    }

    ++ping->ping_seq_num;

    ping_send(pcb, &ping->ping_target, ping);

    sys_timeout(ping->delayms, ping_timeout, ping);
}

/* Ping using the raw ip */
static u8_t ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    struct ping_t *ping = (struct ping_t *)arg;
    struct icmp_echo_hdr *iecho;

    if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) &&
        IPH_PROTO((struct ip_hdr *)p->payload) == IP_PROTO_ICMP &&
        pbuf_header(p, -PBUF_IP_HLEN) == 0) {
        iecho = (struct icmp_echo_hdr *)p->payload;

        if ((iecho->id == PING_IDs) && (iecho->seqno == htons(ping->ping_seq_num))) {
            TVS_LOG_PRINTF("ping: recv ");
            ip_addr_debug_print(LWIP_DBG_ON, addr);
            TVS_LOG_PRINTF("%lu ms\n", GET_TICKS() - ping->ping_time);
            ++ping->succ_cnt;
            sys_untimeout(ping_timeout, ping);
            ping_timeout(ping);
        }

        pbuf_free(p);
        return 1; /* eat the packet */
    }

    return 0; /* don't eat the packet */
}

static int ping_raw_init(struct ping_t *ping)
{
    ping->pcb = raw_new(IP_PROTO_ICMP);
    if (!ping->pcb) {
        free(ping);
        return -1;
    }

    raw_recv(ping->pcb, ping_recv, ping);
    raw_bind(ping->pcb, IP_ADDR_ANY);
    ping_timeout(ping);
    /* sys_timeout(ping->delayms, ping_timeout, ping); */

    return 0;
}

static int ping_init(ip4_addr_t *ip_addr, u32 delayms, u32 ping_total_cnt, u32 ping_size, void (*cb)(void *, u32), void *priv)
{
    struct ping_t *ping = (struct ping_t *)zalloc(sizeof(struct ping_t));
    if (!ping) {
        return -1;
    }

    memcpy(&ping->ping_target, ip_addr, sizeof(*ip_addr));
    ping->delayms = delayms;
    ping->ping_seq_num = LWIP_RAND();
    ping->ping_total_cnt = ping_total_cnt;
    ping->ping_size = ping_size;
    ping->cb = cb;
    ping->priv = priv;

    return ping_raw_init(ping);
}

s32_t net_ping(struct net_ping_data *data)
{
    struct ping_res_t ping_res = {0};
    u32_t request_size;

    if (++PING_IDs == 0x7FFF) {
        PING_IDs = 0x1234;
    }

    if (data->data_long != 0xffff) {
        request_size = data->data_long;
    } else {
        request_size = PING_DATA_SIZE;
    }

    os_sem_create(&ping_res.sem, 0);

    if (0 != ping_init(&data->sin_addr, PING_TO, data->count, request_size, NULL, &ping_res)) {
        return -1;
    }

    os_sem_pend(&ping_res.sem, 0);

    return ping_res.succ_cnt > 0 ? ping_res.succ_cnt : -1;
}

#else

s32_t net_ping(struct net_ping_data *data)
{
    return -1;
}

#endif
