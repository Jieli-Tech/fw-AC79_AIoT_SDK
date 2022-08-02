#ifndef __DYNAMIC_EQ_H__
#define __DYNAMIC_EQ_H__

#include "eq/DynamicEQ_api.h"
#include "eq/DynamicEQ_Detection_api.h"
#include "eq/audio_eq.h"
#include "drc/drc_api.h"
#include "media/convert_data.h"

#define RUN_NORMAL  0
#define RUN_BYPASS  1

struct dynamic_eq_detection {
    void *workbuf;                      //算法运行buf
    int *out_buf;                       //模块运行输出的buf 32bit位宽
    int out_len;                        //输出buf的长度
    int len;                            //输出buf的长度
    DynamicEQDetectionParam parm[2];    //算法相关配置参数
    struct audio_eq *eq[2];
    u32 sample_rate;                    //采样率
    u8 nsection;                        //eq段数
    u8 channel;                         //通道数
    u8 status;                          //内部运行状态机
    u8 update;                          //设置参数更新标志
    u8 in_32bit;
    s16 *in_buf;
    u16 in_len;
};

/*
*********************************************************************
*            dynamic_eq_detection_open
* Description: 动态eq检测模块打开
* Arguments  :*parm 检测模块相关参数(中心截止频率)、若有2段，则parm[0], parm[1],参数连续存方
*             nesection:动态eq检测模块支持的段数
*             channel:输入数据通道数
*             sample_rate:输入数据采样率
* Return	 : 模块句柄.
* Note(s)    : None.
*********************************************************************
*/
struct dynamic_eq_detection *dynamic_eq_detection_open(DynamicEQDetectionParam *parm, u8 nsection, u8 channel, u32 sample_rate);

/*
*********************************************************************
*            dynamic_eq_detection_run
* Description: 动态eq检测模块数据处理,不改变源数据
* Arguments  :*hdl:模块句柄
*             data:输入数据地址，16bit位宽
*             len:输入数据长度，byte
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int dynamic_eq_detection_run(struct dynamic_eq_detection *hdl, short *data, int len);

/*
*********************************************************************
*            get_dynamic_eq_detection_parm
* Description: 获取动态eq检测模块的结果
* Arguments  :*hdl:模块句柄
*             **out:检测模块的结果输出地址,32bit位宽
*             *out_len:检测模块结果数据长度byte
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int get_dynamic_eq_detection_parm(struct dynamic_eq_detection *hdl, int **out, int *out_len);

/*
*********************************************************************
*            dynamic_eq_detection_bypass
* Description: 动态eq检测模块设置直通、正常处理
* Arguments  :*hdl:模块句柄
*             bypass:设置直通(RUN_BYPASS)、正常处理(RUN_NORMAL)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void dynamic_eq_detection_bypass(struct dynamic_eq_detection *hdl, u8 bypass);

/*
*********************************************************************
*            dynamic_eq_detection_update
* Description: 动态eq检测模块设置直通、正常处理
* Arguments  :*hdl:模块句柄
*             bypass:设置直通(RUN_BYPASS)、正常处理(RUN_NORMAL)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void dynamic_eq_detection_update(struct dynamic_eq_detection *hdl, DynamicEQDetectionParam *param);

/*
*********************************************************************
*            dynamic_eq_detection_close
* Description: 动态eq检测模块关闭
* Arguments  :*hdl:模块句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void dynamic_eq_detection_close(struct dynamic_eq_detection *hdl);


struct dynamic_eq {
    void *workbuf;                      //算法运行buf
    DynamicEQEffectParam *effectParam;
    DynamicEQParam parm;                //算法相关配置参数
    // u32 sample_rate;                    //采样率
    // u8 nsection;                        //eq段数
    // u8 channel;                         //通道数
    u8 status;                          //内部运行状态机
    u8 update;                          //设置参数更新标志
    struct dynamic_eq_detection *priv;  //私有指针
    int (*get_detect_parm)(struct dynamic_eq_detection *priv, int **out_data, int *out_len);//该回调，由dynamic_eq_detection 模块实现
    void *det_hdl;
};

/*
*********************************************************************
*            dynamic_eq_open
* Description: 动态eq打开
* Arguments  :*parm 检测模块相关参数、若有2段，则parm[0], parm[1],参数连续存方
*             nesection:动态eq检测模块支持的段数
*             channel:输入数据通道数
*             sample_rate:输入数据采样率
* Return	 : 模块句柄.
* Note(s)    : None.
*********************************************************************
*/
// struct dynamic_eq *dynamic_eq_open(DynamicEQParam *parm, u8 nsection, u8 channel,u32 sample_rate);
struct dynamic_eq *dynamic_eq_open(DynamicEQEffectParam *effectParam, DynamicEQParam *parm);//, u8 nsection, u8 channel, u32 sample_rate)

/*
*********************************************************************
*            dynamic_eq_set_detection_callback
* Description: 动态eq 设置检测模块的结果回调
* Arguments  :*hdl:模块句柄
*             priv:私有指针
*             get_detect_parm:检测模块的输出回调
* Return	 : 模块句柄.
* Note(s)    : None.
*********************************************************************
*/
void dynamic_eq_set_detection_callback(struct dynamic_eq *hdl, void *priv,  int (*get_detect_parm)(struct dynamic_eq_detection *priv, int **out_data, int *out_len));

/*
*********************************************************************
*            dynamic_eq_run
* Description: 动态eq模块数据处理
* Arguments  :*hdl:模块句柄
*             data:输入数据地址，32bit位宽
*             len:输入数据长度，byte
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
int dynamic_eq_run(struct dynamic_eq *hdl, int *data, int len);

/*
*********************************************************************
*            dynamic_eq_bypass
* Description: 动态eq模块设置直通、正常处理
* Arguments  :*hdl:模块句柄
*             bypass:设置直通(RUN_BYPASS)、正常处理(RUN_NORMAL)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void dynamic_eq_bypass(struct dynamic_eq *hdl, u8 bypass);

/*
*********************************************************************
*            dynamic_eq_bypass
* Description: 动态eq模块设置直通、正常处理
* Arguments  :*hdl:模块句柄
*             bypass:设置直通(RUN_BYPASS)、正常处理(RUN_NORMAL)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
// void dynamic_eq_update(struct dynamic_eq *hdl, DynamicEQParam *param);
void dynamic_eq_update(struct dynamic_eq *hdl, DynamicEQEffectParam *effectParam, DynamicEQParam *param);

/*
*********************************************************************
*            dynamic_eq_close
* Description: 动态eq检块关闭
* Arguments  :*hdl:模块句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void dynamic_eq_close(struct dynamic_eq *hdl);

#endif/*__DYNAMIC_EQ_H__*/
