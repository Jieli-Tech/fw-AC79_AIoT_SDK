#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_api/tvs_api.h"
#include "tvs_alert_impl.h"
#include "tvs_api_impl.h"
#include "system/includes.h"

#include "cJSON_common/cJSON.h"
#include "os/os_api.h"
#include "time.h"
#include "timer.h"
#include "tvs_media_player_inner.h"
#include "server/ai_server.h"


//yii:打开则使用RTC闹钟，关闭使用系统闹钟
#define USE_RTC_ALARM 0
#define ALARM_TIMEOUT	60

//yii:引用tencent平台api
extern const struct ai_sdk_api tc_tvs_api;
extern u8 upgrade_flag;
extern void tvs_taskq_post(int msg);

u8 alarm_rings = 0;	//闹钟开启标志位
u8 alarm_interrupt_mode = 0; //0. 关闭语音识别 响应闹钟 1. 无视闹钟 继续当前语音识别
static int wakeup_timeout_id = 0;
static int alarm_wakeup_pid = 0;



typedef struct AlertInfo {
    struct list_head entry;
    char *schedule;
    char *token;
    char *type;
    char *url;
    int  id;
    time_t tSet;
} alert_info;

static alert_info alert_head;

typedef struct AlarmInfo {
    int year;
    int month;
    int day;
    int hour;
    int min;
    int sec;
} alarm_info;

extern void set_alarm_ctrl(u8 set_alarm);
static void rings(void *priv);
static void RTC_alarm(alert_info *new_alert);



static int time_quit = 1;

__attribute__((weak)) int get_listen_flag()
{
    return 0;
}

#if USE_RTC_ALARM
/* 打开闹钟开关 */
static struct sys_time set_alarm = {
    .year 	= 2020,
    .month 	= 10,
    .day 	= 10,
    .hour 	= 10,
    .min 	= 10,
    .sec 	= 10,
};

static void RTC_alarm_task(void)
{
    void *dev = NULL;
    dev = dev_open("rtc", NULL);

    while (1) {
        /* 获取时间信息 */
        dev_ioctl(dev, IOCTL_GET_SYS_TIME, (u32)&set_alarm);
        printf("time_run: %d-%d-%d %d:%d:%d\n", set_alarm.year, set_alarm.month, set_alarm.day, set_alarm.hour, set_alarm.min, set_alarm.sec);
        /* 延时1秒 */
        os_time_dly(100);
        if (time_quit == 1) {
            break;
        }
    }
}

static void RTC_alarm(alert_info *new_alert)
{
    void *dev = NULL;


    /* 打开RTC设备 */
    dev = dev_open("rtc", NULL);

    /* 获取时间信息 */
    dev_ioctl(dev, IOCTL_GET_ALARM, (u32)&set_alarm);
    /* 打印时间信息 */
    printf("get_alarm_time: %d-%d-%d %d:%d:%d\n", set_alarm.year, set_alarm.month, set_alarm.day, set_alarm.hour, set_alarm.min, set_alarm.sec);

    /* u8 weekday = dev_ioctl(dev, IOCTL_GET_WEEKDAY, (u32)&set_alarm); */
    /* printf("weekday = %d", weekday); */

    /* 注册闹钟响铃回调函数 */
    set_rtc_isr_callback(rings, (void *)new_alert);

    set_alarm_ctrl(1);
    /* 设置闹钟 */
    struct tm tm = {0};
    time_t timestamp = new_alert->tSet + 28800;
    localtime_r(&timestamp, &tm);
    struct sys_time info = {0};
    info.year 	= tm.tm_year + 1900;
    info.month 	= tm.tm_mon + 1;
    info.day	= tm.tm_mday;
    info.hour	= tm.tm_hour;
    info.min	= tm.tm_min;
    info.sec	= tm.tm_sec;
    dev_ioctl(dev, IOCTL_SET_ALARM, (u32)&info);

    /* 清空时间信息 */
    memset(&set_alarm, 0, sizeof(set_alarm));
    /* 获取设置的闹钟信息 */
    dev_ioctl(dev, IOCTL_GET_ALARM, (u32)&set_alarm);
    printf("get_alarm_time: %d-%d-%d %d:%d:%d\n", set_alarm.year, set_alarm.month, set_alarm.day, set_alarm.hour, set_alarm.min, set_alarm.sec);

    /* 关闭RTC设备 */
    dev_close(dev);
    dev = NULL;

#if 1
    //创建闹钟线程  用来测试的 看下是否创建或在跑时
    if (time_quit) {
        time_quit = 0;
        thread_fork("RTC_alarm_test", 20, 500, 0, NULL, RTC_alarm_task, NULL);
    }
#endif
}

