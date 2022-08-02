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
 * File: duerapp_config.h
 * Auth: Renhe Zhang (v_zhangrenhe@baidu.com)
 * Desc: Duer Alert function file.
 */

#include "printf.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "duerapp_config.h"
#include "lightduer_dcs_alert.h"
#include "duerapp_media.h"
#include "lightduer_net_ntp.h"
#include "lightduer_types.h"
#include "lightduer_timers.h"
#include "lightduer_mutex.h"
#include "lightduer_memory.h"
#include "duerapp_alert.h"
#include "os/os_api.h"

typedef struct _duerapp_alert_node {
    char *token;
    char *type;
    char *time;
    int ring_count;
    bool isbell;
    duer_timer_handler handle;
    char *ring_tab[0];
} duerapp_alert_node;

typedef struct _duer_alert_list {
    struct _duer_alert_list *next;
    duerapp_alert_node *data;
} duer_alert_list_t;

static duer_alert_list_t s_alert_head;
static duer_mutex_t s_alert_mutex;
static duer_mutex_t s_bell_mutex;
static bool s_is_bell = false;

bool duer_alert_bell()
{
    return s_is_bell;
}

static char *duer_get_json_value_str(const baidu_json *data)
{
    char *ret = NULL;
    int len = 0;
    if (data && data->valuestring) {
        len = strlen(data->valuestring) + 1;
        ret = (char *)DUER_MALLOC(len);
        if (ret) {
            snprintf(ret, len, "%s", data->valuestring);
        } else {
            DUER_LOGE("ret malloc failed.");
        }
    }
    return ret;
}

static char *duer_get_assets_url_str(const baidu_json *assets, const char *assetid)
{
    char *ret = NULL;
    baidu_json *ass_url = NULL;
    baidu_json *asset = NULL;
    baidu_json *ass_id = NULL;
    int ass_count = 0;
    if (!assets) {
        DUER_LOGW("assets is null.");
        return NULL;
    }
    ass_count = baidu_json_GetArraySize(assets);

    for (int i = 0; i < ass_count; i++) {
        asset = baidu_json_GetArrayItem(assets, i);
        if (!asset) {
            continue;
        }
        ass_id = baidu_json_GetObjectItem(asset, "assetId");
        if (ass_id && ass_id->valuestring
            && (strcmp(ass_id->valuestring, assetid) == 0)) {
            ass_url = baidu_json_GetObjectItem(asset, "url");
            break;
        }
    }
    ret = duer_get_json_value_str(ass_url);
    if (!ret) {
        DUER_LOGW("not find assetId(%s) of url.", assetid);
    }
    return ret;
}

static duer_errcode_t duer_alert_list_push(duerapp_alert_node *data)
{
    duer_errcode_t rt = DUER_OK;
    duer_alert_list_t *new_node = NULL;
    duer_alert_list_t *tail = &s_alert_head;

    do {
        new_node = (duer_alert_list_t *)DUER_MALLOC(sizeof(duer_alert_list_t));
        if (!new_node) {
            DUER_LOGE("Memory too low");
            rt = DUER_ERR_MEMORY_OVERLOW;
            break;
        }
        new_node->next = NULL;
        new_node->data = data;

        while (tail->next) {
            tail = tail->next;
        }

        tail->next = new_node;
    } while (0);

    return rt;
}

static duer_errcode_t duer_alert_list_remove(duerapp_alert_node *data)
{
    duer_alert_list_t *pre = &s_alert_head;
    duer_alert_list_t *cur = NULL;
    duer_errcode_t rt = DUER_ERR_FAILED;

    while (pre->next) {
        cur = pre->next;
        if (cur->data == data) {
            pre->next = cur->next;
            DUER_FREE(cur);
            rt = DUER_OK;
            break;
        }
        pre = pre->next;
    }

    return rt;
}

