
/**
* @file  tvs_common_def.h
* @brief TVS SDK结构体和参数、返回值、错误码等定义
* @date  2019-5-10
*/

#ifndef __TVS_COMMON_DEF_H__
#define __TVS_COMMON_DEF_H__

#include "generic/typedef.h"

//---------------------------------------------------------------------------
// 错误码
//---------------------------------------------------------------------------
/**
 * @brief 成功
 */
#define TVS_API_ERROR_NONE                                   0
/**
 * @brief 一般错误
 */
#define TVS_API_ERROR_OTHERS                                -1
/**
 * @brief 当前正处于智能语音对话或者播放控制中，无法启动
 */
#define TVS_API_ERROR_BUSY                                  -2
/**
 * @brief 当前播放器正忙，无法启动
 */
#define TVS_API_ERROR_MEDIAPLAYER_INVALID                   -3
/**
 * @brief 当前终端未鉴权
 */
#define TVS_API_ERROR_NOT_ATHORIZED                         -4
/**
 * @brief 当前终端网络连接异常
 */
#define TVS_API_ERROR_NETWORK_INVALID                       -5
/**
 * @brief 由于终端无client id，导致授权失败
 */
#define TVS_API_ERROR_CLIENT_ID_INVALID                     -6

/**
 * @brief 由于终端SDK处于STOP状态
 */
#define TVS_API_ERROR_NOT_RUNNING                           -7

/**
 * @brief 由于网络异常导致后台无响应
 */
#define TVS_API_ERROR_NETWORK_ERROR                         -8

/**
 * @brief 执行播控切换后，后台未下发媒体，一般是因为歌单播放完毕导致
 */
#define TVS_API_ERROR_NO_MORE_MEDIA                         -10

#define TVS_API_ERROR_INVALID_PARAMS                        -11


/**
* @brief SDK状态
*/
typedef enum {
    TVS_STATE_IDLE,           /*!< 空闲状态 */
    TVS_STATE_PREPARING,      /*!< 准备状态 */
    TVS_STATE_RECOGNIZNG,     /*!< "语音识别中"状态 */
    TVS_STATE_BUSY,           /*!< 忙状态 */
    TVS_STATE_SPEECH_PLAYING, /*!< TTS播报状态 */
} tvs_recognize_state;

#define TVS_STATE_COMPLETE   TVS_STATE_IDLE

/**
* @brief SDK环境
*/
typedef enum {
    TVS_API_ENV_TEST,       /*!< 测试环境 */
    TVS_API_ENV_NORMAL,     /*!< 正式环境 */
    TVS_API_ENV_EXP,        /*!< 体验环境 */
    TVS_API_ENV_DEV,        /*!< 开发环境 */
} tvs_api_env;

/**
* @brief 人设/模式
*/
typedef enum {
    TVS_MODE_NORMAL, /*!< 普通模式 */
    TVS_MODE_BAOLA,  /*!< 宝拉模式 */
    TVS_MODE_CHILD,  /*!< 儿童模式 */
    TVS_MODE_COUNT,
} tvs_mode;

/**
* @brief SDK配置参数
*/
typedef struct {
    tvs_api_env def_env;  /*!< SDK默认环境 */
    bool def_sandbox_open;   /*!< SDK默认是否开启沙箱，true代表默认开启 */
    int recorder_bitrate;   /*!< 录音参数，取值为8000/16000,  默认为8000 */
    int recorder_channels;  /*!< 录音参数，声道数，  默认为1 */
    int reserve[10];
} tvs_default_config;

/**
* @brief 产品QUA
*/
typedef struct {
    char *version;			    /*!< 终端版本号,必须为4段，每一段为一个数字，各个数字之间用"."分开，例如：1.0.1.1000；
									新的版本号必须比旧版本号大，例如
									2.0.1.101 > 2.0.1.100 > 1.0.1.1011 > 1.0.1.1000 */
    char *package_name;         /*!< 终端软件包名，用于区分不同产品，格式一般为com.companyname.product */
    char *reserve;			    /*!< 扩展字段，一般填NULL，仅用于有特殊需求时候，对QUA字段进行扩展，例如：&VIN=1001&DE=SPEAKER */
} tvs_product_qua;

