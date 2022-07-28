#include "asm/cache.h"
#include "os_api.h"
#include "os_compat.h"
#include "list.h"
#include "dtp.h"
#include "sock_api.h"
#include "string.h"

#define DEFAULT_AUTO_CONNECT_INTERVAL 5

#define MAX_TCP_PKT 1460

#define DEFAULT_DTP_CLI_THREAD_PRIO          20
#define DEFAULT_DTP_CLI_THREAD_STK_SIZE       1000

struct dtp_cli_t {
    OS_MUTEX mutex;
    struct list_head hdl_list_head;
    u32 cli_thread_prio;
    u32 cli_thread_stk_size;
};
static struct dtp_cli_t dtp_cli;


enum dtp_state {
    NOT_CONNECT,
    CONNECTED_ING,
    CONNECTED_SUCC = 0x5a6b7c8d,
    CONNECTED_FAIL,
    CLIENT_CLOSE,
    SERVER_CLOSE,
};

struct dtp_hdl {
    struct list_head entry;
    OS_MUTEX wr_mtx;
    OS_MUTEX rd_mtx;
    int dtp_mode;
    int recv_thread_pid;
    int send_thread_pid;
    int cli_connect_thread_pid;
    OS_SEM send_sem;
    void *sock_hdl;
    int recv_flag;
    int send_flag;
    enum dtp_state state;
    struct sockaddr_in dest_addr;
    void *recv_buf;
    u32 recv_buf_len;
    int connect_interval;
    u16 local_port;
    int (*cb_func)(void *hdl, enum dtp_cli_msg_type type, char *buf, u32 len, void *priv);
    void *priv;
};

struct reg_struct {
    struct sockaddr_in dest_addr;
    int (*cb_func)(void *hdl, enum dtp_cli_msg_type type, char *buf, u32 len, void *priv);
    void *priv;
    int dtp_mode;
    void *hdl;
};

struct dtp_grp_mbr {
    struct list_head entry;
    struct dtp_hdl *hdl;
};

inline void dtp_cli_set_recvbuf(void *hdl, u8 *recvbuf, u32 recvbuf_len)
{
    ((struct dtp_hdl *)hdl)->recv_buf = recvbuf;
    ((struct dtp_hdl *)hdl)->recv_buf_len = recvbuf_len;
}

void dtp_cli_set_local_port(void *hdl, u16 port)
{
    ((struct dtp_hdl *)hdl)->local_port = port;
}

void dtp_cli_set_connect_interval(void *hdl, int sec)
{
    ((struct dtp_hdl *)hdl)->connect_interval = sec;
}
void dtp_cli_set_connect_to(void *hdl, int sec)
{
    sock_set_connect_to(((struct dtp_hdl *)hdl)->sock_hdl, sec);
}


