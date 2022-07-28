/**
 * @author  Artium Nihamkin <artium@nihamkin.com>
 * @date May 2016
 *
 * @section LICENSE
 *
 * The MIT License (MIT)
 * Copyright © 2016 Artium Nihamkin, http://nihamkin.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * Test program for the ntp library. It uses UNIX sockets.
 */


#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <string.h>
#include "lwip.h"
#include "time.h"
#include "ntp/ntp.h"
#include "generic/list.h"
#include "os/os_api.h"
#include "device/device.h"
#include "system/includes.h"
#include "event/net_event.h"


#ifdef __GNUC__
#define PACKED __attribute__((packed))
#else
#define PACKED
#error "ntp.h: Your compiler does not support packed attribute"
#endif

#if NTP_ERR_INFO_ON
#define NTP_ERR_PRINTF	printf
#else
#define NTP_ERR_PRINTF
#endif


#if NTP_DBUG_INFO_ON
#define NTP_DBG_PRINTF             printf
#else
#define NTP_DBG_PRINTF
#endif

typedef union  {
    struct {
        unsigned char integer;
        unsigned char fraction;
    };
    unsigned int raw;
} ufixed_16_16_t;

typedef union {
    struct {
        char  integer;
        unsigned char fraction;
    };
    int raw;
} fixed_16_16_t;

typedef union  {
    struct {
        unsigned int integer;
        unsigned int fraction;
    };
    unsigned long long raw;
} ufixed_32_32_t;

typedef ufixed_32_32_t ntp_timestamp_t; // Notice: the value will be in network byte order




/*
 * @brief Extracts date from the timestamp.
 *
 * The following note from the RFC is not implemented:
 * "If bit 0 is set, the UTC time is in the range 1968-
 *  2036, and UTC time is reckoned from 0h 0m 0s UTC on 1 January
 *  1900.  If bit 0 is not set, the time is in the range 2036-2104 and
 *  UTC time is reckoned from 6h 28m 16s UTC on 7 February 2036.  Note
 *  that when calculating the correspondence, 2000 is a leap year, and
 *  leap seconds are not included in the reckoning."
 *
 * @param ts[in]     time stamp in network byte order
 * @param year[out]  1900 to 2037, no correction if bit 0 is not set.
 * @param month[out] 0 to 11.
 * @param day[out]   0 to 30.
 */



typedef enum  {
    NO_WARNING                 = 0,
    LAST_MINUTE_HAS_61_DECONDS = 1,
    LAST_MINUTE_HAS_59_DECONDS = 2,
    ALARM_CONDITION            = 3,
} ntp_leap_indicator_t;


typedef enum  {
    RESERVED          = 0,
    SYMMETRIC_ACTIVE  = 1,
    SYMMETRIC_PASSIVE = 2,
    CLIENT            = 3,
    SERVER            = 4,
    BROADCAST         = 5,
    RESERVED_6        = 6,
    RESERVED_7        = 7,
} ntp_mode_t;




/*
 * This is the main NTP structure that represents an NTP message.
 * The structure is the same for every message that is being sent
 * both by the server and by the client.
 * The content of the message may differ, some fields are unused when
 * the client sends the message.
 *
 * Notice: values are in network byte order.
 *
 */
typedef struct PACKED {
    unsigned char         byte_1;                   // consist of multiple fields
    unsigned char         stratum;
    unsigned char         poll;
    char          precision;
    fixed_16_16_t   root_delay;
    ufixed_16_16_t  root_dispersion;
    char            reference_identifier[4];  // see figure 2 in rfc4330
    ntp_timestamp_t reference_timestamp;
    ntp_timestamp_t originate_timestamp;
    ntp_timestamp_t recieve_timestamp;
    ntp_timestamp_t transmit_timestamp;
    unsigned int        key_identifier;           // not used by sntp
    unsigned int        mesage_digest[4];         // not uses by sntp
} ntp_packet_t;



#define NTP_PORT 123

/*
 * This is the message that is sent from client to server.
 *
 * It's definition is:
 *    mode - CLIENT (4)
 *    vn   - 4
 *
 * @notice Transmit timestamp is optional.
 *
 */
