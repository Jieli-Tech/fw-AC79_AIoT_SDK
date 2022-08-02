#include <string.h>
#include <stdlib.h>
#include "cJSON_common/cJSON.h"
#include "iot.h"
#include "iot_parse.h"
#include "server/ai_server.h"
#include "dui.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif

#define xerror printf

#define IOT_DEVICE_DO               "do"
#define IOT_DEVICE_DATA             "data"
#define IOT_DEVICE_DATA_VOLUME      "volume"
#define IOT_DEVICE_DATA_PROGRESS    "progress"
#define IOT_DEVICE_DATA_MODE        "model"
#define IOT_DEVICE_DATA_URL         "url"
#define IOT_DEVICE_DATA_SONG        "song"
#define IOT_DEVICE_DATA_ALBUM       "album"
#define IOT_DEVICE_DATA_ARTIST      "artist"
#define IOT_DEVICE_DATA_IMG         "img"

typedef int (*do_action_callback_t)(const cJSON *pInput);

typedef struct {
    char                  *command;
    do_action_callback_t  do_action;
} iot_do_action_t;

extern char *strdup(const char *str);

static int iot_do_device_play(const cJSON *pInput)
{
    cJSON *url = cJSON_GetObjectItem(pInput, IOT_DEVICE_DATA_URL);
    cJSON *song = cJSON_GetObjectItem(pInput, IOT_DEVICE_DATA_SONG);
    cJSON *artist = cJSON_GetObjectItem(pInput, IOT_DEVICE_DATA_ARTIST);
    cJSON *img = cJSON_GetObjectItem(pInput, IOT_DEVICE_DATA_IMG);
    cJSON *album = cJSON_GetObjectItem(pInput, IOT_DEVICE_DATA_ALBUM);

    if (!url || !url->valuestring || !strlen(url->valuestring)) {
        return -1;
    }

    media_item_t *media = (media_item_t *)calloc(1, sizeof(media_item_t));
    if (!media) {
        return -1;
    }

    media->linkUrl = strdup(url->valuestring);
    if (!media->linkUrl) {
        free(media);
        return -1;
    }

    if (song && song->valuestring && strlen(song->valuestring)) {
        media->title = strdup(song->valuestring);
    }
    if (img && img->valuestring && strlen(img->valuestring)) {
        media->imageUrl = strdup(img->valuestring);
    }
    if (album && album->valuestring && strlen(album->valuestring)) {
        media->album = strdup(album->valuestring);
    }
    if (artist && artist->valuestring && strlen(artist->valuestring)) {
        media->subTitle = strdup(artist->valuestring);
    }

    dui_iot_music_play(url->valuestring, media);

    return 0;
}

static int iot_do_device_play_prev(const cJSON *pInput)
{
    return dui_net_music_play_prev();
}

static int iot_do_device_play_next(const cJSON *pInput)
{
    return dui_net_music_play_next();
}

static int iot_do_device_play_pause(const cJSON *pInput)
{
    dui_media_audio_pause_play(NULL);
    return iot_ctl_play_pause();
}

static int iot_do_device_play_resume(const cJSON *pInput)
{
    dui_media_audio_continue_play(NULL);
    return iot_ctl_play_resume();
}

static int iot_do_device_play_mode(const cJSON *pInput)
{
    cJSON *mode = cJSON_GetObjectItem(pInput, IOT_DEVICE_DATA_MODE);

    if (mode && mode->valuestring && strlen(mode->valuestring) > 0) {
        dui_media_music_mode_set(atoi(mode->valuestring));
        return iot_ctl_play_mode(atoi(mode->valuestring));
    }

    return -1;
}

static int iot_do_device_play_progress(const cJSON *pInput)
{
    cJSON *progress = cJSON_GetObjectItem(pInput, IOT_DEVICE_DATA_PROGRESS);

    if (progress) {
        dui_media_audio_play_seek(progress->valueint);
        return iot_ctl_play_progress(progress->valueint);
    }

    return -1;
}

