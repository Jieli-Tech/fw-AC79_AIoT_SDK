#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "kcp/ikcp.h"
//#include "system/sys_common.h"
#include "os/os_api.h"

extern u32 timer_get_ms(void);

struct KCP_UDP_SERVER *kcp_udp_create(struct sockaddr_in *dest_addr, u32 conv);

#define UDP_MAX_MTU 1472
#define UDP_MAX_MSS (UDP_MAX_MTU-IKCP_HEAD_SIZE)
#define UDP_MAX_AGGREGATION_PKT_SIZE (44*1472)

#define UDP_SOCKET_TIMEOUT 100
#define KCP_SEM_TIMEOUT 150

struct KCP_UDP_SERVER {
    struct IQUEUEHEAD entry;
    ikcpcb *kcp_hdl;
    struct sockaddr_in dest_addr;
};

static IQUEUE_HEAD(kcp_udp_manager_list);
static OS_MUTEX kcp_udp_manager_mutex;
static int udp_sock;

static void kcp_udp_manager_mtx_init(void)
{
    os_mutex_create(&kcp_udp_manager_mutex);
}
void kcp_udp_manager_mtx_lock(void)
{
    os_mutex_pend(&kcp_udp_manager_mutex, 0);
}
void kcp_udp_manager_mtx_unlock(void)
{
    os_mutex_post(&kcp_udp_manager_mutex);
}

static int ikcp_sem_del(void *psem)
{
    return os_sem_del(psem, OS_DEL_ALWAYS);
}

static int ikcp_sem_post(void *psem)
{
    /* if (os_sem_query(psem) == 0) { */
//        os_sem_set(psem,0);
    os_sem_post(psem);
    /* } */

    return 0;
}
static int ikcp_sem_pend(void *psem, int timeout)
{
    return os_sem_pend(psem, timeout);
}

static int ikcp_mutex_lock(void *mutex)
{
    return os_mutex_pend(mutex, 0);
}
static void ikcp_mutex_unlock(void *mutex)
{
    os_mutex_post(mutex);
}

static void ikcp_mutex_del(void *mutex)
{
    os_mutex_del(mutex, OS_DEL_ALWAYS);
}


static void ikcp_writelog(struct IKCPCB *kcp, char *fmt, va_list va, void *user)
{
    puts("IKCP_DBG: ");
    vprintf(fmt, va);
    puts("\n");
}

static unsigned int iclock(void)
{
    return timer_get_ms();
}

static struct KCP_UDP_SERVER *kcp_manager_find(struct sockaddr_in *dest_addr, u32 conv)
{
    char find = 0;
    struct KCP_UDP_SERVER *member;

    kcp_udp_manager_mtx_lock();

    iqueue_foreach(member, &kcp_udp_manager_list, struct KCP_UDP_SERVER, entry) {
        if (member->kcp_hdl->conv == conv && member->dest_addr.sin_addr.s_addr == dest_addr->sin_addr.s_addr /*&& member->dest_addr.sin_port == dest_addr->sin_port*/) {
            find = 1;
            break;
        }
    }

    kcp_udp_manager_mtx_unlock();

    if (!find) {
        printf("kcp_udp new ip = 0x%x, conv = 0x%x \n", dest_addr->sin_addr.s_addr, conv);
    }

    return find ? member : (struct KCP_UDP_SERVER *)0;
}

static int *udp_output(const char *buf, int len, struct IKCPCB *kcp, void *user)
{
//    putbyte('^');put_u16hex(len);
    int ret;
    struct KCP_UDP_SERVER *kcp_udp_server = (struct KCP_UDP_SERVER *)user;

    ret = sendto(udp_sock, (const void *)buf, len, 0, (struct sockaddr *)&kcp_udp_server->dest_addr, sizeof(struct sockaddr_in));
    if (ret != len) {
        puts("udp_output fail!\n");
    }

    return 0;
}

static void kcp_udp_manager_update(void)
{
    struct KCP_UDP_SERVER *member;

    kcp_udp_manager_mtx_lock();
    iqueue_foreach(member, &kcp_udp_manager_list, struct KCP_UDP_SERVER, entry) {
        ikcp_update(member->kcp_hdl, iclock);
    }
    kcp_udp_manager_mtx_unlock();
}