static duerapp_alert_node *duer_create_alert_node(const baidu_json *data)
{
    baidu_json *payload = NULL;
    baidu_json *playorder = NULL;
    baidu_json *time = NULL;
    baidu_json *token = NULL;
    baidu_json *type = NULL;
    baidu_json *assets = NULL;
    baidu_json *playorder_id = NULL;
    int playorder_count = 0;
    int node_size = sizeof(duerapp_alert_node);
    duerapp_alert_node *alert = NULL;

    if (!data) {
        DUER_LOGW("data is null.");
        return NULL;
    }

    payload = baidu_json_GetObjectItem(data, "payload");
    if (!payload) {
        DUER_LOGW("payload is null.");
        return NULL;
    }

    time = baidu_json_GetObjectItem(payload, "scheduledTime");
    token = baidu_json_GetObjectItem(payload, "token");
    type = baidu_json_GetObjectItem(payload, "type");
    playorder = baidu_json_GetObjectItem(payload, "assetPlayOrder");
    assets = baidu_json_GetObjectItem(payload, "assets");
    if (!(time && token && type && playorder && assets)) {
        DUER_LOGW("Missing required parameters.");
        return NULL;
    }

    playorder_count = baidu_json_GetArraySize(playorder);
    node_size += playorder_count ? ((playorder_count) * sizeof(char *)) : 0;
    alert = (duerapp_alert_node *)DUER_MALLOC(node_size);
    if (!alert) {
        DUER_LOGE("alert malloc failed.");
        return NULL;
    }

    alert->ring_count = playorder_count;
    alert->isbell = false;
    alert->handle = NULL;
    alert->ring_tab[0] = NULL;
    alert->token = duer_get_json_value_str(token);
    alert->type = duer_get_json_value_str(type);
    alert->time = duer_get_json_value_str(time);
    if (!(alert->token && alert->type && alert->time)) {
        if (alert->token) {
            DUER_FREE(alert->token);
        }
        if (alert->type) {
            DUER_FREE(alert->type);
        }
        if (alert->time) {
            DUER_FREE(alert->time);
        }
        if (alert) {
            DUER_FREE(alert);
        }
        return NULL;
    }

    for (int i = 0; i < playorder_count; i++) {
        playorder_id = baidu_json_GetArrayItem(playorder, i);
        if (playorder_id && playorder_id->valuestring) {
            alert->ring_tab[i] = duer_get_assets_url_str(assets, playorder_id->valuestring);
        } else {
            DUER_LOGE("get assetPlayOrder Id failed. num:%d", i);
        }
    }

    return alert;
}

static void duer_free_alert_node(duerapp_alert_node *alert)
{
    if (alert) {
        for (int i = 0; i < alert->ring_count; i++) {
            if (alert->ring_tab[i]) {
                DUER_FREE(alert->ring_tab[i]);
                alert->ring_tab[i] = NULL;
            }
        }
        if (alert->token) {
            DUER_FREE(alert->token);
            alert->token = NULL;
        }

        if (alert->type) {
            DUER_FREE(alert->type);
            alert->type = NULL;
        }

        if (alert->time) {
            DUER_FREE(alert->time);
            alert->time = NULL;
        }

        if (alert->handle) {
            duer_timer_release(alert->handle);
            alert->handle = NULL;
        }

        DUER_FREE(alert);
    }
}

static void duer_bell_start(duerapp_alert_node *alert_info)
{
    duer_mutex_lock(s_bell_mutex);
    duer_media_audio_stop();
    duer_media_speak_stop();
    duer_media_alarm_stop();

    alert_info->isbell = true;
    s_is_bell = true;

    if (alert_info->ring_count) {
        // play url
        for (int i = 0; i < alert_info->ring_count; i++) {
            duer_media_alarm_play(alert_info->ring_tab[i]);
            if (!s_is_bell) {
                break;
            }
        }
    } else {
        // play loacl
    }

    duer_dcs_report_alert_event(alert_info->token, ALERT_STOP);

    alert_info->isbell = false;
    s_is_bell = false;
    duer_mutex_unlock(s_bell_mutex);
}

