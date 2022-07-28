/**
* @file  tvs_authorizer.c
* @brief TVS SDK授权逻辑
* @date  2019-5-10
*/

#include "tvs_http_client.h"
#include "tvs_http_manager.h"
#include "tvs_authorizer.h"
#include "tvs_authorize_infos.h"
#include "tvs_config.h"
#define TVS_LOG_DEBUG_MODULE  "AUTH"
#include "tvs_log.h"
#include "tvs_jsons.h"
#include "tvs_down_channel.h"
#include "tvs_core.h"
#include "tvs_executor_service.h"
#include "tvs_threads.h"
#include "tvs_ip_provider.h"
#include "tvs_exception_report.h"
#include "data_template_tvs_auth.h"

typedef struct {
    int index;
    char *client_id;
    char *authorization;
    char *refresh_token;
    int expires_in;
} tvs_authorizer_param;

#define TVS_MAGIC_NUMBER   "0001"
#define TVS_ENCRYPT        "ENCRYPT:0001"
#define TVS_MD5            "MD5"

static char *g_current_client_id = NULL;

TVS_LOCKER_DEFINE

static long g_last_auth_success_time = 0;
static char *g_tvs_authorization = NULL;
static char *g_refresh_token = NULL;
static tvs_authorize_callback g_auth_callback = NULL;
static int g_time_expires = 0;
static void *g_auth_timer = NULL;
static bool g_first_boot = true;
static bool g_auth_is_manuf = false;

// client id授权请求
#define TVS_ACCOUNT_AUTH_REQUEST  \
		"{" \
			"\"grant_type\":\"authorization_code\"," \
			"\"code\":\"authCode\"," \
			"\"redirect_uri\":\"redirectUri\"," \
			"\"client_id\":\"\"," \
			"\"code_verify\":\"6nfkrz2JpTokh9pj8YShls5uqdoSAoCPTiwsymiYW0Q\"" \
		"}"

// 厂商账号授权请求
#define TVS_ACCOUNT_AUTH_MANUF_REQUEST  \
		"{" \
			"\"grant_type\":\"authorization_code\"," \
			"\"code\":\"authCode\"," \
			"\"redirect_uri\":\"redirectUri\"," \
			"\"client_id\":\"\"," \
			"\"code_verifier\":\"6nfkrz2JpTokh9pj8YShls5uqdoSAoCPTiwsymiYW0Q\"" \
		"}"


// 刷票请求
#define TVS_ACCOUNT_REFRESH_REQUEST  \
		"{" \
			"\"grant_type\":\"refresh_token\"," \
			"\"client_id\":\"\"," \
			"\"refresh_token\":\"\"" \
		"}"

//#define TEST_AUTHORIZATION   "Bearer freerots_xxxxxxxxxxxxx20190307"

// 重试的次数和时间
static int g_retry_next_time_array[] = {5, 5, 5, 5, 300};
static unsigned int g_retry_index = 0;
static char *g_tvs_product_id = NULL;
static char *g_tvs_dsn = NULL;

static void to_upper(char *target)
{
    if (NULL == target) {
        return;
    }

    for (int i = 0; i < strlen(target); i++) {

        if (target[i] >= 'a' && target[i] <= 'z') {
            target[i] -= 32;
        }
    }
}

static void tvs_authorizer_clear_not_lock(bool clear_authorization)
{
    TVS_LOG_PRINTF("clear last auth info\n");
    if (g_refresh_token != NULL) {
        TVS_FREE(g_refresh_token);
        g_refresh_token = NULL;
    }

    if (clear_authorization && g_tvs_authorization != NULL) {
        TVS_FREE(g_tvs_authorization);
        g_tvs_authorization = NULL;
    }
    //云小微工程师添加该函数
    tvs_down_channel_break();
}

static void auth_timer_func(void *param)
{
    g_last_auth_success_time = 0;

    tvs_authorizer_start_inner();
}

static void start_timer(int timer_sec)
{
    os_wrapper_start_timer(&g_auth_timer, auth_timer_func, timer_sec * 1000, false);
}

static void stop_timer()
{
    os_wrapper_stop_timer(g_auth_timer);
    g_auth_timer = 0;
}

