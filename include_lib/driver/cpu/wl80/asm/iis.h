#ifndef __IIS_H__
#define __IIS_H__

#include "generic/typedef.h"

/**
 * \name IIS使用IO组
 * \{
 */
#define IIS_PORTA          0      /*!< 使用IIS_PORTA组IO   */
#define IIS_PORTC          1      /*!< 使用IIS_PORTC组IO   */
#define IIS_PORTG          2      /*!< 使用IIS_PORTG组IO   */
/* \} name */

/**
 * \name 可使用IIS模块数量
 * \{
 */
#define	MAX_IIS_NUM			2    /*!< 可使用IIS的模块数量   */
/* \} name */

/// \cond DO_NOT_DOCUMENT
struct iis_platform_data {
    u8 channel_in;             /*!< 输入通道 BIT(X) */
    u8 channel_out;            /*!< 输出通道 BIT(X) */
    u8 port_sel;               /*!< IO组选择 */
    u8 data_width;             /*!< BIT(X)为通道X使用24bit模式 */
    u8 mclk_output;            /*!< 1:输出mclk 0:不输出mclk */
    u8 slave_mode;             /*!< 1:从机模式 0:主机模式 */
    u8 update_edge;            /*!< 1:SCLK上升沿更新数据，下降沿采样数据 0:SCLK下降沿更新数据，上升沿采样数据 */
    u8 f32e;                   /*!< 主机模式下，每帧数据的SCLK个数    1:32个SCLK    0:64个SCLK */
    u8 keep_alive;             /*!< 是否保持硬件模块一直不关闭 */
    u8 sel;                    /*!< 当输入单通道数据时选择获取的是IIS模块的左通道数据还是右通道数据   0:左通道   1:右通道 */
    u16 dump_points_num;       /*!< 丢弃刚打开iis硬件时的数据点数 */
    u16 sr_points;             /*!< 多少个采样点进一次中断，一般不建议修改 */
};
/// \endcond

/**
 * @brief iis打开通道
 *
 * @param channel 通道 BIT(x)
 * @param index 0：iis0 1：iis1
 */
void iis_channel_on(u8 channel, u8 index);

/**
 * @brief iis关闭通道
 *
 * @param channel 通道 BIT(x)
 * @param index 0：iis0 1：iis1
 */
void iis_channel_off(u8 channel, u8 index);

/**
 * @brief 打开iis
 *
 * @param iis_platform_data *pd iis配置参数结构体参数指针
 * @param index 0：iis0 1：iis1
 */
int iis_open(struct iis_platform_data *pd, u8 index);

/**
 * @brief 关闭iis
 *
 * @param index 0：iis0 1：iis1
 */
void iis_close(u8 index);

/**
 * @brief iis采样率设置
 *
 * @param sample_rate 采样率
 * @param index 0：iis0 1：iis1
 */
int iis_set_sample_rate(int sample_rate, u8 index);

/**
 * @brief iis设置输入通道数据的中断回调
 *
 * @param priv 传入指针
 * @param cb 回调函数
 * @param index 0：iis0 1：iis1
 */
void iis_set_enc_data_handler(void *priv, void (*cb)(void *, u8 *data, int len, u8), u8 index);

/**
 * @brief iis设置输出通道数据的中断回调
 *
 * @param priv 传入指针
 * @param cb 回调函数
 * @param index 0：iis0 1：iis1
 */
void iis_set_dec_data_handler(void *priv, void (*cb)(void *, u8 *data, int len, u8), u8 index);

/**
 * @brief iis中断函数
 *
 * @param index 0：iis0 1：iis1
 */
void iis_irq_handler(u8 index);

#endif


