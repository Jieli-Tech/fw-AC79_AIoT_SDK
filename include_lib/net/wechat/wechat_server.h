#ifndef WECHAT_SERVER_H
#define WECHAT_SERVER_H

#include "typedef.h"
#include "os/os_api.h"


#define WECHAT_REQ                 0x1

enum {
    WECHAT_NEXT_SONG = 0,
    WECHAT_PRE_SONG,
    WECHAT_PAUSE_SONG,
    WECHAT_CONTINUE_SONG,
    WECHAT_VOLUME_CHANGE,
    WECHAT_MEDIA_END,
    WECHAT_PROGRESS_INFO,
    WECHAT_MEDIA_STOP,
    WECHAT_SEND_INIT_STATE,
    WECHAT_KILL_SELF_TASK = 0xfe,
};

enum {
    WECHAT_SERVER_SPEECH_URL_EVENT = 0,
    WECHAT_SERVER_AMR_ERR_EVENT,
    WECHAT_SERVER_AMR_END_EVENT,
};


enum {
    WECHAT_STATE_OPEN = 0x0,
    WECHAT_STATE_START,
    WECHAT_STATE_ENC_AMR_START,
    WECHAT_STATE_ENC_AMR_STOP,
    WECHAT_STATE_SEND,
    WECHAT_STATE_PAUSE,
    WECHAT_STATE_STOP,
    WECHAT_STATE_CLOSE,
};

struct  wechat_req {
    char *filename;
    char *username;
    char *password;
    char *dev_mac;
    int err;
    OS_SEM sem;
    u8 cmd;
};

#endif
