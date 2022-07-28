#include "server/ai_server.h"
#include "turing.h"
#include "server/server_core.h"
#include "os/os_api.h"
#include "turing_iot.h"
#include "turing_mqtt.h"
#include "generic/version.h"


#ifdef TURING_WECHAT_SDK_VERSION
MODULE_VERSION_EXPORT(turing_wechat_sdk, TURING_WECHAT_SDK_VERSION);
#endif


const struct ai_sdk_api wechat_sdk_api;

static void *iot_hdl = NULL;

static const int events[][2] = {
    { AI_EVENT_NEXT_SONG, WECHAT_NEXT_SONG     },
    { AI_EVENT_MEDIA_END, WECHAT_MEDIA_END     },
    { AI_EVENT_PLAY_PAUSE,  WECHAT_PAUSE_SONG   },
    { AI_EVENT_PREVIOUS_SONG, WECHAT_PRE_SONG },
    { AI_EVENT_VOLUME_CHANGE, WECHAT_VOLUME_CHANGE },
    { AI_EVENT_MEDIA_STOP,  WECHAT_MEDIA_STOP },
    { AI_EVENT_COLLECT_RES,  WECHAT_COLLECT_RESOURCE },
};

static int event2wechat(int event)
{
    for (int i = 0; i < ARRAY_SIZE(events); i++) {
        if (events[i][0] == event) {
            return events[i][1];
        }
    }

    return -1;
}

static int wechat_sdk_do_event(int event, int arg)
{
    int err;
    int argc = 3;
    int argv[4];
    int wechat_event;

    wechat_event = event2wechat(event);
    if (wechat_event == -1) {
        return 0;
    }

    printf("wechat_event: %x\n", wechat_event);

    argv[0] = wechat_event;
    argv[1] = arg;
    argv[2] = 0;

    do {
        err = os_taskq_post_type("wechat_api_task", Q_USER, argc, argv);
        if (err != OS_Q_FULL) {
            break;
        }
        os_time_dly(10);
    } while (1);

    return err == OS_NO_ERR ?  0 : -1;
}

void turing_wechat_speak_play(const char *url)
{
    char *save_url = NULL;

    if (url && strlen(url) > 0) {
        save_url = (char *)malloc(strlen(url) + 1);
        if (!save_url) {
            return;
        }
        strcpy(save_url, url);
        if (0 != ai_server_event_notify(&wechat_sdk_api, save_url, AI_SERVER_EVENT_RECV_CHAT)) {
            free(save_url);
        }
    }
}

void turing_wechat_play_promt(const char *fname)
{
    ai_server_event_notify(&wechat_sdk_api, (void *)fname, AI_SERVER_EVENT_PLAY_BEEP);
}

void turing_wechat_media_audio_play(const char *url)
{
    ai_server_event_url(&wechat_sdk_api, url, AI_SERVER_EVENT_URL);
}

void turing_wechat_media_audio_continue(const char *url)
{
    ai_server_event_url(&wechat_sdk_api, url, AI_SERVER_EVENT_CONTINUE);
}

void turing_wechat_media_audio_pause(const char *url)
{
    ai_server_event_url(&wechat_sdk_api, url, AI_SERVER_EVENT_PAUSE);
}

void turing_upgrade_notify(int event, void *arg)
{
    ai_server_event_notify(&wechat_sdk_api, arg, event);
}

void turing_volume_change_notify(int volume)
{
    ai_server_event_notify(&wechat_sdk_api, (void *)volume, AI_SERVER_EVENT_VOLUME_CHANGE);
}

int turing_wechat_state_noitfy(u8 type, const char *status_buffer)
{
    return turing_iot_notify_status(iot_hdl, type, status_buffer);
}

int turing_wechat_next_song(char *title, int *id)
{
    if (turing_iot_get_music_res(iot_hdl, 0)) {
        return -1;
    }
    turing_iot_get_song_title(iot_hdl, title, id);
    return ai_server_event_url(&wechat_sdk_api, turing_iot_get_url(iot_hdl), AI_SERVER_EVENT_URL);
}

int turing_wechat_pre_song(char *title, int *id)
{
    if (turing_iot_get_music_res(iot_hdl, 1)) {
        return -1;
    }
    turing_iot_get_song_title(iot_hdl, title, id);
    return ai_server_event_url(&wechat_sdk_api, turing_iot_get_url(iot_hdl), AI_SERVER_EVENT_URL);
}

int turing_wechat_pause_song(char *title, int id, u8 status)
{
    char buf[128];
    snprintf(buf, sizeof(buf), "\"title\":\"%s\",\"mediaId\":%d,\"play\":%d", title, id, status);
    return turing_iot_notify_status(iot_hdl, MUSIC_STATE, buf);
}

int turing_wechat_collect_resource(char *title, int *id, u8 is_play)
{
    int ret = turing_iot_collect_resource(iot_hdl, *id, is_play);
    if (!ret && is_play) {
        turing_iot_get_song_title(iot_hdl, title, id);
        return ai_server_event_url(&wechat_sdk_api, turing_iot_get_url(iot_hdl), AI_SERVER_EVENT_URL);
    }

    return ret;
}

static int wechat_sdk_connect()
{
    iot_hdl = turing_mqtt_init();
    return 0;
}

static int wechat_sdk_disconnect()
{
    turing_mqtt_uninit();
    return  0;
}

static int wechat_sdk_check()
{
    if (turing_net_ready()) {
        return AI_STAT_CONNECTED;
    }

    return AI_STAT_DISCONNECTED;
}

REGISTER_AI_SDK(wechat_sdk_api) = {
    .name           = "wechat",
    .connect        = wechat_sdk_connect,
    .disconnect     = wechat_sdk_disconnect,
    .state_check    = wechat_sdk_check,
    .do_event       = wechat_sdk_do_event,
};
