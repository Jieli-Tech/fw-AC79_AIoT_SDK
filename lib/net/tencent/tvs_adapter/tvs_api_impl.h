#ifndef __TVS_API_IMPL_H__
#define __TVS_API_IMPL_H__

void tvs_api_impl_init();


enum TC_TVS_EVENT {
    TC_SPEAK_END     = 0x01,
    TC_MEDIA_END     = 0x02,
    TC_PLAY_PAUSE    = 0x03,
    TC_PREVIOUS_SONG = 0x04,
    TC_NEXT_SONG     = 0x05,
    TC_VOLUME_CHANGE = 0X06,
    TC_VOLUME_INCR   = 0x07,
    TC_VOLUME_DECR   = 0x08,
    TC_VOLUME_MUTE   = 0x09,
    TC_RECORD_START  = 0x0a,
    TC_RECORD_SEND   = 0x0b,
    TC_RECORD_STOP   = 0x0c,
    TC_VOICE_MODE    = 0x0d,
    TC_MEDIA_STOP    = 0x0e,
    TC_BIND_DEVICE   = 0x0f,
    TC_RECORD_ERR    = 0x10,
    TC_COLLECT_RESOURCE = 0x12,
    TC_PLAY_CONTIUE  = 0x13,
    TC_SET_VOLUME    = 0x14,
    TC_SPEAK_START   = 0x15,
    TC_MEDIA_START   = 0x16,
    TC_RECORD_BREAK  = 0x17,
    TC_WS_CONNECT_MSG = 0x18,
    TC_WS_DISCONNECT_MSG = 0x19,
    TC_RECV_CHAT     = 0x1a,
    TC_QUIT    	  = 0xff,
};

void tvs_taskq_post(int msg);

/**
 * @brief 收到唤醒事件，开始语音对话
 *
 * @param
 * @return
 */
void tvs_api_impl_speech_wakeup();

/**
 * @brief 前端语音模块写入语音到SDK中进行识别
 *
 * @param audio_data PCM语音数据
 * @param data_size 语音数据的字节数
 * @return
 */
void tvs_api_impl_speech_write_audio(const char *audio_data, int data_size);

/**
 * @brief 收到VAD END事件，结束语音对话
 *
 * @param
 * @return
 */
void tvs_api_impl_speech_write_end();

/**
 * @brief 初始化，注册回调函数，监听SDK识别事件
 *
 * @param
 * @return
 */
void tvs_api_impl_speech_init();

#endif
