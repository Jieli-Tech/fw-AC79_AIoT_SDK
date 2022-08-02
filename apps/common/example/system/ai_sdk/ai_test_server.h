#ifndef AI_TEST_SERVER_H
#define AI_TEST_SERVER_H

#include "generic/typedef.h"

enum {
    AI_SERVER_EVENT_URL,
    AI_SERVER_EVENT_URL_TTS,
    AI_SERVER_EVENT_URL_MEDIA,
    AI_SERVER_EVENT_CONNECTED,
    AI_SERVER_EVENT_DISCONNECTED,
    AI_SERVER_EVENT_CONTINUE,
    AI_SERVER_EVENT_PAUSE,
    AI_SERVER_EVENT_STOP,
    AI_SERVER_EVENT_UPGRADE,
    AI_SERVER_EVENT_UPGRADE_SUCC,
    AI_SERVER_EVENT_UPGRADE_FAIL,
    AI_SERVER_EVENT_MIC_OPEN,
    AI_SERVER_EVENT_MIC_CLOSE,
    AI_SERVER_EVENT_REC_TOO_SHORT,
    AI_SERVER_EVENT_RECV_CHAT,
    AI_SERVER_EVENT_VOLUME_CHANGE,
    AI_SERVER_EVENT_SET_PLAY_TIME,
    AI_SERVER_EVENT_SEEK,
    AI_SERVER_EVENT_PLAY_BEEP,
    AI_SERVER_EVENT_CHILD_LOCK_CHANGE,
    AI_SERVER_EVENT_LIGHT_CHANGE,
    AI_SERVER_EVENT_REC_ERR,
    AI_SERVER_EVENT_BT_CLOSE,
    AI_SERVER_EVENT_BT_OPEN,
    AI_SERVER_EVENT_RESUME_PLAY,
    AI_SERVER_EVENT_PREV_PLAY,
    AI_SERVER_EVENT_NEXT_PLAY,
    AI_SERVER_EVENT_SHUTDOWN,
};


enum {
    AI_STAT_CONNECTED,
    AI_STAT_DISCONNECTED,
};

enum {
    AI_MODE = 0,
    TRANSLATE_MODE,
    WECHAT_MODE,
    ORAL_MODE,
    VAD_ENABLE = 0x80,
};

enum ai_test_server_event {
    AI_EVENT_SPEAK_END     = 0x01,
    AI_EVENT_MEDIA_END     = 0x02,
    AI_EVENT_PLAY_PAUSE    = 0x03,
    AI_EVENT_PREVIOUS_SONG = 0x04,
    AI_EVENT_NEXT_SONG     = 0x05,
    AI_EVENT_VOLUME_CHANGE = 0X06,
    AI_EVENT_VOLUME_INCR   = 0x07,
    AI_EVENT_VOLUME_DECR   = 0x08,
    AI_EVENT_VOLUME_MUTE   = 0x09,
    AI_EVENT_RECORD_START  = 0x0a,
    AI_EVENT_RECORD_BREAK  = 0x0b,
    AI_EVENT_RECORD_STOP   = 0x0c,
    AI_EVENT_VOICE_MODE    = 0x0d,
    AI_EVENT_PLAY_TIME     = 0x0e,
    AI_EVENT_MEDIA_STOP    = 0x0f,
    AI_EVENT_COLLECT_RES   = 0x10,
    AI_EVENT_CHILD_LOCK    = 0x11,
    AI_EVENT_CUSTOM_FUN    = 0x30,
    AI_EVENT_RUN_START     = 0x80,
    AI_EVENT_RUN_STOP      = 0x81,
    AI_EVENT_SPEAK_START   = 0x82,
    AI_EVENT_MEDIA_START   = 0x83,
    AI_EVENT_MEDIA_PLAY_TIME   = 0x84,

    AI_EVENT_QUIT    	   = 0xff,
};

enum {
    AI_REQ_CONNECT,
    AI_REQ_DISCONNECT,
    AI_REQ_LISTEN,
    AI_REQ_EVENT,
    AI_LISTEN_START,
    AI_LISTEN_STOP,
};

struct ai_test_sdk_api {
    const char *name;
    int (*connect)(void);
    int (*state_check)(void);
    int (*do_event)(int event, int arg);
    int (*disconnect)(void);
};

struct ai_test_listen {
    int arg;
    int cmd;
    const char *ai_name;
};

struct ai_test_event {
    int arg;
    int event;
    const char *ai_name;
};

union ai_test_req {
    struct ai_test_listen lis;
    struct ai_test_event evt;
};


int ai_test_server_event_url(const struct ai_test_sdk_api *, const char *url, int event);

int ai_test_server_event_notify(const struct ai_test_sdk_api *, void *arg, int event);

int ai_server_request(void *server, int req_type, union ai_test_req *req);

extern const struct ai_test_sdk_api ai_sdk_api_begin[];
extern const struct ai_test_sdk_api ai_sdk_api_end[];


#define REGISTER_AI_SDK(name) \
    const struct ai_test_sdk_api name SEC_USED(.ai_sdk)


#define list_for_each_ai_sdk(p) \
    for (index = 0, p = ai_sdk_api_begin; p < ai_sdk_api_end; p++,index++)


#endif
