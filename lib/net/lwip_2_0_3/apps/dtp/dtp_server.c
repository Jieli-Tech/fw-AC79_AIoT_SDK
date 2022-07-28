#include "asm/cache.h"
#include "os_api.h"
#include "os_compat.h"
#include "dtp.h"
#include "list.h"
#include <string.h>
#include <sock_api.h>

#define MAX_TCP_PKT 1460

#define MAX_LISTEN_BACKLOG 0xff

#define DTP_SRV_ACCEPT_THREAD_PRIO          20
#define DTP_SRV_ACCEPT_THREAD_STK_SIZE       2000

#define DEFAULT_DTP_SRV_SEND_THREAD_PRIO          20
#define DEFAULT_DTP_SRV_SEND_THREAD_STK_SIZE       2000

#define DEFAULT_DTP_SRV_RECV_THREAD_PRIO          20
#define DEFAULT_DTP_SRV_RECV_THREAD_STK_SIZE       2000

enum dtp_cli_state {
    CLIENT_CONNECTED = 0x1a3b5c7d,
    CLIENT_CLOSE,
    SERVER_CLOSE,
};

enum dtp_hdl_state {
    SRV_ACCEPT,
    SRV_CLOSE,
};

struct dtp_hdl {
    struct list_head entry;
    OS_MUTEX mutex;
    struct list_head cli_list_head;
    int dtp_mode;
    int accept_thread_pid;
    enum dtp_hdl_state state;
    void *sock_hdl;
    int recv_flag;
    int send_flag;
    u16 port;
    void *recv_buf;
    u32 recv_buf_len;
    int (*cb_func)(struct dtp_hdl *hdl, void *cli, enum dtp_srv_msg_type type, char *buf, u32 len, void *priv);
    void *priv;
    u32 send_thread_prio;
    u32 send_thread_stksize;
    u32 recv_thread_prio;
    u32 recv_thread_stksize;
    u8_t backlog;
    u8_t max_backlog;
};

struct dtp_cli_t {
    struct list_head entry;
    OS_MUTEX wr_mtx;
    OS_MUTEX rd_mtx;
    struct sockaddr_in dest_addr;
    struct dtp_hdl *dtp_hdl;
    void *sock_hdl;
    OS_SEM send_sem;
    enum dtp_cli_state state;
    int recv_thread_pid;
    int send_thread_pid;
};

struct dtp_srv_t {
    struct list_head hdl_list_head;
    OS_MUTEX mtx;
};
static struct dtp_srv_t dtp_srv;

inline void dtp_srv_set_recvbuf(void *hdl, u8 *recvbuf, u32 recvbuf_len)
{
    ((struct dtp_hdl *)hdl)->recv_buf = recvbuf;
    ((struct dtp_hdl *)hdl)->recv_buf_len = recvbuf_len;
}

void dtp_srv_set_recv_flag(void *hdl, int flag)
{
    ((struct dtp_hdl *)hdl)->recv_flag = flag;
}
void dtp_srv_set_send_flag(void *hdl, int flag)
{
    ((struct dtp_hdl *)hdl)->send_flag = flag;
}

struct sockaddr_in *dtp_srv_get_cli_addr(void *cli)
{
    if (!cli) {
        return NULL;
    }

    return &((struct dtp_cli_t *)cli)->dest_addr;
}

