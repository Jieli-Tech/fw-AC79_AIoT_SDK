#include "generic/version.h"
#include "server/server_core.h"
#include "server/ai_server.h"
#include "version.h"
#include "aligenie_sdk.h"
#include "auth.h"
#include "os/os_api.h"
#include "sdk.h"

//通过调用版本检测函数使的模块的代码能够被链接
#ifdef ALI_SDK_VERSION
MODULE_VERSION_EXPORT(ali_sdk, ALI_SDK_VERSION);
#endif

static int ali_app_task_pid = 0;


const struct ai_sdk_api ag_sdk_api;

static const int events[][2] = {
    { AI_EVENT_SPEAK_END, ALI_SPEAK_END },
    { AI_EVENT_MEDIA_END, ALI_MEDIA_END },
    { AI_EVENT_MEDIA_STOP, ALI_MEDIA_STOP},
    { AI_EVENT_PREVIOUS_SONG, ALI_PREVIOUS_SONG },
    { AI_EVENT_NEXT_SONG, ALI_NEXT_SONG },
    { AI_EVENT_RECORD_START, ALI_RECORD_START },
    { AI_EVENT_RECORD_BREAK, ALI_RECORD_BREAK },
    { AI_EVENT_RECORD_STOP, ALI_RECORD_STOP },
    { AI_EVENT_COLLECT_RES, ALI_COLLECT_RESOURCE },
    { AI_EVENT_PLAY_PAUSE, ALI_PLAY_PAUSE},
    { AI_EVENT_VOLUME_CHANGE, ALI_VOLUME_CHANGE},
    { AI_EVENT_VOLUME_INCR, ALI_VOLUME_INCR},
    { AI_EVENT_VOLUME_DECR, ALI_VOLUME_DECR},
    { AI_EVENT_CUSTOM_FUN, ALI_RECV_CHAT},
    { AI_EVENT_QUIT, ALI_QUIT },
};

static int event2ali(int event)
{
    for (int i = 0; i < ARRAY_SIZE(events); i++) {
        if (events[i][0] == event) {
            return events[i][1];
        }
    }

    return -1;
}

static int ali_sdk_do_event(int event, int arg)
{
    int err;
    int argc = 2;
    int argv[2];
    int ali_event;

    printf("ali_sdk_do_event %d\n", event);

    ali_event = event2ali(event);

    if (ali_event == -1) {
        return 0;
    }

    argv[0] = ali_event;
    argv[1] = arg;

    err = os_taskq_post_type("ali_app_task", Q_USER, argc, argv);
    if (err != OS_NO_ERR) {
        printf("send msg to ali_app_task fail, event : %d\n", ali_event);
    }

    return 0;
}

static int ali_early_init(void)
{
#define USER_ID  "DARSTPK9X8ID"
//    int aligenie_set_app_key(const char * app_key);
//    int aligenie_set_ext(const char * ext);
//    int aligenie_set_schema(const char * schema);
//    int aligenie_set_user_id(const char * user_id);
//    int aligenie_set_utd_id(const char * utd_id);

    int ret = 0;
    static int inited = 0;
    char authcode[10] = {0};

    printf("ali_early_init\n");

    if (!inited) {
        aligenie_set_user_id(USER_ID);
        aligenie_get_authcode(authcode);
        ret = ag_sdk_set_register_info("4c:11:AE:64:73:FD", USER_ID, authcode, 0);
        ag_sdk_init();
        printf("ag_sdk_set_register_info  status :%d\n", ret);
        inited = 1;
    }

    return 0;
}