static void set_new_rtc_alarm(void)
{
    struct list_head *pos = NULL;
    alert_info *alert = NULL;
    alert_info *save_alert = NULL;
    alert_info *backup_alert = NULL;
    list_for_each(pos, &alert_head.entry) {
        alert = list_entry(pos, alert_info, entry);
        if (save_alert == NULL) {
            save_alert = alert;
        } else {
            if (alert->tSet < save_alert->tSet) {
                save_alert = alert;
            }
        }
    }
    if (save_alert != NULL) {
        RTC_alarm(save_alert);
    } else {
        set_alarm_ctrl(0);
    }
}

#else

static void SYS_alarm(alert_info *new_alert)
{
    u32 now_time = time(NULL);
    u32 timeout = (new_alert->tSet > now_time) ? (new_alert->tSet - now_time) : 0;
    if (!timeout) {
        return ;
    }
    new_alert->id = sys_timeout_add_to_task("app_core", new_alert, rings, timeout * 1000);
}

#endif

static void del_alarm_info(void *priv)
{
    //删除闹钟
    struct list_head *pos = NULL;
    alert_info *del_alert = NULL;
    alert_info *alert 	  = (alert_info *)priv;

    list_for_each(pos, &alert_head.entry) {
        del_alert = list_entry(pos, alert_info, entry);
        if (!strcmp(del_alert->token, alert->token)) {
            list_del(&del_alert->entry);
            free(del_alert->schedule);
            free(del_alert->token);
            free(del_alert->type);
            free(del_alert);
            printf("-------%s---------%d-----del !\n\n", __func__, __LINE__);
            break;
        }
    }
#if USE_RTC_ALARM
    set_new_rtc_alarm();
#endif
}


//要是停止闹钟 调用这个函数
void alarm_rings_stop(int tvs_alert_stop_reason)
{
    if (alarm_rings == 0) {
        return ;
    }
    os_taskq_post("alarm_wakeup_test", 1, tvs_alert_stop_reason);
}

static void wakeup_timeout(void *priv)
{
    if (alarm_rings == 0) {
        return ;
    }
    os_taskq_post("alarm_wakeup_test", 1, TVS_ALART_STOP_REASON_TIMEOUT);
}

