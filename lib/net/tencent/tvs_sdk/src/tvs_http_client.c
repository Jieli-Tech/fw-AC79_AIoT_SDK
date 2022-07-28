#include "tvs_http_client.h"
#include "mongoose.h"
#include "tvs_log.h"
#include "tvs_config.h"
#include "tvs_ip_provider.h"
#include "tvs_echo.h"
#include "string.h"

// 用于统计一共发送/接收了多少数据，用于流量统计
static unsigned long g_total_send_recv = 0;

char g_http_respone_session_id[80] = {0};

extern struct mg_connection *mg_connect_http_base(
    struct mg_mgr *mgr, mg_event_handler_t ev_handler,
    struct mg_connect_opts opts, const char *schema, const char *schema_ssl,
    const char *url, const char **path, char **user, char **pass, char **addr);

static void http_set_timer(struct mg_connection *connection, int time_sec)
{
    mg_set_timer(connection, mg_time() + time_sec);
}

static void clear_timer(struct mg_connection *connection)
{
    mg_set_timer(connection, 0);
}

static void read_session_id(struct http_message *hm)
{
    if (hm == NULL) {
        TVS_LOG_PRINTF("Not found session id,http message is null!!!\n");
        return;
    }
    memset(g_http_respone_session_id, 0, sizeof(g_http_respone_session_id));
    //printf("http respone head:\n");
    for (int i = 0; i < MG_MAX_HTTP_HEADERS; i++) {
        //printf("%.*s:%.*s\n",hm->header_names[i].len,hm->header_names[i].p,hm->header_values[i].len,hm->header_values[i].p);
        if (hm->header_names[i].p != NULL && strncasecmp(hm->header_names[i].p, "SessionId", 9) == 0) {
            int cpylen = hm->header_values[i].len;
            if (cpylen >= sizeof(g_http_respone_session_id)) {
                cpylen = sizeof(g_http_respone_session_id) - 1;
            }
            memcpy(g_http_respone_session_id, hm->header_values[i].p, cpylen);
            TVS_LOG_PRINTF("\nSessionid:%s\n", g_http_respone_session_id);
            break;
        }
    }
}

static void on_http_event_handler(struct mg_connection *connection, int event_type, void *event_data)
{
    struct http_message *hm = (struct http_message *)event_data;
    //TVS_LOG_PRINTF("%s -- %d\n", __func__, event_type);
    int error = 0;

    tvs_http_client_param *param = (tvs_http_client_param *)connection->mgr->user_data;
    switch (event_type) {
    case MG_EV_RECV:
    case MG_EV_SEND:
        // 流量统计
        g_total_send_recv += (unsigned long)(*((int *) event_data));
        TVS_LOG_PRINTF("%s %lu bytes\n", MG_EV_RECV == event_type ? "recv" : "send", g_total_send_recv);

        TVS_LOG_PRINTF("%s %d bytes\n", MG_EV_RECV == event_type ? "recv" : "send", (*((int *) event_data)));

        if (!param->not_check_timer) {
            // 每次收到或者发送数据，刷新timer
            http_set_timer(connection, 15);
        }
        break;
    case MG_EV_CONNECT:
        // error的错误码定义，请参考lwip/errno.h
        error = ((int *)event_data)[0];
        if (error == 0) {
            param->connected = true;
        } else {
            param->exit_flag = 1;
        }

        if (param->cb->cb_on_connect != NULL) {
            param->cb->cb_on_connect(connection, error, param);
        }
        if (!param->not_check_timer) {
            http_set_timer(connection, 10);
        } else {
            clear_timer(connection);
        }
        break;
    case MG_EV_HTTP_CHUNK:
        if (param->cb->cb_on_chunked != NULL) {
            param->cb->cb_on_chunked(connection, event_data, hm->body.p, hm->body.len, param);
        }
        connection->flags |= MG_F_DELETE_CHUNK;
        break;
    case MG_EV_TIMER:
        TVS_LOG_PRINTF("http request time out\n");
        if (param->cb->cb_on_timeout != NULL) {
            param->cb->cb_on_timeout(connection, param);
        }

        param->timeout = true;
        connection->flags |= MG_F_CLOSE_IMMEDIATELY;
        break;
    case MG_EV_HTTP_REPLY:
        param->resp_code = hm->resp_code;
        read_session_id(hm);
        if (param->cb->cb_on_response != NULL) {
            param->cb->cb_on_response(connection, hm->resp_code, hm->body.p, hm->body.len, param);
        }

        connection->flags |= MG_F_CLOSE_IMMEDIATELY;
        break;
    case MG_EV_CLOSE:
        if (param->cb->cb_on_close != NULL) {
            param->cb->cb_on_close(connection, param->exit_flag == 0, param);
        }
        param->exit_flag = 1;
        break;
    default:
        break;
    }
}

static struct mg_connection *start_event_connection(struct mg_mgr *mgr, mg_event_handler_t ev_handler, const char *url, const char **path, char **addr)
{
    struct mg_connect_opts opts;
    memset(&opts, 0, sizeof(opts));

    char *user = NULL, *pass = NULL;

    struct mg_connection *conn = mg_connect_http_base(mgr, ev_handler, opts, "http://", "https://", url,
                                 path, &user, &pass, addr);

    if (user != NULL) {
        TVS_FREE(user);
    }

    if (pass != NULL) {
        TVS_FREE(pass);
    }

    return conn;
}

