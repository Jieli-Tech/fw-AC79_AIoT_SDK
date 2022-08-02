#ifndef __LADC_H__
#define __LADC_H__

#include "generic/typedef.h"

///  \cond DO_NOT_DOCUMENT
/* ADC采样率设置 */
#define ADC_SAMPRATE_44_1KHZ           0
#define ADC_SAMPRATE_48KHZ             1
#define ADC_SAMPRATE_32KHZ             2
#define ADC_SAMPRATE_22_05KHZ          3
#define ADC_SAMPRATE_24KHZ             4
#define ADC_SAMPRATE_16KHZ             5
#define ADC_SAMPRATE_11_025KHZ         6
#define ADC_SAMPRATE_12KHZ             7
#define ADC_SAMPRATE_8KHZ              8
/// \endcond


/**
 * \name MIC通道选择
 * \{
 */
#define LADC_CH_MIC0_P  				BIT(0)			  /*!< MIC通道0正极  PA2 */
#define LADC_CH_MIC0_N  				BIT(1)			  /*!< MIC通道0负极  PA1 */
#define LADC_CH_MIC0_P_N  				(BIT(0) | BIT(1)) /*!< MIC通道0正负极  PA1 PA2 */
#define LADC_CH_MIC1_P  				BIT(2)			  /*!< MIC通道1正极  PH8 */
#define LADC_CH_MIC1_N  				BIT(3)			  /*!< MIC通道1负极  PH9 */
#define LADC_CH_MIC1_P_N  				(BIT(2) | BIT(3)) /*!< MIC通道1正负极  PH8 PH9 */
#define LADC_CH_MIC2_P  				BIT(4)			  /*!< MIC通道2正极  PA3 */
#define LADC_CH_MIC2_N  				BIT(5)			  /*!< MIC通道2负极  PA4 */
#define LADC_CH_MIC2_P_N  				(BIT(4) | BIT(5)) /*!< MIC通道2正负极  PA4 PA3 */
#define LADC_CH_MIC3_P  				BIT(6)			  /*!< MIC通道3正极  PH6 */
#define LADC_CH_MIC3_N  				BIT(7)			  /*!< MIC通道3正极  PH5 */
#define LADC_CH_MIC3_P_N  				(BIT(6) | BIT(7)) /*!< MIC通道3正负极  PH5 PH6 */
/* \} name */

/**
 * \name LINEIN通道选择
 * \{
 */
#define LADC_CH_AUX0  					BIT(0)	  /*!< AUX通道0 PA0 */
#define LADC_CH_AUX1  					BIT(1)	  /*!< AUX通道1 PH7 */
#define LADC_CH_AUX2  					BIT(2)	  /*!< AUX通道2 PA5 */
#define LADC_CH_AUX3  					BIT(3)	  /*!< AUX通道3 PH4 */
/* \} name */

/// \cond DO_NOT_DOCUMENT
struct adc_platform_data {
    u8 mic_channel;         /*!< 用到的mic通道 */
    u8 linein_channel;      /*!< 用到的linein通道 */
    u8 mic_ch_num;          /*!< 用到的mic通道数 */
    u8 linein_ch_num;       /*!< 用到的linein通道数 */
    u8 fake_channel;        /*!< 未使用 */
    u8 fake_ch_num;         /*!< 未使用 */
    u8 isel;                /*!< AD电流档，一般没有特殊要求不建议改动 */
    u8 all_channel_open;    /*!< 所有AD通道打开，使用多路AD时建议打开 */
    u16 sr_points;          /*!< 多少个采样点进一次中断，一般不建议修改 */
    u16 dump_num;           /*!< 打开adc时丢掉点数 */
};
/// \endcond

/**
 * @brief ADC多路复用时的模拟增益设置
 *
 * @param source 采样源 "mic" "linein"
 * @param channel_bit_map 通道选择
 * @param gain 模拟增益  mic:0-31 linein:0-15
 */
void adc_multiplex_set_gain(const char *source, u8 channel_bit_map, u8 gain);

/**
 * @brief ADC多路复用时打开通道
 *
 * @param source 采样源 "mic" "linein"
 * @param channel_bit_map 通道选择
 */
void adc_multiplex_open(const char *source, u8 channel_bit_map);

/**
 * @brief 打开ADC
 *
 * @param source 采样源 "mic" "linein"
 * @param adc_platform_data *pd adc配置参数结构体指针
 */
int adc_open(const char *source, struct adc_platform_data *pd);

/**
 * @brief ADC采样率设置
 *
 * @param sample_rate  采样率
 */
int adc_set_sample_rate(int sample_rate);

/**
 * @brief ADC模块使能
 *
 * @param source 采样源 "mic" "linein"
 */
int adc_start(const char *source);

/**
 * @brief ADC中断函数
 */
void adc_irq_handler(void);

/**
 * @brief ADC模块失能
 */
int adc_stop(void);

/**
 * @brief 关闭ADC
 */
int adc_close(void);

/**
 * @brief 设置ADC中断的回调函数
 *
 * @param priv 传入指针
 * @param handler 回调函数
*/
void adc_set_data_handler(void *priv, void (*handler)(void *, u8 *data, int len));

/**
 * @brief ADC增益设置
 *
 * @param source 采样源 "mic" "linein"
 * @param gain 模拟增益  mic:0-31 linein:0-15
 */
int adc_set_gain(const char *source, u8 gain);

/**
 * @brief LINEIN模拟直通到DACL/R
 *
 * @param enable_dacl 直通使能DACL位
 * @param enable_dacr 直通使能DACR位
 */
void linein_to_fdac(u8 enable_dacl, u8 enable_dacr);

/**
 * @brief MIC模拟直通到DACL/R
 *
 * @param channel_bit_map 通道选择
 * @param enable_dacl 直通使能DACL位
 * @param enable_dacr 直通使能DACR位
 */
void mic_to_fdac(u8 channel_bit_map, u8 enable_dacl, u8 enable_dacr);

/**
 * @brief vcm电压初始化
 *
 * @param delay_ms 毫秒级稳定延时
 */
void vcm_early_init(u32 delay_ms);

/**
 * @brief linein模拟直通DAC的模拟增益控制开关
 *
 * @param enable 1：使能 0：关闭
 */
void linein_to_fdac_high_gain(u8 enable);

#endif


