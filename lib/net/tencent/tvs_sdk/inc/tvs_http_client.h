#ifndef __TVS_HTTP_CLIENT_H__
#define __TVS_HTTP_CLIENT_H__

#include <stdbool.h>

typedef enum {
    TVS_HTTP_CLIENT_RET_OK = 0,
    TVS_HTTP_CLIENT_RET_ERROR = -1,
    TVS_HTTP_CLIENT_CONNECTION_TIMEOUT = -2,
    TVS_HTTP_CLIENT_RESPONSE_TIMEOUT = -3,
} tvs_http_client_ret_code;

typedef struct _tvs_http_client_param tvs_http_client_param;

typedef void(*tvs_http_client_callback_on_connect)(void *connection, int error_code, tvs_http_client_param *param);

typedef void(*tvs_http_client_callback_on_response)(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param);

typedef void(*tvs_http_client_callback_on_close)(void *connection, int by_server, tvs_http_client_param *param);

typedef int(*tvs_http_client_callback_on_start_request)(void *connection, const char *path, const char *host, const char *extra_header, const char *payload, tvs_http_client_param *param);

typedef void(*tvs_http_client_callback_on_recv_chunked)(void *connection, void *ev_data, const char *chunked, int chunked_len, tvs_http_client_param *param);

// 返回true代表loop break， 返回false代表go on
typedef bool (*tvs_http_client_callback_on_loop)(void *connection, tvs_http_client_param *param);

typedef bool (*tvs_http_client_callback_on_timeout)(void *connection, tvs_http_client_param *param);

typedef bool (*tvs_http_client_callback_exit_loop)(void *exit_param);

typedef bool (*tvs_http_client_callback_should_cancel)(void *exit_param);

typedef void (*tvs_http_client_callback_on_loop_end)(tvs_http_client_param *param);

extern char g_http_respone_session_id[80];

typedef struct {
    tvs_http_client_callback_on_connect cb_on_connect;
    tvs_http_client_callback_on_response cb_on_response;
    tvs_http_client_callback_on_close cb_on_close;
    tvs_http_client_callback_on_start_request cb_on_request;
    tvs_http_client_callback_on_recv_chunked cb_on_chunked;
    tvs_http_client_callback_on_loop cb_on_loop;
    tvs_http_client_callback_on_timeout cb_on_timeout;
    tvs_http_client_callback_on_loop_end cb_on_loop_end;
    tvs_http_client_callback_exit_loop exit_loop;
    tvs_http_client_callback_should_cancel should_cancel;
    void *exit_loop_param;
} tvs_http_client_callback;

struct _tvs_http_client_param {
    void *user_data;
    int exit_flag;
    int resp_code;
    bool connected;
    bool force_break;
    bool not_check_timer;
    bool timeout;
    tvs_http_client_callback *cb;
};

typedef struct {
    int connection_timeout_sec;
    int response_timeout_sec;
    int total_timeout;
} tvs_http_client_config;

int tvs_http_client_request(const char *url, const char *extra_headers, const char *payload, void *user_data,
                            tvs_http_client_config *config, tvs_http_client_callback *cb, tvs_http_client_param *param, bool use_ip_unavilable_strategy);

int tvs_http_client_request_with_forceip(const char *url, const char *extra_headers, const char *payload, void *user_data,
        tvs_http_client_config *config, tvs_http_client_callback *cb, tvs_http_client_param *param, bool use_ip_unavilable_strategy, int force_ip);


// 给下行通道用的，内部会处理IP-List
int tvs_http_client_request_ex1(const char *url, const char *extra_headers, const char *payload, void *user_data,
                                tvs_http_client_config *config, tvs_http_client_callback *cb, tvs_http_client_param *param, bool long_time_connection,
                                int poll_time, bool use_ip_unavilable_strategy);

unsigned long tvs_http_client_get_total_send_recv_size();

#endif
