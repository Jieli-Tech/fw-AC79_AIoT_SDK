/**
 * Copyright (2017) Baidu Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * File: duerapp_media.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Media module function implementation.
 */


#include "duerapp_media.h"
#include "duerapp_config.h"
#include "lightduer_dcs.h"
#include "server/audio_server.h"

#define VOLUME_MAX (100)
#define VOLUME_MIX (0)
#define VOLUME_INIT (30)

static volatile duer_speak_state_t s_speak_state;
static volatile duer_audio_state_t s_audio_state;
static volatile duer_alarm_state_t s_alarm_state;
static char *s_url;
static int s_seek = -1;
static int s_vol;
static bool s_mute;

extern void JL_duer_media_audio_continue(const char *url);
extern void JL_duer_media_speak_play(const char *url);
extern void JL_duer_media_audio_play(const char *url);
extern void JL_duer_volume_change_notify(int volume);

__attribute__((weak)) int get_app_music_volume(void)
{
    return 100;
}

__attribute__((weak)) int get_app_music_playtime(void)
{
    return 0;
}

void duer_media_init()
{
    s_speak_state = MEDIA_SPEAK_STOP;
    s_audio_state = MEDIA_AUDIO_STOP;
    s_alarm_state = MEDIA_ALARM_STOP;
    s_url = NULL;
    s_seek = 0;
    s_vol = get_app_music_volume();
    s_mute = FALSE;
}

void duer_media_speak_play(const char *url)
{
    s_speak_state = MEDIA_SPEAK_PLAY;
    JL_duer_media_speak_play(url);
}

void duer_media_speak_stop()
{
    s_speak_state = MEDIA_SPEAK_STOP;
}

/*该函数不能被阻塞*/
void duer_media_audio_start(const char *url)
{
    DUER_LOGI("duer_media_audio_start, Audio play state : %d", s_audio_state);

    if (s_url) {
        free(s_url);
        s_url = NULL;
    }

    s_url = (char *)malloc(strlen(url) + 1);
    if (s_url) {
        strcpy(s_url, url);
    } else {
        DUER_LOGE("malloc url failed!");
    }
    s_seek = 0;
    JL_duer_media_audio_play(url);
    s_audio_state = MEDIA_AUDIO_PLAY;
}

void duer_media_audio_resume(const char *url, int offset)
{
    DUER_LOGI("duer_media_audio_resume, Audio seek state : %d", s_audio_state);

    if (MEDIA_AUDIO_PAUSE == s_audio_state) {
        JL_duer_media_audio_continue(NULL);
        s_audio_state = MEDIA_AUDIO_PLAY;
    } else if (MEDIA_AUDIO_STOP == s_audio_state) {
        duer_media_audio_start(url);
    }
}

/*该函数不能被阻塞*/
void duer_media_audio_stop()
{
    s_audio_state = MEDIA_AUDIO_STOP;
    DUER_LOGI("Audio state : stop ");
}

/*该函数不能被阻塞*/
void duer_media_audio_pause()
{
    if (MEDIA_AUDIO_PLAY == s_audio_state) {
        s_audio_state = MEDIA_AUDIO_PAUSE;
        DUER_LOGI("Audio state : pause ");
    }
}

/*该函数不能被阻塞*/
int duer_media_audio_get_position()
{
    s_seek = get_app_music_playtime() * 1000;

    if (!s_seek) {
        s_seek = 500;
    }

    return s_seek;
}

duer_audio_state_t duer_media_audio_state()
{
    return s_audio_state;
}

void duer_media_set_audio_state(duer_audio_state_t state)
{
    s_audio_state = state;
}

void duer_media_set_volume_notify(int volume)
{
    s_vol = volume;
    if (s_vol == 0) {
        s_mute = true;
    } else {
        s_mute = false;
    }
    DUER_LOGD("volume : %d", s_vol);
    if (DUER_OK == duer_dcs_on_volume_changed()) {
        DUER_LOGD("volume change OK");
    }
}

/*把音量调到相对值范围-100~100,不能堵塞*/
void duer_media_volume_change(int volume)
{
    s_vol += volume;
    if (s_vol < VOLUME_MIX) {
        s_vol = 0;
    } else if (s_vol > VOLUME_MAX) {
        s_vol = VOLUME_MAX;
    }

    if (s_vol == 0) {
        s_mute = true;
    } else {
        s_mute = false;
    }

    JL_duer_volume_change_notify(s_vol);

    DUER_LOGI("volume : %d", s_vol);
    if (DUER_OK == duer_dcs_on_volume_changed()) {
        DUER_LOGI("volume change OK");
    }
}

/*把音量调到绝对值范围0~100,不能堵塞*/
void duer_media_set_volume(int volume)
{
    if (volume == 1) {
        volume = 0;
    }
    s_vol = volume;
    if (s_vol == 0) {
        s_mute = true;
    } else {
        s_mute = false;
    }
    DUER_LOGD("volume : %d", s_vol);
    JL_duer_volume_change_notify(s_vol);
    if (DUER_OK == duer_dcs_on_volume_changed()) {
        DUER_LOGD("volume change OK");
    }
}

/*获取设备音量值，不能堵塞*/
int duer_media_get_volume()
{
    return s_vol;
}

/*设置和取消静音，不能堵塞*/
void duer_media_set_mute(bool mute)
{
    if (s_mute == mute) {
        return;
    }
    s_mute = mute;

    if (mute) {
        JL_duer_volume_change_notify(0);
    } else {
        JL_duer_volume_change_notify(60);
    }

    duer_dcs_on_mute();
}

/*获取设备是否静音，不能堵塞*/
bool duer_media_get_mute()
{
    return s_mute;
}

void duer_media_alarm_play(const char *url)
{
    if (MEDIA_ALARM_STOP == s_alarm_state) {
        s_alarm_state = MEDIA_ALARM_PLAY;
        JL_duer_media_audio_play(url);
    }
}

void duer_media_alarm_stop()
{
    if (MEDIA_ALARM_PLAY == s_alarm_state) {
        s_alarm_state = MEDIA_ALARM_STOP;
    }
}
