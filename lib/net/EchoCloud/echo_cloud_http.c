#include <string.h>
#include "server/ai_server.h"
#include "echo_cloud.h"
#include "echo_random.h"
#include "json_c/json_tokener.h"

#define ECHO_CLOUD_STREAM_URL       "http://api.echocloud.com/stream"
#define ECHO_CLOUD_MSG_URL	        "http://api.echocloud.com/entry"

typedef void (*funcptr)(json_object *, void *);

typedef struct _jsonc_handle {
    const char *rsp_msg;
    funcptr func;
} Jsonc_handle;

static const char echo_cloud_enc_http_tab[] =
    "POST /stream HTTP/1.1\r\n"
    "Host: api.echocloud.com\r\n"
    "Connection: close\r\n"
    "Transfer-Encoding: chunked\r\n"
    "X-Echocloud-Version: 3\r\n"
    "X-Echocloud-Channel-Uuid: %s\r\n"
    "X-Echocloud-Device-ID: %s\r\n"
    "X-Echocloud-Nonce: %s\r\n"
    "X-Echocloud-Signature: %s\r\n"
    "X-Echocloud-Type: %s\r\n"
    "Content-type: audio/amr;bit=16;rate=8000;frame=32\r\n"
    "\r\n";

static const char echo_cloud_msg_http_tab[] =
    "POST /entry HTTP/1.1\r\n"
    "Host: api.echocloud.com\r\n"
#ifdef HTTP_KEEP_ALIVE
    "Connection: keep-alive\r\n"
#else
    "Connection: close\r\n"
#endif
    "X-Echocloud-Version: 3\r\n"
    "Content-type: application/json\r\n"
    "Content-Length: %lu\r\n"
    "\r\n";

static const char *const msg_type_name_array[] = {
    "device.log",
    "device.heartbeat",
    "device.online",
    "device.offline",

    "bandwidth.changed",
    "volume.changed",
    "light.changed",
    "battery.changed",
    "battery.charge_started",
    "battery.charge_ended",

    "media_player.started",
    "media_player.finished",
    "media_player.stopped",
    "media_player.paused",
    "media_player.resumed",
    "media_player.progress_changed",

    "ai_dialog.started",
    "ai_dialog.finished",
    "ai_dialog.stopped",

    "intercom.started",
    "intercom.finished",
    "intercom.stopped",
    "intercom.alerted",

    "alert.scheduled",
    "alert.canceled",
    "alert.triggered",
    "alert.finished",
    "alert.terminated",

    "child_lock.changed",

    "firmware.upgrade_started",
    "firmware.upgrade_succeeded",
    "firmware.upgrade_failed",

    "wifi.soundwave_configuration_succeeded",

    "ai_dialog.input.device",

    "intercom.receive.device",
    "device.request_code.device",

    "volume.change.device",
    "media_player.pause.device",
    "media_player.resume.device",
    "media_player.previous.device",
    "media_player.next.device",
    "media_player.favorite.device",
    "media_player.play_favorite.device",

    "child_lock.change.device",
    "media_player.add_favorite.device",
    "media_player.delete_favorite.device",
    "custom.event.device",
    "media_player.play_custom_art_lists.device",
    "playlist_mode.change.device",
    "skill.enter.device",
    "month_age.get.device",
};


static const char *cmd_to_msg_type(DEVICE_MSG_TYPE cmd)
{
    if (cmd < MAX_DEVICE_MSG) {
        return msg_type_name_array[cmd];
    }

    return "";
}

static struct json_object *echo_cloud_create_cjson(struct echo_cloud_http_hdl *hdl, DEVICE_MSG_TYPE cmd, json_object *send_data)
{
    char nonce[33];
    char signature_str[65];
    avGenRand(nonce, 32);
    nonce[32] = 0;
    echo_cloud_get_hmac_sha256(hdl->auth->device_id, hdl->auth->channel_uuid, nonce, signature_str, hdl->auth->hash);
    signature_str[64] = 0;

