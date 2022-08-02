/*AliGenie SDK for FreeRTOS header*/
/*Ver.20180509*/

#ifndef _ALIGENIE_SDK_HEADER_
#define _ALIGENIE_SDK_HEADER_

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * aligenie sdk callbacks return value defination.
 */
typedef enum {
    /*0*/ AGR_OK = 0,
    /*1001*/ AGR_REGISTER_NETWORK_ERROR = 1001,
    /*1002*/ AGR_REGISTER_APPENDED,
    /*1003*/ AGR_REGISTER_IGNORED,
    /*1004*/ AGR_REGISTER_ERROR_MAC_CHANGED,
    /*1005*/ AGR_KEY_HANDLED,
    /*1006*/ AGR_KEY_UNHANDLED_UNRECGONIZED,
    /*1007*/ AGR_KEY_UNHANDLED_UNINITIALIZED,
    /*1008*/ AGR_KEY_UNHANDLED_TOO_FREQUENT,
    /*1009*/ AGR_KEY_UNHANDLED_NETWORK_DISCONNECTED,
    /*1010*/ AGR_KEY_UNHANDLED_IGNORED,
    /*1011*/ AGR_UNKNOWN_ERROR,
} AGRET_E;

/**
 * Network status defination
 */
typedef enum {
    /*2001*/ AG_NETWORK_UNKNOWN = 2001,
    /*2002*/ AG_NETWORK_CONNECTED,
    /*2003*/ AG_NETWORK_DISCONNECT,
} AG_NETWORK_STATUS_E;

/**
 * Keyevent defination
 */
typedef enum {
    /*3001*/ AG_KEYEVENT_AI_START = 3001,    //语音识别键按下
    /*3002*/ AG_KEYEVENT_AI_STOP,            //语音识别键弹起
    /*3003*/ AG_KEYEVENT_INTERCOM_START,     //留言键按下
    /*3004*/ AG_KEYEVENT_INTERCOM_STOP,      //留言键弹起
    /*3005*/ AG_KEYEVENT_INTERCOM_PLAY,      //播放留言键
    /*3006*/ AG_KEYEVENT_TRANSLATE_START,    //汉译英键按下
    /*3007*/ AG_KEYEVENT_TRANSLATE_STOP,     //汉译英键弹起
    /*3008*/ AG_KEYEVENT_NEXT,               //下一首键
    /*3009*/ AG_KEYEVENT_PREVIOUS,           //上一首键
    /*3010*/ AG_KEYEVENT_PLAYPAUSE,          //播放暂停键
    /*3011*/ AG_KEYEVENT_PLAY_DOMAIN_SEQ,    //按故事-音乐-国学-英语的顺序，循环播放四个领域之一
    /*3012*/ AG_KEYEVENT_PLAY_MUSIC,         //播放音乐键
    /*3013*/ AG_KEYEVENT_PLAY_STORY,         //播放故事键
    /*3014*/ AG_KEYEVENT_PLAY_SINOLOGY,      //播放国学键
    /*3015*/ AG_KEYEVENT_PLAY_ENGLISH,       //播放英语键
    /*3016*/ AG_KEYEVENT_VAD_START,          //VAD开始
    /*3017*/ AG_KEYEVENT_VAD_STOP,           //VAD结束
    /*3018*/ AG_KEYEVENT_VOLUME_UP,          //音量增
    /*3019*/ AG_KEYEVENT_VOLUME_DOWN,        //音量减
    /*3020*/ AG_KEYEVENT_DISCARD_SPEECH,         //丢弃VAD
    /*3021*/ AG_KEYEVENT_ADD_REMOVE_FAVORITE,    //收藏和取消收藏键
    /*3022*/ AG_KEYEVENT_PLAY_FAVORITE,          //播放收藏键
    /*3023*/ AG_KEYEVENT_PLAY_BREAKPOINT,   //续播之前的媒体
} AG_KEYEVENT_E;

