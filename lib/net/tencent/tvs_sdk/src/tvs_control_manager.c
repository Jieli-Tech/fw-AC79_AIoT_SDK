/**
* @file  tvs_data_buffer.c
* @brief TVS 播控流程的实现，向后台发起http请求的流程
* @date  2019-5-10
*
*/

#include "tvs_http_client.h"
#include "tvs_http_manager.h"
#include "tvs_config.h"
#include "tvs_jsons.h"
#include "tvs_executor_service.h"
#include "tvs_multipart_handler.h"
#include "tvs_audio_track_interface.h"

#define TVS_LOG_DEBUG_MODULE  "CONTROL"
#include "tvs_log.h"

#include "tvs_state.h"


typedef struct {
    void *multipart_info;
    tvs_multipart_callback multipart_callback;
    tvs_multi_param multi_param;
} tvs_control_param;

void tvs_control_callback_on_connect(void *connection, int error_code, tvs_http_client_param *param)
{
    if (error_code == 0) {
        //TVS_LOG_PRINTF("server connected\n");
    } else {
        TVS_LOG_PRINTF("%s, error %d\n", __func__, error_code);
    }
}

void tvs_control_callback_on_response(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param)
{
    TVS_LOG_PRINTF("get resonse ret %d\n", ret_code);
}

void tvs_control_callback_on_recv_chunked(void *connection, void *ev_data, const char *chunked, int chunked_len, tvs_http_client_param *param)
{
    tvs_control_param *control_param = (tvs_control_param *)param->user_data;

    tvs_multipart_process_chunked(&control_param->multipart_info, &control_param->multipart_callback, &control_param->multi_param, ev_data, false, false);
}

void tvs_control_callback_on_close(void *connection, int by_server, tvs_http_client_param *param) { }

int tvs_control_callback_on_send_request(void *connection, const char *path, const char *host, const char *extra_header, const char *payload, tvs_http_client_param *param)
{
    tvs_http_send_normal_multipart_request((struct mg_connection *)connection, path, strlen(path), host, strlen(host), payload);
    return 0;
}

bool tvs_control_callback_on_loop(void *connection, tvs_http_client_param *param)
{
    return false;
}

void tvs_control_callback_on_loop_end(tvs_http_client_param *param)
{
    tvs_control_param *control_param = (tvs_control_param *)param->user_data;

    tvs_release_multipart_parser(&control_param->multipart_info, false);
}

int tvs_control_manager_start(int control_type, const char *payload, int session_id,
                              tvs_http_client_callback_exit_loop should_exit_func,
                              tvs_http_client_callback_should_cancel should_cancel,
                              void *exit_param,
                              bool *expect_speech,
                              bool *has_media,
                              bool *connected)
{
    tvs_http_client_callback cb = { 0 };
    cb.cb_on_close = tvs_control_callback_on_close;
    cb.cb_on_connect = tvs_control_callback_on_connect;
    cb.cb_on_chunked = tvs_control_callback_on_recv_chunked;
    cb.cb_on_loop = tvs_control_callback_on_loop;
    cb.cb_on_request = tvs_control_callback_on_send_request;
    cb.cb_on_response = tvs_control_callback_on_response;
    cb.cb_on_loop_end = tvs_control_callback_on_loop_end;
    cb.exit_loop = should_exit_func;
    cb.exit_loop_param = exit_param;
    cb.should_cancel = should_cancel;

    tvs_http_client_config config = { 0 };
    tvs_http_client_param http_param = { 0 };
    tvs_control_param event_param = { 0 };
    event_param.multi_param.control_type = control_type;

    event_param.multi_param.directives_param.session_id = session_id;

    char *url = tvs_config_get_event_url();
    TVS_LOG_PRINTF("start control, payload %s, session %d\n", payload, session_id);
    tvs_init_multipart_callback(&event_param.multipart_callback);

    tvs_state_set(TVS_STATE_BUSY, control_type, 0);

    int ret = tvs_http_client_request(url, NULL, payload, &event_param, &config, &cb, &http_param, true);

    //TVS_LOG_PRINTF("control end, has tts %d, total tts %d\n, ret %d",
    //		event_param.multi_param.has_tts, event_param.multi_param.tts_bytes, ret);

    if (expect_speech != NULL) {
        *expect_speech = event_param.multi_param.directives_param.expect_speech;
    }

    if (has_media != NULL) {
        *has_media = event_param.multi_param.directives_param.has_media;
    }

    if (connected != NULL) {
        *connected = http_param.connected;
    }

    return ret;
}

char *tvs_control_manager_create_payload(int cmd, int iparam, void *pparam, void *other_param)
{
    return get_control_request_body(cmd, pparam, other_param);
}

