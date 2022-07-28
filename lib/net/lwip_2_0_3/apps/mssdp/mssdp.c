#include "os/os_api.h"
#include "mssdp.h"
#include "list.h"
#include <string.h>
#include "lwip/sys.h"
#define MAX_UDP_PKT 1472

#define MSSDP_THREAD_PRIO          5
#define MSSDP_THREAD_STK_SIZE      1024

#define MSSDP_PKG_TRYS      2

enum mssdp_state {
    STOP,
    START,
};

struct mssdp_t {
    OS_MUTEX mutex;
    enum mssdp_state state;
    const char *search_msg;//搜索报文,
    const char *notify_msg;//宣告报文,
    u32_t search_time;//搜索时间间隔
    u32_t notify_time;//宣告时间间隔
    void (*recv_func)(struct sockaddr_in *si, enum mssdp_recv_msg_type type, char *buf, void *priv);
    void *priv;
    int socket;
    u16_t port;
    int pid;
    int search_time_hdl;
    int notify_time_hdl;
    struct sockaddr_in dest_addr;
    const char *search_prefix;
    const char *notify_prefix;
    const char *user_prefix;//第三项用户自定义协议宣告报文头
    struct sockaddr src_ss __attribute__((aligned(4)));
};
static struct mssdp_t mssdp;


static void mssdp_thread(void *arg)
{
    int ret, i;
    int byteReceived;
    socklen_t socklen = sizeof(mssdp.src_ss);
    u8_t recv_buf[MAX_UDP_PKT];
    fd_set rd_set, err_set;
    struct timeval tv = {0, 200 * 1000};
    char *p_msg;
    enum mssdp_recv_msg_type type;
    while (1) {

        FD_ZERO(&rd_set);
        FD_ZERO(&err_set);

        FD_SET(mssdp.socket, &rd_set);
        FD_SET(mssdp.socket, &err_set);

        ret = select(mssdp.socket + 1, &rd_set, NULL, &err_set, &tv);
        if (ret < 0 || FD_ISSET(mssdp.socket, &err_set)) {
            printf("%s %d->Error in select()\n", __FUNCTION__, __LINE__);
            goto EXIT;
        } else if (ret == 0) {
            if (mssdp.state == STOP) {
                goto EXIT;
            }
            if (mssdp.search_time && mssdp.search_msg && time_lapse((unsigned int *)(&mssdp.search_time_hdl), mssdp.search_time * 1000)) {
                if (mssdp.recv_func) {
                    mssdp.recv_func((struct sockaddr_in *)mssdp.dest_addr.sin_addr.s_addr, MSSDP_BEFORE_SEND_SEARCH_MSG, 0, mssdp.priv);
                }
                os_mutex_pend(&mssdp.mutex, 0);
                for (i = MSSDP_PKG_TRYS; i > 0; i--) {
                    ret = sendto(mssdp.socket, mssdp.search_msg, strlen(mssdp.search_msg) + 1, 0, (struct sockaddr *)&mssdp.src_ss, sizeof(struct sockaddr_in));
                    if (ret <= 0) {
                        printf("%s %d->Error in sendto() \n", __FUNCTION__, __LINE__);
//                        os_mutex_post(&mssdp.mutex);
//                        goto EXIT;
                    }
                }
                os_mutex_post(&mssdp.mutex);
            }
            if (mssdp.notify_time && mssdp.notify_msg && time_lapse((unsigned int *)(&mssdp.notify_time_hdl), mssdp.notify_time * 1000)) {
                if (mssdp.recv_func) {
                    mssdp.recv_func((struct sockaddr_in *)mssdp.dest_addr.sin_addr.s_addr, MSSDP_BEFORE_SEND_NOTIFY_MSG, 0, mssdp.priv);
                }
                os_mutex_pend(&mssdp.mutex, 0);
                for (i = MSSDP_PKG_TRYS; i > 0; i--) {
                    ret = sendto(mssdp.socket, mssdp.notify_msg, strlen(mssdp.notify_msg) + 1, 0, (struct sockaddr *)&mssdp.src_ss, sizeof(struct sockaddr_in));
                    if (ret <= 0) {
                        printf("%s %d->Error in sendto() \n", __FUNCTION__, __LINE__);
//                        os_mutex_post(&mssdp.mutex);
//                        goto EXIT;
                    }
                }
                os_mutex_post(&mssdp.mutex);
            }
        } else if (ret > 0 && FD_ISSET(mssdp.socket, &rd_set)) {
            os_mutex_pend(&mssdp.mutex, 0);
            byteReceived = recvfrom(mssdp.socket, recv_buf, MAX_UDP_PKT, 0, (struct sockaddr *)&mssdp.dest_addr, &socklen);
            os_mutex_post(&mssdp.mutex);

            recv_buf[byteReceived] = '\0';

//            printf("mssdp_recv->|%s|\n", recv_buf);
            p_msg = (char *)str_find((const char *)recv_buf, mssdp.search_prefix);
            if (p_msg) {
                type = MSSDP_SEARCH_MSG;
            } else {
                p_msg = (char *)str_find((const char *)recv_buf, mssdp.notify_prefix);
                if (p_msg) {
                    type = MSSDP_NOTIFY_MSG;
                } else {
                    p_msg = (char *)str_find((const char *)recv_buf, mssdp.user_prefix);
                    if (p_msg) {
                        type = MSSDP_USER_MSG;
                    } else {
                        printf("mssdp recv other msg --->%s\n", recv_buf);
                        continue;
                    }
                }
            }

            if (type == MSSDP_SEARCH_MSG && mssdp.notify_msg) {
                if (mssdp.recv_func) {
                    mssdp.recv_func((struct sockaddr_in *)mssdp.dest_addr.sin_addr.s_addr, MSSDP_BEFORE_SEND_NOTIFY_MSG, 0, mssdp.priv);
                }
                ret = sendto(mssdp.socket, mssdp.notify_msg, strlen(mssdp.notify_msg) + 1, 0, (struct sockaddr *)&mssdp.dest_addr, sizeof(struct sockaddr_in));
                if (ret <= 0) {
                    os_mutex_post(&mssdp.mutex);
                    printf("%s %d->Error in sendto() \n", __FUNCTION__, __LINE__);
                    goto EXIT;
                }
            } else {
                if (mssdp.recv_func) {
                    mssdp.recv_func((struct sockaddr_in *)mssdp.dest_addr.sin_addr.s_addr, type, p_msg, mssdp.priv);
                }
                if (type == MSSDP_USER_MSG && mssdp.notify_msg) {
                    ret = sendto(mssdp.socket, mssdp.notify_msg, strlen(mssdp.notify_msg), 0, (struct sockaddr *)&mssdp.dest_addr, sizeof(struct sockaddr_in));
                    if (ret <= 0) {
                        os_mutex_post(&mssdp.mutex);
                        printf("%s %d->Error in sendto() \n", __FUNCTION__, __LINE__);
                        goto EXIT;
                    }
                }
            }
        }
    }
EXIT:
    printf("ssdp_thread EXIT! \n");
}

