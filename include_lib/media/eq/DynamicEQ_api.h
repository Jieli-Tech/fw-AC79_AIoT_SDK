#ifndef DYNAMICEQ_API_H
#define DYNAMICEQ_API_H

#include "asm/hw_eq.h"

#ifdef WIN32

#define AT_DYNAMICEQ(x)
#define AT_DYNAMICEQ_CODE
#define AT_DYNAMICEQ_CONST
#define AT_DYNAMICEQ_SPARSE_CODE
#define AT_DYNAMICEQ_SPARSE_CONST

#else

#define AT_DYNAMICEQ(x)				__attribute((section(#x)))
#define AT_DYNAMICEQ_CODE			AT_DYNAMICEQ(.dynamic_eq_code)
#define AT_DYNAMICEQ_CONST			AT_DYNAMICEQ(.dynamic_eq_const)
#define AT_DYNAMICEQ_SPARSE_CODE	AT_DYNAMICEQ(.dynamic_eq_sparse_code)
#define AT_DYNAMICEQ_SPARSE_CONST	AT_DYNAMICEQ(.dynamic_eq_sparse_const)

#endif

typedef enum {
    DYNAMIC_EQ_IIR_TYPE_BAND_PASS = EQ_IIR_TYPE_BAND_PASS,
    //DYNAMIC_EQ_IIR_TYPE_HIGH_SHELF,
//	DYNAMIC_EQ_IIR_TYPE_LOW_SHELF
} DYNAMIC_EQ_IIR_TYPE;

/* enum { */
// PEAK = 0,
// RMS
// };

// enum {
// PERPOINT = 0,
// TWOPOINT
// };

enum {
    SHORT = 0,
    INT
};

typedef struct _DynamicEQEffectParam {
    int fc;                     //中心频率
    float Q;                    //Q值
    float gain;                 //衰减最大增益或者增强最大增益，当gain < 0时衰减模式，当gain > 0时增强模式
    int type;                   //类型  只支持EQ_IIR_TYPE_BAND_PASS
    int attackTime;             //启动时间
    int releaseTime;            //释放时间
    int rmsTime;                //rmsTime   algorithm为RMS时有效
    float threshold;            //启动阈值  衰减模式为大于阈值启动  增强模式小于阈值启动
    float ratio;                //比率  衰减模式ratio > 1[1,30]   增强模式  ratio < 1 [0.1,1]
    float noisegate_threshold;  //噪声门限阈值，低于噪声门限阈值不启动
    float fixGain;              //在中心频率fc处的固定增益
    unsigned char algorithm;    //算法 PEAK或者RMS
} DynamicEQEffectParam;

typedef struct _DynamicEQParam {
    int nSection;               //段数
    int channel;                //通道
    int SampleRate;             //采样率
    int detect_mode;            //检测模式  PERPOINT或者TWOPOINT 对应工具的Precision+与Precision
    int DetectdataInc;          //检测数据相同通道下一点的步进  例如左右左右 为2
    int IndataInc;              //输入数据相同通道下一点的步进
    int OutdataInc;             //输出数据相同通道下一点的步进
} DynamicEQParam;

int getDynamicEQBuf(DynamicEQEffectParam *effectParam, DynamicEQParam *param); //bufsize 与nSection rmsTime algorithm channel SampleRate 有关
void DynamicEQInit(void *WorkBuf, DynamicEQEffectParam *effectParam, DynamicEQParam *param);
void DynamicEQUpdate(void *WorkBuf, DynamicEQEffectParam *effectParam, DynamicEQParam *param);
int DynamicEQRun(void *WorkBuf, int *detectdata, int *indata, int *outdata, int per_channel_npoint);

#endif // !DYNAMICEQ_API_H