static int get_ip_from_provider()
{
    return tvs_ip_provider_get_valid_ip();
}

unsigned long tvs_http_client_get_total_send_recv_size()
{
    return g_total_send_recv;
}

static int tvs_http_client_request_inner(const char *url, const char *extra_headers, const char *payload, void *user_data,
        tvs_http_client_config *config, tvs_http_client_callback *cb, tvs_http_client_param *http_param,
        bool long_time_connection, int poll_time, bool use_ip_unavilable_strategy, int force_ip)
{

    // use_ip_unavilable_strategy参数用于speech、control、授权、下行通道等访问tvs域名的情况，可以触发IP失效策略

    struct mg_mgr _mgr;

    //清除上次保存的session id
    memset(g_http_respone_session_id, 0, sizeof(g_http_respone_session_id));

    TVS_LOG_PRINTF("start request %s, force_ip %d - %s\n", url, force_ip, inet_ntoa(*(struct in_addr *)&force_ip));

    memset(&_mgr, 0, sizeof(struct mg_mgr));

    struct mg_mgr *mgr = &_mgr;
    const char *path = NULL;
    char *addr = NULL;
    int ret = 0;

    bool force_cancel = false;

    memset(http_param, 0, sizeof(tvs_http_client_param));
    http_param->user_data = user_data;
    http_param->cb = cb;
    http_param->not_check_timer = long_time_connection;

    mg_mgr_init(mgr, http_param);

    struct mg_connection *conn = NULL;

    long start_time = os_wrapper_get_time_ms();

    do {
        if (force_ip != 0) {
            if (tvs_echo_is_in_error_list(force_ip)) {
                // 测试用，如果在echo黑名单里，则请求失败
                mgr->force_ip = 67305985;
            } else {
                mgr->force_ip = force_ip;
            }
        }

        conn = start_event_connection(mgr, on_http_event_handler, url, &path, &addr);

        if (conn == NULL) {
            break;
        }

        int conn_timeout = 8;
        if (config != NULL && config->connection_timeout_sec > 0) {
            conn_timeout = config->connection_timeout_sec;
        }

        //TVS_LOG_PRINTF("set connection timeout %d s\n", conn_timeout);
        http_set_timer(conn, conn_timeout);

        if (cb->cb_on_request != NULL) {
            ret = cb->cb_on_request(conn, path, addr, extra_headers, payload, http_param);
        } else {
            ret = -1;
        }
        if (ret != 0) {
            break;
        }
        while (http_param->exit_flag == 0) {
            if (config != NULL && config->total_timeout > 0) {
                if (os_wrapper_get_time_ms() - start_time > config->total_timeout * 1000) {
                    TVS_LOG_PRINTF("http total timeout\n");
                    break;
                }
            }

            if (cb->exit_loop != NULL && cb->exit_loop(cb->exit_loop_param)) {
                TVS_LOG_PRINTF("http force break\n");
                http_param->force_break = true;
                break;
            }

            if (cb->should_cancel != NULL && cb->should_cancel(cb->exit_loop_param)) {
                TVS_LOG_PRINTF("http force cancel\n");
                force_cancel = true;
                break;
            }

            if (cb->cb_on_loop != NULL) {
                if (cb->cb_on_loop(conn, http_param)) {
                    break;
                }
            }
            mg_mgr_poll(mgr, poll_time);
        }
    } while (0);

    mg_mgr_free(mgr);

    if (cb->cb_on_loop_end != NULL) {
        cb->cb_on_loop_end(http_param);
    }

    if (addr != NULL) {
        TVS_FREE(addr);
    }

    if (cb->exit_loop != NULL && !http_param->force_break && cb->exit_loop(cb->exit_loop_param)) {
        http_param->force_break = true;
    }

    if (force_ip != 0 && use_ip_unavilable_strategy && !http_param->connected && !http_param->force_break && !force_cancel) {
        // 服务器未连接上，且不是强制break或者强制cancel的情况下，触发IP失效机制
        tvs_ip_provider_on_ip_invalid(force_ip);
    }

    return http_param->resp_code;
}

int tvs_http_client_request(const char *url, const char *extra_headers, const char *payload, void *user_data,
                            tvs_http_client_config *config, tvs_http_client_callback *cb, tvs_http_client_param *param, bool use_ip_unavilable_strategy)
{
    return tvs_http_client_request_inner(url, extra_headers, payload, user_data, config, cb, param, false, 20, use_ip_unavilable_strategy, get_ip_from_provider());
}

int tvs_http_client_request_ex1(const char *url, const char *extra_headers, const char *payload, void *user_data,
                                tvs_http_client_config *config, tvs_http_client_callback *cb, tvs_http_client_param *param, bool long_time_connection,
                                int poll_time, bool use_ip_unavilable_strategy)
{
    // for 下行通道
    return tvs_http_client_request_inner(url, extra_headers, payload, user_data, config, cb, param, long_time_connection, poll_time, use_ip_unavilable_strategy, get_ip_from_provider());
}

int tvs_http_client_request_with_forceip(const char *url, const char *extra_headers, const char *payload, void *user_data,
        tvs_http_client_config *config, tvs_http_client_callback *cb, tvs_http_client_param *param, bool use_ip_unavilable_strategy, int force_ip)
{
    return tvs_http_client_request_inner(url, extra_headers, payload, user_data, config, cb, param, false, 100, use_ip_unavilable_strategy, force_ip);
}
