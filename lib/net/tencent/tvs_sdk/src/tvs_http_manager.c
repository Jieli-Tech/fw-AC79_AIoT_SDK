
#include "tvs_http_manager.h"
#include "tvs_config.h"

#include "tvs_authorizer.h"
#define TVS_LOG_DEBUG  0

#define TVS_LOG_DEBUG_MODULE  "HTTPM"
#include "tvs_log.h"

#include "mongoose.h"

#define TVS_REQUEST_BOUNDARY      "tvs_free_rtos_bondary_2019"

#define TENCENT_SANDBOX      "TvsSettings: env=sandbox\r\n"


#define TVS_HTTP_HEADER_PARAM    \
	"Connection: Keep-Alive\r\n" \
	"user-agent: free_rtos/1.0.0\r\n" \
	"force_http: true\r\n"

#define TVS_HTTP_RECOGNIZE_HEADER_FMT    \
		"POST %.*s HTTP/1.1\r\n" \
		"HOST: %.*s\r\n" \
		"Authorization: %s\r\n" \
		"Q-UA: %s\r\n" \
		"Transfer-Encoding: chunked\r\n" \
		"Content-Type: multipart/form-data; boundary=" TVS_REQUEST_BOUNDARY "\r\n" \
		"%s" \
		TVS_HTTP_HEADER_PARAM \
		"\r\n"

#define TVS_HTTP_NOMAL_HEADER_FMT    \
		"POST %.*s HTTP/1.1\r\n" \
		"HOST: %.*s\r\n" \
		"Authorization: %s\r\n" \
		"Q-UA: %s\r\n" \
		"Content-Length: %d\r\n" \
		"Content-Type: multipart/form-data; boundary=" TVS_REQUEST_BOUNDARY "\r\n" \
		TVS_HTTP_HEADER_PARAM \
		"%s" \
		"\r\n" \
		"%s"

#define TVS_HTTP_GET_HEADER_FMT    \
		"GET %.*s HTTP/1.1\r\n" \
		"HOST: %.*s\r\n" \
		"Authorization: %s\r\n" \
		"Q-UA: %s\r\n" \
		"%s" \
		TVS_HTTP_HEADER_PARAM \
		"\r\n"

#define TVS_HTTP_POST_HEADER_FMT    \
		"POST %.*s HTTP/1.1\r\n" \
		"HOST: %.*s\r\n" \
		"Q-UA: %s\r\n" \
		"Content-Length: %d\r\n" \
		TVS_HTTP_HEADER_PARAM \
		"%s" \
		"\r\n" \
		"%s"

typedef struct {
    int exit_flag;
    int resp_code;
    bool connected;
} tvs_http_manager_param;

unsigned int tvs_htonl(unsigned int ip)
{
    return mg_htonl(ip);
}

int tvs_http_send_normal_multipart_request(struct mg_connection *conn, const char *path, int path_len,
        const char *host, int host_len, const char *metadata)
{
    if (metadata == NULL || strlen(metadata) == 0) {
        metadata = "{ }";
    }

    char *auth = tvs_authorizer_get_authtoken();
    char *qua = tvs_config_get_qua();
    bool sandbox = tvs_config_is_sandbox_open();

    TVS_LOG_PRINTF("send normal header\n");

    int metadata_len = strlen(metadata);

    char *body_fmt_str =
        "--" TVS_REQUEST_BOUNDARY "\r\n"
        "Content-Disposition: form-data; name=\"metadata\"\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s"
        "\r\n"
        "--" TVS_REQUEST_BOUNDARY "--\r\n";

    int body_len = strlen(body_fmt_str) + metadata_len + 100;

    char *body_buffer = TVS_MALLOC(body_len);
    if (NULL == body_buffer) {
        return -1;
    }

    memset(body_buffer, 0, body_len);
    sprintf(body_buffer, body_fmt_str, metadata_len, metadata);

    body_len = strlen(body_buffer);

    mg_printf(conn, TVS_HTTP_NOMAL_HEADER_FMT, path_len, path, host_len, host, auth, qua, body_len, !sandbox ? "" : TENCENT_SANDBOX, body_buffer);

    TVS_FREE(body_buffer);

    return 0;
}

int tvs_http_send_normal_get_request(struct mg_connection *conn, const char *path, int path_len,
                                     const char *host, int host_len)
{
    char *auth = tvs_authorizer_get_authtoken();
    char *qua = tvs_config_get_qua();
    bool sandbox = tvs_config_is_sandbox_open();
    mg_printf(conn, TVS_HTTP_GET_HEADER_FMT, (int)strlen(path), path, host_len, host, auth, qua, !sandbox ? "" : TENCENT_SANDBOX);

    return 0;
}

int tvs_http_send_normal_post(struct mg_connection *conn, const char *path, int path_len,
                              const char *host, int host_len, const char *payload)
{
    char *qua = tvs_config_get_qua();
    bool sandbox = tvs_config_is_sandbox_open();

    int body_len = strlen(payload);

    mg_printf(conn, TVS_HTTP_POST_HEADER_FMT, path_len, path, host_len, host, qua, body_len, !sandbox ? "" : TENCENT_SANDBOX, payload);

    return 0;
}

void tvs_http_send_multipart_start(struct mg_connection *conn, const char *path, int path_len,
                                   const char *host, int host_len, const char *metadata)
{
    if (metadata == NULL || strlen(metadata) == 0) {
        metadata = "{ }";
    }

    char *fmt_str = TVS_HTTP_RECOGNIZE_HEADER_FMT;

    char *auth = tvs_authorizer_get_authtoken();
    char *qua = tvs_config_get_qua();
    bool sandbox = tvs_config_is_sandbox_open();

    mg_printf(conn, fmt_str, path_len, path, host_len, host, auth, qua, !sandbox ? "" : TENCENT_SANDBOX, metadata);

    int metadata_len = strlen(metadata);

    fmt_str =
        "--" TVS_REQUEST_BOUNDARY "\r\n"
        "Content-Disposition: form-data; name=\"metadata\"\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s"
        "\r\n"
        "--" TVS_REQUEST_BOUNDARY "\r\n"
        "Content-Disposition: form-data; name=\"audio\"\r\n"
        "Content-Type: application/octet-stream\r\n"
        "\r\n";

    mg_printf_http_chunk(conn, fmt_str, metadata_len, metadata);
    TVS_LOG_PRINTF("send multipart start\n");
    //TVS_LOG_PRINTF("send multipart start%.*s\n", conn->send_mbuf.len, conn->send_mbuf.buf);
}

void tvs_http_send_multipart_data(struct mg_connection *conn, const char *bin_data, int bin_data_len)
{
    //TVS_LOG_PRINTF("send multipart data %d bytes\n", bin_data_len);
    mg_send_http_chunk(conn, bin_data, bin_data_len);
}

void tvs_http_send_multipart_end(struct mg_connection *conn)
{
    //TVS_LOG_PRINTF("send multipart end\n");

    char *request_str =
        "\r\n"
        "--" TVS_REQUEST_BOUNDARY "--\r\n";
    TVS_LOG_PRINTF("send audio chunked end\n");
    mg_send_http_chunk(conn, request_str, strlen(request_str));
    mg_send_http_chunk(conn, "", 0);
}
