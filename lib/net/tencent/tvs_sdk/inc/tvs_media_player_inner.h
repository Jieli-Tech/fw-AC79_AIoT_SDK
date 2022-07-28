#ifndef __TVS_MEDIA_PLAYER_INNER_H__
#define __TVS_MEDIA_PLAYER_INNER_H__
#include "tvs_media_player_interface.h"

int tvs_media_player_inner_set_source(const char *url, const char *token, unsigned int offset);

int tvs_media_player_inner_set_source_and_play(const char *url, const char *token, unsigned int offset);

int tvs_media_player_inner_start_play();

int tvs_media_player_inner_get_offset();

int tvs_media_player_inner_pause_play();

int tvs_media_player_inner_stop_play();

void tvs_media_player_inner_init();

void tvs_media_player_inner_on_play_finished(const char *token);

void tvs_media_player_inner_on_play_stopped(int error_code, const char *token);

void tvs_media_player_inner_on_play_started(const char *token);

void tvs_media_player_inner_on_play_paused(const char *token);

char *tvs_media_player_inner_get_token();

char *tvs_media_player_inner_get_url();

const char *tvs_media_player_inner_get_playback_state();

int tvs_media_player_inner_get_playback_offset();

const char *get_player_upload_state(int state);

#endif