/**
* @brief AUDIO_PROVIDER返回码
*/
typedef enum {
    TVS_API_AUDIO_PROVIDER_ERROR_NONE = 0,   /*!< 正常结束 */
    TVS_API_AUDIO_PROVIDER_ERROR_OTHERS = -1,     /*!< 其他原因 */
    TVS_API_AUDIO_PROVIDER_ERROR_STOP_CAPTURE = -2,    /*!< 收到云端VAD的结束标识 */
    TVS_API_AUDIO_PROVIDER_ERROR_TIME_OUT = -3,     /*!< 流式语音上传超时，一般是请求超时或者服务器未及时响应 */
    TVS_API_AUDIO_PROVIDER_ERROR_NETWORK = -10, 	/*!< 流式语音上传失败，由于网络原因导致 */
} tvs_api_audio_provider_error;

/**
* @brief 设备端启动智能语音的方式，包括按下说话/抬起停止模式、单击按钮录音模式以及唤醒词唤醒模式
*/
typedef enum {
    TVS_RECOGNIZER_PRESS_AND_HOLD,     /*!< push to talk 模式 */
    TVS_RECOGNIZER_TAP,                /*!< 点击按钮录音模式 */
    TVS_RECOGNIZER_WAKEWORD,           /*!< 唤醒词模式 */
    TVS_RECOGNIZER_TYPE_MAX,
} tvs_api_recognizer_type;

/**
* @brief 设备端当前正在执行的指令类型
*/
typedef enum {
    TVS_CONTROL_SPEECH = 100,       /*!< 语音对话 */
    TVS_CONTROL_PLAY_NEXT,          /*!< 播控切换，下一首 */
    TVS_CONTROL_PLAY_PREV,          /*!< 播控切换，上一首 */
    TVS_CONTROL_PLAY_FINISH,        /*!< 通知后台，播放完毕 */
    TVS_CONTROL_SEMANTIC,           /*!< 发送明确语义 */
} tvs_api_control_type;

/**
* @brief 设备端状态变化时的参数
*/
typedef struct {
    tvs_api_control_type control_type;  /*!< 当前执行的指令类型，见enum tvs_api_control_type */
    int error;                          /*!< 错误码，值见TVS_API_ERROR_ */
    int reserve[5];                     /*!< 预留*/
} tvs_api_state_param;


#define TVS_TIMBRE_ZHOULONGFEI    "ZHOULONGFEI"
#define TVS_TIMBRE_CHENANQI       "CHENANQI"
#define TVS_TIMBRE_YEZI           "YEZI"
#define TVS_TIMBRE_YEWAN          "YEWAN"
#define TVS_TIMBRE_DAJI           "DAJI"
#define TVS_TIMBRE_LIBAI          "LIBAI"
#define TVS_TIMBRE_NAZHA          "NAZHA"
#define TVS_TIMBRE_MUZHA          "MUZHA"
#define TVS_TIMBRE_WY             "WY"

/**
* @brief 语音合成参数
*/
typedef struct {
    char *tts_text;      /*!< 语音合成的内容 */
    char *timbre;		 /*!< 音色(取值见TVS_TIMBRE_*, 如果设置为NULL，后台进行语音合成时，将采用默认音色) */
    int volume;          /*!< 音量(取值0~100, 如果设置为-1，后台进行语音合成时，将采用默认值100) */
    int speed;           /*!< 语速(取值0~100, 如果设置为-1，后台进行语音合成时，将采用默认值50) */
    int pitch;           /*!< 声调(取值0~100, 如果设置为-1，后台进行语音合成时，将采用默认值50) */
} tvs_api_tts_param;

