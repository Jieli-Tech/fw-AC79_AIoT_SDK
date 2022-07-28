#ifndef __TVS_MP3_PLAYER__
#define __TVS_MP3_PLAYER__

int tvs_mp3_player_start();

void tvs_mp3_player_stop(bool stop_now);

int tvs_mp3_player_fill(const char *data, int data_size);

int tvs_mp3_player_initialize();
#endif