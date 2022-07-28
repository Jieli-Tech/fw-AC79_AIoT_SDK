#include "executor_service.h"
#include "tvs_event_manager.h"
#include "tvs_authorizer.h"
#include "tvs_executor_service.h"
#include "tvs_speech_manager.h"
#include "tvs_control_manager.h"
#include "tvs_config.h"
#include "tvs_audio_track_interface.h"
#include "tvs_media_player_inner.h"
#include "tvs_state.h"
#include "tvs_version.h"
#include "tvs_iplist.h"
#include "data_template_tvs_auth.h"


#define TVS_LOG_DEBUG_MODULE  "EXECUTOR"
#include "tvs_log.h"

extern tvs_api_callback g_tvs_core_callback;

extern void tvs_tts_player_close();

static char *get_command_name(int cmd)
{
    char *name = "Unknown";
    switch (cmd) {
    case TVS_EXECUTOR_CMD_SPEECH:
        name = "Recognize";
        break;
    case TVS_EXECUTOR_CMD_PLAY_NEXT:
        name = "PlayNext";
        break;
    case TVS_EXECUTOR_CMD_PLAY_PREV:
        name = "PlayPrev";
        break;
    case TVS_EXECUTOR_CMD_PLAY_FINISH:
        name = "PlayFinish";
        break;
    case TVS_EXECUTOR_CMD_SEMANTIC:
        name = "Semantic";
        break;
    default:
        break;
    }

    return name;
}

static int get_command_type(int cmd)
{
    int type = 0;
    switch (cmd) {
    case TVS_EXECUTOR_CMD_SPEECH:
        type = TVS_CONTROL_SPEECH;
        break;
    case TVS_EXECUTOR_CMD_PLAY_NEXT:
        type = TVS_CONTROL_PLAY_NEXT;
        break;
    case TVS_EXECUTOR_CMD_PLAY_PREV:
        type = TVS_CONTROL_PLAY_PREV;
        break;
    case TVS_EXECUTOR_CMD_PLAY_FINISH:
        type = TVS_CONTROL_PLAY_FINISH;
        break;
    case TVS_EXECUTOR_CMD_SEMANTIC:
        type = TVS_CONTROL_SEMANTIC;
        break;
    default:
        break;
    }

    return type;
}


static void print_start_log(int cmd, int session_id)
{
    TVS_LOG_PRINTF("*************start %s, version %s_%s_%s, host %s, sandbox %d, session id %d***********\n",
                   get_command_name(cmd),
                   TVS_BUILD_DATE,
                   TVS_BUILD_NUMBER,
                   TVS_BUILD_TYPE,
                   tvs_config_get_current_host(),
                   tvs_config_is_sandbox_open(),
                   session_id);
}

// 将http接口的exit和executor service的每个节点的break操作关联起来；
// 每个Node的任务都是一个http请求，break一个Node的任务，就是将Http会话打断的过程
bool tvs_http_client_cb_exit_loop(void *exit_param)
{
    executor_node_param *param = (executor_node_param *)exit_param;
    // should_break在executor_service.c的create_node函数中，指向executor_should_break函数
    bool do_break = param->should_break(param);
    if (do_break) {
        TVS_LOG_PRINTF("cmd %d should break\n", param->cmd);
    }
    return do_break;
}

// 将http接口的cancel和executor service的每个节点的cancel操作关联起来；
// 每个Node的任务都是一个http请求，cancel一个Node的任务，就是将Http会话取消的过程
bool tvs_http_client_cb_cancel_loop(void *exit_param)
{
    executor_node_param *param = (executor_node_param *)exit_param;
    // should_cancel在executor_service.c的create_node函数中，指向executor_should_cancel函数
    bool cancel = param->should_cancel(param);
    if (cancel) {
        TVS_LOG_PRINTF("cmd %d should cancel\n", param->cmd);
    }

    return cancel;
}

int speech_open_check(void)
{
    return tvs_is_executor_valid_ex(TVS_EXECUTOR_CMD_SPEECH, false);
}


