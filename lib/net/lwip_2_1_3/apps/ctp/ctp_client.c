#include "os/os_api.h"
#include "generic/list.h"
#include "ctp/ctp.h"
#include "sock_api/sock_api.h"
#include <string.h>

#define MAX_TCP_PKT 1460

#define DEFAULT_CTP_CLI_THREAD_PRIO          20
#define DEFAULT_CTP_CLI_THREAD_STK_SIZE       1000

struct ctp_cli_t {
    OS_MUTEX mutex;
    struct list_head hdl_list_head;
    u32 cli_thread_prio;
    u32 cli_thread_stk_size;
};
static struct ctp_cli_t ctp_cli;


enum ctp_state {
    NOT_CONNECT,
    CONNECTED_SUCC,
    SERVER_CLOSE,
};

struct ctp_hdl {
    struct list_head entry;
    int id;
    int recv_thread_pid;
    void *sock_hdl;
    enum ctp_state state;
    struct sockaddr_in dest_addr;
    int (*cb_func)(void *hdl, enum ctp_cli_msg_type type, const char *topic, const char *content, void *priv);
    void *priv;
};

void ctp_cli_unreg(void *handle);
int ctp_cli_send(void *handle, const char *topic, const char *content);

static void ctp_cli_recv_thread(void *arg)
{
    struct ctp_hdl *hdl = (struct ctp_hdl *)arg;

    int ret, recv_id, cmd_num, parm_str_len;
    char recv_buf[MAX_TCP_PKT];
    u16 topic_len = 0;
    u32 content_len = 0;

    while (1) {
        ret = sock_recv(hdl->sock_hdl, recv_buf, CTP_PREFIX_LEN, MSG_WAITALL);
        if (ret != CTP_PREFIX_LEN) {
            printf("%s %d->  recv: %d\n", __FUNCTION__, __LINE__, ret);
            goto EXIT;
        }
        ret = sock_recv(hdl->sock_hdl, (char *)&topic_len, CTP_TOPIC_LEN, MSG_WAITALL);
        if (ret != CTP_TOPIC_LEN) {
            printf("%s %d->  recv: %d\n", __FUNCTION__, __LINE__, ret);
            goto EXIT;
        }

        char *topic_data = calloc(topic_len, 1);
        if (topic_data == NULL) {
            printf("%s %d->  malloc: %d\n", __FUNCTION__, __LINE__, ret);
            goto EXIT;
        }

        ret = sock_recv(hdl->sock_hdl, topic_data, topic_len, MSG_WAITALL);
        if (ret != topic_len) {
            printf("%s %d->  recv: %d\n", __FUNCTION__, __LINE__, ret);
            goto EXIT;
        }

        /* printf("####  topic_data:%s  topic_len:%d\n", topic_data, topic_len); */


        ret = sock_recv(hdl->sock_hdl, (char *)&content_len, CTP_TOPIC_CONTENT_LEN, MSG_WAITALL);
        if (ret != CTP_TOPIC_CONTENT_LEN) {
            printf("%s %d->  recv: %d\n", __FUNCTION__, __LINE__, ret);
            goto EXIT;
        }

        char *content_data = calloc(content_len, 1);
        if (content_data == NULL) {
            printf("%s %d->  malloc: %d\n", __FUNCTION__, __LINE__, ret);
            goto EXIT;
        }

        ret = sock_recv(hdl->sock_hdl, content_data, content_len, MSG_WAITALL);
        if (ret != content_len) {
            printf("%s %d->  recv: %d\n", __FUNCTION__, __LINE__, ret);
            goto EXIT;
        }

        /* printf("####  content_data:%s  content_len:%d\n", content_data, content_len); */


        if (!hdl->cb_func && hdl->cb_func(hdl, CTP_CLI_RECV_MSG, topic_data, content_len ? content_data : NULL, hdl->priv)) {
            goto EXIT;
        }
        if (topic_data != NULL) {
            free(topic_data);
        }
        if (content_data != NULL) {
            free(content_data);
        }


    }

EXIT:
    hdl->state = SERVER_CLOSE;
    ctp_cli_unreg(hdl);

    return;
}

