
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_audio_recorder_interface.h"

#include "tvs_audio_recorder_thread.h"

#include "tvs_data_buffer.h"
#include "tvs_threads.h"

#include "tvs_audio_provider.h"
#include "tvs_log.h"

#include "tvs_audio_provider.h"
#include "tvs_core.h"
#include "tvs_api/tvs_api.h"

extern tvs_platform_adaptor g_tvs_platform_adaptor;

static bool g_recorder_begin = false;

static int rec_start(int bitrate, int channels, int period_size, int period_count, int session_id)
{
    int ret = -1;
    if (g_tvs_platform_adaptor.soundcard_control != NULL) {
        tvs_soundcard_config config = {0};
        config.bitrate = bitrate;
        config.channel = channels;
        ret = g_tvs_platform_adaptor.soundcard_control(true, true, &config);
    }

    return ret;
}

int tvs_audio_recorder_open(int bitrate, int channels, int period_size, int period_count, int session_id)
{
    if (g_recorder_begin) {
        return 0;
    }

    int ret = rec_start(bitrate, channels, period_size, period_count, session_id);
    if (ret == 0) {
        g_recorder_begin = true;
    }

    return 0;
}

void tvs_audio_recorder_close(int error, int session_id)
{
    if (!g_recorder_begin) {
        return;
    }

    g_recorder_begin = false;
    if (g_tvs_platform_adaptor.soundcard_control != NULL) {
        g_tvs_platform_adaptor.soundcard_control(false, true, NULL);
    }
}

int tvs_audio_recorder_read(char *recorder_buffer, int buffer_size, int session_id)
{
    if (!g_recorder_begin) {
        return -1;
    }
    int ret = 0;

    if (g_tvs_platform_adaptor.soundcard_pcm_read != NULL) {
        ret = g_tvs_platform_adaptor.soundcard_pcm_read(recorder_buffer, buffer_size);
    }

    return ret;
}

int tvs_audio_recorder_init()
{
    return 0;
}