    struct json_object *root = json_object_new_object();
    struct json_object *message = json_object_new_object();
    struct json_object *meta = json_object_new_object();

    json_object_object_add(root, "nonce", json_object_new_string(nonce));
    json_object_object_add(root, "signature", json_object_new_string(signature_str));
    json_object_object_add(root, "message", message);
    json_object_object_add(root, "meta", meta);
    json_object_object_add(meta, "channel_uuid", json_object_new_string(hdl->auth->channel_uuid));
    json_object_object_add(meta, "device_id", json_object_new_string(hdl->auth->device_id));
    json_object_object_add(message, "data", send_data);
    json_object_object_add(message, "type", json_object_new_string(cmd_to_msg_type(cmd)));

    return root;
}

static void aiDialogPlayHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *tmp = NULL;

    tmp = json_object_object_get(data, "audio_enabled");
    if (tmp) {
        if (FALSE == json_object_get_boolean(tmp)) {
            return;
        }
    }

    tmp = json_object_object_get(data, "token");
    if (tmp) {
        strcpy(p->token.ai_dialog_token, json_object_get_string(tmp));
    }

    tmp = json_object_object_get(data, "audio_url");
    if (tmp &&
        p->media_play_opt != MEDIA_PLAY_IMMEDIATELY &&
        p->media_play_opt != MEDIA_IGNORE_SPEEK) {
        JL_echo_cloud_media_speak_play(json_object_get_string(tmp));
        /* echo_cloud_enc_http_close(&p->http); */
        /* json_object *cmd_data = json_object_new_object(); */
        /* json_object_object_add(cmd_data, "token", json_object_new_string(p->token.ai_dialog_token)); */
        /* echo_cloud_msg_http_req(&p->http, AI_DIALOG_STARTED, cmd_data); */
    }
}

static void intercomAlertHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *tmp = NULL;

    tmp = json_object_object_get(data, "audio_url");
    if (tmp) {
        /* JL_echo_cloud_media_speak_play(json_object_get_string(tmp)); */
    }
}

static void intercomNotifyHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *tmp = NULL;

    tmp = json_object_object_get(data, "token");
    if (tmp) {
        strcpy(p->token.intercom_notify_token, json_object_get_string(tmp));
    }
    tmp = json_object_object_get(data, "alert_url");
    if (tmp) {
        /* JL_echo_cloud_media_speak_play(json_object_get_string(tmp)); */
    }
    tmp = json_object_object_get(data, "audio_url");
    if (tmp) {
        JL_echo_cloud_wechat_speak_play(json_object_get_string(tmp));
    }
}

static void mediaPlayerStartHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *tmp = NULL;

    tmp = json_object_object_get(data, "audio_enabled");
    if (tmp) {
        if (FALSE == json_object_get_boolean(tmp)) {
            return;
        }
    }

    tmp = json_object_object_get(data, "token");
    if (tmp) {
        strcpy(p->token.media_play_token, json_object_get_string(tmp));
    }

    tmp = json_object_object_get(data, "audio_url");
    if (tmp) {
        if (json_object_get_string_len(tmp) + 1 > p->media_url_len) {
            free(p->media_url);
            p->media_url_len = json_object_get_string_len(tmp) + 1;
            p->media_url = (char *)malloc(p->media_url_len);
            if (!p->media_url) {
                return;
            }
        }

        strcpy(p->media_url, json_object_get_string(tmp));
        if (p->media_play_opt == MEDIA_PLAY_IMMEDIATELY || p->media_play_opt == MEDIA_IGNORE_SPEEK) {
            if (!p->is_record) {
                JL_echo_cloud_media_audio_play(p->media_url);
                p->media_play_state = ECHO_CLOUD_MEDIA_STATE_START;
                json_object *cmd_data = json_object_new_object();
                json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
                echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_STARTED, cmd_data);
            }
        } else {
            p->media_play_opt = MEDIA_WAIT_PLAY;
        }
    }
}

