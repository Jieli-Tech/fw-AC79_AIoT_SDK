#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <nopoll.h>
#include "os/os_api.h"

//#define WEBSOCKET_SSL_ENABLE
//#define DEBUG_ENABLE

#if defined(WEBSOCKET_SSL_ENABLE)
//wss://192.168.10.169:443
static char *serverHost = "192.168.10.100";
static char *serverPort = "443";
#else
//ws://192.168.10.169:80
static char *serverHost = "192.168.10.100";
static char *serverPort = "80";
#endif // WEBSOCKET_SSL_ENABLE

static char *websocket_server_ca =
    "-----BEGIN CERTIFICATE-----\r\n"
    "MIID4DCCA0mgAwIBAgIJAN+fokFBG4UhMA0GCSqGSIb3DQEBBQUAMIGnMQswCQYD\r\n"
    "VQQGEwJFUzEPMA0GA1UECBMGTWFkcmlkMRswGQYDVQQHFBJBbGNhbMOhIGRlIEhl\r\n"
    "bmFyZXMxNzA1BgNVBAoTLkFTUEwgKEFkdmFuY2VkIFNvZnR3YXJlIFByb2R1Y3Rp\r\n"
    "b24gTGluZSwgUy5MLikxEzARBgNVBAsTClRJIFN1cHBvcnQxHDAaBgNVBAMTE3Rl\r\n"
    "c3Qubm9wb2xsLmFzcGwuZXMwHhcNMTMwMzAzMDkxOTU2WhcNMjMwMzAxMDkxOTU2\r\n"
    "WjCBpzELMAkGA1UEBhMCRVMxDzANBgNVBAgTBk1hZHJpZDEbMBkGA1UEBxQSQWxj\r\n"
    "YWzDoSBkZSBIZW5hcmVzMTcwNQYDVQQKEy5BU1BMIChBZHZhbmNlZCBTb2Z0d2Fy\r\n"
    "ZSBQcm9kdWN0aW9uIExpbmUsIFMuTC4pMRMwEQYDVQQLEwpUSSBTdXBwb3J0MRww\r\n"
    "GgYDVQQDExN0ZXN0Lm5vcG9sbC5hc3BsLmVzMIGfMA0GCSqGSIb3DQEBAQUAA4GN\r\n"
    "ADCBiQKBgQC4GcLeRL4busZZ5IqJQunH5wxYgVHEIgt499HBJ78BYv5Wi0lVvD4x\r\n"
    "/fUlvGsvsID0WQGzsgim03KUInDA30vFqBhsz2Eooi0aO0evOrVQmnK1RCL6EQzv\r\n"
    "L+EVYIBWk4FVWlxYxsFMyf7rLSz8wrJThDFcyNeq1lIBFlOzGNnHwQIDAQABo4IB\r\n"
    "EDCCAQwwHQYDVR0OBBYEFE/5BX/ShOWk6EbFgbcf4XQGpSJBMIHcBgNVHSMEgdQw\r\n"
    "gdGAFE/5BX/ShOWk6EbFgbcf4XQGpSJBoYGtpIGqMIGnMQswCQYDVQQGEwJFUzEP\r\n"
    "MA0GA1UECBMGTWFkcmlkMRswGQYDVQQHFBJBbGNhbMOhIGRlIEhlbmFyZXMxNzA1\r\n"
    "BgNVBAoTLkFTUEwgKEFkdmFuY2VkIFNvZnR3YXJlIFByb2R1Y3Rpb24gTGluZSwg\r\n"
    "Uy5MLikxEzARBgNVBAsTClRJIFN1cHBvcnQxHDAaBgNVBAMTE3Rlc3Qubm9wb2xs\r\n"
    "LmFzcGwuZXOCCQDfn6JBQRuFITAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBQUA\r\n"
    "A4GBAKucVMTXYsW8x06XxTNXFbsLYGMc/UIMx5w4DbYIOXJz3yvIdJ8Alzg2O7DR\r\n"
    "XDzDJUE+WHlq7ZafVIFIWyY+eB5lINAMMTiwrVoqoyTT1X3vNd3W7Y0/v5afXxQ1\r\n"
    "UvTcOPeIkxs3Iee51BkjU98k7FwNBccPuVSyMKhdbIolC7Zb\r\n"
    "-----END CERTIFICATE-----\r\n";