static void kcp_udp_recv_thread(void *priv)
{
    struct KCP_UDP_SERVER *kcp_udp_server;
    int ret, nread, conv;
    fd_set rdset, errset;
    static struct sockaddr_in addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char *recv_buf = (char *)malloc(UDP_MAX_AGGREGATION_PKT_SIZE);

    struct timeval tv = {0, UDP_SOCKET_TIMEOUT * 1000};

    while (1) {
        FD_ZERO(&rdset);
        FD_ZERO(&errset);
        FD_SET(udp_sock, &rdset);
        FD_SET(udp_sock, &errset);

        ret = select(udp_sock + 1, &rdset, NULL, &errset, &tv);
        if (ret == 0) {
        } else if (FD_ISSET(udp_sock, &rdset)) {
            nread = recvfrom(udp_sock, recv_buf, UDP_MAX_AGGREGATION_PKT_SIZE, 0, (struct sockaddr *) &addr, &addr_len);
            if (nread > (IKCP_HEAD_SIZE - 1)) {
                conv = ikcp_getconv(recv_buf);
                kcp_udp_server = kcp_manager_find(&addr, conv);
                if (!kcp_udp_server) {
                    kcp_udp_server = kcp_udp_create(&addr, conv);
                }
#if 0
                static int timehdl;
                //drop packet test
                if (time_lapse(&timehdl, 10 * 1000)) {
                    puts("kcp d~\n");
                    continue;
                }
#endif
                ikcp_input(kcp_udp_server->kcp_hdl, recv_buf, nread);
            } else {
                printf("\r\n \r\n \r\n %s %d->Error in ()\n", __FUNCTION__, __LINE__);
                while (1);
            }
        } else {
            printf("\r\n \r\n \r\n %s %d->Error in ()\n", __FUNCTION__, __LINE__);
            while (1);
        }

        kcp_udp_manager_update();
    }
EXIT:
    free(recv_buf);
}