static void alarm_wakeup_test(void *priv)
{
    alert_info *alert = (alert_info *)priv;
    int err = 0;
    int msg[32] = {0};
    if (!upgrade_flag) {

        if (alarm_interrupt_mode == 0) {			//0. 关闭语音识别 响应闹钟
            if (get_listen_flag() >= 2) {
                alarm_rings = 1;
                tvs_taskq_post(TC_RECORD_STOP);
                while (get_listen_flag() >= 2) {	//等待关闭语音识别
                    os_time_dly(20);
                }
            }
        } else if (alarm_interrupt_mode == 1) {	//1. 无视闹钟 继续当前语音识别
            if (get_listen_flag() >= 2) {
                //不上报闹钟响铃也不上报闹钟停止, 看下云端会不会有影响, 没影响就不处理
                goto alarm_exit;
            }
        }

        alarm_rings = 1;
        tvs_media_player_inner_pause_play();
        wakeup_timeout_id = sys_timeout_add_to_task("app_core", NULL, wakeup_timeout, ALARM_TIMEOUT * 1000);
        on_alert_trigger_start((const char *)alert->token);
        ai_server_event_url(&tc_tvs_api, alert->url, AI_SERVER_EVENT_URL);
        while (1) {
            err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
            if (err != OS_TASKQ || msg[0] != Q_USER) {
                continue;
            }
            if ((msg[1] == TVS_ALART_STOP_REASON_NEW_RECO) || (msg[1] == TVS_ALART_STOP_REASON_MANNUAL) || (msg[1] == TVS_ALART_STOP_REASON_TIMEOUT)) {
                break;
            }
        }
        if (wakeup_timeout_id) {
            sys_timeout_del(wakeup_timeout_id);
        }
        ai_server_event_url(&tc_tvs_api, NULL, AI_SERVER_EVENT_STOP);
        on_alert_trigger_stop((const char *)alert->token, msg[1]);
        alarm_rings = 0;
        tvs_media_player_inner_start_play();
    }
alarm_exit:

    thread_fork("del_alarm_info", 20, 1024, 0, NULL, del_alarm_info, priv);
}

/* 闹钟响了进行这个函数 */
static void rings(void *priv)
{
    alert_info *alert = (alert_info *)priv;
    printf("alarm clock is rings!!!!!!!!!!!!!!!");
    time_quit = 1;
    if (!alarm_wakeup_pid) {
        thread_fork("alarm_wakeup_test", 20, 500, 10, &alarm_wakeup_pid, alarm_wakeup_test, priv);
    }

}

