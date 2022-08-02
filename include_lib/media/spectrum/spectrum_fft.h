#ifndef SPECTRUM_FFT_API_H
#define SPECTRUM_FFT_API_H

#include "spectrum/SpectrumShow_api.h"

//该模块 mips 消耗5M
//RAM 单声道时：4.4Kbyte
//RAM 双声道时：5.4Kbyte

/**
 * @brief 频谱运算参数结构体
 */
typedef struct _spectrum_fft_open_parm {
    unsigned int sr;                /*!< 采样率 */
    unsigned char channel;          /*!< 通道数 */
    unsigned char mode;             /*!< 模式，双声道起作用，0计算的是第一声道的频谱值，1计算的是第二声道频谱值，2为第一声道与第二声道相加除2的频谱值 */
    unsigned short run_points;      /*!< 运行一次采样点数 */
    float attackFactor;             /*!< 下降因子[0,1) */
    float releaseFactor;            /*!< 上升因子[0,1) */
} spectrum_fft_open_parm;


/*----------------------------------------------------------------------------*/
/**@brief    打开频谱运算
   @param    priv 私有指针
   @param    parm 始化参数，详见结构体spectrum_fft_open_parm
   @return   句柄
*/
/*----------------------------------------------------------------------------*/
void *audio_spectrum_fft_open(void *priv, void *parm);

/*----------------------------------------------------------------------------*/
/**@brief    关闭频谱计算处理
   @param    priv 句柄
   @return   0:成功  -1：失败
*/
/*----------------------------------------------------------------------------*/
int audio_spectrum_fft_close(void *priv);

/*----------------------------------------------------------------------------*/
/**@brief    频谱计算同步处理，每次run都会把输入buf消耗完，才会往下走
   @param    priv 句柄
   @param    data 输入数据
   @param    len 输入数据长度
   @param    sample_rate 采样率
   @return   len
   @note     频谱计算处理，只获取输入的数据，不改变输入的数据
*/
/*----------------------------------------------------------------------------*/
int audio_spectrum_fft_run(void *priv, short *data, int len, int sample_rate);

/*----------------------------------------------------------------------------*/
/**@brief    频谱计算运行过程做开关处理
   @param    priv 句柄
   @param    en 0 关闭频响运算  1 打开频响运算
*/
/*----------------------------------------------------------------------------*/
void audio_spectrum_fft_switch(void *priv, unsigned char en);

/*----------------------------------------------------------------------------*/
/**@brief    获取频谱个数
   @param    priv 句柄
   @return   返回频谱的个数
*/
/*----------------------------------------------------------------------------*/
int audio_spectrum_fft_get_num(void *priv);

/*----------------------------------------------------------------------------*/
/**@brief    获取频谱值
   @param    priv 句柄
   @return   返回存储频谱值的地址
*/
/*----------------------------------------------------------------------------*/
short *audio_spectrum_fft_get_val(void *priv);

#endif
