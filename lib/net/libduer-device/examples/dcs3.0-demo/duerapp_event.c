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
 * File: duerapp_event.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Duer Application Key functions to achieve.
 */

#include <string.h>

#include "duerapp_config.h"
#include "duerapp_event.h"
#include "lightduer_dcs.h"
#include "lightduer_voice.h"
#include "duerapp_recorder.h"
#include "duerapp_media.h"
#include "os/os_api.h"
#include "duerapp_alert.h"
#include "duerapp.h"
#include "lightduer_connagent.h"
#include "lightduer_sleep.h"
#include "lightduer_event_emitter.h"
#include "baidu_ca_adapter_internal.h"
#include "lightduer_bind_device.h"
#ifdef	BREAK_DNS_QUERY
#include "lwip/netdb.h"
#endif
#include "server/ai_server.h"

#define VOLUME_STEP (5)


static void event_play_puase()
{
    DUER_LOGV("KEY_DOWN");
    if (duer_alert_bell()) {
        duer_alert_stop();
        return;
    }
    if (DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode()) {
        duer_audio_state_t audio_state = duer_media_audio_state();
        if (MEDIA_AUDIO_PLAY == audio_state) {
            duer_dcs_send_play_control_cmd(DCS_PAUSE_CMD);
            duer_media_set_audio_state(MEDIA_AUDIO_PAUSE);
        } else if (MEDIA_AUDIO_PAUSE == audio_state) {
            duer_dcs_send_play_control_cmd(DCS_PLAY_CMD);
            duer_media_set_audio_state(MEDIA_AUDIO_PLAY);
        } else {
            // do nothing
        }
    }
}

static void event_record_start(int sample_rate, u8 voice_mode)
{
    DUER_LOGV("KEY_DOWN");

    duer_media_speak_stop();
    duer_media_audio_stop();
    duer_alert_stop();

#ifdef DUER_USE_VAD
    if (DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode() || DUER_VOICE_MODE_C2E_BOT == duer_voice_get_mode()) {
        if (DUER_OK != duer_recorder_start(sample_rate, voice_mode)) {
            DUER_LOGE("duer recorder start failed!");
        }
    } else {
        duer_voice_start(sample_rate);
        if (DUER_OK != duer_recorder_start(sample_rate, voice_mode)) {
            DUER_LOGE("duer recorder start failed!");
        }
    }
#else
    duer_voice_start(sample_rate);
    if (DUER_OK != duer_recorder_start(sample_rate, voice_mode)) {
        DUER_LOGE("duer recorder start failed!");
    }
#endif
}

static void event_record_stop()
{
    duer_recorder_stop();
    duer_voice_stop(NULL, 0);
    if (DUER_VOICE_MODE_DEFAULT != duer_voice_get_mode()) {
        duer_dcs_on_listen_stopped();
    }
    DUER_LOGI("Listen stop");
}

static void event_previous_song()
{
    DUER_LOGV("KEY_DOWN");
    if (DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode()) {
        duer_dcs_send_play_control_cmd(DCS_PREVIOUS_CMD);
    }
}

static void event_next_song()
{
    DUER_LOGV("KEY_DOWN");
    if (DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode()) {
        duer_dcs_send_play_control_cmd(DCS_NEXT_CMD);
    }
}

static void event_volume_incr()
{
    DUER_LOGV("KEY_DOWN");
    duer_media_volume_change(VOLUME_STEP);
}

static void event_volume_decr()
{
    DUER_LOGV("KEY_DOWN");
    duer_media_volume_change(-VOLUME_STEP);
}

static void event_volune_mute()
{
    static bool mute = false;
    DUER_LOGI("KEY_DOWN : %d", mute);
    if (mute) {
        mute = false;
    } else {
        mute = true;
    }
    duer_media_set_mute(mute);
}

static void event_voice_mode(enum _duer_voice_mode_enum mode)
{
    if (duer_voice_get_mode() == mode) {
        return;
    }
    if (DUER_VOICE_MODE_DEFAULT != duer_voice_get_mode()) {
        duer_dcs_close_multi_dialog();
    }
    duer_media_speak_stop();
    if (MEDIA_AUDIO_PLAY == duer_media_audio_state()) {
        duer_media_audio_stop();
        duer_dcs_audio_on_stopped();
    }
    duer_voice_set_mode(mode);
    DUER_LOGI("Current speech interaction mode: %d", mode);
}

