#ifndef __EFFECTS_ADJ__H
#define __EFFECTS_ADJ__H


#include "eq/audio_eq.h"
#include "drc/audio_drc.h"
#include "drc/sw_drc.h"
#include "eq/DynamicEQ_api.h"
#include "eq/DynamicEQ_Detection_api.h"
// #include "media/howling_api.h"
// #include "media/reverb_api.h"
// #include "media/noisegate_api.h"
// #include "media/audio_gain_process.h"
// #include "media/voiceChanger_api.h"
// #include "media/audio_vbass.h"
// #include "media/multi_ch_mix.h"
// #include "media/audio_surround.h"
// #include "media/audio_ch_swap.h"


struct effect_adj {
    u8 eq_type;
    uint8_t password_ok;
    struct __effect_mode_cfg 	*parm;
    struct __tool_callback      *cb;
};

typedef struct {
    int cmd;     			///<EQ_ONLINE_CMD
    int data[64];       	///<data
} EFF_ONLINE_PACKET;

enum {
    EFF_CMD_INQUIRE = 0x4,
    EFF_CMD_GETVER = 0x5,
    EFF_CMD_FILE_SIZE = 0xB,
    EFF_CMD_FILE = 0xC,
    EFF_CMD_CHANGE_MODE = 0xE,//切模式
    EFF_CMD_RESYNC_PARM_END = 0x28,//参数重复结束
    EFF_CMD_RESYNC_PARM_BEGIN = 0x30,//参数重复开始

    EFF_MIC_EQ0 = 0x1001,
    EFF_MIC_EQ1 = 0x1002,
    EFF_MIC_EQ2 = 0x1003,
    EFF_MIC_EQ3 = 0x1004,
    EFF_MIC_EQ4 = 0x1005,
    EFF_MIC_DRC0 = 0x1006,
    EFF_MIC_DRC1 = 0x1007,
    EFF_MIC_DRC2 = 0x1008,
    EFF_MIC_DRC3 = 0x1009,
    EFF_MIC_DRC4 = 0x100a,

    EFF_MIC_PLATE_REVERB  = 0x100c,
    EFF_MIC_ECHO          = 0x100e,
    EFF_MIC_NOISEGATE     = 0x1014,
    EFF_MIC_HOWLINE_PS    = 0x1015,
    EFF_MIC_NOTCH_HOWLING = 0x1016,
    EFF_MIC_VOICE_CHANGER = 0x1017,
    EFF_MIC_MIX_GAIN      = 0x1018,

    EFF_PHONE_WIDEBAND_EQ   = 0x1101,
    EFF_PHONE_NARROWBAND_EQ = 0x1102,
    EFF_AEC_WIDEBAND_EQ     = 0x1103,
    EFF_AEC_NARROWBAND_EQ   = 0x1104,
    EFF_PHONE_WIDEBAND_DRC  = 0x1105,
    EFF_PHONE_NARROWBAND_DRC = 0x1106,
    EFF_AEC_WIDEBAND_DRC    = 0x1107,
    EFF_AEC_NARROWBAND_DRC  = 0x1108,

    EFF_MUSIC_EQ          = 0x2001,
    EFF_MUSIC_LOW_DRC     = 0x2002,
    EFF_MUSIC_FR_EQ       = 0x2003,
    EFF_MUSIC_MID_DRC     = 0x2011,
    EFF_MUSIC_HIGH_DRC    = 0x2012,
    EFF_MUSIC_WHOLE_DRC   = 0x2013,
    EFF_MUSIC_CROSSOVER   = 0x2014,
    EFF_MUSIC_DYNAMIC_EQ  = 0x2015,
    EFF_MUSIC_EQ2         = 0x2016,
    EFF_MUSIC_GAIN        = 0x2017,

    EFF_MIC_GAIN          = 0x2018,//混响输出位置的gain
    EFF_MUSIC_RL_GAIN     = 0x2019,//rl_rr通道gain

