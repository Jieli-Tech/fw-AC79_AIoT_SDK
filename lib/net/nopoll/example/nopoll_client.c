#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <nopoll.h>
#include "os/os_api.h"

//#define WEBSOCKET_SSL_ENABLE
//#define DEBUG_ENABLE

#if defined(WEBSOCKET_SSL_ENABLE)
//wss://echo.websocket.org
static char *hostName = "echo.websocket.org";
static char *hostPort = "443";
static char *pathUrl = "/";
#else
//ws://echo.websocket.org
static char *hostName = "echo.websocket.org";
static char *hostPort = "80";
static char *pathUrl = "/";
#endif // WEBSOCKET_SSL_ENABLE

static char *websocket_test_ca = NULL;
static noPollCtx *ctx = NULL;
static noPollConn *conn = NULL;
static noPollConnOpts *opts = NULL;
const char *websocket_msg = "websocket client test!";
extern struct hostent *gethostbyname(const char *name);

static int nopoll_client_test(char *hostName, char *hostPort, char *pathUrl)
{
    struct hostent *hp;
    int ret = -1;
    char *hostIp;
    noPollMsg *msg = NULL;
    const char *payload = NULL;
    int length = 0;
    int iterator = 0;

    ctx = nopoll_ctx_new();
    if (ctx == NULL) {
        printf("ERROR: nopoll_ctx_new failed\n");
        ret = -1;
        goto exit;
    }

#if defined(DEBUG_ENABLE)
    nopoll_log_enable(ctx, nopoll_true);
#endif

    opts = nopoll_conn_opts_new();
    if (opts == NULL) {
        printf("ERROR: nopoll_conn_opts_new failed\n");
        ret = -1;
        goto exit;
    }

    hp = gethostbyname(hostName);
    if (hp != NULL) {
        hostIp = inet_ntoa(*(struct in_addr *)hp->h_addr);
        printf("host ip:%s\n", hostIp);
    } else {
        printf("ERROR: get host ip err\n");
        goto exit;
    }

#if defined(WEBSOCKET_SSL_ENABLE)
    /* if use ssl, then set the certs */
    if (!nopoll_conn_opts_set_ssl_certs(opts, NULL, 0, NULL, 0, NULL, 0, \
                                        websocket_test_ca, strlen(websocket_test_ca) + 1)) {
        printf("nopoll_conn_opts_set_ssl_certs failed\n");
        goto exit;
    }

    /* set ssl verfy */
    nopoll_conn_opts_ssl_peer_verify(opts, nopoll_true);
    conn = nopoll_conn_tls_new(ctx, opts, hostIp, hostPort, hostName, pathUrl, NULL, NULL);
#else
    conn = nopoll_conn_new_opts(ctx, opts, hostIp, hostPort, hostName, pathUrl, NULL, NULL);
#endif // WEBSOCKET_SSL_ENABLE

    if (! nopoll_conn_is_ok(conn)) {
        printf("ERROR: Expected to find proper client connection status, but found error..\n");
        goto exit;
    }

    printf("waiting until connection is ok\n");

    if (nopoll_conn_wait_until_connection_ready(conn, 10)) {
        printf("ERROR: failed to fully establish connection\n");
    } else {
        printf("connection timeout\n");
        goto exit;
    }

    length = strlen(websocket_msg);
    ret = nopoll_conn_send_text(conn, websocket_msg, length);
    if (ret != length) {
        iterator = 0;
        while (iterator < 10) {
            printf("found pending write bytes=%d\n", nopoll_conn_pending_write_bytes(conn));

            /* call to flush bytes */
            nopoll_conn_complete_pending_write(conn);

            if (nopoll_conn_pending_write_bytes(conn) == 0) {
                printf("all bytes written..\n");
                break;
            } /* end if */

            /* sleep a bit */
            nopoll_sleep(10000);

            /* next iterator */
            iterator++;
        }
    }

    while (1) {
        if (!nopoll_conn_is_ok(conn)) {
            printf("websocket connection close\n");
            break;
        }

        msg = nopoll_conn_get_msg(conn);
        if (msg) {
            payload = (char *)nopoll_msg_get_payload(msg);
            printf("websock received msg from %s: >>>[%s]<<<\n", hostName, payload);

            nopoll_sleep(10000);
            ret = nopoll_conn_send_text(conn, websocket_msg, length);
            if (ret == -1) {
                break;
            }
        }
    }

exit:
    /* finish connection */
    nopoll_conn_close(conn);

    /* finish */
    nopoll_ctx_unref(ctx);

    return ret;
}

static void websocket_test_task(void *priv)
{
    nopoll_client_test(hostName, hostPort, pathUrl);
}

void nopoll_client_main(void)
{
    if (thread_fork("websocket_test_task", 10, 4 * 1024, 0, NULL, websocket_test_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}
