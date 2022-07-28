#ifndef _AUDIO_BFILT_API_H_
#define _AUDIO_BFILT_API_H_

#include "generic/typedef.h"
#include "application/filtparm_api.h"

typedef struct _bfilt_open_parm {
    u32 sr; //输入音频采样率
} bfilt_open_parm;

typedef struct _bfilt_update_parm {
    int freq;//滤波器中心频率
    int bandwidth; //带宽
    s16 iir_type;//滤波器类型
    s16 filt_num;//第几个滤波

    int *filt_tar_tab;//滤波器系数表输出目标地址
    int gain;//增益
} audio_bfilt_update_parm;

typedef struct _bfilt_hdl {
    BFILT_FUNC_API *ops;
    void *work_buf;
    u8 bfilt_en: 1;
    u8 update: 1;
    bfilt_open_parm parm;
} audio_bfilt_hdl;

audio_bfilt_hdl *audio_bfilt_open(bfilt_open_parm *parm);
void audio_bfilt_close(audio_bfilt_hdl *hdl);
void audio_bfilt_init_cal(audio_bfilt_hdl *hdl, audio_bfilt_update_parm *parm);
void audio_bfilt_cal_coeff(audio_bfilt_hdl *hdl, audio_bfilt_update_parm *parm);

#endif