// 设置定时器，失败重试或者刷票
static int start_auth_retry(bool success, int time_expires)
{
    int timer_sec = 0;

    if (!success) {
        timer_sec = g_retry_next_time_array[g_retry_index];
        g_retry_index++;
        int size = sizeof(g_retry_next_time_array) / sizeof(int);
        if (g_retry_index >= size) {
            g_retry_index = size - 1;
        }
    } else {
        g_retry_index = 0;
        timer_sec = time_expires;
    }

    TVS_LOG_PRINTF("start authorize in %d seconds\n", timer_sec);
    start_timer(timer_sec);
    return 0;
}

void tvs_authorizer_set_current_client_id(char *client_id)
{
    if (client_id == NULL) {
        return;
    }
    //TVS_LOG_PRINTF("start set client id %s\n", client_id);
    do_lock();
    if (g_current_client_id != NULL) {
        bool same = strcmp(g_current_client_id, client_id) == 0;
        if (!same) {
            TVS_LOG_PRINTF("client id has changed\n");
            // client id改变，需要清除账号信息
            tvs_authorizer_clear_not_lock(true);
        }
    }
    if (g_current_client_id != NULL) {
        TVS_FREE(g_current_client_id);
    }

    g_current_client_id = client_id == NULL ? NULL : strdup(client_id);
    TVS_LOG_PRINTF("set current client id %s\n", g_current_client_id == NULL ? "" : g_current_client_id);
    do_unlock();

    tvs_executor_cancel_authorize();
}

int tvs_authorize_build_auth_req(char *authReqInfo, int len)
{
    return tvs_auth_req_infos_build(authReqInfo, len);
}


int tvs_authorizer_set_manuf_current_client_id(const char *client_id, const char *auth_resp_info)
{
    if (client_id == NULL) {
        TVS_LOG_PRINTF("%s call error,client id is NULL!!!", __func__);
        return -1;
    }
    cJSON *root = cJSON_Parse(auth_resp_info);
    if (root == NULL) {
        TVS_LOG_PRINTF("%s call error,auth resp info not json!!!", __func__);
        return -1;
    }
    cJSON *authCode = cJSON_GetObjectItem(root, "authCode");
    cJSON *sessionId = cJSON_GetObjectItem(root, "sessionId");
    if (authCode == NULL || sessionId == NULL) {
        TVS_LOG_PRINTF("%s call error,auth resp info not found authCode or sessionId!!!", __func__);
        return -1;
    }

    TVS_LOG_PRINTF("Start manuf authorize:\n%s\n%s\n", auth_resp_info, client_id);

    char *build_session_id = tvs_auth_get_session_id();

    if (build_session_id != NULL && sessionId->valuestring != NULL && authCode->valuestring != NULL &&
        strncmp(build_session_id, sessionId->valuestring, strlen(build_session_id)) == 0) {
        //是否厂商账号授权
        g_auth_is_manuf = true;
        tvs_auth_set_authCode(authCode->valuestring, strlen(authCode->valuestring));
        tvs_authorizer_set_current_client_id((char *)client_id);		//yii:设置clientID跟取消之前的授权
        return 0;
    } else {
        TVS_LOG_PRINTF("%s call error,auth resp info format error:%s!!!", __func__, auth_resp_info);
        return -1;
    }
}


// 返回true代表和当前client_id相等
bool tvs_authorizer_check_client_id(char *client_id)
{
    do_lock();

    bool same = strcmp(g_current_client_id, client_id) == 0;

    do_unlock();

    return same;
}

static bool is_str_empty(char *str)
{
    return (str == NULL || str[0] == 0);
}

bool cJSON_Impl_HasObjectItem(const cJSON *object, const char *string)
{
    return cJSON_GetObjectItem(object, string) ? 1 : 0;
}