    EFF_MUSIC_RL_RR_LOW_PASS = 0x2020,//2.1/2.2声道低通滤波器
    EFF_MUSIC_RL_EQ          = 0x2005,
    EFF_MUSIC_RL_LOW_DRC     = 0x2006,
    EFF_MUSIC_RR_EQ          = 0x2007,
    EFF_MUSIC_RL_MID_DRC     = 0x2027,
    EFF_MUSIC_RL_HIGH_DRC    = 0x2028,
    EFF_MUSIC_RL_WHOLE_DRC   = 0x2029,
    EFF_MUSIC_RL_CROSSOVER   = 0x2029,

    EFF_AUX_EQ         = 0x2030,//linein 需要独立的音效时，使用的效果id
    EFF_AUX_DRC        = 0x2031,
    EFF_AUX_GAIN       = 0x2032,

    EFF_MUSIC_VOICE_CHANGER   = 0x2033,
    EFF_MUSIC_VBASS           = 0x2034,//音乐虚拟低音
    EFF_MUSIC_VBASS_PREV_GAIN = 0x2035,

    EFF_MUSIC_LPF_EQ              = 0x2036,
    EFF_MUSIC_HIGH_BASS_EQ        = 0x2037,
    EFF_MUSIC_HIGH_BASS_DRC       = 0x2038,

    EFF_MUSIC_NOISEGATE           = 0x2039,
    EFF_MUSIC_SURRROUND_EFF       = 0x2040,
    EFF_MUSIC_EXT_EQ              = 0x2041,//2.1声道全频音箱通路的后级eq

    EFF_MUSIC_CH_SWAP             = 0x2042,
    EFF_MUSIC_RL_CH_SWAP          = 0x2043,
    EFF_DAC_PGA                   = 0x2044,
    EFF_MUSIC_EXT_EQ2             = 0x2045,//2.1声道低音音箱通路的后级eq
    EFF_MUSIC_FR_EXT_EQ           = 0x2046,
    EFF_ADC_PGA                   = 0x2047,

    EFF_MUSIC_FR_LOW_DRC         = 0x2060,
    EFF_MUSIC_FR_MID_DRC         = 0x2061,
    EFF_MUSIC_FR_HIGH_DRC        = 0x2062,
    EFF_MUSIC_FR_WHOLE_DRC       = 0x2063,
    EFF_MUSIC_FR_CROSSOVER       = 0x2064,

    EFF_MUSIC_RR_LOW_DRC         = 0x2065,
    EFF_MUSIC_RR_MID_DRC         = 0x2066,
    EFF_MUSIC_RR_HIGH_DRC        = 0x2067,
    EFF_MUSIC_RR_WHOLE_DRC       = 0x2068,
    EFF_MUSIC_RR_CROSSOVER       = 0x2069,

    EFF_CMD_MAX,//最后一个
};

typedef enum {
//模式id,效果文件解析时会用于定位相应模式下效果参数
    phone_mode_seq = 1,
    aec_mode_seq = 2,

    music_mode_seq0 = 0x4,

    mic_mode_seq0 = 0x5,
    mic_mode_seq1 = 0x6,
    mic_mode_seq2 = 0x7,
    mic_mode_seq3 = 0x8,
    mic_mode_seq4 = 0x9,
    mic_mode_seq5 = 0xa,
    mic_mode_seq6 = 0xb,
    mic_mode_seq7 = 0xc,

    linein_mode_seq = 0xe,
//add xx
    max_seq,
} MODE_NUM;