int tvs_is_executor_valid_ex(int cmd, bool check_for_auth)
{
    // 判断是否能连接外网
    if (!tvs_config_is_network_valid()) {
        TVS_LOG_PRINTF("start cmd %d fail for network invalid\n", cmd);
        return TVS_API_ERROR_NETWORK_INVALID;
    }

    if (check_for_auth) {
        // 授权逻辑需要判断是否超时
        if (!tvs_authorizer_is_timeout()) {
            TVS_LOG_PRINTF("start auth fail for not timeout\n");
            return TVS_API_ERROR_OTHERS;
        }
    } else {
        // 非授权逻辑，需要判断SDK RUNNING状态，以及是否授权
        if (!tvs_config_is_sdk_running()) {
            TVS_LOG_PRINTF("start cmd %d fail for sdk paused\n", cmd);
            return TVS_API_ERROR_NOT_RUNNING;
        }

        //异常上报不需要授权
        if (!tvs_authorizer_is_authorized() && cmd != TVS_EXECUTOR_CMDS_EVENT_EXCEPTION_REPORT) {
            TVS_LOG_PRINTF("start cmd %d fail for authorize invalid\n", cmd);
            return TVS_API_ERROR_NOT_ATHORIZED;
        }
    }

    return TVS_API_ERROR_NONE;
}


bool tvs_is_executor_valid(int cmd, bool check_for_auth)
{

    return tvs_is_executor_valid_ex(cmd, check_for_auth) == 0;
}

static void free_event_param(tvs_executor_param_events *ev_params)
{
    if (ev_params != NULL) {
        if (ev_params->free_func != NULL) {
            ev_params->free_func(ev_params->free_func_param);
        }

        if (ev_params->param != NULL) {
            TVS_FREE(ev_params->param);
            ev_params->param = NULL;
        }

        if (ev_params->param2 != NULL) {
            TVS_FREE(ev_params->param2);
            ev_params->param2 = NULL;
        }
    }
}

bool executor_runnable_start_play(executor_node_param *param)
{
    TVS_LOG_PRINTF("start PLAY, cmd %d\n", param->cmd);

    bool should_retry = false;

    tvs_media_player_inner_start_play();

    return should_retry;
}

static void iplist_func(void *param)
{
    tvs_executor_start_iplist(false);
}

bool executor_runnable_start_iplist(executor_node_param *param)
{
    bool should_retry = false;
    bool force_exit = false;

    int ret = tvs_iplist_start(tvs_http_client_cb_exit_loop, tvs_http_client_cb_cancel_loop, param, &force_exit);

    if (ret == 0) {
        should_retry = false;
    } else {
        should_retry = force_exit;
    }

    int time = tvs_iplist_get_time(ret == 0);
    tvs_iplist_reset_timer(time, iplist_func);

    return should_retry;
}

extern bool tvs_ping_start(tvs_http_client_callback_exit_loop should_exit_func,
                           tvs_http_client_callback_should_cancel should_cancel,
                           void *exit_param);

bool executor_runnable_start_ping(executor_node_param *param)
{
    bool should_retry = false;

    tvs_ping_start(tvs_http_client_cb_exit_loop, tvs_http_client_cb_cancel_loop, param);

    should_retry = false;

    return should_retry;
}

bool executor_runnable_send_event(executor_node_param *param)
{
    TVS_LOG_PRINTF("start event, cmd %d\n", param->cmd);
    tvs_executor_param_events *ev_params = (tvs_executor_param_events *)param->runnable_param;

    if (!tvs_is_executor_valid(param->cmd, false)) {
        TVS_LOG_PRINTF("start event, cmd %d, failed\n", param->cmd);
        free_event_param(ev_params);
        return false;
    }

    bool should_retry = false;

    char *payload = tvs_event_manager_create_payload(param->cmd, ev_params->iparam, ev_params->param, ev_params->param2);
    free_event_param(ev_params);
    if (payload == NULL || strlen(payload) <= 0) {
        TVS_LOG_PRINTF("start event, cmd %d, invalid payload\n", param->cmd);
        return false;
    }
    bool force_exit = false;

    int ret = tvs_event_manager_start(payload, tvs_http_client_cb_exit_loop, tvs_http_client_cb_cancel_loop, param, &force_exit);

    if (ret >= 200 && ret < 300) {
        should_retry = false;
    } else {
        should_retry = force_exit;
    }

    TVS_FREE(payload);

    TVS_LOG_PRINTF("end event, cmd %d, should retry %d\n", param->cmd, should_retry);
    // event上报如果被speech或者control给break了，需要重新开始
    return should_retry;
}

bool executor_runnable_start_authorize(executor_node_param *param)
{
    //TVS_LOG_PRINTF("start send authorize, cmd %d\n", param->cmd);

    if (!tvs_is_executor_valid(param->cmd, true)) {
        TVS_LOG_PRINTF("start send authorize, failed\n");
        return false;
    }

    bool should_retry = false;
    tvs_executor_param_authorize *auth = (tvs_executor_param_authorize *)param->runnable_param;

    tvs_authorizer_manager_start((char *)auth->client_id, (char *)auth->refresh_token, tvs_http_client_cb_exit_loop, tvs_http_client_cb_cancel_loop, param);
    //TVS_LOG_PRINTF("end send authorize, cmd %d, should retry %d\n", param->cmd, should_retry);
    return should_retry;
}

