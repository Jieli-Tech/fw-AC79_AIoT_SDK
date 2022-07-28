#include "os/os_api.h"
#include "generic/list.h"
#include "cdp/cdp.h"
#include "sock_api.h"
#include "string.h"
#include "lwip.h"
#include "asm/debug.h"

#define DEFAULT_CDP_RECV_THREAD_PRIO          20
#define DEFAULT_CDP_RECV_THREAD_STK_SIZE      1600// 0x4000

enum cdp_cli_state {
    CLI_NOT_CONNECT,
    CLI_CONNECTED = 0x2f27e6ab,
    CLI_CLOSE,
};

struct cdp_srv_t {
    struct list_head cli_list_head;
    int pid;
    OS_MUTEX mutex;
    void *sock_hdl;
    int (*cb_func)(void *cli, enum cdp_srv_msg_type type, char *topic, char *content, void *priv);
    void *priv;
    u32 thread_prio;
    u32 thread_stksize;
    u32 max_topic_len;
    u32 max_content_slice_len;
    u32_t cli_cnts;
    int keep_alive_en;
    u32 keep_alive_timeout_sec;
    char *keep_alive_parm;
    char *keep_alive_recv;
    char *keep_alive_send;
    int login_en;
    char *login_recv;
    char *login_send;
    u16_t port;
};
static struct cdp_srv_t cdp_srv;

struct cdp_cli_t {//跟CTP结构相同
    char name[4];//must add first of this struct
    int type;//must add second of this struct
    struct list_head entry;
    OS_MUTEX mutex;
    void *sock_hdl;
    struct sockaddr_in dest_addr;
    char thread_name[16];
    int pid;
    unsigned int keep_alive_timeout;
    int login_state;
    enum cdp_cli_state state;
    struct eth_addr dhwaddr;
};





static int cdp_cli_sock_cb(enum sock_api_msg_type type, void *priv)
{
    struct list_head *pos, *node;
    struct cdp_cli_t *cli = NULL;

//如果不使能CDP心跳包则不进行链表查询操作
    if (cdp_srv.keep_alive_en  == 0) {
        return 0;
    }
//遍历CDP所有连接客户端，递增心跳包计数值
    os_mutex_pend(&cdp_srv.mutex, 0);
//不允许循环遍历链表的时候，去删除链表,释放cli
    list_for_each_safe(pos, node, &cdp_srv.cli_list_head) {
        cli = list_entry(pos, struct cdp_cli_t, entry);
        if (time_lapse(&cli->keep_alive_timeout, cdp_srv.keep_alive_timeout_sec) && cli->state != CLI_CLOSE) {
            puts("|CDP_SRV_CLI_KEEP_ALIVE_TO!|\n");
            list_del(&cli->entry);
            --cdp_srv.cli_cnts;
            //心跳包超时先通知外面，删除链表,然后由外面释放cli
            cli->state = CLI_CLOSE;
            cdp_srv.cb_func(cli, CDP_SRV_CLI_KEEP_ALIVE_TO, CDP_RESERVED_TOPIC, NULL, cdp_srv.priv);
            //有一个客户端超时，剩下的客户端等待下一次进来回调，再去检测超时
            break;
        }
    }
    os_mutex_post(&cdp_srv.mutex);
    return 0;
}

