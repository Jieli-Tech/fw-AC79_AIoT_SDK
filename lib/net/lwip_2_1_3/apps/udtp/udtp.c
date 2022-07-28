#include "lwip/netdb.h"
#include "os_api.h"
#include "os_compat.h"
#include "udtp.h"
#include <sock_api.h>
#include "list.h"
#include <string.h>

#define DEFAULT_UDTP_RECV_THREAD_PRIO 15
#define DEFAULT_UDTP_RECV_THREAD_STK_SIZE 1500

#define DEFAULT_UDTP_SEND_THREAD_PRIO 15
#define DEFAULT_UDTP_SEND_THREAD_STK_SIZE 1500

enum udtp_hdl_state {
    UDTP_CLOSE,
    UDTP_OPEN = 0x994772ac,
};

struct udtp_hdl {
    struct list_head entry;
    OS_MUTEX mutex;
    OS_SEM send_sem;
    int udtp_mode;
    enum udtp_hdl_state state;
    void *sock_hdl;
    u32 listen_port;
    struct sockaddr_in dst_addr;
    int recv_flag;
    int send_flag;
    void *recv_buf;
    u32 recv_buf_len;
    int (*cb_func)(struct udtp_hdl *hdl, struct sockaddr_in *dst_addr, enum udtp_msg_type type, u8 *buf, u32 len, void *priv);
    int recv_thread_pid;
    int send_thread_pid;
    u32 send_thread_prio;
    u32 send_thread_stksize;
    u32 recv_thread_prio;
    u32 recv_thread_stksize;
    void *priv;
};

struct udtp_t {
    struct list_head hdl_list_head;
    OS_MUTEX mtx;
};

static struct udtp_t udtp;

int udtp_init(void)
{
    memset(&udtp, 0, sizeof(struct udtp_t));

    INIT_LIST_HEAD(&udtp.hdl_list_head);

    if (os_mutex_create(&udtp.mtx)) {
        goto EXIT;
    }

    return 0;

EXIT:

    return -1;
}

static void udtp_send_thread(void *arg)
{
#define MAX_UDP_PKT 1472

    struct udtp_hdl *hdl = (struct udtp_hdl *)arg;

    int ret, len;
    char _data_buf[MAX_UDP_PKT];
    u8 *data_buf = _data_buf;

SEND_AG:
    os_sem_pend(&hdl->send_sem, 0);

    while (1) {
        if (hdl->state == UDTP_CLOSE) {
            goto EXIT;
        }

        len = hdl->cb_func(hdl, &hdl->dst_addr, UDTP_SEND_DATA, data_buf, MAX_UDP_PKT, hdl->priv);
        if (len == 0) {
            goto SEND_AG;
        } else if (len < 0) {
            goto EXIT;
        }

        ret = sock_sendto(hdl->sock_hdl, data_buf, len, hdl->send_flag, (const struct sockaddr *)&hdl->dst_addr, sizeof(struct sockaddr_in));
        if (ret <= 0) {
            printf("%s %d->  send: %d\n", __FUNCTION__, __LINE__, ret);
//            goto EXIT;
        }
    }

EXIT:
    hdl->state = UDTP_CLOSE;
    udtp_unreg(hdl);
}

static int udtp_sock_cb(enum sock_api_msg_type type, void *priv)
{
    struct udtp_hdl *hdl = (struct udtp_hdl *)priv;

    if (hdl->state == UDTP_CLOSE) {
        return -1;
    }

    return 0;
}

static void udtp_recv_thread(void *arg)
{
#define MAX_UDP_PKT 1472

    struct udtp_hdl *hdl = (struct udtp_hdl *)arg;

    int ret, len;
    u8 *p_buf;
    u8 _data_buf[MAX_UDP_PKT];
    u8 *data_buf = _data_buf;

    struct sockaddr_in __ss_r;
    socklen_t socklen = sizeof(struct sockaddr_in);

    while (1) {
        ret = hdl->cb_func(hdl, NULL, UDTP_BEFORE_RECV, data_buf, MAX_UDP_PKT, hdl->priv);
        if (ret < 0) {
            goto EXIT;
        }

        if (hdl->recv_buf) {
            p_buf = hdl->recv_buf;
        } else {
            p_buf = data_buf;
        }

        if (hdl->recv_buf_len) {
            len = hdl->recv_buf_len;
        } else {
            len = sizeof(data_buf);
        }

        ret = sock_recvfrom(hdl->sock_hdl, p_buf, len, 0, (struct sockaddr *)&__ss_r, &socklen);
        if (ret <= 0) {
            printf("%s %d->  sock_recvfrom: %d\n", __FUNCTION__, __LINE__, ret);
//            goto EXIT;
        }

        ret = hdl->cb_func(hdl, &__ss_r, UDTP_RECV_DATA, p_buf, ret, hdl->priv);
        if (ret < 0) {
            goto EXIT;
        }
    }

EXIT:
    hdl->state = UDTP_CLOSE;
    udtp_unreg(hdl);
}

