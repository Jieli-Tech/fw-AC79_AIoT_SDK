/**
* @file  tvs_directives_handler.c
* @brief 处理下行通道/event通道下发的指令
* @date  2019-5-10
*
*/

#define TVS_LOG_DEBUG   1
#include "tvs_log.h"
#include "tvs_jsons.h"
#include "tvs_directives_handler.h"
#include "tvs_executor_service.h"
#include "tvs_media_player_inner.h"
#include "tvs_speech_manager.h"
#include "tvs_core.h"
#include "tvs_system_interface.h"
#include "tvs_api/tvs_api.h"
#include "tvs_api_config.h"


extern tvs_api_callback g_tvs_core_callback;

void on_directives_set_volume(tvs_directives_infos *directives)
{
    int max_volume = 100;

    if (directives->volume > max_volume) {
        directives->volume = max_volume;
    } else if (directives->volume < 0) {
        directives->volume = 0;
    }

    tvs_media_player_set_volume(directives->volume);

    /* tvs_executor_upload_volume(directives->volume); */
}

void on_directives_adjust_volume(tvs_directives_infos *directives)
{
    int max_volume = 100;
    int volume = tvs_media_player_get_volume();
    volume += directives->volume;
    if (volume < 0) {
        volume = 0;
    } else if (volume > max_volume) {
        volume = max_volume;
    }

    tvs_media_player_set_volume(volume);
    /* tvs_executor_upload_volume(volume); */
}

extern int tvs_executor_start_play();

void on_new_media_source(tvs_directives_infos *directives, bool down_channel, tvs_directives_params *params)
{
    tvs_media_player_inner_set_source(directives->play_url, directives->token, directives->play_offset);
    if (down_channel) {
        tvs_executor_start_play();
    }

    if (directives->play_url != NULL) {
        params->has_media = true;
    }
}

void on_recv_media_stop(tvs_directives_infos *directives)
{
    tvs_media_player_inner_stop_play(directives->token);
}

static void on_expect_speech(tvs_directives_params *params, bool down_channel)
{
    if (params != NULL) {
        params->expect_speech = 1;
    }
}
void on_recv_switch_mode(tvs_directives_infos *directives)
{
    if (directives->src_mode == NULL || directives->dst_mode == NULL) {
        return;
    }
    tvs_system_set_current_mode(directives->dst_mode);
    tvs_executor_upload_mode_changed(directives->src_mode, directives->dst_mode);
}

static void process_semantic(tvs_directives_infos *directives)
{
    TVS_LOG_PRINTF("try to send senmantic: %s\n", directives->semantic);

    tvs_core_send_semantic(directives->semantic);
}

static void tvs_on_get_terminal_sync(tvs_directives_infos *directives)
{
    TVS_LOG_PRINTF("terminal sync %s - %s\n", directives->semantic, directives->token);
    if (g_tvs_core_callback.on_terminal_sync != NULL) {
        g_tvs_core_callback.on_terminal_sync(directives->semantic, directives->token);
    }
}

static void tvs_on_get_tvs_control(tvs_directives_infos *directives)
{
    //TVS_LOG_PRINTF("tvs control %s\n", directives->semantic);
    if (g_tvs_core_callback.on_recv_tvs_control != NULL) {
        g_tvs_core_callback.on_recv_tvs_control(directives->semantic);
    }
}

static void on_directives_unbind(tvs_directives_infos *directives)
{
    if (g_tvs_core_callback.on_unbind != NULL) {
        g_tvs_core_callback.on_unbind(directives->unbind_times);
    }
}


void tvs_directives_handle(tvs_directives_infos *directives, bool down_channel, tvs_directives_params *params)
{
    if (NULL == directives) {
        return;
    }

    switch (directives->type) {
    case TVS_DIRECTIVES_TYPE_AUDIO_PLAY:
        on_new_media_source(directives, down_channel, params);
        break;
    case TVS_DIRECTIVES_TYPE_STOP_CAPTURE:
        tvs_speech_manager_stop_capture(directives->dialog_id);
        break;
    case TVS_DIRECTIVES_TYPE_EXPECT_SPEECH:
        on_expect_speech(params, down_channel);
        break;
    case TVS_DIRECTIVES_TYPE_AUDIO_PLAY_STOP:
        on_recv_media_stop(directives);
        break;
    case TVS_DIRECTIVES_TYPE_SET_VOLUME:
        on_directives_set_volume(directives);
        break;
    case TVS_DIRECTIVES_TYPE_ADJUST_VOLUME:
        on_directives_adjust_volume(directives);
        break;
    case TVS_DIRECTIVES_TYPE_SWITCH_MODE:
        on_recv_switch_mode(directives);
        break;
    case TVS_DIRECTIVES_TYPE_SEMANTIC:
        if (down_channel) {
            process_semantic(directives);
        }
        break;
    case TVS_DIRECTIVES_TYPE_TERMINALSYNC:
        if (down_channel) {
            tvs_on_get_terminal_sync(directives);
        }
        break;
    case TVS_DIRECTIVES_TYPE_TVS_CONTROL:
        tvs_on_get_tvs_control(directives);
        break;
    case TVS_DIRECTIVES_TYPE_UNBIND:
        on_directives_unbind(directives);
        break;
    default:
        break;
    }
}


void tvs_directives_parse_metadata(const char *name, const char *metadata,  int metadata_len, bool down_channel, tvs_directives_params *params)
{

#if ARREARS_ENABLE
    audio_play_disable = 0;		//关闭tts播放标志清零
#endif
    on_recv_tvs_directives(metadata, down_channel, params);
}


