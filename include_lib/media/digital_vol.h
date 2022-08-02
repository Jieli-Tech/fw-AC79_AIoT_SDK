#ifndef DIGITAL_VOL_API_H
#define DIGITAL_VOL_API_H

#include "system/includes.h"

/**
 * @brief 数字音量参数结构体
 */
typedef struct _digital_vol_open_parm {
    unsigned char ch;  		        /*!< 通道数 */
    unsigned char vol;  		    /*!< 当前音量大小 */
} digital_vol_open_parm;


/*----------------------------------------------------------------------------*/
/**@brief    打开数字音量处理
   @param    priv 私有指针
   @param    parm 始化参数，详见结构体digital_vol_open_parm
   @return   句柄
*/
/*----------------------------------------------------------------------------*/
void *user_audio_digital_volume_open(void *priv, void *parm);

/*----------------------------------------------------------------------------*/
/**@brief    关闭数字音量处理
   @param    priv 句柄
   @return   0:成功  -1：失败
*/
/*----------------------------------------------------------------------------*/
int user_audio_digital_volume_close(void *priv);

/*----------------------------------------------------------------------------*/
/**@brief    数字音量运行
   @param    priv 句柄
   @param    data 输入数据
   @param    len 输入数据长度
   @param    sample_rate 采样率
   @return   0:成功  -1：失败
   @note     数字音量调节, 调整输入数据的幅值
*/
/*----------------------------------------------------------------------------*/
int user_audio_digital_volume_run(void *priv, void *data, u32 len, u32 sample_rate);

/*----------------------------------------------------------------------------*/
/**@brief    获取当前数字音量大小
   @param    priv 句柄
   @return   返回音量大小
*/
/*----------------------------------------------------------------------------*/
u8 user_audio_digital_volume_get(void *priv);

/*----------------------------------------------------------------------------*/
/**@brief    设置当前数字音量大小,是否淡入淡出
   @param    priv 句柄
   @param    vol 音量大小
   @param    fade_en 淡入淡出使能
   @return   0:成功  -1：失败
*/
/*----------------------------------------------------------------------------*/
int user_audio_digital_volume_set(void *priv, u8 vol, u8 fade_en);

/*----------------------------------------------------------------------------*/
/**@brief    重置清零淡入淡出
   @param    priv 句柄
   @return   0:成功  -1：失败
*/
/*----------------------------------------------------------------------------*/
int user_audio_digital_volume_reset_fade(void *priv);

/*----------------------------------------------------------------------------*/
/**@brief    设置自定义音量表
   @param    priv 句柄
   @param    user_vol_tab 自定义音量表,自定义表长user_vol_max+1
   @param    user_vol_max 音量级数
*/
/*----------------------------------------------------------------------------*/
void user_audio_digital_set_volume_tab(void *priv, u16 *user_vol_tab, u8 user_vol_max);
#endif