static int iot_do_device_play_volume(const cJSON *pInput)
{
    cJSON *volume = cJSON_GetObjectItem(pInput, IOT_DEVICE_DATA_VOLUME);

    if (volume) {
        dui_volume_change_notify(volume->valueint);
        return iot_ctl_play_volume(volume->valueint);
    }

    return -1;
}

static int iot_do_device_add_collect(const cJSON *pInput)
{
    return dui_tone_tts_play(TTS_COLLECT_OK);
}

static int iot_do_device_remove_collect(const cJSON *pInput)
{
    return dui_tone_tts_play(TTS_COLLECT_CANCEL);
}

static int iot_do_device_collect_none(const cJSON *pInput)
{
    return dui_tone_tts_play(TTS_COLLECT_NONE);
}

static int iot_do_device_error(const cJSON *pInput)
{
    cJSON *msg = cJSON_GetObjectItem(pInput, "msg");
    cJSON *code = cJSON_GetObjectItem(pInput, "code");

    if (msg && code && code->valuestring && msg->valuestring) {
        printf("error code : %s, error msg : %s\n", code->valuestring, msg->valuestring);
    }

    return 0;
}

static int iot_do_device_change(const cJSON *pInput)
{
    cJSON *change_key = cJSON_GetObjectItem(pInput, "change_key");

    if (change_key && change_key->valuestring) {
        printf("change_key : %s\n", change_key->valuestring);
    }

    return 0;
}

static int iot_do_device_state(const cJSON *pInput)
{
    media_item_t item = {0};
    if (0 != dui_media_music_item(&item)) {
        item.album = item.imageUrl = item.linkUrl = item.subTitle = item.title = "";
        item.duration = 0;
    }

    extern void set_alarm_notify_flag(u8 value);
    set_alarm_notify_flag(1);
    DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_ALARM_SYNC);
    return iot_ctl_state(&item);
}

static int iot_do_device_upgrade(const cJSON *pInput)
{
    cJSON *version_json = cJSON_GetObjectItem(pInput, "version");
    cJSON *url_json = cJSON_GetObjectItem(pInput, "url");
    if (!version_json || !url_json) {
        return -1;
    }

    if (url_json->valuestring != NULL && version_json->valuestring != NULL) {
#if 0
        printf("upgrade url     = %s", url_json->valuestring);
        printf("upgrade version = %s", version_json->valuestring);

        dui_event_notify(AI_SERVER_EVENT_UPGRADE, NULL);
        if (get_update_data(url_json->valuestring)) {
            dui_event_notify(AI_SERVER_EVENT_UPGRADE_FAIL, NULL);
            return -1;
        }
        dui_event_notify(AI_SERVER_EVENT_UPGRADE_SUCC, version_json->valuestring);
#endif
        //通知升级成功
        iot_ctl_ota_upgrade();
    }

    return 0;
}

static int iot_do_device_child_identification(const cJSON *pInput)
{
    cJSON *state = cJSON_GetObjectItem(pInput, "state");

    if (state) {
        return iot_ctl_child_identification(state->valueint);
    }

    return 0;
}

static int iot_do_device_wake_up(const cJSON *pInput)
{
    cJSON *state = cJSON_GetObjectItem(pInput, "state");

    if (state) {
        return iot_ctl_wakeup(state->valueint);
    }
    return 0;
}

static int iot_do_device_message_in(const cJSON *pInput)
{
    //设备申请语音消息
    return iot_speech_play();
}

static int iot_do_device_play_wechat(const cJSON *pInput)
{
    cJSON *remainCount  = cJSON_GetObjectItem(pInput, "remainCount");
    cJSON *url  = cJSON_GetObjectItem(pInput, "url");
    if (remainCount) {
        printf("\n remainCount = %d\n", remainCount->valueint);
    }
    if (url && url->valuestring) {
        dui_iot_media_audio_play(url->valuestring);
    }
    return 0;
}

