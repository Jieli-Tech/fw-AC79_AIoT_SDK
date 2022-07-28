/**
* @file  tvs_down_channel.c
* @brief TVS SDK下行通道实现
* @date  2019-5-10
* 下行通道用于接收STOP_CAPTURE指令、ASR结果等，并用于接收云端的推送数据
*/

#include "tvs_http_client.h"
#include "tvs_http_manager.h"
#include "tvs_config.h"
#include "tvs_jsons.h"
#include "tvs_executor_service.h"
#include "tvs_multipart_handler.h"
#include "tvs_authorizer.h"
#include "tvs_config.h"
#include "tvs_threads.h"


#define TVS_LOG_DEBUG_MODULE  "DOWNCHANNEL"
#include "tvs_log.h"

#define HEART_TICK_BENDATA_LEN   5

static char g_bin_data[HEART_TICK_BENDATA_LEN] = {0};

typedef struct {
    void *multipart_info;
    tvs_multipart_callback multipart_callback;
    long last_time;
    tvs_multi_param multi_param;
} tvs_down_channel_param;


TVS_LOCKER_DEFINE

// 用于阻塞等待建立下行通道的条件满足
static void *g_waiter_mutex = NULL;

static bool g_down_channel_connected = false;

static bool g_break = false;

extern bool tvs_is_executor_valid(int cmd, bool check_for_auth);

bool tvs_down_channel_can_connect()
{
    if (tvs_is_executor_valid(TVS_EXECUTOR_CMD_DOWN_CHANNEL, false)) {
        return true;
    }

    return false;
}

bool tvs_down_channel_is_connect()
{
    do_lock();
    bool conn = g_down_channel_connected;
    do_unlock();

    return conn;
}

void tvs_down_channel_set_connect(bool conn)
{
    //TVS_LOG_PRINTF("%s, conn %d\n", __func__, conn);

    do_lock();
    g_down_channel_connected = conn;
    do_unlock();
}

void tvs_down_channel_set_break(bool do_break)
{
    do_lock();

    g_break = do_break;

    do_unlock();
}

bool tvs_down_channel_should_break()
{
    do_lock();

    bool do_break = g_break;

    do_unlock();

    return do_break;
}

void tvs_down_channel_break()
{
    tvs_down_channel_set_break(true);
}

void tvs_down_channel_notify()
{
    os_wrapper_post_signal(g_waiter_mutex);
}

void tvs_down_channel_wait(int time_ms)
{
    os_wrapper_wait_signal(g_waiter_mutex, time_ms);
}

extern int tvs_down_channel_start_connect();

void tvs_down_channel_thread(void *param)
{
    int fail_count = 0;
    int wait_long = 120;
    while (1) {
        if (!tvs_down_channel_can_connect()) {
            fail_count = 0;
            // 建立下行通道的前提条件不满足，则等待
            tvs_down_channel_wait(300 * 1000);
            continue;
        }

        TVS_LOG_PRINTF("start down channel\n");

        if (tvs_down_channel_start_connect() == 200) {
            fail_count = 0;
        } else {
            fail_count++;
            // 失败后，等待n秒再试
            tvs_down_channel_wait(3000);
        }

        tvs_down_channel_set_connect(false);

        if (fail_count == 6) {
            TVS_LOG_PRINTF("stop down channel, wait %d secs to connect\n", wait_long);
            tvs_down_channel_wait(wait_long * 1000);
            fail_count = 0;
        } else {
            TVS_LOG_PRINTF("stop down channel, fail count %d\n", fail_count);
        }
    }
}

void tvs_down_channel_init()
{
    TVS_LOCKER_INIT

    if (g_waiter_mutex == NULL) {
        g_waiter_mutex = os_wrapper_create_signal_mutex(0);
    }

    if (tvs_config_is_down_channel_enable()) {
        os_wrapper_start_thread(tvs_down_channel_thread, NULL, "down", 3, 1024);
    }
}

void tvs_down_channel_system_sync()
{
    if (tvs_down_channel_is_connect()) {
        // 同步本地状态到云端，如果和云端状态有差异，会通过下行通道将差异下发
        tvs_executor_send_system_sync();
    }
}

void tvs_down_channel_callback_on_connect(void *connection, int error_code, tvs_http_client_param *param)
{
    tvs_down_channel_param *reco_param = (tvs_down_channel_param *)param->user_data;
    if (error_code == 0) {
        TVS_LOG_PRINTF("down channel connected\n");
        reco_param->last_time = os_wrapper_get_time_ms();
        tvs_down_channel_set_connect(true);

        tvs_down_channel_system_sync();
    }
}

