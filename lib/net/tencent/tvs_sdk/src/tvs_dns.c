#include "tvs_http_client.h"
#include "tvs_http_manager.h"

#include "mongoose.h"

#define TVS_LOG_DEBUG_MODULE  "DNS"
#include "tvs_log.h"
#include "tvs_config.h"
#include "tvs_echo.h"
#include "tvs_ip_provider.h"

#include "tvs_threads.h"

#define HTTP_DNS_PATH   "http://119.29.29.29:80/d?dn="

static long g_last_dns_time = 0;

static int g_http_dns_ip = 0;
static int g_system_dns_ip = 0;

static int g_http_dns_ip_tmp = 0;
static int g_system_dns_ip_tmp = 0;

// 兜底IP
#define BACKUP_IP   "183.3.225.67"

static tvs_thread_handle_t *g_tvs_dns_thread = NULL;

typedef struct {
    int ip_addr;
} tvs_http_dns_param;

TVS_LOCKER_DEFINE

extern void tvs_dns_fetch_all();

void tvs_dns_retry_dns(tvs_http_client_callback_exit_loop should_exit_func,
                       tvs_http_client_callback_should_cancel should_cancel_func, void *exit_param)
{
    long cur_time = os_wrapper_get_time_ms();
    if (g_last_dns_time == 0 || cur_time - g_last_dns_time > 600 * 1000) {
        TVS_LOG_PRINTF("dns timeout, begin\n");

        g_last_dns_time = cur_time;
        // 超时，重新获取一次dns
        tvs_dns_fetch_all(should_exit_func, should_cancel_func, exit_param);
    }
}

void tvs_http_dns_callback_on_connect(void *connection, int error_code, tvs_http_client_param *param)
{
    //TVS_LOG_PRINTF("send iplist connect\n");
}

void tvs_http_dns_callback_on_response(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param)
{
    if (ret_code == 200) {

        unsigned int a, b, c, d = 0;
        if (response_len > 0 && response != NULL) {
            char *ip_str = malloc(response_len + 1);
            if (ip_str == NULL) {
                TVS_LOG_PRINTF("get resonse ip OOM\n");
                return;
            }

            memset(ip_str, 0, response_len + 1);
            strncpy(ip_str, response, response_len);

            if (sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
                tvs_http_dns_param *dns_param = (tvs_http_dns_param *)param->user_data;
                dns_param->ip_addr =
                    mg_htonl(((uint32_t) a << 24) | ((uint32_t) b << 16) | c << 8 | d);
                //TVS_LOG_PRINTF("get resonse ip %s, %d\n", response, dns_param->ip_addr);
            }

            free(ip_str);
        }

    } else {
        TVS_LOG_PRINTF("get resonse err %d - %.*s\n", ret_code, response_len, response);
    }
}

void tvs_http_dns_callback_on_close(void *connection, int by_server, tvs_http_client_param *param) { }

int tvs_http_dns_callback_on_send_request(void *connection, const char *path, const char *host, const char *extra_header, const char *payload, tvs_http_client_param *param)
{
    tvs_http_send_normal_get_request((struct mg_connection *)connection, path, strlen(path), host, strlen(host));
    return 0;
}

bool tvs_http_dns_callback_on_loop(void *connection, tvs_http_client_param *param)
{
    return false;
}

int tvs_http_dns_start(const char *host,
                       tvs_http_client_callback_exit_loop should_exit_func,
                       tvs_http_client_callback_should_cancel should_cancel_func,
                       void *exit_param,
                       int *ip_addr)
{
    if (host == NULL || strlen(host) <= 0) {
        TVS_LOG_PRINTF("%s -- invalid host\n", __func__);
        return -1;
    }
    int max_buf_len = 200;
    char *http_dns_url = malloc(max_buf_len);
    if (NULL == http_dns_url) {
        TVS_LOG_PRINTF("%s -- OOM\n", __func__);
        return -1;
    }

    memset(http_dns_url, 0, max_buf_len);

    sprintf(http_dns_url, "%s%s", HTTP_DNS_PATH, host);

    //TVS_LOG_PRINTF("send http dns url %s\n", http_dns_url);

    tvs_http_client_callback cb = { 0 };
    cb.cb_on_close = tvs_http_dns_callback_on_close;
    cb.cb_on_connect = tvs_http_dns_callback_on_connect;
    cb.cb_on_loop = tvs_http_dns_callback_on_loop;
    cb.cb_on_request = tvs_http_dns_callback_on_send_request;
    cb.cb_on_response = tvs_http_dns_callback_on_response;
    cb.exit_loop = should_exit_func;
    cb.should_cancel = should_cancel_func;
    cb.exit_loop_param = exit_param;

    tvs_http_client_config config = { 0 };
    tvs_http_client_param http_param = { 0 };
    tvs_http_dns_param http_dns_param = { 0 };

    int ret = tvs_http_client_request_with_forceip(http_dns_url, NULL, NULL, &http_dns_param, &config, &cb, &http_param, false, 0);

    if (http_dns_url != NULL) {
        free(http_dns_url);
    }

    *ip_addr = http_dns_param.ip_addr;
    TVS_LOG_PRINTF("send http dns result %d\n", *ip_addr);

    return ret;
}

extern int mg_resolve2(const char *host, struct in_addr *ina);

int tvs_resolve(const char *host, int *ina)
{
    return mg_resolve2(host, (struct in_addr *)ina);
}

int tvs_dns_system_start(const char *host)
{
    int ipaddr = 0;
    tvs_resolve(host, &ipaddr);

    return ipaddr;
}

