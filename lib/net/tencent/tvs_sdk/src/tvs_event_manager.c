/**
* @file  tvs_event_manager.c
* @brief 发送EVENT数据，用于数据上报
* @date  2019-5-10
*/

#include "tvs_http_client.h"
#include "tvs_http_manager.h"
#include "tvs_config.h"
#include "tvs_jsons.h"
#include "tvs_executor_service.h"
#define TVS_LOG_DEBUG_MODULE  "EVENTS"
#include "tvs_log.h"

typedef struct {
    void *p;
} tvs_event_param;

void tvs_event_callback_on_connect(void *connection, int error_code, tvs_http_client_param *param) { }

void tvs_event_callback_on_response(void *connection, int ret_code, const char *response, int response_len, tvs_http_client_param *param)
{
    TVS_LOG_PRINTF("get resonse ret %d - %.*s\n", ret_code, response_len, response);
}

void tvs_event_callback_on_close(void *connection, int by_server, tvs_http_client_param *param) { }

int tvs_event_callback_on_send_request(void *connection, const char *path, const char *host, const char *extra_header, const char *payload, tvs_http_client_param *param)
{
    tvs_http_send_normal_multipart_request((struct mg_connection *)connection, path, strlen(path), host, strlen(host), payload);
    return 0;
}

bool tvs_event_callback_on_loop(void *connection, tvs_http_client_param *param)
{
    return false;
}

int tvs_event_manager_start(const char *payload,
                            tvs_http_client_callback_exit_loop should_exit_func,
                            tvs_http_client_callback_should_cancel should_cancel,
                            void *exit_param, bool *is_force_break)
{
    TVS_LOG_PRINTF("event body: %s\n", payload);
    tvs_http_client_callback cb = { 0 };
    cb.cb_on_close = tvs_event_callback_on_close;
    cb.cb_on_connect = tvs_event_callback_on_connect;
    cb.cb_on_loop = tvs_event_callback_on_loop;
    cb.cb_on_request = tvs_event_callback_on_send_request;
    cb.cb_on_response = tvs_event_callback_on_response;
    cb.exit_loop = should_exit_func;
    cb.exit_loop_param = exit_param;
    cb.should_cancel = should_cancel;

    tvs_http_client_config config = { 0 };
    tvs_http_client_param http_param = { 0 };
    tvs_event_param event_param = { 0 };

    char *url = tvs_config_get_event_url();
    TVS_LOG_PRINTF("start events, url %s\n", url);
    int ret = tvs_http_client_request(url, NULL, payload, &event_param, &config, &cb, &http_param, true);

    if (is_force_break != NULL) {
        *is_force_break = http_param.force_break;
    }

    return ret;
}

char *tvs_event_manager_create_payload(int cmd, int iparam, void *pparam, void *pparam2)
{
    switch (cmd) {
    case TVS_EXECUTOR_CMDS_EVENT_VOLUME_CHANGED:
        return get_speaker_volume_upload_body(iparam);
    case TVS_EXECUTOR_CMDS_EVENT_MODE_CHANGED:
        return get_mode_changed_upload((char *)pparam, (char *)pparam2);
    case TVS_EXECUTOR_CMDS_EVENT_MEDIA_PLAYER_STATE_CHANGED:
        return get_media_player_upload_body(iparam);
    case TVS_EXECUTOR_CMDS_EVENT_PUSH:
        return get_push_upload_body((char *)pparam);
    case TVS_EXECUTOR_CMDS_EVENT_PUSH_ACK:
        return get_push_ack_body((char *)pparam);
    case TVS_EXECUTOR_CMDS_EVENT_SYSTEM_SYNC:
        return get_sync_state_upload_body();
    case TVS_EXECUTOR_CMDS_EVENT_ALERT_NEW:
        return get_alart_process_body(true, iparam, (char *)pparam);
    case TVS_EXECUTOR_CMDS_EVENT_ALERT_DELETE:
        return get_alart_process_body(false, iparam, (char *)pparam);
    case TVS_EXECUTOR_CMDS_EVENT_ALERT_START:
        return get_alert_trigger_body(true, (char *)pparam);
    case TVS_EXECUTOR_CMDS_EVENT_ALERT_STOP:
        return get_alert_trigger_body(false, (char *)pparam);
    case TVS_EXECUTOR_CMDS_EVENT_EXCEPTION_REPORT:
        return get_exception_report_body(iparam, (char *)pparam);
    default:
        break;
    }
    return NULL;
}