static void mediaPlayerStopHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;

    if (p->media_play_state != ECHO_CLOUD_MEDIA_STATE_STOP) {
        JL_echo_cloud_media_audio_pause(NULL);
        p->media_play_state = ECHO_CLOUD_MEDIA_STATE_STOP;
        json_object *cmd_data = json_object_new_object();
        json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
        json_object_object_add(cmd_data, "offset", json_object_new_int(1000 * get_app_music_playtime()));
        echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_STOPPED, cmd_data);
    }
}

static void mediaPlayerPauseHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;

    if (p->media_play_state == ECHO_CLOUD_MEDIA_STATE_START) {
        JL_echo_cloud_media_audio_pause(NULL);
        p->media_play_state = ECHO_CLOUD_MEDIA_STATE_PAUSE;
        json_object *cmd_data = json_object_new_object();
        json_object_object_add(cmd_data, "token", json_object_new_string(p->token.media_play_token));
        json_object_object_add(cmd_data, "offset", json_object_new_int(1000 * get_app_music_playtime()));
        echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_PAUSED, cmd_data);
    }
}

static void mediaPlayerResumeHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;

    if (p->media_play_state == ECHO_CLOUD_MEDIA_STATE_PAUSE) {
        JL_echo_cloud_media_audio_continue(NULL);
        p->media_play_state = ECHO_CLOUD_MEDIA_STATE_START;
        json_object *cmd_data = json_object_new_object();
        echo_cloud_msg_http_req(&p->http, MEDIA_PLAYER_RESUMED, cmd_data);
    }
}

static void VolumeChangeHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *volume = json_object_object_get(data, "volume");

    if (volume) {
        JL_echo_cloud_volume_change_notify(json_object_get_int(volume));
        json_object *cmd_data = json_object_new_object();
        json_object_object_add(cmd_data, "volume", json_object_new_int(json_object_get_int(volume)));
        echo_cloud_msg_http_req(&p->http, VOLUME_CHANGED, cmd_data);
    }
}

static void alertTriggerHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;

    json_object *tmp = json_object_object_get(data, "token");
    if (tmp) {
        strcpy(p->token.alert_schedule_token, json_object_get_string(tmp));
    }
    tmp = json_object_object_get(data, "audio_urls");
    if (tmp) {
        json_object *val = json_object_array_get_idx(tmp, 1);
        if (val) {
            p->media_play_state = ECHO_CLOUD_MEDIA_STATE_STOP;
            JL_echo_cloud_media_speak_play(json_object_get_string(val));
            return;
        }

        val = json_object_array_get_idx(tmp, 0);
        if (val) {
            p->media_play_state = ECHO_CLOUD_MEDIA_STATE_STOP;
            JL_echo_cloud_media_speak_play(json_object_get_string(val));
        }
    }
}

static void FirmwareUpgradeHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *version = json_object_object_get(data, "version");
    json_object *checksum = json_object_object_get(data, "checksum");
    json_object *cmd_data = NULL;
    json_object *url = json_object_object_get(data, "url");

    if (!version || !checksum || !url) {
        return;
    }

    if (0 != check_echo_cloud_firmware_version(json_object_get_string(url), json_object_get_string(version), json_object_get_string(checksum))) {
        return;
    }

    cmd_data = json_object_new_object();
    echo_cloud_msg_http_req(&p->http, FIRMWARE_UPGRADE_STARTED, cmd_data);

    p->media_play_state = ECHO_CLOUD_MEDIA_STATE_STOP;

    echo_cloud_msg_http_close(&p->http);
    int error = echo_cloud_get_firmware_data(&p->http.ctx, json_object_get_string(url), json_object_get_string(version));
    if (0 == error) {
        cmd_data = json_object_new_object();
        echo_cloud_msg_http_req(&p->http, FIRMWARE_UPGRADE_SUCCEEDED, cmd_data);
    } else {
        char error_code[32];
        snprintf(error_code, sizeof(error_code), "error code is %d", error);
        cmd_data = json_object_new_object();
        json_object_object_add(cmd_data, "log", json_object_new_string(error_code));
        echo_cloud_msg_http_req(&p->http, FIRMWARE_UPGRADE_FAILED, cmd_data);
    }
}

