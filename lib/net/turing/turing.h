#ifndef _TURING_H_
#define _TURING_H_


#include "sock_api/sock_api.h"
#include "json_c/json_tokener.h"

// #define TURING_MEMORY_DEBUG

#define TURING_DEBUG
#ifdef  TURING_DEBUG

#define TR_DEBUG_E(...) printf("[ERROR] in func:%s line:%d ", __func__, __LINE__), \
    printf(__VA_ARGS__), \
	printf("\r\n");

#define TR_DEBUG_I(...) printf(__VA_ARGS__)

#else

#define TR_DEBUG_E(...)
#define TR_DEBUG_I(...)

#endif

#define TURING_ENC_USE_OPUS	1

typedef enum {
    ASR_PCM_16K_16BIT,
    ASR_PCM_8K_16BIT,
    ASR_AMR_8K_16BIT,
    ASR_AMR_16K_16BIT,
    ASR_OPUS,
    ASR_SPEEX,
} ASR_TYPE;

typedef enum {
    TTS_PCM_8K_16BIT,
    TTS_MP3_64,
    TTS_MP3_24,
    TTS_MP3_16,
    TTS_AMR_NB,
} TTS_TYPE;

typedef enum {
    NOT_OUTPUT_TXT,                 //不输出文本（默认）
    OUTPUT_ASR_TXT,                 //输出asr文本信息
    OUTPUT_TTS_TXT,                 //输出tts文本信息
    OUTPUT_ARS_TTS_TXT,             //输出asr&tts文本信息
} OUT_TXT;

typedef enum {
    NOT_STREAM_IDENTIFY,            //非流式识别（默认）
    STREAM_IDENTIFY,                //流式识别
} STREAM_CTL;

typedef enum {
    NORMAL_ENCODE,                  //通用编码
    CUSTORM_ENCODE,                 //自定义编码
} ENCODE_MODE;

typedef enum {
    SMARTCHAT,                      //智能聊天
    ACTIVE_INTERACTION,             //主动交互
    MARKED_WORDS,					//提示语
    ORAL_EVALUATION,				//口语评测
    PICTURE_RECOG,					//绘本识别
    INPUT_TEXT,						//文本输入
} MUTUAL_TYPE;

typedef enum {
    PLAY_NEXT,
    PLAY_PREV,
    CONNECT_SUCC,
} CMD_DODE;

enum TURING_SDK_EVENT {
    TURING_SPEAK_END     = 0x01,
    TURING_MEDIA_END     = 0x02,
    TURING_PLAY_PAUSE    = 0x03,
    TURING_PREVIOUS_SONG = 0x04,
    TURING_NEXT_SONG     = 0x05,
    TURING_VOLUME_CHANGE = 0X06,
    TURING_VOLUME_INCR   = 0x07,
    TURING_VOLUME_DECR   = 0x08,
    TURING_VOLUME_MUTE   = 0x09,
    TURING_RECORD_START  = 0x0a,
    TURING_RECORD_SEND   = 0x0b,
    TURING_RECORD_STOP   = 0x0c,
    TURING_VOICE_MODE    = 0x0d,
    TURING_MEDIA_STOP    = 0x0e,
    TURING_BIND_DEVICE   = 0x0f,
    TURING_RECORD_ERR    = 0x10,
    TURING_PICTURE_RECOG = 0x11,
    TURING_COLLECT_RESOURCE = 0x12,
    TURING_PICTURE_PLAY_END = 0x13,
    TURING_QUIT    	     = 0xff,
};

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
    WECHAT_COLLECT_RESOURCE,
    WECHAT_KILL_SELF_TASK = 0xfe,
};