static void dtp_srv_recv_thread(void *arg)
{
    struct dtp_cli_t *cli_hdl = (struct dtp_cli_t *)arg;

    int ret, len;
    char *p_buf;
    u8 _data_buf[MAX_TCP_PKT + CACHE_LINE_COUNT];
    char *data_buf = _data_buf;

    while (1) {
#if 0
        ret = sock_select_rdset(cli_hdl->sock_hdl);
        if (ret < 0) {
            printf("%s %d->  sock_select_rdset: %d\n", __FUNCTION__, __LINE__, ret);

            if (cli_hdl->state != SERVER_CLOSE) {
                cli_hdl->state = CLIENT_CLOSE;
            }

            goto EXIT;
        }
#endif

        ret = cli_hdl->dtp_hdl->cb_func(cli_hdl->dtp_hdl, (void *)cli_hdl, DTP_SRV_BEFORE_RECV, data_buf, MAX_TCP_PKT, cli_hdl->dtp_hdl->priv);
        if (ret < 0) {
            if (cli_hdl->state != CLIENT_CLOSE) {
                cli_hdl->state = SERVER_CLOSE;
            }
            goto EXIT;
        }

        if (cli_hdl->dtp_hdl->recv_buf) {
            p_buf = cli_hdl->dtp_hdl->recv_buf;
        } else {
            p_buf = data_buf;
        }

        if (cli_hdl->dtp_hdl->recv_buf_len) {
            len = cli_hdl->dtp_hdl->recv_buf_len;
        } else {
            len = sizeof(data_buf);
        }

        ret = sock_recv(cli_hdl->sock_hdl, p_buf, len, cli_hdl->dtp_hdl->recv_flag);
        if (ret <= 0 && !sock_recv_timeout(cli_hdl->sock_hdl)) {
            printf("%s %d->  recv: %d\n", __FUNCTION__, __LINE__, ret);

            if (cli_hdl->state != SERVER_CLOSE) {
                cli_hdl->state = CLIENT_CLOSE;
            }
            goto EXIT;
        } else if (ret > 0) {
            ret = cli_hdl->dtp_hdl->cb_func(cli_hdl->dtp_hdl, (void *)cli_hdl, DTP_SRV_RECV_DATA, p_buf, ret, cli_hdl->dtp_hdl->priv);
            if (ret < 0) {
                cli_hdl->state = SERVER_CLOSE;
                goto EXIT;
            }
        }
    }

EXIT:

    dtp_srv_disconnect_cli(cli_hdl->dtp_hdl, cli_hdl);
}

static void dtp_srv_send_thread(void *arg)
{
    struct dtp_cli_t *cli_hdl = (struct dtp_cli_t *)arg;

    int ret, len;
    u8 _data_buf[MAX_TCP_PKT + CACHE_LINE_COUNT];
    char *data_buf = _data_buf;

SEND_AG:
    os_sem_pend(&cli_hdl->send_sem, 0);
    if (cli_hdl->state == CLIENT_CLOSE || cli_hdl->state == SERVER_CLOSE) {
        goto EXIT;
    }

    while (1) {
        len = cli_hdl->dtp_hdl->cb_func(cli_hdl->dtp_hdl, (void *)cli_hdl, DTP_SRV_SEND_DATA, data_buf, MAX_TCP_PKT, cli_hdl->dtp_hdl->priv);
        if (len == 0) {
            goto SEND_AG;
        } else if (len < 0) {
            cli_hdl->state == SERVER_CLOSE;
            goto EXIT;;
        }

        ret = sock_send(cli_hdl->sock_hdl, data_buf, len, cli_hdl->dtp_hdl->send_flag);
        if (ret <= 0 && !sock_send_timeout(cli_hdl->sock_hdl)) {
            printf("%s %d->sock_send: %d\n", __FUNCTION__, __LINE__, ret);

            if (cli_hdl->state != SERVER_CLOSE) {
                cli_hdl->state = CLIENT_CLOSE;
            }
            goto EXIT;
        }
    }

EXIT:

    dtp_srv_disconnect_cli(cli_hdl->dtp_hdl, cli_hdl);

    return;
}

