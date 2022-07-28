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
 * File: duerapp.c
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Duer Application Main.
 */

#include <string.h>
#include <stdlib.h>
#include "printf.h"
#include "typedef.h"

#include "duerapp_config.h"
#include "lightduer_connagent.h"
#include "lightduer_voice.h"
#include "lightduer_dcs.h"
#include "lightduer_timestamp.h"
#include "lightduer_dcs_alert.h"
#include "lightduer_memory.h"
#include "lightduer_timers.h"
#include "duerapp_recorder.h"
#include "duerapp_media.h"
#include "duerapp_event.h"
#include "duerapp_alert.h"
#include "duerapp.h"
#include "os/os_api.h"
#include "lightduer_system_info.h"
#include "lightduer_bind_device.h"
#include "net_timer.h"

duer_system_static_info_t g_system_static_info = {
    .os_version         = "FreeRTOS",
    .sw_version         = "DCS3.0",
    .brand              = "Baidu",
    .hardware_version   = "1.0.0",
    .equipment_type     = "AC521x",
    .ram_size           = 2 * 1024,
    .rom_size           = 4 * 1024,
};

void set_duer_system_static_info(const char *os_version,
                                 const char *sw_version,
                                 const char *brand,
                                 const char *hardware_version,
                                 const char *equipment_type,
                                 u32 ram_KB,
                                 u32 rom_KB)
{
    strncpy(g_system_static_info.os_version, os_version, OS_VERSION_LEN);
    g_system_static_info.os_version[OS_VERSION_LEN] = 0;
    strncpy(g_system_static_info.sw_version, sw_version, SW_VERSION_LEN);
    g_system_static_info.os_version[SW_VERSION_LEN] = 0;
    strncpy(g_system_static_info.brand, brand, BRAND_LEN);
    g_system_static_info.os_version[BRAND_LEN] = 0;
    strncpy(g_system_static_info.hardware_version, hardware_version, HARDWARE_VERSION_LEN);
    g_system_static_info.os_version[HARDWARE_VERSION_LEN] = 0;
    strncpy(g_system_static_info.equipment_type, equipment_type, EQUIPMENT_TYPE_LEN);
    g_system_static_info.os_version[EQUIPMENT_TYPE_LEN] = 0;
    g_system_static_info.ram_size = ram_KB;
    g_system_static_info.rom_size = rom_KB;
}

__attribute__((weak)) int ai_platform_if_support_poweron_recommend(void)
{
    return 1;
}

static duer_bool s_is_initialized;
static duer_bool s_is_binded;
static volatile duer_bool s_started = false;
static duer_bool duer_app_init_flag = false;
static duer_timer_handler g_duer_timer = NULL;
static duer_u16_t g_duer_start_timeout = DUER_RESTART_TIMEOUT;
static duer_bool s_is_not_first;
static int duer_app_task_pid;

static duer_status_t duer_app_test_control_point(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    DUER_LOGI("duer_app_test_control_point");

    if (msg) {
        duer_response(msg, DUER_MSG_RSP_INVALID, NULL, 0);
    } else {
        return DUER_ERR_FAILED;
    }
    printf("token:0x");
    for (int i = 0; i < msg->token_len; ++i) {
        printf("%x", msg->token[i]);
    }
    printf("\n");
    duer_u8_t *cachedToken = DUER_MALLOC(msg->token_len);
    DUER_MEMCPY(cachedToken, msg->token, msg->token_len);
    printf("token:0x");
    for (int i = 0; i < msg->token_len; ++i) {
        printf("%x", cachedToken[i]);
    }
    printf("\n");

    baidu_json *payload = baidu_json_CreateObject();
    baidu_json_AddStringToObject(payload, "result", "OK");
    baidu_json_AddNumberToObject(payload, "timestamp", duer_timestamp());

    duer_seperate_response((const char *)cachedToken, msg->token_len, DUER_MSG_RSP_CONTENT, payload);

    if (payload) {
        baidu_json_Delete(payload);
    }
    DUER_FREE(cachedToken);
    return DUER_OK;
}

