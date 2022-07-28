//#include "cache.h"
#include "os/os_api.h"
#include "generic/list.h"
#include "ctp/ctp.h"
#include "string.h"
#include "lwip.h"
#include "sock_api.h"
#include "asm/debug.h"
#if USE_CTPS
#include "mbedtls/mbedtls_config.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
#include "mbedtls/net.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#if defined(MBEDTLS_SSL_CACHE_C)
#include "mbedtls/ssl_cache.h"
#endif
#define DEBUG_LEVEL 0
static void my_debug(void *ctx, int level,
                     const char *file, int line,
                     const char *str)
{
//    if(level < DEBUG_LEVEL)
//        mbedtls_printf("%s", str);
}
#endif

#define CTP_ACCEPT_THREAD_PRIO          21
#define CTP_ACCEPT_THREAD_STK_SIZE      256// 0x2000

#define DEFAULT_CTP_SRV_THREAD_PRIO          22
#define DEFAULT_CTP_SRV_THREAD_STK_SIZE      800


enum ctp_cli_state {
    CLI_NOT_CONNECT,
    CLI_CONNECTED = 0x2f27e6ab,
    CLI_CLOSE,
};

struct ctp_srv_t {
    struct list_head cli_list_head;
    int pid;
    OS_MUTEX mutex;
#if USE_CTPS
    mbedtls_net_context listen_fd;
    const char *pers;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_context cache;
#endif
#else
    void *sock_hdl;
#endif
    int (*cb_func)(void *cli, enum ctp_srv_msg_type type, char *topic, char *content, void *priv);
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
static struct ctp_srv_t ctp_srv;

struct ctp_cli_t {
    char name[4];//must add first of this struct
    int type;//must add second of this struct
    struct list_head entry;
    OS_MUTEX mutex;
#if USE_CTPS
    mbedtls_ssl_context ssl;
    mbedtls_net_context client_fd;
#else
    void *sock_hdl;
#endif
    struct sockaddr_in dest_addr;
    char thread_name[32];
    int pid;
    unsigned int keep_alive_timeout;
    int login_state;
    enum ctp_cli_state state;
    struct eth_addr dhwaddr;
};

#if USE_CTPS
static int ctp_srv_sock_init(void)
{
    int ret;

    ctp_srv.pers = "ssl_server";

    mbedtls_net_init(&ctp_srv.listen_fd);
    mbedtls_net_set_cb(&ctp_srv.listen_fd, NULL, &ctp_srv);
    mbedtls_ssl_config_init(&ctp_srv.conf);
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_init(&ctp_srv.cache);
#endif
    mbedtls_x509_crt_init(&ctp_srv.srvcert);
    mbedtls_pk_init(&ctp_srv.pkey);
    mbedtls_entropy_init(&ctp_srv.entropy);
    mbedtls_ctr_drbg_init(&ctp_srv.ctr_drbg);

#if defined(MBEDTLS_DEBUG_C)
    mbedtls_debug_set_threshold(DEBUG_LEVEL);
#endif

    /*
     * 1. Load the certificates and private RSA key
     */
    /*
     * This demonstration program uses embedded test certificates.
     * Instead, you may want to use mbedtls_x509_crt_parse_file() to read the
     * server and CA certificates, as well as mbedtls_pk_parse_keyfile().
     */
    ret = mbedtls_x509_crt_parse(&ctp_srv.srvcert, (const unsigned char *) mbedtls_test_srv_crt,
                                 mbedtls_test_srv_crt_len);
    if (ret != 0) {
        mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret);
        goto exit;
    }

    ret = mbedtls_x509_crt_parse(&ctp_srv.srvcert, (const unsigned char *) mbedtls_test_cas_pem,
                                 mbedtls_test_cas_pem_len);
    if (ret != 0) {
        mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned %d\n\n", ret);
        goto exit;
    }

    ret =  mbedtls_pk_parse_key(&ctp_srv.pkey, (const unsigned char *) mbedtls_test_srv_key,
                                mbedtls_test_srv_key_len, NULL, 0);
    if (ret != 0) {
        mbedtls_printf(" failed\n  !  mbedtls_pk_parse_key returned %d\n\n", ret);
        goto exit;
    }

