
/**
* @file  tvs_media_player_adapter.h
* @brief TVS 流媒体播放适配接口层
* @date  2019-5-10
*/

#ifndef __TVS_MEDIA_PLAYER_ADAPTER_H__
#define __TVS_MEDIA_PLAYER_ADAPTER_H__

#include <stdint.h>

/**
 * @brief 手动停止播放
 */
#define TVS_MEDIA_PLAYER_ERROR_MANNUAL           0
/**
 * @brief 播放出错
 */
#define TVS_MEDIA_PLAYER_ERROR_OTHER            -1

/**
 * 当用户通过智能语音、媒体播控，或者在APP端推送媒体时，云端将媒体下发到设备端，SDK将回调此接口，
 * 实现此接口，将下发的新媒体的流媒体url，以及媒体对应的唯一标识，和需要快进的进度设置给平台播放器，
 * 准备开始播放
 *
 * @param song_url 待播放的流媒体url。
 * @param token 媒体对应的唯一标识。
 * @param offsetInSeconds 如果此参数大于0，代表此媒体需要快进到offsetInSeconds秒处，再开始播放
 * @return 0代表成功，其他值代表失败
 */
typedef int (*tvs_mediaplayer_adapter_set_source)(const char *song_url, const char *token, uint32_t offsetInSeconds);

/**
 * @brief 开始播放
 * 当云端下发了一个新媒体后，SDK将调用此接口。
 * 需要实现此接口，调用平台的播放器，播放之前set_source阶段下发的媒体url
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
typedef int (*tvs_mediaplayer_adapter_start_play)(const char *token);

/**
 * @brief 停止播放
 * 当用户通过语音暂停/停止播放（比如通过语音输入“暂停播放”、“停止播放”等指令），
 * 或者在APP端暂停/停止播放之前下发的媒体的时候，SDK将会调用此接口。
 * 实现此接口，调用平台播放器的stop功能，停止播放之前下发的流媒体
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
typedef int (*tvs_mediaplayer_adapter_stop_play)(const char *token);

/**
 * @brief 继续播放
 * 在终端SDK从繁忙状态进入空闲状态后（比如当前一轮智能语音交互、媒体播控等工作结束），SDK将回调此接口，
 * 继续播放之前下发的流媒体；如果之前终端并未处于播放网络流媒体的状态，则SDK不会回调此接口。
 * 实现此接口，调用平台播放器的resume功能
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
typedef int (*tvs_mediaplayer_adapter_resume_play)(const char *token);

/**
 * @brief 暂停播放
 * 在终端SDK进入繁忙状态（比如开始输入语音、开始媒体播控等情况下），SDK将回调此接口，
 * 暂停当前播放的流媒体。
 * 实现此接口，调用平台播放器的pause功能，暂停播放当前流媒体，以让出资源供SDK执行对应工作
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
typedef int (*tvs_mediaplayer_adapter_pause_play)(const char *token);

/**
 * @brief 获取当前播放的网络流媒体的已播放时间
 *
 * @param
 * @return 已经播放的时间，单位为毫秒
 */
typedef int (*tvs_mediaplayer_adapter_get_offset)(const char *token);

/**
 * @brief media player是否有效，如果无效，新的智能语音对话无法发起
 *
 * @param
 * @return true代表有效，false代表无效
 */
typedef bool (*tvs_mediaplayer_adapter_is_player_valid)();

/**
 * @brief 是否在SDK内部播放TTS(可选)
 *
 * @param
 * @return true代表在SDK内部播放TTS，false代表由适配层接管TTS的播放，必须实现tts_start、tts_end和tts_data
 */
typedef bool (*tvs_mediaplayer_adapter_play_tts_inner)();

/**
 * @brief TTS流开始播放，此回调函数被调用，代表SDK已经准备下发TTS数据流，此时需要准备并开启播放器，等待TTS数据写入(可选)
 *
 * @param type TTS数据类型，1为MP3，其他值保留
 * @return 0代表
 */
typedef int (*tvs_mediaplayer_adapter_tts_start)(int type);

/**
 * @brief TTS流结束播放(可选)
 *
 * @param force_stop 取值true代表强制结束，不管当前是否还有TTS数据未播放完毕
 *                   取值false代表此回调需要阻塞等待TTS数据播放完毕后，停止播放器，再返回
 * @return
 */
typedef void (*tvs_mediaplayer_adapter_tts_end)(bool force_stop);

/**
 * @brief TTS流数据下发(可选)
 *
 * @param data TTS流数据
 * @param data_len 当前下发的TTS流数据的字节数
 * @return
 */
typedef int (*tvs_mediaplayer_adapter_tts_data)(char *data, int data_len);


typedef struct {
    tvs_mediaplayer_adapter_set_source set_source;
    tvs_mediaplayer_adapter_start_play start_play;
    tvs_mediaplayer_adapter_stop_play stop_play;
    tvs_mediaplayer_adapter_resume_play resume_play;
    tvs_mediaplayer_adapter_pause_play pause_play;
    tvs_mediaplayer_adapter_get_offset get_offset;
    tvs_mediaplayer_adapter_is_player_valid is_player_valid;
    tvs_mediaplayer_adapter_play_tts_inner play_inner;
    tvs_mediaplayer_adapter_tts_start tts_start;
    tvs_mediaplayer_adapter_tts_end tts_end;
    tvs_mediaplayer_adapter_tts_data tts_data;
} tvs_mediaplayer_adapter;

/**
 * @brief 初始化mediaplayer adapter
 *
 * @param adapter
 * @return
 */
void tvs_mediaplayer_adapter_init(tvs_mediaplayer_adapter *adapter);

/**
 * @brief 播放腾讯云叮当下发的网络媒体结束的时候，调用此函数通知TVS SDK
 *
 * @param token 网络媒体token
 * @return
 */
void tvs_mediaplayer_adapter_on_play_finished(const char *token);

/**
 * @brief 正在播放腾讯云叮当下发的网络媒体的时候，出错或者停止时，调用此函数通知TVS SDK
 *
 * @param error_code 错误码，见TVS_MEDIA_PLAYER_ERROR_*
 * @param token 网络媒体token
 * @return
 */
void tvs_mediaplayer_adapter_on_play_stopped(int error_code, const char *token);

/**
 * @brief 播放腾讯云叮当下发的网络媒体开始播放的时候，调用此函数通知TVS SDK
 *
 * @param token 网络媒体token
 * @return
 */
void tvs_mediaplayer_adapter_on_play_started(const char *token);

/**
 * @brief 播放腾讯云叮当下发的网络媒体暂停的时候，调用此函数通知TVS SDK
 *
 * @param token 网络媒体token
 * @return
 */
void tvs_mediaplayer_adapter_on_play_paused(const char *token);

#endif
