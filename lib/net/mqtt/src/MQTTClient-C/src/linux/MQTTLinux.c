/*******************************************************************************
 * Copyright (c) 2014 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Allan Stockdill-Mander - initial API and implementation and/or initial documentation
 *******************************************************************************/

#include "MQTTLinux.h"

#include "sock_api/sock_api.h"

#define CONFIG_MQTT_USE_TLS1_2

extern int gettimeofday(struct timeval *tv, void *tz);
extern char *itoa(int num, char *str, int radix);

char mqtt_expired(Timer *timer)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);
    return res.tv_sec < 0 || (res.tv_sec == 0 && res.tv_usec <= 0);
}


void mqtt_countdown_ms(Timer *timer, unsigned int timeout)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {timeout / 1000, (timeout % 1000) * 1000};
    timeradd(&now, &interval, &timer->end_time);
}


void mqtt_countdown(Timer *timer, unsigned int timeout)
{
    struct timeval now;
    gettimeofday(&now, NULL);
    struct timeval interval = {timeout, 0};
    timeradd(&now, &interval, &timer->end_time);
}


unsigned long mqtt_left_ms(Timer *timer)
{
    struct timeval now, res;
    gettimeofday(&now, NULL);
    timersub(&timer->end_time, &now, &res);
    //printf("left %d ms\n", (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000);
    return (res.tv_sec < 0) ? 0 : res.tv_sec * 1000 + res.tv_usec / 1000;
}


void mqtt_InitTimer(Timer *timer)
{
    timer->end_time = (struct timeval) {
        0, 0
    };
}



#ifdef CONFIG_MQTT_USE_TLS1_2

//--------------------------------------
#include "mbedtls/mbedtls_config.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"
#include "fs/fs.h"


//-----------use mbedtls
struct mbedtls_ctx {
    mbedtls_x509_crt cacert;
    mbedtls_x509_crt clicert;
    mbedtls_pk_context pkey;
    mbedtls_ssl_context ssl;
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    const char *pers;
    u32 recv_to_ms;
    u32 send_to_ms;
};


static void my_debug(void *ctx, int level,
                     const char *file, int line,
                     const char *str)
{
//    if(level < 1)
    mbedtls_printf("%s", str);
}

