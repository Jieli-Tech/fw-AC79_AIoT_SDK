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
/*
 * Auth: Yiheng Li(liyiheng01@baidu.com)
 * Desc: recorder module log report
 */
#ifndef BAIDU_DUER_LIGHTDUER_DS_LOG_RECORDER_H
#define BAIDU_DUER_LIGHTDUER_DS_LOG_RECORDER_H

#include "lightduer_types.h"
#include "baidu_json.h"

#define REC_LOG_MOD_NAME(name) (#name)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    DUER_DS_LOG_REC_START = 0x001,
    DUER_DS_LOG_REC_STOP = 0x002,
} duer_ds_log_rec_code_t;

/**
 * update compress infomation,set start/end = duer_timestamp() before/after compress functions
 */
void duer_ds_rec_compress_info_update(const duer_u32_t start, const duer_u32_t end);

/**
 * update compress infomation,set start/end = duer_timestamp() before/after compress functions
 */
void duer_ds_rec_delay_info_update(const duer_u32_t request, const duer_u32_t send_start, const duer_size_t send_finish);

/**
 * report the log code DUER_DS_LOG_REC_START
 * {
 *     "id": [session id]
 * }
*/
duer_status_t duer_ds_log_rec_start(duer_u32_t id);

/**
 * report the message for log code DUER_DS_LOG_REC_STOP
 * {
 *     "id": [session id],
 *     "rec_compress" : {
 *         "max" : [maximum consuming time of compressing one frame],
 *         "min" : [minimum consuming time of compressing one frame],
 *         "avg" : [average consuming time of compressing one frame],
 *         "frames" : [total frames of the speech],
 *         "length" : [total length of speech data(before compress)]
 *     }
 *     "speex_compress" : {
 *         "max" : [maximum consuming time of compressing one frame in speex],
 *         "min" : [minimum consuming time of compressing one frame in speex],
 *         "avg" : [average consuming time of compressing one frame in speex],
 *         "frames" : [total frames of speex-compressed-speech],
 *         "length" : [total length of speex-compressed-speech data(after speex compress)]
 *     }
 * }
 */
duer_status_t duer_ds_log_rec_stop(duer_u32_t id);

#ifdef __cplusplus
}
#endif

#endif // BAIDU_DUER_LIGHTDUER_DS_LOG_RECORDER_H