static int tvs_alert_adapter_impl_new(tvs_alert_infos *alerts, int alert_count)
{
    //保存闹钟
    int days_in_month[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    cJSON *json = NULL, *schedule = NULL, *token = NULL, *type = NULL, *assets = NULL, *index = NULL, *url = NULL;
    struct list_head *pos = NULL;
    alert_info *new_alert = NULL;

    json = cJSON_Parse(alerts->alert_detail);

    schedule = cJSON_GetObjectItem(json, "scheduledTime");
    token = cJSON_GetObjectItem(json, "token");
    type = cJSON_GetObjectItem(json, "type");

    assets	= cJSON_GetObjectItem(json, "assets");
    //看json字符串 目前只有一个index，后续如果还有其他的再修改
    index	= cJSON_GetArrayItem(assets, 0);
    url		= cJSON_GetObjectItem(index, "url");


    //yii:判断是否已经创建该闹钟
    list_for_each(pos, &alert_head.entry) {
        new_alert = list_entry(pos, alert_info, entry);
        if (!strcmp(new_alert->schedule, schedule->valuestring)) {
            printf("-------%s---------%d-----already existence !", __func__, __LINE__);
            return 0;
        }

    }
    //yii：存储闹钟信息到节点
    new_alert = (alert_info *)calloc(1, sizeof(alert_info));
    if (!new_alert) {
        printf("-----------%s-----------%d calloc failed", __func__, __LINE__);
        return -1;
    }
    new_alert->schedule	= 	strdup(schedule->valuestring);
    new_alert->token	=	strdup(token->valuestring);
    new_alert->type		=	strdup(type->valuestring);
    new_alert->url		=	strdup(url->valuestring);

    list_add_tail(&new_alert->entry, &alert_head.entry);

    alarm_info info = {0};
    //yii：获取年月日时分秒，继而创建闹钟
    sscanf(schedule->valuestring, "%d-%d-%dT%d:%d:%d", &info.year, &info.month, &info.day, &info.hour, &info.min, &info.sec);

    struct tm tm_set;
    memset(&tm_set, 0, sizeof(struct tm));
    tm_set.tm_year 	= info.year - 1900;
    tm_set.tm_mon 	= info.month - 1;
    tm_set.tm_mday 	= info.day;
    tm_set.tm_hour	= info.hour;
    tm_set.tm_min	= info.min;
    tm_set.tm_sec	= info.sec;

    new_alert->tSet = mktime(&tm_set);
    printf("---------%s-------------%d---------time = %ld set = %ld\r\n", __func__, __LINE__, time(NULL), new_alert->tSet);

#if 0	//只供测试使用只供测试使用
    info.hour += 8;		//yii:东八区
    if (info.hour > 24) {
        info.hour -= 24;
        info.day += 1;
        int leap = ((!(info.year / 4) && (info.year / 100)) || !(info.year / 400)) ? 1 : 0;
        if (info.day > days_in_month[info.month] + ((info.month == 2) ? leap : 0)) {
            info.day = 1;
            info.month += 1;
            if (info.month > 12) {
                info.month = 1;
                info.year += 1;
            }
        }
    }

    printf("---------%s-------------%d---%d-%d-%dT%d:%d:%d----------------------------------\n\n\n", __func__, __LINE__,  info.year, info.month, info.day, info.hour, info.min, info.sec);
#endif

#if USE_RTC_ALARM
    /* RTC_alarm(new_alert); */
    set_new_rtc_alarm();
#else
    SYS_alarm(new_alert);
#endif

    return 0;
}

static int tvs_alert_adapter_impl_delete(tvs_alert_summary *alerts, int alert_count)
{
    //删除闹钟
    struct list_head *pos = NULL;
    alert_info *del_alert = NULL;


    list_for_each(pos, &alert_head.entry) {
        del_alert = list_entry(pos, alert_info, entry);
        if (!strcmp(del_alert->token, alerts->alert_token)) {
#if !(USE_RTC_ALARM)
            sys_timeout_del(del_alert->id);
#endif
            list_del(&del_alert->entry);
            free(del_alert->schedule);
            free(del_alert->token);
            free(del_alert->type);
            free(del_alert);
            printf("-------%s---------%d-----del !\n\n", __func__, __LINE__);
            break;
        }
    }
#if USE_RTC_ALARM
    set_new_rtc_alarm();
#endif

    return 0;
}

void *tvs_alert_adapter_impl_get_all_alerts()
{
    cJSON *alert_arr = cJSON_CreateArray();
    //获取闹钟概要信息并填充json，必须返回cJSON*类型的数据
    struct list_head *pos = NULL;
    alert_info *alert = NULL;

    list_for_each(pos, &alert_head.entry) {
        alert = list_entry(pos, alert_info, entry);

        cJSON *node = cJSON_CreateObject();
        cJSON_AddItemToObject(node, "scheduledTime", cJSON_CreateString(alert->schedule));
        cJSON_AddItemToObject(node, "token", cJSON_CreateString(alert->token));
        cJSON_AddItemToObject(node, "type", cJSON_CreateString(alert->type));

        cJSON_AddItemToArray(alert_arr, node);
    }


    return alert_arr;
}

// TO-DO 闹钟响铃时需要调用此函数通知SDK
void on_alert_trigger_start(const char *alert_token)
{
    if (alert_token == NULL) {
        return;
    }
    tvs_alert_adapter_on_trigger(alert_token);
}

// TO-DO 闹钟响铃之后，超时或者手动停止，都需要调用此函数通知SDK
void on_alert_trigger_stop(const char *alert_token, tvs_alert_stop_reason reason)
{
    if (alert_token == NULL) {
        return;
    }
    tvs_alert_adapter_on_trigger_stop(alert_token, reason);
}

static void alert_init()
{
    //yii:闹钟节点头初始化
    INIT_LIST_HEAD(&alert_head.entry);
}

int tvs_init_alert_adater_impl(tvs_alert_adapter *ad)
{
    if (NULL == ad) {
        return -1;
    }

    // 初始化adapter
    ad->do_delete = tvs_alert_adapter_impl_delete;
    ad->do_new_alert = tvs_alert_adapter_impl_new;
    ad->get_alerts_ex = tvs_alert_adapter_impl_get_all_alerts;
    alert_init();
    return 0;
}