static void expect_speech_timeout_func(void *timer)
{
    TVS_LOG_PRINTF("expect speech timeout!\n");
    tvs_state_start_work(false, TVS_CONTROL_SPEECH, 0);
}

static void do_check_expect_speech(bool expect_speech, bool force_break, int error, int type)
{
    if (!expect_speech && !force_break) {
        // 如果speech结束，未被打断，且没有多轮对话，则恢复IDLE状态
        tvs_state_start_work(false, type, error);
    } else if (expect_speech) {
        // 如果后台下发了多轮对话指令，起一个定时器，防止外层不触发下一轮speech
        start_expect_speech_timer(expect_speech_timeout_func, 8 * 1000);

        if (g_tvs_core_callback.on_expect_speech != NULL) {
            // 通知外层触发下一轮对话
            g_tvs_core_callback.on_expect_speech();
        }
    } else {
        // speech被打断的情况
        TVS_LOG_PRINTF("force break speech\n");
    }
}

static void do_reset_expect_speech()
{
    stop_expect_speech_timer();
}

bool executor_runnable_start_speech(executor_node_param *param)
{
    // 开启智能语音
    if (!tvs_is_executor_valid(param->cmd, false)) {
        return false;
    }

    bool expect_speech = false;
    tvs_speech_manager_config *config = (tvs_speech_manager_config *)param->runnable_param;

    int session_id = tvs_core_get_current_session_id();
    if (session_id != config->session_id) {
        TVS_LOG_PRINTF("speech %d is out-of-date\n", config->session_id);
        return false;
    }

    tvs_state_start_work(true, TVS_CONTROL_SPEECH, 0);

    // 停止上一次TTS，如果有的话
    tvs_tts_player_close();
    print_start_log(param->cmd, config->session_id);
    bool connected = false;

    // 发起智能语音请求
    int ret = tvs_speech_manager_start(config, tvs_http_client_cb_exit_loop, tvs_http_client_cb_cancel_loop, param, &expect_speech, &connected);

    bool force_break = !tvs_core_check_session_id(config->session_id);

    TVS_LOG_PRINTF("***stop %s, ret %d, connected %d, force_break %d, expect speech %d, session %d***\n",
                   get_command_name(param->cmd), ret, connected,
                   force_break, expect_speech, config->session_id);
    int error = 0;
    if (!connected || (ret != 200 && ret != 204)) {
        error = TVS_API_ERROR_NETWORK_ERROR;
    }

    do_check_expect_speech(expect_speech, force_break, error, TVS_CONTROL_SPEECH);

    return false;
}

static bool is_media_control(int cmd)
{
    switch (cmd) {
    case TVS_EXECUTOR_CMD_PLAY_NEXT:
    case TVS_EXECUTOR_CMD_PLAY_PREV:
    case TVS_EXECUTOR_CMD_PLAY_FINISH:
        return true;
    default:
        break;
    }

    return false;
}

bool executor_runnable_start_control(executor_node_param *param)
{
    // 用于播控切换
    tvs_executor_param_events *ev_params = (tvs_executor_param_events *)param->runnable_param;

    if (!tvs_is_executor_valid(param->cmd, false)) {
        free_event_param(ev_params);
        return false;
    }

    int session_id = tvs_core_get_current_session_id();
    if (session_id != ev_params->session_id) {
        TVS_LOG_PRINTF("control %d is out-of-date\n", ev_params->session_id);
        return false;
    }

    bool expect_speech = false;
    char *payload = tvs_control_manager_create_payload(param->cmd, ev_params->iparam, ev_params->param, ev_params->free_func_param);
    free_event_param(ev_params);
    if (payload == NULL || strlen(payload) <= 0) {
        TVS_LOG_PRINTF("control create payload failed\n");
        return false;
    }

    int c_type = get_command_type(param->cmd);

    tvs_state_start_work(true, c_type, 0);

    // 停止上一次TTS，如果有的话
    tvs_tts_player_close();

    print_start_log(param->cmd, 0);

    bool has_media = false;
    bool connected = false;
    int ret = tvs_control_manager_start(c_type, payload, ev_params->session_id, tvs_http_client_cb_exit_loop,
                                        tvs_http_client_cb_cancel_loop, param, &expect_speech, &has_media, &connected);

    TVS_LOG_PRINTF("***stop %s, ret %d, connected %d, has_media %d***\n",
                   get_command_name(param->cmd), ret, connected, has_media);

    TVS_FREE(payload);

    int error = 0;
    if (!connected || (ret != 200 && ret != 204)) {
        error = TVS_API_ERROR_NETWORK_ERROR;
    } else if (!has_media && is_media_control(param->cmd)) {
        error = TVS_API_ERROR_NO_MORE_MEDIA;
    }

    do_check_expect_speech(expect_speech, false, error, c_type);

    return false;
}


