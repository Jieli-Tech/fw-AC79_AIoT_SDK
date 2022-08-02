#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_media_player_interface.h"

#define TVS_LOG_DEBUG_MODULE  "mediaplayer"
#include "tvs_log.h"

#include "tvs_core.h"
#include "tvs_api/tvs_api.h"
#include "tvs_preference.h"
#include "tvs_tts_player.h"


tvs_mediaplayer_adapter g_tvs_mediaplayer_adapter = {0};
extern tvs_platform_adaptor g_tvs_platform_adaptor;

int tvs_media_player_set_source(const char *url, const char *token, unsigned int offset_sec)
{
    if (g_tvs_mediaplayer_adapter.set_source == NULL) {
        TVS_LOG_PRINTF("call %s, invalud adapter\n",  __func__);
        return -1;
    }

    return g_tvs_mediaplayer_adapter.set_source(url, token, offset_sec);
}

int tvs_media_player_start_play(const char *token)
{
    if (g_tvs_mediaplayer_adapter.start_play == NULL) {
        TVS_LOG_PRINTF("call %s, invalud adapter\n",  __func__);
        return -1;
    }
    return g_tvs_mediaplayer_adapter.start_play(token);
}

int tvs_media_player_stop_play(const char *token)
{
    if (g_tvs_mediaplayer_adapter.stop_play == NULL) {
        TVS_LOG_PRINTF("call %s, invalud adapter\n",  __func__);
        return -1;
    }
    return g_tvs_mediaplayer_adapter.stop_play(token);
}

int tvs_media_player_resume_play(const char *token)
{
    if (g_tvs_mediaplayer_adapter.resume_play == NULL) {
        TVS_LOG_PRINTF("call %s, invalud adapter\n",  __func__);
        return -1;
    }
    return g_tvs_mediaplayer_adapter.resume_play(token);
}

int tvs_media_player_pause_play(const char *token)
{
    if (g_tvs_mediaplayer_adapter.pause_play == NULL) {
        TVS_LOG_PRINTF("call %s, invalud adapter\n",  __func__);
        return -1;
    }
    return g_tvs_mediaplayer_adapter.pause_play(token);
}

int tvs_media_player_get_offset(const char *token)
{
    if (g_tvs_mediaplayer_adapter.get_offset == NULL) {
        TVS_LOG_PRINTF("call %s, invalud adapter\n",  __func__);
        return 0;
    }
    return g_tvs_mediaplayer_adapter.get_offset(token);
}

int tvs_tools_get_current_volume()
{
    int value = 80;
    tvs_preference_get_number_value(TVS_PREFERENCE_VOLUME, &value, 80);
    return value;
}

void tvs_tools_save_current_volume(int volume)
{
    tvs_preference_set_number_value(TVS_PREFERENCE_VOLUME, volume);
}

int tvs_media_player_get_volume()
{
    return tvs_tools_get_current_volume();
}

void tvs_media_player_set_volume(int volume)
{
    //tvs_tools_save_current_volume(volume);

    if (g_tvs_platform_adaptor.set_volume != NULL) {
        TVS_LOG_PRINTF("tvs_cloud_volume set to %d\n", volume);
        g_tvs_platform_adaptor.set_volume(volume, 100, false);
    }
}

int tvs_media_player_init()
{
    return 0;
}

void tvs_mediaplayer_adapter_init(tvs_mediaplayer_adapter *adapter)
{
    if (adapter != NULL) {
        memcpy(&g_tvs_mediaplayer_adapter, adapter, sizeof(tvs_mediaplayer_adapter));
    }

    if (g_tvs_mediaplayer_adapter.play_inner != NULL) {
        if (g_tvs_mediaplayer_adapter.play_inner()) {
            tvs_tts_set_dec_in_sdk(true);
        } else {
            tvs_tts_set_dec_in_sdk(false);
        }
    } else {
        tvs_tts_set_dec_in_sdk(true);
    }
}

void tvs_mediaplayer_adapter_on_play_finished(const char *token)
{
    tvs_core_notify_media_player_on_play_finished(token);
}

void tvs_mediaplayer_adapter_on_play_stopped(int error_code, const char *token)
{
    tvs_core_notify_media_player_on_play_stopped(error_code, token);
}

void tvs_mediaplayer_adapter_on_play_started(const char *token)
{
    tvs_core_notify_media_player_on_play_started(token);
}

void tvs_mediaplayer_adapter_on_play_paused(const char *token)
{
    tvs_core_notify_media_player_on_play_paused(token);
}