enum TURING_EVENT_FUN_CODE {
    TURING_EVENT_NOT_FUNC,
    /*功能事件返回*/
    TURING_EVENT_FUN_CHAT = 20000,              	 //聊天                      普通聊天模式
    TURING_EVENT_FUN_SLEEP_CTL = 20001,              //休眠控制                  口令控制设备休眠，口令：再见、拜拜等
    TURING_EVENT_FUN_VOL_CTL = 20002,                //音量控制                  控制当前音量大小，具体可调幅度根据设备不存在差异
    TURING_EVENT_FUN_WEATHER_INQUIRE = 20003,        //天气查询                  支持全国660个主要城市和地区的天气查询
    TURING_EVENT_FUN_TIME_INQUIRE = 20005,           //日期时间查询              查询目标地区当前日期时间
    TURING_EVENT_FUN_CALCULATOR = 20006,             //计算器                    支持1000以内的四则混合运算
    TURING_EVENT_FUN_PLAY_MUSIC = 20007,             //播音乐                    支持服务器本地歌曲库随机播放与点播歌曲
    TURING_EVENT_FUN_PLAY_STORY = 20008,             //讲故事                    支持工程师爸爸随机播放与点播故事
    TURING_EVENT_FUN_POETRY = 20009,                 //古诗词                    支持5000余首古诗朗诵
    TURING_EVENT_FUN_ANIMAL_SOUND = 20011,           //动物叫声                  支持38种常见物叫声
    TURING_EVENT_FUN_KNOWLEDGE = 20012,            	 //百科知识                  支持107万个百科知识
    TURING_EVENT_FUN_PHONE_CALL = 20013,             //打电话                    支持打电话功能
    TURING_EVENT_FUN_GUESS_SOUND = 20014,            //猜叫声                    猜动物叫声游戏
    TURING_EVENT_FUN_TRANSLATION = 20015,            //中英互译                  中文翻译英文，英文翻译中文
    TURING_EVENT_FUN_DANCE = 20016,                  //跳舞                      命令机器人跳舞
    TURING_EVENT_FUN_ENGLISHSPEAK = 20018,           //英文对话                  返回的为英文
    TURING_EVENT_FUN_INSTRUMENT = 20019,             //乐器声音                  播放乐器的声音
    TURING_EVENT_FUN_NATURE_SOUND = 20020,           //大自然的声音              播放打雷等大自然的声音
    TURING_EVENT_FUN_SCREEN_LIGHT = 20021,           //屏幕亮度                  调整屏幕亮度
    TURING_EVENT_FUN_BATTERY_CHECK = 20022,          //电量查询                  暂不支持
    TURING_EVENT_FUN_SPORT_CTL = 20023,              //运动控制                  控制机器人运动
    TURING_EVENT_FUN_PHOTO_CTL = 20024,              //拍照                      发出拍照指令
    TURING_EVENT_FUN_ALARM_CTL = 20025,              //闹钟                      设置闹钟或提醒
    TURING_EVENT_FUN_OPEN_APP = 20026,               //打开应用                  打开应用，需要上传应用列表
    TURING_EVENT_FUN_KNOWLEDGE_REPOSITORY = 20027,   //知识库                    智能FAQ，需设置知识库
    TURING_EVENT_FUN_PICTURE_RECOGNITION = 20039,    //绘本识别                  绘本识别返回结果
    TURING_EVENT_FUN_ACTIVE_INTERACTION = 29998,     //主动交互                  机器人长时间无交互由机器人主动交互
    TURING_EVENT_FUN_PROMPT_TONE = 29999,            //提示语                    开机等场景下的提示语
};

enum {
    TURING_FUN_PLAY_OPR_NOR   = 1000,
    TURING_FUN_PLAY_OPR_PREV  = 2005,
    TURING_FUN_PLAY_OPR_NEXT  = 2006,
    TURING_FUN_PLAY_OPR_STOP  = 2002,
    TURING_FUN_PLAY_OPR_CYCLE = 3002,
    TURING_FUN_PLAY_OPR_PAUSE = 1200,
    TURING_FUN_PLAY_OPR_CONTINUE  = 1300,
};

typedef enum {
    TURING_ERR_NON,
    TURING_ERR_MALLOC,
    TURING_ERR_ENC_INIT,
    TURING_ERR_ENC_OPEN,
    TURING_ERR_ENC_START,
    TURING_ERR_ENC_PAUSE,
    TURING_ERR_ENC_STOP,
    TURING_ERR_ENC_WRITE,
    TURING_ERR_POINT_NULL,
    TURING_ERR_THREAD_FORK,
    TURING_ERR_SOCKET_FD,
    TURING_ERR_BIND,
    TURING_ERR_HOST_IP,
    TURING_ERR_CONNECT,
    TURING_ERR_SEND_DATA,
    TURING_ERR_RECV_DATA,
    TURING_ERR_HTTP_CODE,
} TURING_ERR;

struct turing_para {
    ASR_TYPE asr;
    TTS_TYPE tts;
    OUT_TXT  flag;
    STREAM_CTL real_time;
    int index;
    int book_id;
    int camera_id;
    int page_num;
    int piece_index;
    CMD_DODE cmd_code;
    ENCODE_MODE encode;
    MUTUAL_TYPE type;
    u8 speed;	//1-9
    u8 pitch;	//1-9
    u8 tone;	//0-15 20-22
    u8 volume;	//1-9
    u8 asr_lan;	//0-zh 1-en
    u8 tts_lan;	//0-zh 1-en
    u8 voice_mode;
    u8 recv_rsp_cnt;
    u8 ocr_flag;
    u8 finger_flag;
    u8 oral_flag;
    char identify[20];
    char token[65];
    char user_id[17];
    char aes_key[17];
    char api_key[33];
    const char *oral_string;
};


int turing_app_init(void);
void turing_app_uninit(void);
int turing_recorder_start(u16 sample_rate, u8 voice_mode);
int turing_recorder_stop(u8 voice_mode);
void JL_turing_media_speak_play(const char *url);
void JL_turing_media_audio_play(const char *url);
void JL_turing_picture_audio_play(const char *url);
void JL_turing_volume_change_notify(int volume);
void JL_turing_rec_err_notify(void *arg);
u8 get_turing_msg_notify(void);
u8 get_turing_rec_is_stopping(void);
u8 turing_app_get_connect_status(void);

#endif  //_TURING_H_
