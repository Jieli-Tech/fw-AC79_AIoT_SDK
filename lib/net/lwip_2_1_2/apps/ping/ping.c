/**
 * @file
 * Ping sender module
 *
 */

/*
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 */

/**
 * This is an example of a "ping" sender (with raw API and socket API).
 * It can be used as a start point to maintain opened a network connection, or
 * like a network "watchdog" for your device.
 *
 */

#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */

#include "ping.h"

#include "lwip/mem.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/inet.h"

#if PING_USE_SOCKETS
#include "lwip/sockets.h"
#include "os/os_compat.h"
#endif /* PING_USE_SOCKETS */


#define inet_addr_from_ipaddr(target_inaddr, source_ipaddr) ((target_inaddr)->s_addr = ip4_addr_get_u32(source_ipaddr))
#define inet_addr_to_ipaddr(target_ipaddr, source_inaddr)   (ip4_addr_set_u32(target_ipaddr, (source_inaddr)->s_addr))
/* ATTENTION: the next define only works because both s_addr and ip_addr_t are an u32_t effectively! */
#define inet_addr_to_ipaddr_p(target_ipaddr_p, source_inaddr)   ((target_ipaddr_p) = (ip_addr_t*)&((source_inaddr)->s_addr))

/**
 * PING_DEBUG: Enable debugging for PING.
 */
#undef PING_DEBUG
#ifndef PING_DEBUG
#define PING_DEBUG     LWIP_DBG_ON
#endif

static const ip_addr_t ping_test = {0x0201A8C0};//192.168.1.2
/*const ip_addr_t ping_test = {0xACE7E8B7};*/
/** ping target - should be a "ip_addr_t" */
#ifndef PING_TARGET
/*#define PING_TARGET   (netif_default?netif_default->gw:ip_addr_any)*/
#define PING_TARGET   (ping_test)
#endif

/** ping receive timeout - in milliseconds */
#ifndef PING_RCV_TIMEO
#define PING_RCV_TIMEO 4000
#endif

/** ping delay - in milliseconds */
#ifndef PING_DELAY
#define PING_DELAY     100
#endif

/** ping identifier - must fit on a u16_t */
#ifndef PING_ID
#define PING_ID        0xAFAF
#endif

/** ping additional data size to include in the packet */
#ifndef PING_DATA_SIZE
#define PING_DATA_SIZE 32
#endif

/** ping result action - no default action */
#ifndef PING_RESULT
#define PING_RESULT(ping_ok)
#endif

/* ping variables */
/* static u16_t ping_seq_num; */
/* static u32_t ping_time; */
#if !PING_USE_SOCKETS
/* static struct raw_pcb *ping_pcb; */

struct ping_t {
    struct raw_pcb *pcb;
    void (*cb)(void *, u32_t succ_cnt);
    void *priv;
    ip_addr_t ping_target;
    u32_t delayms;
    u32_t ping_time;
    u16_t ping_seq_num;
    u16_t ping_cnt;
    u16_t ping_total_cnt;
    u8_t succ_cnt;
};
#endif /* PING_USE_SOCKETS */

/** Prepare a echo ICMP request */
static void
ping_prepare_echo(struct icmp_echo_hdr *iecho, u16_t len, u16_t ping_seq_num)
{
    size_t i;
    size_t data_len = len - sizeof(struct icmp_echo_hdr);

    ICMPH_TYPE_SET(iecho, ICMP_ECHO);
    ICMPH_CODE_SET(iecho, 0);
    iecho->chksum = 0;
    iecho->id     = PING_ID;
    iecho->seqno  = htons(ping_seq_num);

    /* fill the additional data buffer with some data */
    for (i = 0; i < data_len; i++) {
        ((char *)iecho)[sizeof(struct icmp_echo_hdr) + i] = (char)i;
    }

    iecho->chksum = inet_chksum(iecho, len);
}

#if PING_USE_SOCKETS