//AudioEffects ID(AEID) List: EQ/DRC等模块ID，识别不同模式下EQ\DRC效果用
typedef enum {
//通话下行音效处理
    AEID_ESCO_DL_EQ = 1,
    AEID_ESCO_DL_DRC,

//通话上行音效处理
    AEID_ESCO_UL_EQ,
    AEID_ESCO_UL_DRC,

//音乐播放音效处理
    AEID_MUSIC_EQ,
    AEID_MUSIC_DRC,
    AEID_MUSIC_FR_EQ,
    AEID_MUSIC_FR_DRC,
    AEID_MUSIC_RL_EQ,
    AEID_MUSIC_RL_DRC,
    AEID_MUSIC_RR_EQ,
    AEID_MUSIC_RR_DRC,
    AEID_MUSIC_EQ2,
    AEID_MUSIC_EXTEQ,
    AEID_MUSIC_EXTEQ2,

//混响音效处理
    AEID_MIC_EQ0,
    AEID_MIC_DRC0,
    AEID_MIC_EQ1,
    AEID_MIC_DRC1,
    AEID_MIC_EQ2,
    AEID_MIC_DRC2,
    AEID_MIC_EQ3,
    AEID_MIC_DRC3,
    AEID_MIC_EQ4,
    AEID_MIC_DRC4,
    AEID_MIC_NS_GATE,
    AEID_MIC_AUTOTUNE,
    AEID_MIC_TUNNING_EQ,
//增益计算
    AEID_MIC_GAIN,
    AEID_MUSIC_GAIN,
    AEID_MUSIC_RL_GAIN,//rl通道
//linein
    /* AEID_LINEIN_EQ, */
    // AEID_AUX_DRC,
    /* AEID_LINEIN_GAIN, */
//高低音eq
    AEID_HIGH_BASS_EQ,
    AEID_HIGH_BASS_DRC,
//变声

    AEID_MIC_VOICE_CHANGER,
    AEID_MUSIC_VOICE_CHANGER,
//虚拟低音
    AEID_MUSIC_VBASS,
    AEID_MUSIC_VBASS_PREV_GAIN,
//门限噪声
    AEID_MUSIC_NS_GATE,

//环绕音效
    AEID_MUSIC_SURROUND,

    AEID_MUSIC_LPF_EQ,

    AEID_MUSIC_DYNAMIC_EQ,

    AEID_MUSIC_CH_SWAP,

//aux eff
    AEID_AUX_VBASS,
    AEID_AUX_VBASS_PREV_GAIN,
    AEID_AUX_NS_GATE,
//AUX 播放音效处理
    AEID_AUX_EQ,
    AEID_AUX_DRC,
    AEID_AUX_FR_EQ,
    AEID_AUX_FR_DRC,
    AEID_AUX_RL_EQ,
    AEID_AUX_RL_DRC,
    AEID_AUX_RR_EQ,
    AEID_AUX_RR_DRC,
    AEID_AUX_EQ2,
    AEID_AUX_EXTEQ,
    AEID_AUX_EXTEQ2,
    AEID_AUX_GAIN,
    AEID_AUX_RL_GAIN,//rl通道
    AEID_AUX_LPF_EQ,
    AEID_AUX_DYNAMIC_EQ,
    AEID_AUX_HIGH_BASS_EQ,
    AEID_AUX_HIGH_BASS_DRC,
    AEID_AUX_SURROUND,
    AEID_AUX_CH_SWAP,
} AudioEffectsID; //模块id

struct mode_list {
    u16 module_name;
    u8 nsection;
    u8 group_num;
    u16 group_id[5];
};

#if 0
//reverb 模块：
typedef struct REVERBN_PARM_TOOL_SET_ {
    int is_bypass;          // 1-> byass 0 -> no bypass
    REVERBN_PARM_SET parm;
} REVERBN_PARM_TOOL_SET;

//reverb_palte 模块：
typedef  struct  _Plate_reverb_TOOL_SET_ {
    int is_bypass;          // 1-> byass 0 -> no bypass
    Plate_reverb_parm parm;
} Plate_reverb_TOOL_SET;

