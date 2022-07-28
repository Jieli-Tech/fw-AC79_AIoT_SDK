#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "os/os_api.h"
#include "system/database.h"
#include "turing_mqtt.h"
#include "turing_iot.h"
#include "event.h"
#include "app_core.h"
#include "fs/fs.h"
#include "turing.h"

struct wechat_html_file_info {
    char filename[256];
    char fileurl[1024];
    int album_id;
    u32 state;
    u32 message_type;
};

struct tl_iot_device_state {
    int vol;
    int battery;
    int sfree;
    int stotal;
    int shake;
    int power;
    int bln;
    int play;
    int charging;
    int lbi;
    int tcard;
};

enum {
    WECHAT_MUSIC_PLAYING = 0x1,
    WECHAT_MUSIC_PAUSE,
    WECHAT_MUSIC_STOP,
};

enum {
    MQTT_MESSAGE_CHAT = 0x0,
    MQTT_MESSAGE_AUDIO,
    MQTT_MESSAGE_CONTROL,
    MQTT_MESSAGE_NOTIFY
};

enum {
    MQTT_MESSAGE_MUSIC_PLAY = 0x1,
    MQTT_MESSAGE_MUSIC_STOP,
};

enum {
    CONCROL_VOL_DOWN = 0x0,
    CONCROL_VOL_UP,
    CONCROL_LED_CLOSE,
    CONCROL_LED_OPEN,
    CONCROL_LOW_POWER_OPEN,
    CONCROL_LOW_POWER_CLOSE,
    CONCROL_STROAGE_FORMAT,
    CONCROL_DEFAULT,
    CONCROL_SET_CLOCK,
    CONCROL_CALL_PHONE,
    CONCROL_SLEEP_TIME,
    CONCROL_SET_UP,
    CONCROL_SET_CLOSE_PLAYING,
};

struct __wechat_html_status {
    struct wechat_html_file_info cur;
    struct tl_iot_device_state dev_state;
};

static struct __wechat_html_status wechat_html_status;

#define __this (&wechat_html_status)


static int mqtt_init_state_noitfy(json_object *message)
{
    char buf[512];
#if 0
    u32 space = 0;
    struct vfs_partition *part = NULL;

    if (storage_device_ready() == 0) {
        /* dev_mqtt_push_status("storage", "不在线"); */
        __this->dev_state.tcard = 0;
        __this->dev_state.sfree = 0;
        __this->dev_state.stotal = 0;
    } else {
        part = fget_partition(CONFIG_ROOT_PATH);
        fget_free_space(CONFIG_ROOT_PATH, &space);

        __this->dev_state.tcard = 1;
        __this->dev_state.sfree = part->total_size - space;
        __this->dev_state.stotal = part->total_size;
    }
#endif

    __this->dev_state.vol = get_app_music_volume();

    {
        //没实现
        __this->dev_state.battery  = 100;
        __this->dev_state.shake = 1;//意义不明
        __this->dev_state.power = 0; //是否低电
        __this->dev_state.bln = 1; //是否开启灯
        __this->dev_state.play = 0;
        __this->dev_state.charging = 0;//是否正在充电中
        __this->dev_state.lbi  = 0;//是否开启低电提示
    }

    snprintf(buf, sizeof(buf),
             "\"vol\": %d,\"battery\": %d,\"sfree\": %d,\"stotal\": %d,\"shake\": %d,\"power\": %d,\"bln\": %d,\"play\": %d,\"charging\": %d,\"lbi\": %d,\"tcard\": %d"
             , __this->dev_state.vol
             , __this->dev_state.battery
             , __this->dev_state.sfree
             , __this->dev_state.stotal
             , __this->dev_state.shake
             , __this->dev_state.power
             , __this->dev_state.bln
             , __this->dev_state.play
             , __this->dev_state.charging
             , __this->dev_state.lbi
             , __this->dev_state.tcard);

    turing_wechat_state_noitfy(DEVICE_STATE, buf);

    return 0;
}

static int mqtt_message_chat(json_object *message)
{
    json_object *url = NULL;

    if (!json_object_object_get_ex(message, "url", &url)) {
        return -1;
    }

    const char *url_string = json_object_get_string(url);

    if (url_string && strlen(url_string) > 0) {
        turing_wechat_speak_play(url_string);
    }

    return 0;
}