static int iot_do_device_chat_unread(const cJSON *pInput)
{
    return 0;
}

static int iot_do_device_im_account(const cJSON *pInput)
{
    return 0;
}

static int iot_do_device_unread(const cJSON *pInput)
{
    return 0;
}

static int iot_do_device_query_unread(const cJSON *pInput)
{
    return 0;
}

static int iot_do_device_speech_none(const cJSON *pInput)
{
    return 0;
}

static int iot_do_device_chat_records(const cJSON *pInput)
{
    return 0;
}

static int iot_do_device_triger_intent(const cJSON *pInput)
{
    cJSON *intent = cJSON_GetObjectItem(pInput, "intent");
    cJSON *slots = cJSON_GetObjectItem(pInput, "slots");

    if (intent && slots && intent->valuestring && slots->valuestring) {
        char *msg1 = malloc(strlen(intent->valuestring) + 1);
        char *msg2 = malloc(strlen(slots->valuestring) + 1);
        strcpy(msg1, intent->valuestring);
        strcpy(msg2, slots->valuestring);
        os_taskq_post("dui_net_task", 3, DUI_ALARM_OPERATE, msg1, msg2);
    }

    return 0;
}

static const iot_do_action_t iot_do_action_list[] = {
    {"device_play",                 iot_do_device_play},
    {"device_prev",                 iot_do_device_play_prev},
    {"device_next",                 iot_do_device_play_next},
    {"device_suspend",              iot_do_device_play_pause},
    {"device_continue_play",        iot_do_device_play_resume},
    {"device_play_mode",            iot_do_device_play_mode},
    {"device_drag",                 iot_do_device_play_progress},
    {"device_sound",                iot_do_device_play_volume},
    {"device_box_like_return",      iot_do_device_add_collect},
    {"device_box_cancel_return",    iot_do_device_remove_collect},
    {"device_like_null",            iot_do_device_collect_none},
    {"device_error",                iot_do_device_error},
    {"device_change",               iot_do_device_change},
    {"device_wake_up",              iot_do_device_wake_up},
    {"device_state",                iot_do_device_state},
    {"device_box_upgrade",          iot_do_device_upgrade},
    {"device_child_identification", iot_do_device_child_identification},
    //TODO
    {"device_message_in",           iot_do_device_message_in},
    {"device_play_wechat",          iot_do_device_play_wechat},
    {"device_chat_unread",          iot_do_device_chat_unread},
    {"device_im_account",           iot_do_device_im_account},
    {"device_unread",               iot_do_device_unread},
    {"device_query_unread",         iot_do_device_query_unread},
    {"device_speech_none",          iot_do_device_speech_none},
    {"device_chat_records",         iot_do_device_chat_records},

    //闹钟
    {"device_triger_intent",        iot_do_device_triger_intent},
};

int dui_iot_parse_process(const char *msg)
{
    int err = -1;
    cJSON *root = NULL, *action = NULL, *data = NULL;

    root = cJSON_Parse(msg);

    if (root) {
        action = cJSON_GetObjectItem(root, IOT_DEVICE_DO);
        data = cJSON_GetObjectItem(root, IOT_DEVICE_DATA);
    } else {
        xerror("iot json invalid\n");
        err = -1;
    }

    if (action && data) {
        for (int i = 0; i < sizeof(iot_do_action_list) / sizeof(iot_do_action_list[0]); i++) {
            if (0 == strcmp(iot_do_action_list[i].command, action->valuestring)) {
                printf("%lu : %s\n", strlen(msg), action->valuestring);
                err = iot_do_action_list[i].do_action(data);
                if (err < 0) {
                    xerror("iot do action [%d] %p failed\n", i, iot_do_action_list[i].do_action);
                }
                break;
            }
        }
    } else {
        xerror("iot action or data invalid\n");
        err = -2;
    }

    cJSON_Delete(root);

    return err;
}

