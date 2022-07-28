#ifndef __REVERB_DEAL
#define __REVERB_DEAL

#include "typedef.h"


struct __EF_REVERB_PARM_ {
    unsigned int deepval;	//调节混响深度，影响的是pre-delay ：0-4096，4096代表max_ms
    unsigned int decayval;	// 衰减系数: 调节范围0-4096
    unsigned int filtsize;	// 0-4096, 如果 要回声效果的话，将它置0
    unsigned int wetgain;	//湿声增益： 0到4096
    unsigned int drygain;	//干声增益： 0到4096
    unsigned int sr;		// 配置输入的采样率
    unsigned int max_ms;	//所需要的最大延时
    unsigned int centerfreq_bandQ;	//留声机音效，不需要的话，请置0
};

/*
1.EFFECT_PITCH_SHIFT  移频，变调不变速，init_parm.shiftv调节有效，init_parm.formant_shift调节无效
2.EFFECT_VOICECHANGE_KIN0  变声，可以调节不同的 init_parm.shiftv  跟 init_parm.formant_shift ，调出更多的声音效果
3.EFFECT_VOICECHANGE_KIN1  变声，同EFFECT_VOICECHANGE_KIN0类似的，但是2者由于运算的不同，会有区别。
4.EFFECT_ROBORT 机器音效果，类似 喜马拉雅那样的
5.EFFECT_AUTOTUNE  电音效果
*/
enum {
    EFFECT_PITCH_SHIFT       = 0x00,
    EFFECT_VOICECHANGE_KIN0,
    EFFECT_VOICECHANGE_KIN1,
    EFFECT_ROBORT,
    EFFECT_AUTOTUNE,
};


struct __PITCH_SHIFT_PARM_ {
    u32 sr;                          //input  audio samplerate
    u32 shiftv;                      //pitch rate:  <8192(pitch up), >8192(pitch down)
    u32 effect_v;
    s32 formant_shift;
};


struct __HOWLING_PARM_ {
    int threshold;  //初始化阈值
    int depth;  //陷波器深度
    int bandwidth;//陷波器带宽
    int attack_time; //门限噪声启动时间
    int release_time;//门限噪声释放时间
    int noise_threshold;//门限噪声阈值
    int low_th_gain; //低于门限噪声阈值增益
    int sample_rate;
    int channel;
};


void echo_mic_gain_change(int gain);
void echo_deal_set_reverb_param(void *reverb);
void echo_deal_set_pitch_param(void *pitch);
void echo_deal_pitch_en(void);		//打开变调功能
void echo_deal_pitch_unable(void);	//关闭变调功能
int echo_reverb_init(u32 resample_sr, u32 sample_rate, u8 enc_channel_bit_map, u8 enc_volume, u8 dec_volume, void *reverb, void *pitch, void *howling, void (*cb)(void *, u32));
int echo_reverb_uninit(void);
void echo_deal_start(void);
void echo_deal_pause(void);

#endif