int mssdp_send_msg(struct sockaddr_in *si, char *buf, u32_t buf_len)
{
    int ret;

    if (mssdp.state == STOP) {
        return 0;
    }

    if (os_mutex_pend(&mssdp.mutex, 0)) {
        return -1;
    }
    ret = sendto(mssdp.socket, buf, buf_len, 0, si ? (struct sockaddr *)si : (struct sockaddr *)&mssdp.src_ss, sizeof(struct sockaddr_in));
    os_mutex_post(&mssdp.mutex);
    if (ret <= 0) {
        puts("mssdp_notify error!\n");
        return -1;
    }
    return 0;
}

int mssdp_notify(struct sockaddr_in *si)
{
    int ret;

    if (mssdp.state == STOP) {
        return 0;
    }

    if (os_mutex_pend(&mssdp.mutex, 0)) {
        return -1;
    }
    ret = sendto(mssdp.socket, mssdp.notify_msg, strlen(mssdp.notify_msg) + 1, 0, si ? (struct sockaddr *)si : (struct sockaddr *)&mssdp.src_ss, sizeof(struct sockaddr_in));
    os_mutex_post(&mssdp.mutex);
    if (ret <= 0) {
        puts("mssdp_notify error!\n");
        return -1;
    }
    return 0;
}

int mssdp_search(void)
{
    int ret;

    if (mssdp.state == STOP) {
        return 0;
    }

    if (os_mutex_pend(&mssdp.mutex, 0)) {
        return -1;
    }
    ret = sendto(mssdp.socket, mssdp.search_msg, strlen(mssdp.search_msg) + 1, 0, (struct sockaddr *)&mssdp.src_ss, sizeof(struct sockaddr_in));
    os_mutex_post(&mssdp.mutex);
    if (ret <= 0) {
        puts("mssdp_search error!\n");
        return -1;
    }
    return 0;
}