static void duer_app_test_control_point_init()
{
    duer_res_t res[] = {
        {
            DUER_RES_MODE_DYNAMIC,
            DUER_RES_OP_PUT,
            "duer_app_test_cp",
            .res.f_res = duer_app_test_control_point
        },

    };

    duer_add_resources(res, sizeof(res) / sizeof(res[0]));
}

static int duer_dcs_recommend_request_iftt(void)
{
    baidu_json *data = NULL;

    data = baidu_json_CreateObject();
    if (!data) {
        DUER_LOGE("Memory not enough");
        return DUER_ERR_FAILED;
    }

    baidu_json_AddNumberToObject(data, "dueros_boot_remind_dcs", 3);
    duer_data_report(data);
    baidu_json_Delete(data);

    return DUER_OK;
}

#ifdef DUER_WECHAT_SUPPORT

#include "lightduer_dcs_local.h"

#define DCS_WECHAT_SPEAK_OUTPUT_NAME "Speak"
#define DCS_WECHAT_NAMESPACE	"ai.dueros.device_interface.iot_cloud.wechat"

extern void duer_wechat_speak_play(const char *url);

static void duer_dcs_wechat_speak_handler(const char *url)
{
    duer_wechat_speak_play(url);
}

static duer_status_t duer_wechat_speak_output_cb(const baidu_json *directive)
{
    baidu_json *payload = NULL;
    baidu_json *url = NULL;
    duer_status_t ret = DUER_OK;

    payload = baidu_json_GetObjectItem(directive, DCS_PAYLOAD_KEY);
    if (!payload) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    url = baidu_json_GetObjectItem(payload, DCS_URL_KEY);
    if (!url) {
        return DUER_MSG_RSP_BAD_REQUEST;
    }

    duer_dcs_wechat_speak_handler(url->valuestring);

    return ret;
}

static void duer_dcs_wechat_init(void)
{
    DUER_DCS_CRITICAL_ENTER();

    if (s_is_initialized) {
        DUER_DCS_CRITICAL_EXIT();
        return;
    }

    duer_directive_list res[] = {
        {DCS_WECHAT_SPEAK_OUTPUT_NAME, duer_wechat_speak_output_cb},
    };

    duer_add_dcs_directive_internal(res, sizeof(res) / sizeof(res[0]), DCS_WECHAT_NAMESPACE);
    s_is_initialized = DUER_TRUE;
    DUER_DCS_CRITICAL_EXIT();
}

static void duer_dcs_wechat_uninitialize(void)
{
    s_is_initialized = DUER_FALSE;
}
#endif

static void duer_app_dcs_init()
{
    duer_dcs_framework_init();
    duer_dcs_voice_input_init();
    duer_dcs_voice_output_init();
    duer_dcs_speaker_control_init();
    duer_dcs_audio_player_init();
    duer_dcs_screen_init();
    duer_dcs_alert_init();
#ifdef DUER_WECHAT_SUPPORT
    duer_dcs_wechat_init();
#endif
    duer_dcs_sync_state();

    duer_app_test_control_point_init();

#ifdef OTA_REG_UPDATE
    duer_app_ota_init();
#endif
    /* duer_my_dcs_init(); */

#ifdef DUER_WECHAT_SUPPORT

#ifdef DUER_HTTPS_SUPPORT
    duer_dcs_capability_declare(DCS_TTS_HTTPS_PROTOCAL_SUPPORTED | DCS_WECHAT_SUPPORTED);
#else
    duer_dcs_capability_declare(DCS_WECHAT_SUPPORTED);
#endif

    /* duer_dcs_recommend_request(0); */
#endif

    if (!s_is_not_first) {
        if (ai_platform_if_support_poweron_recommend()) {
            duer_dcs_recommend_request_iftt();
        }
        s_is_not_first = DUER_TRUE;
    }
}

static void duer_app_dcs_uninitialize()
{
    duer_dcs_uninitialize();
#ifdef DUER_WECHAT_SUPPORT
    duer_dcs_wechat_uninitialize();
#endif
}