/* Ping using the socket ip */
static err_t
ping_send(int s, ip_addr_t *addr, u16_t ping_seq_num)
{
    int err;
    struct icmp_echo_hdr *iecho;
    struct sockaddr_in to;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;
    LWIP_ASSERT("ping_size is too big", ping_size <= 0xffff);

    iecho = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
    if (!iecho) {
        return ERR_MEM;
    }

    ping_prepare_echo(iecho, (u16_t)ping_size, ping_seq_num);

    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    inet_addr_from_ipaddr(&to.sin_addr, addr);

    err = lwip_sendto(s, iecho, ping_size, 0, (struct sockaddr *)&to, sizeof(to));

    mem_free(iecho);

    return (err ? ERR_OK : ERR_VAL);
}

static int
ping_recv(int s, u32_t ping_time, u16_t ping_seq_num)
{
    char buf[64];
    int fromlen, len;
    struct sockaddr_in from;
    struct ip_hdr *iphdr;
    struct icmp_echo_hdr *iecho;

    while ((len = lwip_recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *)&from, (socklen_t *)&fromlen)) > 0) {
        if (len >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
            ip_addr_t fromaddr;
            inet_addr_to_ipaddr(&fromaddr, &from.sin_addr);
            LWIP_DEBUGF(PING_DEBUG, ("ping: recv "));
            ip_addr_debug_print(PING_DEBUG, &fromaddr);
            LWIP_DEBUGF(PING_DEBUG, (" %"U32_F" ms\n", (sys_now() - ping_time)));

            iphdr = (struct ip_hdr *)buf;
            iecho = (struct icmp_echo_hdr *)(buf + (IPH_HL(iphdr) * 4));
            if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping_seq_num))) {
                /* do some ping result processing */
                PING_RESULT((ICMPH_TYPE(iecho) == ICMP_ER));
                return 0;
            } else {
                LWIP_DEBUGF(PING_DEBUG, ("ping: drop\n"));
            }
        }
    }

    if (len == 0) {
        LWIP_DEBUGF(PING_DEBUG, ("ping: recv - %"U32_F" ms - timeout\n", (sys_now() - ping_time)));
    }

    /* do some ping result processing */
    PING_RESULT(0);

    return -1;
}

static void
ping_thread(void *arg)
{
    int s;
    int timeout = PING_RCV_TIMEO;
    ip_addr_t ping_target;
    u32_t ping_time = 0;
    u16_t ping_seq_num = LWIP_RAND();
    u16_t ping_cnt = 4;

    memcpy(&ping_target, arg, sizeof(ping_target));

    LWIP_UNUSED_ARG(arg);

    if ((s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP)) < 0) {
        return;
    }

    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    while (ping_cnt) {
        /* ping_target = PING_TARGET; */

        if (ping_send(s, &ping_target, ping_seq_num) == ERR_OK) {
            LWIP_DEBUGF(PING_DEBUG, ("ping: send "));
            ip_addr_debug_print(PING_DEBUG, &ping_target);
            LWIP_DEBUGF(PING_DEBUG, ("\n"));

            ping_time = sys_now();

            ping_recv(s, ping_time, ping_seq_num);
        } else {
            LWIP_DEBUGF(PING_DEBUG, ("ping: send "));
            ip_addr_debug_print(PING_DEBUG, &ping_target);
            LWIP_DEBUGF(PING_DEBUG, (" - error\n"));
        }
        /* sys_msleep(PING_DELAY); */
        msleep(PING_DELAY);

        ping_seq_num++;
        ping_cnt--;
    }
}

int
ping_init(const char *ip_addr_str)
{
    static u16_t ping_thread_id = 0;
    u16_t id = 0;
    char thread_name[32];
    OS_ENTER_CRITICAL();
    id = ping_thread_id++;
    OS_EXIT_CRITICAL();
    snprintf(thread_name, sizeof(thread_name), "ping_thread_%d", id);
    return thread_fork(thread_name, 6, 512, 0, NULL, ping_thread, (void *)inet_addr(ip_addr_str));
}

#else /* PING_USE_SOCKETS */