/**
 * Player status defination
 */
typedef enum {
    /*4001*/ AG_PLAYER_URL_PLAYING = 4001,
    /*4002*/ AG_PLAYER_URL_PAUSED,
    /*4003*/ AG_PLAYER_URL_FINISHED,
    /*4004*/ AG_PLAYER_URL_STOPPED,
    /*4005*/ AG_PLAYER_URL_SEEKING,
    /*4006*/ AG_PLAYER_URL_ERROR,
    /*4007*/ AG_PLAYER_URL_UNSUPPORTED_FORMAT,
    /*4008*/ AG_PLAYER_URL_RESUME,
    /*4009*/ AG_PLAYER_URL_INVALID,
} AG_PLAYER_STATUS_E;

/**
 * URL Media info struct
 */
typedef struct {
    char audioAuthor[50];
    char audioAlbum[50];
    char audioName[50];
    char audioId[12];
    char audioSource[50];
    char url[256];
    char traceId[32];
    char commandId[32 + 1];
    char audioType[32];

    /*
     * Total length of media in seconds,
     * this value should be written in player,
     * before as the param of callback function ag_notify_player_status_change.
     */
    int totalLen;

    /*
     * media play status
     * this value should be written in player,
     * before as the param of callback function ag_notify_player_status_change.
     */
    AG_PLAYER_STATUS_E status;
} AG_AUDIO_INFO_T;

/*************************************************************/
/*                         Callbacks                         */
/*                     Implemented in SDK                    */
/*************************************************************/

/**
 * Summary:
 *     AliGenie component initialize.
 * Return:
 *     type of AG_RET_E, init result.
 */
extern AGRET_E ag_sdk_init();

/**
 * Summary:
 *     AliGenie keyevent processor.
 * Parameters:
 *     keyevent: value in enum AG_KEYEVENT_E.
 * Return:
 *     keyevent handle result.
 */
extern AGRET_E ag_sdk_notify_keyevent(AG_KEYEVENT_E keyevent);

/**
 * Summary:
 *     set device register info into AliGenie SDK.
 * Parameters:
 *     mac: MAC address in format of 12:34:56:78:90:AB
 *         It is highly recommended that all letters should be in capitalized.
 *     extUserId: must be unique in each single manufacture
 *     authCode: get it from AliGenie userinfo API.
 *     forcely: If it set to 0, register request will be sent after server is connected.
 *              Otherwise, send register request immediatly, return error when failed.
 *              Usage: set to 0 after boot up, and set to 1 when a new user bind this device.
 * Return:
 *     enum value of AG_RET_E, register result.
 */
extern AGRET_E ag_sdk_set_register_info(const char *mac, const char *extUserId, const char *authCode, char forcelySend);

/**
 * Summary:
 *     Tells AliGenie component the status of network connection.
 * Parameters:
 *     networkStatus: AG_NETWORK_STATUS_E
 */
extern void ag_sdk_notify_network_status_change(AG_NETWORK_STATUS_E networkStatus);

/**
 * Summary:
 *     Player callback function, tells AliGenie component the status of audio player.
 * ATTENTION:
 *     1. If a local media is playing, this function should NEVER be called.
 *     2. callback player status ONLY when playing url, prompt and tts status should NEVER call it.
 * Parameters:
 *     audioStatus: struct of AG_AUDIO_STATUS_T.
 */
extern void ag_sdk_notify_player_status_change(AG_AUDIO_INFO_T *audioInfo);

/**
 * Take the control of player from aligenie.
 * After this call, aligenie will not call the ag_audio_idle (callback function in aligenie_audio.h).
 * Until the next url play from aligenie is started.
 */
extern void ag_sdk_take_player_control(void);

/**
 * NLP request to gw.
 * char *ai_request = "来点音乐";
 */
extern void ag_event_nlp_request(char *ai_request);


#ifdef __cplusplus
}
#endif
#endif /*_ALIGENIE_SDK_HEADER_*/

