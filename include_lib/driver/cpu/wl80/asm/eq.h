#ifndef _HW_EQ_H_
#define _HW_EQ_H_

#include "generic/typedef.h"

//EQ 设置
#define EQ_CHANNEL_MAX      2
#define EQ_SECTION_MAX      20
#define EQ_SR_IDX_MAX       9


struct eq_s_attr {
    u8 channel;
    u8 nsection;
    u8 soft_sec_set : 1;
    u8 soft_sec_num : 3;
    u8 out_32bit : 1;
    u8 Reserved : 3;
    float gain;
    u32 sr;
    void *L_coeff;
    void *R_coeff;
    int (*set_sr_cb)(int sr, void **L_coeff, void **R_coeff);
    int (*eq_out_cb)(void *priv, void *buf, u32 byte_len, u32 sample_rate, u8 channel);
};


void *eq_open(u8 stream_ch);

void eq_close(void *_eq);

int audio_do_eq(void *_eq, void *input, void *output, int len);

int eq_get_s_attr(void *_eq, struct eq_s_attr *attr);
int eq_set_s_attr(void *_eq, struct eq_s_attr *attr);
int eq_set_sr(void *_eq, int sr);

void eq_set_irq_handler(void *_eq,
                        void *priv,
                        int (*handler)(void *, void *, int));

void eq_get_AllpassCoeff(void *Coeff);
#endif
