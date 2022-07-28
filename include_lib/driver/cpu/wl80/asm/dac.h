#ifndef __DAC_H__
#define __DAC_H__

#include "generic/typedef.h"

/// \cond DO_NOT_DOCUMENT
#define DAC_44_1KHZ            0
#define DAC_48KHZ              1
#define DAC_32KHZ              2
#define DAC_22_05KHZ           3
#define DAC_24KHZ              4
#define DAC_16KHZ              5
#define DAC_11_025KHZ          6
#define DAC_12KHZ              7
#define DAC_8KHZ               8
/// \endcond

/**
 * \name 淡入淡出状态
 * \{
 */
#define FADE_DISABLE            0   /*!< 淡入淡出失能 */
#define FADE_VOL_OUT            1   /*!< 淡入淡出模拟音量淡出 */
#define FADE_VOL_IN             2   /*!< 淡入淡出模拟音量淡入 */
#define FADE_VOL_UPDOWN         3   /*!< 淡入淡出模拟音量调节 */
/* \} name */

/// \cond DO_NOT_DOCUMENT
struct dac_platform_data {
    u8 sw_differ;               /*!< 软件差分  0:不使用  1:使用 */
    u8 pa_auto_mute;            /*!< 关闭DAC时是否自动MUTE功放  0:不mute功放 1:MUTE功放 */
    u8 pa_mute_port;            /*!< 功放MUTE IO */
    u8 pa_mute_value;           /*!< MUTE电平值 0:低电平 1:高电平 */
    u8 differ_output;           /*!< 是否使用硬件差分输出模式  0:不使用 1:使用 */
    u8 hw_channel;              /*!< 硬件DAC模拟通道 */
    u8 ch_num;                  /*!< 数字通道，软件输出通道数 */
    u8 fade_enable;				/*!< 模拟音量淡入淡出使能位 */
    u16 mute_delay_ms;			/*!< MUTE延时 */
    u16 fade_delay_ms;			/*!< 模拟音量淡入淡出延时 */
    u16 sr_points;				/*!< 多少个采样点进一次中断，一般不建议修改 */
    u16 vcm_init_delay_ms;		/*!< VCM电压初始化后等待稳定的延时ms */
    u32 poweron_delay;			/*!< 低功耗睡眠时恢复DAC模块的延迟us */
};
/// \endcond

/**
 * @brief dac_early_init, dac初始化
 *
 * @param trim_en 0:初始化不trim dac  1:初始化时trim dac
 * @param hw_channel 硬件dac通道配置BIT(x)
 * @param dly_msecs 延时时间
*/
void dac_early_init(u8 trim_en, u8 hw_channel, u32 dly_msecs);

/**
 * @brief 打开dac模块
 *
 * @param struct dac_platform_data *pd dac配置参数结构体参数指针
*/
int dac_open(const struct dac_platform_data *pd);

/**
 * @brief dac模块使能
*/
int dac_on(void);

/**
 * @brief dac模块失能
*/
int dac_off(void);

/**
 * @brief 关闭dac模块
*/
int dac_close(void);

/**
 * @brief 低功耗休眠起来后恢复dac模块
*/
void dac_power_on(void);

/**
 * @brief 低功耗进入前挂起dac模块
*/
void dac_power_off(void);

/**
 * @brief dac数字音量设置
 *
 * @param volume 音量0-16384
*/
void dac_set_volume(s16 volume);

/**
 * @brief dac采样率设置
 *
 * @param volume 音量
*/
int dac_set_sample_rate(u32 sample_rate);

/**
 * @brief dac中断函数
*/
void dac_irq_handler(void);

/**
 * @brief dac中断的回调函数设置
 *
 * @param priv 传入指针
 * @param handler 传入的回调函数
*/
void dac_set_data_handler(void *priv, void (*handler)(void *, u8 *data, int len));

/**
 * @brief dac模拟增益设置
 *
 * @param gain 模拟增益0-31
 * @param fade_opt 淡入淡出状态位
*/
void dac_set_gain(u8 gain, u8 fade_opt);

/**
 * @brief dac直通模拟增益设置
 *
 * @param gain 模拟增益0-31
*/
void dac_direct_set_gain(u8 gain);

/**
 * @brief dac模拟增益淡入淡出设置
 *
 * @param enable 0：关闭 1：使能
*/
void dac_fade_inout_set(u8 enable);

/**
 * @brief 读取DAC DMA BUF的单通道数据
 *
 * @param read_pos 读指针
 * @param points_offset 偏移量
 * @param data 数据指针
 * @param len 数据长度
 *
 * @return 数据长度
*/
int audio_dac_read_single_channel(u16 *read_pos, s16 points_offset, void *data, int len);

#endif