static void ChildLockHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *child_lock = json_object_object_get(data, "child_lock");

    if (child_lock) {
        JL_echo_cloud_child_lock_change_notify(json_object_get_boolean(child_lock));
        json_object *cmd_data = json_object_new_object();
        json_object_object_add(cmd_data, "child_lock", json_object_new_boolean(json_object_get_boolean(child_lock)));
        echo_cloud_msg_http_req(&p->http, CHILD_LOCK_CHANGED, cmd_data);
    }
}

static void LightChangeHandle(json_object *data, void *priv)
{
    struct echo_cloud_hdl *p = (struct echo_cloud_hdl *)priv;
    json_object *light = json_object_object_get(data, "light");

    if (light) {
        JL_echo_cloud_light_change_notify(json_object_get_int(light));
        json_object *cmd_data = json_object_new_object();
        json_object_object_add(cmd_data, "light", json_object_new_int(json_object_get_int(light)));
        echo_cloud_msg_http_req(&p->http, LIGHT_CHANGED, cmd_data);
    }
}

static const Jsonc_handle jsonc_handle[] = {
    {"ai_dialog.play",  		aiDialogPlayHandle},
    {"intercom.alert", 			intercomAlertHandle},
    {"intercom.play", 			NULL/*intercomPlayHandle*/},
    {"intercom.notify", 		intercomNotifyHandle},
    {"media_player.start", 		mediaPlayerStartHandle},
    {"media_player.stop", 		mediaPlayerStopHandle},
    {"media_player.pause", 		mediaPlayerPauseHandle},
    {"media_player.resume", 	mediaPlayerResumeHandle},
    {"alert.trigger", 			alertTriggerHandle},
#if 0
    {"alert.schedule", 			alertScheduleHandle},
    {"alert.cancel", 			alertCancelHandle},
    {"alert.terminate", 		alertTerminateHandle},
#endif
    {"volume.change", 			VolumeChangeHandle},
    {"firmware.upgrade", 		FirmwareUpgradeHandle},
    {"child_lock.change", 		ChildLockHandle},
    {"light.change", 			LightChangeHandle},
#if 0
    {"device.purge", 			NULL/*DevicePurgeHandle*/},
    {"device.sleep", 			NULL/*DeviceSleepHandle*/},
    {"device.output_verification_code",	NULL},
    {"custom.command",					NULL},
    {"vad.start",						NULL},
    {"month_age.send",					NULL},
    {"device.bind_succeeded",			NULL},
#endif
};

int echo_cloud_rcv_json_handler(void *priv, const char *rsp_data)
{
    int ret = 0;

    printf("rsp : %s\n", rsp_data);
    json_object *recvJson = json_tokener_parse(rsp_data);
    if (NULL == recvJson) {
        return -1;
    }

    json_object *val = json_object_object_get(recvJson, "count");
    if (NULL == val) {
        json_object_put(recvJson);
        return -1;
    }

    int count = json_object_get_int(val);
    if (0 == count) {
        json_object_put(recvJson);
        return -1;
    }

    json_object *msgJson = json_object_object_get(recvJson, "messages");
    if (NULL == msgJson) {
        json_object_put(recvJson);
        return -1;
    }

    for (int i = 0; i < count; i++) {
        val = json_object_array_get_idx(msgJson, i);
        json_object *type = json_object_object_get(val, "type");
        if (NULL == type) {
            continue;
        }
        json_object *data = json_object_object_get(val, "data");
        if (NULL == data) {
            continue;
        }
        for (int j = 0; j < ARRAY_SIZE(jsonc_handle); j++) {
            if (jsonc_handle[j].func && !strcmp(json_object_get_string(type), jsonc_handle[j].rsp_msg)) {
                printf("handle : %s\n", jsonc_handle[j].rsp_msg);
                jsonc_handle[j].func(data, priv);
                break;
            }
        }
    }

    json_object_put(recvJson);

    return ret;
}