static void ali_app_task(void *priv)
{
    int msg[32];
    int err;
    u8 exit = 0;
    u8 voice_mode = 0;

    ali_early_init();

    ag_sdk_notify_network_status_change(AG_NETWORK_CONNECTED);

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        switch (msg[1]) {
        case ALI_SPEAK_END:
            puts("===========ALI_SPEAK_END\n");
            void ag_message_sem_post(void);
            ag_message_sem_post();
            break;
        case ALI_MEDIA_END:
            puts("===========ALI_MEDIA_END\n");
            /* ag_sdk_notify_keyevent(AG_KEYEVENT_NEXT); */
            void clear_ag_message_flag(void);
            clear_ag_message_flag();
            extern AG_AUDIO_INFO_T ag_audio_info;
            ag_audio_info.status = AG_PLAYER_URL_FINISHED;
            ag_sdk_notify_player_status_change(&ag_audio_info);
            break;
        case ALI_MEDIA_STOP:
            puts("===========ALI_MEDIA_STOP\n");
            ag_sdk_notify_keyevent(AG_KEYEVENT_PLAYPAUSE);
            ag_sdk_take_player_control();
            break;
        case ALI_PLAY_PAUSE:
            puts("===========ALI_PLAY_PAUSE\n");
            ag_sdk_notify_keyevent(AG_KEYEVENT_PLAYPAUSE);
            break;
        case ALI_RECORD_START:
            voice_mode = msg[2] & 0x3;
            printf("ALI_RECORD_START\n");
            if (voice_mode == AI_MODE) {
                printf("ALI_RECORD_START IN AI_MODE\n");
                ag_sdk_notify_keyevent(AG_KEYEVENT_AI_START);
            } else if (voice_mode == TRANSLATE_MODE) {
                ag_sdk_notify_keyevent(AG_KEYEVENT_TRANSLATE_START);
            } else if (voice_mode == WECHAT_MODE) {
                ag_sdk_notify_keyevent(AG_KEYEVENT_INTERCOM_START);
            }
            break;
        case ALI_RECORD_STOP:
            printf("ALI_RECORD_STOP\n");
            if (voice_mode == AI_MODE) {
                printf("ALI_RECORD_STOP IN AI_MODE\n");
                ag_sdk_notify_keyevent(AG_KEYEVENT_AI_STOP);
            } else if (voice_mode == TRANSLATE_MODE) {
                ag_sdk_notify_keyevent(AG_KEYEVENT_TRANSLATE_STOP);
            } else if (voice_mode == WECHAT_MODE) {
                ag_sdk_notify_keyevent(AG_KEYEVENT_INTERCOM_STOP);
            }
            break;
        case ALI_PREVIOUS_SONG:
            puts("===========ALI_PREVIOUS_SONG\n");
            ag_sdk_notify_keyevent(AG_KEYEVENT_PREVIOUS);
            break;
        case ALI_NEXT_SONG:
            puts("===========ALI_NEXT_SONG\n");
            ag_sdk_notify_keyevent(AG_KEYEVENT_NEXT);
            break;
        case ALI_VOLUME_INCR:
            ag_sdk_notify_keyevent(AG_KEYEVENT_VOLUME_UP);
            break;
        case ALI_VOLUME_DECR:
            ag_sdk_notify_keyevent(AG_KEYEVENT_VOLUME_DOWN);
            break;
        case ALI_VOLUME_CHANGE:
            break;
        case ALI_COLLECT_RESOURCE:
            if (msg[2]) {
                ag_sdk_notify_keyevent(AG_KEYEVENT_PLAY_FAVORITE);
            } else {
                ag_sdk_notify_keyevent(AG_KEYEVENT_ADD_REMOVE_FAVORITE);
            }
            break;
        case ALI_WS_CONNECT_MSG:
            ag_ws_on_connect();
            break;
        case ALI_WS_DISCONNECT_MSG:
            ag_ws_on_disconnect();
            if (exit) {
                return;
            }
            break;
        case ALI_RECV_CHAT:
            ag_sdk_notify_keyevent(AG_KEYEVENT_INTERCOM_PLAY);
            break;
        case ALI_QUIT:
            ag_sdk_notify_network_status_change(AG_NETWORK_DISCONNECT);
            exit = 1;
            break;
        default:
            break;
        }
    }
}

static int ali_sdk_open(void)
{
    return thread_fork("ali_app_task", 10, 2048, 64, &ali_app_task_pid, ali_app_task, NULL);
}

static int ali_sdk_check(void)
{
    return AI_STAT_CONNECTED;
}

static int ali_sdk_disconnect(void)
{
    do {
        if (OS_Q_FULL != os_taskq_post("ali_app_task", 1, ALI_QUIT)) {
            break;
        }
        log_e("ali_app_task send msg QUIT timeout \n");
        os_time_dly(5);
    } while (1);

    thread_kill(&ali_app_task_pid, KILL_WAIT);

    return 0;
}

REGISTER_AI_SDK(ag_sdk_api) = {
    .name           = "ali",
    .connect        = ali_sdk_open,
    .state_check    = ali_sdk_check,
    .do_event       = ali_sdk_do_event,
    .disconnect     = ali_sdk_disconnect,
};