static int cdp_srv_sock_init(void)
{
    int ret = -1;
    struct sockaddr _cdp_addr;
    int flags;


    cdp_srv.sock_hdl = sock_reg(AF_INET, SOCK_DGRAM, 0, cdp_cli_sock_cb, NULL);
    if (cdp_srv.sock_hdl == NULL) {
        printf("%s %d->Error in socket()\n", __func__, __LINE__);
        goto EXIT;
    }
    if (sock_set_reuseaddr(cdp_srv.sock_hdl)) {
        printf("%s %d->Error in sock_set_reuseaddr(),errno=%d\n", __func__, __LINE__, errno);
        goto EXIT;
    }

    //Get_IPAddress(1,ip);//使用无线
    struct sockaddr_in *cdp_addr = (struct sockaddr_in *)&_cdp_addr;
    cdp_addr->sin_family = AF_INET;
    cdp_addr->sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("192.168.2.205");//htonl(INADDR_ANY);
    cdp_addr->sin_port = htons(cdp_srv.port);

    ret = sock_bind(cdp_srv.sock_hdl, (struct sockaddr *)&_cdp_addr, sizeof(_cdp_addr));
    if (ret) {
        printf("%s %d->Error in bind(),errno=%d\n", __func__, __LINE__, errno);
        goto EXIT;
    }

    if ((flags = fcntl(sock_get_socket(cdp_srv.sock_hdl), F_GETFL, 0)) < 0) {
        printf("%s %d->Error in fcntl()\n", __func__, __LINE__);
        goto EXIT;
    }
    flags |= O_NONBLOCK;
    if (fcntl(sock_get_socket(cdp_srv.sock_hdl), F_SETFL, flags) < 0) {
        printf("%s %d->Error in fcntl()\n", __func__, __LINE__);
        goto EXIT;
    }

    return 0;

EXIT:
    if (cdp_srv.sock_hdl) {
        sock_unreg(cdp_srv.sock_hdl);
    }

    return ret;
}

static void cdp_srv_sock_unreg(void)
{
    if (cdp_srv.sock_hdl != NULL) {
        sock_unreg(cdp_srv.sock_hdl);
    }
}

static int cdp_srv_sock_send(void *client_ctx, char *buf, unsigned int len)
{
    int ret = -1;
    struct cdp_cli_t *cli = client_ctx;

    if (cdp_srv.sock_hdl != NULL) {
        ret = sock_sendto(cdp_srv.sock_hdl, buf, len, 0, (struct sockaddr *)&cli->dest_addr, sizeof(struct sockaddr_in));
    }
    if (cli->state == CLI_CLOSE) {
        sock_set_quit(cdp_srv.sock_hdl);
        return -1;
    }
    return ret;
}
static int cdp_srv_send_ext(void *client_ctx, char *buf, unsigned int len)
{
    struct list_head *pos;
    struct cdp_cli_t *cli = client_ctx;
    int ret = 0;
    if (cli != NULL) {
        if (os_mutex_pend(&cli->mutex, 0)) {
            printf(">>>%s %d->Error in pend.\n", __func__, __LINE__);
            ret = -1;
            goto EXIT;
        }
        if (cli->state == CLI_CONNECTED) {
            if (cdp_srv_sock_send(cli, buf, len) <= 0) {
                ret = -1;
                cli->state = CLI_CLOSE;
                printf(">>>%s %d->Error in send(0x%x)\n", __func__, __LINE__, cli->dest_addr.sin_addr.s_addr);
            }
        } else {
            ret = -1;
        }
        os_mutex_post(&cli->mutex);
    } else {
        os_mutex_pend(&cdp_srv.mutex, 0);

        list_for_each(pos, &cdp_srv.cli_list_head) {
            cli = list_entry(pos, struct cdp_cli_t, entry);

            if (os_mutex_pend(&cli->mutex, 0)) {
                ret = -1;
                printf(">>>%s %d->Error in pend.\n", __func__, __LINE__);
                continue;
            }

            if (cli->state == CLI_CONNECTED) {
                if (cdp_srv_sock_send(cli, buf, len) <= 0) {
                    ret = -1;
                    cli->state = CLI_CLOSE;
                    os_mutex_post(&cli->mutex);
                    printf(">>>%s %d->Error in send_all(0x%x)\n", __func__, __LINE__, cli->dest_addr.sin_addr.s_addr);
                    continue;
                }
            }
            os_mutex_post(&cli->mutex);
        }

        os_mutex_post(&cdp_srv.mutex);
    }
EXIT:
    return ret;
}

