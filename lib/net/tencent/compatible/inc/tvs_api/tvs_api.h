
/**
* @file  tvs_api.h
* @brief TVS SDK对外接口
* @date  2019-5-10
* 接入TVS SDK并实现TVS Adapter，需要遵循如下流程：
*
* - 实现Meidia Player Adapter定义的各个回调函数，赋予SDK播放流媒体的数据和TTS的能力；监听系统媒体播放器状态，实时同步播放状态给SDK；
* - 实现Platform Adapter定义的各个回调函数，赋予SDK操作声卡、获取/设置音量、保存/读取全局配置等系统能力；监听系统网络状态变化、音量变化，实时同步给SDK；
* - 实现Alert Adapter定义的各个回调函数，赋予SDK操作闹钟、提醒的能力；监听闹钟振铃状态，实时同步给SDK；
* - 初始化SDK，注册监听器监听SDK状态变化；
* - SDK授权，确定有账号/无账号方案，监听授权状态并保存授权结果；
*/

#ifndef __TVS_API_H_1FJILELIHUY34__
#define __TVS_API_H_1FJILELIHUY34__

#include <stdbool.h>
#include "tvs_authorize.h"
#include "tvs_media_player_adapter.h"
#include "tvs_platform_adapter.h"
#include "tvs_alert_adapter.h"
#include "tvs_common_def.h"

/**
 * @brief 开启智能语音对话
 *
 * @param
 * @return 为0代表开启成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_start_recognize();

/**
 * @brief 此函数已失效
 *
 * @param use_8k_bitrate 设置为true，代表录音阶段采用8000Hz采样率，设置为false，代表录音阶段采用16000Hz采样率
 * @param use_speex 设置为true，在录音阶段对PCM采用speex编码并上传到云端进行识别，设置为false，不进行编码，直接上传PCM
 * @return 为0代表开启成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_start_recognize_ex(bool use_8k_bitrate, bool use_speex);

/**
 * @brief 中止当前的智能语音对话的录音流程，并立即开始识别
 *
 * @param
 * @return 为0代表开功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_stop_recognize();

/**
 * @brief 中止当前正在进行的智能语音对话、播放控制等活动
 *
 * @param
 * @return 为0代表成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_stop_all_activity();

/**
 * @brief 播放控制，切到下一首
 *
 * @param
 * @return 为0代表切换成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_playcontrol_next();

/**
 * @brief 播放控制，切到上一首
 *
 * @param
 * @return 为0代表切换成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_playcontrol_previous();

/**
 * @brief 智能文本解析接口
 *
 * @param text 待解析的文本
 * @return 为0代表成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_start_text_recognize(const char *text);

/**
 * @brief 明确语义接口
 *
 * @param semantic 待解析的明确语义
 * @return 为0代表成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_send_semantic(const char *semantic);

/**
 * @brief 语音合成接口，语音合成参数，包括音量、声调、语速和音色均采用默认值
 *
 * @param text 待语音合成的文本
 * @return 为0代表成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_start_text_to_speech(char *text);

/**
 * @brief 语音合成接口，带参数
 *
 * @param tts_param 语音合成的参数，包括本文、音量、声调、语速和音色等
 * @return 为0代表成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_start_text_to_speech_ex(tvs_api_tts_param *tts_param);

/**
 * @brief 初始化SDK
 *
 * @param api_callbacks 监听器
 * @param produce_qua 产品QUA，用于区分不同的产品，以及相同产品的不同版本
 * @param config SDK默认配置
 * @return 0代表初始化成功，其他值代表失败
 */
int tvs_api_init(tvs_api_callback *api_callbacks, tvs_default_config *config, tvs_product_qua *produce_qua);

/**
 * @brief 监听后台下发的语音转文本结果
 *
 * @param callback 监听器
 * @return 0代表设置成功，其他值代表失败
 */
int tvs_api_set_asr_callback(tvs_callback_on_asr_result callback);

/**
 * @brief 此函数已失效
 *
 *
 */
void tvs_api_set_env(tvs_api_env env);

/**
 * @brief 此函数已失效
 *
 */
void tvs_api_set_sandbox(bool open_sandbox);

/**
 * @brief 获取当前模式
 *
 * @param
 * @return 当前模式（普通/宝拉/儿童模式等）
 */
tvs_mode tvs_api_get_current_mode();

/**
 * @brief 禁止/允许打印日志，默认为禁止打印
 *
 * @param enable
 * @return 无
 */
void tvs_api_log_enable(bool enable);

/**
 * @brief 启动SDK
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
int tvs_api_start();

/**
 * @brief 停止SDK，用于有账号方案重新授权的情况
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
int tvs_api_stop();

/**
 * @brief 创建一个新的session id，代表开启一轮新的智能语音对话
 *
 * @param 无
 * @return 新的session id
 */
int tvs_api_new_session_id();

/**
 * @brief 获取当前的session id
 *
 * @param 无
 * @return 当前的session id
 */
int tvs_api_get_current_session_id();

/**
 * @brief 设备端响应本地热词唤醒事件，开始一轮新的智能语音识别；
 * 此函数用于在SDK之外提供前端音频处理模块的方案。在收到本地热词唤醒事件时，接入方需要调用此函数，准备接收前端
 * 音频模块发送的语音数据，需要配合tvs_api_audio_provider_writer_begin一起使用；
 *
 * @param session_id -- 通过调用tvs_api_new_session_id函数获取
 * @param wakeword -- 设备端使用的唤醒词文本，比如“叮当叮当”
 * @param startIndexInSamples -- 唤醒词的第一个采样点，在语音流中的索引
 * @param endIndexInSamples -- 唤醒词的最后一个采样点在语音流中的索引
 * @return 0代表成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_on_voice_wakeup(int session_id, const char *wakeword, int startIndexInSamples, int endIndexInSamples);

/**
 * @brief 通知SDK，前端音频模块准备开始录音
 * 此函数用于在SDK之外提供前端音频处理模块的方案。
 * 收到本地热词唤醒事件后，先调用此函数，再调用tvs_api_on_voice_wakeup
 *
 * @param 无
 * @return 0代表成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_audio_provider_writer_begin();

/**
 * @brief 前端音频模块监听SDK的停止事件；
 *
 * @param callback 监听器
 * @return 0代表成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_audio_provider_listen(tvs_api_callback_on_provider_reader_stop callback);

/**
 * @brief 通知SDK，前端音频模块录音停止，一般是检测到了本地VAD的结束标识
 * 此函数用于在SDK之外提供前端音频处理模块的方案。
 * 前端音频模块收到本地VAD的结束标识时，调用此函数
 *
 * @param 无
 * @return 无
 */
void tvs_api_audio_provider_writer_end();

/**
 * @brief 将前端音频模块采集到的语音（PCM）喂给SDK，SDK将会为这些数据编码，并发送到云端进行识别
 *
 * @param audio_data 前端音频模块采集到的PCM数据
 * @param size       前端音频模块采集到的PCM数据的长度，要求小于1k字节
 * @return 0代表成功，其他值代表失败，见TVS_API_ERROR_*
 */
int tvs_api_audio_provider_write(const char *audio_data, int size);


#endif
