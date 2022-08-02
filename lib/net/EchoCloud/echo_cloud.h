#ifndef _ECHO_CLOUD_H_
#define _ECHO_CLOUD_H_

#include "generic/typedef.h"
#include "http/http_cli.h"
#include "json_c/json_tokener.h"

#define HTTP_KEEP_ALIVE

#define WECHAT_ID_LEN					32
#define DEVICE_ID_LEN					32
#define PASSWORD_LEN					64
#define UUID_LEN						64
#define NONCE_LEN						32
#define HASH_LEN						64
#define AUTH_KEY_LEN					32
#define CODE_LEN						32
#define TOKEN_LENGTH					64
#define USERNAME_LEN					(UUID_LEN + DEVICE_ID_LEN + NONCE_LEN + 2)


struct echo_cloud_auth_parm {
    char device_id[DEVICE_ID_LEN + 1];
    char channel_uuid[UUID_LEN + 1];
    char hash[HASH_LEN + 1];
};

struct httpc_token_info {
    char media_play_token[TOKEN_LENGTH + 1];
    char ai_dialog_token[TOKEN_LENGTH + 1];
    char intercom_token[TOKEN_LENGTH + 1];
    char intercom_notify_token[TOKEN_LENGTH + 1];
    char alert_schedule_token[TOKEN_LENGTH + 1];
};

struct echo_cloud_http_hdl {
    struct echo_cloud_auth_parm *auth;
    void *priv;
    int (*out_cb)(void *priv, const char *data);
    u32 timeout_ms;
    httpcli_ctx     ctx;
    httpcli_ctx     rec_ctx;
    http_body_obj   body;
    http_body_obj   rec_body;
};

struct echo_cloud_hdl {
    struct echo_cloud_http_hdl http;
    struct echo_cloud_auth_parm auth;
    struct httpc_token_info token;
    int heartbeat_timer;
    int sync_timer;
    u8 is_record;
    u8 media_play_opt;
    u8 media_play_state;
    char *media_url;
    u32 media_url_len;
};

enum echo_cloud_sdk_events {
    ECHO_CLOUD_SPEAK_END     = 0x01,
    ECHO_CLOUD_MEDIA_END     = 0x02,
    ECHO_CLOUD_PLAY_PAUSE    = 0x03,
    ECHO_CLOUD_PREVIOUS_SONG = 0x04,
    ECHO_CLOUD_NEXT_SONG     = 0x05,
    ECHO_CLOUD_VOLUME_CHANGE = 0X06,
    ECHO_CLOUD_VOLUME_INCR   = 0x07,
    ECHO_CLOUD_VOLUME_DECR   = 0x08,
    ECHO_CLOUD_VOLUME_MUTE   = 0x09,
    ECHO_CLOUD_RECORD_START  = 0x0a,
    ECHO_CLOUD_RECORD_SEND   = 0x0b,
    ECHO_CLOUD_RECORD_STOP   = 0x0c,
    ECHO_CLOUD_VOICE_MODE    = 0x0d,
    ECHO_CLOUD_MEDIA_STOP    = 0x0e,
    ECHO_CLOUD_BIND_DEVICE   = 0x0f,
    ECHO_CLOUD_RECORD_ERR    = 0x10,
    ECHO_CLOUD_COLLECT_RESOURCE = 0x11,
    ECHO_CLOUD_DEVICE_CODE_REPORT = 0x12,
    ECHO_CLOUD_CHILD_LOCK_CHANGE = 0x13,
    ECHO_CLOUD_MQTT_MSG_DEAL = 0x20,
    ECHO_CLOUD_QUIT    	     = 0xff,
};