static void cdp_recv_thread(void *arg)
{
    struct sockaddr_in dest_addr;
    socklen_t dest_addr_len = sizeof(dest_addr);

    unsigned short cdp_topic_len;
    char _topic[cdp_srv.max_topic_len + 1];
    char *topic = _topic;
    char topic_ct[cdp_srv.max_content_slice_len + 1];
    unsigned int cdp_ct_len;
    u8 recv_buf[1472];
    int rlen, ret;
    int is_keepavlie_topic;
    struct list_head *pos;
    struct cdp_cli_t *cli, *cli_exist;
    int find_flag = 0;
    while (1) {

        if (sock_select_rdset(cdp_srv.sock_hdl) == 0) {
            rlen = sock_recvfrom(cdp_srv.sock_hdl, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&dest_addr, &dest_addr_len);
            printf("\n rlen is %d  \n", rlen);
            if (rlen <= 0) {
                break;
            }
            if (strncmp(CDP_PREFIX, (const char *)recv_buf, CDP_PREFIX_LEN)) {
                continue;
            }

            memcpy((char *)&cdp_topic_len, recv_buf + CDP_PREFIX_LEN, CDP_TOPIC_LEN);
            memcpy((char *)&cdp_ct_len, recv_buf + CDP_PREFIX_LEN + CDP_TOPIC_LEN + cdp_topic_len, CDP_TOPIC_CONTENT_LEN);
            printf("\ncdp_topic_len is %d cdp_ct_len is %d\n", cdp_topic_len, cdp_ct_len);

            if (cdp_topic_len > cdp_srv.max_topic_len || cdp_ct_len > cdp_srv.max_content_slice_len) {
                printf("\n>>>cdp_topic_len or cdp_ct_len is too large\n");
            }
            memcpy(topic, recv_buf + CDP_PREFIX_LEN + CDP_TOPIC_LEN, cdp_topic_len);
            topic[cdp_topic_len] = '\0';
            memcpy(topic_ct, recv_buf + CDP_PREFIX_LEN + CDP_TOPIC_LEN + cdp_topic_len + CDP_TOPIC_CONTENT_LEN, cdp_ct_len);
            topic_ct[cdp_ct_len] = '\0';
            printf("\n topic is %s topic_ct is %s\n", topic, topic_ct);

            find_flag = 0;
            os_mutex_pend(&cdp_srv.mutex, 0);
            list_for_each(pos, &cdp_srv.cli_list_head) {
                cli_exist = list_entry(pos, struct cdp_cli_t, entry);
                if (cli_exist->dest_addr.sin_addr.s_addr == dest_addr.sin_addr.s_addr) {
                    find_flag = 1;
                    break;
                }
            }
            os_mutex_post(&cdp_srv.mutex);
            if (find_flag) {
                cli = cli_exist;
            } else {
                cli = calloc(1, sizeof(struct cdp_cli_t));
                if (cli == NULL) {
                    printf(">>> malloc fail in cdp_recv_thread");
                    os_time_dly(1);
                    continue;
                }
                cli->type = USE_CDP;
                memcpy(&cli->dest_addr, &dest_addr, sizeof(struct sockaddr_in));
                printf("\n>>> new cli addr is %s , port is %d \n", inet_ntoa(cli->dest_addr.sin_addr.s_addr), ntohs(cli->dest_addr.sin_port));
                lwip_get_dest_hwaddr(1, (ip4_addr_t *)&cli->dest_addr.sin_addr.s_addr, &cli->dhwaddr);
                os_mutex_pend(&cdp_srv.mutex, 0);
                ++cdp_srv.cli_cnts;
                list_add_tail(&cli->entry, &cdp_srv.cli_list_head);
                os_mutex_post(&cdp_srv.mutex);
                os_mutex_create(&cli->mutex);
                cli->state = CLI_CONNECTED;
                memcpy(cli->name, "CDP", 3);
            }

            if (cdp_srv.keep_alive_en && !strcmp(topic, cdp_srv.keep_alive_recv ? cdp_srv.keep_alive_recv : CDP_KEEP_ALIVE_TOPIC)) {

                //先回一个心跳包
                cli->keep_alive_timeout = 0;
                ret = cdp_srv_send(cli, cdp_srv.keep_alive_send ? cdp_srv.keep_alive_send : CDP_KEEP_ALIVE_TOPIC, cdp_srv.keep_alive_parm);
                if (ret) {
                    printf("%s %d->Error in cdp_srv_send()\n", __func__, __LINE__);
                    goto EXIT;
                }
                if (cdp_ct_len == 0) {

                    if (cdp_srv.cb_func(cli, CDP_SRV_RECV_KEEP_ALIVE_MSG, topic, NULL, cdp_srv.priv)) {
                        goto EXIT;
                    }
                } else {
                    if (cdp_srv.cb_func(cli, CDP_SRV_RECV_KEEP_ALIVE_MSG, topic, topic_ct, cdp_srv.priv)) {
                        goto EXIT;
                    }
                }
                continue;
            }
            if (cdp_srv.cb_func(cli, CDP_SRV_RECV_MSG, topic, topic_ct, cdp_srv.priv)) {
                goto EXIT;
            }

        } else {
            printf("%s %d->Error in sock_select_rdset()\n", __func__, __LINE__);
            break;
        }
    }

EXIT:
    printf("\nEXIT cdp_recv_thread\n");
    //选择关闭服务的时候才去释放sock
}