static char *get_request_json(const char *client_id, const char *refresh_token)
{
    if (is_str_empty((char *)client_id)) {
        TVS_LOG_PRINTF("client info invalid\n");
        return NULL;
    }

    bool do_auth = false;

    if (is_str_empty((char *)refresh_token)) {
        do_auth = true;
    }

    cJSON *root = NULL;
    char *request = NULL;

    TVS_LOG_PRINTF("start %s\n", do_auth ? "authorize" : "refresh authorize");

    do {
        char *json = do_auth ? (g_auth_is_manuf ? TVS_ACCOUNT_AUTH_MANUF_REQUEST : TVS_ACCOUNT_AUTH_REQUEST) : TVS_ACCOUNT_REFRESH_REQUEST;

        root = cJSON_Parse(json);

        if (NULL == root) {
            TVS_LOG_PRINTF("parse root error\n");
            break;
        }

        cJSON_ReplaceItemInObject(root, "client_id", cJSON_CreateString(client_id));

        if (cJSON_Impl_HasObjectItem(root, "refresh_token")) {
            // 刷票需要设置refresh_token
            cJSON_ReplaceItemInObject(root, "refresh_token", cJSON_CreateString(refresh_token));
        }

        if (do_auth && g_auth_is_manuf) {
            if (tvs_auth_get_codeVerifier() == NULL || tvs_auth_get_authCode() == NULL) {
                TVS_LOG_PRINTF("Read codeVerifier or authCode error!!!\n");
            } else {
                TVS_LOG_PRINTF("set authinfo,code:%s,code_verify:%s\n", tvs_auth_get_authCode(), tvs_auth_get_codeVerifier());
                cJSON_ReplaceItemInObject(root, "code", cJSON_CreateString(tvs_auth_get_authCode()));
                cJSON_ReplaceItemInObject(root, "code_verifier", cJSON_CreateString(tvs_auth_get_codeVerifier()));
            }
        }

        request = cJSON_PrintUnformatted(root);

        if (NULL == request) {
            TVS_LOG_PRINTF("parse request error\n");
            break;
        }

        TVS_LOG_PRINTF("reqeust: %s\n", request);

    } while (0);

    if (root != NULL) {
        cJSON_Delete(root);
    }

    return request;
}

static void do_callback(bool success, int error, char *client_id, char *refresh_token)
{
    char *auth_info = NULL;

    if (success) {
        // 组装auth info，通知外层
        cJSON *json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "refresh_token", cJSON_CreateString(refresh_token));
        cJSON_AddItemToObject(json, "client_id", cJSON_CreateString(client_id));

        int cur_env = tvs_config_get_current_env();

        cJSON_AddItemToObject(json, "env", cJSON_CreateNumber((int)cur_env));
        auth_info = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
        TVS_LOG_PRINTF("authorizer success - %s\n", auth_info);

        if (g_auth_callback != NULL) {
            g_auth_callback(success, auth_info, strlen(auth_info) + 1, client_id, error);
        } else {
            TVS_LOG_PRINTF("authorizer callback is NULL\n");
        }
    } else {

        TVS_LOG_PRINTF("authorizer failed\n");
        if (g_auth_callback != NULL) {
            g_auth_callback(success, NULL, 0, client_id, error);
        } else {
            TVS_LOG_PRINTF("authorizer callback is NULL\n");
        }
    }

    if (auth_info) {
        TVS_FREE(auth_info);
    }
}

static void on_auth_complete(bool success, tvs_authorizer_param *param)
{
    if (!tvs_authorizer_check_client_id(param->client_id)) {
        return;
    }

    if (success) {
        do_lock();
        // 授权成功，保存结果到RAM中
        if (g_tvs_authorization != NULL) {
            TVS_FREE(g_tvs_authorization);
        }

        g_tvs_authorization = strdup(param->authorization);

        if (g_refresh_token != NULL) {
            TVS_FREE(g_refresh_token);
        }

        // 保存refresh token,用于刷票
        g_refresh_token = strdup(param->refresh_token);
        g_last_auth_success_time = os_wrapper_get_time_ms();

        g_time_expires = param->expires_in;
        do_unlock();

        // 通知上层
        /* IOT_Tvs_Auth_Error_Cb(true, 0); */
        do_callback(true, 0, param->client_id, param->refresh_token);
    } else {
        // 授权失败的情况
        /* IOT_Tvs_Auth_Error_Cb(false, -1); */
        do_callback(false, -1, param->client_id, NULL);
    }

    start_auth_retry(success, param->expires_in);

    // 通知下行通道，授权成功，可以开始保持长连接
    tvs_down_channel_notify();
}