bool executor_runnable_event_release(executor_node_param *node_param)
{
    if (node_param == NULL || node_param->runnable_param == NULL) {
        return true;
    }

    if (node_param->cmd == TVS_EXECUTOR_CMD_AUTHORIZE) {
        tvs_executor_param_authorize *auth = (tvs_executor_param_authorize *)node_param->runnable_param;
        if (auth->client_id != NULL) {
            TVS_FREE(auth->client_id);
        }

        if (auth->refresh_token != NULL) {
            TVS_FREE(auth->refresh_token);
        }
    }

    return true;
}

int tvs_executor_start_play()
{
    tvs_executor_param_events ev_param = {0};
    executor_service_start_tail(TVS_EXECUTOR_CMD_START_PLAY, executor_runnable_start_play, executor_runnable_event_release, &ev_param, sizeof(tvs_executor_param_events));
    return 0;
}

int tvs_executor_start_event(int cmd, tvs_executor_param_events *ev_params)
{
    int ret = 0;
    if ((ret = tvs_is_executor_valid_ex(cmd, false)) != 0) {
        free_event_param(ev_params);
        return ret;
    }

    executor_service_start_tail(cmd, executor_runnable_send_event, executor_runnable_event_release, ev_params, sizeof(tvs_executor_param_events));
    return 0;
}

int tvs_executor_start_speech(tvs_speech_manager_config *config)
{
    TVS_LOG_PRINTF("%s\n", __func__);

    // 停止expect speech的计时器，如果有的话
    do_reset_expect_speech();
    int cmd = TVS_EXECUTOR_CMD_SPEECH;
    int ret = 0;
    if ((ret = tvs_is_executor_valid_ex(cmd, false)) != 0) {
        return ret;
    }

    // 立刻开始执行，speech的实时性很高，所以优先级也很高，会打断当前正在执行的任务
    executor_service_start_immediately(cmd, executor_runnable_start_speech, executor_runnable_event_release, config, sizeof(tvs_speech_manager_config));
    return 0;
}

int tvs_executor_start_iplist(bool check_timeout)
{
    if (!tvs_config_is_network_valid()) {
        TVS_LOG_PRINTF("start iplist fail for network invalid\n");
        return TVS_API_ERROR_NETWORK_INVALID;
    }

    if (check_timeout && !tvs_iplist_is_timeout()) {
        TVS_LOG_PRINTF("iplist is not timeout\n");
        return 0;
    }

    tvs_iplist_clear();

    tvs_iplist_exe_cfg cfg = {0};

    executor_service_start_tail(TVS_EXECUTOR_CMD_IPLIST, executor_runnable_start_iplist, executor_runnable_event_release, &cfg, sizeof(cfg));
    return 0;

}

int tvs_executor_start_control(int cmd, tvs_executor_param_events *ev_params)
{
    int session = tvs_core_new_session_id();
    do_reset_expect_speech();

    int ret = 0;
    if ((ret = tvs_is_executor_valid_ex(cmd, false)) != 0) {
        free_event_param(ev_params);
        return ret;
    }
    ev_params->session_id = session;
    // 立刻开始执行，control的实时性很高，所以优先级也很高，会打断当前正在执行的任务
    executor_service_start_immediately(cmd, executor_runnable_start_control, executor_runnable_event_release, ev_params, sizeof(tvs_executor_param_events));
    return 0;
}

void tvs_executor_stop_all_activities()
{
    executor_service_stop_all_activities(TVS_EXECUTOR_CMD_ACTIVITIES);
}


int tvs_executor_start_authorize()
{
    TVS_LOG_PRINTF("%s\n", __func__);
    int ret = 0;

    //检查是否连上网络
    if ((ret = tvs_is_executor_valid_ex(TVS_EXECUTOR_CMD_AUTHORIZE, true)) != 0) {
        TVS_LOG_PRINTF("%s, failed\n", __func__);
        /* if (ret == TVS_API_ERROR_OTHERS) { */
        /* IOT_Tvs_Auth_Error_Cb(false, TVS_API_ERROR_CLIENT_ID_INVALID); */
        /* } */
        return ret;
    }

    char *client_id = tvs_authorizer_dup_current_client_id();	//拿到当前clientID

    if (client_id == NULL || strlen(client_id) <= 0) {
        if (client_id != NULL) {
            TVS_FREE(client_id);
        }

        TVS_LOG_PRINTF("%s, invalid client id\n", __func__);
        /* IOT_Tvs_Auth_Error_Cb(false, TVS_API_ERROR_CLIENT_ID_INVALID); */
        return TVS_API_ERROR_CLIENT_ID_INVALID;
    }

    tvs_executor_param_authorize auth_param = {0};
    auth_param.client_id = client_id;

    auth_param.refresh_token = tvs_authorizer_dup_refresh_token();		//从设备存储中获得refresh_token

    executor_service_start_tail(TVS_EXECUTOR_CMD_AUTHORIZE, executor_runnable_start_authorize, executor_runnable_event_release, &auth_param, sizeof(tvs_executor_param_authorize));

    return 0;
}

