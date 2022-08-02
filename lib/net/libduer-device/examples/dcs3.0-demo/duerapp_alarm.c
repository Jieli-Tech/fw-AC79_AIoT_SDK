#include <stdlib.h>
#include <string.h>

#include "duerapp_media.h"
#include "duerapp_alarm.h"
#include "duerapp_config.h"
#include "lightduer_net_ntp.h"
#include "lightduer_types.h"
#include "lightduer_dcs.h"
#include "lightduer_dcs_alert.h"
#include "lightduer_memory.h"
#include "system/time.h"
#include "lightduer_mutex.h"
#include "asm/rtc.h"

enum {
    DUER_TIMER = 0,
    DUER_ALARM,
};

static duerapp_alarm_node alarms[DUER_MAX_ALARM_NUM];
static duer_mutex_t duer_alarm_mutex;
static bool s_is_bell = false;
static char *s_ring_path = NULL;

static void duer_alarm_callback(void *param)
{
    duerapp_alarm_node *node = (duerapp_alarm_node *)param;

    if (node->del_flag) {
        return;
    }
    node->del_flag = 1;

    DUER_LOGI("alarm started: token: %s\n", node->token);

    duer_dcs_report_alert_event(node->token, ALERT_START);

    if (node->url) {
        /* duer_media_audio_pause(); */
        /* duer_media_alarm_play(node->url); */
        /* s_is_bell = true; */
    }

    duer_mutex_lock(duer_alarm_mutex);
    duer_timer_release(node->handle);
    node->handle = NULL;
    DUER_FREE(node->token);
    node->token = NULL;
    DUER_FREE(node->time);
    node->time = NULL;
    if (node->url) {
        DUER_FREE(node->url);
        node->url = NULL;
    }
    node->id = -1;
    duer_mutex_unlock(duer_alarm_mutex);
}

void duer_dcs_alert_set_handler(const duer_dcs_alert_info_type *alert_info)
{
    size_t len = 0;
    int i = 0;
    int ret = 0;
    u32 time = 0;
    duerapp_alarm_node *empty_alarm = NULL;
    DuerTime cur_time;
    struct sys_time rtc_time;
    size_t delay = 0;

    duer_mutex_lock(duer_alarm_mutex);
    for (i = 0; i < sizeof(alarms) / sizeof(alarms[0]); i++) {
        if (alarms[i].token == NULL) {
            empty_alarm = &alarms[i];
            break;
        }
    }

    if (empty_alarm == NULL) {
        DUER_LOGE("Too many alarms\n");
        goto error_out;
    }

    len = strlen(alert_info->token) + 1;
    empty_alarm->token = DUER_MALLOC(len);
    if (!empty_alarm->token) {
        DUER_LOGI("alarm token no memory\n");
        goto error_out;
    }
    snprintf(empty_alarm->token, len, "%s", alert_info->token);

    len = strlen(alert_info->time) + 1;
    empty_alarm->time = DUER_MALLOC(len);
    if (!empty_alarm->time) {
        DUER_LOGI("alarm time no memory\n");
        goto error_out;
    }
    snprintf(empty_alarm->time, len, "%s", alert_info->time);

    if (alert_info->url) {
        len = strlen(alert_info->url) + 1;
        empty_alarm->url = DUER_MALLOC(len);
        if (!empty_alarm->url) {
            DUER_LOGI("alarm url no memory\n");
            goto error_out;
        }
        snprintf(empty_alarm->url, len, "%s", alert_info->url);
    } else {
        empty_alarm->url = NULL;
    }

    DUER_LOGI("set alarm: scheduled_time: %s, token: %s, type: %s, url: %s\n", alert_info->time, alert_info->token, alert_info->type, alert_info->url);

    time = iso8601_to_mktime(alert_info->time);

    ret = duer_ntp_client(NULL, 0, &cur_time, NULL);
    if (ret < 0) {
        DUER_LOGE("Failed to get NTP time\n");
        //获取RTC时间，并转换为cur_time对应格式,赋值给cur_time
        if (get_rtc_sys_time(&rtc_time)) {
            DUER_LOGE("Failed to get rtc time\n");
            goto error_out;
        }
        cur_time.sec = sys_time_to_mktime(&rtc_time);
        cur_time.usec = 0;
    }

    if (time <= cur_time.sec) {
        DUER_LOGE("The alarm is expired\n");
        goto error_out;
    }

    delay = (time - cur_time.sec) * 1000 + cur_time.usec / 1000;

    empty_alarm->handle = duer_timer_acquire(duer_alarm_callback, empty_alarm, DUER_TIMER_ONCE);
    if (!empty_alarm->handle) {
        DUER_LOGE("Failed to set alarm: failed to create timer\n");
        goto error_out;
    }

    ret = duer_timer_start(empty_alarm->handle, delay);
    if (ret != DUER_OK) {
        DUER_LOGE("Failed to set alarm: failed to start timer\n");
        goto error_out;
    }

    if (DUER_OK != duer_dcs_report_alert_event(alert_info->token, SET_ALERT_SUCCESS)) {
        DUER_LOGE("Failed to report alert event\n");
        duer_dcs_report_alert_event(alert_info->token, SET_ALERT_FAIL);
        duer_timer_release(empty_alarm->handle);
        empty_alarm->handle = NULL;
        DUER_FREE(empty_alarm->token);
        empty_alarm->token = NULL;
        DUER_FREE(empty_alarm->time);
        empty_alarm->time = NULL;
        if (empty_alarm->url) {
            DUER_FREE(empty_alarm->url);
            empty_alarm->url = NULL;
        }
        duer_mutex_unlock(duer_alarm_mutex);

        return;
    }

    empty_alarm->id = i;
    empty_alarm->del_flag = 0;
    empty_alarm->is_active = 0;
    if (!strncmp(alert_info->type, "TIMER", 5)) {
        empty_alarm->alarm_type = DUER_TIMER;
    } else if (!strncmp(alert_info->type, "ALARM", 5)) {
        empty_alarm->alarm_type = DUER_ALARM;
    } else {
        DUER_LOGE("NO this alarm type\n");
    }
    /*此处需要将闹钟信息保存在VM里*/

    duer_mutex_unlock(duer_alarm_mutex);

    return;

error_out:
    duer_dcs_report_alert_event(alert_info->token, SET_ALERT_FAIL);

    if (empty_alarm->token) {
        DUER_FREE(empty_alarm->token);
        empty_alarm->token = NULL;
    }
    if (empty_alarm->time) {
        DUER_FREE(empty_alarm->time);
        empty_alarm->time = NULL;
    }
    if (empty_alarm->url) {
        DUER_FREE(empty_alarm->url);
        empty_alarm->url = NULL;
    }

    duer_mutex_unlock(duer_alarm_mutex);
}