static int mqtt_message_audio(json_object *message)
{
    json_object *url = NULL;
    json_object *operate = NULL;
    json_object *album_id = NULL;
    json_object *name = NULL;

    if (!json_object_object_get_ex(message, "arg", &name)) {
        return -1;
    }

    if (!json_object_get_string(name) || !json_object_get_string_len(name)) {
        return -1;
    }

    if (!json_object_object_get_ex(message, "url", &url)) {
        return -1;
    }

    if (!json_object_get_string(url) || !json_object_get_string_len(url)) {
        return -1;
    }

    if (!json_object_object_get_ex(message, "mediaId", &album_id)) {
        return -1;
    }

    if (!json_object_object_get_ex(message, "operate", &operate)) {
        return -1;
    }

    strncpy(__this->cur.filename, json_object_get_string(name), sizeof(__this->cur.filename));
    __this->cur.filename[sizeof(__this->cur.filename) - 1] = 0;

    strncpy(__this->cur.fileurl, json_object_get_string(url), sizeof(__this->cur.fileurl));
    __this->cur.fileurl[sizeof(__this->cur.fileurl) - 1] = 0;

    int music_state = json_object_get_int(operate);

    //to do something
    if (music_state == MQTT_MESSAGE_MUSIC_PLAY) {
        if (json_object_get_int(album_id) != __this->cur.album_id || __this->cur.state == WECHAT_MUSIC_STOP) {
            turing_wechat_media_audio_play(__this->cur.fileurl);
            __this->cur.state = WECHAT_MUSIC_PLAYING;
            __this->cur.album_id = json_object_get_int(album_id);
        } else if (__this->cur.state == WECHAT_MUSIC_PAUSE) {
            __this->cur.state = WECHAT_MUSIC_PLAYING;
            turing_wechat_media_audio_continue(__this->cur.fileurl);
        }
    } else {
        if (__this->cur.state == WECHAT_MUSIC_PLAYING) {
            turing_wechat_media_audio_pause(NULL);
            __this->cur.state = WECHAT_MUSIC_PAUSE;
        }
    }

    return 0;
}

static int mqtt_message_control(json_object *message)
{
    json_object *operate = NULL;
    json_object *arg = NULL;
    struct sys_event evt;

    if (!json_object_object_get_ex(message, "operate", &operate)) {
        return -1;
    }

    int state = json_object_get_int(operate);

    switch (state) {
    case CONCROL_VOL_DOWN:
    case CONCROL_VOL_UP:
        printf("CONCROL_SET_VOL\n");
        if (!json_object_object_get_ex(message, "arg", &arg)) {
            return -1;
        }
        turing_volume_change_notify(json_object_get_int(arg));
        break;
    case CONCROL_LED_CLOSE:
        printf("CONCROL_LED_CLOSE\n");
#if 0
        if (get_current_app() && !strcmp(get_current_app()->name, "app_music")) {
            evt.type = SYS_KEY_EVENT;
            evt.u.key.event = KEY_EVENT_CLICK;
            evt.u.key.value = KEY_RIGHT;
            sys_event_notify(&evt);
        }
#endif
        break;
    case CONCROL_LED_OPEN:
        printf("CONCROL_LED_OPEN\n");
#if 0
        if (get_current_app() && !strcmp(get_current_app()->name, "app_music")) {
            evt.type = SYS_KEY_EVENT;
            evt.u.key.event = KEY_EVENT_CLICK;
            evt.u.key.value = KEY_RIGHT;
            sys_event_notify(&evt);
        }
#endif
        break;
    case CONCROL_LOW_POWER_CLOSE:
        printf("CONCROL_LOW_POWER_CLOSE\n");
        break;
    case CONCROL_LOW_POWER_OPEN:
        printf("CONCROL_LOW_POWER_OPEN\n");
        break;
    case CONCROL_STROAGE_FORMAT:
        printf("CONCROL_STROAGE_FORMAT\n");
        /* storage_device_format(); */
        break;
    case CONCROL_DEFAULT:
        printf("CONCROL_DEFAULT\n");
        /* db_reset(); */
        break;
    default:
        printf("no support\n");
        break;
    }

    return 0;
}