void tvs_executor_cancel_authorize()
{
    executor_service_cancel_cmds(TVS_EXECUTOR_CMD_AUTHORIZE);
}

void tvs_executor_start_ping()
{
    if (!tvs_is_executor_valid(TVS_EXECUTOR_CMD_PING, false)) {
        //TVS_LOG_PRINTF("start send ping, failed\n");
        return;
    }

    executor_service_start_tail(TVS_EXECUTOR_CMD_PING, executor_runnable_start_ping, executor_runnable_event_release, NULL, 0);
}


int tvs_executor_service_start()
{
    executor_service_init();
    return 0;
}

void tvs_executor_upload_volume(int volume)
{
    tvs_executor_param_events ev_param = {0};
    ev_param.iparam = volume;

    executor_service_cancel_cmds(TVS_EXECUTOR_CMDS_EVENT_VOLUME_CHANGED);

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_VOLUME_CHANGED, &ev_param);
}

void tvs_executor_upload_alert_new(int success, char *token)
{
    if (token == NULL) {
        return;
    }
    tvs_executor_param_events ev_param = {0};
    ev_param.param = strdup(token);
    ev_param.iparam = success;

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_ALERT_NEW, &ev_param);
}

void tvs_executor_upload_alert_delete(int success, char *token)
{
    if (token == NULL) {
        return;
    }
    tvs_executor_param_events ev_param = {0};
    ev_param.param = strdup(token);
    ev_param.iparam = success;

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_ALERT_DELETE, &ev_param);
}

void tvs_executor_upload_alert_start(char *token)
{
    if (token == NULL) {
        return;
    }
    tvs_executor_param_events ev_param = {0};
    ev_param.param = strdup(token);

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_ALERT_START, &ev_param);
}

void tvs_executor_upload_alert_stop(char *token)
{
    if (token == NULL) {
        return;
    }
    tvs_executor_param_events ev_param = {0};
    ev_param.param = strdup(token);

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_ALERT_STOP, &ev_param);
}

void tvs_executor_send_push_text(char *text)
{
    if (text == NULL) {
        return;
    }
    tvs_executor_param_events ev_param = {0};
    ev_param.param = strdup(text);

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_PUSH, &ev_param);
}

void tvs_executor_send_system_sync()
{
    tvs_executor_param_events ev_param = {0};

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_SYSTEM_SYNC, &ev_param);
}

void tvs_executor_send_push_ack(char *token)
{
    if (token == NULL) {
        return;
    }
    tvs_executor_param_events ev_param = {0};
    ev_param.param = strdup(token);

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_PUSH_ACK, &ev_param);
}

void tvs_executor_upload_mode_changed(char *src, char *dst)
{
    if (src == NULL || dst == NULL) {
        return;
    }
    tvs_executor_param_events ev_param = {0};
    ev_param.param = strdup(src);
    ev_param.param2 = strdup(dst);

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_MODE_CHANGED, &ev_param);
}

void tvs_executor_upload_media_player_state(int state)
{
    tvs_executor_param_events ev_param = {0};
    ev_param.iparam = state;

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_MEDIA_PLAYER_STATE_CHANGED, &ev_param);
}

void tvs_executor_start_exception_report(exception_report_type type, const char *message)
{
    tvs_executor_param_events ev_param = {0};
    ev_param.iparam = (int)type;
    int str_len = strlen(message) + strlen(g_http_respone_session_id) + 3;
    char *ptemp = TVS_MALLOC(str_len);
    snprintf(ptemp, str_len, "%s:%s", g_http_respone_session_id, message);
    ev_param.param = ptemp;

    executor_service_cancel_cmds(TVS_EXECUTOR_CMDS_EVENT_EXCEPTION_REPORT);

    tvs_executor_start_event(TVS_EXECUTOR_CMDS_EVENT_EXCEPTION_REPORT, &ev_param);
}