static void dtp_cli_recv_thread(void *arg)
{
    struct dtp_hdl *hdl = (struct dtp_hdl *)arg;

    int ret, len;
    char *p_buf;
    u8 _data_buf[MAX_TCP_PKT + CACHE_LINE_COUNT];
    char *data_buf = _data_buf;

    while (1) {
#if 0
        ret = sock_select_rdset(hdl->sock_hdl);
        if (ret < 0) {
            printf("%s %d->  sock_select_rdset: %d\n", __func__, __line__, ret);
            if (hdl->state != CLIENT_CLOSE) {
                hdl->state = SERVER_CLOSE;
            }

            goto EXIT;
        }
#endif

        ret = hdl->cb_func(hdl, DTP_CLI_BEFORE_RECV, data_buf, MAX_TCP_PKT, hdl->priv);
        if (ret < 0) {
            if (hdl->state != SERVER_CLOSE) {
                hdl->state = CLIENT_CLOSE;
            }
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

        ret = sock_recv(hdl->sock_hdl, p_buf, len, hdl->recv_flag);
        if (ret <= 0 && !sock_recv_timeout(hdl->sock_hdl)) {
            printf("%s %d->  recv: %d\n", __FUNCTION__, __LINE__, ret);
            if (hdl->state != CLIENT_CLOSE) {
                hdl->state = SERVER_CLOSE;
            }

            goto EXIT;
        }
        if (ret > 0) {
            ret = hdl->cb_func(hdl, DTP_CLI_RECV_DATA, p_buf, ret, hdl->priv);
            if (ret < 0) {
                hdl->state = CLIENT_CLOSE;
                goto EXIT;
            }
        }
    }

EXIT:
    dtp_cli_unreg(hdl);

    return;
}

static void dtp_cli_send_thread(void *arg)
{
    struct dtp_hdl *hdl = (struct dtp_hdl *)arg;

    int ret, len;
    u8 _data_buf[MAX_TCP_PKT + CACHE_LINE_COUNT];
    char *data_buf = _data_buf;

SEND_AG:
    os_sem_pend(&((struct dtp_hdl *)hdl)->send_sem, 0);

    if (hdl->state == CLIENT_CLOSE || hdl->state == SERVER_CLOSE) {
        goto EXIT;
    }

    while (1) {
        len = hdl->cb_func(hdl, DTP_CLI_SEND_DATA, data_buf, MAX_TCP_PKT, hdl->priv);
        if (len == 0) {
            goto SEND_AG;
        } else if (len < 0) {
            hdl->state = CLIENT_CLOSE;
            goto EXIT;
        }

        ret = sock_send(hdl->sock_hdl, data_buf, len, hdl->send_flag);
        if (ret <= 0 && !sock_send_timeout(hdl->sock_hdl)) {
            printf("%s %d->  send: %d\n", __FUNCTION__, __LINE__, ret);
            if (hdl->state != CLIENT_CLOSE) {
                hdl->state = SERVER_CLOSE;
            }
            goto EXIT;
        }
    }

EXIT:

    dtp_cli_unreg(hdl);
}

static int dtp_sock_cb(enum sock_api_msg_type type, void *priv)
{
    int ret;
    struct dtp_hdl *hdl = (struct dtp_hdl *)priv;

    if (hdl->state == CLIENT_CLOSE || hdl->state == SERVER_CLOSE) {
        return -1;
    }

    switch (type) {
    case SOCK_SEND_TO:
        ret = hdl->cb_func(hdl, DTP_CLI_SEND_TO, NULL, 0, hdl->priv);
        if (ret) {
            hdl->state == CLIENT_CLOSE;
        }
        break;
    case SOCK_RECV_TO:
        ret = hdl->cb_func(hdl, DTP_CLI_RECV_TO, NULL, 0, hdl->priv);
        if (ret) {
            hdl->state == CLIENT_CLOSE;
        }
        break;
    case SOCK_CONNECT_FAIL:
        hdl->connect_interval = hdl->cb_func(hdl, DTP_CLI_CONNECT_FAIL, NULL, 0, hdl->priv);
        ret = -1;
        break;
    case SOCK_CONNECT_SUCC:
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

static int _dtp_cli_connect(struct dtp_hdl *hdl)
{
    int ret, delay, connect_cnt;
    struct sockaddr_in local_addr;

RE_CONNECT:

    hdl->sock_hdl = sock_reg(AF_INET, SOCK_STREAM, 0, dtp_sock_cb, hdl);
    if (hdl->sock_hdl == NULL) {
        printf("%s %d->Error in sock_reg()\n", __FUNCTION__, __LINE__);
        os_time_dly(300);
        goto RE_CONNECT;
    }
    delay = sock_get_recv_timeout(hdl->sock_hdl);

    hdl->local_port = hdl->cb_func(hdl, DTP_CLI_SET_BIND_PORT, NULL, 0, hdl->priv);
    if (hdl->local_port) {
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(hdl->local_port);
        local_addr.sin_addr.s_addr = INADDR_ANY;
        ret = sock_bind(hdl, (struct sockaddr *)&local_addr, sizeof(struct sockaddr));
        if (ret < 0) {
            ret = hdl->cb_func(hdl, DTP_CLI_BIND_FAIL, NULL, 0, hdl->priv);
            if (ret) {
                printf("%s %d->Error in sock_bind()\n", __FUNCTION__, __LINE__);
                goto EXIT;
            }
            goto RE_CONNECT;
        }
    }

    sock_set_connect_to(hdl->sock_hdl, hdl->cb_func(hdl, DTP_CLI_SET_CONNECT_TO, NULL, 0, hdl->priv));
    if (sock_connect(hdl->sock_hdl, (struct sockaddr *)&hdl->dest_addr, sizeof(struct sockaddr)) < 0) {
        printf("%s %d->Error in sock_connect->%d\n", __FUNCTION__, __LINE__, hdl->connect_interval);
        sock_unreg(hdl->sock_hdl);
        hdl->sock_hdl = NULL;

        if (hdl->connect_interval < 0) {
            goto EXIT;
        }

        if (hdl->connect_interval == 0) {
            hdl->connect_interval = DEFAULT_AUTO_CONNECT_INTERVAL;
        }
        connect_cnt = hdl->connect_interval * 1000 / delay;
        delay /= 10;
        printf("waiting %dS reconnect.\n", hdl->connect_interval);
        while (connect_cnt--) {
            if (hdl->state == CLIENT_CLOSE || hdl->state == SERVER_CLOSE) {
                goto EXIT;
            }
            os_time_dly(delay);
        }
        goto RE_CONNECT;
    }

    hdl->state = CONNECTED_SUCC;
    hdl->cb_func(hdl, DTP_CLI_CONNECT_SUCC, NULL, 0, hdl->priv);

    if (hdl->dtp_mode & DTP_WRITE) {
        thread_fork("dtp_cli_send_thread", dtp_cli.cli_thread_prio, dtp_cli.cli_thread_stk_size, 0, &((struct dtp_hdl *)hdl)->send_thread_pid, dtp_cli_send_thread, (void *)hdl);
    }

    if (hdl->dtp_mode & DTP_READ) {
        thread_fork("dtp_cli_recv_thread", dtp_cli.cli_thread_prio, dtp_cli.cli_thread_stk_size, 0, &((struct dtp_hdl *)hdl)->recv_thread_pid, dtp_cli_recv_thread, (void *)hdl);
    }

    return 0;

EXIT:
    puts("_dtp_cli_connect fail!\n");
    return -1;
}

static void *_dtp_cli_reg(struct dtp_hdl *hdl, struct sockaddr_in *dest_addr, int (*cb_func)(void *hdl, enum dtp_cli_msg_type type, char *buf, u32 len, void *priv), void *priv, int dtp_mode)
{
    if (hdl) {
        goto REREG_LABEL;
    }

    struct list_head *pos;

    if (dest_addr == NULL || cb_func == NULL || !(dtp_mode & (DTP_NONE | DTP_WRITE | DTP_READ))) {
        printf("dtp_cli_reg parm err->dest_addr<0x%x>,cb_func<0x%x>, dtp_mode<0x%x>\n", dest_addr, cb_func, dtp_mode);
        return NULL;
    }

    os_mutex_pend(&dtp_cli.mutex, 0);
    list_for_each(pos, &dtp_cli.hdl_list_head) {
        hdl = list_entry(pos, struct dtp_hdl, entry);

        if (dest_addr->sin_port == hdl->dest_addr.sin_port && dest_addr->sin_addr.s_addr == hdl->dest_addr.sin_addr.s_addr) {
            os_mutex_post(&dtp_cli.mutex);

            printf("dtp_cli_reg repeat hdl=0x%x\n", hdl);
            return hdl;
        }
    }
    os_mutex_post(&dtp_cli.mutex);

    hdl = (struct dtp_hdl *)malloc(sizeof(struct dtp_hdl));
    if (hdl == NULL) {
        goto EXIT;
    }

REREG_LABEL:
    memset(hdl, 0, sizeof(struct dtp_hdl));
    memcpy(&hdl->dest_addr, dest_addr, sizeof(struct sockaddr_in));
    hdl->dest_addr.sin_family = AF_INET;
    hdl->state = NOT_CONNECT;
    hdl->dtp_mode = dtp_mode;
    hdl->cb_func = cb_func;
    hdl->priv = priv;

    if (hdl->dtp_mode & DTP_WRITE) {
        os_sem_create(&hdl->send_sem, 0);
    }

    os_mutex_create(&hdl->wr_mtx);
    os_mutex_create(&hdl->rd_mtx);

    if (hdl->dtp_mode & DTP_CONNECT_NON_BLOCK) {
        thread_fork("_dtp_cli_connect", 0, 0, 0, &((struct dtp_hdl *)hdl)->cli_connect_thread_pid, (void (*)(void *))_dtp_cli_connect, (void *)hdl);
    } else {
        if (_dtp_cli_connect(hdl)) {
            goto EXIT;
        }
    }

    os_mutex_pend(&dtp_cli.mutex, 0);
    list_add_tail(&hdl->entry, &dtp_cli.hdl_list_head);
    os_mutex_post(&dtp_cli.mutex);

    return hdl;

EXIT:

    printf("dtp_cli_reg FAIL!!-<0x%x><%d>-\n", hdl->dest_addr.sin_addr.s_addr, hdl->dest_addr.sin_port);

    if (hdl) {
        free(hdl);
    }

    return NULL;
}

inline void *dtp_cli_reg(struct sockaddr_in *dest_addr, int (*cb_func)(void *hdl, enum dtp_cli_msg_type type, char *buf, u32 len, void *priv), void *priv, int dtp_mode)
{
    return _dtp_cli_reg(NULL, dest_addr, cb_func, priv, dtp_mode);
}

static void _dtp_cli_rereg(struct reg_struct *reg_struct)
{
    _dtp_cli_reg(reg_struct->hdl, &reg_struct->dest_addr, reg_struct->cb_func, reg_struct->priv, reg_struct->dtp_mode);
    free(reg_struct);
}

static void dtp_cli_rereg(struct dtp_hdl *hdl)
{
    struct reg_struct *reg_struct = (struct reg_struct *)calloc(1, sizeof(struct reg_struct));

    memcpy(&reg_struct->dest_addr, &hdl->dest_addr, sizeof(struct sockaddr_in));
    reg_struct->hdl = hdl;
    reg_struct->dtp_mode = hdl->dtp_mode;
    reg_struct->cb_func = hdl->cb_func;
    reg_struct->priv = hdl->priv;

    puts("<dtp_cli_rereg>\n");
    thread_rpc(0, NULL, 0, 0, (void (*)(void *))_dtp_cli_rereg, reg_struct);
}

void dtp_cli_unreg(void *handle)
{
    if (!handle) {
        return;
    }
    int ret;
    struct list_head *pos, *node;
    struct dtp_hdl *hdl = (struct dtp_hdl *)handle;
    struct dtp_hdl *hdl_find;
    bool find = 0;

    os_mutex_pend(&dtp_cli.mutex, 0);
    list_for_each_safe(pos, node, &dtp_cli.hdl_list_head) {
        hdl_find = list_entry(pos, struct dtp_hdl, entry);
        if (hdl_find == hdl) {
            find = 1;
            list_del(&hdl->entry);
            break;
        }
    }
    os_mutex_post(&dtp_cli.mutex);

    if (!find) {
        return;
    }

    if (hdl->state == SERVER_CLOSE) {
        ret = hdl->cb_func(hdl, DTP_CLI_SRV_DISCONNECT, NULL, 0, hdl->priv);
    } else {
        hdl->state = CLIENT_CLOSE;
        hdl->cb_func(hdl, DTP_CLI_UNREG, NULL, 0, hdl->priv);
    }

    os_mutex_pend(&hdl->wr_mtx, 0);
    os_mutex_post(&hdl->wr_mtx);
    os_mutex_del(&hdl->wr_mtx, 1);

    os_mutex_pend(&hdl->rd_mtx, 0);
    os_mutex_post(&hdl->rd_mtx);
    os_mutex_del(&hdl->rd_mtx, 1);

    if (hdl->dtp_mode & DTP_CONNECT_NON_BLOCK) {
        thread_kill(&hdl->cli_connect_thread_pid, KILL_WAIT);
    }

    if (hdl->dtp_mode & DTP_WRITE) {
        if (hdl->send_sem.OSEventType == OS_EVENT_TYPE_SEM) {
            os_sem_del(&hdl->send_sem, 1);
        }
        thread_kill(&hdl->send_thread_pid, KILL_WAIT);
    }

    if (hdl->dtp_mode & DTP_READ) {
        thread_kill(&hdl->recv_thread_pid, KILL_WAIT);
    }

    sock_unreg(hdl->sock_hdl);
    if (hdl->state == CLIENT_CLOSE) {
        memset(hdl, 0xff, sizeof(struct dtp_hdl));
        free(hdl);
    } else if (ret == 0) {
        dtp_cli_rereg(hdl);
    }
}

#if 0
void dtp_cli_uninit(void)
{
    struct list_head *pos;
    struct list_head *n;
    struct dtp_hdl *hdl;

    while (1) {
        os_mutex_pend(&dtp_cli.mutex, 0, 0);

        if (list_empty(&dtp_cli.hdl_list_head)) {
            os_mutex_post(&dtp_cli.mutex);
            break;
        }
        hdl = list_first_entry(&dtp_cli.hdl_list_head, struct dtp_hdl, entry);
        list_del(&hdl->entry);
        os_mutex_post(&dtp_cli.mutex);

        hdl->cb_func(hdl, DTP_CLI_SRV_DISCONNECT, NULL, 0, hdl->priv);

        hdl->state = CLIENT_CLOSE;
        if (hdl->dtp_mode & DTP_WRITE) {
            os_sem_del(&hdl->send_sem, 1);
            thread_kill(&hdl->send_thread_pid, KILL_WAIT);
        }

        if (hdl->dtp_mode & DTP_READ) {
            thread_kill(&hdl->recv_thread_pid, KILL_WAIT);
        }

        if (hdl->sock_hdl) {
            sock_unreg(hdl->sock_hdl);
        }
        free(hdl);
    }

    os_mutex_del(&dtp_cli.mutex, 1);

    return 0;
}
#endif


int dtp_cli_init(void)
{
    memset(&dtp_cli, 0, sizeof(struct dtp_cli_t));

    INIT_LIST_HEAD(&dtp_cli.hdl_list_head);

    if (os_mutex_create(&dtp_cli.mutex)) {
        goto EXIT;
    }

    dtp_cli.cli_thread_prio = DEFAULT_DTP_CLI_THREAD_PRIO;
    dtp_cli.cli_thread_stk_size = DEFAULT_DTP_CLI_THREAD_STK_SIZE;

    return 0;

EXIT:
    return -1;
}

void dtp_cli_set_thread_prio_stksize(u32 prio, u32 stk_size)
{
    dtp_cli.cli_thread_prio = prio;
    dtp_cli.cli_thread_stk_size = stk_size;
}

int dtp_cli_recv(void *handle, char *buf, u32 len, int flag)
{
    int ret = -1;
    struct dtp_hdl *hdl = (struct dtp_hdl *)handle;

    if (!hdl) {
        return -1;
    }

    if (os_mutex_pend(&hdl->rd_mtx, 0)) {
        return -1;
    }
    if (hdl->state == CONNECTED_SUCC) {
        ret = sock_recv(hdl->sock_hdl, buf, len, flag);
        if (ret <= 0 && !sock_recv_timeout(hdl->sock_hdl)) {
            if (hdl->state != CLIENT_CLOSE) {
                hdl->state = SERVER_CLOSE;
            }
        }
    }
    os_mutex_post(&hdl->rd_mtx);

    return ret;
}

void dtp_cli_set_recv_flag(void *hdl, int flag)
{
    ((struct dtp_hdl *)hdl)->recv_flag = flag;
}
void dtp_cli_set_send_flag(void *hdl, int flag)
{
    ((struct dtp_hdl *)hdl)->send_flag = flag;
}

int dtp_cli_send(void *handle)
{
    int ret = -1;
    struct dtp_hdl *hdl = (struct dtp_hdl *)handle;

    if (!hdl) {
        return -1;
    }

    if (os_mutex_pend(&hdl->wr_mtx, 0)) {
        return -1;
    }
    if ((hdl->dtp_mode & DTP_WRITE) && hdl->state == CONNECTED_SUCC) {
        os_sem_set(&((struct dtp_hdl *)hdl)->send_sem, 0);
        os_sem_post(&((struct dtp_hdl *)hdl)->send_sem);

        ret = 0;
    }
    os_mutex_post(&hdl->wr_mtx);

    return ret;
}

int dtp_cli_send_buf(void *handle, char *buf, u32 len, int flag)
{
    int ret = -1;
    struct dtp_hdl *hdl = (struct dtp_hdl *)handle;

    if (!hdl) {
        return -1;
    }

    if (os_mutex_pend(&hdl->wr_mtx, 0)) {
        return -1;
    }
    if (hdl->state == CONNECTED_SUCC) {
        ret = sock_send(hdl->sock_hdl, buf, len, flag);

        if (ret <= 0 && !sock_send_timeout(hdl->sock_hdl)) {
            if (hdl->state != CLIENT_CLOSE) {
                hdl->state = SERVER_CLOSE;
            }
        }
    }
    os_mutex_post(&hdl->wr_mtx);

    return ret;
}

inline void dtp_cli_set_recv_timeout(void *hdl, u32 millsec)
{
    return sock_set_recv_timeout(((struct dtp_hdl *)hdl)->sock_hdl, millsec);
}

inline int dtp_cli_recv_timeout(void *hdl)
{
    return sock_recv_timeout(((struct dtp_hdl *)hdl)->sock_hdl);
}
inline int dtp_cli_set_send_timeout(void *hdl, u32 millsec)
{
    sock_set_send_timeout(((struct dtp_hdl *)hdl)->sock_hdl, millsec);
    return 0;
}

inline int dtp_cli_send_timeout(void *hdl)
{
    return sock_send_timeout(((struct dtp_hdl *)hdl)->sock_hdl);
}

struct sockaddr_in *dtp_cli_get_hdl_addr(void *handle)
{
    if (!handle) {
        return NULL;
    }

    return &((struct dtp_hdl *)handle)->dest_addr;
}

void *dtp_cli_grp_create(void)
{
//    struct list_head *grp_head;
//    grp_head = malloc(sizeof(struct list_head));
//
//    if(!grp_head)
//        return NULL;
//
//    INIT_LIST_HEAD(grp_head);
//
//    return grp_head;
}

void dtp_cli_grp_del(void *srv_grp)
{
//	struct list_head *pos;
//	struct list_head *n;
//    struct dtp_grp_mbr *mbr;
//
//    if(srv_grp==NULL)
//        return;
//
//    os_mutex_pend(&dtp_cli.mutex, 0, 0);
//    list_for_each_safe(pos, n, (struct list_head *)srv_grp)
//    {
//        mbr = list_entry(pos,struct dtp_grp_mbr, entry);
//
//        list_del(pos);
//        free(mbr);
//    }
//    free(srv_grp);
//    os_mutex_post(&dtp_cli.mutex);
}

int dtp_cli_grp_add(void *srv_grp, void *hdl)
{
//    struct dtp_grp_mbr *mbr;
//
//    mbr = calloc(sizeof(struct dtp_grp_mbr), 1);
//    if(mbr==NULL)
//        return -1;
//
//    mbr->hdl = hdl;
//    os_mutex_pend(&dtp_cli.mutex,0,0);
//    list_add_tail(&mbr->entry, srv_grp);
//    os_mutex_post(&dtp_cli.mutex);
//
//    return 0;
}

int dtp_cli_grp_connect(void *srv_grp, bool wait_complete)
{
//    struct list_head *pos;
//    struct dtp_grp_mbr *mbr;
//
//    os_mutex_pend(&dtp_cli.mutex, 0, 0);
//    list_for_each(pos, (struct list_head *)srv_grp)
//    {
//        mbr = list_entry(pos, struct dtp_grp_mbr, entry);
//        if(dtp_cli_connect(mbr->hdl, wait_complete))
//        {
//            printf("dtp_cli_grp_connect err 0x%x, 0x%x\n", mbr, mbr->hdl);
//            os_mutex_post(&dtp_cli.mutex);
//            return -1;
//        }
//    }
//    os_mutex_post(&dtp_cli.mutex);
//
//    return 0;
}

int dtp_cli_grp_send(void *srv_grp)
{
//    struct list_head *pos;
//    struct dtp_grp_mbr *mbr;
//
//    os_mutex_pend(&dtp_cli.mutex, 0, 0);
//    list_for_each(pos, (struct list_head *)srv_grp)
//    {
//        mbr = list_entry(pos, struct dtp_grp_mbr, entry);
//        if(dtp_cli_send(mbr->hdl))
//        {
//            printf("dtp_cli_grp_send err 0x%x, 0x%x\n", mbr, mbr->hdl);
//            os_mutex_post(&dtp_cli.mutex);
//            return -1;
//        }
//    }
//    os_mutex_post(&dtp_cli.mutex);
//
//    return 0;
}