static int do_process_auth_response(const char *response, int len, tvs_authorizer_param *param)
{
    cJSON *root = NULL;

    int ret = -1;

    do {
        root = cJSON_Parse(response);

        if (root == NULL) {
            TVS_LOG_PRINTF("invalid response: %.*s\n", len, response);
            break;
        }

        TVS_LOG_PRINTF("Auth response:%.*s\n", len, response);

        cJSON *access_token = cJSON_GetObjectItem(root, "access_token");
        cJSON *refresh_token = cJSON_GetObjectItem(root, "refresh_token");
        cJSON *token_type = cJSON_GetObjectItem(root, "token_type");
        cJSON *expires_in = cJSON_GetObjectItem(root, "expires_in");

        if (access_token == NULL || is_str_empty(access_token->valuestring)) {
            TVS_LOG_PRINTF("invalid access_token\n");
            break;
        }
        if (refresh_token == NULL || is_str_empty(refresh_token->valuestring)) {
            TVS_LOG_PRINTF("invalid refresh_token\n");
            break;
        }
        if (token_type == NULL || is_str_empty(token_type->valuestring)) {
            TVS_LOG_PRINTF("invalid token_type\n");
            break;
        }

        if (expires_in == NULL || expires_in->valueint == 0) {
            TVS_LOG_PRINTF("invalid expires_in\n");
            break;
        }

        param->refresh_token = strdup(refresh_token->valuestring);

        // 过期时间，需要设置定时器，在过期之前进行刷票
        param->expires_in = expires_in->valueint;

        param->expires_in -= 800;
        if (param->expires_in < 500) {
            param->expires_in = 500;
        }

        int size = strlen(token_type->valuestring) + strlen(access_token->valuestring) + 30;

        // 组装authorization,之后每次speech/control都会在http头中携带
        param->authorization = TVS_MALLOC(size);
        if (param->authorization != NULL) {
            memset(param->authorization, 0, size);
            sprintf(param->authorization, "%s %s", token_type->valuestring, access_token->valuestring);
        }

        /*TVS_LOG_PRINTF("auth result: authorization %s, refresh_token %s, expires_in %d, client id %s\n",
        	param->authorization,
        	param->refresh_token,
        	param->expires_in,
        	param->client_id);*/

        ret = 0;
    } while (0);

    if (root != NULL) {
        cJSON_Delete(root);
    }

    if (ret) {
        //服务器返回错误，上报异常
        tvs_exception_report_start(EXCEPTION_AUTH, (char *)response, len);
    }

    return ret;
}


void tvs_authorizer_callback_on_connect(void *connection, int error_code, tvs_http_client_param *param) { }

void tvs_authorizer_callback_on_response(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param)
{
    int ret = -1;
    tvs_authorizer_param *auth_param = (tvs_authorizer_param *)param->user_data;

    if (ret_code == 200) {
        // 授权/刷票，后台返回了200，解析response
        ret = do_process_auth_response(response, response_len, auth_param);
    } else {
        TVS_LOG_PRINTF("get resp error %d - %.*s\n", ret_code, response_len, response);
        char ch_temp[40] = {0};
        snprintf(ch_temp, sizeof(ch_temp), "http return code error:%d", ret_code);
        tvs_exception_report_start(EXCEPTION_AUTH, ch_temp, strlen(ch_temp));
    }

    if (ret != 0) {
        param->resp_code = -1;
    }
}

void tvs_authorizer_callback_on_close(void *connection, int by_server, tvs_http_client_param *param) { }

int tvs_authorizer_callback_on_send_request(void *connection, const char *path, const char *host, const char *extra_header, const char *payload, tvs_http_client_param *param)
{
    // 发送http请求
    tvs_http_send_normal_post((struct mg_connection *)connection, path, strlen(path), host, strlen(host), payload);
    return 0;
}

bool tvs_authorizer_callback_on_loop(void *connection, tvs_http_client_param *param)
{
    return false;
}

void tvs_authorizer_callback_on_loop_end(tvs_http_client_param *param)
{
    tvs_authorizer_param *auth_param = (tvs_authorizer_param *)param->user_data;

    on_auth_complete(param->resp_code == 200, auth_param);

    if (auth_param->authorization != NULL) {
        TVS_FREE(auth_param->authorization);
    }

    if (auth_param->refresh_token != NULL) {
        TVS_FREE(auth_param->refresh_token);
    }
}

char *tvs_authorizer_get_request_body(const char *client_id, const char *refresh_token)
{
    // 根据是否有refresh token，决定是授权还是刷票
    return get_request_json(client_id, refresh_token);
}

extern char *cs_md5(char buf[33], ...);

