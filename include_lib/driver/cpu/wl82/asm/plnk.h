#ifndef __PLNK_H__
#define __PLNK_H__

#include "generic/typedef.h"

/**
 * \name 可使用PDMLINK模块数量
 * \{
 */
#define		MAX_PLNK_NUM		2	/*!< 可使用PDMLINK的模块数量   */
/* \} name */

/**
 * \name 输入通道选择
 * \{
 */
#define PLNK_MIC_MASK					(BIT(0) | BIT(1))
#define PLNK_CH_MIC_L					BIT(0)	/*!< 两个MIC共用DAT0也使用该宏 */
#define PLNK_CH_MIC_R					BIT(1)  /*!< 两个MIC共用DAT1也使用该宏 */
#define PLNK_CH_MIC_DOUBLE				(BIT(0) | BIT(1))
/* \} name */

/// \cond DO_NOT_DOCUMENT
struct plnk_platform_data {
    void (*port_remap_cb)(void);		/*!< IO重映射设置函数 */
    void (*port_unremap_cb)(void);		/*!< IO重映射解除函数 */
    u8 hw_channel;						/*!< 输入通道选择 */
    u8 clk_out;							/*!< 是否输出SCLK */
    u8 high_gain;						/*!< 0:-6db增益    1:0db增益 */
    u8 sample_edge;						/*!< 0:DAT0下降沿采样数据  1::DAT0上升沿采样数据 */
    u16 sr_points;						/*!< 每次中断起来时的采样点数 */
    u8 share_data_io;					/*!< 双数字麦是否共用同一个数据输入IO */
    u8 dc_cancelling_filter;			/*!< 去直流滤波器等级0-15 */
    u32 dump_points_num;				/*!< 丢弃刚打开硬件时的数据点数 */
};
/// \endcond

/**
 * @brief 初始化PLNK
 * @param pd plnk配置参数结构体参数指针
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_open(const struct plnk_platform_data *pd, u8 index);

/**
 * @brief 启动PLNK
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_start(u8 index);

/**
 * @brief 停止PLNK
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_stop(u8 index);

/**
 * @brief 关闭PLNK
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_close(u8 index);

/**
 * @brief 设置PLNK采样率
 * @param sample_rate 采样率
 * @param index 0:plnk0  1:plnk1
 *
 * @return 0: 成功
 * @return -1: 失败
 */
int plnk_set_sample_rate(int sample_rate, u8 index);

/**
 * @brief 设置PLNK中断处理函数中的回调函数，用于处理采集到的数据
 * @param priv 回调函数的私有指针
 * @param cb 回调函数
 * @param index 0:plnk0  1:plnk1
 */
void plnk_set_handler_cb(void *priv, void (*cb)(void *, u8 *data, int len), u8 index);

/**
 * @brief PLNK中断处理函数
 * @param index 0:plnk0  1:plnk1
 */
void plnk_irq_handler(u8 index);

#endif