#define NTP_REQUEST_MSG { .byte_1 = 0b000100011 } ;

#define NTP_ORIGIN_YEAR 1900 /// aka epoch
#define NTP_GET_LI(b1)   ((ntp_leap_indicator_t)(((b1) & 0b11000000) >> 6))
#define NTP_GET_VN(b1)                          (((b1) & 0b00111000) >> 3)
#define NTP_GET_MODE(b1) ((ntp_mode_t)          (((b1) & 0b00000111)))

#define NTP_GET_TS_SECONDS_AFTER_MINUTE(ts)  ((ntohl((ts).integer)      )  % 60)
#define NTP_GET_TS_MINUTES_AFTER_HOUR(ts)    ((ntohl((ts).integer) /  60)  % 60)
#define NTP_GET_TS_HOURS_SINCE_MIDNIGHT(ts)  ((ntohl((ts).integer) / 3600) % 24)

#define NTP_GET_TS_DAYS_SINCE_JAN_1_1900(ts) (ntohl((ts).integer) / 86400)

#define NTP_IS_LEAP_YEAR(y) ((((y)%4 == 0) && ((y)%100 != 0)) || (y)%400 == 0)

extern u32 timer_get_sec(void);

static void ntp_get_time(struct tm *tmr)
{
    printf("%4d:%02d:%02d   %02d:%02d:%02d \n\n"
           , tmr->tm_year + 1900
           , tmr->tm_mon + 1
           , tmr->tm_mday
           , tmr->tm_hour
           , tmr->tm_min
           , tmr->tm_sec);
}

#if 0
struct host_name {
    struct list_head entry;
    char *name;
};

struct ntp_server_data {
    struct list_head host_name_head;
    int fd;
    u32 timeout;
    u32 houroffset;
    u32 query_interval;
    u32 close_flag;
    void (*get_time_cb)(struct tm *t);
};


static struct ntp_server_data ntp_data = {
    .houroffset = 8,
    .query_interval = 1,
    .get_time_cb = ntp_get_time,
};

void ntp_set_query_interval_min(u32 min)
{
    ntp_data.query_interval = min;
}

void ntp_set_timezone(u32 zone)
{
    ntp_data.houroffset = zone;
}

void ntp_set_time_cb(void (*f)(struct tm *t))
{
    ntp_data.get_time_cb = f;
}

int ntp_add_host_name(char *name)
{
    struct host_name *n = malloc(sizeof(struct host_name));

    if (n == NULL) {
        return -1;
    }

    n->name = name;
    list_add_tail(&n->entry, &ntp_data.host_name_head);

    return 0;
}

int ntp_free_all_host_name(void)
{
    struct list_head *pos, *node;
    struct host_name *n;
    list_for_each_safe(pos, node, &ntp_data.host_name_head) {
        n = list_entry(pos, struct host_name, entry);
        list_del(&n->entry);
        free(n);
    }

    return 0;
}

int ntp_remove_host_name(char *name)
{
    struct list_head *pos, *node;
    struct host_name *n;
    list_for_each_safe(pos, node, &ntp_data.host_name_head) {
        n = list_entry(pos, struct host_name, entry);

        if (!strcmp(name, n->name)) {
            list_del(&n->entry);
            free(n);
            return 0;
        }
    }
    NTP_DBG_PRINTF("ntp not find host name \n");

    return -1;
}
#endif

static void ntp_get_date(ntp_timestamp_t ts, unsigned int *year, unsigned int *month, unsigned int *day)
{
    unsigned int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    /* initial value of day, changes throughout the computation of year and month */
    *day  = NTP_GET_TS_DAYS_SINCE_JAN_1_1900(ts);

    /* year */
    for (*year = NTP_ORIGIN_YEAR; *year < 2037; (*year)++) {

        if (NTP_IS_LEAP_YEAR(*year)) {
            if (*day < 366) {
                break;
            } else {
                *day -= 366;
            }
        } else {
            if (*day < 365) {
                break;
            } else {
                *day -= 365;
            }
        }
    }

    /* Month */
    if (NTP_IS_LEAP_YEAR(*year)) {
        days_in_month[1] = 29; // february, index starts at 0
    }

    for (*month = 0; *month < 12; (*month)++) {
        if (*day >= days_in_month[*month]) {
            *day -= days_in_month[*month];
        } else {
            break;
        }
    }
}

