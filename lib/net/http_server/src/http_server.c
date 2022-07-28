#include <stdlib.h>
#include "os/os_api.h"
#include "lwip/sockets.h"
#include "sock_api.h"
#include "http_server.h"

#define HTTP_SERVER_MAIN_THREAD_PRIO    25
#define HTTP_SERVER_MAIN_THREAD_STK_SIZE    256

#define HTTP_SERVER_ACCEPTPETITION_THREAD_PRIO  20
#define HTTP_SERVER_ACCEPTPETITION_THREAD_STK_SIZE  1600

#define MAX_LISTEN_BACKLOG 0xff

enum http_srv_state {
    HTTP_CLOSE,
    HTTP_ACCEPTED,
};

struct http_srv_t {
    int pid;
    OS_MUTEX mutex;
    OS_SEM sem;
    void *sock_hdl;
    struct list_head cli_list_head;
    enum http_srv_state state;
    int backlog;
};
static struct http_srv_t http_srv;

static int http_accept_sock_cb(enum sock_api_msg_type type, void *priv)
{
    if (http_srv.state == HTTP_CLOSE) {
        puts("http_accept_sock_cb->HTTP_CLOSE\n");
        return -1;
    }

    return 0;
}

static int http_cli_sock_cb(enum sock_api_msg_type type, void *priv)
{
    struct http_cli_t *cli = priv;

    if (http_srv.state == HTTP_CLOSE) {
//        puts("http_cli_sock_cb->HTTP_CLOSE\n");
        return -1;
    }

    if (cli->req_exit_flag) {
//        puts("http_cli_sock_cb->req_exit_flag\n");
        return -1;
    }

    return 0;
}

void http_cli_thread_kill(struct http_cli_t *cli)
{
    struct list_head *pos, *node;
    bool find_cli = 0;

    os_mutex_pend(&http_srv.mutex, 0);
    list_for_each_safe(pos, node, &http_srv.cli_list_head) {
        if (list_entry(pos, struct http_cli_t, entry) == cli) {
            find_cli = 1;
            list_del(&cli->entry);
            --http_srv.backlog;
            break;
        }
    }
    os_mutex_post(&http_srv.mutex);

    if (!find_cli) {
//        puts("http_cli_thread_kill not find_cli.\n");
        return;
    }

//   printf("http_cli_thread_kill = 0x%x, 0x%x\n",cli, cli->dst_addr.sin_addr.s_addr);

    thread_kill(&cli->pid, KILL_WAIT);
    sock_unreg(cli->sock_hdl);
    free(cli);
    os_sem_post(&http_srv.sem);
}

static void http_server_thread(void *priv)
{
    struct sockaddr_in  s_client;
    int socketLength = sizeof(struct sockaddr_in);
    struct http_cli_t *cli;
    char buf[64] = {0};
    u32 count = 0;

    while (1) {
        while (http_srv.backlog >= MAX_LISTEN_BACKLOG) {
            os_sem_pend(&http_srv.sem, 0);

            if (http_srv.state == HTTP_CLOSE) {
                goto EXIT;
            }
        }

        cli = (struct http_cli_t *)calloc(sizeof(struct http_cli_t), 1);
        if (cli == NULL) {
            printf("%s %d->Error in calloc()\n", __FUNCTION__, __LINE__);
            os_time_dly(10);
            continue;
        }

        cli->sock_hdl = sock_accept(http_srv.sock_hdl, (struct sockaddr *)&s_client, (socklen_t *)&socketLength, http_cli_sock_cb, cli);
        if (cli->sock_hdl == NULL) {
            printf("%s %d->Error in sock_accept()\n", __FUNCTION__, __LINE__);
            free(cli);
            goto EXIT;
        }

        memcpy(&cli->dst_addr, &s_client, sizeof(struct sockaddr_in));

        os_mutex_pend(&http_srv.mutex, 0);
        list_add_tail(&cli->entry, &http_srv.cli_list_head);
        ++http_srv.backlog;
        os_mutex_post(&http_srv.mutex);

//       printf("http cli = 0x%x, 0x%x\n", cli, cli->dst_addr.sin_addr.s_addr);

        extern void acceptPetition(void *cli);
        sprintf(buf, "http_reques%d", count++);
        thread_fork(buf, HTTP_SERVER_ACCEPTPETITION_THREAD_PRIO, HTTP_SERVER_ACCEPTPETITION_THREAD_STK_SIZE, 0, &cli->pid, acceptPetition, (void *)cli);
    }

EXIT:
    sock_unreg(http_srv.sock_hdl);
}