int echo_cloud_enc_http_start(struct echo_cloud_http_hdl *hdl, u8 voice_mode)
{
    int error = HERROR_OK;
    char *http_head = NULL;

    memset(&hdl->rec_ctx,  0x0, sizeof(httpcli_ctx));
    memset(&hdl->rec_body, 0x0, sizeof(http_body_obj));

    hdl->rec_body.recv_len  = 0;
    hdl->rec_body.buf_len   = 1024; // 接受buf大小，根据需求设置
    hdl->rec_body.buf_count = 1;
    hdl->rec_body.p = (char *) malloc(hdl->rec_body.buf_len * sizeof(char));

    if (hdl->rec_body.p == NULL) {
        return HERROR_MEM;
    }

    hdl->rec_ctx.url        = ECHO_CLOUD_STREAM_URL;     // 链接地址
    hdl->rec_ctx.timeout_millsec = hdl->timeout_ms;     // 连接超时设置

    // 设置一些http头部参数。如果设置了user_http_header参数，
    // httpcli_post()或者heepcli_get()内部将不在设置http头部

    http_head = malloc(1024);
    if (http_head == NULL) {
        error = HERROR_MEM;
        goto __exit;
    }

    char nonce[33];
    char signature_str[65];
    avGenRand(nonce, 32);
    nonce[32] = 0;
    echo_cloud_get_hmac_sha256(hdl->auth->device_id, hdl->auth->channel_uuid, nonce, signature_str, hdl->auth->hash);
    signature_str[64] = 0;

    if (AI_MODE == voice_mode) {
        snprintf(http_head, 1024, echo_cloud_enc_http_tab,
                 hdl->auth->channel_uuid,
                 hdl->auth->device_id,
                 nonce,
                 signature_str,
                 "ai_dialog.recognize.device"
                );
    } else if (TRANSLATE_MODE == voice_mode) {
        int length = snprintf(http_head, 1024, echo_cloud_enc_http_tab,
                              hdl->auth->channel_uuid,
                              hdl->auth->device_id,
                              nonce,
                              signature_str,
                              "translator.input.device"
                             );
        snprintf(http_head + length - 2, 1024 - length, "%s%s",
                 "X-Echocloud-Payload-From: zh\r\n",
                 "X-Echocloud-Payload-To: en\r\n\r\n"
                );
    } else if (WECHAT_MODE == voice_mode) {
        snprintf(http_head, 1024, echo_cloud_enc_http_tab,
                 hdl->auth->channel_uuid,
                 hdl->auth->device_id,
                 nonce,
                 signature_str,
                 "intercom.input.device"
                );
    }

    hdl->rec_ctx.user_http_header = http_head;
    hdl->rec_ctx.priv   = &hdl->rec_body;

    error = httpcli_post_header(&hdl->rec_ctx);

__exit:
    if (error) {
        httpcli_close(&hdl->rec_ctx);
        if (hdl->rec_body.p) {
            free(hdl->rec_body.p);
            hdl->rec_body.p = NULL;
        }
    }
    if (http_head) {
        free(http_head);
    }

    return error;
}

int echo_cloud_enc_http_send(struct echo_cloud_http_hdl *hdl, void *data, u32 data_len)
{
    int error = HERROR_OK;

    if (hdl->rec_body.p == NULL) {
        return HERROR_MEM;
    }

    hdl->rec_ctx.post_data  = data;
    hdl->rec_ctx.data_len   = data_len;

    error = httpcli_chunked_send(&hdl->rec_ctx);
    if (error == HERROR_OK && !data_len) {
        if (hdl->out_cb) {
            error = hdl->out_cb(hdl->priv, hdl->rec_body.p);
        }
    }

    return error;
}

