#ifndef FILTPARM_CAL_api_h__
#define FILTPARM_CAL_api_h__

enum {
    TYPE_LOWPASS = 0x01,
    TYPE_HIGHPASS,
    TYPE_BANDPASS,
    TYPE_NORTCH,
    TYPE_LOWPASS_A,
    TYPE_HIGHPASS_A,
    TYPE_NULL
};

typedef struct __BFILT_FUNC_API_ {
    int (*needbuf)();           //开了3个的空间
    int (*open)(void *ptr);
    int (*init)(void *ptr, int freq, int bw, int filt_type, int samplerate, int filtNUM); //滤波器中心频率，带宽，滤波器类型,第几个滤波
    int (*cal_coef)(void *ptr, int *filtTab, int gain, int filtNUM);   //传入增益，计算到的滤波器表
    void (*run)(void *ptr, short *data, int len);
} BFILT_FUNC_API;

extern BFILT_FUNC_API   *get_bfiltTAB_func_api();

#endif