static int mqtt_message_noitfy(json_object *message)
{
    json_object *operate = NULL;

    if (!json_object_object_get_ex(message, "operate", &operate)) {
        return -1;
    }

    int value = json_object_get_int(operate);

    if (value == 1) {
        turing_wechat_play_promt("045.mp3");
    } else if (value == 2) {
        turing_wechat_play_promt("046.mp3");
    } else if (value == 0) {
        mqtt_init_state_noitfy(NULL);
    }

    return 0;
}

void dev_mqtt_cb_user_msg(const char *data, u32 len)
{
    json_object *new_obj = NULL;
    json_object *type = NULL;
    json_object *message = NULL;

    new_obj = json_tokener_parse(data);
    if (!new_obj) {
        return;
    }
    if (!json_object_object_get_ex(new_obj, "type", &type)) {
        goto __result_exit;
    }
    __this->cur.message_type = json_object_get_int(type);
    if (!json_object_object_get_ex(new_obj, "message", &message)) {
        goto __result_exit;
    }

    switch (__this->cur.message_type) {

    case MQTT_MESSAGE_CHAT:
        mqtt_message_chat(message);
        break;
    case MQTT_MESSAGE_AUDIO:
        mqtt_message_audio(message);
        break;
    case MQTT_MESSAGE_CONTROL:
        mqtt_message_control(message);
        break;
    case MQTT_MESSAGE_NOTIFY:
        mqtt_message_noitfy(message);
        break;
    default:
        break;
    }

__result_exit:
    json_object_put(new_obj);
}

void wechat_api_task(void *arg)
{
    int msg[32];
    int err;

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        switch (msg[1]) {
        case WECHAT_MEDIA_END:
        case WECHAT_NEXT_SONG:
            puts("===========WECHAT_PLAY_NEXT\n");
            if (turing_wechat_next_song(__this->cur.filename, &__this->cur.album_id)) {
                __this->cur.state = WECHAT_MUSIC_STOP;
            } else {
                __this->cur.state = WECHAT_MUSIC_PLAYING;
            }
            break;
        case WECHAT_PRE_SONG:
            puts("===========WECHAT_PLAY_PRE\n");
            if (turing_wechat_pre_song(__this->cur.filename, &__this->cur.album_id)) {
                __this->cur.state = WECHAT_MUSIC_STOP;
            } else {
                __this->cur.state = WECHAT_MUSIC_PLAYING;
            }
            break;
        case WECHAT_PAUSE_SONG:
            puts("===========WECHAT_PAUSE\n");
            if (__this->cur.state == WECHAT_MUSIC_PAUSE) {
                turing_wechat_pause_song(__this->cur.filename, __this->cur.album_id, 1);
                __this->cur.state = WECHAT_MUSIC_PLAYING;
            } else if (__this->cur.state == WECHAT_MUSIC_PLAYING) {
                turing_wechat_pause_song(__this->cur.filename, __this->cur.album_id, 2);
                __this->cur.state = WECHAT_MUSIC_PAUSE;
            }
            break;
        case WECHAT_VOLUME_CHANGE:
            puts("===========WECHAT_VOLUME_CHANGE\n");
            char buf[32];
            sprintf(buf, "\"vol\":%d", msg[2]);
            turing_wechat_state_noitfy(DEVICE_STATE, buf);
            break;
        case WECHAT_MEDIA_STOP:
            puts("===========WECHAT_MEDIA_STOP\n");
            __this->cur.state = WECHAT_MUSIC_STOP;
            break;
        case WECHAT_SEND_INIT_STATE:
            puts("===========WECHAT_SEND_INIT_STATE\n");
            mqtt_init_state_noitfy(NULL);
            break;
        case WECHAT_COLLECT_RESOURCE:
            puts("===========WECHAT_COLLECT_RESOURCE\n");
            if (msg[3]) {
                turing_wechat_collect_resource(__this->cur.filename, &msg[3], msg[2]);
            } else {
                turing_wechat_collect_resource(__this->cur.filename, &__this->cur.album_id, msg[2]);
            }
            break;
        case WECHAT_KILL_SELF_TASK:
            puts("\nwechat task kill\n");
            return;
        default:
            break;
        }
    }
}