static void *tls_connect(char *hostname, char *port, int to_ms, Network *ctx)
{
    int ret, pkey_flag = 0, clicert_flag = 0;
    uint32_t flags;
    unsigned char *buf = NULL;
    char server_name[64] = {0};

    if (!hostname) {
        return NULL;
    }

    struct mbedtls_ctx *tls_ctx = calloc(1, sizeof(struct mbedtls_ctx));
    if (tls_ctx == NULL) {
        return NULL;
    }

    tls_ctx->pers = "mqtt";

    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_net_init(&tls_ctx->server_fd);
    mbedtls_net_set_cb(&tls_ctx->server_fd, ctx->cb_func, ctx->priv);

    mbedtls_net_set_timeout(&tls_ctx->server_fd, to_ms, to_ms);
    tls_ctx->recv_to_ms = to_ms;
    tls_ctx->send_to_ms = to_ms;

    /* extern void mbedtls_debug_set_threshold(int threshold); */
    /* mbedtls_debug_set_threshold(4); */
    mbedtls_ssl_init(&tls_ctx->ssl);
    mbedtls_ssl_config_init(&tls_ctx->conf);
    mbedtls_ssl_conf_read_timeout(&tls_ctx->conf, to_ms);
    mbedtls_x509_crt_init(&tls_ctx->cacert);
    mbedtls_x509_crt_init(&tls_ctx->clicert);

    mbedtls_pk_init(&(tls_ctx->pkey));
    mbedtls_ctr_drbg_init(&tls_ctx->ctr_drbg);
    mbedtls_entropy_init(&tls_ctx->entropy);

#ifdef MBEDTLS_SSL_EXPORT_KEYS
    /* mbedtls_ssl_conf_export_keys_cb(&tls_ctx->conf, mbedtls_ssl_export_keys, (void *)(&tls_ctx->conf)); */
#endif

    if ((ret = mbedtls_ctr_drbg_seed(&tls_ctx->ctr_drbg, mbedtls_entropy_func, &tls_ctx->entropy,
                                     (const unsigned char *) tls_ctx->pers,
                                     strlen(tls_ctx->pers))) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto exit;
    }



#if 0
    void *cacert_fd = fopen("mnt/spiflash/res/CA.CRT", "r");
    if (!cacert_fd) {
        mbedtls_printf("cacert_fd open fail\n");
        goto exit;
    }
#endif

    buf = malloc(4 * 1024);
    if (!buf) {
        goto exit;
    }

    if (ctx->cas_pem_path) {
        log_d("LOAD  CA\n");

        void *cacert_fd = fopen(ctx->cas_pem_path, "r");
        if (cacert_fd) {
            int cacert_len = fread(buf, 1, 4 * 1024, cacert_fd);
            printf("cacert_len=>%d\n", cacert_len);
            fclose(cacert_fd);
            if (cacert_len <= 0) {
                goto exit;
            }
            buf[cacert_len] = '\0';
            ret = mbedtls_x509_crt_parse(&tls_ctx->cacert, buf, cacert_len + 1);
            if (ret < 0) {
                mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
                goto exit;
            }
        } else {
            strcpy((char *)buf, ctx->cas_pem_path);
            buf[ctx->cas_pem_len] = '\0';
            ret = mbedtls_x509_crt_parse(&tls_ctx->cacert, buf, ctx->cas_pem_len + 1);
            /* ret = mbedtls_x509_crt_parse(&tls_ctx->cacert, ctx->cas_pem_path, ctx->cas_pem_len + 1); */
            if (ret < 0) {
                mbedtls_printf(" failed\n  ! CA mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
                goto exit;
            }
        }
    } else {
        ret = mbedtls_x509_crt_parse(&tls_ctx->cacert, (const unsigned char *)mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
        if (ret < 0) {
            mbedtls_printf(" failed\n  ! CA mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
            goto exit;
        }
    }

    if (ctx->cli_pem_path) {
        log_d("LOAD  PEM\n");
        /* void * clicert_fd = fopen("mnt/spiflash/res/CLI.PEM","r"); */
        void *clicert_fd = fopen(ctx->cli_pem_path, "r");
        if (clicert_fd) {
            int clicert_len = fread(buf, 1, 4 * 1024, clicert_fd);
            printf("clicert_len=>%d\n", clicert_len);
            fclose(clicert_fd);
            if (clicert_len <= 0) {
                goto exit;
            }
            buf[clicert_len] = '\0';
            ret = mbedtls_x509_crt_parse(&tls_ctx->clicert, buf, clicert_len + 1);
            if (ret < 0) {
                mbedtls_printf(" failed\n  ! CLI mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
                goto exit;
            }
            clicert_flag = 1;
        } else {
            strcpy((char *)buf, ctx->cli_pem_path);
            buf[ctx->cli_pem_len] = '\0';
            ret = mbedtls_x509_crt_parse(&tls_ctx->clicert, buf, ctx->cli_pem_len + 1);
            if (ret < 0) {
                mbedtls_printf(" failed\n  ! CLI mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
                goto exit;
            }
            clicert_flag = 1;
        }
    }

    if (ctx->pkey_path) {
        log_d("LOAD  KEY\n");
        /* void * pkey_fd = fopen("mnt/spiflash/res/PRI.KEY","r"); */
        void *pkey_fd = fopen(ctx->pkey_path, "r");
        if (pkey_fd) {
            int pkey_len = fread(buf, 1, 4 * 1024, pkey_fd);
            printf("pkey_len=>%d\n", pkey_len);
            fclose(pkey_fd);
            if (pkey_len <= 0) {
                goto exit;
            }
            buf[pkey_len] = '\0';
            ret = mbedtls_pk_parse_key(&tls_ctx->pkey, buf, pkey_len + 1, NULL, 0);
            if (ret < 0) {
                mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
                goto exit;
            }
            pkey_flag = 1;
        } else {
            strcpy((char *)buf, ctx->pkey_path);
            buf[ctx->pkey_len] = '\0';
            ret = mbedtls_pk_parse_key(&tls_ctx->pkey, buf, ctx->pkey_len + 1, NULL, 0);
            if (ret < 0) {
                mbedtls_printf(" failed\n  ! PKEY mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
                goto exit;
            }
            pkey_flag = 1;
        }
    }

    free(buf);
    buf = NULL;

    /*
     * 1. Start the connection
     */
    mbedtls_printf("  . Connecting to tcp/%s/%s...\r\n", hostname, port);

    if ((ret = mbedtls_net_connect(&tls_ctx->server_fd, hostname,
                                   port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
        goto exit;
    }

    /*
     * 2. Setup stuff
     */
    /* mbedtls_printf("  . Setting up the SSL/TLS structure...\r\n"); */
    if ((ret = mbedtls_ssl_config_defaults(&tls_ctx->conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        goto exit;
    }

    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode(&tls_ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&tls_ctx->conf, &tls_ctx->cacert, NULL);
    mbedtls_ssl_conf_rng(&tls_ctx->conf, mbedtls_ctr_drbg_random, &tls_ctx->ctr_drbg);
    mbedtls_ssl_conf_dbg(&tls_ctx->conf, my_debug, 0);

    if (pkey_flag && clicert_flag) {
        ret = mbedtls_ssl_conf_own_cert(&(tls_ctx->conf), &(tls_ctx->clicert), &(tls_ctx->pkey));
        if (ret < 0) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
            goto exit;
        }
    }

    const char *alpnProtocols[] = { "x-amzn-mqtt-ca", NULL };
    ret = mbedtls_ssl_conf_alpn_protocols(&(tls_ctx->conf), alpnProtocols);
    if (ret < 0) {
        mbedtls_printf(" failed\n  !  returne_ssl_conf_alpn_protocols returned %d\n\n", ret);
        goto exit;
    }

    if ((ret = mbedtls_ssl_setup(&tls_ctx->ssl, &tls_ctx->conf)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        goto exit;
    }

    if ((ret = mbedtls_ssl_set_hostname(&tls_ctx->ssl, hostname)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        goto exit;
    }

    /* mbedtls_ssl_set_bio(&tls_ctx->ssl, &tls_ctx->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL); */
    mbedtls_ssl_set_bio(&tls_ctx->ssl, &tls_ctx->server_fd, mbedtls_net_send, NULL, mbedtls_net_recv_timeout);

    /*
     * 4. Handshake
     */
    /* mbedtls_printf("  . Performing the SSL/TLS handshake...\r\n"); */

    while ((ret = mbedtls_ssl_handshake(&tls_ctx->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
            goto exit;
        }
    }

    /*
     * 5. Verify the server certificate
     */
    /* mbedtls_printf("  . Verifying peer X.509 certificate...\r\n"); */

    /* In real life, we probably want to bail out when ret != 0 */
    if ((flags = mbedtls_ssl_get_verify_result(&tls_ctx->ssl)) != 0) {
        char vrfy_buf[512];

        mbedtls_printf(" failed\r\n");

        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

        mbedtls_printf("%s\r\n", vrfy_buf);
    } else {
        mbedtls_printf(" ok\r\n");
    }

    return tls_ctx;

exit:

#ifdef MBEDTLS_ERROR_C
    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, 100);
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
    }
#endif

    mbedtls_ssl_close_notify(&tls_ctx->ssl);
    mbedtls_net_free(&tls_ctx->server_fd);
    mbedtls_x509_crt_free(&tls_ctx->cacert);
    mbedtls_x509_crt_free(&tls_ctx->clicert);
    mbedtls_ssl_free(&tls_ctx->ssl);
    mbedtls_ssl_config_free(&tls_ctx->conf);
    mbedtls_ctr_drbg_free(&tls_ctx->ctr_drbg);
    mbedtls_entropy_free(&tls_ctx->entropy);
    mbedtls_pk_free(&tls_ctx->pkey);

    free(tls_ctx);

    if (buf) {
        free(buf);
    }

    return (NULL);
}

static void socks_disconnect(Network *n)
{
    struct mbedtls_ctx *tls_ctx = (struct mbedtls_ctx *)n->my_socket;
    if (tls_ctx == NULL) {
        return;
    }
    mbedtls_ssl_close_notify(&tls_ctx->ssl);
    mbedtls_net_free(&tls_ctx->server_fd);
    mbedtls_x509_crt_free(&tls_ctx->cacert);
    mbedtls_x509_crt_free(&tls_ctx->clicert);
    mbedtls_ssl_free(&tls_ctx->ssl);
    mbedtls_ssl_config_free(&tls_ctx->conf);
    mbedtls_ctr_drbg_free(&tls_ctx->ctr_drbg);
    mbedtls_entropy_free(&tls_ctx->entropy);
    mbedtls_pk_free(&tls_ctx->pkey);
    free(tls_ctx);
}

static int socks_read(Network *n, unsigned char *buf, int len, unsigned long ts)
{
    struct mbedtls_ctx *tls_ctx = (struct mbedtls_ctx *)n->my_socket;

    int ret = mbedtls_ssl_read(&tls_ctx->ssl, (unsigned char *)buf, len);

    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        printf("~~MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY\r\n");
        return 0;
    }

    if (ret < 0 && ret != MBEDTLS_ERR_SSL_TIMEOUT) {
        printf("socks_read fail:0x%d ", ret);
    }

    return ret;
}

// #define MBEDTLS_SSL_MAX_CONTENT_LEN         16384   //Size of the input / output buffer

static int socks_write(Network *n, unsigned char *buf, int len, unsigned long ts)
{
    struct mbedtls_ctx *tls_ctx = (struct mbedtls_ctx *)n->my_socket;
    int ret;
    int remain_len = len;
    int payload = 0;
    int offset = 0;

    while (remain_len) {
        if (remain_len >= MBEDTLS_SSL_MAX_CONTENT_LEN) {
            payload = MBEDTLS_SSL_MAX_CONTENT_LEN;
            remain_len -= MBEDTLS_SSL_MAX_CONTENT_LEN;
        } else {
            payload = remain_len;
            remain_len = 0;
        }
        ret = mbedtls_ssl_write(&tls_ctx->ssl, (unsigned char *)buf + offset, payload);

        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            return 0;
        }

        if (ret < 0) {
            printf("socks_write fail:%d \n", ret);
            return ret;
        }
        offset += payload;
    }

    return ret;
}


void NetWorkSetTLS(Network *n)
{
    n->mqttread = socks_read;
    n->mqttwrite = socks_write;
    n->disconnect = socks_disconnect;
    n->used_tls1_2 = true;
}


void NetWorkSetTLS_key(Network *n, const char *cas_pem, int cas_pem_len, const char *cli_pem, int cli_pem_len, const char *pkey, int pkey_len)
{
    n->cas_pem_path = cas_pem;
    n->cli_pem_path = cli_pem;
    n->pkey_path 	= pkey;

    n->cas_pem_len 	= cas_pem_len;
    n->cli_pem_len 	= cli_pem_len;
    n->pkey_len	   	= pkey_len;
}

#endif



static int linux_read(Network *n, unsigned char *buffer, int len, unsigned long timeout_ms)
{
    int bytes;

    bytes = sock_recv(n->my_socket, buffer, len, 0);

    if (bytes <= 0) {
        if (bytes == 0 || !sock_would_block(n->my_socket)) {
            n->state = -1;
            printf("Socket %d recv ret %d, error %d \n", *(int *)n->my_socket, bytes, sock_get_error(n->my_socket));
        }
        bytes = -1;
    }

    return bytes;
}


static int linux_write(Network *n, unsigned char *buffer, int len, unsigned long timeout_ms)
{
    int bytes;

    bytes = sock_send(n->my_socket, buffer, len, 0);

    if (bytes <= 0) {
        if (bytes == 0 || !sock_would_block(n->my_socket)) {
            n->state = -1;
            printf("Socket %d send ret %d, error %d \n", *(int *)n->my_socket, bytes, sock_get_error(n->my_socket));
        }
        bytes = -1;
    }

    return bytes;
}


static void linux_disconnect(Network *n)
{
    if (n->my_socket) {
        sock_unreg(n->my_socket);
        n->my_socket = NULL;
    }
}


void NewNetwork(Network *n)
{
    n->my_socket = NULL;
    n->mqttread = linux_read;
    n->mqttwrite = linux_write;
    n->disconnect = linux_disconnect;
    n->state = 0;
    n->cb_func = NULL;
    n->priv = NULL;
    n->used_tls1_2 = false;
}


void SetNetworkCb(Network *n, int (*cb_func)(enum sock_api_msg_type, void *), void *priv)
{
    n->cb_func = cb_func;
    n->priv = priv;
}


void SetNetworkRecvTimeout(Network *n, u32 timeout_ms)
{
    if (n && n->my_socket) {
        if (n->used_tls1_2 == 0) {
            sock_set_recv_timeout(n->my_socket, timeout_ms);
        } else {
#ifdef CONFIG_MQTT_USE_TLS1_2
            struct mbedtls_ctx *tls_ctx = (struct mbedtls_ctx *)n->my_socket;
            mbedtls_net_set_timeout(&tls_ctx->server_fd, tls_ctx->send_to_ms, timeout_ms);
            tls_ctx->recv_to_ms = timeout_ms;
#else
            printf("mqtt not define CONFIG_MQTT_USE_TLS1_2\n");
#endif
        }
    }
}


void SetNetworkSendTimeout(Network *n, u32 timeout_ms)
{
    if (n && n->my_socket) {
        if (n->used_tls1_2 == 0) {
            sock_set_send_timeout(n->my_socket, timeout_ms);
        } else  {
#ifdef CONFIG_MQTT_USE_TLS1_2
            struct mbedtls_ctx *tls_ctx = (struct mbedtls_ctx *)n->my_socket;
            mbedtls_net_set_timeout(&tls_ctx->server_fd, timeout_ms, tls_ctx->recv_to_ms);
            tls_ctx->send_to_ms = timeout_ms;
#else
            printf("mqtt not define CONFIG_MQTT_USE_TLS1_2\n");
#endif
        }
    }
}


int ConnectNetwork(Network *n, char *addr, int port)
{
    int type = SOCK_STREAM;
    struct sockaddr_in address;
    int rc = -1;
    sa_family_t family = AF_INET;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

    if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0) {
        struct addrinfo *res = result;

        /* prefer ip4 addresses */
        while (res) {
            if (res->ai_family == AF_INET) {
                result = res;
                break;
            }
            res = res->ai_next;
        }

        if (result->ai_family == AF_INET) {
            address.sin_port = htons(port);
            address.sin_family = family = AF_INET;
            address.sin_addr = ((struct sockaddr_in *)(result->ai_addr))->sin_addr;
        } else {
            rc = -1;
        }

        freeaddrinfo(result);
    }

    if (n->used_tls1_2 == 0) {
        if (rc == 0) {
            n->my_socket = sock_reg(family, type, 0, n->cb_func, n->priv);
            if (n->my_socket) {
                rc = sock_connect(n->my_socket, (struct sockaddr *)&address, sizeof(address));
            } else {
                rc = -1;
            }
        }
    } else {
#ifdef CONFIG_MQTT_USE_TLS1_2
        printf("MqttLinux socket use TLS1.2\n");
        char serv[10] = {0};
        itoa(port, serv, 10);

        n->my_socket = tls_connect(addr, serv, 5000, n);
        if (n->my_socket) {
            rc = 0;
        } else {
            rc = -1;
        }
#else
        printf("mqtt not define CONFIG_MQTT_USE_TLS1_2\n");
        rc = -1;
#endif
    }

    return rc;
}

void StackTrace_entry(const char *name, int line, int trace)
{
    //printf("StackTrace_entry:%s %d\n", name, line);
}
void StackTrace_exit(const char *name, int line, void *return_value, int trace)
{
    //printf("StackTrace_exit:%s %d\n", name, line);
}
