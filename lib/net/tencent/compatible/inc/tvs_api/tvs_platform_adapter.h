
/**
* @file  tvs_platform_adapter.h
* @brief TVS platform 接口层
* @date  2019-5-10
*/

#ifndef __TVS_PLATFORM_ADAPTER_H_FHUIEWRT__
#define __TVS_PLATFORM_ADAPTER_H_FHUIEWRT__

/**
* @brief 录音/播放状态
*/
typedef struct {
    int bitrate;   /*!< 采样率，取值8000/16000 */
    int channel;   /*!< 声道数，一般为1 */
} tvs_soundcard_config;

/**
 * @brief 实现此接口，向TVS SDK提供开启/关闭PCM语音录制、播放功能的能力；
 *
 * @param open 开启/关闭
 * @param recorder 设置为true，代表操作PCM录制功能, 设置为false，代表操作PCM播放功能
 * @param sc_config 声卡参数, 只用于开启PCM录制或者播放的情况，其他情况都为NULL
 * @return 等于0代表操作成功，其他值代表操作失败
 */
typedef int (*tvs_platform_adaptor_soundcard_control)(bool open, bool recorder, tvs_soundcard_config *sc_config);

/**
 * @brief 实现此接口，向TVS SDK提供录制PCM语音的能力
 *
 * @param data 录制PCM语音的缓冲区
 * @param data_bytes 缓冲区data的长度，单位为字节
 * @return 大于0，代表实际录制的字节数，小于等于0代表录制失败
 */
typedef int (*tvs_platform_adaptor_soundcard_pcm_read)(void *data, unsigned int data_bytes);

/**
 * @brief 实现此接口，向TVS SDK提供播放PCM语音的能力
 *
 * @param data 待播放的PCM语音的缓冲区
 * @param data_bytes 缓冲区data的长度，单位为字节
 * @return 大于0，代表实际播放的字节数，小于等于0代表播放失败
 */
typedef int (*tvs_platform_adaptor_soundcard_pcm_write)(void *data, unsigned int data_bytes);

/**
 * @brief 实现此接口，向TVS SDK提供获取当前网络状态的能力
 *
 * @param
 * @return
 *		为true，代表当前网络处于连接状态；
 *	 	为false，代表当前网络处于断开状态；
 */
typedef bool (*tvs_platform_adaptor_is_network_valid)();

/**
 * @brief 实现此接口，向TVS SDK提供设置音量的能。
 * 在TVS云端识别到用户的设置音量请求时，将下发音量设置命令，此函数将被回调
 *
 * @param cloud_volume TVS云端下发的音量，需要转换为当前终端的实际音量，计算公式为(cloud_volume * 设备端平台支持的最大音量值 / 100)
 * @param max_value SDK定义的最大音量值，固定为100
 * @param do_init 取值true代表SDK当前正从preference中读取音量并设置（一般在调用tvs_platform_adapter_init过程中）
 *                取值false代表SDK当前正响应语音对话指令，或者从APP一侧设置音量
 * @return
 * 		0为设置成功，其他值为设置失败
 */
typedef int (*tvs_platform_adaptor_set_current_volume)(int cloud_volume, int max_value, bool do_init);

/**
 * @brief 实现此接口，向TVS SDK提供读取持久化配置的能力。
 * 一般在设备重启的时候，会回调此函数加载持久化配置的各项参数
 *
 * @param preference_size 出参，代表当前持久化配置数据占用内存空间的总字节数
 * @return
 *		当前系统的持久化配置数据，由此函数内部通过malloc在堆上申请内存，SDK调完此接口，将会调用free释放内存
 */
typedef const char *(*tvs_platform_adaptor_load_preference)(int *preference_size);

/**
 * @brief 实现此接口，向TVS SDK提供保存持久化配置的能力。
 * SDK在各种配置参数的值改变的时候，会回调此函数，保存新的持久化配置
 *
 * @param preference_buffer 带保存的持久化配置数据
 * @param preference_size 当前持久化配置数据占用内存空间的总字节数
 * @return
 *		0为保存成功，其他值为保存失败
 */
typedef int (*tvs_platform_adaptor_save_preference)(const char *preference_buffer, int preference_size);


typedef struct {
    tvs_platform_adaptor_soundcard_control soundcard_control;
    tvs_platform_adaptor_soundcard_pcm_read soundcard_pcm_read;
    tvs_platform_adaptor_soundcard_pcm_write soundcard_pcm_write;
    tvs_platform_adaptor_is_network_valid is_network_valid;
    tvs_platform_adaptor_set_current_volume set_volume;
    tvs_platform_adaptor_load_preference load_preference;
    tvs_platform_adaptor_save_preference save_preference;
} tvs_platform_adaptor;

/**
 * @brief 初始化Platform Adapter
 *
 * @param adapter 已经实现的各个回调函数
 * @return
 *		0为成功，其他值为失败
 */
int tvs_platform_adapter_init(tvs_platform_adaptor *adapter);

/**
 * @brief 当系统音量改变的时候，需要调用此函数通知sdk
 *
 * @param cloud_volume 当前音量值，取值为(当前设备端音量 * 100 /设备端平台支持的最大音量值)
 * @return
 *		0为成功，其他值为失败
 */
void tvs_platform_adapter_on_volume_changed(int cloud_volume);

/**
 * @brief 当网络状态发生变化的时候，需要调用此函数通知SDK;
 * 注意，在配网时终端作为AP的情况下，网络应该被认为是断开状态，
 * 直到配网结束，终端退出AP模式，重新连上目标WIFI之后，才能调用此函数
 *
 * @param connect 为true表示当前终端已经连上外网，为false表示当前网络断开
 * @return
 */
void tvs_platform_adapter_on_network_state_changed(bool connect);

#endif