int cdp_srv_keep_alive_en(const char *recv, const char *send, const char *parm)
{
    int ret = -1;
    const char *p = cdp_srv.keep_alive_parm;

    if (os_mutex_pend(&cdp_srv.mutex, 0)) {
        return -1;
    }

    if (parm) {
        free(cdp_srv.keep_alive_parm);
        if (asprintf(&cdp_srv.keep_alive_parm, "%s", parm) <= 0) {
            goto EXIT;
        }
    }
    if (send) {
        free(cdp_srv.keep_alive_send);
        if (asprintf(&cdp_srv.keep_alive_send, "%s", send) <= 0) {
            goto EXIT;
        }
    }
    if (recv) {
        free(cdp_srv.keep_alive_recv);
        if (asprintf(&cdp_srv.keep_alive_recv, "%s", recv) <= 0) {
            goto EXIT;
        }
    }

    ret = 0;
    cdp_srv.keep_alive_timeout_sec = CDP_KEEP_ALIVE_DEFAULT_TIMEOUT;
    cdp_srv.keep_alive_en = 1;

EXIT:
    if (ret) {
        free(cdp_srv.keep_alive_parm);
        cdp_srv.keep_alive_parm = NULL;
        free(cdp_srv.keep_alive_recv);
        cdp_srv.keep_alive_recv = NULL;
        free(cdp_srv.keep_alive_send);
        cdp_srv.keep_alive_send = NULL;
    }
    os_mutex_post(&cdp_srv.mutex);
    return ret;
}

static void cdp_srv_keep_alive_unen(void)
{
    if (cdp_srv.keep_alive_en == 0) {
        return;
    }

    if (os_mutex_pend(&cdp_srv.mutex, 0)) {
        return;
    }

    cdp_srv.keep_alive_en = 0;

    free(cdp_srv.keep_alive_parm);
    cdp_srv.keep_alive_parm = NULL;
    free(cdp_srv.keep_alive_recv);
    cdp_srv.keep_alive_recv = NULL;
    free(cdp_srv.keep_alive_send);
    cdp_srv.keep_alive_send = NULL;

    os_mutex_post(&cdp_srv.mutex);
}

