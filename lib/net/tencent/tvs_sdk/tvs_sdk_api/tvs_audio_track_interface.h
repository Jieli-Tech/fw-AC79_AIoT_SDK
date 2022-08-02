#ifndef __TVS_TTS_PLAYER_H__
#define __TVS_TTS_PLAYER_H__

typedef enum {
    TVS_MEDIA_TYPE_PCM,
    TVS_MEDIA_TYPE_MP3,
} tvs_media_type;

int tvs_audio_track_open(tvs_media_type type, int bitrate, int channels);

void tvs_audio_track_close();

int tvs_audio_track_write(char *data, int data_size);

void tvs_audio_track_no_more_data();

int tvs_audio_track_init();

#endif