static enum _duer_voice_mode_enum voicemode2duer(int arg)
{
    if (TRANSLATE_MODE == arg) {
        return DUER_VOICE_MODE_C2E_BOT;
    } else if (WECHAT_MODE == arg) {
        return DUER_VOICE_MODE_WCHAT;
    }

    return DUER_VOICE_MODE_DEFAULT;
}

void duer_event_loop(void)
{
    int msg[32];
    int err;
    bool s_is_record = false;

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        switch (msg[1]) {
        case DUER_RECORD_SEND:
            if (s_is_record) {
                if (duer_app_get_connect_state()) {
                    duer_voice_send((void *)msg[2], msg[3]);
                }
                os_sem_post((OS_SEM *)msg[4]);
            }
            break;
        case DUER_RECORD_STOP:
            if (s_is_record) {
#ifdef DUER_USE_VAD
                if ((DUER_VOICE_MODE_DEFAULT == duer_voice_get_mode() ||
                     DUER_VOICE_MODE_C2E_BOT == duer_voice_get_mode()) && 0 == duer_get_recorder_vad_status()) {
                    duer_recorder_stop();
                } else {
                    event_record_stop();
                }
#else
                event_record_stop();
#endif
                s_is_record = false;
                duer_voice_set_mode(DUER_VOICE_MODE_DEFAULT);
            }
            if (msg[2]) {
                os_sem_post((OS_SEM *)msg[2]);
            }
            break;
        case DUER_QUIT:
            if (s_is_record) {
                duer_recorder_stop();
                s_is_record = false;
            }
            bcasoc_break_connect();
            puts("DUER_QUIT\n");
            duer_stop();
            puts("duer_stop\n");
            while (duer_app_get_connect_state()) {
                duer_sleep(10);
            }
            puts("duer_stop: 1\n");
            duer_finalize();
            puts("duer_stop: 2\n");
            while (!duer_check_emitter_if_destroy()) {
                duer_sleep(50);
            }
            puts("duer_stop: 3\n");
            duer_emitter_destroy();
            puts("duer_stop: 4\n");
            bcasoc_uninitialize();
            puts("duer_stop: 5\n");
            duer_voice_set_mode(DUER_VOICE_MODE_DEFAULT);
            return;
        }

        if (duer_app_get_connect_state() && !s_is_record) {
            switch (msg[1]) {
            case DUER_SPEAK_END:
                puts("===========DUER_SPEAK_END\n");
                duer_dcs_speech_on_finished();
                break;
            case DUER_MEDIA_END:
                puts("===========DUER_MEDIA_END\n");
                duer_media_audio_stop();
                duer_dcs_audio_on_finished();
                break;
            case DUER_MEDIA_STOP:
                puts("===========DUER_MEDIA_STOP\n");
                duer_media_audio_stop();
                duer_dcs_audio_on_stopped();
                break;
            case DUER_PLAY_PAUSE:
                event_play_puase();
                break;
            case DUER_RECORD_START:
                event_voice_mode(voicemode2duer(msg[2] & 0x03));
                event_record_start(DUER_SAMPLE_RATE, msg[2]);
                s_is_record = true;
                break;
            case DUER_PREVIOUS_SONG:
                puts("===========DUER_PREVIOUS_SONG\n");
                event_previous_song();
                break;
            case DUER_NEXT_SONG:
                puts("===========DUER_NEXT_SONG\n");
                event_next_song();
                break;
            case DUER_VOLUME_INCR:
                event_volume_incr();
                break;
            case DUER_VOLUME_DECR:
                event_volume_decr();
                break;
            case DUER_VOLUME_MUTE:
                event_volune_mute();
                break;
            case DUER_VOLUME_CHANGE:
                duer_media_set_volume_notify(msg[2]);
                break;
            case DUER_BIND_DEVICE:
                duer_start_bind_device_task(msg[2]);
                break;
            case DUER_COLLECT_RES:
                if (msg[2]) {
                    /* duer_dcs_recommend_request(0); */
                }
                break;
            default:
                break;
            }
        }
    }
}




