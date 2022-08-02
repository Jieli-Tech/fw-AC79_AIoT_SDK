#ifndef __TVS_MEDIA_PLAYER_H__
#define __TVS_MEDIA_PLAYER_H__

#include <stdint.h>

typedef enum {
    TVS_MEDIA_PLAYER_STATE_STARTED,
    TVS_MEDIA_PLAYER_STATE_FINISHED,
    TVS_MEDIA_PLAYER_STATE_STOPED,
    TVS_MEDIA_PLAYER_STATE_PAUSED,
    TVS_MEDIA_PLAYER_STATE_ERROR,
} tvs_media_player_state;
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
int tvs_media_player_set_source(const char *song_url, const char *token, unsigned int offsetInSeconds);

/**
 * @brief 开始播放
 * 当云端下发了一个新媒体后，SDK将调用此接口。
 * 需要实现此接口，调用平台的播放器，播放之前set_source阶段下发的媒体url
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
int tvs_media_player_start_play(const char *token);

/**
 * @brief 停止播放
 * 当用户通过语音暂停/停止播放（比如通过语音输入“暂停播放”、“停止播放”等指令），
 * 或者在APP端暂停/停止播放之前下发的媒体的时候，SDK将会调用此接口。
 * 实现此接口，调用平台播放器的stop功能，停止播放之前下发的流媒体
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
int tvs_media_player_stop_play(const char *token);

/**
 * @brief 继续播放
 * 在终端SDK从繁忙状态进入空闲状态后（比如当前一轮智能语音交互、媒体播控等工作结束），SDK将回调此接口，
 * 继续播放之前下发的流媒体；如果之前终端并未处于播放网络流媒体的状态，则SDK不会回调此接口。
 * 实现此接口，调用平台播放器的resume功能
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
int tvs_media_player_resume_play(const char *token);

/**
 * @brief 暂停播放
 * 在终端SDK进入繁忙状态（比如开始输入语音、开始媒体播控等情况下），SDK将回调此接口，
 * 暂停当前播放的流媒体。
 * 实现此接口，调用平台播放器的pause功能，暂停播放当前流媒体，以让出资源供SDK执行对应工作
 *
 * @param
 * @return 0代表成功，其他值代表失败
 */
int tvs_media_player_pause_play(const char *token);

/**
 * @brief 获取当前播放的网络流媒体的已播放时间
 *
 * @param
 * @return 已经播放的时间，单位为毫秒
 */
int tvs_media_player_get_offset(const char *token);

/**
 * @brief 获取当前播放器的音量
 *
 * @param
 * @return 播放器音量，取值0~100
 */
int tvs_media_player_get_volume();

/**
 * @brief 设置当前播放器的音量
 *
 * @param volume 播放器音量，取值0~100
 * @return 无
 */
void tvs_media_player_set_volume(int volume);

/**
 * @brief 初始化mediaplayer
 *
 * @param
 * @return
 */
int tvs_media_player_init();

#endif
