#define TVS_LOG_DEBUG_MODULE  "ECHO"
#include "tvs_log.h"

#include "tvs_http_client.h"
#include "tvs_http_manager.h"
#include "tvs_config.h"
#include "tvs_executor_service.h"
#include "tvs_jsons.h"
#include "tvs_preference.h"
#include "tvs_ip_provider.h"
#include "mongoose.h"


static int *g_echo_error_ip_list = NULL;

#define MAX_ERROR_IP   15

static void print_error_echo_list()
{
    if (g_echo_error_ip_list == NULL) {
        TVS_LOG_PRINTF("echo error list is empty\n");
        return;
    }

    for (int i = 0; i < MAX_ERROR_IP; i++) {
        TVS_LOG_PRINTF("echo error list %d -- %s\n", i, inet_ntoa(*(struct in_addr *)&g_echo_error_ip_list[i]));
    }
}

void tvs_echo_test_print()
{
    print_error_echo_list();
}

void tvs_echo_test_clear()
{
    if (g_echo_error_ip_list != NULL) {
        TVS_FREE(g_echo_error_ip_list);
        g_echo_error_ip_list = NULL;
    }
}

// 测试用
void tvs_echo_test_add_error_ip(const char *ip_str)
{
    TVS_LOG_PRINTF("add echo error ip %s\n", ip_str == NULL ? "0.0.0.0" : ip_str);

    int ip = tvs_ip_provider_convert_ip_str(ip_str);

    if (ip == 0) {
        TVS_LOG_PRINTF("invalid ip\n");
        return;
    }

    if (g_echo_error_ip_list == NULL) {
        g_echo_error_ip_list = TVS_MALLOC(sizeof(int) * MAX_ERROR_IP);
        memset(g_echo_error_ip_list, 0, sizeof(int) * MAX_ERROR_IP);
    }

    for (int i = 0; i < MAX_ERROR_IP; i++) {
        if (g_echo_error_ip_list[i] == 0) {
            g_echo_error_ip_list[i] = ip;
            break;
        }
    }

    print_error_echo_list();
}

// 测试用
bool tvs_echo_is_in_error_list(int ip)
{
    if (g_echo_error_ip_list == NULL) {
        return false;
    }

    if (ip == 0) {
        return false;
    }

    for (int i = 0; i < MAX_ERROR_IP; i++) {
        if (g_echo_error_ip_list[i] == ip) {
            return true;
        }
    }

    return false;
}

// 测试用
void tvs_echo_test_remove_error_ip(const char *ip_str)
{
    TVS_LOG_PRINTF("del echo error ip %s\n", ip_str == NULL ? "0.0.0.0" : ip_str);

    if (g_echo_error_ip_list == NULL) {
        TVS_LOG_PRINTF("echo error list is empty\n");
        return;
    }

    int ip = tvs_ip_provider_convert_ip_str(ip_str);

    if (ip == 0) {
        TVS_LOG_PRINTF("invalid ip\n");
        return;
    }

    for (int i = 0; i < MAX_ERROR_IP; i++) {
        if (g_echo_error_ip_list[i] == ip) {
            g_echo_error_ip_list[i] = 0;
        }
    }

    print_error_echo_list();
}


typedef struct {
    char msg_str[20];
} tvs_echo_param;

bool check_echo_result(const char *response, int response_len, tvs_echo_param *param)
{
    cJSON *root = cJSON_Parse(response);
    if (NULL == root) {
        return false;
    }

    cJSON *echoMsg = cJSON_GetObjectItem(root, "echoMsg");
    if (NULL != echoMsg && NULL != echoMsg->valuestring && strcmp(echoMsg->valuestring, param->msg_str) == 0) {
        cJSON_Delete(root);
        return true;
    }

    cJSON_Delete(root);
    return false;
}


void tvs_echo_callback_on_connect(void *connection, int error_code, tvs_http_client_param *param)
{

}

void tvs_echo_callback_on_response(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param)
{
    if (ret_code == 200) {
        //TVS_LOG_PRINTF("get resonse %d - %.*s\n", ret_code, response_len, response);
        tvs_echo_param *echo_param = (tvs_echo_param *)param->user_data;
        if (!check_echo_result(response, response_len, echo_param)) {
            // echo失败
            param->resp_code = 0;
            TVS_LOG_PRINTF("echo failed\n");
            tvs_exception_report_start(EXCEPTION_ECHO, (char *)response, response_len);
        }
    } else {
        TVS_LOG_PRINTF("get resonse err %d - %.*s\n", ret_code, response_len, response);
        char ch_temp[40] = {0};
        snprintf(ch_temp, sizeof(ch_temp), "http return code error:%d", ret_code);
        tvs_exception_report_start(EXCEPTION_ECHO, ch_temp, strlen(ch_temp));
    }
}

void tvs_echo_callback_on_close(void *connection, int by_server, tvs_http_client_param *param) { }

int tvs_echo_callback_on_send_request(void *connection, const char *path, const char *host, const char *extra_header, const char *payload, tvs_http_client_param *param)
{
    tvs_http_send_normal_get_request((struct mg_connection *)connection, path, strlen(path), host, strlen(host));
    return 0;
}

bool tvs_echo_callback_on_loop(void *connection, tvs_http_client_param *param)
{
    return false;
}

bool tvs_echo_start(int ipaddr)
{
    if (ipaddr == 0) {
        return false;
    }

    if (tvs_echo_is_in_error_list(ipaddr)) {
        // 测试用，如果在error ip list中就强行判定为失效
        TVS_LOG_PRINTF("found in error iplist, echo failed\n");
        return false;
    }

    tvs_echo_param echo_param;
    memset(&echo_param, 0, sizeof(tvs_echo_param));

    int size = 120;
    char *url = malloc(size);

    memset(url, 0, size);

    int msg = os_wrapper_get_time_ms();

    sprintf(echo_param.msg_str, "%d", msg);

    sprintf(url, "%s%d&msg=%d", tvs_config_get_echo_url(), msg, msg);

    //TVS_LOG_PRINTF("echo ip %d, url %s\n", ipaddr, url);

    tvs_http_client_callback cb = { 0 };
    cb.cb_on_close = tvs_echo_callback_on_close;
    cb.cb_on_connect = tvs_echo_callback_on_connect;
    cb.cb_on_loop = tvs_echo_callback_on_loop;
    cb.cb_on_request = tvs_echo_callback_on_send_request;
    cb.cb_on_response = tvs_echo_callback_on_response;

    tvs_http_client_config config = { 0 };
    // echo的超时时间
    config.connection_timeout_sec = 7;
    config.total_timeout = 13;
    tvs_http_client_param http_param = { 0 };

    int ret = tvs_http_client_request_with_forceip(url, NULL, NULL, &echo_param, &config, &cb, &http_param, false, ipaddr);

    free(url);

    return ret == 200;
}


