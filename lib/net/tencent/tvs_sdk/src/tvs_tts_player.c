
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define TVS_LOG_DEBUG_MODULE  "TTS"
#include "tvs_log.h"
#include "tvs_tts_player.h"
#include "tvs_audio_track_interface.h"

static bool g_tts_open = false;

int tvs_tts_player_open()
{
    if (g_tts_open) {
        return 0;
    }
    g_tts_open = true;

    return tvs_audio_track_open(TVS_MEDIA_TYPE_MP3, 0, 0);
}

void tvs_tts_player_close()
{
    if (!g_tts_open) {
        return;
    }

    g_tts_open = false;
    tvs_audio_track_close();
}

void tvs_tts_player_no_more_data()
{
    if (!g_tts_open) {
        return;
    }

    tvs_audio_track_no_more_data();
}

int tvs_tts_player_write(char *data, int data_size)
{
    if (!g_tts_open) {
        return -1;
    }
    return tvs_audio_track_write(data, data_size);
}

void tvs_tts_set_dec_in_sdk(bool in_sdk)
{
}

