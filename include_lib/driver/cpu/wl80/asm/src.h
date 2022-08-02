#ifndef _SRC_H_
#define _SRC_H_

#include "typedef.h"
#include "generic/circular_buf.h"

/// \cond DO_NOT_DOCUMENT
/**
 * @brief src配置参数结构体
 */
typedef struct {
    u8 nchannel;        /*!< 一次转换的通道个数，取舍范围(1 ~ 8)，最大支持8个通道 */
    u8 reserver[3];     /*!< 未使用 */
    u16 in_rate;        /*!< 输入采样率 */
    u16 out_rate;       /*!< 输出采样率 */
    u16 in_chinc;       /*!< 输入方向,多通道转换时，每通道数据的地址增量 */
    u16 in_spinc;       /*!< 输入方向,同一通道后一数据相对前一数据的地址增量 */
    u16 out_chinc;      /*!< 输出方向,多通道转换时，每通道数据的地址增量 */
    u16 out_spinc;      /*!< 输出方向,同一通道后一数据相对前一数据的地址增量 */
    s16 *in_addr;		/*!< 输入数据地址指针 */
    u32 in_len;			/*!< 输入数据长度 */
    s16 *out_addr;		/*!< 输出数据地址指针 */
    u32 out_len;		/*!< 输入数据长度 */
    void (*output_cbk)(void *priv, u8 *, u16, u8); /*!< 一次转换完成后，输出中断会调用此函数用于接收输出数据，数据量大小由outbuf_len决定 */
    void *priv;			/*!< 指针 */
} src_param_t;


/**
 * @brief src滤波器参数结构体
 */
typedef struct {
    u16 fltb_buf[24 * 2]; /*!< 滤波器buf */
    u32 phase;			  /*!< 相位 */
    u8 fltb_offset;       /*!< 滤波器偏移量 */
} src_fltb_t;


void src_disable(void);
void src_enable(src_param_t *arg);
u32 src_run(u8 *buf, u16 len);
void src_kick_start(u8 start_flag);
/// \endcond

/**
 * @brief   src变采样函数
 * @param   src_param_t：src配置参数结构体
 * @param   src_fltb_t：src滤波器参数结构体
 * @return
 */
void src_resample(src_param_t *src, src_fltb_t *flt);

/**
 * @brief    src变采样函数使用cbuf存取数据
 * @param    cbuffer_t *in：输入数据cbuf
 * @param    cbuffer_t *out：输出数据cbuf
 * @param    insr：输入数据采样率
 * @param    insr：outsr
 * @param    ch：src转换用到的通道数
 * @param   src_fltb_t：src滤波器参数结构体
 */
void src_resample_cbuf(cbuffer_t *in, cbuffer_t *out, int insr, int outsr, u8 ch, src_fltb_t *flt);


/// \cond DO_NOT_DOCUMENT
#if 0
struct audio_resample_context {
    u32 CON;
    s16 fltb_addr[24 * 2];
};

struct audio_resample_data {
    void *buffer;
    int count;
    int offset;
    u8 ch_num;
    u16 sample_rate;
};

/*初始化一个变采样context*/
struct audio_resample_context *audio_resample_open(void);

/*音频数据变采样*/
int audio_resample(struct audio_resample_context *rtx,
                   struct audio_resample_data *src,
                   struct audio_resample_data *dst);

/*释放变采样context*/
void audio_resample_close(struct audio_resample_context *rtx);
#endif
/// \endcond

#endif