static char *websocket_private_key =
    "-----BEGIN RSA PRIVATE KEY-----\r\n"
    "MIICXAIBAAKBgQC4GcLeRL4busZZ5IqJQunH5wxYgVHEIgt499HBJ78BYv5Wi0lV\r\n"
    "vD4x/fUlvGsvsID0WQGzsgim03KUInDA30vFqBhsz2Eooi0aO0evOrVQmnK1RCL6\r\n"
    "EQzvL+EVYIBWk4FVWlxYxsFMyf7rLSz8wrJThDFcyNeq1lIBFlOzGNnHwQIDAQAB\r\n"
    "AoGAIGUIESxvd1mqRW8doYGQuYhCd+BpjuWety6ETkS8K3ZL4tanlNqG5y0U0gsR\r\n"
    "oVahml1/Gyucsh5K7x4QUR/5qQWqN+da9zndO2ssH0Viyu6vxvPeMEbPe74qMsT4\r\n"
    "eO+slkpJVFZzpYP8sPsoBlJ5nZHtpD47UjfZBKXXbEyNY/ECQQDjq1yIMIb/W8YJ\r\n"
    "7rwiEYHUdMD9YeTHjkDZWVfuwwdoM4ut7O0UXuScME/XY3PbAjpDO/utTL/cKhSJ\r\n"
    "pIlXH7fzAkEAzwJ5HJfBaBz0dhYIG4Y7P/AnhRBgCA6m7mrzAci4vqwLw1poD573\r\n"
    "gSNMCxZPk2vm6pO+b3cEeu18KJvvgcGCewJAQcyb8Kx9x73Bbct2yi3fJQUdZd3u\r\n"
    "HhKqAWdF97acJGyJWRoZpwKJ9e4slSakLE7ngdkLMxn0dXAgAWvxWaHMKwJAWMPQ\r\n"
    "twgDsOcplDEiTNskMOiDqbU52Hqf7gACL7OoNGqFqMDtejVKIB/IjcCFYsuT+uZb\r\n"
    "dGRukV+gK7Gh49vcXQJBAJdCc9PAaj1w729vMl+uXZX7luHOa5/rxMcNsjUR25q9\r\n"
    "BnByCR1YUnSr2H6PEfX0qxA8jt38BgZj0zb90Kt2p64=\r\n"
    "-----END RSA PRIVATE KEY-----\r\n";

void listener_on_message(noPollCtx *ctx, noPollConn *conn, noPollMsg *msg, noPollPtr  user_data)
{
    if (nopoll_msg_is_fragment(msg)) {
        printf("Found fragment, FIN = %d (%p)?..\n", nopoll_msg_is_final(msg), msg);
        return;
    }

    printf("the data received : %s\n", (const char *) nopoll_msg_get_payload(msg));

    /* send reply as received */
    printf("Sending reply... (same message size: %d)\n", nopoll_msg_get_payload_size(msg));
    nopoll_conn_send_text(conn, (const char *) nopoll_msg_get_payload(msg),
                          nopoll_msg_get_payload_size(msg));
}

static int nopoll_server_test(void)
{
    noPollConn *listener = NULL;
    noPollCtx *ctx = NULL;

    /* create the context */
    ctx = nopoll_ctx_new();
    if (!ctx) {
        printf("ERROR: nopoll_ctx_new fail\n");
        return -1;
    }

#if defined(DEBUG_ENABLE)
    nopoll_log_enable(ctx, nopoll_true);
#endif

#if defined(WEBSOCKET_SSL_ENABLE)
    /* now start a TLS version */
    listener = nopoll_listener_tls_new(ctx, serverHost, serverPort);
#else
    /* call to create a listener */
    listener = nopoll_listener_new(ctx, serverHost, serverPort);
#endif // WEBSOCKET_SSL_ENABLE

    if (! nopoll_conn_is_ok(listener)) {
        printf("ERROR: Expected to find proper listener connection status, but found..\n");
        return -1;
    }

#if defined(WEBSOCKET_SSL_ENABLE)
    /* configure certificates to be used by this listener */
    if (!nopoll_listener_set_certificate(listener, websocket_server_ca, strlen(websocket_server_ca) + 1, websocket_private_key, strlen(websocket_private_key) + 1, NULL, 0)) {
        printf("ERROR: unable to configure certificates for TLS websocket..\n");
        return -1;
    }

    /* register certificates at context level */
    if (! nopoll_ctx_set_certificate(nopoll_server_ctx, NULL, websocket_server_ca, strlen(websocket_server_ca) + 1, websocket_private_key, strlen(websocket_private_key) + 1, NULL, 0)) {
        printf("ERROR: unable to setup certificates at context level..\n");
        return -1;
    }
#endif //WEBSOCKET_SSL_ENABLE

    // now set a handler that will be called when a message (fragment or not) is received
    nopoll_ctx_set_on_msg(ctx, listener_on_message, NULL);

    // now call to wait for the loop to notify events
    nopoll_loop_wait(ctx, 0);

    /* finish */
    printf("Listener: finishing references: %d\n", nopoll_ctx_ref_count(ctx));

    /* unref connection */
    nopoll_conn_close(listener);

    nopoll_ctx_unref(ctx);

    /* call to release all pending memory allocated as a
     * consequence of using nopoll (especially TLS) */
    nopoll_cleanup_library();

    return 0;
}

static void websocket_test_task(void *priv)
{
    nopoll_server_test();
}

void nopoll_server_main(void)
{
    if (thread_fork("websocket_test_task", 10, 4 * 1024, 0, NULL, websocket_test_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}