typedef enum DEVICE_MSG_TYPE {
    //设备事件
    DEVICE_LOG,								//device.log
    DEVICE_HEARTBEAT,  						//device.heartbeat
    DEVICE_ONLINE,        					//device.online
    DEVICE_OFFLINE,       					//device.offline

    BANDWIDTH_CHANGED,						//bandwidth.changed
    VOLUME_CHANGED,							//volume.changed
    LIGHT_CHANGED,							//light.changed
    BATTERY_CHANGED,						//battery.changed
    BATTERY_CHARGE_STARTED,					//battery.charge_started
    BATTERY_CHARGE_ENDED,					//battery.charge_ended

    MEDIA_PLAYER_STARTED,					//media_player.started
    MEDIA_PLAYER_FINISHED,					//media_player.finished
    MEDIA_PLAYER_STOPPED,					//media_player.stopped
    MEDIA_PLAYER_PAUSED,					//media_player.paused
    MEDIA_PLAYER_RESUMED,					//media_player.resumed
    MEDIA_PLAYER_PROGRESS_CHANGER,			//media_player.progress_changed

    AI_DIALOG_STARTED,						//ai_dialog.started
    AI_DIALOG_FINISHED,						//ai_dialog.finished
    AI_DIALOG_STOPPED,						//ai_dialog.stopped

    INTERCOM_STARTED,						//intercom.started
    INTERCOM_FINISHED,						//intercom.finished
    INTERCOM_STOPPED,						//intercom.stopped
    INTERCOM_ALERTED, 						//intercom.alerted

    ALERT_SCHEDULED,						//alert.scheduled
    ALERT_CANCELED,							//alert.canceled
    ALERT_TRIGGERED,						//alert.triggered
    ALERT_FINISHED,							//alert.finished
    ALERT_TERMINATED,						//alert.terminated

    CHILD_LOCK_CHANGED,						//child_lock.changed

    FIRMWARE_UPGRADE_STARTED,				//firmware.upgrade_started
    FIRMWARE_UPGRADE_SUCCEEDED,				//firmware.upgrade_succeeded
    FIRMWARE_UPGRADE_FAILED,				//firmware.upgrade_failed

    SOUNDWAVE_CONF_SUCC,					//wifi.soundwave_configuration_succeeded

    //设备控制事件
    AI_DIALOG_INPUT,						//ai_dialog.input.device

    INTERCOM_RECEIVE_DEVICE,				//intercom.receive.device
    DEVICE_REQUEST_CODE,					//device.request_code.device

    VOLUME_CHANGE_DEVICE,					//volume.change.device
    MEDIA_PLAYER_PAUSE_DEVICE,				//media_player.pause.device
    MEDIA_PLAYER_RESUME_DEVICE,				//media_player.resume.device
    MEDIA_PLAYER_PREVIOUS_DEVICE,			//media_player.previous.device
    MEDIA_PLAYER_NEXT_DEVICE,				//media_player.next.device
    MEDIA_PLAYER_FAVORITE_DEVICE,			//media_player.favorite.device
    MEDIA_PLAYER_PLAY_FAVORITE_DEVICE,		//media_player.play_favorite.device

    CHILD_LOCK_CHANGE_DEVICE,				//child_lock.change.device
    MEDIA_ADD_FAVORITE,						//media_player.add_favorite.device
    MEDIA_DELETE_FAVORITE,					//media_player.delete_favorite.device
    CUSTOM_EVENT,							//custom.event.device
    PLAY_CUSTOM_ART_LISTS,					//media_player.play_custom_art_lists.device
    PLAYLIST_MODE_CHANGE,					//playlist_mode.change.device
    SKILL_ENTER,							//skill.enter.device
    GET_MONTH_AGE,							//month_age.get.device

    MAX_DEVICE_MSG,
} DEVICE_MSG_TYPE;

enum {
    MEDIA_WAIT_PLAY,
    MEDIA_HAS_PALY,
    MEDIA_PLAY_IMMEDIATELY,
    MEDIA_IGNORE_SPEEK,
};

enum {
    ECHO_CLOUD_MEDIA_STATE_STOP,
    ECHO_CLOUD_MEDIA_STATE_START,
    ECHO_CLOUD_MEDIA_STATE_PAUSE,
};

int echo_cloud_enc_http_start(struct echo_cloud_http_hdl *hdl, u8 voice_mode);

int echo_cloud_enc_http_send(struct echo_cloud_http_hdl *hdl, void *data, u32 data_len);

int echo_cloud_enc_http_close(struct echo_cloud_http_hdl *hdl);

int echo_cloud_msg_http_close(struct echo_cloud_http_hdl *hdl);

int echo_cloud_msg_http_req(struct echo_cloud_http_hdl *hdl, int cmd, json_object *cmd_data);

int echo_cloud_rcv_json_handler(void *priv, const char *rsp_data);

int echo_cloud_mqtt_init(struct echo_cloud_auth_parm *auth);

void echo_cloud_mqtt_uninit(void);

void JL_echo_cloud_wechat_speak_play(const char *url);

void JL_echo_cloud_media_speak_play(const char *url);

void JL_echo_cloud_media_audio_play(const char *url);

void JL_echo_cloud_media_audio_continue(const char *url);

void JL_echo_cloud_media_audio_pause(const char *url);

void JL_echo_cloud_upgrade_notify(int event, void *arg);

void JL_echo_cloud_volume_change_notify(int volume);

void JL_echo_cloud_wechat_speak_recv_notify(void);

void JL_echo_cloud_child_lock_change_notify(int value);

void JL_echo_cloud_light_change_notify(int value);

int get_app_music_playtime(void);

int echo_cloud_get_firmware_data(httpcli_ctx *ctx, const char *url, const char *version);

int check_echo_cloud_firmware_version(const char *url, const char *new_ver, const char *checksum);

const char *get_echo_cloud_firmware_version(void);

#endif
