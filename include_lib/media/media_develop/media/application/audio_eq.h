#ifndef _EQ_API_H_
#define _EQ_API_H_

#include "generic/typedef.h"
#include "asm/hw_eq.h"

#define EQ_CHANNEL_MAX      2

#define EQ_SR_IDX_MAX       9

/*eq type*/
typedef enum {
    EQ_MODE_NORMAL = 0,
    EQ_MODE_ROCK,
    EQ_MODE_POP,
    EQ_MODE_CLASSIC,
    EQ_MODE_JAZZ,
    EQ_MODE_COUNTRY,
    EQ_MODE_CUSTOM,//自定义
    EQ_MODE_MAX,
} EQ_MODE;

/*eq type*/
typedef enum {
    EQ_TYPE_FILE = 0x01,
    EQ_TYPE_ONLINE,
    EQ_TYPE_MODE_TAB,
} EQ_TYPE;

#define audio_eq_filter_info 	eq_coeff_info

typedef int (*audio_eq_filter_cb)(void *eq, int sr, struct audio_eq_filter_info *info);

struct audio_eq_param {
    u32 channels : 2;    //通道数
    u32 online_en : 1;   //是否支持在线调试  1:支持 0：不支持
    u32 mode_en : 1;     //支持默认系数表写1
    u32 remain_en : 1;   //数据输出支持remain,写1
    u32 no_wait : 1;     //是否使能异步eq, 1:使能  0：不使能
    u32 max_nsection : 6;//最大的eq段数,根据使用填写，要小于等于EQ_SECTION_MAX
    u32 nsection : 6;    //实际需要的eq段数，需小于等于max_nsection
    u32 drc_en: 1;       //16bit drc时，使用硬件eq对分频器部分进行加速，目前默认使用32bit drc。
    u32 out_32bit : 1;   //是否支持32bit eq输出，仅在 no_wait 写1时，out_32bit 才能写1
    audio_eq_filter_cb cb;//获取eq系数的回调函数
    u32 eq_name;         //eq名字，在线调试时，用于区分不同eq更新系数 播歌一般写song_eq_mode
    u32 sr;              //采样率，更根据当前数据实际采样率填写

    void *priv;          //私有指针
    int (*output)(void *priv, void *data, u32 len);//异步eq输出回调
    void (*irq_callback)(void *priv);//总段回调函数，数据流激活
};

struct audio_eq_async {
    u16 ptr;
    u16 len;
    u16 buf_len;
    u8 clear;
    u8 out_stu;
    char *buf;
};

struct audio_eq {
    void *eq_ch;                               //硬件eq句柄
    u32 sr;                                    //采样率
    u8 updata;                                 //系数是否需要更新
    u8 check_hw_running;                       //检测到硬件正在运行时不等待其完成1:设置检查  0：不检查
    u8 remain_flag : 1;                        //数据未输出完
    u8 online_en : 1;                          //是否支持在线调试  1:支持 0：不支持
    u8 mode_en : 1;                            //支持默认系数表写1
    u8 remain_en : 1;                          //数据输出支持remain,写1
    u8 start : 1;                              //eq start标识
    u8 reserved : 3;
    u8 max_nsection;                           //eq最大段数
    u32 eq_name;                               //eq标识

#ifdef CONFIG_EQ_SUPPORT_ASYNC
    void *run_buf;
    void *run_out_buf;
    int run_len;
    struct audio_eq_async async;                     //异步eq 输出管理
#endif

    u32 mute_cnt_l;                                  //左声道eq mem清除计数
    u32 mute_cnt_r;                                  //右声道eq mem清除计数
    u32 mute_cnt_max;                                //记录最大点数，超过该点数，清eq mem

    audio_eq_filter_cb cb;                           //系数回调
    void *output_priv;                               //私有指针
    int (*output)(void *priv, void *data, u32 len);  //输出回调

    struct eq_seg_info *eq_seg_tab;                 //运算前系数表
    float *eq_coeff_tab;                            //运算后系数表
    void *entry;                                    //无效
};