void tvs_down_channel_callback_on_response(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param)
{
    //TVS_LOG_PRINTF("get resonse ret %d - %.*s\n", ret_code, response_len, response);
}

void tvs_down_channel_callback_on_recv_chunked(void *connection, void *ev_data, const char *chunked, int chunked_len, tvs_http_client_param *param)
{
    tvs_down_channel_param *speech_param = (tvs_down_channel_param *)param->user_data;

    tvs_multipart_process_chunked(&speech_param->multipart_info, &speech_param->multipart_callback, &speech_param->multi_param, ev_data, false, true);
}

void tvs_down_channel_callback_on_close(void *connection, int by_server, tvs_http_client_param *param)
{
    TVS_LOG_PRINTF("directives exit by call %s\n", __func__);
}

int tvs_down_channel_callback_on_send_request(void *connection, const char *path, const char *host, const char *extra_header, const char *payload, tvs_http_client_param *param)
{
    tvs_http_send_multipart_start((struct mg_connection *)connection, path, strlen(path), host, strlen(host), NULL);
    return 0;
}

bool tvs_down_channel_callback_on_loop(void *connection, tvs_http_client_param *param)
{
    //TVS_LOG_PRINTF("directives on loop...................\n");

    tvs_down_channel_param *reco_param = (tvs_down_channel_param *)param->user_data;
    long time = os_wrapper_get_time_ms();

    int interval = tvs_config_get_hearttick_interval();

    if (interval <= 25) {
        interval = 25;
    } else if (interval >= 46) {
        interval = 46;
    }

    if (param->connected && time - reco_param->last_time > interval * 1024) {
        reco_param->last_time = time;
        // send heart tick，每45秒发一次，防止下行通道断开
        tvs_http_send_multipart_data(connection, g_bin_data, HEART_TICK_BENDATA_LEN);
    }
    return false;
}

bool tvs_down_channel_callback_should_exit_loop(void *exit_param)
{

    if (tvs_down_channel_can_connect()) {
        return false;
    }
    TVS_LOG_PRINTF("directives exit by call %s\n", __func__);
    // return true代表退出循环
    return true;
}

bool tvs_down_channel_callback_should_cancel_loop(void *exit_param)
{


    bool do_break = tvs_down_channel_should_break();

    if (do_break) {
        tvs_down_channel_set_break(false);
        TVS_LOG_PRINTF("directives exit by call %s\n", __func__);
    }

    return do_break;
}

void tvs_down_channel_callback_on_loop_end(tvs_http_client_param *param)
{
    TVS_LOG_PRINTF("directives exit by call %s\n", __func__);
    tvs_down_channel_param *control_param = (tvs_down_channel_param *)param->user_data;

    tvs_release_multipart_parser(&control_param->multipart_info, true);
}

int tvs_down_channel_start_connect()
{
    tvs_http_client_callback cb = { 0 };
    cb.cb_on_close = tvs_down_channel_callback_on_close;
    cb.cb_on_connect = tvs_down_channel_callback_on_connect;
    cb.cb_on_chunked = tvs_down_channel_callback_on_recv_chunked;
    cb.cb_on_loop = tvs_down_channel_callback_on_loop;
    cb.cb_on_request = tvs_down_channel_callback_on_send_request;
    cb.cb_on_response = tvs_down_channel_callback_on_response;
    cb.cb_on_loop_end = tvs_down_channel_callback_on_loop_end;
    cb.exit_loop = tvs_down_channel_callback_should_exit_loop;
    cb.should_cancel = tvs_down_channel_callback_should_cancel_loop;

    tvs_http_client_config config = { 0 };
    config.total_timeout = 1800;       //应TVS那边工作人员要求修改半个小时超时
    tvs_http_client_param http_param = { 0 };
    tvs_down_channel_param event_param = { 0 };
    http_param.not_check_timer = true;

    char *url = tvs_config_get_down_channel_url();
    //TVS_LOG_PRINTF("start down channel, url %s\n", url);
    tvs_init_multipart_callback(&event_param.multipart_callback);

    int ret = tvs_http_client_request_ex1(url, NULL, NULL, &event_param, &config, &cb, &http_param, true, 2000, false);

    return ret;
}