static void duer_test_start(const char *profile)
{
    const char *data = duer_load_profile(profile);
    if (data == NULL) {
        DUER_LOGE("load profile failed!");
        if (g_duer_timer != NULL) {
            duer_timer_stop(g_duer_timer);
            g_duer_start_timeout = DUER_RESTART_TIMEOUT * 2;
            duer_timer_start(g_duer_timer, g_duer_start_timeout);
        }
        return;
    }

    DUER_LOGD("profile: \n%s", data);

    // We start the duer with duer_start(), when network connected.
    duer_start(data, strlen(data));

#ifndef LOAD_PROFILE_BY_FLASH
    DUER_FREE((void *)data);
#endif

#ifdef DUER_WECHAT_SUPPORT
    if (s_is_binded == DUER_FALSE) {
        s_is_binded = DUER_TRUE;
        duer_start_bind_device_task(90);
    }
#endif
}

static void duer_app_timer_expired(void *param)
{
    int msg[3];
    msg[0] = (int)duer_test_start;
    msg[1] = 1;
    msg[2] = (int)DUER_PROFILE_PATH;;
    if (os_taskq_post_type("duer_app_task", Q_CALLBACK, 3, msg)) {
        DUER_LOGE("duer_test_start restart os_taskq_post error\n");
    }
}

static void duer_event_hook(duer_event_t *event)
{
    if (!event) {
        DUER_LOGE("NULL event!!!");
    }

    DUER_LOGD("event: %d", event->_event);
    switch (event->_event) {
    case DUER_EVENT_STARTED:
        duer_app_dcs_init();
        s_started = true;
        if (g_duer_timer != NULL) {
            duer_timer_stop(g_duer_timer);
            g_duer_start_timeout = DUER_RESTART_TIMEOUT;
        }
        break;
    case DUER_EVENT_STOPPED:
        s_started = false;
        if (duer_app_init_flag) {
            if (g_duer_timer != NULL) {
                DUER_LOGI("timerout:%d \n", g_duer_start_timeout);
                duer_timer_stop(g_duer_timer);
                duer_timer_start(g_duer_timer, g_duer_start_timeout);
                if (g_duer_start_timeout < 1000 * 8) {
                    g_duer_start_timeout <<= 1;
                }
            }
        }
        break;
    default:
        break;
    }
}

static void duer_app_task(void *priv)
{
    s_is_binded = DUER_FALSE;

    //init timer task
    net_timer_init();

    // init CA
    duer_initialize();

    // Set the Duer Event Callback
    duer_set_event_callback(duer_event_hook);

    //init recorder
    /* duer_recorder_init(); */

    //init alarm
    /* duer_init_alarm(); */

    // init media
    duer_media_init();

    if (g_duer_timer == NULL) {
        g_duer_timer = duer_timer_acquire(duer_app_timer_expired, NULL, DUER_TIMER_ONCE);
    }

    // try conntect baidu cloud
    duer_test_start(DUER_PROFILE_PATH);

    duer_event_loop();

#ifdef OTA_REG_UPDATE
    duer_app_ota_uninitialize();
#endif

    duer_stop_bind_device_task();

    net_timer_uninit();

    DUER_LOGI("DUER OS GOOD BYE !!!");

    s_is_not_first = DUER_FALSE;
}

void duer_dcs_speak_handler(const char *url)
{
    if (DUER_VOICE_MODE_DEFAULT != duer_voice_get_mode()
        && RECORDER_START == duer_get_recorder_state()) {
        duer_dcs_stop_listen_handler();
    }

    duer_media_speak_play(url);
    DUER_LOGI("SPEAK url:%s", url);
}

void duer_dcs_audio_play_handler(const duer_dcs_audio_info_t *audio_info)
{
    DUER_LOGI("Audio paly offset:%d url:%s", audio_info->offset, audio_info->url);
    if (audio_info->offset && audio_info->offset != 1) {
        duer_media_audio_resume(audio_info->url, audio_info->offset);
    } else {
        duer_media_audio_start(audio_info->url);
    }
}

void duer_dcs_get_speaker_state(int *volume, duer_bool *is_mute)
{
    if (volume) {
        *volume = duer_media_get_volume();
    }
    if (is_mute) {
        *is_mute = duer_media_get_mute();
    }
}

