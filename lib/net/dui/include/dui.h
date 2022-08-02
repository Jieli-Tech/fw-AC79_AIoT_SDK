#ifndef _DUI_H_
#define _DUI_H_

#include "web_socket/websocket_api.h"
#include "dui_net.h"
#include "dui_api.h"
#include "dui_alarm.h"

#define CONFIG_CLOUD_VAD_ENABLE       1

enum DUI_SDK_EVENT {
    DUI_SPEAK_END     = 0x01,
    DUI_MEDIA_END     = 0x02,
    DUI_PLAY_PAUSE    = 0x03,
    DUI_PREVIOUS_SONG = 0x04,
    DUI_NEXT_SONG     = 0x05,
    DUI_VOLUME_CHANGE = 0X06,
    DUI_VOLUME_INCR   = 0x07,
    DUI_VOLUME_DECR   = 0x08,
    DUI_VOLUME_MUTE   = 0x09,
    DUI_RECORD_START  = 0x0a,
    DUI_RECORD_SEND   = 0x0b,
    DUI_RECORD_STOP   = 0x0c,
    DUI_VOICE_MODE    = 0x0d,
    DUI_MEDIA_STOP    = 0x0e,
    DUI_BIND_DEVICE   = 0x0f,
    DUI_RECORD_ERR    = 0x10,
    DUI_PICTURE_RECOG = 0x11,
    DUI_COLLECT_RESOURCE = 0x12,
    DUI_PLAY_CONTIUE  = 0x13,
    DUI_SET_VOLUME    = 0x14,
    DUI_SPEAK_START   = 0x15,
    DUI_MEDIA_START   = 0x16,
    DUI_RECORD_BREAK  = 0x17,

    DUI_QUIT    	     = 0xff,
};

enum DUI_NET_MSG {
    DUI_RECORD_START_MSG = 0x01,
    DUI_RECORD_SEND_MSG  = 0x02,
    DUI_RECORD_STOP_MSG  = 0x03,
    DUI_PREVIOUS_SONG_MSG = 0x04,
    DUI_NEXT_SONG_MSG = 0x05,
    DUI_ALARM_SYNC = 0x06,
    DUI_ALARM_RING = 0x07,
    DUI_ALARM_OPERATE = 0x08,
    DUI_INFO_SEARCH = 0X09,

    DUI_QUIT_MSG  = 0xFF,
};

typedef enum {
    TEXT_REQUEST,                     //文本请求
    AUDIO_REQUEST,                    //语音请求
    SPEECH_RECOGNITION,               //语音识别
    INTENT_REQUEST,                   //意图请求
    SKILL_SETTING,                    //技能配置
    SYSTEM_SETTING,                   //系统配置
    CUSTOM_REQUEST,                   //自定义信息查询
} MUTUAL_TYPE;                        //交互类型


typedef enum {
    ORDERPLAY_MODE,//orderPlay
    LOOPPLAY_MODE,//loopPlay
    SINGLELOOP_MODE,//singleLoop
    RANDOMPLAY_MODE,//randomPlay
} PLAY_MODE;

typedef enum {
    STOP,
    PAUSE,
    PLAY
} PLAY_STATUS;

typedef enum {
    ALARM_SYNC,//同步提醒
    ALARM_RING,//播放响铃
    ALARM_OPERATE,//闹钟操作
} MUTUAL_INTENT;

typedef struct {
    char productKey[40];//当前32位的key
    char ProductSecret[40];//当前32位的key
    char deviceID[40];//没有限制,默认32位
    char productId[16];//当前是9位
    //文本请求
    char *text;
    //意图请求
    char *intent;
    char *slots;
    char *extra;
} MUTUAL_PARAM;

typedef struct {
    struct websocket_struct websockets_info;
    u8 connect_status;
} MUTUAL_HDL;

typedef struct {
    char task[20];
    char sessionId[40]; //会话ID
    char deviceSecret[40];
    char url[1460];
} MUTUAL_REPLY;

struct dui_para {
    MUTUAL_TYPE type;
    MUTUAL_INTENT intent;
    MUTUAL_PARAM param;
    MUTUAL_REPLY reply;
    MUTUAL_HDL hdl;
};

struct dui_var {
    struct dui_para para;
    u8 voice_mode;
    u8 exit_flag;
    u8 use_vad;
};


#define DUI_OS_TASKQ_POST(name,argc, ...)                       \
    do{                                                         \
        int err =  os_taskq_post(name,argc, __VA_ARGS__);       \
        if(err){                                                \
            log_e("\n %s %d err = %d\n",__func__,__LINE__,err); \
        }                                                       \
    }while(0)

extern int dui_app_init(void);
extern void dui_app_uninit(void);
extern u8 dui_app_get_connect_status(void);
extern u8 get_dui_msg_notify(void);
extern u32 get_record_sessionid(void);
extern struct dui_var *get_dui_hdl(void);
extern const char *dui_get_net_cfg_uid(void);
extern const char *wifi_module_get_sta_ssid(void);
extern int get_update_data(const char *url);
extern int get_app_music_volume(void);
extern int get_app_music_total_time(void);
extern int get_dui_token(struct dui_para *para);
extern const char *dui_get_userid(void);
extern const char *dui_get_device_id(void);
extern const char *dui_get_product_code(void);
extern const char *dui_get_product_id(void);
extern const char *dui_get_version(void);
extern void dui_media_music_mode_set(int mode);
extern int dui_tone_tts_play(int tts_type);
extern const int dui_media_get_playing_status(void);
extern void dui_iot_media_audio_play(const char *url);
extern void dui_ai_media_audio_play(const char *url);
extern int dui_net_music_play_prev(void);
extern int dui_net_music_play_next(void);
extern u8 get_record_break_flag(void);
extern void set_record_break_flag(u8 value);
extern void dui_media_set_playing_status(int value);

#endif
