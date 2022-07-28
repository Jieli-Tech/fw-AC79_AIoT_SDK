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
 * File: duerapp_recorder.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Record module function implementation.
 */

#include "duerapp_config.h"
#include "duerapp_recorder.h"
#include "lightduer_voice.h"
#include "generic/circular_buf.h"
//#include "net_record_api.h"


static duer_rec_state_t s_duer_rec_state = RECORDER_STOP;
static u32 curr_sample_rate;
static u8 record_data_buf[608];

#ifdef DUER_SAVE_RECORD_FILE
#include "fs.h"
static FILE *f_fp = NULL;
#endif

int duer_recorder_send(void)
{
    u32 total_len = 0;
    u32 recv_len;
    u16 send_len = 0;

    if (curr_sample_rate == 8000) {
        send_len = 608;
    } else {
        send_len = 598;
    }

#if 0
    do {
        recv_len = get_net_record_data(record_data_buf, send_len);
        if (!recv_len) {
            break;
        }

        total_len += recv_len;

        printf("data_len:%d\n\n", recv_len);
#ifdef DUER_SAVE_RECORD_FILE
        if (!f_fp) {
            f_fp = fopen(CONFIG_ROOT_PATH"record.pcm", "w+");
        }
        if (f_fp) {
            fwrite(f_fp, record_data_buf, recv_len);
        }
#endif
        duer_voice_send(record_data_buf, recv_len);
    } while (recv_len > 0);
#endif

    return total_len;
}

int duer_recorder_start(u16 sample_rate)
{
    int ret = 0;

    if (RECORDER_STOP != s_duer_rec_state) {
        DUER_LOGI("Recorder Start failed! state:%d", s_duer_rec_state);
        return DUER_ERR_FAILED;
    }

#if 0
    union audio_req r = {0};

    r.format        = "spx";
    r.channel       = 1;
    r.frame_size    = 4480;
    r.sample_rate   = sample_rate;

    ai_server_do_event_req(AI_EVENT_REQ_START_ENC, &r);

#endif

    s_duer_rec_state = RECORDER_START;

    return DUER_OK;
}

int duer_recorder_stop()
{
    int ret = 0;

    if (RECORDER_START != s_duer_rec_state) {
        DUER_LOGI("Recorder Stop failed! state:%d", s_duer_rec_state);
        return DUER_ERR_FAILED;
    }
#if 0
    ret = net_record_api_close();
    if (ret != 0) {
        DUER_LOGI("Recorder Stop failed! state:%d", s_duer_rec_state);
        return DUER_ERR_FAILED;
    }
#endif
    s_duer_rec_state = RECORDER_STOP;

#ifdef DUER_SAVE_RECORD_FILE
    if (f_fp) {
        fclose(f_fp);
        f_fp = NULL;
    }
#endif

    return DUER_OK;
}

duer_rec_state_t duer_get_recorder_state()
{
    return s_duer_rec_state;
}
