#include "tvs_speech_manager.h"
#include "tvs_executor_service.h"
#include "tvs_audio_recorder_thread.h"
#include "tvs_audio_provider.h"
#include "tvs_audio_track_interface.h"
#include "tvs_core.h"
#include "tvs_authorizer.h"
#include "tvs_down_channel.h"
#include "tvs_speech_manager.h"
#include "tvs_config.h"
#include "tvs_media_player_inner.h"
#include "tvs_log.h"
#include "tvs_preference.h"
#include "tvs_audio_provider.h"
#include "tvs_mp3_player.h"
#include "tvs_ip_provider.h"
#include "tvs_state.h"
#include "tvs_threads.h"

#include <stdlib.h>
#include <string.h>

#if CONFIG_USE_MTK_LOG

log_create_module(TVS, PRINT_LEVEL_INFO);

#endif

static int g_current_session_id = 0;

tvs_api_callback g_tvs_core_callback = {0};

tvs_default_config g_tvs_system_config = {0};

static bool g_log_enable = true;

TVS_LOCKER_DEFINE

int tvs_core_new_session_id()
{
    do_lock();
    int id = os_wrapper_get_time_ms();
    if (g_current_session_id == id) {
        id++;
    }
    g_current_session_id = id;

    //TVS_LOG_PRINTF("---------------------------------new session id %d, start\n", id);
    do_unlock();

    return id;
}

int tvs_core_get_current_session_id()
{
    do_lock();
    int id = g_current_session_id;
    do_unlock();

    return id;
}

void tvs_core_set_current_session_id(int session_id)
{
    /*do_lock();
    g_current_session_id = session_id;
    do_unlock();*/
}


bool tvs_core_check_session_id(int session_id)
{
    do_lock();
    bool same = g_current_session_id == session_id;
    do_unlock();

    return same;
}


extern int tvs_audio_provider_initialize();
extern int tvs_audio_recorder_thread_init();

int tvs_core_initialize(tvs_api_callback *callback, tvs_default_config *default_config, tvs_product_qua *qua)
{
    TVS_LOG_PRINTF("%s, start\n", __func__);

    TVS_LOCKER_INIT

    memset(&g_tvs_core_callback, 0, sizeof(tvs_api_callback));

    if (NULL != callback) {
        memcpy(&g_tvs_core_callback, callback, sizeof(tvs_api_callback));
    }

    memset(&g_tvs_system_config, 0, sizeof(tvs_default_config));

    g_tvs_system_config.def_env = TVS_API_ENV_NORMAL;
    g_tvs_system_config.def_sandbox_open = false;
    g_tvs_system_config.recorder_bitrate = 8000;
    g_tvs_system_config.recorder_channels = 1;

    if (NULL != default_config) {
        memcpy(&g_tvs_system_config, default_config, sizeof(tvs_default_config));
    }

    if (g_tvs_system_config.recorder_bitrate != 8000 && g_tvs_system_config.recorder_bitrate != 16000) {
        g_tvs_system_config.recorder_bitrate = 8000;
    }

    if (g_tvs_system_config.recorder_channels <= 0) {
        g_tvs_system_config.recorder_channels = 1;
    }

    tvs_state_manager_init();           //互斥锁初始化
    tvs_config_set_qua(qua);            //写入设备信息
    tvs_preference_module_init();       //互斥锁初始化
    tvs_config_init();                  //互斥锁初始化
    tvs_authorizer_init();              //互斥锁初始化
    //tvs_audio_provider_initialize();    //互斥锁初始化 + 指针分配
    tvs_down_channel_init();            //互斥锁初始化 + down 线程创建
    tvs_speech_manager_init();          //互斥锁初始化
    tvs_media_player_inner_init();      //互斥锁初始化
    tvs_audio_recorder_thread_init();   //互斥锁初始化
    tvs_audio_track_init();             //无
    //tvs_mp3_player_initialize();        //互斥锁初始化 + mp3 线程初始化
    tvs_executor_service_start();       //互斥锁初始化 + 队列初始化 + executor 线程创建
    tvs_ip_provider_init();             //互斥锁初始化 + 指针分配 + 线程初始化

    TVS_LOG_PRINTF("%s, end, env_type %d, sandbox %d, bitrate %d, channels %d\n", __func__,
                   g_tvs_system_config.def_env, g_tvs_system_config.def_sandbox_open,
                   g_tvs_system_config.recorder_bitrate, g_tvs_system_config.recorder_channels);

    return 0;
}

int tvs_core_start_recognize(int session_id,
                             tvs_api_recognizer_type type, const char *wakeword, int startIndexInSamples, int endIndexInSamples)
{

    tvs_core_set_current_session_id(session_id);

    tvs_speech_manager_config config = {0};
    config.session_id = session_id;
    config.bitrate = tvs_config_get_recorder_bitrate();
    if (wakeword != NULL) {
        int len = strlen(wakeword);
        if (len > 0) {
            int wkupMaxSize = sizeof(config.wakeword) - 1;
            len = len > wkupMaxSize ? wkupMaxSize : len;
            strncpy(config.wakeword, wakeword, len);
        }
    }
    config.type = type;
    config.startIndexInSamples = startIndexInSamples;
    config.endIndexInSamples = endIndexInSamples;
    return tvs_executor_start_speech(&config);
}