//reverb_filter模块：
typedef struct _EF_REVERB0__TOOL_SET_ {
    int is_bypass;          // 1-> byass 0 -> no bypass
    EF_REVERB0_PARM parm;
} EF_REVERB0_TOOL_SET;

//echo模块:
typedef struct _EF_ECHO_TOOL_SET_ {
    int is_bypass;          // 1-> byass 0 -> no bypass
    ECHO_PARM_SET parm;
} EF_ECHO_TOOL_SET;

//移频模块 HowlingPitchShift:
typedef struct _HowlingPs_PARM_TOOL_SET_ {
    int is_bypass;          // 1-> byass 0 -> no bypass
    HOWLING_PITCHSHIFT_PARM parm;
} HowlingPs_PARM_TOOL_SET;

typedef struct _NotchHowlingParam_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    NotchHowlingParam parm;
} NotchHowlingParam_TOOL_SET;

typedef struct _VoiceChangerParam_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    VOICECHANGER_PARM parm;
} VoiceChangerParam_TOOL_SET;

typedef struct _NoiseGateParam_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    noisegate_update_param parm;
} NoiseGateParam_TOOL_SET;

typedef struct _SurroundEffect_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    surround_update_parm parm;
} SurroundEffect_TOOL_SET;

typedef struct _gain_process_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    struct aud_gain_parm_update parm;
} Gain_Process_TOOL_SET;

typedef struct _ChannelSwap_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
} ChannelSwap_TOOL_SET;
#endif

typedef struct _dac_pga_TOOL_SET {
    u8 again_fl;
    u8 again_fr;
    u8 again_rl;
    u8 again_rr;
    u16 reserve[4];
} dac_pga_TOOL_SET;

typedef struct _ADC_PGA_TOOL_SET {
    u8 gain_mic;
    u8 gain_line;
    u8 MicBoostFlag;
    u8 reserve[13];
} ADC_PGA_TOOL_SET;

struct advance_iir {
    int fc;
    int order;
    int type;
};

#if 0
// #低通 LowPass:
typedef struct _LowPassParam_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct advance_iir low_pass;
} LowPassParam_TOOL_SET;

// #高通 HighPass:
typedef struct _HighPassParam_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    struct advance_iir high_pass;
} HighPassParam_TOOL_SET;

//虚拟低音
typedef struct _VirtualBass_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    VirtualBassUdateParam parm;
} VirtualBass_TOOL_SET;

typedef struct _Mix_TOOL_SET {
    float gain1;
    float gain2;
    float gain3;
} Mix_TOOL_SET;

struct _AUTOTUNE_TOOL_SET {
    int is_bypass; // 1-> byass 0 -> no bypass
    AUTOTUNE_PARM parm;
};
#endif

//动态EQ DynamicEQ:
typedef struct _DynamicEQParam_TOOL_SET {
    int is_bypass;          // 1-> byass 0 -> no bypass
    DynamicEQEffectParam  effect_param[4];
    int nSection;					//段数
    int detect_mode;			//检测模式
} DynamicEQParam_TOOL_SET; //实际发送这个结构体

#define OTHER_SECTION_MAX (5)
struct eq_tool {
    float global_gain;
    int seg_num;          //eq效果文件存储的段数
    int enable_section;   //
    struct eq_seg_info seg[OTHER_SECTION_MAX];   //eq系数存储地址
};

#define mSECTION_MAX (32)
#define MULTI_BAND_DRC 1

struct music_eq_tool {
    float global_gain;
    int seg_num;          //eq效果文件存储的段数
    int enable_section;   //
    struct eq_seg_info seg[mSECTION_MAX];   //eq系数存储地址
};

#define mSECTION_MAX2 (10)
struct music_eq2_tool {
    float global_gain;
    int seg_num;          //eq效果文件存储的段数
    int enable_section;   //
    struct eq_seg_info seg[mSECTION_MAX2];   //eq系数存储地址
};