int mssdp_init(const char *search_prefix, const char *notify_prefix, const char *user_prefix, u16_t port, void (*recv_func)(u32 dest_ipaddr, enum mssdp_recv_msg_type type, char *buf, void *priv), void *priv)
{
    int ret;
    int opt = 1;
    int onOff = 1;
    struct sockaddr_in *dest_addr;

    if (mssdp.state != STOP) {
        return 0;
    }

    memset(&mssdp, 0, sizeof(struct mssdp_t));

    mssdp.port = port;
    mssdp.search_prefix = search_prefix;
    mssdp.notify_prefix = notify_prefix;
    mssdp.user_prefix = user_prefix;

    ret = os_mutex_create(&mssdp.mutex);
    if (ret) {
        goto EXIT;
    }

    dest_addr = (struct sockaddr_in *)&mssdp.src_ss;
    mssdp.socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (mssdp.socket == -1) {
        printf("%s %d->Error in socket()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }
    ret = setsockopt(mssdp.socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
    if (ret) {
        printf("%s %d->Error in setsockopt()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    dest_addr->sin_family = AF_INET;
    dest_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr->sin_port = htons(mssdp.port);
    ret = bind(mssdp.socket, (struct sockaddr *)dest_addr, sizeof(*dest_addr));
    if (ret == -1) {
        printf("%s %d->Error in bind()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    ret = setsockopt(mssdp.socket, SOL_SOCKET, SO_BROADCAST,
                     (char *)&onOff, sizeof(onOff));
    if (ret == -1) {
        printf("%s %d->Error in setsockopt() SO_BROADCAST\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }
    inet_pton(AF_INET, "255.255.255.255", &dest_addr->sin_addr.s_addr);

    mssdp.recv_func = (void (*)(struct sockaddr_in *, enum mssdp_recv_msg_type, char *, void *))recv_func;
    mssdp.priv = priv;

    thread_fork("MSSDP_THREAD", MSSDP_THREAD_PRIO, MSSDP_THREAD_STK_SIZE, 0, &mssdp.pid, mssdp_thread, NULL);

    mssdp.state = START;

    return 0;

EXIT:
    if (mssdp.socket != -1) {
        closesocket(mssdp.socket);
    }

    return -1;
}

void mssdp_uninit(void)
{
    mssdp.state = STOP;

    thread_kill(&mssdp.pid, KILL_WAIT);

    os_mutex_pend(&mssdp.mutex, 0);
    os_mutex_post(&mssdp.mutex);
    os_mutex_del(&mssdp.mutex, 1);

    closesocket(mssdp.socket);
    free((void *)mssdp.search_msg);
    free((void *)mssdp.notify_msg);
}

int mssdp_set_search_msg(const char *search_msg, u32_t search_time)
{
    if (mssdp.state == STOP) {
        return -1;
    }

    if (os_mutex_pend(&mssdp.mutex, 0)) {
        return -1;
    }

    free((void *)mssdp.search_msg);

    if (asprintf((char **)&mssdp.search_msg, "%s%s", mssdp.search_prefix, search_msg == NULL ? "" : search_msg) <= 0) {
        os_mutex_post(&mssdp.mutex);
        return -1;
    }
    mssdp.search_time = search_time;
    os_mutex_post(&mssdp.mutex);

    return 0;
}
int mssdp_set_user_msg(const char *user_msg, u32_t notify_time)
{
    if (mssdp.state == STOP) {
        return -1;
    }

    if (os_mutex_pend(&mssdp.mutex, 0)) {
        return -1;
    }
    free((void *)mssdp.notify_msg);
    if (asprintf((char **)(&mssdp.notify_msg), "%s%s", mssdp.user_prefix, user_msg == NULL ? "" : user_msg) <= 0) {
        os_mutex_post(&mssdp.mutex);
        return -1;
    }
    mssdp.notify_time = notify_time;
    os_mutex_post(&mssdp.mutex);
    return 0;
}
int mssdp_set_notify_msg(const char *notify_msg, u32_t notify_time)
{
    if (mssdp.state == STOP) {
        return -1;
    }

    if (os_mutex_pend(&mssdp.mutex, 0)) {
        return -1;
    }

    free((void *)mssdp.notify_msg);
    if (asprintf((char **)(&mssdp.notify_msg), "%s%s", mssdp.notify_prefix, notify_msg == NULL ? "" : notify_msg) <= 0) {
        os_mutex_post(&mssdp.mutex);
        return -1;
    }
    mssdp.notify_time = notify_time;
    os_mutex_post(&mssdp.mutex);

    return 0;
}

void get_mssdp_info(const char **notify_prefix, int *socket, u16_t *port)
{
    if (notify_prefix) {
        *notify_prefix = mssdp.notify_prefix;
    }
    if (port) {
        *port = mssdp.port;
    }
    if (socket) {
        *socket = mssdp.socket;
    }
}