// 在访客登陆的情况下，利用procuct id和DSN生成client id
char *tvs_authorizer_generate_client_id(const char *tvs_product_id, const char *tvs_dsn)
{
    if (NULL == tvs_product_id || NULL == tvs_dsn) {
        return NULL;
    }

    char md5[33] = { 0 };

    cs_md5(md5, tvs_product_id, strlen(tvs_product_id), tvs_dsn, strlen(tvs_dsn), TVS_MAGIC_NUMBER, strlen(TVS_MAGIC_NUMBER), NULL);

    to_upper(md5);

    cs_md5(md5, md5, strlen(md5), TVS_MD5, strlen(TVS_MD5), NULL);

    to_upper(md5);

    int total_buf_len = strlen(TVS_ENCRYPT) + strlen(md5) + strlen(tvs_product_id) + strlen(tvs_dsn) + 20;

    char *client_id = TVS_MALLOC(total_buf_len);
    if (NULL == client_id) {
        return NULL;
    }

    memset(client_id, 0, total_buf_len);

    sprintf(client_id, "%s,%s,%s,%s", TVS_ENCRYPT, md5, tvs_product_id, tvs_dsn);

    //非厂商账号登录
    g_auth_is_manuf = false;

    TVS_LOG_PRINTF("generate product client id %s\n", client_id);
    return client_id;
}

int tvs_authorizer_init()
{

    TVS_LOCKER_INIT

    return 0;
}

// 开始授权/刷票
int tvs_authorizer_manager_start(const char *client_id, const char *refresh_token,
                                 tvs_http_client_callback_exit_loop should_exit_func,
                                 tvs_http_client_callback_should_cancel should_cancel,
                                 void *exit_param)
{
    if (is_str_empty((char *)client_id)) {
        TVS_LOG_PRINTF("client info invalid\n");
        return -1;
    }

    if (!tvs_authorizer_check_client_id((char *)client_id)) {
        TVS_LOG_PRINTF("client id %s is out of date\n", client_id);
        /* IOT_Tvs_Auth_Error_Cb(false, -2); */
        return -1;
    }

    if (g_first_boot) {
        g_first_boot = false;
        // 开机第一次授权/刷票的时候，进行DNS并设置ip provider
        tvs_ip_provider_get_first_ip();
    }

    //回调函数初始化
    tvs_http_client_callback cb = { 0 };
    cb.cb_on_close = tvs_authorizer_callback_on_close;
    cb.cb_on_connect = tvs_authorizer_callback_on_connect;
    cb.cb_on_loop = tvs_authorizer_callback_on_loop;
    cb.cb_on_request = tvs_authorizer_callback_on_send_request;
    cb.cb_on_response = tvs_authorizer_callback_on_response;
    cb.cb_on_loop_end = tvs_authorizer_callback_on_loop_end;
    cb.exit_loop = should_exit_func;
    cb.should_cancel = should_cancel;
    cb.exit_loop_param = exit_param;

    tvs_http_client_config config = { 0 };
    tvs_http_client_param http_param = { 0 };
    tvs_authorizer_param auth_param = { 0 };

    // client id和refresh_token，外面会释放，不要free它们
    auth_param.client_id = (char *)client_id;

    char *url = tvs_config_get_auth_url();
    TVS_LOG_PRINTF("start authorize, url %s\n", url);
    char *body = tvs_authorizer_get_request_body(client_id, refresh_token);		//yii:拼接好了要发送授权/刷票的buf

    if (NULL == body) {
        return -1;
    }

    stop_timer();

    // 发起授权/刷票请求
    int ret = tvs_http_client_request(url, NULL, body, &auth_param, &config, &cb, &http_param, true);

    if (body != NULL) {
        TVS_FREE(body);
    }

    return ret;
}

char *tvs_authorizer_get_authtoken()
{
    do_lock();
    //char* token = g_tvs_authorization != NULL ? TEST_AUTHORIZATION : "";
    char *token = g_tvs_authorization != NULL ? g_tvs_authorization : "";
    do_unlock();

    return token;
}

bool tvs_authorizer_is_timeout()
{
    do_lock();
    // 未授权或者超时的情况下需要重试
    bool invalid = is_str_empty(g_tvs_authorization)
                   || (os_wrapper_get_time_ms() - g_last_auth_success_time > g_time_expires * 1000);
    do_unlock();

    return invalid;
}

bool tvs_authorizer_is_authorized()
{
    do_lock();
    // authorization为空，代表未授权
    bool valid = !is_str_empty(g_tvs_authorization);
    do_unlock();
    return valid;
}