    /*
     * 2. Setup the listening TCP socket
     */
    char port[16];
    sprintf(port, "%d", ctp_srv.port);
    if ((ret = mbedtls_net_bind(&ctp_srv.listen_fd, NULL, port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_net_bind[%s] returned %d\n\n", port, ret);
        goto exit;
    }
    /*
     * 3. Seed the RNG
     */

    if ((ret = mbedtls_ctr_drbg_seed(&ctp_srv.ctr_drbg, mbedtls_entropy_func, &ctp_srv.entropy,
                                     (const unsigned char *) ctp_srv.pers,
                                     strlen(ctp_srv.pers))) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto exit;
    }

    /*
     * 4. Setup stuff
     */

    if ((ret = mbedtls_ssl_config_defaults(&ctp_srv.conf,
                                           MBEDTLS_SSL_IS_SERVER,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_ssl_conf_rng(&ctp_srv.conf, mbedtls_ctr_drbg_random, &ctp_srv.ctr_drbg);
    mbedtls_ssl_conf_dbg(&ctp_srv.conf, my_debug, NULL);

#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_conf_session_cache(&conf, &cache,
                                   mbedtls_ssl_cache_get,
                                   mbedtls_ssl_cache_set);
#endif

    mbedtls_ssl_conf_ca_chain(&ctp_srv.conf, ctp_srv.srvcert.next, NULL);
    if ((ret = mbedtls_ssl_conf_own_cert(&ctp_srv.conf, &ctp_srv.srvcert, &ctp_srv.pkey)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
        goto exit;
    }

    return 0;

exit:
#ifdef MBEDTLS_ERROR_C
    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, 100);
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
    }
#endif

    mbedtls_net_free(&ctp_srv.listen_fd);

    mbedtls_x509_crt_free(&ctp_srv.srvcert);
    mbedtls_pk_free(&ctp_srv.pkey);
    mbedtls_ssl_config_free(&ctp_srv.conf);
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_free(&ctp_srv.cache);
#endif
    mbedtls_ctr_drbg_free(&ctp_srv.ctr_drbg);
    mbedtls_entropy_free(&ctp_srv.entropy);

    return (ret);
}
static int ctp_srv_sock_accept(void *client_ctx, struct sockaddr *addr, socklen_t *addrlen, int (*cb_func)(enum sock_api_msg_type type, void *priv), void *priv)
{
    int ret;

    struct ctp_cli_t *cli = client_ctx;

    mbedtls_ssl_init(&cli->ssl);

    if ((ret = mbedtls_ssl_setup(&cli->ssl, &ctp_srv.conf)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        return -1;
    }
    mbedtls_ssl_session_reset(&cli->ssl);

    mbedtls_net_init(&cli->client_fd);
    if ((ret = mbedtls_net_accept(&ctp_srv.listen_fd, &cli->client_fd, addr, sizeof(struct sockaddr), (size_t *)&addrlen)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_net_accept returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_ssl_set_bio(&cli->ssl, &cli->client_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    /*
     * 5. Handshake
     */
    while ((ret = mbedtls_ssl_handshake(&cli->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned %d\n\n", ret);
            goto exit;
        }
    }

    return 0;

exit:
    return ret;
}

static int ctp_srv_sock_recv(void *client_ctx, char *buf, unsigned int len, int flag)
{
    struct ctp_cli_t *cli = client_ctx;

    mbedtls_net_set_fd_priv(&cli->client_fd, (void *)MSG_WAITALL);
    return mbedtls_ssl_read(&cli->ssl, buf, len);
}
static int ctp_srv_sock_send(void *client_ctx, char *buf, unsigned int len, int flag)
{
    struct ctp_cli_t *cli = client_ctx;

    mbedtls_net_set_fd_priv(&cli->client_fd, (void *)MSG_DONTWAIT);
    return mbedtls_ssl_write(&cli->ssl, buf, len);
}

static void ctp_srv_sock_unreg(void)
{
    mbedtls_net_free(&ctp_srv.listen_fd);

    mbedtls_x509_crt_free(&ctp_srv.srvcert);
    mbedtls_pk_free(&ctp_srv.pkey);
    mbedtls_ssl_config_free(&ctp_srv.conf);
#if defined(MBEDTLS_SSL_CACHE_C)
    mbedtls_ssl_cache_free(&ctp_srv.cache);
#endif
    mbedtls_ctr_drbg_free(&ctp_srv.ctr_drbg);
    mbedtls_entropy_free(&ctp_srv.entropy);
}
static void ctp_cli_sock_unreg(void *ctx)
{
    int ret;

    ret = mbedtls_ssl_close_notify(&(((struct ctp_cli_t *)ctx)->ssl));
    {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ &&
            ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_close_notify returned %d\n\n", ret);
        }
    }

    mbedtls_net_free(&(((struct ctp_cli_t *)ctx)->client_fd));
    mbedtls_ssl_free(&(((struct ctp_cli_t *)ctx)->ssl));
}

#else
static int ctp_srv_sock_init(void)
{
    int ret = -EINVAL;
    struct sockaddr _ss;
    char ip[16];

    ctp_srv.sock_hdl = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, &ctp_srv);
    if (ctp_srv.sock_hdl == NULL) {
        printf("%s %d->Error in socket()\n", __func__, __LINE__);
        goto EXIT;
    }
    if (sock_set_reuseaddr(ctp_srv.sock_hdl)) {
        printf("%s %d->Error in sock_set_reuseaddr(),errno=%d\n", __func__, __LINE__, errno);
        goto EXIT;
    }

    //Get_IPAddress(1,ip);//使用无线
    struct sockaddr_in *dest_addr = (struct sockaddr_in *)&_ss;
    dest_addr->sin_family = AF_INET;
    dest_addr->sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("192.168.2.205");//htonl(INADDR_ANY);
    dest_addr->sin_port = htons(ctp_srv.port);

    ret = sock_bind(ctp_srv.sock_hdl, (struct sockaddr *)&_ss, sizeof(_ss));
    if (ret) {
        printf("%s %d->Error in bind(),errno=%d\n", __func__, __LINE__, errno);
        goto EXIT;
    }
    ret = sock_listen(ctp_srv.sock_hdl, 0xff);
    if (ret) {
        printf("%s %d->Error in listen()\n", __func__, __LINE__);
        goto EXIT;
    }

    return 0;

EXIT:
    if (ctp_srv.sock_hdl) {
        sock_unreg(ctp_srv.sock_hdl);
    }
    return ret;
}

static int ctp_srv_sock_accept(void *client_ctx, struct sockaddr *addr, socklen_t *addrlen, int (*cb_func)(enum sock_api_msg_type type, void *priv), void *priv)
{
    struct ctp_cli_t *cli = (struct ctp_cli_t *)client_ctx;
    cli->sock_hdl = sock_accept(ctp_srv.sock_hdl, addr, addrlen, cb_func, priv);
    if (cli->sock_hdl == NULL) {
        return -1;
    }
    return 0;
}

static int ctp_srv_sock_recv(void *client_ctx, char *buf, unsigned int len, int flag)
{
    struct ctp_cli_t *cli = (struct ctp_cli_t *)client_ctx;

    return sock_recv(cli->sock_hdl, buf, len, flag);
}
static int ctp_srv_sock_send(void *client_ctx, char *buf, unsigned int len, int flag)
{
    struct ctp_cli_t *cli = client_ctx;

    return sock_send(cli->sock_hdl, buf, len, flag);
}

static void ctp_srv_sock_unreg(void)
{
    sock_unreg(ctp_srv.sock_hdl);
}
static void ctp_cli_sock_unreg(void *ctx)
{
    struct ctp_cli_t *cli = ctx;

    sock_unreg(cli->sock_hdl);
}
#endif

void ctp_srv_free_cli(void *cli_hdl)
{
    struct ctp_cli_t *cli = (struct ctp_cli_t *)cli_hdl;
    if (cli && cli->type == USE_CTP) {
        ctp_cli_sock_unreg(cli_hdl);
        cli->type = 0;
        free(cli_hdl);
        cli_hdl = NULL;
    }
}

static void ctp_srv_disconnect_spec_cli(struct ctp_cli_t  *cli)
{
    struct list_head *pos_cli, *node;
    bool find_cli = 0;

    os_mutex_pend(&ctp_srv.mutex, 0);
    list_for_each_safe(pos_cli, node, &ctp_srv.cli_list_head) {
        if (list_entry(pos_cli, struct ctp_cli_t, entry) == cli) {
            find_cli = 1;
            list_del(&cli->entry);
            break;
        }
    }
    os_mutex_post(&ctp_srv.mutex);
    if (!find_cli) {
        puts("|ctp_srv_disconnect_not_find_cli\n");
        return;
    }

    printf("CTP_CLI_disconnect(0x%x)(0x%x)(0x%x)!\n", (unsigned int)cli, cli->dest_addr.sin_addr.s_addr, cli->dest_addr.sin_port);

    cli->state = CLI_CLOSE;
    --ctp_srv.cli_cnts;
    sock_set_quit(cli->sock_hdl);
    os_mutex_pend(&cli->mutex, 0);
    os_mutex_del(&cli->mutex, 1);
    thread_kill(&cli->pid, KILL_WAIT);
    ctp_srv.cb_func(cli, CTP_SRV_CLI_DISCONNECT, CTP_RESERVED_TOPIC, NULL, ctp_srv.priv);
}

void ctp_srv_disconnect_all_cli(void)
{
    struct ctp_cli_t  *cli;

    while (1) {
        os_mutex_pend(&ctp_srv.mutex, 0);
        if (list_empty(&ctp_srv.cli_list_head)) {
            os_mutex_post(&ctp_srv.mutex);
            break;
        }
        cli = list_first_entry(&ctp_srv.cli_list_head, struct ctp_cli_t, entry);
        os_mutex_post(&ctp_srv.mutex);
        ctp_srv_disconnect_spec_cli(cli);
    }
}

void ctp_srv_disconnect_cli(void *cli_hdl)
{
    if (cli_hdl) {
        ctp_srv_disconnect_spec_cli(cli_hdl);
    } else {
        ctp_srv_disconnect_all_cli();
    }
}

static void ctp_cli_thread(void *arg)
{
    struct ctp_cli_t *cli = (struct ctp_cli_t *)arg;

    int ret, rv_len;
    char _topic[ctp_srv.max_topic_len + 1];
    char *topic = _topic;
    char topic_ct[ctp_srv.max_content_slice_len + 1];
    char ctp_prefix[CTP_PREFIX_LEN + 1] = {'\0'};
    unsigned short ctp_topic_len;
    unsigned int ctp_ct_len;
    char is_keepavlie_topic = 0;
    char is_login_topic = 0;
    char is_malloc = 0;
    char tp_malloc = 0;

    char *topic_ct_ptr = topic_ct;


    if (ctp_srv.cb_func(cli, CTP_SRV_CLI_CONNECTED, CTP_RESERVED_TOPIC, NULL, ctp_srv.priv)) {
        goto EXIT;
    }
    while (1) {
DEAL_RECV_AG:
        ret = ctp_srv_sock_recv(cli, ctp_prefix, CTP_PREFIX_LEN, MSG_WAITALL);
        if (ret != CTP_PREFIX_LEN) {
            printf("%s %d->  sock_recv: %d\n", __func__, __LINE__, ret);
            goto EXIT;
        }
        if (strcmp(CTP_PREFIX, ctp_prefix)) {
            printf("ctp_cli recv(0x%x) = %s, maybe hack!\n", cli->dest_addr.sin_addr.s_addr, ctp_prefix);
            continue;
        }

        ret = ctp_srv_sock_recv(cli, (char *)&ctp_topic_len, CTP_TOPIC_LEN, MSG_WAITALL);
        if (ret != CTP_TOPIC_LEN) {
            printf("%s %d->  sock_recv: %d\n", __func__, __LINE__, ret);
            goto EXIT;
        }
        if (ctp_topic_len >= MAX_RECV_TOPIC_LEN) {
            topic = (char *)malloc(ctp_topic_len + 1);
            if (topic == NULL) {
                //TODO
                printf("%s %d-> sock_recv: malloc error\n", __func__, __LINE__);
                continue;
            }
            tp_malloc = 1;
        }

        ret = ctp_srv_sock_recv(cli, topic, ctp_topic_len, MSG_WAITALL);
        if (ret != ctp_topic_len) {
            printf("%s %d->  sock_recv: %d\n", __func__, __LINE__, ret);
            goto EXIT;
        }
        topic[ctp_topic_len] = '\0';

        ret = ctp_srv_sock_recv(cli, (char *)&ctp_ct_len, CTP_TOPIC_CONTENT_LEN, MSG_WAITALL);
        if (ret != CTP_TOPIC_CONTENT_LEN) {
            printf("%s %d->  sock_recv: %d\n", __func__, __LINE__, ret);
            goto EXIT;
        }

        //CTP_KEEP_ALIVE
        if (ctp_srv.keep_alive_en && !strcmp(topic, ctp_srv.keep_alive_recv ? ctp_srv.keep_alive_recv : CTP_KEEP_ALIVE_TOPIC)) {
            is_keepavlie_topic = 1;
            puts("CTP_KEEP_ALIVE \r\n");
            ret = ctp_srv_send(cli, ctp_srv.keep_alive_send ? ctp_srv.keep_alive_send : CTP_KEEP_ALIVE_TOPIC, ctp_srv.keep_alive_parm);
            if (ret) {
                printf("%s %d->Error in ctp_srv_send() err : %d \n", __func__, __LINE__, ret);
                goto EXIT;
            }
            if (ctp_ct_len == 0) {
                if (ctp_srv.cb_func(cli, CTP_SRV_RECV_KEEP_ALIVE_MSG, topic, NULL, ctp_srv.priv)) {
                    goto EXIT;
                }
            }
        }

        if (ctp_srv.login_en && !strcmp(topic, ctp_srv.login_recv ? ctp_srv.login_recv : CTP_LOGIN_TOPIC)) {
            is_login_topic = 1;
            if (ctp_ct_len == 0) {
                if (ctp_srv.cb_func(cli, CTP_SRV_RECV_LOGIN_MSG, topic, NULL, ctp_srv.priv)) {
                    goto EXIT;
                }
                cli->login_state = 1;
            }
        }

        cli->keep_alive_timeout = 0;
        while (ctp_ct_len) {
            rv_len = ctp_ct_len > MAX_RECV_TOPIC_CONTENT_LEN_SLICE ? MAX_RECV_TOPIC_CONTENT_LEN_SLICE : ctp_ct_len;

            if (ctp_ct_len > ctp_srv.max_content_slice_len) {
                topic_ct_ptr = malloc(ctp_ct_len  + 1);
                if (!topic_ct_ptr) {
                    printf("%s   %d  malloc fail\n", __func__, __LINE__);
                    goto EXIT;
                }
                is_malloc = 1;
                rv_len = ctp_ct_len;


            }
            ret = ctp_srv_sock_recv(cli, topic_ct_ptr, rv_len, MSG_WAITALL);
            if (ret != rv_len) {
                printf("%s %d->  sock_recv: %d\n", __func__, __LINE__, ret);
                goto EXIT;
            }

            ctp_ct_len -= rv_len;



            if (ctp_ct_len) {
                if (ctp_srv.cb_func(cli, CTP_SRV_RECV_MSG_SLICE, topic, topic_ct_ptr, ctp_srv.priv)) {
                    goto EXIT;
                }
            } else {
                topic_ct_ptr[rv_len] = '\0';
                if (is_keepavlie_topic) {
                    is_keepavlie_topic = 0;

                    if (ctp_srv.cb_func(cli, CTP_SRV_RECV_KEEP_ALIVE_MSG, topic, topic_ct_ptr, ctp_srv.priv)) {
                        goto EXIT;
                    }

                    goto DEAL_RECV_AG;
                }

                if (is_login_topic) {
                    is_login_topic = 0;
                    if (ctp_srv.cb_func(cli, CTP_SRV_RECV_LOGIN_MSG, topic, topic_ct_ptr, ctp_srv.priv)) {
                        goto EXIT;
                    }
                    cli->login_state = 1;
                    goto DEAL_RECV_AG;
                }

                if (ctp_srv.login_en && (!cli->login_state)) {
                    if (ctp_srv.cb_func(cli, CTP_SRV_RECV_MSG_WITHOUT_LOGIN, topic, topic_ct_ptr, ctp_srv.priv)) {
                        goto EXIT;
                    }
                } else {
                    if (ctp_srv.cb_func(cli, CTP_SRV_RECV_MSG, topic, topic_ct_ptr, ctp_srv.priv)) {
                        goto EXIT;
                    }
                }
                break;
            }
            cli->keep_alive_timeout = 0;
            if (is_malloc) {
                printf("\n[WARNING] free topic_ct_len\n");
                is_malloc = 0;
                free(topic_ct_ptr);
                topic_ct_ptr = topic_ct;
            }
            if (tp_malloc) {
                tp_malloc = 0;
                free(topic);
                topic = _topic;
            }
        }
        is_keepavlie_topic = 0;
        is_login_topic = 0;
    }

EXIT:

    if (tp_malloc) {
        free(topic);
    }
    ctp_srv_disconnect_spec_cli(cli);
}

void ctp_keep_alive_find_dhwaddr_disconnect(struct eth_addr *dhwaddr)
{
    if (!os_mutex_valid(&ctp_srv.mutex)) {
        return;
    }

    struct ctp_cli_t *cli ;
    struct list_head *pos_cli, *node;

    os_mutex_pend(&ctp_srv.mutex, 0);
    list_for_each_safe(pos_cli, node, &ctp_srv.cli_list_head) {
        cli = list_entry(pos_cli, struct ctp_cli_t, entry);
        if (0 == memcmp(&cli->dhwaddr, dhwaddr, sizeof(struct eth_addr))) {
            printf("ctp_keep_alive_find_hwaddr_disconnect.\r\n");
            cli->state = CLI_CLOSE;
            break;
        }
    }
    os_mutex_post(&ctp_srv.mutex);
}


static int ctp_cli_sock_cb(enum sock_api_msg_type type, void *priv)
{
    struct list_head *pos;
    struct ctp_cli_t *cli = (struct ctp_cli_t *)priv;

    if (cli->state == CLI_CLOSE) {
        puts("|ctp_cli_sock_cb CLI_CLOSE!\n");
        return -1;
    }
    if (ctp_srv.keep_alive_en) {
        if (time_lapse(&cli->keep_alive_timeout, ctp_srv.keep_alive_timeout_sec)) {
            puts("|CTP_SRV_CLI_KEEP_ALIVE_TO!|\n");
            /*ctp_srv.cb_func(cli, CTP_SRV_CLI_KEEP_ALIVE_TO, CTP_RESERVED_TOPIC, NULL, ctp_srv.priv);*/

            return -1;
        }
    }

    return 0;
}

static void ctp_accept_thread(void *arg)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(struct sockaddr_in);
    struct list_head *pos;
    char find_flag;
    int ret;
    struct ctp_cli_t *cli, *cli_exist;
    u32 count = 0;
    while (1) {

        cli = (struct ctp_cli_t *)calloc(1, sizeof(struct ctp_cli_t));
        if (cli == NULL) {
            os_time_dly(10);
            continue;
        }
        cli->type = USE_CTP;
        if (ctp_srv_sock_accept(cli, (struct sockaddr *)&client_addr, &client_len, ctp_cli_sock_cb, cli)) {
            goto EXIT;
        }
        memcpy(&cli->dest_addr, &client_addr, sizeof(struct sockaddr_in));

        printf("ctp cli connected(0x%x)(0x%x)(0x%x)!\n", (unsigned int)cli, cli->dest_addr.sin_addr.s_addr, cli->dest_addr.sin_port);
        printf("cli ip addr %s  port %d\n", inet_ntoa(cli->dest_addr.sin_addr.s_addr), ntohs(cli->dest_addr.sin_port));
        find_flag = 0;
        os_mutex_pend(&ctp_srv.mutex, 0);
        list_for_each(pos, &ctp_srv.cli_list_head) {
            cli_exist = list_entry(pos, struct ctp_cli_t, entry);
            if (cli_exist->dest_addr.sin_addr.s_addr == cli->dest_addr.sin_addr.s_addr) {
                find_flag = 1;
                break;
            }
        }
        os_mutex_post(&ctp_srv.mutex);

        if (find_flag) {
            ctp_srv.cb_func(cli, CTP_SRV_CLI_PREV_LINK_NOT_CLOSE, CTP_RESERVED_TOPIC, NULL, ctp_srv.priv);

            printf("2ctp cli disconnect old exist_cli!->IP:0x%x,PORT:0x%x\n", cli_exist->dest_addr.sin_addr.s_addr, cli_exist->dest_addr.sin_port);
            ctp_srv_disconnect_cli(cli_exist);
        }

        lwip_get_dest_hwaddr(1, (ip4_addr_t *)&cli->dest_addr.sin_addr.s_addr, &cli->dhwaddr);

        os_mutex_pend(&ctp_srv.mutex, 0);
        ++ctp_srv.cli_cnts;
        list_add_tail(&cli->entry, &ctp_srv.cli_list_head);
        os_mutex_post(&ctp_srv.mutex);

        os_mutex_create(&cli->mutex);

        cli->state = CLI_CONNECTED;

        memcpy(cli->name, "CTP", 3);

        ctp_srv.cb_func(cli, CTP_SRV_SET_RECV_THREAD_PRIO_STKSIZE, CTP_RESERVED_TOPIC, NULL, ctp_srv.priv);

        sprintf(cli->thread_name, "CTP_%x_%d", cli->dest_addr.sin_addr.s_addr, count++);
        ret = thread_fork(cli->thread_name, ctp_srv.thread_prio, ctp_srv.thread_stksize, 0, &cli->pid, ctp_cli_thread, (void *)cli);
        if (ret != OS_NO_ERR) {
            printf("thread fork fail ret=%d\n", ret);
            break;
        }
    }

EXIT:
    printf("ctp_accept_thread EXIT! \n");
}

void ctp_srv_set_keep_alive_timeout(u32 timeout_sec)
{
    ctp_srv.keep_alive_timeout_sec = timeout_sec;
}
int ctp_srv_get_keep_alive_timeout()
{
    return ctp_srv.keep_alive_timeout_sec;
}

int ctp_srv_login_en(const char *login_recv, const char *login_send)
{
    int ret = -1;
    if (os_mutex_pend(&ctp_srv.mutex, 0)) {
        return -1;
    }

    if ((!login_recv) || (!login_send)) {
        goto EXIT;
    }

    free(ctp_srv.login_recv);
    free(ctp_srv.login_send);
    ctp_srv.login_recv = NULL;
    ctp_srv.login_send = NULL;
    if (asprintf(&ctp_srv.login_recv, "%s", login_recv) <= 0) {
        goto EXIT;
    }

    if (asprintf(&ctp_srv.login_send, "%s", login_send) <= 0) {
        goto EXIT;
    }

    ret = 0;
    ctp_srv.login_en = 1;
EXIT:
    if (ret) {
        free(ctp_srv.login_recv);
        ctp_srv.login_recv = NULL;
        free(ctp_srv.login_send);
        ctp_srv.login_send = NULL;
    }
    os_mutex_post(&ctp_srv.mutex);
    return ret;
}

static void ctp_srv_login_unen(void)
{
    if (ctp_srv.login_en == 0) {
        return;
    }

    if (os_mutex_pend(&ctp_srv.mutex, 0)) {
        return;
    }

    ctp_srv.login_en = 0;

    free(ctp_srv.login_recv);
    ctp_srv.login_recv = NULL;

    free(ctp_srv.login_send);
    ctp_srv.login_send = NULL;

    os_mutex_post(&ctp_srv.mutex);
}

int ctp_srv_keep_alive_en(const char *recv, const char *send, const char *parm)
{
    int ret = -1;
    const char *p = ctp_srv.keep_alive_parm;

    if (os_mutex_pend(&ctp_srv.mutex, 0)) {
        return -1;
    }

    if (parm) {
        free(ctp_srv.keep_alive_parm);
        if (asprintf(&ctp_srv.keep_alive_parm, "%s", parm) <= 0) {
            goto EXIT;
        }
    }
    if (send) {
        free(ctp_srv.keep_alive_send);
        if (asprintf(&ctp_srv.keep_alive_send, "%s", send) <= 0) {
            goto EXIT;
        }
    }
    if (recv) {
        free(ctp_srv.keep_alive_recv);
        if (asprintf(&ctp_srv.keep_alive_recv, "%s", recv) <= 0) {
            goto EXIT;
        }
    }

    ret = 0;
    ctp_srv.keep_alive_timeout_sec = CTP_KEEP_ALIVE_DEFAULT_TIMEOUT;
    ctp_srv.keep_alive_en = 1;

EXIT:
    if (ret) {
        free(ctp_srv.keep_alive_parm);
        ctp_srv.keep_alive_parm = NULL;
        free(ctp_srv.keep_alive_recv);
        ctp_srv.keep_alive_recv = NULL;
        free(ctp_srv.keep_alive_send);
        ctp_srv.keep_alive_send = NULL;
    }
    os_mutex_post(&ctp_srv.mutex);
    return ret;
}

static void ctp_srv_keep_alive_unen(void)
{
    if (ctp_srv.keep_alive_en == 0) {
        return;
    }

    if (os_mutex_pend(&ctp_srv.mutex, 0)) {
        return;
    }

    ctp_srv.keep_alive_en = 0;

    free(ctp_srv.keep_alive_parm);
    ctp_srv.keep_alive_parm = NULL;
    free(ctp_srv.keep_alive_recv);
    ctp_srv.keep_alive_recv = NULL;
    free(ctp_srv.keep_alive_send);
    ctp_srv.keep_alive_send = NULL;

    os_mutex_post(&ctp_srv.mutex);
}

void ctp_srv_uninit(void)
{
    sock_set_quit(ctp_srv.sock_hdl);

    thread_kill(&ctp_srv.pid, KILL_WAIT);
    ctp_srv_sock_unreg();
    ctp_srv_disconnect_all_cli();


    ctp_srv_keep_alive_unen();

    ctp_srv_login_unen();

    os_mutex_del(&ctp_srv.mutex, 1);
}


void ctp_srv_set_thread_prio_stksize(u32 prio, u32 stk_size)
{
    ctp_srv.thread_prio = prio;
    ctp_srv.thread_stksize = stk_size;
}

void ctp_srv_set_thread_payload_max_len(u32 max_topic_len, u32 max_content_slice_len)
{
    ctp_srv.max_topic_len = max_topic_len;
    ctp_srv.max_content_slice_len = max_content_slice_len;
}

int ctp_srv_init(u16_t port, int (*cb_func)(void *cli, enum ctp_srv_msg_type type, char *topic, char *content, void *priv), void *priv)
{
    int ret;

    memset(&ctp_srv, 0, sizeof(struct ctp_srv_t));
    os_mutex_create(&ctp_srv.mutex);
    INIT_LIST_HEAD(&ctp_srv.cli_list_head);
    ctp_srv.cb_func = cb_func;
    ctp_srv.priv = priv;
    ctp_srv.port = port;
    ctp_srv.max_topic_len = MAX_RECV_TOPIC_LEN;
    ctp_srv.max_content_slice_len = MAX_RECV_TOPIC_CONTENT_LEN_SLICE;

    if (ctp_srv_sock_init()) {
        goto EXIT;
    }

    printf("ctp_srv_init listen port = %d\n", ctp_srv.port);

    ctp_srv.thread_prio = DEFAULT_CTP_SRV_THREAD_PRIO;
    ctp_srv.thread_stksize = DEFAULT_CTP_SRV_THREAD_STK_SIZE;

    thread_fork("CTP_ACCEPT_THREAD", CTP_ACCEPT_THREAD_PRIO, CTP_ACCEPT_THREAD_STK_SIZE, 0, &ctp_srv.pid, ctp_accept_thread, (void *)NULL);

    return 0;

EXIT:
    ctp_srv_sock_unreg();

    return -1;
}

int ctp_srv_send_ext(void *cli, char *user_pkt, u32 pkt_len)
{
    int ret = 0;
    struct list_head *pos_cli;
    struct ctp_cli_t *cli_hdl = (struct ctp_cli_t *)cli;

    if (cli_hdl) {
        if (os_mutex_pend(&cli_hdl->mutex, 0)) {
            ret = -1;
            printf("%s %d->Error in pend.\n", __func__, __LINE__);
            goto EXIT;
        }
        if (cli_hdl->state == CLI_CONNECTED) {
            /*if ((ctp_srv_sock_send(cli_hdl, user_pkt, pkt_len, MSG_DONTWAIT)) <= 0) {*/
            if ((ctp_srv_sock_send(cli_hdl, user_pkt, pkt_len, 0)) <= 0) {
                ret = -1;
                cli_hdl->state = CLI_CLOSE;
                printf("%s %d->Error in send(0x%x)\n", __func__, __LINE__, cli_hdl->dest_addr.sin_addr.s_addr);
            }
        } else {
            printf("err %s %d \n\n", __func__, __LINE__);
            ret = -1;
        }
        os_mutex_post(&cli_hdl->mutex);
    } else {
        os_mutex_pend(&ctp_srv.mutex, 0);
        list_for_each(pos_cli, &ctp_srv.cli_list_head) {
            cli_hdl = list_entry(pos_cli, struct ctp_cli_t, entry);

            if (os_mutex_pend(&cli_hdl->mutex, 0)) {
                ret = -1;
                printf("%s %d->Error in pend.\n", __func__, __LINE__);
                continue;
            }

            if (ctp_srv.login_en && !cli_hdl->login_state) {
                os_mutex_post(&cli_hdl->mutex);
                printf("%s %d->Error in login.\n", __func__, __LINE__);
                continue;
            } else if (cli_hdl->state == CLI_CONNECTED) {

                if (ctp_srv_sock_send(cli_hdl, user_pkt, pkt_len, MSG_DONTWAIT) <= 0) {
                    ret = -1;
                    cli_hdl->state = CLI_CLOSE;
                    os_mutex_post(&cli_hdl->mutex);
                    printf("%s %d->Error in send_all(0x%x)\n", __func__, __LINE__, cli_hdl->dest_addr.sin_addr.s_addr);
                    printf("CMD_e = %s\n", user_pkt);
                    continue;
                }

                /* printf("%s %din send_all(0x%x)port(0x%x)\n", __func__, __LINE__, cli_hdl->dest_addr.sin_addr.s_addr, cli_hdl->dest_addr.sin_port); */

            }
            os_mutex_post(&cli_hdl->mutex);

        }
        os_mutex_post(&ctp_srv.mutex);
    }

EXIT:
    return ret;
}

int ctp_srv_send(void *cli, char *topic, char *content)
{
    int ret = 0;
    struct ctp_cli_t *cli_hdl = (struct ctp_cli_t *)cli;
    char *ctp_msg;
    unsigned short topic_len;
    unsigned int topic_ct_len;
    unsigned int ctp_msg_len;

    if (cli_hdl && cli_hdl->state != CLI_CONNECTED) {
        printf("ctp_srv_send(0x%x) disconnected.. \n", cli_hdl->dest_addr.sin_addr.s_addr);
        return 0;
    } else if (ctp_srv_get_cli_cnt() == 0) {
        puts("ctp no clis\n");
        return 0;
    }

    if (cli_hdl && ctp_srv.login_en && !cli_hdl->login_state) {
        //没有登录只能发送心跳topic和登录topic的数据
        if ((strcmp(topic, ctp_srv.keep_alive_send) != 0) && (strcmp(topic, ctp_srv.login_send) != 0)) {
            return 0;
        }
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
    ret = ctp_srv_send_ext(cli, ctp_msg, ctp_msg_len);

    free(ctp_msg);

    return ret;
}

u32 ctp_srv_get_cli_cnt(void)
{
    return ctp_srv.cli_cnts;
}

int ctp_sock_set_send_timeout(void *cli, u32 millsec)
{
    if (!cli) {
        return -1;
    }
    sock_set_send_timeout(((struct ctp_cli_t *)cli)->sock_hdl, millsec);
    return 0;
}

int ctp_sock_set_recv_timeout(void *cli, u32 millsec)
{
    if (!cli) {
        return -1;
    }
    sock_set_recv_timeout(((struct ctp_cli_t *)cli)->sock_hdl, millsec);
    return 0;
}

struct sockaddr_in *ctp_srv_get_cli_addr(void *cli)
{
    struct ctp_cli_t *pcli = (struct ctp_cli_t *)cli;
    if (pcli && pcli->type == USE_CTP) {
        return &pcli->dest_addr;
    }
    return NULL;
}

struct sockaddr_in *ctp_srv_get_first_cli(void)
{
    struct ctp_cli_t  *cli;

    while (1) {
        os_mutex_pend(&ctp_srv.mutex, 0);

        if (list_empty(&ctp_srv.cli_list_head)) {
            os_mutex_post(&ctp_srv.mutex);
            return NULL;
        }
        cli = list_first_entry(&ctp_srv.cli_list_head, struct ctp_cli_t, entry);
        os_mutex_post(&ctp_srv.mutex);
        return ctp_srv_get_cli_addr(cli);
    }
}