static int ctp_sock_cb(enum sock_api_msg_type type, void *priv)
{
    struct ctp_hdl *hdl = (struct ctp_hdl *)priv;
    int ret = 0;

    if (hdl->state == SERVER_CLOSE) {
        return -1;
    }

    switch (type) {
    case SOCK_SEND_TO:
        ret = hdl->cb_func(hdl, CTP_CLI_SEND_TO, NULL, NULL, hdl->priv);
        break;
    case SOCK_RECV_TO:
        ret = hdl->cb_func(hdl, CTP_CLI_RECV_TO, NULL, NULL, hdl->priv);
        break;
    case SOCK_CONNECT_FAIL:
        ret = hdl->cb_func(hdl, CTP_CLI_CONNECT_FAIL, NULL, NULL, hdl->priv);
        break;
    default:
        break;
    }

    return ret;
}


void *ctp_cli_reg(u16_t id, struct sockaddr_in *dest_addr, int (*cb_func)(void *hdl, enum ctp_cli_msg_type type, const char *topic, const char *parm_list, void *priv), void *priv)
{
    struct ctp_hdl *hdl;
    struct list_head *pos;
    bool id_repeat = 0, ip_repeat = 0;

    if (id == 0) {
        puts("ctp_cli_reg id err\n");
        return NULL;
    }

    os_mutex_pend(&ctp_cli.mutex, 0);
    list_for_each(pos, &ctp_cli.hdl_list_head) {
        hdl = list_entry(pos, struct ctp_hdl, entry);
        if (hdl->id == id) {
            id_repeat = 1;
        }
        if (dest_addr->sin_port == hdl->dest_addr.sin_port && dest_addr->sin_addr.s_addr == hdl->dest_addr.sin_addr.s_addr) {
            ip_repeat = 1;
        }
        if (id_repeat && ip_repeat) {
            os_mutex_post(&ctp_cli.mutex);
            puts("ctp_cli_reg repeat hdl\n");
            return hdl;
        }
    }

    hdl = (struct ctp_hdl *)calloc(sizeof(struct ctp_hdl), 1);
    if (hdl == NULL) {
        os_mutex_post(&ctp_cli.mutex);
        return NULL;
    }
    list_add_tail(&hdl->entry, &ctp_cli.hdl_list_head);

    hdl->sock_hdl = sock_reg(AF_INET, SOCK_STREAM, 0, ctp_sock_cb, hdl);
    if (hdl->sock_hdl == NULL) {
        printf("%s %d->Error in socket()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    memcpy(&hdl->dest_addr, dest_addr, sizeof(struct sockaddr_in));
    hdl->dest_addr.sin_family = AF_INET;

    hdl->id = id;
    hdl->cb_func = cb_func;
    hdl->priv = priv;

    if (sock_connect(hdl->sock_hdl, (struct sockaddr *)&hdl->dest_addr, sizeof(struct sockaddr)) == -1) {
        hdl->cb_func(hdl, CTP_CLI_CONNECT_FAIL, NULL, NULL, hdl->priv);
        printf("%s %d->Error in connect()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    hdl->state = CONNECTED_SUCC;
#if 0
    if (ctp_cli_send(hdl, 0, NULL) < 0) {
        goto EXIT;
    }
#endif

    thread_fork("ctp_cli_recv_thread", ctp_cli.cli_thread_prio, ctp_cli.cli_thread_stk_size, 0, &((struct ctp_hdl *)hdl)->recv_thread_pid, ctp_cli_recv_thread, (void *)hdl);

    os_mutex_post(&ctp_cli.mutex);

    hdl->cb_func(hdl, CTP_CLI_CONNECT_SUCC, NULL, NULL, hdl->priv);

    return hdl;

EXIT:

    list_del(&hdl->entry);
    if (hdl->sock_hdl != NULL) {
        sock_unreg(hdl->sock_hdl);
    }
    free(hdl);

    os_mutex_post(&ctp_cli.mutex);

    puts("ctp_cli_reg FAIL!! \n");
    return NULL;
}

void ctp_cli_unreg(void *handle)
{
    struct ctp_hdl *hdl = (struct ctp_hdl *)handle;
    struct list_head *pos;
    bool find = 0;

    if (!hdl) {
        return;
    }

    os_mutex_pend(&ctp_cli.mutex, 0);
    list_for_each(pos, &ctp_cli.hdl_list_head) {
        if (list_entry(pos, struct ctp_hdl, entry) == hdl) {
            find = 1;
            list_del(&hdl->entry);
            break;
        }
    }
    os_mutex_post(&ctp_cli.mutex);

    if (!find) {
        return;
    }

    hdl->cb_func(hdl, CTP_CLI_DISCONNECT, NULL, NULL, hdl->priv);

    hdl->state = SERVER_CLOSE;

    thread_kill(&hdl->recv_thread_pid, KILL_WAIT);

    sock_unreg(hdl->sock_hdl);
    free(hdl);
}

void ctp_cli_uninit(void)
{
    struct list_head *pos;
    struct list_head *n;
    struct ctp_hdl *hdl;

    while (1) {
        os_mutex_pend(&ctp_cli.mutex, 0);

        if (list_empty(&ctp_cli.hdl_list_head)) {
            os_mutex_post(&ctp_cli.mutex);
            break;
        }
        hdl = list_first_entry(&ctp_cli.hdl_list_head, struct ctp_hdl, entry);
        list_del(&hdl->entry);
        os_mutex_post(&ctp_cli.mutex);

        hdl->cb_func(hdl, CTP_CLI_DISCONNECT, NULL, NULL, hdl->priv);

        hdl->state = SERVER_CLOSE;

        thread_kill(&hdl->recv_thread_pid, KILL_WAIT);

        sock_unreg(hdl->sock_hdl);
        free(hdl);
    }

    os_mutex_del(&ctp_cli.mutex, 1);

}


int ctp_cli_init(void)
{
    memset(&ctp_cli, 0, sizeof(struct ctp_cli_t));

    INIT_LIST_HEAD(&ctp_cli.hdl_list_head);

    if (os_mutex_create(&ctp_cli.mutex)) {
        goto EXIT;
    }

    ctp_cli.cli_thread_prio = DEFAULT_CTP_CLI_THREAD_PRIO;
    ctp_cli.cli_thread_stk_size = DEFAULT_CTP_CLI_THREAD_STK_SIZE;

    return 0;

EXIT:
    return -1;
}

void ctp_cli_set_thread_prio_stksize(u32 prio, u32 stk_size)
{
    ctp_cli.cli_thread_prio = prio;
    ctp_cli.cli_thread_stk_size = stk_size;
}

int ctp_cli_send(void *handle, const char *topic, const char *content)
{
    int ret = 0;
    struct ctp_hdl *hdl = (struct ctp_hdl *)handle;
    char *ctp_msg = NULL;
    u32 topic_len = 0;
    u32 topic_ct_len = 0;
    u32 ctp_msg_len = 0;

    printf("hdl=0x%x  hdl->state=%d\n", (u32)hdl, hdl->state);
    if (!hdl || hdl->state != CONNECTED_SUCC) {
        return -1;
    }
    topic_len = strlen(topic);
    topic_ct_len = content ? strlen(content) : 0;

    ctp_msg_len = CTP_PREFIX_LEN + CTP_TOPIC_LEN + CTP_TOPIC_CONTENT_LEN + topic_len + topic_ct_len;
    ctp_msg = (char *)malloc(ctp_msg_len);
    if (ctp_msg == NULL) {
        return -1;
    }

    memcpy(ctp_msg, CTP_PREFIX, CTP_PREFIX_LEN);
    memcpy(ctp_msg + CTP_PREFIX_LEN, &topic_len, CTP_TOPIC_LEN);
    memcpy(ctp_msg + CTP_PREFIX_LEN + CTP_TOPIC_LEN, topic, topic_len);
    memcpy(ctp_msg + CTP_PREFIX_LEN + CTP_TOPIC_LEN + topic_len, &topic_ct_len, CTP_TOPIC_CONTENT_LEN);
    if (content) {
        memcpy(ctp_msg + CTP_PREFIX_LEN + CTP_TOPIC_LEN + topic_len + CTP_TOPIC_CONTENT_LEN, content, topic_ct_len);
    }
    if (sock_send(hdl->sock_hdl, ctp_msg, ctp_msg_len, 0) <= 0) {
        ret = -1;
        hdl->state = SERVER_CLOSE;
        printf("%s %d->Error in send(0x%x)\n", __FUNCTION__, __LINE__, hdl->dest_addr.sin_addr.s_addr);
    }


    free(ctp_msg);
    return ret;
}

struct sockaddr_in *ctp_cli_get_hdl_addr(void *handle)
{
    if (!handle) {
        return NULL;
    }

    return &((struct ctp_hdl *)handle)->dest_addr;
}