// 从外部传入的之前的authorize_info中解析出refresh token等信息，用于开机初始化阶段
static int load_auth_info(const char *authorize_info, int len)
{
    cJSON *root = NULL;

    int ret = -1;
    if (!authorize_info || !len) {
        return ret;
    }
    do {
        cJSON *root =  cJSON_Parse(authorize_info);

        if (root == NULL) {
            TVS_LOG_PRINTF("invalid auth info: %.*s\n", len, authorize_info);
            break;
        }

        cJSON *refresh_token = cJSON_GetObjectItem(root, "refresh_token");
        if (refresh_token == NULL || is_str_empty(refresh_token->valuestring)) {
            TVS_LOG_PRINTF("invalid refresh_token\n");
            break;
        }

        tvs_authorizer_clear_not_lock(false);
        g_refresh_token = strdup(refresh_token->valuestring);

        //TVS_LOG_PRINTF("load preference refresh_token %s\n", g_refresh_token);

        int last_auth_env = -1;
        cJSON *env = cJSON_GetObjectItem(root, "env");
        if (env != NULL) {
            last_auth_env = env->valueint;
        }

        int current_env = tvs_config_get_current_env();

        // 如果之前的auth info对应的环境（体验/测试/正式）与当前不符合，需要丢弃掉
        if (current_env != last_auth_env) {
            TVS_LOG_PRINTF("load preference env has changed, last %d, cur %d\n", last_auth_env, current_env);
            tvs_authorizer_clear_not_lock(true);
            break;
        }

        cJSON *client_id = cJSON_GetObjectItem(root, "client_id");

        if (client_id == NULL || is_str_empty(client_id->valuestring)) {
            TVS_LOG_PRINTF("load preference invalid client id\n");
            tvs_authorizer_clear_not_lock(true);
            break;
        }

        // 解析之前保存的client id
        tvs_authorizer_set_current_client_id(client_id->valuestring);

        ret = 0;
    } while (0);

    if (root != NULL) {
        cJSON_Delete(root);
    }

    if (authorize_info) {
        free((void *)authorize_info);
    }

    return ret;
}

int tvs_authorizer_load_auth_info(const char *authorize_info, int len)
{
    return load_auth_info(authorize_info, len);
}

char *tvs_authorizer_dup_refresh_token()
{
    do_lock();
    char *ret = g_refresh_token == NULL ? NULL : strdup(g_refresh_token);
    do_unlock();

    return ret;
}

char *tvs_authorizer_dup_current_client_id()
{
    do_lock();
    char *ret = g_current_client_id == NULL ? NULL : strdup(g_current_client_id);
    do_unlock();

    return ret;
}


int tvs_authorizer_start_inner()
{
    // 取消当前正在进行的auth操作，如果有的话，为了避免重复授权
    tvs_executor_cancel_authorize();
    // 开始执行auth
    int ret = tvs_executor_start_authorize();
    return ret;
}

int tvs_core_authorize_set_callback(tvs_authorize_callback auth_callback)
{
    g_auth_callback = auth_callback;
    return 0;
}

int tvs_core_authorize_initalize(const char *product_id, const char *dsn, const char *authorize_info, int authorize_info_size, tvs_authorize_callback auth_callback)
{
    g_tvs_product_id = product_id == NULL ? NULL : strdup(product_id);
    g_tvs_dsn = dsn == NULL ? NULL : strdup(dsn);
    g_auth_callback = auth_callback;
    return load_auth_info(authorize_info, authorize_info_size);
}

// 访客登陆
int tvs_core_authorize_guest_login()
{
    //是否厂商账号授权
    g_auth_is_manuf = false;
    char *client_id = tvs_authorizer_generate_client_id(g_tvs_product_id, g_tvs_dsn);
    tvs_authorizer_set_current_client_id(client_id);

    if (client_id != NULL) {
        TVS_FREE(client_id);
    }

    return tvs_authorizer_start_inner();
}

int tvs_core_authorize_set_client_id(const char *client_id)
{
    //是否厂商账号授权
    g_auth_is_manuf = false;
    tvs_authorizer_set_current_client_id((char *)client_id);
    return 0;
}

int tvs_core_authorize_login()
{

    return tvs_authorizer_start_inner();
}

int tvs_core_authorize_logout()
{
    do_lock();
    // 清除授权信息
    tvs_authorizer_clear_not_lock(true);
    do_unlock();
    return 0;
}