void *udtp_reg(struct sockaddr_in *dst_addr, u16 listen_port, int (*cb_func)(void *hdl, struct sockaddr_in *dst_addr, enum udtp_msg_type type, u8 *buf, u32 len, void *priv),  void *priv, int udtp_mode)
{
    int ret;
    struct udtp_hdl *hdl;
    struct list_head *pos;

    if (udtp_mode & UDTP_READ) {
        os_mutex_pend(&udtp.mtx, 0);
        list_for_each(pos, &udtp.hdl_list_head) {
            hdl = list_entry(pos, struct udtp_hdl, entry);
            if (listen_port == hdl->listen_port) {
                os_mutex_post(&udtp.mtx);
                printf("udtp_reg listen_port has reg = 0x%x\n", listen_port);
                return NULL;
            }
        }
        os_mutex_post(&udtp.mtx);
    }

    hdl = (struct udtp_hdl *)calloc(sizeof(struct udtp_hdl), 1);
    if (hdl == NULL) {
        goto EXIT;
    }

    hdl->state = UDTP_OPEN;
    hdl->cb_func = (int (*)(struct udtp_hdl *, struct sockaddr_in *, enum udtp_msg_type, u8 *, u32, void *))cb_func;
    hdl->priv = priv;
    hdl->udtp_mode = udtp_mode;
    hdl->listen_port = listen_port;

    hdl->sock_hdl = sock_reg(AF_INET, SOCK_DGRAM, 0, udtp_sock_cb, hdl);
    if (hdl->sock_hdl == NULL) {
        printf("%s %d->Error in socket()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    hdl->dst_addr.sin_family = AF_INET;
    hdl->dst_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    hdl->dst_addr.sin_port = htons(hdl->listen_port);
    ret = sock_bind(hdl->sock_hdl, (struct sockaddr *)&hdl->dst_addr, sizeof(struct sockaddr));
    if (ret) {
        printf("%s %d->Error in bind()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    hdl->dst_addr.sin_addr.s_addr = dst_addr->sin_addr.s_addr;
    hdl->dst_addr.sin_port = dst_addr->sin_port;
    if (hdl->dst_addr.sin_addr.s_addr == 0xffffffff) {
        int onOff = 1;
        ret = setsockopt(sock_get_socket(hdl->sock_hdl), SOL_SOCKET, SO_BROADCAST,
                         (char *)&onOff, sizeof(onOff));
        if (ret == -1) {
            printf("%s %d->Error in setsockopt() SO_BROADCAST\n", __FUNCTION__, __LINE__);
            goto EXIT;
        }
    }

    os_mutex_create(&hdl->mutex);
    os_mutex_pend(&udtp.mtx, 0);
    list_add_tail(&hdl->entry, &udtp.hdl_list_head);
    os_mutex_post(&udtp.mtx);

    hdl->send_thread_prio = DEFAULT_UDTP_SEND_THREAD_PRIO;
    hdl->send_thread_stksize = DEFAULT_UDTP_SEND_THREAD_STK_SIZE;
    hdl->recv_thread_prio = DEFAULT_UDTP_RECV_THREAD_PRIO;
    hdl->recv_thread_stksize = DEFAULT_UDTP_RECV_THREAD_STK_SIZE;

    hdl->cb_func(hdl, NULL, UDTP_SET_THREAD_PRIO_STKSIZE, NULL, 0, hdl->priv);

    if (hdl->udtp_mode & UDTP_WRITE) {
        os_sem_create(&hdl->send_sem, 0);
        thread_fork("udtp_send_thread", hdl->send_thread_prio, hdl->send_thread_stksize, 0, &hdl->send_thread_pid, udtp_send_thread, (void *)hdl);
    }

    if (hdl->udtp_mode & UDTP_READ) {
        thread_fork("udtp_recv_thread", hdl->recv_thread_prio, hdl->recv_thread_stksize, 0, &hdl->recv_thread_pid, udtp_recv_thread, (void *)hdl);
    }

    return hdl;

EXIT:
    if (hdl) {
        free(hdl);
    }

    return NULL;
}

void udtp_unreg(void *handle)
{
    struct udtp_hdl *hdl = (struct udtp_hdl *)handle;

    if (!hdl) {
        return;
    }

    struct list_head *pos, *node;
    bool find = 0;

    os_mutex_pend(&udtp.mtx, 0);
    list_for_each_safe(pos, node, &udtp.hdl_list_head) {
        if (hdl == list_entry(pos, struct udtp_hdl, entry)) {
            find = 1;
            list_del(&hdl->entry);
            break;
        }
    }
    os_mutex_post(&udtp.mtx);
    if (!find) {
        return;
    }

    hdl->cb_func(hdl, NULL, UDTP_HDL_CLOSE, NULL, 0, hdl->priv);

    hdl->state = UDTP_CLOSE;
    if (hdl->udtp_mode & UDTP_WRITE) {
        os_sem_del(&hdl->send_sem, 1);
        thread_kill(&hdl->send_thread_pid, KILL_WAIT);
    }

    if (hdl->udtp_mode & UDTP_READ) {
        thread_kill(&hdl->recv_thread_pid, KILL_WAIT);
    }

    os_mutex_pend(&hdl->mutex, 0);
    os_mutex_post(&hdl->mutex);
    os_mutex_del(&hdl->mutex, 1);

    sock_unreg(hdl->sock_hdl);

    free(hdl);
}

#if 0
void udtp_uninit(void)
{
    struct list_head *pos;
    struct udtp_hdl *hdl;

    while (1) {
        os_mutex_pend(&udtp.mtx, 0, 0);

        if (list_empty(&udtp.hdl_list_head)) {
            os_mutex_post(&udtp.mtx);
            break;
        }
        hdl = list_first_entry(&udtp.hdl_list_head, struct udtp_hdl, entry);
        list_del(&hdl->entry);
        os_mutex_post(&udtp.mtx);

        hdl->cb_func(hdl, NULL, UDTP_HDL_CLOSE, NULL, 0, hdl->priv);

        if (hdl->udtp_mode & UDTP_WRITE) {
            os_sem_del(&hdl->send_sem, 1);
            thread_kill(&hdl->send_thread_pid, KILL_WAIT);
        }

        if (hdl->udtp_mode & UDTP_READ) {
            thread_kill(&hdl->recv_thread_pid, KILL_WAIT);
        }

        sock_unreg(hdl->sock_hdl);
        free(hdl);
    }

    os_mutex_del(&udtp.mtx, 1);
}
#endif

int udtp_send(void *handle)
{
    struct udtp_hdl *hdl = (struct udtp_hdl *)handle;

    if (!hdl) {
        return -1;
    }

    if (hdl->state != UDTP_OPEN) {
        return -1;
    }

    os_sem_set(&hdl->send_sem, 0);
    os_sem_post(&hdl->send_sem);

    return 0;
}

inline int udtp_send_buf(void *handle, char *buf, u32 len, int flag)
{
    int ret = -1;
    struct udtp_hdl *hdl = (struct udtp_hdl *)handle;

    if (os_mutex_pend(&hdl->mutex, 0)) {
        printf("%s %d->error.\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (hdl->state == UDTP_OPEN) {
        ret = sock_sendto(hdl->sock_hdl, buf, len, flag, (const struct sockaddr *)&hdl->dst_addr, sizeof(struct sockaddr_in));
    }

    os_mutex_post(&hdl->mutex);

    return ret;
}

int udtp_recv(void *handle, u8 *buf, u32 len, int flag, struct sockaddr_in *dest_addr)
{
    int ret;
    struct udtp_hdl *hdl = (struct udtp_hdl *)handle;
    socklen_t socklen = sizeof(struct sockaddr_in);

    if (os_mutex_pend(&hdl->mutex, 0)) {
        printf("%s %d->error.\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (hdl->state == UDTP_OPEN) {
        if (dest_addr) {
            ret = sock_recvfrom(hdl->sock_hdl, buf, len, flag, (struct sockaddr *)dest_addr, &socklen);
        } else {
            ret = sock_recvfrom(hdl->sock_hdl, buf, len, flag, NULL, NULL);
        }
    }
    os_mutex_post(&hdl->mutex);

    return ret;
}

void udtp_set_send_thread_prio_stksize(void *hdl, u32 prio, u32 stk_size)
{
    ((struct udtp_hdl *)hdl)->send_thread_prio = prio;
    ((struct udtp_hdl *)hdl)->send_thread_stksize = stk_size;
}
void udtp_set_recv_thread_prio_stksize(void *hdl, u32 prio, u32 stk_size)
{
    ((struct udtp_hdl *)hdl)->recv_thread_prio = prio;
    ((struct udtp_hdl *)hdl)->recv_thread_stksize = stk_size;
}

inline void udtp_set_recvbuf(void *hdl, u8 *recvbuf, u32 recvbuf_len)
{
    ((struct udtp_hdl *)hdl)->recv_buf = recvbuf;
    ((struct udtp_hdl *)hdl)->recv_buf_len = recvbuf_len;
}

inline void udtp_set_recv_timeout(void *hdl, u32 millsec)
{
    sock_set_recv_timeout(((struct udtp_hdl *)hdl)->sock_hdl, millsec);
}

inline int udtp_recv_timeout(void *hdl)
{
    return sock_recv_timeout(((struct udtp_hdl *)hdl)->sock_hdl);
}