#if 0
static inline void analysis_ntp_protocol(ntp_timestamp_t ts, struct tm *s_tm)
{
    unsigned int y, mo, d;
    unsigned int s  = NTP_GET_TS_SECONDS_AFTER_MINUTE(ts);
    unsigned int mi = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
    unsigned int h  = NTP_GET_TS_HOURS_SINCE_MIDNIGHT(ts);
    ntp_get_date(ts, &y, &mo, &d);
    s_tm->tm_sec = s;
    s_tm->tm_min = mi;
    s_tm->tm_hour = h + ntp_data.houroffset;
    s_tm->tm_mday = d + 1;
    s_tm->tm_mon = mo + 1; //fake
    s_tm->tm_year = y;
}

static inline u32 get_time_diff(ntp_timestamp_t ts)
{
    static u32 tmp = 0;
    u32 sec;
    u32 min;
    u32 hour;
    sec = NTP_GET_TS_SECONDS_AFTER_MINUTE(ts);
    min = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
    hour = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
    printf("tmp %d time %d \n", tmp, sec + 60 * min + 3600 * hour);

    if (tmp == 0) {
        tmp = sec + 60 * min + 3600 * hour;
        return 0;
    } else {
        return ((sec + 60 * min + 3600 * hour) - tmp);
    }

}