int tvs_core_play_control_next()
{
    tvs_executor_param_events ev_param = {0};

    return tvs_executor_start_control(TVS_EXECUTOR_CMD_PLAY_NEXT, &ev_param);
}

int tvs_core_play_control_prev()
{
    tvs_executor_param_events ev_param = {0};

    return tvs_executor_start_control(TVS_EXECUTOR_CMD_PLAY_PREV, &ev_param);
}

int tvs_core_send_semantic(const char *semantic)
{
    if (semantic == NULL) {
        TVS_LOG_PRINTF("%s call error, param is null!!\n", __func__);
        return -1;
    }
    tvs_executor_param_events ev_param = {0};
    ev_param.param = strdup(semantic);

    return tvs_executor_start_control(TVS_EXECUTOR_CMD_SEMANTIC, &ev_param);
}

static void tts_free_func(void *param)
{
    if (param == NULL) {
        return;
    }

    tvs_api_tts_param *tts_param = (tvs_api_tts_param *)param;
    if (tts_param->tts_text != NULL) {
        TVS_FREE(tts_param->tts_text);
    }

    if (tts_param->timbre != NULL) {
        TVS_FREE(tts_param->timbre);
    }

    free(tts_param);
}

int tvs_core_start_text_to_speech(tvs_api_tts_param *tts_param)
{
    if (tts_param == NULL) {
        TVS_LOG_PRINTF("%s, invalid param\n", __func__);
        return TVS_API_ERROR_INVALID_PARAMS;
    }

    if (tts_param->tts_text == NULL || strlen(tts_param->tts_text) == 0) {
        TVS_LOG_PRINTF("%s, invalid tts text\n", __func__);
        return TVS_API_ERROR_INVALID_PARAMS;
    }

    int tts_param_size = sizeof(tvs_api_tts_param);

    tvs_executor_param_events ev_param = {0};

    // 设置释放函数
    ev_param.free_func = tts_free_func;

    // 拷贝一份
    tvs_api_tts_param *dump_param = TVS_MALLOC(tts_param_size);
    if (dump_param == NULL) {
        TVS_LOG_PRINTF("%s, OOM\n", __func__);
        return -1;
    }

    memcpy(dump_param, tts_param, tts_param_size);

    dump_param->tts_text = strdup(tts_param->tts_text);
    dump_param->timbre = (tts_param->timbre == NULL || strlen(tts_param->timbre) == 0) ? NULL : strdup(tts_param->timbre);
    TVS_LOG_PRINTF("start tts, text %s\n", dump_param->tts_text);
    ev_param.free_func_param = dump_param;

    return tvs_executor_start_control(TVS_EXECUTOR_CMD_TTS, &ev_param);
}

char *tvs_core_generate_client_id(const char *tvs_product_id, const char *tvs_dsn)
{
    return tvs_authorize_generate_client_id(tvs_product_id, tvs_dsn);
}

extern void tvs_ping_refresh_start();

void tvs_core_notify_net_state_changed(bool connected)
{
    TVS_LOG_PRINTF("%s, conn %d\n", __func__, connected);

    bool valid = tvs_config_is_network_valid();
    if (valid == connected) {
        TVS_LOG_PRINTF("%s, net work state is not changed\n", __func__);
        return;
    }

    tvs_config_set_network_valid(connected);    //更改全局变量的值为connected
    tvs_ip_provider_on_network_changed(connected);
    if (connected) {
        tvs_authorizer_start_inner();
        tvs_ping_refresh_start();
    }
    tvs_down_channel_notify();
}

int tvs_core_send_push_text(const char *text)
{
    tvs_executor_send_push_text((char *)text);
    return 0;
}

extern void tvs_tools_save_current_volume(int volume);

void tvs_core_notify_volume_changed(int volume)
{

#if 0
    //不使用flash保存音量大小，改为vm保存
    tvs_tools_save_current_volume(volume);
#endif

    tvs_executor_upload_volume(volume);
}

void tvs_core_start_sdk()
{
    tvs_config_set_sdk_running(true);
}

void tvs_core_pause_sdk()
{
    tvs_config_set_sdk_running(false);

    tvs_down_channel_notify();
}

void tvs_core_notify_alert_start_ring(const char *token)
{
    tvs_executor_upload_alert_start((char *)token);
}

void tvs_core_notify_alert_stop_ring(const char *token)
{
    tvs_executor_upload_alert_stop((char *)token);
}

void tvs_core_enable_log(bool enable)
{
    g_log_enable = enable;
}

int tvs_log_print_enable()
{
    return g_log_enable;
}