void duer_dcs_alert_delete_handler(const char *token)
{
    int i = 0;
    volatile duerapp_alarm_node *target_alarm = NULL;

    DUER_LOGI("delete alarm: token %s", token);

    duer_mutex_lock(duer_alarm_mutex);
    for (i = 0; i < sizeof(alarms) / sizeof(alarms[0]); i++) {
        if (alarms[i].token) {
            if (strcmp(alarms[i].token, token) == 0) {
                target_alarm = &alarms[i];
                break;
            }
        }
    }

    if (!target_alarm) {
        duer_mutex_unlock(duer_alarm_mutex);
        DUER_LOGE("Cannot find the target alarm\n");
        duer_dcs_report_alert_event(token, DELETE_ALERT_FAIL);
        return;
    }

    if (!target_alarm->del_flag) {
        target_alarm->del_flag = 1;
        duer_timer_release(target_alarm->handle);
        target_alarm->handle = NULL;
        DUER_FREE(target_alarm->token);
        target_alarm->token = NULL;
        DUER_FREE(target_alarm->time);
        target_alarm->time = NULL;
        if (target_alarm->url) {
            DUER_FREE(target_alarm->url);
            target_alarm->url = NULL;
        }
        target_alarm->id = -1;
    }

    duer_mutex_unlock(duer_alarm_mutex);
    duer_dcs_report_alert_event(token, DELETE_ALERT_SUCCESS);
}

void duer_init_alarm()
{
    memset(alarms, 0, sizeof(alarms));
    if (!duer_alarm_mutex) {
        duer_alarm_mutex = duer_mutex_create();
        if (!duer_alarm_mutex) {
            DUER_LOGE("duer_alarm_mutex create error\n");
        }
    }
    //开机读取VM，保存在alarm数组里，重新设置定时器？
}

void duer_dcs_get_all_alert(baidu_json *alert_array)
{
    int i;
    duerapp_alarm_node *empty_alarm = NULL;
    duer_dcs_alert_info_type alert_info;
    u8 is_active = false;

    duer_mutex_lock(duer_alarm_mutex);
    for (i = 0; i < sizeof(alarms) / sizeof(alarms[0]); i++) {
        if (alarms[i].token) {
            empty_alarm = &alarms[i];
            alert_info.token = empty_alarm->token;
            alert_info.type = empty_alarm->alarm_type ? "ALARM" : "TIMER";
            alert_info.time = empty_alarm->time;
            is_active = empty_alarm->is_active;
            duer_insert_alert_list(alert_array, &alert_info, is_active);
        }
    }
    duer_mutex_unlock(duer_alarm_mutex);
}

bool duer_alert_bell()
{
    return s_is_bell;
}

void duer_alert_stop()
{
    if (s_is_bell) {
        duer_media_alarm_stop();
    } else {
        DUER_LOGD("Now the alert is not ringing.");
    }
}