void duer_dcs_volume_set_handler(int volume)
{
    duer_media_set_volume(volume);
}

void duer_dcs_volume_adjust_handler(int volume)
{
    duer_media_volume_change(volume);
}

void duer_dcs_mute_handler(duer_bool is_mute)
{
    duer_media_set_mute(is_mute);
}

void duer_dcs_audio_stop_handler(void)
{
    if (MEDIA_AUDIO_PLAY == duer_media_audio_state()) {
        extern void JL_duer_media_audio_pause(const char *url);
        JL_duer_media_audio_pause(NULL);
        duer_media_audio_pause();
    }
}

void duer_dcs_audio_resume_handler(const duer_dcs_audio_info_t *audio_info)
{
    duer_media_audio_resume(audio_info->url, audio_info->offset);
    DUER_LOGI("Audio resume offset:%d url:%s", audio_info->offset, audio_info->url);
}

void duer_dcs_audio_pause_handler(void)
{
    duer_media_audio_pause();
}

int duer_dcs_audio_get_play_progress(void)
{
    int pos = duer_media_audio_get_position();
    DUER_LOGI("PAUSE offset : %d", pos);
    return pos;
}

void duer_dcs_listen_handler(void)
{
    do {
        if (OS_Q_FULL != os_taskq_post("duer_app_task", 2, DUER_RECORD_START, 0)) {
            break;
        }
        DUER_LOGW("duer_app_task send msg DUER_RECORD_START timeout ");
        msleep(20);
    } while (1);
}

void duer_dcs_stop_listen_handler(void)
{
    OS_SEM sem;

    os_sem_create(&sem, 0);

    do {
        if (OS_Q_FULL != os_taskq_post("duer_app_task", 2, DUER_RECORD_STOP, (int)&sem)) {
            break;
        }
        DUER_LOGW("duer_app_task send msg DUER_RECORD_STOP timeout ");
        msleep(20);
    } while (1);

    os_sem_pend(&sem, 0);	//等待麦克风关闭
    os_sem_del(&sem, 1);
}

bool duer_app_get_connect_state()
{
    return s_started;
}

duer_status_t duer_dcs_render_card_handler(baidu_json *payload)
{
    baidu_json *type = NULL;
    baidu_json *content = NULL;
    duer_status_t ret = DUER_OK;

    do {
        if (!payload) {
            ret = DUER_ERR_FAILED;
            break;
        }

        type = baidu_json_GetObjectItem(payload, "type");
        if (!type) {
            ret = DUER_ERR_FAILED;
            break;
        }

        if (strcmp("TextCard", type->valuestring) == 0) {
            content = baidu_json_GetObjectItem(payload, "content");
            if (!content) {
                ret = DUER_ERR_FAILED;
                break;
            }
            DUER_LOGI("Render card content: %s", content->valuestring);
        }
    } while (0);

    return ret;
}

duer_status_t duer_dcs_input_text_handler(const char *text, const char *type)
{
    DUER_LOGI("ASR result: %s", text);
    return DUER_OK;
}

void duer_dcs_stop_speak_handler(void)
{
    DUER_LOGI("stop speak");
    duer_media_speak_stop();
}

int duer_app_init(void)
{
    if (!duer_app_init_flag) {
        duer_app_init_flag = 1;
        return thread_fork("duer_app_task", 26, 768, 128, &duer_app_task_pid, duer_app_task, NULL);
    }
    return -1;
}

void duer_app_uninit(void)
{
    if (duer_app_init_flag) {

        duer_app_init_flag = 0;

        if (g_duer_timer) {
            duer_timer_stop(g_duer_timer);
            duer_timer_release(g_duer_timer);
            g_duer_timer = NULL;
        }
        duer_dcs_del_all_alert();
        duer_app_dcs_uninitialize();

        do {
            if (OS_Q_FULL != os_taskq_post("duer_app_task", 1, DUER_QUIT)) {
                break;
            }
            DUER_LOGW("duer_app_task send msg DUER_QUIT timeout ");
            msleep(20);
        } while (1);

        thread_kill(&duer_app_task_pid, KILL_WAIT);
    }
}
