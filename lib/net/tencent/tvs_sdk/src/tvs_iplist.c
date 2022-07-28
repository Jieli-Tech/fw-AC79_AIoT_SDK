
#define TVS_LOG_DEBUG_MODULE  "IPLIST"
#include "tvs_log.h"

#include "tvs_http_client.h"
#include "tvs_http_manager.h"
#include "tvs_config.h"
#include "tvs_executor_service.h"
#include "tvs_jsons.h"
#include "tvs_preference.h"
#include "tvs_threads.h"

#define HTTP_IPLIST_PATH  "https://aiwx.html5.qq.com/dns/lookup/freeRTOS1.0/"

typedef struct {
    int *ip_list;
    int ip_list_count;
} tvs_iplist_param;

#define MAX_IPLIST_IP_COUNT   10

#define TVS_NAME_IPLIST_TEST "iplist_t"
#define TVS_NAME_IPLIST      "iplist"


// IPList刷新时间间隔，单位为毫秒
#define IP_LIST_REFRESH_TIME    4 * 60 * 60 * 1000

// IPList失败重试时间间隔，单位为毫秒
#define IP_LIST_RETRY_TIME    1 * 60 * 60 * 1000


static int g_ipaddr_array[MAX_IPLIST_IP_COUNT] = {0};
static int g_ip_count = 0;

static void *g_timer = NULL;

static int g_last_read_suc_time = 0;

TVS_LOCKER_DEFINE

static void tvs_iplist_print_all()
{
    for (int i = 0; i < MAX_IPLIST_IP_COUNT && i < g_ip_count; i++) {
        TVS_LOG_PRINTF("iplist mem: index %d, ip %d\n", i, g_ipaddr_array[i]);
    }
}

bool tvs_iplist_check_start()
{
    return true;
}

bool tvs_iplist_check_stop()
{
    return true;
}

int tvs_iplist_get_count()
{
    do_lock();

    int count = g_ip_count;

    do_unlock();
    if (count > MAX_IPLIST_IP_COUNT) {
        count = MAX_IPLIST_IP_COUNT;
    }
    return count;
}

int tvs_iplist_get_next_ip(int index)
{
    do_lock();
    if (index >= MAX_IPLIST_IP_COUNT || index >= g_ip_count) {
        do_unlock();
        return 0;
    }

    int ip = g_ipaddr_array[index];

    do_unlock();

    return ip;
}

bool tvs_iplist_remove_ip(int ip_addr)
{
    if (ip_addr == 0) {
        return false;
    }
    do_lock();

    for (int i = 0; i < MAX_IPLIST_IP_COUNT && i < g_ip_count; i++) {
        if (ip_addr == g_ipaddr_array[i]) {
            g_ipaddr_array[i] = 0;
        }
    }

    do_unlock();

    return true;
}

bool tvs_iplist_fill(int *ip_addr, int count)
{
    if (ip_addr == NULL || count == 0) {
        return false;
    }
    do_lock();
    g_ip_count = count > MAX_IPLIST_IP_COUNT ? MAX_IPLIST_IP_COUNT : count;
    memcpy(g_ipaddr_array, ip_addr, sizeof(int) * g_ip_count);
    tvs_iplist_print_all();
    do_unlock();

    return true;
}

void load_iplist_from_mem()
{
    // load iplist form memory
    const char *node_name = NULL;
    tvs_api_env env = tvs_config_get_current_env();
    if (env == TVS_API_ENV_TEST) {
        node_name = TVS_NAME_IPLIST_TEST;
    } else {
        node_name = TVS_NAME_IPLIST;
    }

    int *iplist_mem = NULL;
    int arr_size = tvs_preference_get_int_array_value(node_name, &iplist_mem);

    tvs_iplist_fill(iplist_mem, arr_size);
    if (iplist_mem != NULL) {
        free(iplist_mem);
    }
}

void tvs_iplist_on_network_changed(bool connected)
{
    if (!connected) {
        return;
    }
}