int dtp_srv_send(void *cli)
{
    int ret = -1;
    struct dtp_cli_t *cli_hdl = (struct dtp_cli_t *)cli;

    if (!cli_hdl) {
        return -1;
    }

    if (os_mutex_pend(&cli_hdl->wr_mtx, 0)) {
        printf("%s %d->dtp_srv_disconnected.\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (cli_hdl->state == CLIENT_CONNECTED) {
        os_sem_set(&cli_hdl->send_sem, 0);
        os_sem_post(&cli_hdl->send_sem);

        ret = 0;
    }
    os_mutex_post(&cli_hdl->wr_mtx);

    return  ret;
}

inline int dtp_srv_send_buf(void *cli, char *buf, u32 len, int flag)
{
    int ret = -1;
    struct dtp_cli_t *cli_hdl = (struct dtp_cli_t *)cli;

    if (!cli_hdl) {
        return -1;
    }

    if (os_mutex_pend(&cli_hdl->wr_mtx, 0)) {
        printf("%s %d->dtp_srv_disconnected.\n", __FUNCTION__, __LINE__);
        return -1;
    }

    if (cli_hdl->state == CLIENT_CONNECTED) {
        ret = sock_send(cli_hdl->sock_hdl, buf, len, flag);
        if (ret <= 0 && !sock_send_timeout(cli_hdl->sock_hdl)) {
            if (cli_hdl->state != SERVER_CLOSE) {
                cli_hdl->state = CLIENT_CLOSE;
            }
        }
    }
    os_mutex_post(&cli_hdl->wr_mtx);

    return  ret;
}

inline int dtp_srv_recv(void *cli, char *buf, u32 len, int flag)
{
    int ret = -1;
    struct dtp_cli_t *cli_hdl = (struct dtp_cli_t *)cli;

    if (!cli_hdl) {
        return -1;
    }

    if (os_mutex_pend(&cli_hdl->rd_mtx, 0)) {
        printf("%s %d->dtp_srv_disconnected.\n", __FUNCTION__, __LINE__);
        return -1;
    }
    if (cli_hdl->state == CLIENT_CONNECTED) {
        ret = sock_recv(cli_hdl->sock_hdl, buf, len, flag);
        if (ret <= 0 && !sock_recv_timeout(cli_hdl->sock_hdl)) {
            if (cli_hdl->state != SERVER_CLOSE) {
                cli_hdl->state = CLIENT_CLOSE;
            }
        }
    }
    os_mutex_post(&cli_hdl->rd_mtx);

    return  ret;
}

inline int dtp_srv_set_recv_timeout(void *cli, u32 millsec)
{
    sock_set_recv_timeout(((struct dtp_cli_t *)cli)->sock_hdl, millsec);
    return 0;
}

inline int dtp_srv_recv_timeout(void *cli)
{
    return sock_recv_timeout(((struct dtp_cli_t *)cli)->sock_hdl);
}

inline int dtp_srv_set_send_timeout(void *cli, u32 millsec)
{
    sock_set_send_timeout(((struct dtp_cli_t *)cli)->sock_hdl, millsec);
    return 0;
}

inline int dtp_srv_send_timeout(void *cli)
{
    return sock_send_timeout(((struct dtp_cli_t *)cli)->sock_hdl);
}

static int cli_sock_cb(enum sock_api_msg_type type, void *priv)
{
    int ret;
    struct dtp_cli_t *cli_hdl = (struct dtp_cli_t *)priv;

    if (cli_hdl->state == CLIENT_CLOSE || cli_hdl->state == SERVER_CLOSE) {
        return -1;
    }

    switch (type) {
    case SOCK_SEND_TO:
        ret = cli_hdl->dtp_hdl->cb_func(cli_hdl->dtp_hdl, (void *)cli_hdl, DTP_SRV_SEND_TO, NULL, 0, cli_hdl->dtp_hdl->priv);
        break;
    case SOCK_RECV_TO:
        ret = cli_hdl->dtp_hdl->cb_func(cli_hdl->dtp_hdl, (void *)cli_hdl, DTP_SRV_RECV_TO, NULL, 0, cli_hdl->dtp_hdl->priv);
        break;
    default :
        ret = 0;
        break;
    }

    if (ret) {
        cli_hdl->state = SERVER_CLOSE;
        return -1;
    }

    return 0;
}


static void dtp_srv_accept_thread(void *arg)
{
    struct dtp_hdl *hdl = (struct dtp_hdl *)arg;
    int ret;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(struct sockaddr);
    struct list_head *pos;
    struct dtp_cli_t *cli_hdl = NULL;

    while (1) {
        cli_hdl = (struct dtp_cli_t *)calloc(sizeof(struct dtp_cli_t), 1);
        if (cli_hdl == NULL) {
            printf("%s %d->Error in calloc()\n", __FUNCTION__, __LINE__);
            os_time_dly(10);
            continue;
        }

        cli_hdl->sock_hdl = sock_accept(hdl->sock_hdl, (struct sockaddr *)&client_addr, &client_len, cli_sock_cb, cli_hdl);
        if (cli_hdl->sock_hdl == NULL) {
            printf("%s %d->Error in sock_accept()\n", __FUNCTION__, __LINE__);
            goto EXIT;
        }

        memcpy(&cli_hdl->dest_addr, &client_addr, sizeof(struct sockaddr_in));
        cli_hdl->dtp_hdl = hdl;
        cli_hdl->state = CLIENT_CONNECTED;

        os_mutex_create(&cli_hdl->rd_mtx);
        os_mutex_create(&cli_hdl->wr_mtx);
        os_mutex_pend(&hdl->mutex, 0);
        list_add_tail(&cli_hdl->entry, &hdl->cli_list_head);
        os_mutex_post(&hdl->mutex);

        if (hdl->dtp_mode & DTP_WRITE) {
            os_sem_create(&cli_hdl->send_sem, 0);
        }

        if (hdl->cb_func(cli_hdl->dtp_hdl, (void *)cli_hdl, DTP_SRV_CLI_CONNECTED, NULL, 0, hdl->priv) < 0) {
            puts("SRV GIVE UP.\n");

            os_mutex_del(&cli_hdl->rd_mtx, 1);
            os_mutex_del(&cli_hdl->wr_mtx, 1);
            os_mutex_pend(&hdl->mutex, 0);
            list_del(&cli_hdl->entry);
            os_mutex_post(&hdl->mutex);

            if (hdl->dtp_mode & DTP_WRITE) {
                os_sem_del(&cli_hdl->send_sem, 1);
            }

            sock_unreg(cli_hdl->sock_hdl);
            free(cli_hdl);

            continue;
        }

        printf("DTP_CLI(0x%x)(0x%x) CONNECTED !\n", client_addr.sin_addr.s_addr, client_addr.sin_port);

        while (1) {
            if (hdl->backlog < hdl->max_backlog) {
                ++hdl->backlog;

                if (hdl->dtp_mode & DTP_READ) {
                    thread_fork("dtp_srv_recv_thread", hdl->recv_thread_prio, hdl->recv_thread_stksize, 0, &cli_hdl->recv_thread_pid, dtp_srv_recv_thread, (void *)cli_hdl);
                }

                if (hdl->dtp_mode & DTP_WRITE) {
                    thread_fork("dtp_srv_send_thread", hdl->send_thread_prio, hdl->send_thread_stksize, 0, &cli_hdl->send_thread_pid, dtp_srv_send_thread, (void *)cli_hdl);
                }
                break;
            } else {
                os_time_dly(10);
                if (hdl->state == SRV_CLOSE) {
                    goto EXIT;
                }
            }
        }
    }

EXIT:
    if (cli_hdl) {
        if (cli_hdl->sock_hdl) {
            sock_unreg(cli_hdl->sock_hdl);
        }
        free(cli_hdl);
    }

    hdl->cb_func(hdl, NULL, DTP_SRV_HDL_CLOSE, NULL, 0, hdl->priv);
}

static int srv_sock_cb(enum sock_api_msg_type type, void *priv)
{
    if (((struct dtp_hdl *)priv)->state == SRV_CLOSE) {
        return -1;
    }
    return 0;
}

void *dtp_srv_reg(u16 port, int (*cb_func)(void *hdl, void *cli, enum dtp_srv_msg_type type, u8 *buf, u32 len, void *priv),  void *priv, int dtp_mode)
{
    int ret;
    struct dtp_hdl *hdl;
    struct list_head *pos;

    if (cb_func == NULL || !(dtp_mode & (DTP_NONE | DTP_WRITE | DTP_READ))) {
        printf("dtp_srv_reg parm err->cb_func<0x%x>, dtp_mode<0x%x>\n", cb_func, dtp_mode);
        return NULL;
    }

    os_mutex_pend(&dtp_srv.mtx, 0);
    list_for_each(pos, &dtp_srv.hdl_list_head) {
        hdl = list_entry(pos, struct dtp_hdl, entry);

        if (hdl->port == port) {
            os_mutex_post(&dtp_srv.mtx);
            printf("port has reg = 0x%x\n", port);
            return NULL;
        }
    }
    os_mutex_post(&dtp_srv.mtx);

    hdl = (struct dtp_hdl *)calloc(sizeof(struct dtp_hdl), 1);
    if (hdl == NULL) {
        goto EXIT;
    }

    hdl->send_thread_prio = DEFAULT_DTP_SRV_SEND_THREAD_PRIO;
    hdl->send_thread_stksize = DEFAULT_DTP_SRV_SEND_THREAD_STK_SIZE;
    hdl->recv_thread_prio = DEFAULT_DTP_SRV_RECV_THREAD_PRIO;
    hdl->recv_thread_stksize = DEFAULT_DTP_SRV_RECV_THREAD_STK_SIZE;

    hdl->state = SRV_ACCEPT;
    hdl->port = port;
    hdl->cb_func = (int (*)(struct dtp_hdl *, void *, enum dtp_srv_msg_type, char *, u32, void *))cb_func;
    hdl->priv = priv;
    hdl->dtp_mode = dtp_mode;
    INIT_LIST_HEAD(&hdl->cli_list_head);
    if (os_mutex_create(&hdl->mutex)) {
        goto EXIT;
    }

    hdl->sock_hdl = sock_reg(AF_INET, SOCK_STREAM, 0, srv_sock_cb, hdl);
    if (hdl->sock_hdl == NULL) {
        printf("%s %d->Error in sock_reg()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = htons(hdl->port);
    ret = sock_bind(hdl->sock_hdl, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
    if (ret) {
        printf("%s %d->Error in bind()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    hdl->max_backlog = hdl->cb_func(hdl, NULL, DTP_SRV_SET_MAX_BACKLOG, NULL, 0, hdl->priv);
    if (hdl->max_backlog == 0) {
        hdl->max_backlog = MAX_LISTEN_BACKLOG;
    }

    ret = sock_listen(hdl->sock_hdl, hdl->max_backlog);
    if (ret) {
        printf("%s %d->Error in listen()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    os_mutex_pend(&dtp_srv.mtx, 0);
    list_add_tail(&hdl->entry, &dtp_srv.hdl_list_head);
    os_mutex_post(&dtp_srv.mtx);

    thread_fork("dtp_srv_accept_thread", DTP_SRV_ACCEPT_THREAD_PRIO, DTP_SRV_ACCEPT_THREAD_STK_SIZE, 0, &((struct dtp_hdl *)hdl)->accept_thread_pid, dtp_srv_accept_thread, (void *)hdl);

    return hdl;

EXIT:
    if (hdl) {
        if (hdl->sock_hdl) {
            sock_unreg(hdl->sock_hdl);
        }
        free(hdl);
    }
    return NULL;
}

void dtp_srv_disconnect_cli(void *handle, void *cli)
{
    struct dtp_hdl *hdl = (struct dtp_hdl *)handle;
    struct dtp_cli_t *cli_hdl = (struct dtp_cli_t *)cli;

    if (cli_hdl == NULL || hdl == NULL) {
        return;
    }

    struct list_head *pos, *node;
    bool find = 0;

    os_mutex_pend(&hdl->mutex, 0);
    list_for_each_safe(pos, node, &hdl->cli_list_head) {
        if (list_entry(pos, struct dtp_cli_t, entry) == cli_hdl) {
            find = 1;
            list_del(&cli_hdl->entry);
            break;
        }
    }
    os_mutex_post(&hdl->mutex);
    if (!find) {
        return;
    }

    printf("%s %d....\n", __FUNCTION__, __LINE__);

    if (hdl->state == CLIENT_CLOSE) {
        hdl->cb_func(hdl, (void *)cli_hdl, DTP_SRV_CLI_DISCONNECT, NULL, 0, hdl->priv);
    } else {
        hdl->state = SERVER_CLOSE;
        hdl->cb_func(hdl, (void *)cli_hdl, DTP_SRV_SRV_DISCONNECT, NULL, 0, hdl->priv);
    }

    os_mutex_pend(&cli_hdl->wr_mtx, 0);
    os_mutex_post(&cli_hdl->wr_mtx);
    os_mutex_del(&cli_hdl->wr_mtx, 1);

    os_mutex_pend(&cli_hdl->rd_mtx, 0);
    os_mutex_post(&cli_hdl->rd_mtx);
    os_mutex_del(&cli_hdl->rd_mtx, 1);

    if (hdl->dtp_mode & DTP_WRITE) {
        os_sem_del(&cli_hdl->send_sem, 1);
        thread_kill(&cli_hdl->send_thread_pid, KILL_WAIT);
    }
    if (hdl->dtp_mode & DTP_READ) {
        thread_kill(&cli_hdl->recv_thread_pid, KILL_WAIT);
    }

    sock_unreg(cli_hdl->sock_hdl);
    memset(cli_hdl, 0xff, sizeof(struct dtp_cli_t));
    free(cli_hdl);
    --hdl->backlog;
}

void dtp_srv_disconnect_all(void *handle)
{
    if (!handle) {
        return;
    }

    struct dtp_hdl *hdl = handle;
    struct dtp_cli_t *cli_hdl;

    while (1) {
        os_mutex_pend(&hdl->mutex, 0);

        if (list_empty(&hdl->cli_list_head)) {
            os_mutex_post(&hdl->mutex);
            break;
        }
        cli_hdl = list_first_entry(&hdl->cli_list_head, struct dtp_cli_t, entry);
        list_del(&cli_hdl->entry);
        os_mutex_post(&hdl->mutex);

        if (hdl->state == CLIENT_CLOSE) {
            hdl->cb_func(hdl, (void *)cli_hdl, DTP_SRV_CLI_DISCONNECT, NULL, 0, hdl->priv);
        } else {
            hdl->state = SERVER_CLOSE;
            hdl->cb_func(hdl, (void *)cli_hdl, DTP_SRV_SRV_DISCONNECT, NULL, 0, hdl->priv);
        }

        os_mutex_pend(&cli_hdl->wr_mtx, 0);
        os_mutex_post(&cli_hdl->wr_mtx);
        os_mutex_del(&cli_hdl->wr_mtx, 1);

        os_mutex_pend(&cli_hdl->rd_mtx, 0);
        os_mutex_post(&cli_hdl->rd_mtx);
        os_mutex_del(&cli_hdl->rd_mtx, 1);

        if (cli_hdl->dtp_hdl->dtp_mode & DTP_WRITE) {
            os_sem_del(&cli_hdl->send_sem, 1);
            thread_kill(&cli_hdl->send_thread_pid, KILL_WAIT);
        }

        if (cli_hdl->dtp_hdl->dtp_mode & DTP_READ) {
            thread_kill(&cli_hdl->recv_thread_pid, KILL_WAIT);
        }

        sock_unreg(cli_hdl->sock_hdl);
        memset(cli_hdl, 0xff, sizeof(struct dtp_cli_t));
        free(cli_hdl);
        --hdl->backlog;
    }
}

void dtp_srv_unreg(void *handle)
{
    struct dtp_hdl *hdl = (struct dtp_hdl *)handle;

    if (!hdl) {
        return ;
    }

    dtp_srv_disconnect_all(hdl);

    os_mutex_pend(&dtp_srv.mtx, 0);
    list_del(&hdl->entry);
    os_mutex_post(&dtp_srv.mtx);

    hdl->state = SRV_CLOSE;
    thread_kill(&hdl->accept_thread_pid, KILL_WAIT);
    sock_unreg(hdl->sock_hdl);
    os_mutex_del(&hdl->mutex, 1);
    free(hdl);
}

void dtp_srv_uninit(void)
{
    struct dtp_hdl *hdl;

    while (1) {
        os_mutex_pend(&dtp_srv.mtx, 0);

        if (list_empty(&dtp_srv.hdl_list_head)) {
            os_mutex_post(&dtp_srv.mtx);
            break;
        }
        hdl = list_first_entry(&dtp_srv.hdl_list_head, struct dtp_hdl, entry);
        os_mutex_post(&dtp_srv.mtx);

        dtp_srv_unreg(hdl);
    }

    os_mutex_del(&dtp_srv.mtx, 1);
}

int dtp_srv_init(void)
{
    memset(&dtp_srv, 0, sizeof(struct dtp_srv_t));

    INIT_LIST_HEAD(&dtp_srv.hdl_list_head);

    if (os_mutex_create(&dtp_srv.mtx)) {
        goto EXIT;
    }

    return 0;

EXIT:

    return -1;
}

void dtp_srv_set_send_thread_prio_stksize(void *hdl, u32 prio, u32 stk_size)
{
    if (!hdl) {
        return;
    }

    ((struct dtp_hdl *)hdl)->send_thread_prio = prio;
    ((struct dtp_hdl *)hdl)->send_thread_stksize = stk_size;
}

void dtp_srv_set_recv_thread_prio_stksize(void *hdl, u32 prio, u32 stk_size)
{
    if (!hdl) {
        return;
    }

    ((struct dtp_hdl *)hdl)->recv_thread_prio = prio;
    ((struct dtp_hdl *)hdl)->recv_thread_stksize = stk_size;
}