int cdp_srv_init(u16_t port, int (*cb_func)(void *cli, enum cdp_srv_msg_type type, char *topic, char *content, void *priv), void *priv)
{
    int ret;

    memset(&cdp_srv, 0, sizeof(struct cdp_srv_t));
    os_mutex_create(&cdp_srv.mutex);
    INIT_LIST_HEAD(&cdp_srv.cli_list_head);
    cdp_srv.cb_func = cb_func;
    cdp_srv.priv = priv;
    cdp_srv.port = port;
    cdp_srv.max_topic_len = MAX_RECV_TOPIC_LEN;
    cdp_srv.max_content_slice_len = MAX_RECV_TOPIC_CONTENT_LEN_SLICE;

    if (cdp_srv_sock_init()) {
        goto EXIT;
    }

    printf("cdp_srv_init listen port = %d\n", cdp_srv.port);

    cdp_srv.thread_prio = DEFAULT_CDP_RECV_THREAD_PRIO;
    cdp_srv.thread_stksize = DEFAULT_CDP_RECV_THREAD_STK_SIZE;

    thread_fork("CDP_RECV_THREAD", cdp_srv.thread_prio, cdp_srv.thread_stksize, 0, &cdp_srv.pid, cdp_recv_thread, (void *)NULL);

    return 0;

EXIT:
    if (cdp_srv.sock_hdl) {
        sock_unreg(cdp_srv.sock_hdl);
    }
    return -1;
}

void cdp_keep_alive_find_dhwaddr_disconnect(struct eth_addr *dhwaddr)
{
    struct list_head *pos_cli, *node;
    struct cdp_cli_t  *cli;
    if (!os_mutex_valid(&cdp_srv.mutex)) {
        return;
    }


    os_mutex_pend(&cdp_srv.mutex, 0);
    list_for_each_safe(pos_cli, node, &cdp_srv.cli_list_head) {
        cli = list_entry(pos_cli, struct cdp_cli_t, entry);
        if (0 == memcmp(&cli->dhwaddr, dhwaddr, sizeof(struct eth_addr))) {
            printf("cdp_keep_alive_find_hwaddr_disconnect.\r\n");
            cli->state = CLI_CLOSE;
            cdp_srv.cb_func(cli, CDP_SRV_CLI_DISCONNECT, CDP_RESERVED_TOPIC, NULL, cdp_srv.priv);
            break;
        }
    }
    os_mutex_post(&cdp_srv.mutex);
}
struct sockaddr_in *cdp_srv_get_cli_addr(void *cli)
{
    struct cdp_cli_t *pcli = (struct cdp_cli_t *)cli;
    if (pcli && pcli->type == USE_CDP) {
        return &pcli->dest_addr;
    }
    return NULL;
}
void cdp_srv_free_cli(void *cli_hdl)
{
    struct cdp_cli_t *cli = (struct cdp_cli_t *)cli_hdl;
    if (cli && cli->type == USE_CDP) {
        cli->type = 0;
        free(cli_hdl);
        cli_hdl = NULL;
    }
}
static void cdp_srv_disconnect_spec_cli(struct cdp_cli_t  *cli)
{
    struct list_head *pos_cli, *node;
    bool find_cli = 0;
    os_mutex_pend(&cdp_srv.mutex, 0);
    list_for_each_safe(pos_cli, node, &cdp_srv.cli_list_head) {
        if (list_entry(pos_cli, struct cdp_cli_t, entry) == cli) {
            find_cli = 1;
            list_del(&cli->entry);
            break;
        }
    }
    os_mutex_post(&cdp_srv.mutex);
    if (!find_cli) {
        puts("|cdp_srv_disconnect_not_find_cli\n");
        return;
    }
    printf("CDP_CLI_disconnect(0x%x)(0x%x)(0x%x)!\n", (unsigned int)cli, cli->dest_addr.sin_addr.s_addr, cli->dest_addr.sin_port);
    --cdp_srv.cli_cnts;
    cli->state = CLI_CLOSE;
    os_mutex_pend(&cli->mutex, 0);
    os_mutex_del(&cli->mutex, 1);
    cdp_srv.cb_func(cli, CDP_SRV_CLI_DISCONNECT, CDP_RESERVED_TOPIC, NULL, cdp_srv.priv);
}
void cdp_srv_disconnect_all_cli(void)
{
    struct cdp_cli_t  *cli;

    while (1) {
        os_mutex_pend(&cdp_srv.mutex, 0);
        if (list_empty(&cdp_srv.cli_list_head)) {
            os_mutex_post(&cdp_srv.mutex);
            break;
        }
        cli = list_first_entry(&cdp_srv.cli_list_head, struct cdp_cli_t, entry);
        cli->state = CLI_CLOSE;
        os_mutex_post(&cdp_srv.mutex);
        cdp_srv_disconnect_spec_cli(cli);
    }
}
void cdp_srv_disconnect_cli(void  *cli_hdl)
{
    if (cli_hdl) {
        cdp_srv_disconnect_spec_cli(cli_hdl);
    } else {
        cdp_srv_disconnect_all_cli();
    }
}