int http_get_server_init(unsigned short port)
{
    int ret = -1;

    if (http_srv.state == HTTP_ACCEPTED) {
        return 0;
    }

    memset(&http_srv, 0, sizeof(struct http_srv_t));

    INIT_LIST_HEAD(&http_srv.cli_list_head);
    os_mutex_create(&http_srv.mutex);
    os_sem_create(&http_srv.sem, 0);

    http_srv.sock_hdl = sock_reg(AF_INET, SOCK_STREAM, 0, http_accept_sock_cb, NULL);
    if (http_srv.sock_hdl == NULL) {
        printf("%s %d->Error in socket()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }
    if (sock_set_reuseaddr(http_srv.sock_hdl)) {
        printf("%s %d->Error in sock_set_reuseaddr(),errno=%d\n", __FUNCTION__, __LINE__, errno);
        goto EXIT;
    }

    struct sockaddr _ss;
    struct sockaddr_in *dest_addr = (struct sockaddr_in *)&_ss;
    dest_addr->sin_family = AF_INET;
    dest_addr->sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr->sin_port = htons(port);

    ret = sock_bind(http_srv.sock_hdl, (struct sockaddr *)&_ss, sizeof(_ss));
    if (ret) {
        printf("%s %d->Error in bind(),errno=%d\n", __FUNCTION__, __LINE__, errno);
        goto EXIT;
    }

    ret = sock_listen(http_srv.sock_hdl, MAX_LISTEN_BACKLOG);
    if (ret) {
        printf("%s %d->Error in listen()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }
    http_srv.state = HTTP_ACCEPTED;

    return thread_fork("http_get_server_thread", HTTP_SERVER_MAIN_THREAD_PRIO, HTTP_SERVER_MAIN_THREAD_STK_SIZE, 0, &http_srv.pid, http_server_thread, NULL);

EXIT:
    return ret;
}

void http_get_server_discpnnect_cli(struct sockaddr_in *dst_addr)
{
    struct list_head *pos, *node;
    struct http_cli_t *cli = NULL;
    bool find_cli = 0;

    if (http_srv.state == HTTP_CLOSE) {
        return;
    }

    os_mutex_pend(&http_srv.mutex, 0);

    list_for_each_safe(pos, node, &http_srv.cli_list_head) {
        cli = list_entry(pos, struct http_cli_t, entry);
        if (cli->dst_addr.sin_addr.s_addr == dst_addr->sin_addr.s_addr) {
            find_cli = 1;
            list_del(&cli->entry);
            break;
        }
    }
    os_mutex_post(&http_srv.mutex);

    if (!find_cli) {
//        puts("http_get_server_discpnnect_cli not find_cli.\n");
        return;
    }

    cli->req_exit_flag = 1;
    thread_kill(&cli->pid, KILL_WAIT);
    sock_unreg(cli->sock_hdl);
    free(cli);
}

void http_get_server_uninit(void)
{
    struct list_head *pos;
    struct http_cli_t *cli;

    puts("|http_get_server_uninit|\n");

    http_srv.state = HTTP_CLOSE;
    os_sem_post(&http_srv.sem);
    thread_kill(&http_srv.pid, KILL_WAIT);

    os_mutex_pend(&http_srv.mutex, 0);

    while (1) {
        if (list_empty(&http_srv.cli_list_head)) {
            break;
        }

        cli = list_first_entry(&http_srv.cli_list_head, struct http_cli_t, entry);
        list_del(&cli->entry);

        cli->req_exit_flag = 1;
        thread_kill(&cli->pid, KILL_WAIT);
        sock_unreg(cli->sock_hdl);
        free(cli);
    }
    os_mutex_post(&http_srv.mutex);

    os_mutex_del(&http_srv.mutex, 1);
    os_sem_del(&http_srv.sem, 1);
}
