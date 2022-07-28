#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "tvs_audio_track_interface.h"
#include "tvs_log.h"
#include "tvs_core.h"
#include "tvs_api/tvs_api.h"


#define TVS_EMPTY_TRACK   0
extern tvs_platform_adaptor g_tvs_platform_adaptor;

extern tvs_mediaplayer_adapter g_tvs_mediaplayer_adapter;
static tvs_media_type g_start_type = 0;

static bool g_start_play = false;

static int g_speech_end_time = 0;

static int g_tts_size = 0;

void tvs_speech_set_end_time(int time)
{
    g_speech_end_time = time;
}

static int start_track(tvs_media_type type, int bitrate, int channels)
{
    g_start_type = type;
    if (bitrate == 0) {
        bitrate = 16000;
    }

    if (channels == 0) {
        channels = 1;
    }

    tvs_soundcard_config config = {0};
    config.bitrate = bitrate;
    config.channel = channels;

    g_start_play = false;

    if (TVS_MEDIA_TYPE_PCM == g_start_type) {
        if (g_tvs_platform_adaptor.soundcard_control != NULL) {
            return g_tvs_platform_adaptor.soundcard_control(true, false, &config);
        }
    } else {
        g_tts_size = 0;
        if (g_tvs_mediaplayer_adapter.tts_start != NULL) {
            return g_tvs_mediaplayer_adapter.tts_start(1);
        }
    }

    return -1;
}

static void stop_track(bool force_close)
{
    g_start_play = false;

    if (TVS_MEDIA_TYPE_PCM == g_start_type) {
        if (g_tvs_platform_adaptor.soundcard_control != NULL) {
            g_tvs_platform_adaptor.soundcard_control(false, false, NULL);
        }
    } else {
        g_tts_size = 0;
        if (g_tvs_mediaplayer_adapter.tts_end != NULL) {
            g_tvs_mediaplayer_adapter.tts_end(force_close);
        }
    }
}

static int write_track(char *data, int data_size)
{
    if (TVS_MEDIA_TYPE_PCM == g_start_type) {
        if (g_tvs_platform_adaptor.soundcard_pcm_write != NULL) {
            return g_tvs_platform_adaptor.soundcard_pcm_write(data, data_size);
        }
    } else {
        g_tts_size += data_size;

        if (!g_start_play && g_tts_size > 4 * 1024) {
            g_start_play = true;

            TVS_LOG_ERROR("tts start from %d bytes, cost time %ld ms after VAD End\n", g_tts_size, os_wrapper_get_time_ms() - g_speech_end_time);
        }
        if (g_tvs_mediaplayer_adapter.tts_data != NULL) {
            return g_tvs_mediaplayer_adapter.tts_data(data, data_size);
        }
    }

    return -1;
}

int tvs_audio_track_open(tvs_media_type type, int bitrate, int channels)
{
    if (TVS_EMPTY_TRACK) {
        return 0;
    }

    return start_track(type, bitrate, channels);
}

void tvs_audio_track_close()
{
    if (TVS_EMPTY_TRACK) {
        return;
    }

    stop_track(true);
}

void tvs_audio_track_no_more_data()
{
    if (TVS_EMPTY_TRACK) {
        return;
    }

    stop_track(false);
}

int tvs_audio_track_write(char *data, int data_size)
{
    if (TVS_EMPTY_TRACK) {
        return data_size;
    }

    return write_track(data, data_size);
}

int tvs_audio_track_init()
{
    return 0;
}