void tvs_dns_fetch_all(tvs_http_client_callback_exit_loop should_exit_func,
                       tvs_http_client_callback_should_cancel should_cancel_func, void *exit_param)
{
    const char *host = tvs_config_get_current_host();
    int ipaddr = 0;
    tvs_http_dns_start(host, should_exit_func, should_cancel_func, exit_param, &ipaddr);
    TVS_LOG_PRINTF("tvs_dns_fetch_all http dns %d \n", ipaddr);
    if (ipaddr != 0) {
        do_lock();
        g_http_dns_ip = ipaddr;

        g_http_dns_ip_tmp = g_http_dns_ip;
        TVS_LOG_PRINTF("get http dns %d - %s\n", ipaddr, inet_ntoa(*(struct in_addr *)&ipaddr));
        do_unlock();
    }

    ipaddr = tvs_dns_system_start(host);

    if (ipaddr != 0) {
        do_lock();
        if (g_http_dns_ip != ipaddr) {
            g_system_dns_ip = ipaddr;
            TVS_LOG_PRINTF("get system dns %d - %s\n", ipaddr, inet_ntoa(*(struct in_addr *)&ipaddr));
        } else {
            g_system_dns_ip = 0;
        }

        g_system_dns_ip_tmp = g_system_dns_ip;

        do_unlock();
    }
}

bool tvs_http_dns_check_start()
{
    return true;
}

bool tvs_http_dns_remove_ip(int ip_addr)
{
    if (ip_addr == 0) {
        return false;
    }
    do_lock();

    if (g_http_dns_ip_tmp == ip_addr) {
        g_http_dns_ip_tmp = 0;
    }

    do_unlock();

    return true;
}

int tvs_http_dns_get_count()
{
    return 1;
}

int tvs_system_dns_get_count()
{
    return 1;
}

int tvs_http_dns_get_next_ip(int index)
{
    if (index != 0) {
        return 0;
    }

    do_lock();

    int ip = g_http_dns_ip_tmp;

    do_unlock();

    return ip;
}

bool tvs_system_dns_check_start()
{
    return true;
}


int tvs_system_dns_get_next_ip(int index)
{
    if (index != 0) {
        return 0;
    }

    do_lock();

    int ip = g_system_dns_ip_tmp;

    do_unlock();

    return ip;
}

bool tvs_system_dns_remove_ip(int ip_addr)
{
    if (ip_addr == 0) {
        return false;
    }
    do_lock();

    if (g_system_dns_ip_tmp == ip_addr) {
        g_system_dns_ip_tmp = 0;
    }

    do_unlock();

    return true;
}

static void dns_thread_func(tvs_thread_handle_t *thread_handle_t);

int tvs_dns_init()
{
    TVS_LOCKER_INIT

    if (g_tvs_dns_thread == NULL) {
        g_tvs_dns_thread = tvs_thread_new(dns_thread_func, NULL);
    }

    return 0;
}

void tvs_dns_get_first_ip()
{
    // 首次连接，获取DNS IP，不进行echo校验，直接设置为valid ip
    tvs_dns_fetch_all(NULL, NULL, NULL);

    do_lock();
    int ip = g_http_dns_ip == 0 ? g_system_dns_ip : g_http_dns_ip;
    do_unlock();

    if (ip != 0) {
        tvs_ip_provider_set_valid_ip(ip, true);
    } else {
        // 如果系统dns和http dns都没有，那么使用兜底IP
        tvs_ip_provider_set_valid_ip_ex(BACKUP_IP, true);
    }
}

static void dns_thread_func(tvs_thread_handle_t *thread_handle_t)
{
    tvs_dns_fetch_all(NULL, NULL, NULL);

    int validip = 0;

    do_lock();
    int http_ip = g_http_dns_ip;
    int system_ip = g_system_dns_ip;
    do_unlock();

    bool ret = tvs_echo_start(http_ip);

    if (!ret) {
        ret = tvs_echo_start(system_ip);
        if (ret) {
            validip = system_ip;
        }
    } else {
        validip = http_ip;
    }

    if (validip != 0) {
        tvs_ip_provider_set_valid_ip(validip, true);
    }
}

void tvs_dns_thread_start()
{
    if (!tvs_thread_is_stop(g_tvs_dns_thread)) {
        return;
    }

    tvs_thread_start_prepare(g_tvs_dns_thread, NULL, 0);

    tvs_thread_start_now(g_tvs_dns_thread, "dns", 2, 1024);
}

static void *g_tvs_dns_timer = NULL;
#define DNS_REFRESH_TIMER     40  // 更新DNS的时间间隔

void tvs_dns_timer_func(void *param)
{
    tvs_dns_thread_start();
}

void tvs_dns_refresh_start()
{
    if (!g_tvs_dns_timer) {
        os_wrapper_start_timer(&g_tvs_dns_timer, tvs_dns_timer_func, DNS_REFRESH_TIMER * 60 * 1000, true);
    }
}

void tvs_dns_refresh_stop()
{
    if (g_tvs_dns_timer) {
        TVS_LOG_PRINTF("stop ping refresh timer\n");
        os_wrapper_stop_timer(g_tvs_dns_timer);
        g_tvs_dns_timer = NULL;
    }

}

void tvs_dns_on_network_changed(bool connected)
{
    TVS_LOG_PRINTF("network changed,dns:%d\n", connected);
    if (connected) {
        // 网络重连的时候，需要刷新DNS IP
        tvs_dns_refresh_start();
    }
}