static void duer_alert_start(duerapp_alert_node *alert_info)
{
    if (s_is_bell) {
        duer_alert_stop();
    }

    duer_bell_start(alert_info);
}

void duer_alert_stop()
{
    if (s_is_bell) {
        s_is_bell = false;
        duer_media_alarm_stop();
    } else {
        DUER_LOGI("Now the alert is not ringing.");
    }
}

static void duer_alert_callback(void *param)
{
    duerapp_alert_node *alert = (duerapp_alert_node *)param;

    DUER_LOGI("alert started: token: %s", alert->token);

    duer_dcs_report_alert_event(alert->token, ALERT_START);
    // play url
    duer_alert_start(alert);
}

static time_t duer_dcs_get_time_stamp(const char *time)
{
    time_t time_stamp = 0;
    struct tm time_tm;
    int rs = 0;
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int min = 0;
    int sec = 0;

    rs = sscanf(time, "%d-%d-%dT%d:%d:%d+", &year, &month, &day, &hour, &min, &sec);
    if (rs != 6) {
        return (time_t) - 1;
    }

    time_tm.tm_year  = year - 1900;
    time_tm.tm_mon   = month - 1;
    time_tm.tm_mday  = day;
    time_tm.tm_hour  = hour;
    time_tm.tm_min   = min;
    time_tm.tm_sec   = sec;
    time_tm.tm_isdst = 0;

    return mktime(&time_tm);
}

static void duer_alert_set_thread(void *priv)
{
    duerapp_alert_node *alert = (duerapp_alert_node *)priv;
    int ret = 0;
    DuerTime cur_time;
    size_t delay = 0;
    int rs = DUER_OK;
    time_t time_stamp = 0;

    if (!alert) {
        DUER_LOGE("alert is null.");
        goto exit;
    }

    DUER_LOGI("set alert: scheduled_time: %s, token: %s\n", alert->time, alert->token);

#if 1
    time_stamp = duer_dcs_get_time_stamp(alert->time);
    if (time_stamp < 0) {
        duer_dcs_report_alert_event(alert->token, SET_ALERT_FAIL);
        duer_free_alert_node(alert);
        goto exit;
    }

#else
    struct tm pt;
    memset(&pt, 0, sizeof(struct tm));

    extern char *strptime(const char *buf, const char *format, struct tm * tm);
    if (!strptime(alert->time, "%Y-%m-%dT%H:%M:%S+0000", &pt)) {
        duer_dcs_report_alert_event(alert->token, SET_ALERT_FAIL);
        duer_free_alert_node(alert);
        goto exit;
    }

    pt.tm_year += 70;
    pt.tm_mon;

    time_stamp = mktime(&pt);
#endif

    DUER_LOGI("time_stamp: %d\n", time_stamp);

    ret = duer_ntp_client(NULL, 0, &cur_time, NULL);
    if (ret < 0) {
        DUER_LOGE("Failed to get NTP time.ret = %d", ret);
        duer_dcs_report_alert_event(alert->token, SET_ALERT_FAIL);
        duer_free_alert_node(alert);
        goto exit;
    }

    if (time_stamp <= cur_time.sec) {
        DUER_LOGE("The alert is expired\n");
        duer_dcs_report_alert_event(alert->token, SET_ALERT_FAIL);
        duer_free_alert_node(alert);
        goto exit;
    }

    delay = (time_stamp - cur_time.sec) * 1000 - cur_time.usec / 1000;

    alert->handle = duer_timer_acquire(duer_alert_callback, alert, DUER_TIMER_ONCE);
    if (!alert->handle) {
        DUER_LOGE("Failed to set alert: failed to create timer\n");
        duer_dcs_report_alert_event(alert->token, SET_ALERT_FAIL);
        duer_free_alert_node(alert);
        goto exit;
    }

    rs = duer_timer_start(alert->handle, delay);
    if (rs != DUER_OK) {
        DUER_LOGE("Failed to set alert: failed to start timer\n");
        duer_dcs_report_alert_event(alert->token, SET_ALERT_FAIL);
        duer_free_alert_node(alert);
        goto exit;
    }

    duer_mutex_lock(s_alert_mutex);
    duer_alert_list_push(alert);
    duer_mutex_unlock(s_alert_mutex);
    duer_dcs_report_alert_event(alert->token, SET_ALERT_SUCCESS);

    DUER_LOGI("Set alert successful timer delay : %d \n", delay);

exit:
    msleep(2000);
}