int echo_cloud_enc_http_close(struct echo_cloud_http_hdl *hdl)
{
    httpcli_close(&hdl->rec_ctx);

    if (hdl->rec_body.p) {
        free(hdl->rec_body.p);
        hdl->rec_body.p = NULL;
    }

    return 0;
}

int echo_cloud_msg_http_close(struct echo_cloud_http_hdl *hdl)
{
    httpcli_close(&hdl->ctx);

    if (hdl->body.p) {
        free(hdl->body.p);
        hdl->body.p = NULL;
    }

    return 0;
}

int echo_cloud_msg_http_req(struct echo_cloud_http_hdl *hdl, int cmd, json_object *cmd_data)
{
    struct json_object *root = NULL;
    const char *send_data = NULL;
    int error = HERROR_OK;
    char http_head[256];
    u8 first_try = 1;

#ifdef HTTP_KEEP_ALIVE
    if (hdl->body.p) {
        free(hdl->body.p);
        hdl->body.p = NULL;
    }
#else
    echo_cloud_msg_http_close(hdl);
#endif

    memset(&hdl->body, 0x0, sizeof(http_body_obj));
    hdl->body.recv_len  = 0;
    hdl->body.buf_len   = 1024; // 接受buf大小，根据需求设置
    hdl->body.buf_count = 1;
    hdl->body.p = (char *) malloc(hdl->body.buf_len * sizeof(char));

    if (hdl->body.p == NULL) {
        error = HERROR_MEM;
        goto __exit;
    }

    root = echo_cloud_create_cjson(hdl, cmd, cmd_data);
    if (!root) {
        goto __exit;
    }

    send_data = json_object_to_json_string(root);
    snprintf(http_head, sizeof(http_head), echo_cloud_msg_http_tab, strlen(send_data));
    printf("request data : %s\n", send_data);

__req_again:

#ifdef HTTP_KEEP_ALIVE
    if (hdl->ctx.sock_hdl == NULL) {
        hdl->body.recv_len  = 0;
        memset(&hdl->ctx,  0x0, sizeof(httpcli_ctx));
        hdl->ctx.url        = ECHO_CLOUD_MSG_URL;     // 链接地址
        hdl->ctx.timeout_millsec = hdl->timeout_ms;     // 连接超时设置
        hdl->ctx.priv   = &hdl->body;
        error = httpcli_post_keepalive_init(&hdl->ctx);
        if (error != HERROR_OK) {
            goto __exit;
        }
    }
#else
    memset(&hdl->ctx,  0x0, sizeof(httpcli_ctx));
    hdl->ctx.url        = ECHO_CLOUD_MSG_URL;     // 链接地址
    hdl->ctx.timeout_millsec = hdl->timeout_ms;     // 连接超时设置
    hdl->ctx.priv   = &hdl->body;
#endif

    hdl->ctx.user_http_header = http_head;
    hdl->ctx.post_data  = send_data;
    hdl->ctx.data_len   = strlen(send_data);

#ifdef HTTP_KEEP_ALIVE
    error = httpcli_post_keepalive_send(&hdl->ctx);
#else
    error = httpcli_post(&hdl->ctx);
#endif
    if (error == HERROR_OK) {
        if (hdl->out_cb) {
            error = hdl->out_cb(hdl->priv, hdl->body.p);
        }
#ifdef HTTP_KEEP_ALIVE
    } else {
        httpcli_close(&hdl->ctx);
        if (first_try && error != HERROR_CALLBACK) {
            first_try = 0;
            goto __req_again;
        }
#endif
    }

__exit:
    if (hdl->body.p) {
        free(hdl->body.p);
        hdl->body.p = NULL;
    }
    if (cmd_data) {
        json_object_put(cmd_data);
    }
    if (root) {
        json_object_put(root);
    }

    return error;
}