struct nband_drc {
    CrossOverParam_TOOL_SET crossover;
#if !MULTI_BAND_DRC
    wdrc_struct_TOOL_SET wdrc_parm[1];//[0]全带
#else
    wdrc_struct_TOOL_SET wdrc_parm[4];//[0]low  [1]mid [2]high [3]多带之后附加的全带
#endif
};

struct music_parm_tool_set {
    struct music_eq_tool eq_parm;
    struct nband_drc drc_parm;
};

struct music_parm_tool_set *get_audio_eff_music_mode(void);
int ext_eff_file_analyze(u32 mode_id, u16 group_id, u8 tar_snap, void *data_buf, u32 buf_len);
int eff_file_analyze(u32 mode_id, u16 group_id, void *data_buf, u32 buf_len);
void get_eff_mode(u16 mode_id, u16 group_id, u8 *mode_index, u8 *drc_index);//获取混响模式的index
int get_phone_mode(u16 group_id);
int get_group_id(u16 mode_name, u8 tar);
int get_eq_nsection(u16 module_name);

void noisegate_update_parm(void *parm, int bypass);
void plate_reverb_update_parm(void *parm, int bypass);
void echo_updata_parm(void *parm, int bypass);
void howling_pitch_shift_update_parm(void *parm, int bypass);
void notchhowline_update_parm(void *parm, int bypass);
void audio_dynamic_eq_detection_update_parm(u32 dynamic_eq_name, void *parm, int bypass);
void audio_dynamic_eq_update_parm(u32 dynamic_eq_name, void *parm, void *parm2, int bypass);
void mic_gain_update_parm(u16 gain_name, void *parm, int bypass);
void set_mic_reverb_mode_by_id(u8 mode);
void cp_eq_file_seg_to_custom_tab();
int get_module_name_and_index(u16 group_id, u16 *index, u8 tar);
void dynamic_eq_printf(DynamicEQParam_TOOL_SET *dy_parm);

void music_eq_printf(void *_parm);
void music_eq2_printf(void *_parm);
void wdrc_printf(void *_wdrc);
void eq_printf(void *_parm);

int get_index_by_group_id(u16 groud_id, u8 tar);
void set_list_nsection(u8 tar, u8 index, u8 nsection);

/*
 *效果文件切换
 *path:效果文件路径
 * */
void eff_file_switch(u8 *path);


struct cmd_interface {
    u32 cmd;
    void (*eff_default_parm_init)(void);//音效参数默认初始化默认值
    void (*eff_file_analyze_init)(void);//解析效果文件内相应的音效参数到目标地址
    int (*cmd_deal)(EFF_ONLINE_PACKET *packet, u8 id, u8 sq);//在线调试处理函数
};

#define REGISTER_CMD_TARGET(cmd_eff) \
	static const struct cmd_interface cmd_eff SEC_USED(.eff_cmd)

extern const struct cmd_interface cmd_interface_begin[];
extern const struct cmd_interface cmd_interface_end[];

#define list_for_each_cmd_interface(p) \
	    for (p = (struct cmd_interface *)cmd_interface_begin; p < (struct cmd_interface *)cmd_interface_end; p++)

struct snap_interface {
    void (*eff_snap_parm_switch)(u8 tar_snap);//解析音效模块快照信息
};

#define REGISTER_SNAP_TARGET(snap_eff) \
	const struct snap_interface snap_eff SEC_USED(.snap_parm)

extern const struct snap_interface snap_interface_begin[];
extern const struct snap_interface snap_interface_end[];

#define list_for_each_snap_interface(p) \
	    for (p = (struct snap_interface *)snap_interface_begin; p < (struct snap_interface *)snap_interface_end; p++)


#define phone_eq_nsection  OTHER_SECTION_MAX
#define music_eq_nsection  10
#define music_eq2_nsection  10
#define mic_eq_nsection   OTHER_SECTION_MAX

#endif/*__EFFECTS_ADJ__H*/
