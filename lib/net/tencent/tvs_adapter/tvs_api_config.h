#ifndef __TVS_API_CONFIG_H__
#define __TVS_API_CONFIG_H__

// 1 -- 访客授权;  0 -- 设备授权
#define CONFIG_USE_GUEST_AUTHORIZE   0

// 1 -- TTS阶段，在SDK内部解码，并通过回调tvs_platform_adaptor_soundcard_pcm_write播放PCM
// 0 -- TTS阶段，SDK通过回调tvs_mediaplayer_adapter_tts_data, 将MP3数据传出，由上层解码并播放
#define CONFIG_DECODE_TTS_IN_SDK     0

#ifndef PLATFORM_RT_THREAD
#define TVS_FREE             free
#else
#include <rtthread.h>
#define TVS_FREE  rt_free
#endif

#define TVS_ADAPTER_PRINTF   printf

#define ARREARS_ENABLE		1	//欠费屏蔽使能
#define USE_MAC_DEVICE_NAME	0	//使用MAC地址作为device_name
#define OTA_MQTT_ENABLE		1	//OTA使能

#define FW_RUNNING_VERSION "1.0.0"	//固件版本号

//腾讯云flash索引地址
#define TENCENT_PATH "mnt/sdfile/app/tencent"

#if ARREARS_ENABLE

typedef enum {
    OPEN_AUDIO,				//打开音频播放
    CLOSE_WITH_PROMPT,		//有欠费提示的关闭
    CLOSE_WITHOUT_PROMPT,	//没欠费提示的关闭
} arrears_list;

extern int arrears_type;			//欠费类型
extern u8  audio_play_disable;			//tts音频播放禁用
extern int 	arrears_flag;			//欠费标志位

#endif

int tvs_get_devInfo(char       *tvs_productId, char *tvs_dsn);
int tvs_set_devInfo(char       *tvs_productId, char *tvs_dsn);

/***************配网部分*****************/
#include "os_wrapper.h"
extern bool BTCombo_complete;			//蓝牙配网完成标志位
extern void *BTCombo_mutex;

//配网标志位上锁
#define BTCOMBO_LOCK os_wrapper_wait_signal(BTCombo_mutex,1000);
//配网标志位解锁
#define BTCOMBO_UNLOCK os_wrapper_post_signal(BTCombo_mutex);

//读取配网标志位
bool read_BTCombo(void);
//写入配网标志位
void write_BTCombo(bool flag);


/*******************信息上报部分*****************/
int report_dev_sys_info(void);	//设备基本信息上报

#if OTA_MQTT_ENABLE

void report_ota_info(void);	//ota信息上报

#endif




#endif
