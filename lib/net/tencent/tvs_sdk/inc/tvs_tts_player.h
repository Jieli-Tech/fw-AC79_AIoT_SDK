
#ifndef __TVS_TTS_PLAYER__
#define __TVS_TTS_PLAYER__

int tvs_tts_player_open();

void tvs_tts_player_close();

void tvs_tts_player_no_more_data();

int tvs_tts_player_write(char *data, int data_size);

void tvs_tts_set_dec_in_sdk(bool in_sdk);

#endif
