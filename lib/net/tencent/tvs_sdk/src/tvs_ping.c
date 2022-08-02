
#define TVS_LOG_DEBUG_MODULE  "ping"
#include "tvs_log.h"

#include "tvs_threads.h"
#include "tvs_executor_service.h"
#include "tvs_config.h"
#include "tvs_http_client.h"
#include "tvs_http_manager.h"
#include "tvs_api_config.h"
#include "net_ping.h"
#include "os/os_api.h"

extern int mg_resolve2(const char *host, struct in_addr *ina);
extern bool tvs_ping_one();
extern int get_ping_recv_time();
extern u8 net_connected;


static void *g_tvs_ping_mutex = NULL;
static int g_tvs_ping_pid;
static int g_tvs_ping_start;

#define REFRESH_TIMER     210 // 时间间隔


int tvs_ping_host(const char *host, int times)
{

    if (times <= 0) {
        times = 2;
    }

    TVS_LOG_PRINTF("ping host %s\n", host);

    int ipaddr = 0;
    mg_resolve2(host, (struct in_addr *)&ipaddr);
    if (ipaddr == 0) {
        TVS_LOG_PRINTF("ping host %s, invalid ip\n", host);
        return -1;
    }
    struct net_ping_data pdata;
    memset(&pdata, 0, sizeof(struct net_ping_data));

    pdata.sin_addr.addr = ipaddr;
    pdata.count = times;
    pdata.data_long = 0xffff;

    return net_ping(&pdata);

}

#if 0
static tvs_thread_handle_t *g_thread = NULL;

static void ping_thread_func(tvs_thread_handle_t *thread_handle_t)
{
    void *param = tvs_thread_get_param(thread_handle_t);

    if (NULL == param) {
        return;
    }

    int ret = ping((struct ping_data *)param);

    TVS_LOG_PRINTF("ping ret %d\n", ret);

}

int tvs_ping_start(const char *host)
{
    if (NULL == host) {
        return -1;
    }

    int ipaddr = 0;
    mg_resolve2(host, (struct in_addr *)&ipaddr);

    if (ipaddr == 0) {
        return -1;
    }

    if (g_thread == NULL) {
        g_thread = tvs_thread_new(ping_thread_func, NULL);
    }

    if (!tvs_thread_is_stop(g_thread)) {
        return -1;
    }

    struct ping_data pdata = {0};

    pdata.sin_addr.addr = ipaddr;

    if (!tvs_thread_start_prepare(g_thread, &pdata, sizeof(ping_data))) {
        tvs_thread_start_now(g_thread, "ping", 2, 1024);
    }
}
#endif

typedef struct {
    char msg_str[20];
} tvs_ping_param;

void tvs_ping_callback_on_connect(void *connection, int error_code, tvs_http_client_param *param)
{

}

void tvs_ping_callback_on_response(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param)
{
    /* if (ret_code == 200 || ret_code == 204) { */
    /* TVS_LOG_PRINTF("ping success\n"); */
    /* } else { */
    /* TVS_LOG_PRINTF("get resonse err %d\n", ret_code); */
    /* } */
}

void tvs_ping_callback_on_close(void *connection, int by_server, tvs_http_client_param *param) { }

int tvs_ping_callback_on_send_request(void *connection, const char *path, const char *host, const char *extra_header, const char *payload, tvs_http_client_param *param)
{
    tvs_http_send_normal_get_request((struct mg_connection *)connection, path, strlen(path), host, strlen(host));
    return 0;
}

bool tvs_ping_callback_on_loop(void *connection, tvs_http_client_param *param)
{
    return false;
}

bool tvs_ping_start(tvs_http_client_callback_exit_loop should_exit_func,
                    tvs_http_client_callback_should_cancel should_cancel,
                    void *exit_param)
{

    tvs_ping_param ping_param;
    memset(&ping_param, 0, sizeof(tvs_ping_param));

    char *url = tvs_config_get_ping_url();

    //TVS_LOG_PRINTF("echo ip %d, url %s\n", ipaddr, url);

    tvs_http_client_callback cb = { 0 };
    cb.cb_on_close = tvs_ping_callback_on_close;
    cb.cb_on_connect = tvs_ping_callback_on_connect;
    cb.cb_on_loop = tvs_ping_callback_on_loop;
    cb.cb_on_request = tvs_ping_callback_on_send_request;
    cb.cb_on_response = tvs_ping_callback_on_response;
    cb.exit_loop = should_exit_func;
    cb.exit_loop_param = exit_param;
    cb.should_cancel = should_cancel;

    tvs_http_client_config config = { 0 };
    // ping的超时时间
    config.connection_timeout_sec = 7;
    config.total_timeout = 15;
    tvs_http_client_param http_param = { 0 };

    int ret = tvs_http_client_request(url, NULL, NULL, &ping_param, &config, &cb, &http_param, true);

    return ret == 200;
}


void tvs_ping_timer_func(void *param)
{
    while (g_tvs_ping_start) {
        tvs_executor_start_ping();
        os_wrapper_wait_signal(g_tvs_ping_mutex, REFRESH_TIMER * 1000);
    }
}

void tvs_ping_one_func()
{
    if (g_tvs_ping_start) {
        return ;
    }
    g_tvs_ping_start = 1;
    if (tvs_ping_one()) {
        if (get_ping_recv_time() < 100) {
            //网络良好
        } else {
            //网络较差
        }
    } else {
        //网络不通
        net_connected = 0;
    }
    g_tvs_ping_start = 0;
}

void tvs_ping_refresh_start()
{
    if (tvs_config_is_down_channel_enable()) {
        if (!g_tvs_ping_pid) {
            if (!g_tvs_ping_mutex) {
                g_tvs_ping_mutex = os_wrapper_create_signal_mutex(0);
            }
            g_tvs_ping_start = 1;
            thread_fork("tvs_ping_thread", 20, 256, 0, &g_tvs_ping_pid, tvs_ping_timer_func, NULL);
        }
    }
}

void tvs_ping_refresh_stop()
{
    if (g_tvs_ping_pid) {
        g_tvs_ping_start = 0;
        os_wrapper_post_signal(g_tvs_ping_mutex);
        thread_kill(&g_tvs_ping_pid, KILL_WAIT);
        g_tvs_ping_pid = 0;
        os_wrapper_delete_signal_mutex(&g_tvs_ping_mutex);
    }
}