void ntp_thread(void *arg)
{
    int msg[32];
    int         port = NTP_PORT;
    int i;
    ntp_packet_t server_msg = {0};
    ntp_packet_t client_msg = NTP_REQUEST_MSG;

    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    struct hostent *hp;     /* host information */
    char ipaddr[20];
    struct list_head *pos;
    struct host_name *n;
    u8 is_already_set_time = 1;
    struct tm s_tm;
    u32 flag;
    int err;
    u32 count = ntp_data.query_interval * 60;
    if ((ntp_data.fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NTP_DBG_PRINTF("ntp cannot create socket");
        return ;
    }

    /* fill in the server's address and data */
    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    Get_IPAddress(1, ipaddr);
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = inet_addr(ipaddr);

    if (bind(ntp_data.fd, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0) {
        NTP_DBG_PRINTF("ntp bind fail \n");
        return ;
    }

    flag = fcntl(ntp_data.fd, F_GETFL, 0);
    flag |= O_NONBLOCK;
    err = fcntl(ntp_data.fd, F_SETFL, flag);
    if (err < 0) {
        NTP_DBG_PRINTF("std stdion to non-block fails\n");
        return ;
    }

    while (1) {

        if (ntp_data.close_flag) {
            goto err;
        }

        err = __os_taskq_pend(msg, ARRAY_SIZE(msg), 100);
        if (err == OS_Q_EMPTY) {
            if (count == ntp_data.query_interval * 60) { //60s
                count = 0;
                is_already_set_time = 0;
                list_for_each(pos, &ntp_data.host_name_head) {
                    n = list_entry(pos, struct host_name, entry);
                    hp = gethostbyname(n->name);

                    NTP_DBG_PRINTF("will attemt to communicate with %s\n", n->name);

                    if (!hp) {
                        NTP_DBG_PRINTF("could not obtain address of %s\n", n->name);
                        continue;
                    }
                    memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
                    NTP_DBG_PRINTF("sending data..\n");
                    if (sendto(ntp_data.fd, &client_msg, sizeof(client_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                        NTP_DBG_PRINTF("sendto failed");
                        continue;
                    }
                }
            }

            count++;

            i = recv(ntp_data.fd, &server_msg, sizeof(server_msg), 0);
            /* NTP_DBG_PRINTF("recieved: %d out of %lu bytes \n", i, sizeof(server_msg)); */
            if (!is_already_set_time && i > 0) {

                analysis_ntp_protocol(server_msg.recieve_timestamp, &s_tm);

                if (ntp_data.get_time_cb) {
                    ntp_data.get_time_cb(&s_tm);
                }

                /* is_already_set_time = 1; */

            }
        }

    }

err:
    ntp_free_all_host_name();
    closesocket(ntp_data.fd);
}


void ntp_thread(void *arg)
{
    int         port = NTP_PORT;
    int i;
    ntp_packet_t server_msg = {0};
    ntp_packet_t client_msg = NTP_REQUEST_MSG;

    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    struct hostent *hp;     /* host information */
    char ipaddr[20];
    struct list_head *pos;
    struct host_name *n;
    fd_set rdfs;
    int retval = 0;
    struct timeval tv;
    u8 is_already_set_time = 1;
    u8 count = 0;
    struct tm s_tm;
    u32 recv_timout_tocal = 0;
    tv.tv_sec = ntp_data.query_interval;

    if ((ntp_data.fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NTP_DBG_PRINTF("ntp cannot create socket");
        return;
    }

    /* fill in the server's address and data */
    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    Get_IPAddress(1, ipaddr);
    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = inet_addr(ipaddr);

    if (bind(ntp_data.fd, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0) {
        NTP_DBG_PRINTF("ntp bind fail \n");
        return;
    }

    while (1) {
        if (ntp_data.close_flag) {
            goto exit;
        }

        FD_ZERO(&rdfs);
        FD_SET(ntp_data.fd, &rdfs);

        NTP_DBG_PRINTF("set timeout %d sec \n", tv.tv_sec);
        retval = select(ntp_data.fd + 1, &rdfs, NULL, NULL, &tv);

        if (retval < 0) {
            goto exit;
        } else if (retval == 0) {
            count++;
            if (count == 10) {
                goto exit;
            }
            is_already_set_time = 0;
            NTP_DBG_PRINTF("%d sec send of one time \n\n", ntp_data.query_interval);
            tv.tv_sec = ntp_data.query_interval;
            recv_timout_tocal += tv.tv_sec;
            list_for_each(pos, &ntp_data.host_name_head) {
                n = list_entry(pos, struct host_name, entry);
                hp = gethostbyname(n->name);

                NTP_DBG_PRINTF("will attemt to communicate with %s\n", n->name);

                if (!hp) {
                    NTP_DBG_PRINTF("could not obtain address of %s\n", n->name);
                    continue;
                }

                memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);
                NTP_DBG_PRINTF("sending data..\n");

                if (sendto(ntp_data.fd, &client_msg, sizeof(client_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
                    NTP_DBG_PRINTF("sendto failed");
                    goto exit;
                }

            }

        } else {
            count = 0;
            NTP_DBG_PRINTF("recieving data..\n");
            i = recv(ntp_data.fd, &server_msg, sizeof(server_msg), 0);
            NTP_DBG_PRINTF("received: %u out of %lu bytes \n", i, sizeof(server_msg));

            //如果需要更精确，修改这函数
            analysis_ntp_protocol(server_msg.recieve_timestamp, &s_tm);

            if (recv_timout_tocal <= ntp_data.query_interval) {
                tv.tv_sec = ntp_data.timeout * 60 ;
            } else {
                tv.tv_sec = ntp_data.timeout * 60 - recv_timout_tocal;
            }

            recv_timout_tocal = 0;

            if (!is_already_set_time) {
                if (ntp_data.get_time_cb) {
                    ntp_data.get_time_cb(&s_tm);
                }
                is_already_set_time = 1;
            }
        }
    }

exit:
    ntp_free_all_host_name();
    closesocket(ntp_data.fd);
}



static int ntp_thread_pid = 0;

void ntp_client_init(void)
{
    INIT_LIST_HEAD(&ntp_data.host_name_head);
    /* ntp_add_host_name("www.catcoder.cn"); */
    /* ntp_add_host_name("time.nist.gov"); */
    ntp_set_time_cb(ntp_get_time);
    ntp_set_query_interval_min(1);
}

int ntp_client_start(void)
{
    return thread_fork("ntp_thread", NTP_CLIENT_THREAD_PRIO, NTP_CLIENT_THREAD_STACK_SIZE, 0, &ntp_thread_pid, ntp_thread, NULL);
}

void ntp_client_uninit(void)
{
    ntp_data.close_flag = 1;
    thread_kill(&ntp_thread_pid, KILL_WAIT);
    ntp_data.close_flag = 0;
}
#endif


static inline void analysis_ntp_protocol_ext(ntp_timestamp_t ts, struct tm *s_tm)
{
    unsigned int y, mo, d;
    unsigned int s  = NTP_GET_TS_SECONDS_AFTER_MINUTE(ts);
    unsigned int mi = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
    unsigned int h  = NTP_GET_TS_HOURS_SINCE_MIDNIGHT(ts);
    ntp_get_date(ts, &y, &mo, &d);
    s_tm->tm_sec = s;
    s_tm->tm_min = mi;
    s_tm->tm_hour = h;
    s_tm->tm_mday = d + 1;
    s_tm->tm_mon = mo;
    s_tm->tm_year = y - 1900;
}

extern u32 t_compensate_ms;
extern time_t t_compensate;
extern time_t source_timer;
extern u8 local_time_init_flag;

static u8 net_time_init_flag = 0;
static u8 ntp_time_exit = 0;
static volatile u8 ntp_time_running = 0;
static OS_SEM ntp_sem;

static void ntp_client_init(void)
{
    os_sem_create(&ntp_sem, 0);
}

late_initcall(ntp_client_init);

u8 ntp_client_get_time_status(void)
{
    return net_time_init_flag;
}

void ntp_client_get_time_clear(void)
{
    net_time_init_flag = 0;
}

void ntp_client_get_time_exit(void)
{
    ntp_time_exit = 1;
    os_sem_post(&ntp_sem);
}

void ntp_client_get_time(const char *host)
{
    if (ntp_time_running) {
        NTP_ERR_PRINTF("ntp_client_get_time is still running...\n");
        return ;
    }
    ntp_time_running = 1;

    time_t t;
    struct sys_time st;
    struct tm pt;
    int timeout_cnt = 0, ret = 0;

    ntp_time_exit = 0;
    os_sem_set(&ntp_sem, 0);

    memset(&pt, 0, sizeof(struct tm));

    while (!ntp_time_exit) {
        if (!host) {
            ret = ntp_client_get_time_all(&pt, 4000);
        } else {
            ret = ntp_client_get_time_once(host, &pt, 4000);
        }
        if (!ret) {
            source_timer = mktime(&pt);
            t_compensate = (time_t)timer_get_sec();
            t_compensate_ms = timer_get_ms();
            void *fd = dev_open("rtc", NULL);
            if (fd) {
                time_t t_timer = source_timer + 28800;
                localtime_r(&t_timer, &pt);
                memset(&st, 0, sizeof(struct sys_time));
                st.year = pt.tm_year + 1900;
                st.month = pt.tm_mon + 1;
                st.day = pt.tm_mday;
                st.hour = pt.tm_hour;
                st.min = pt.tm_min;
                st.sec = pt.tm_sec;
                printf("set_sys_time : %d-%d-%d,%d:%d:%d\n", st.year, st.month, st.day, st.hour, st.min, st.sec);
                dev_ioctl(fd, IOCTL_SET_SYS_TIME, (u32)&st);
                dev_close(fd);
            }
            //notify
            struct net_event evt = {0};
            evt.arg = "net";
            evt.event = NET_NTP_GET_TIME_SUCC;
            net_event_notify(NET_EVENT_FROM_USER, &evt);

            local_time_init_flag = 1;
            net_time_init_flag = 1;
            break;
        } else {
            if (timeout_cnt > 10) {
                timeout_cnt = 0;
            }
            os_sem_pend(&ntp_sem, ++timeout_cnt * 100);
        }
    }
    ntp_time_running = 0;
}

int ntp_client_get_time_all(struct tm *s_tm, int recv_to)
{
    if (!recv_to) {
        recv_to = 2000;
    }
    if (s_tm == NULL) {
        return -1;
    }

    /****/
    struct hostent *hp[NTP_HOST_NUM] = {0};     /* host information */
    struct sockaddr_in servaddr[NTP_HOST_NUM] = {0};

    const int port = NTP_PORT;
    int j = 0;
    int ret = -1;
    ntp_packet_t server_msg = {0};
    ntp_packet_t client_msg = NTP_REQUEST_MSG;

    struct sockaddr_in localaddr;
    int fd;
    char ipaddr[20];
    u8 cnt = 10;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NTP_ERR_PRINTF("cannot create ntp socket\n");
        return -1;
    }

    /* fill in the server's address and data */
    for (int i = 0; i < NTP_HOST_NUM; i++) {
        servaddr[i].sin_family = AF_INET;
        servaddr[i].sin_port = htons(port);
    }


    Get_IPAddress(1, ipaddr);

    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = inet_addr(ipaddr);
    if (bind(fd, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0) {
        NTP_ERR_PRINTF("ntp bind fail \n");
        goto fail;
    }

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&recv_to, sizeof(recv_to));
    /* look up the address of the server given its name */

    int inactive_host = 0;
    for (int i = 0; i < NTP_HOST_NUM; i++) {
        hp[i] = gethostbyname(ntp_host[i]);
        if (!hp[i]) {
            NTP_ERR_PRINTF("could not obtain ntp address of %s\n", ntp_host[i]);
            inactive_host++;
            continue;
        }
        /* put the host's address into the server address structure */
        memcpy((void *)&servaddr[i].sin_addr, hp[i]->h_addr_list[0], hp[i]->h_length);
    }
    if (inactive_host >= NTP_HOST_NUM) {
        goto fail;
    }

    NTP_DBG_PRINTF("sending data..\n");

    /* send a message to the server */
    for (int i = 0; i < NTP_HOST_NUM; i++) {
        cnt = 10;
        do {
            if (!servaddr[i].sin_addr.s_addr) {
                continue;
            }
            if (sendto(fd, &client_msg, sizeof(client_msg), 0, (struct sockaddr *)&servaddr[i], sizeof(servaddr[i])) < 0) {
                NTP_ERR_PRINTF("ntp sendto failed\n");
                goto fail;
            }
        } while (--cnt > 0);

        NTP_DBG_PRINTF("recieving data..\n");

    }
    j = recv(fd, &server_msg, sizeof(server_msg), 0);
    if (j > 0) {
        NTP_DBG_PRINTF("received: %u out of %lu bytes \n", j, sizeof(server_msg));
        analysis_ntp_protocol_ext(server_msg.recieve_timestamp, s_tm);
        ntp_get_time(s_tm);
        ret = 0;
        /* break; */
    } else {
        NTP_ERR_PRINTF("recieving data err: %d\n", j);
    }

fail:
    closesocket(fd);

    return ret;
}



int ntp_client_get_time_once(const char *host, struct tm *s_tm, int recv_to)
{
    if (!recv_to) {
        recv_to = 2000;
    }
    if (s_tm == NULL) {
        return -1;
    }
    if (host == NULL) {
        host = "s2c.time.edu.cn";
    }
    const int port = NTP_PORT;
    int i = 0;
    int ret = -1;
    ntp_packet_t server_msg = {0};
    ntp_packet_t client_msg = NTP_REQUEST_MSG;

    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    struct hostent *hp;     /* host information */
    int fd;
    char ipaddr[20];
    NTP_DBG_PRINTF("will attemt to communicate with %s\n", host);
    u8 cnt = 5;

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NTP_ERR_PRINTF("cannot create ntp socket\n");
        return -1;
    }

    /* fill in the server's address and data */
    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    Get_IPAddress(1, ipaddr);

    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = inet_addr(ipaddr);
    if (bind(fd, (struct sockaddr *)&localaddr, sizeof(localaddr)) < 0) {
        NTP_ERR_PRINTF("ntp bind fail \n");
        ret = 1;
        os_time_dly(200);
        goto err;
    }

    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&recv_to, sizeof(recv_to));
    /* look up the address of the server given its name */
    hp = gethostbyname(host);

    if (!hp) {
        NTP_ERR_PRINTF("could not obtain ntp address of %s\n", host);
        goto err;
    }

    /* put the host's address into the server address structure */
    memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

    NTP_DBG_PRINTF("sending data..\n");

    /* send a message to the server */
    do {
        if (sendto(fd, &client_msg, sizeof(client_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            NTP_ERR_PRINTF("ntp sendto failed\n");
            goto err;
        }
    } while (--cnt > 0);

    NTP_DBG_PRINTF("recieving data..\n");

    i = recv(fd, &server_msg, sizeof(server_msg), 0);
    if (i > 0) {
        NTP_DBG_PRINTF("received: %u out of %lu bytes \n", i, sizeof(server_msg));
        analysis_ntp_protocol_ext(server_msg.recieve_timestamp, s_tm);
        ntp_get_time(s_tm);
        ret = 0;
    } else {
        NTP_ERR_PRINTF("recieving data err: %d\n", i);
    }

err:
    closesocket(fd);

    return ret;
}

#if 0
void printf_ts(char header[], ntp_timestamp_t ts)
{
    unsigned int y, mo, d;
    unsigned int s  = NTP_GET_TS_SECONDS_AFTER_MINUTE(ts);
    unsigned int mi = NTP_GET_TS_MINUTES_AFTER_HOUR(ts);
    unsigned int h  = NTP_GET_TS_HOURS_SINCE_MIDNIGHT(ts);

    ntp_get_date(ts, &y, &mo, &d);
    printf("%s\t= ", header);
    printf("%02u:%02u:%02u %02u/%02u/%02u\n", h, mi, s, d + 1, mo + 1, y);
}

int ntp_test(void)
{
    const char const *host   = "www.catcoder.cn";
    const int         port = NTP_PORT;
    int i;
    ntp_packet_t server_msg = {0};
    ntp_packet_t client_msg = NTP_REQUEST_MSG;

    struct sockaddr_in servaddr;
    struct sockaddr_in localaddr;
    struct hostent *hp;     /* host information */
    int fd;
    char ipaddr[20];
    printf("will attemt to communicate with %s\n", host);


    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("cannot create socket");
        return 0;
    }

    /* fill in the server's address and data */
    memset((char *)&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    Get_IPAddress(1, ipaddr);

    localaddr.sin_family = AF_INET;
    localaddr.sin_port = htons(port);
    localaddr.sin_addr.s_addr = inet_addr(ipaddr);
    bind(fd, (struct sockaddr *)&localaddr, sizeof(localaddr));


    /* look up the address of the server given its name */
    hp = gethostbyname(host);

    if (!hp) {
        printf("could not obtain address of %s\n", host);
        return 0;
    }

    /* put the host's address into the server address structure */
    memcpy((void *)&servaddr.sin_addr, hp->h_addr_list[0], hp->h_length);

    printf("sending data..\n");

    /* send a message to the server */
    if (sendto(fd, &client_msg, sizeof(client_msg), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        printf("sendto failed");
        return 0;
    }

    printf("recieving data..\n");

    i = recv(fd, &server_msg, sizeof(server_msg), 0);

    printf("recieved: %u out of %lu bytes \n", i, sizeof(server_msg));

    printf("LI\t\t\t= %u\n",   NTP_GET_LI(server_msg.byte_1));
    printf("VN\t\t\t= %u\n",   NTP_GET_VN(server_msg.byte_1));
    printf("MODE\t\t\t= %u\n", NTP_GET_MODE(server_msg.byte_1));

    printf("STRATUM\t\t\t= %u\n", server_msg.stratum);

    printf("POLL INTERVAL\t\t= %u\n",         server_msg.poll);
    printf("PRECISION\t\t= %d\n",             server_msg.precision);
    printf("ROOT DELAY\t\t= %d\n",      ntohl(server_msg.root_delay.raw));
    printf("ROOT DISPRESION\t\t= %d\n", ntohl(server_msg.root_dispersion.raw));
    printf("REF ID\t\t\t= %s\n",            server_msg.reference_identifier);

    printf_ts("ORIGINATE_TIMESTAMP", server_msg.originate_timestamp);
    printf_ts("REFERENCE TIMESTAMP", server_msg.reference_timestamp);
    printf_ts("RECIEVE TIMESTAMP", server_msg.recieve_timestamp);
    printf_ts("TRANSMIT TIMESTAMP", server_msg.transmit_timestamp);

    return 0;
}
#endif