static int udp_socket_create(u16 port)
{
    int ret = -1;
    int flags;
    struct sockaddr_in dest_addr;
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = htons(port);
    if (bind(udp_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr))) {
        printf("%s %d->Error in bind()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    if ((flags = fcntl(udp_sock, F_GETFL, 0)) < 0) {
        printf("%s %d->Error in fcntl()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }
    flags |= O_NONBLOCK;
    if (fcntl(udp_sock, F_SETFL, flags) < 0) {
        printf("%s %d->Error in fcntl()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

EXIT:
    return ret;
}

struct KCP_UDP_SERVER *kcp_udp_create(struct sockaddr_in *dest_addr, u32 conv)
{
    int ret;
    char find = 0;
    struct KCP_UDP_SERVER *kcp_udp_server;

    kcp_udp_server = kcp_manager_find(dest_addr, conv);
    if (kcp_udp_server) {
        puts("kcp_udp_create already have a kcp_udp_server \n");
        return kcp_udp_server;
    }

    kcp_udp_server = (struct KCP_UDP_SERVER *)calloc(1, sizeof(struct KCP_UDP_SERVER));
    memcpy(&kcp_udp_server->dest_addr, dest_addr, sizeof(struct sockaddr_in));
    kcp_udp_server->kcp_hdl = ikcp_create(conv, (void *)kcp_udp_server);

    kcp_udp_server->kcp_hdl->output = (int (*)(const char *, int, struct IKCPCB *, void *))udp_output;
    ikcp_wndsize(kcp_udp_server->kcp_hdl, 256, 512);    //set snd recv windows size
    ikcp_nodelay(kcp_udp_server->kcp_hdl, 0, 2, 2, 0);    //set kcp mode

    ikcp_setmtu(kcp_udp_server->kcp_hdl, UDP_MAX_MTU, UDP_MAX_AGGREGATION_PKT_SIZE);

//    ikcp_set_writelog(kcp_udp_server->kcp_hdl, ikcp_writelog, IKCP_LOG_OUTPUT|IKCP_LOG_INPUT|IKCP_LOG_SEND|IKCP_LOG_RECV|IKCP_LOG_IN_DATA|\
//                      IKCP_LOG_IN_ACK|IKCP_LOG_IN_PROBE|IKCP_LOG_IN_WINS|IKCP_LOG_OUT_DATA|IKCP_LOG_OUT_ACK	|IKCP_LOG_OUT_PROBE|IKCP_LOG_OUT_WINS);


    OS_MUTEX *pmutex = (OS_MUTEX *)malloc(sizeof(OS_MUTEX));
    os_mutex_create(pmutex);
    ikcp_set_mutex_lock_func(kcp_udp_server->kcp_hdl, ikcp_mutex_lock, ikcp_mutex_unlock, ikcp_mutex_del, pmutex);

    OS_SEM *psendsem = (OS_SEM *)malloc(sizeof(OS_SEM));
    os_sem_create(psendsem, 0);
    ikcp_set_send_block(kcp_udp_server->kcp_hdl, 0, ikcp_sem_post, ikcp_sem_pend, ikcp_sem_del, KCP_SEM_TIMEOUT, psendsem);

    OS_SEM *precvsem = (OS_SEM *)malloc(sizeof(OS_SEM));
    os_sem_create(precvsem, 0);
    ikcp_set_recv_block(kcp_udp_server->kcp_hdl, 0, ikcp_sem_post, ikcp_sem_pend, ikcp_sem_del,  KCP_SEM_TIMEOUT, precvsem);

    ikcp_update(kcp_udp_server->kcp_hdl, iclock);

    iqueue_add_tail(&kcp_udp_server->entry, &kcp_udp_manager_list);

    kcp_udp_manager_mtx_unlock();

EXIT:
    return kcp_udp_server;
}


int kcp_send_test(void)
{
#define KCP_TEST_SEND_BUF_SIZE 123*1024+4567

    int time_hdl = 0;
    unsigned int total_send = 0;
    int ret;
    struct KCP_UDP_SERVER *kcp_udp_server;
    struct sockaddr_in dest_addr;
    char *send_buf;
    unsigned int send_len;

    udp_socket_create(1234);

    dest_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.1.3", &dest_addr.sin_addr);
    dest_addr.sin_port =  htons(4321);

    kcp_udp_server = kcp_udp_create(&dest_addr, 0x56444c4a);

    /* thread_fork("KCP_CLI_RECV_THREAD", 15, 0x2000, 0, NULL, kcp_udp_recv_thread, (void *)udp_sock); */

    send_buf = (char *)malloc(KCP_TEST_SEND_BUF_SIZE);
    for (ret = 0; ret < KCP_TEST_SEND_BUF_SIZE; ret++) {
        send_buf[ret] = ret;
    }

    while (1) {
        if (time_lapse((u32 *)(&time_hdl), 1 * 1000)) {
            printf(" KCP_SEND RATE = %d KB/S \r\n", total_send / 1024 / 1);
            total_send = 0;
        }
        send_len = KCP_TEST_SEND_BUF_SIZE;
        ret = ikcp_send(kcp_udp_server->kcp_hdl, (const char *)send_buf, send_len);
        if (!ret) {
            total_send += send_len;
//            ikcp_flush(kcp_udp_server->kcp_hdl,iclock);
            ikcp_update(kcp_udp_server->kcp_hdl, iclock);
        } else {
            puts("*");
        }
    }


EXIT:
    free(send_buf);

    return ret;
}

int kcp_recv_test(void)
{
#define KCP_TEST_RECV_BUF_SIZE 500*1024
    struct KCP_UDP_SERVER *kcp_udp_server;
    int time_hdl = 0;
    unsigned int total_recv = 0;
    struct sockaddr_in dest_addr;
    char *recv_buf;
    int nread;

    udp_socket_create(4321);
    dest_addr.sin_family = AF_INET;
    inet_pton(AF_INET, "192.168.1.3", &dest_addr.sin_addr);
    dest_addr.sin_port =  htons(1234);
    kcp_udp_server = kcp_udp_create(&dest_addr, 0x56444c4a);


    thread_fork("KCP_CLI_RECV_THREAD", 15, 0x2000, 0, NULL, kcp_udp_recv_thread, (void *)udp_sock);

    recv_buf = malloc(KCP_TEST_RECV_BUF_SIZE);
    while (1) {
        if (time_lapse((u32 *)(&time_hdl), 1 * 1000)) {
            printf(" KCP_RECV RATE = %d KB/S \r\n", total_recv / 1024 / 1);
            total_recv = 0;
        }

        nread = ikcp_recv(kcp_udp_server->kcp_hdl, recv_buf, KCP_TEST_RECV_BUF_SIZE);
        if (nread > 0) {
            total_recv += nread;
//            printf("                    ^^^^ ikcp_recv size = %d\r\n", nread);
//                hexdump(recv_buf, nread);
        } else {
            puts("*");
        }
    }
    free(recv_buf);
}


void kcp_udp_test(void)
{
    kcp_udp_manager_mtx_init();
//msleep(6*1000);
//   kcp_send_test();
    kcp_recv_test();

}