static bool save_ip_list_to_preference(int *iplist, int size)
{
    if (iplist == NULL || size == 0) {
        return true;
    }

    const char *node_name = NULL;
    tvs_api_env env = tvs_config_get_current_env();
    if (env == TVS_API_ENV_TEST) {
        node_name = TVS_NAME_IPLIST_TEST;
    } else {
        node_name = TVS_NAME_IPLIST;
    }

    int *iplist_mem = NULL;
    int arr_size = tvs_preference_get_int_array_value(node_name, &iplist_mem);
    bool diff = false;

    if (arr_size == size) {
        for (int i = 0; i < size; i++) {
            bool found = false;
            for (int j = 0; j < size; j++) {
                if (iplist[i] == iplist_mem[j]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                diff = true;
                break;
            }
        }
    } else {
        diff = true;
    }

    if (iplist_mem != NULL) {
        free(iplist_mem);
    }

    if (diff) {
        tvs_preference_set_int_array_value(node_name, iplist, size);
        tvs_iplist_fill(iplist, size);
    } else {
        TVS_LOG_PRINTF("iplist not change\n");
    }

    return diff;
}

static int do_on_fetch_iplist(const char *resp_body, tvs_iplist_param *param)
{
    cJSON *root = NULL;
    cJSON *dnsList = NULL;
    int size = 0;
    cJSON *child = NULL;
    cJSON *ip = NULL;
    cJSON *port = NULL;
    cJSON *ispName = NULL;
    do {
        root = cJSON_Parse(resp_body);
        if (root == NULL) {
            break;
        }

        dnsList = cJSON_GetObjectItem(root, "dnsList");

        if (dnsList == NULL) {
            break;
        }

        size = cJSON_GetArraySize(dnsList);

        if (size <= 0) {
            break;
        }

        param->ip_list = malloc(sizeof(int) * size);
        if (NULL == param->ip_list) {
            break;
        }

        memset(param->ip_list, 0, sizeof(int) * size);

        for (int i = 0; i < size; i++) {
            child = cJSON_GetArrayItem(dnsList, i);
            if (child == NULL) {
                continue;
            }

            ispName = cJSON_GetObjectItem(child, "ispName");
            if (ispName != NULL && ispName->valuestring != NULL && strstr(ispName->valuestring, "CAP") != NULL) {
                //continue;
            }

            port = cJSON_GetObjectItem(child, "port");
            if (port != NULL && port->valueint != 443) {
                continue;
            }

            ip = cJSON_GetObjectItem(child, "ip");

            if (ip == NULL || ip->valuestring == NULL) {
                continue;
            }

            unsigned int a, b, c, d = 0;
            if (sscanf(ip->valuestring, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {

                param->ip_list[param->ip_list_count] =
                    tvs_htonl(((uint32_t) a << 24) | ((uint32_t) b << 16) | c << 8 | d);

                TVS_LOG_PRINTF("iplist %d - %s - %d\n", param->ip_list_count, ip->valuestring, param->ip_list[param->ip_list_count]);
                param->ip_list_count++;
            }


        }
    } while (0);

    if (root != NULL) {
        cJSON_Delete(root);
    }

    return param->ip_list_count <= 0 ? -1 : 0;
}


void tvs_iplist_callback_on_connect(void *connection, int error_code, tvs_http_client_param *param)
{
    //TVS_LOG_PRINTF("send iplist connect\n");
}

void tvs_iplist_callback_on_response(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param)
{
    if (ret_code == 200) {
        //TVS_LOG_PRINTF("get resonse %d - %.*s\n", ret_code, response_len, response);

        tvs_iplist_param *iplist_param = (tvs_iplist_param *)param->user_data;
        int ret = do_on_fetch_iplist(response, iplist_param);
        if (ret != 0) {
            TVS_LOG_PRINTF("fetch iplist err %d - %.*s\n", ret, response_len, response);
            param->resp_code = 0;
        }

        if (iplist_param != NULL) {
            save_ip_list_to_preference(iplist_param->ip_list, iplist_param->ip_list_count);
        }

    } else {
        TVS_LOG_PRINTF("get resonse err %d - %.*s\n", ret_code, response_len, response);
    }
}

void tvs_iplist_callback_on_close(void *connection, int by_server, tvs_http_client_param *param) { }

int tvs_iplist_callback_on_send_request(void *connection, const char *path, const char *host, const char *extra_header, const char *payload, tvs_http_client_param *param)
{
    tvs_http_send_normal_get_request((struct mg_connection *)connection, path, strlen(path), host, strlen(host));
    return 0;
}

bool tvs_iplist_callback_on_loop(void *connection, tvs_http_client_param *param)
{
    return false;
}

void tvs_iplist_clear()
{
    //TVS_LOG_PRINTF("tvs iplist clear timer\n");
    os_wrapper_stop_timer(g_timer);
    g_timer = NULL;
}

int tvs_iplist_get_time(bool last_success)
{
    int timer = 0;
    if (last_success) {
        timer = IP_LIST_REFRESH_TIME;
    } else {
        timer = IP_LIST_RETRY_TIME;
    }

    return timer;
}

void tvs_iplist_reset_timer(int time, void *func)
{
    os_wrapper_start_timer(&g_timer, func, time, false);
}

bool tvs_iplist_is_timeout()
{
    if (g_last_read_suc_time == 0 || os_wrapper_get_time_ms() - g_last_read_suc_time > IP_LIST_REFRESH_TIME) {
        return true;
    }

    return false;
}

int tvs_iplist_start(tvs_http_client_callback_exit_loop should_exit_func,
                     tvs_http_client_callback_should_cancel should_cancel_func,
                     void *exit_param, bool *force_break)
{

    char url[80] = {0};
    int target = 0;
    tvs_api_env env = tvs_config_get_current_env();
    switch (env) {
    case TVS_API_ENV_TEST:
        target = 8;
        break;
    case TVS_API_ENV_NORMAL:
        target = 4;
        break;
    default:
        return -1;
    }

    sprintf(url, "%sfreeRTOS_%d/%d", HTTP_IPLIST_PATH, (int)os_wrapper_get_time_ms(), target);

    tvs_http_client_callback cb = { 0 };
    cb.cb_on_close = tvs_iplist_callback_on_close;
    cb.cb_on_connect = tvs_iplist_callback_on_connect;
    cb.cb_on_loop = tvs_iplist_callback_on_loop;
    cb.cb_on_request = tvs_iplist_callback_on_send_request;
    cb.cb_on_response = tvs_iplist_callback_on_response;
    cb.exit_loop = should_exit_func;
    cb.should_cancel = should_cancel_func;
    cb.exit_loop_param = exit_param;

    tvs_http_client_config config = { 0 };
    tvs_http_client_param http_param = { 0 };
    tvs_iplist_param iplist_param = { 0 };

    int ret = tvs_http_client_request_with_forceip(url, NULL, NULL, &iplist_param, &config, &cb, &http_param, false, 0);

    if (force_break != NULL) {
        *force_break = http_param.force_break;
    }

    if (ret == 0) {
        // 记录上一次读取成功的时间
        g_last_read_suc_time = os_wrapper_get_time_ms();
    } else {
        g_last_read_suc_time = 0;
    }

    return ret;
}

int tvs_iplist_init()
{
    TVS_LOCKER_INIT
    return 0;
}
