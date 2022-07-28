#include "system/includes.h"
#include "http/http_cli.h"
#include "ip_addr.h"
#include "dns.h"

#define log_info(x, ...)    printf("[LTE_TEST][INFO]" x " ", ## __VA_ARGS__)
#define log_err(x, ...)     printf("[LTE_TEST][ERR]" x " ", ## __VA_ARGS__)

/* #define PING_TEST */
/* #define TCP_TEST */
/* #define DNS_TEST */
#define HTTP_TEST


#ifdef TCP_TEST
#include "sock_api/sock_api.h"

#define CLIENT_TCP_PORT 5888
static void *sock = NULL;

int tcp_client_init(const char *server_ip, const int server_port)
{
    struct sockaddr_in local;
    struct sockaddr_in dest;

    sock = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);
    if (sock == NULL) {
        log_info("%s tcp: sock_reg fail\n",  __FILE__);
        return -1;
    }

    char *get_lte_ip(void);
    local.sin_addr.s_addr = inet_addr(get_lte_ip());//INADDR_ANY;
    /* local.sin_addr.s_addr = inet_addr("192.168.1.1");//INADDR_ANY; */
    local.sin_port = htons(CLIENT_TCP_PORT);
    local.sin_family = AF_INET;
    if (0 != sock_bind(sock, (struct sockaddr *)&local, sizeof(struct sockaddr_in))) {
        sock_unreg(sock);
        log_info("%s sock_bind fail\n",  __FILE__);
        return -1;
    }

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(server_ip);
    dest.sin_port = htons(server_port);

    if (0 != sock_connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr_in))) {
        log_info("sock_connect fail\n");
        sock_unreg(sock);
        return -1;
    }

    return 0;
}


int tcp_send_data(const void *sock_hdl, const void *buf, const u32 len)
{
    return sock_send(sock_hdl, buf, len, 0);
}


int tcp_recv_data(const void *sock_hdl, void *buf, u32 len)
{
    return sock_recvfrom(sock_hdl, buf, len, 0, NULL, NULL);
}


static void tcp_rx_task(void *priv)
{
    s32 recv_len;
    char recv_buf[64];

    for (;;) {
        recv_len = tcp_recv_data(sock, recv_buf, sizeof(recv_buf));
        if ((recv_len != -1) && (recv_len != 0)) {
            recv_buf[recv_len] = '\0';
            log_info("receive data length : %d, data: %s\n\r", recv_len, recv_buf);
        } else {
            log_info("sock_recvfrom err!!!");
            break;
        }
    }
}


static void tcp_tx_task(void *priv)
{
    s32 send_len = 0;
    char send_buf[] = "hello, how are you!";

    for (;;) {
        send_len = tcp_send_data(sock, send_buf, strlen(send_buf));
        if (send_len == -1) {
            log_info("sock_sendto err!!!");
            sock_unreg(sock);
            return;
        }
        os_time_dly(50);
    }
}


static void tcp_client_test(void)
{
    int err;

    err = tcp_client_init("47.92.146.210", 8888);
    if (err) {
        log_info("tcp_client_init err!!!");
        return;
    } else {
        log_info("tcp_client_init succ!!!");
    }

    thread_fork("tcp_rx_task", 10, 2 * 1024, 0, NULL, tcp_rx_task, NULL);
    thread_fork("tcp_tx_task", 8, 2 * 1024, 0, NULL, tcp_tx_task, NULL);

    return;
}
#endif


#ifdef  HTTP_TEST

#define DOWNLOAD_BUF_SIZE   (512)


static int __httpcli_cb(void *ctx, void *buf, unsigned int size, void *priv, httpin_status status)
{
    return 0;
}


static s32 http_download(const char *url)
{
    s32 ret = 0;
    u8 *buf = NULL;
    u32 total_len = 0, offset = 0, remain = 0;
    const struct net_download_ops *ops = &http_ops;

    buf = (u8 *)malloc(DOWNLOAD_BUF_SIZE);
    if (!buf) {
        log_err("buf malloc err\n");
        goto _download_exit_;
    }

    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (NULL == ctx) {
        log_err("calloc err\n");
        goto _download_exit_;
    }

    ctx->url = url;
    ctx->connection = "close";
    ctx->timeout_millsec = 10000;
    ctx->cb = __httpcli_cb;

    log_info("URL : %s\n", url);

    if (ops->init(ctx) != HERROR_OK) {
        log_err("http init err\n");
        goto _download_exit_;
    }

    total_len = ctx->content_length;
    log_info("total_len = %d\n", total_len);

    while (total_len > 0) {
        if (total_len >= DOWNLOAD_BUF_SIZE) {
            remain = DOWNLOAD_BUF_SIZE;
            total_len -= DOWNLOAD_BUF_SIZE;
        } else {
            remain = total_len;
            total_len = 0;
        }

        do {
            ret = ops->read(ctx, buf + offset, remain - offset);
            if (ret < 0) {
                log_err("http download err\n");
                goto _download_exit_;
            }
            offset += ret;
        } while (remain != offset);
        offset = 0;
        /* log_info("total_len = %d\n", total_len); */
        putchar('.');
    }

    ops->close(ctx);
    log_info("%s, succ\n", __FUNCTION__);

_download_exit_:
    if (buf) {
        free(buf);
    }

    if (ctx) {
        free(ctx);
    }
    return 0;
}
#endif

static void dns_found(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    log_info("%s, name = %s\n, ip = %s", __func__, name, ipaddr_ntoa(ipaddr));
}


static void lte_network_test_task(void *priv)
{
#ifdef  PING_TEST
    ping_init("120.24.247.138", 1000, 10, NULL, NULL);
#endif

#ifdef  TCP_TEST
    tcp_client_test();
#endif

#ifdef  DNS_TEST
    ip_addr_t host_ip;
    dns_gethostbyname("www.taobao.com", &host_ip, dns_found, NULL);
#endif

#ifdef  HTTP_TEST
    /* http_download("https://profile.jieliapp.com/license/v1/fileupdate/download/4e8a7fbb-d432-4090-9966-b1c6a1f37bf3"); */
    http_download("https://profile.jieliapp.com/license/v1/fileupdate/download/92bb2c13-4360-4cc4-b7c2-75a57c82e8a9");
    /* http_download("https://profile.jieliapp.com/license/v1/fileupdate/download/d07f3202-4da0-4c96-8ba3-bb80960097dd"); */
#endif

}


void lte_network_test(void)
{
    thread_fork("lte_network_test_task", 8, 2 * 1024, 0, NULL, lte_network_test_task, NULL);
}