int cdp_srv_uninit(void)
{
    struct list_head *pos;
    struct cdp_cli_t *cli;

    sock_set_quit(cdp_srv.sock_hdl);
    thread_kill(&cdp_srv.pid, KILL_WAIT);
    cdp_srv_sock_unreg();
//遍历所有链表删除客户端
    cdp_srv_disconnect_all_cli();
    cdp_srv_keep_alive_unen();
    return 0;
}

int cdp_srv_send(void *_cli, char *topic, char *content)
{
    int ret = 0;
    char *cdp_msg;
    unsigned short topic_len;
    unsigned int topic_ct_len;
    unsigned int cdp_msg_len;
    struct cdp_cli_t *cli = (struct cdp_cli_t *)_cli;
    if (cli && cli->state != CLI_CONNECTED) {
        printf("cdp_srv_send(0x%x) disconnected.. \n", cli->dest_addr.sin_addr.s_addr);
        return 0;
    } else if (cdp_srv_get_cli_cnt() == 0) {
        printf("\n >>> cdp no cli\n");
        return 0;
    }
    topic_len = strlen(topic);
    topic_ct_len = content ? strlen(content) : 0;

    cdp_msg_len = CDP_PREFIX_LEN + CDP_TOPIC_LEN + CDP_TOPIC_CONTENT_LEN + topic_len + topic_ct_len;
    cdp_msg = (char *)malloc(cdp_msg_len);
    if (cdp_msg == NULL) {
        return -1;
    }

    memcpy(cdp_msg, CDP_PREFIX, CDP_PREFIX_LEN);
    memcpy(cdp_msg + CDP_PREFIX_LEN, &topic_len, CDP_TOPIC_LEN);
    memcpy(cdp_msg + CDP_PREFIX_LEN + CDP_TOPIC_LEN, topic, topic_len);
    memcpy(cdp_msg + CDP_PREFIX_LEN + CDP_TOPIC_LEN + topic_len, &topic_ct_len, CDP_TOPIC_CONTENT_LEN);
    if (content) {
        memcpy(cdp_msg + CDP_PREFIX_LEN + CDP_TOPIC_LEN + topic_len + CDP_TOPIC_CONTENT_LEN, content, topic_ct_len);
    }
    ret = cdp_srv_send_ext(cli, cdp_msg, cdp_msg_len);

    free(cdp_msg);

    return ret;
}

void cdp_srv_set_thread_prio_stksize(u32 prio, u32 stk_size)
{
    cdp_srv.thread_prio = prio;
    cdp_srv.thread_stksize = stk_size;
}

void cdp_srv_set_thread_payload_max_len(u32 max_topic_len, u32 max_content_slice_len)
{
    cdp_srv.max_topic_len = max_topic_len;
    cdp_srv.max_content_slice_len = max_content_slice_len;
}

u32 cdp_srv_get_cli_cnt(void)
{
    return cdp_srv.cli_cnts;
}

void cdp_srv_set_keep_alive_timeout(u32 timeout_sec)
{
    cdp_srv.keep_alive_timeout_sec = timeout_sec;
}
int cdp_srv_get_keep_alive_timeout()
{
    return cdp_srv.keep_alive_timeout_sec;
}