/*----------------------------------------------------------------------------*/
/**@brief    eq模块初始化
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_init(void);
void audio_eq_init_new(int eq_section_num);

/*----------------------------------------------------------------------------*/
/**@brief    eq 打开
   @param    *param: eq参数句柄,参数详见结构体struct audio_eq_param
   @return   eq句柄
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_open(struct audio_eq *eq, struct audio_eq_param *param);

/*----------------------------------------------------------------------------*/
/**@brief    异步eq设置输出回调
   @param    eq:句柄
   @param    *output:回调函数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_set_output_handle(struct audio_eq *eq, int (*output)(void *priv, void *data, u32 len), void *output_priv);

/*----------------------------------------------------------------------------*/
/**@brief    同步eq设置输出buf
   @param    eq:句柄
   @param    *output:回调函数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_set_output_buf(struct audio_eq *eq, s16 *buf, u32 len);

/*----------------------------------------------------------------------------*/
/**@brief    eq设置采样率
   @param    eq:句柄
   @param    sr:采样率
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_set_samplerate(struct audio_eq *eq, int sr);

/*----------------------------------------------------------------------------*/
/**@brief    eq设置输入输出通道数
   @param    eq:句柄
   @param    channel:通道数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_set_channel(struct audio_eq *eq, u8 channel);

/*----------------------------------------------------------------------------*/
/**@brief    设置检查eq是否正在运行
   @param    eq:句柄
   @param    check_hw_running: 1:设置检查  0：不检查
   @return
   @note     检测到硬件正在运行时不等待其完成，直接返回, 仅异步EQ有效
*/
/*----------------------------------------------------------------------------*/
int audio_eq_set_check_running(struct audio_eq *eq, u8 check_hw_running);

/*----------------------------------------------------------------------------*/
/**@brief    设置eq信息
   @param    eq:句柄
   @param    channels:设置输入输出通道数
   @param    out_32bit:使能eq输出32bit数据，1：输出32bit数据， 0：输出16bit是数据
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_set_info(struct audio_eq *eq, u8 channels, u8 out_32bit);

/*----------------------------------------------------------------------------*/
/**@brief    设置eq信息
   @param    eq:句柄
   @param    channels:设置入输出通道数
   @param    in_mode:使能eq输入32bit数据，2:float, 1：输入32bit数据， 0：输入16bit是数据
   @param    out_mode:使能eq输出32bit数据，2:float, 1：输出32bit数据， 0：输出16bit是数据
   @param    run_mode:运行模式，0：normal, 1:mono, 2:stero
   @param    data_in_mode:输入数据存放方式 0：块模式  1：序列模式
   @param    data_out_mode:输入数据存放方式 0：块模式  1：序列模式
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_set_info_new(struct audio_eq *eq, u8 channels, u8 in_mode, u8 out_mode, u8 run_mode, u8 data_in_mode, u8 data_out_mode);

/*----------------------------------------------------------------------------*/
/**@brief    eq启动
   @param    eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_start(struct audio_eq *eq);

/*----------------------------------------------------------------------------*/
/**@brief    eq的数据处理
   @param    eq:句柄
   @param    *data:输入数据地址
   @param    len:输入数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_run(struct audio_eq *eq, s16 *data, u32 len);

/*----------------------------------------------------------------------------*/
/**@brief    eq关闭
   @param    eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_close(struct audio_eq *eq);

/*----------------------------------------------------------------------------*/
/**@brief    清除异步eq剩余的数据
   @param    eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_async_data_clear(struct audio_eq *eq);

/*----------------------------------------------------------------------------*/
/**@brief    获取异步eq剩余的数据
   @param    eq:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_eq_data_len(struct audio_eq *eq);

/*----------------------------------------------------------------------------*/
/**@brief    audio_eq_open重新封装，简化使用,该接口不接入audio_stream流处理
   @param    *parm: eq参数句柄,参数详见结构体struct audio_eq_param
   @return   eq句柄
   @note
*/
/*----------------------------------------------------------------------------*/
struct audio_eq *audio_dec_eq_open(struct audio_eq_param *parm);

/*----------------------------------------------------------------------------*/
/**@brief    audio_eq_close重新封装，简化使用,该接口不接入audio_stream流处理
   @param    eq句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_dec_eq_close(struct audio_eq *eq);

int audio_do_eq(void *eq, void *input, void *output, u32 len);

#endif