/**
 * @brief 监听SDK状态变化
 *
 * @param last_state 上一个状态。
 * @param new_state 下一个状态。
 * @param state_param 状态变化参数，见tvs_api_state_param的定义，需要进行一次转换：
 *                    tvs_api_state_param* param = (tvs_api_state_param*)state_param
 *
 * @return 无
 */
typedef void (*tvs_callback_on_state_changed)(tvs_recognize_state last_state, tvs_recognize_state new_state, void *state_param);

/**
 * @brief 监听多端互动推送信息，一般是来自手机APP端的自定义消息。
 * 实现此接口，可以接受这些自定义消息的推送，并跟进消息内容进行处理。
 *
 * @param text 推送信息，一般是转义后的Json字符串，要与消息推送方约定格式。
 * @param token 消息的唯一标识。
 * @return 无
 */
typedef void (*tvs_callback_on_terminal_sync)(const char *text, const char *token);

/**
 * @brief 监听模式切换，用户使用语音或者从手机APP端切换模式后，设备端会收到模式切换指令。
 * 实现此接口，可以在模式切换的时候得到通知
 *
 * @param src 当前模式。
 * @param dst 切换的目标模式。
 * @return 无
 */
typedef void (*tvs_callback_on_mode_changed)(tvs_mode src, tvs_mode dst);

/**
 * @brief 监听多轮会话事件，用户使用语音触发多轮会话的场景，设备端会收到多轮会话指令。
 * 实现此接口，可以在触发多轮对话的时候得到通知，需要触发一轮新的智能语音对话流程。
 *
 * @param 无
 * @return 无
 */
typedef void (*tvs_callback_on_expect_speech)();

/**
 * @brief 监听后台下发的自定义技能数据
 *
 * @param json_payload 自定义技能数据，为一个Json字符串
 * @return 无
 */
typedef void (*tvs_callback_on_recv_tvs_control)(const char *json_payload);

/**
 * @brief 监听后台下发的语音转文本结果
 *
 * @param asr_text 语音转文本结果
 * @param is_end 取值true代表asr_text为语音识别最终结果，false代表asr_text为语音识别的中间结果
 * @return 无
 */
typedef void (*tvs_callback_on_asr_result)(const char *asr_text, bool is_end);

/**
 * @brief 监听后台下发的厂商账号解绑消息
 *
 * @param unbind_time 解绑的时间
 * @return 无
 */
typedef void (*tvs_callback_on_unbind)(long long unbind_time);


/**
* @brief 向SDK注册的回调函数
*/
typedef struct {
    tvs_callback_on_state_changed on_state_changed;      /*!< 监听SDK状态变化 */
    tvs_callback_on_terminal_sync on_terminal_sync;      /*!< 监听多端互动推送信息 */
    tvs_callback_on_mode_changed on_mode_changed;        /*!< 监听模式切换 */
    tvs_callback_on_expect_speech on_expect_speech;      /*!< 多轮会话 */
    tvs_callback_on_recv_tvs_control on_recv_tvs_control;    /*!< 自定义技能 */
    tvs_callback_on_unbind on_unbind;	/*!< 解绑消息 */
    int reserve[10];
} tvs_api_callback;

/**
 * @brief 监听authorize的结果，需要持久化此结果，作为下一次authorize的依据
 *
 * @param ok 为true代表操作成功，为false代表操作失败。
 * @param auth_info authorize结果。
 * @param auth_info_len authorize结果的长度。
 * @param error 失败原因
 * @return 无
 */
typedef void (*tvs_authorize_callback)(bool ok, char *auth_info, int auth_info_len, const char *client_id, int error);

/**
 * @brief 当SDK内部的流式语音上传操作停止时，将回调此接口
 *
 * @param session_id 会话ID
 * @param error 停止原因，见tvs_api_audio_provider_error枚举
 * @return 0代表成功，其他值代表失败
 */
typedef void (*tvs_api_callback_on_provider_reader_stop)(int session_id, int error);

#endif