/* Ping using the raw ip */
static u8_t
ping_recv(void *arg, struct raw_pcb *pcb, struct pbuf *p, const ip_addr_t *addr)
{
    struct ping_t *ping = (struct ping_t *)arg;
    struct icmp_echo_hdr *iecho;
    LWIP_UNUSED_ARG(arg);
    LWIP_UNUSED_ARG(pcb);
    LWIP_UNUSED_ARG(addr);
    LWIP_ASSERT("p != NULL", p != NULL);

    if ((p->tot_len >= (PBUF_IP_HLEN + sizeof(struct icmp_echo_hdr))) &&
        pbuf_header(p, -PBUF_IP_HLEN) == 0) {
        iecho = (struct icmp_echo_hdr *)p->payload;

        if ((iecho->id == PING_ID) && (iecho->seqno == htons(ping->ping_seq_num))) {
            LWIP_DEBUGF(PING_DEBUG, ("ping: recv "));
            ip_addr_debug_print(PING_DEBUG, addr);
            LWIP_DEBUGF(PING_DEBUG, (" %"U32_F" ms\n", (sys_now() - ping->ping_time)));

            /* do some ping result processing */
            PING_RESULT(1);
            pbuf_free(p);
            ping->succ_cnt++;
            return 1; /* eat the packet */
        }
    }

    return 0; /* don't eat the packet */
}

static void
ping_send(struct raw_pcb *raw, ip_addr_t *addr, void *arg)
{
    struct ping_t *ping = (struct ping_t *)arg;
    struct pbuf *p;
    struct icmp_echo_hdr *iecho;
    size_t ping_size = sizeof(struct icmp_echo_hdr) + PING_DATA_SIZE;

    LWIP_DEBUGF(PING_DEBUG, ("ping: send "));
    ip_addr_debug_print(PING_DEBUG, addr);
    LWIP_DEBUGF(PING_DEBUG, ("\n"));
    LWIP_ASSERT("ping_size <= 0xffff", ping_size <= 0xffff);

    p = pbuf_alloc(PBUF_IP, (u16_t)ping_size, PBUF_RAM);
    if (!p) {
        return;
    }
    if ((p->len == p->tot_len) && (p->next == NULL)) {
        iecho = (struct icmp_echo_hdr *)p->payload;

        ping_prepare_echo(iecho, (u16_t)ping_size, ping->ping_seq_num);

        raw_sendto(raw, p, addr);
        ping->ping_time = sys_now();
    }
    pbuf_free(p);
}

static void
ping_timeout(void *arg)
{
    struct ping_t *ping = (struct ping_t *)arg;
    struct raw_pcb *pcb = (struct raw_pcb *)ping->pcb;

    LWIP_ASSERT("ping_timeout: no pcb given!", pcb != NULL);

    if (++ping->ping_cnt > ping->ping_total_cnt) {
        if (ping->cb) {
            ping->cb(ping->priv, ping->succ_cnt);
        }
        raw_remove(pcb);
        free(ping);
        return;
    }

    ++ping->ping_seq_num;

    ping_send(pcb, &ping->ping_target, ping);

    sys_timeout(ping->delayms, ping_timeout, ping);
}

static void
ping_raw_init(struct ping_t *ping)
{
    ping->pcb = raw_new(IP_PROTO_ICMP);
    LWIP_ASSERT("ping_pcb != NULL", ping->pcb != NULL);

    raw_recv(ping->pcb, ping_recv, ping);
    raw_bind(ping->pcb, IP_ADDR_ANY);
    sys_timeout(ping->delayms, ping_timeout, ping);
}

int
ping_init(const char *ip_addr_str, u32 delayms, u32 ping_total_cnt, void (*cb)(void *, u32), void *priv)
{
    struct ping_t *ping = (struct ping_t *)zalloc(sizeof(struct ping_t));
    if (!ping) {
        return -1;
    }

    u32_t ip = inet_addr(ip_addr_str);
    memcpy(&ping->ping_target, &ip, sizeof(ip));
    ping->delayms = delayms;
    ping->ping_seq_num = LWIP_RAND();
    ping->ping_total_cnt = ping_total_cnt;
    ping->cb = cb;
    ping->priv = priv;

    ping_raw_init(ping);

    return 0;
}

#endif /* PING_USE_SOCKETS */

#endif /* LWIP_RAW */

