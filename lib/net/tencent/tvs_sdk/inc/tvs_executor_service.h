#ifndef __TVS_EXECUTOR_SERVICE_H_8JNWAK__
#define __TVS_EXECUTOR_SERVICE_H_8JNWAK__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "os_wrapper.h"
#include "executor_service.h"
#include "tvs_speech_manager.h"
#include "tvs_exception_report.h"

typedef enum {
    TVS_EXECUTOR_CMDS_EVENT_START = 1,
    TVS_EXECUTOR_CMDS_EVENT_VOLUME_CHANGED,
    TVS_EXECUTOR_CMDS_EVENT_MODE_CHANGED,
    TVS_EXECUTOR_CMDS_EVENT_MEDIA_PLAYER_STATE_CHANGED,
    TVS_EXECUTOR_CMDS_EVENT_SYSTEM_SYNC,
    TVS_EXECUTOR_CMDS_EVENT_PUSH,
    TVS_EXECUTOR_CMDS_EVENT_PUSH_ACK,
    TVS_EXECUTOR_CMDS_EVENT_ALERT_NEW,
    TVS_EXECUTOR_CMDS_EVENT_ALERT_DELETE,
    TVS_EXECUTOR_CMDS_EVENT_ALERT_START,
    TVS_EXECUTOR_CMDS_EVENT_ALERT_STOP,
    TVS_EXECUTOR_CMDS_EVENT_EXCEPTION_REPORT,
    TVS_EXECUTOR_CMDS_EVENT_END = 99,
    TVS_EXECUTOR_CMD_AUTHORIZE = 100,
    TVS_EXECUTOR_CMD_START_PLAY,
    TVS_EXECUTOR_CMD_DOWN_CHANNEL,
    TVS_EXECUTOR_CMD_RECORDER,
    TVS_EXECUTOR_CMD_IPLIST,
    TVS_EXECUTOR_CMD_PING,
    TVS_EXECUTOR_CMD_ACTIVITIES = 199,
    TVS_EXECUTOR_CMD_SPEECH = 200,
    TVS_EXECUTOR_CMD_PLAY_NEXT = 201,
    TVS_EXECUTOR_CMD_PLAY_PREV,
    TVS_EXECUTOR_CMD_PLAY_FINISH,
    TVS_EXECUTOR_CMD_SEMANTIC,
    TVS_EXECUTOR_CMD_TTS,
} tvs_executor_cmd;

typedef struct {
    char *client_id;
    char *refresh_token;
} tvs_executor_param_authorize;

typedef void (*func_free_param)(void *param);

typedef struct {
    func_free_param free_func;
    void *free_func_param;
    void *param;
    void *param2;
    int iparam;
    int session_id;
} tvs_executor_param_events;


// exe service开始执行
int tvs_executor_service_start();

// 开启智能语音对话
int tvs_executor_start_speech(tvs_speech_manager_config *config);

// 开始授权/刷票
int tvs_executor_start_authorize();

// 取消当前正在进行的授权操作
void tvs_executor_cancel_authorize();

// 发起一个数据上报
int tvs_executor_start_event(int cmd, tvs_executor_param_events *ev_params);

// 开始执行一个播控/明确语义等任务
int tvs_executor_start_control(int cmd, tvs_executor_param_events *ev_params);

// 上报音量改变事件
void tvs_executor_upload_volume(int volume);

// 上报人设切换事件
void tvs_executor_upload_mode_changed();

// 上报媒体状态改变的事件
void tvs_executor_upload_media_player_state(int state);

// 从设备端透传文本到手机端
void tvs_executor_send_push_text(char *text);

// 收到推送后，回复ACK
void tvs_executor_send_push_ack(char *token);

// 上报系统状态刷新的事件
void tvs_executor_send_system_sync();

// 停止当前正在执行的speech、control、semantic操作
void tvs_executor_stop_all_activities();

// 上报闹钟/提醒新增的事件
void tvs_executor_upload_alert_new(int success, char *token);

// 上报闹钟/提醒被删除的事件
void tvs_executor_upload_alert_delete(int success, char *token);

// 上报闹钟/提醒振铃的事件
void tvs_executor_upload_alert_start(char *token);

// 上报闹钟/提醒停止振铃的事件
void tvs_executor_upload_alert_stop(char *token);

int tvs_executor_start_iplist(bool check_timeout);

void tvs_executor_start_ping();

int tvs_is_executor_valid_ex(int cmd, bool check_for_auth);
//异常上报
void tvs_executor_start_exception_report(exception_report_type type, const char *message);


#ifdef __cplusplus
}
#endif
#endif