duer_status_t duer_dcs_tone_alert_set_handler(const baidu_json *directive)
{
    static u8 alarm_num = 0;
    char thread_name[32];
    duerapp_alert_node *alert = NULL;
    alert = duer_create_alert_node(directive);
    if (alert && alert->token && alert->type && alert->time) {
        snprintf(thread_name, sizeof(thread_name), "%s%d", "duer_alert_set_thread_", alarm_num++);
        int ret = thread_fork(thread_name, 22, 1536, 0, NULL, duer_alert_set_thread, (void *)alert);
        if (ret != 0) {
            DUER_LOGE("Create alert failed!");
            duer_dcs_report_alert_event(alert->token, SET_ALERT_FAIL);
        }
        return DUER_OK;
    } else {
        DUER_LOGE("ERROR SetAlert Failed.");
        return DUER_ERR_FAILED;
    }
}

static duerapp_alert_node *duer_find_target_alert(const char *token)
{
    duer_alert_list_t *node = s_alert_head.next;
    while (node) {
        if (node->data) {
            if (strcmp(token, node->data->token) == 0) {
                return node->data;
            }
        }

        node = node->next;
    }

    return NULL;
}

void duer_dcs_alert_delete_handler(const char *token)
{
    duerapp_alert_node *target_alert = NULL;

    DUER_LOGI("delete alert: token %s", token);

    duer_mutex_lock(s_alert_mutex);

    target_alert = duer_find_target_alert(token);
    if (!target_alert) {
        DUER_LOGE("Cannot find the target alert\n");
        duer_mutex_unlock(s_alert_mutex);
        duer_dcs_report_alert_event(token, DELETE_ALERT_FAIL);
        return;
    }

    duer_alert_list_remove(target_alert);
    duer_free_alert_node(target_alert);

    duer_mutex_unlock(s_alert_mutex);

    duer_dcs_report_alert_event(token, DELETE_ALERT_SUCCESS);
}

void duer_dcs_get_all_alert(baidu_json *alert_array)
{
    duer_alert_list_t *alert_node = s_alert_head.next;
    duer_dcs_alert_info_type alert_info;

    while (alert_node) {
        alert_info.token = alert_node->data->token;
        alert_info.type = alert_node->data->type;
        alert_info.time = alert_node->data->time;
        duer_insert_alert_list(alert_array, &alert_info, alert_node->data->isbell);
        alert_node = alert_node->next;
    }
}

void duer_dcs_del_all_alert(void)
{
    duer_alert_list_t *alert_node = s_alert_head.next;
    duerapp_alert_node *alert;

    while (alert_node) {
        alert = alert_node->data;
        alert_node = alert_node->next;
        /* duer_dcs_report_alert_event(alert->token, DELETE_ALERT_SUCCESS); */
        duer_alert_list_remove(alert);
        duer_free_alert_node(alert);
    }
}

void duer_alert_init()
{
    if (!s_alert_mutex) {
        s_alert_mutex = duer_mutex_create();
    }
    if (!s_bell_mutex) {
        s_bell_mutex = duer_mutex_create();
    }
    duer_dcs_alert_init();
}

