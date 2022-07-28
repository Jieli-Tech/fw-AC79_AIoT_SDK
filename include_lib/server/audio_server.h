#ifndef AUDIO_SERVER_H
#define AUDIO_SERVER_H

#include "fs/fs.h"
#include "server/audio_dev.h"

/**
 * @brief enum AUDIO_SERVER事件回调
 */
enum {
    AUDIO_SERVER_EVENT_CURR_TIME = 0x20,  /*!< AUDIO_SERVER编/解码当前时间 */
    AUDIO_SERVER_EVENT_END,               /*!< AUDIO_SERVER编/解码结束 */
    AUDIO_SERVER_EVENT_ERR,               /*!< AUDIO_SERVER编/解码错误 */
    AUDIO_SERVER_EVENT_SPEAK_START,       /*!< VAD检测到开始说话 */
    AUDIO_SERVER_EVENT_SPEAK_STOP,        /*!< VAD检测到停止说话 */
};

/**
 * @brief 解码附加属性
 */
enum {
    AUDIO_ATTR_REAL_TIME   = BIT(0),     /*!< 保证解码的实时性，解码读数不能堵塞，仅限于蓝牙播歌时时钟同步使用 */
    AUDIO_ATTR_LR_SUB      = BIT(1),     /*!< 伴奏功能，只支持双声道 */
    AUDIO_ATTR_PS_EN       = BIT(2),     /*!< 变速变声功能开关 */
    AUDIO_ATTR_LR_ADD      = BIT(3),     /*!< 解码器左右通道数据叠加 */
    AUDIO_ATTR_DECRYPT_DEC = BIT(4),     /*!< 文件解密播放，需要配合对应的加密工具 */
    AUDIO_ATTR_FADE_INOUT  = BIT(5),     /*!< 模拟音量淡入淡出，解码开始和暂停时使用 */
    AUDIO_ATTR_EQ_EN       = BIT(6),     /*!< EQ功能开关*/
    AUDIO_ATTR_DRC_EN      = BIT(7),     /*!< DRC功能开关，使能时需要打开EQ功能 */
    AUDIO_ATTR_EQ32BIT_EN  = BIT(8),     /*!< EQ 32bit输出 */
    AUDIO_ATTR_BT_AAC_EN   = BIT(9),     /*!< 蓝牙AAC解码 */
};

/**
 * \name 音频编解码请求操作类型
 * \{
 */
#define AUDIO_REQ_DEC                  0  /*!< 解码请求 */
#define AUDIO_REQ_ENC                  1  /*!< 编码请求 */
#define AUDIO_REQ_GET_INFO             2  /*!< 信息获取 */
#define AUDIO_REQ_IOCTL                4  /*!< 命令控制 */

#define AUDIO_DEC_OPEN                 0  /*!< 打开解码 */
#define AUDIO_DEC_START                1  /*!< 开始解码 */
#define AUDIO_DEC_PAUSE                2  /*!< 暂停解码 */
#define AUDIO_DEC_STOP                 3  /*!< 停止解码 */
#define AUDIO_DEC_FF                   4  /*!< 快进 */
#define AUDIO_DEC_FR                   5  /*!< 快退 */
#define AUDIO_DEC_GET_BREAKPOINT       6  /*!< 获取断点数据 */
#define AUDIO_DEC_PP                   7  /*!< 暂停/播放 */
#define AUDIO_DEC_SET_VOLUME           8  /*!< 设置解码音量 */
#define AUDIO_DEC_DIGITAL_GAIN_SET     9  /*!< 设置当前解码的数字增益 */
#define AUDIO_DEC_PS_PARM_SET          10 /*!< 设置变速变调的参数 */
#define AUDIO_DEC_GET_STATUS           11 /*!< 获取当前解码器状态 */

#define AUDIO_ENC_OPEN                 4  /*!< 打开编码 */
#define AUDIO_ENC_START                5  /*!< 开始编码 */
#define AUDIO_ENC_PAUSE                6  /*!< 暂停编码 */
#define AUDIO_ENC_STOP                 7  /*!< 停止编码 */
#define AUDIO_ENC_CLOSE                8  /*!< 关闭解码 */
#define AUDIO_ENC_SET_VOLUME           9  /*!< 设置编码模拟增益 */
#define AUDIO_ENC_GET_STATUS           10 /*!< 获取当前编码器状态 */
#define AUDIO_ENC_PP                   11 /*!< 暂停/编码 */
/* \} name */

/**
 * @brief 解码虚拟输出时的cbuf读写参数结构体
 */
struct audio_cbuf_t {
    void *cbuf;       /*!< cbuf句柄 */
    void *wr_sem;     /*!< 写信号量指针 */
    void *rd_sem;     /*!< 读信号量指针 */
    volatile int end; /*!< 读写结束 */
};

///  \cond DO_NOT_DOCUMENT
/**
 * @brief 数据流解码信息
 */
struct stream_codec_info {
    int time;            /*!< 时间 */
    int frame_num;       /*!< 帧数 */
    u32 frame_len;       /*!< 帧长度 */
    int frame_points;    /*!< 帧采样点 */
    int sequence_number; /*!< 序列号 */
    u32 sample_rate;     /*!< 采样率 */
    u8  channel;         /*!< 通道 */
};
/// \endcond

/**
 * @brief 解码断点播放信息结构体
 */
struct audio_dec_breakpoint {
    int len;   /*!< buf长度 */
    u32 fptr;  /*!< 断点位置偏移量 */
    u8 *data;  /*!< 断点数据指针 ape格式断点最大2036字节 */
};

/**
 * @brief 获取audio解码器信息
 */
struct audio_finfo {
    u8 channel;       /*!< 通道 */
    u8 name_code;     /*!< 名称编码 0:ansi, 1:unicode_le, 2:unicode_be */
    int sample_rate;  /*!< 采样率 */
    int bit_rate;     /*!< 比特率 */
    int total_time;   /*!< 总时间 */
};

/**
 * @brief audio命令控制
 */
struct audio_ioctl {
    u32 cmd;    /*!< 请求操作类型 */
    void *priv; /*!< 传入指针 */
};

/**
 * @brief 指定位置播放参数
 */
struct audio_dest_time_play_param {
    u32 start_time;	      /*!< 要跳转过去播放的起始时间。单位：ms。设置后跳到start_time开始播放 */
    u32 dest_time;	      /*!< 要跳转过去播放的目标时间。单位：ms。播放到dest_time后如果callback_func存在，则调用callback_func */
    u32(*callback_func)(void *priv); /*!< 到达目标时间后回调 */
    void *callback_priv;  /*!< 回调参数，可以在callback_func回调中实现对应需要的动作 */
};

/**
 * @brief 音频虚拟文件操作句柄
 */
struct audio_vfs_ops {
    void *(*fopen)(const char *path, const char *mode);  /*!< 打开创建路径文件 */
    int (*fread)(void *file, void *buf, u32 len);        /*!< 读文件 */
    int (*fwrite)(void *file, void *buf, u32 len);       /*!< 写文件 */
    int (*fseek)(void *file, u32 offset, int seek_mode); /*!< 寻址文件 */
    int (*ftell)(void *file);                            /*!< 返回给定流stream的当前文件位置 */
    int (*flen)(void *file);                             /*!< 获取文件长度 */
    int (*fclose)(void *file);                           /*!< 关闭文件 */
};

///  \cond DO_NOT_DOCUMENT
/**
 * @brief 解码输出句柄
 */
struct audio_dec_output_ops {
    void *(*get_buf)(void *priv, u32 *len);          /*!< 获取buf空间 */
    void (*put_buf)(void *priv, void *buf, u32 len); /*!< 推送buf空间 */
    const struct audio_vfs_ops *vfs_ops;             /*!< 音频虚拟文件操作句柄指针 */
};

/**
 * @brief audio解码句柄
 */
struct audio_decoder_ops {
    const char *name;/*!< 解码文件名称 */
    void *(*open)(FILE *file, void *priv,
                  const struct audio_dec_output_ops *ops, struct audio_dec_breakpoint *);     /*!< 打开文件 */
    void *(*try_open)(FILE *file, void *priv,
                      const struct audio_dec_output_ops *ops, struct audio_dec_breakpoint *); /*!< 再次尝试打开 */
    int (*get_audio_info)(void *, struct audio_finfo *info);                                  /*!< 获取音频信息 */
    int (*get_play_time)(void *);                                                             /*!< 获取播放时间 */
    int (*fast_forward)(void *, int step_s);                                                  /*!< 快进 */
    int (*fast_rewind)(void *, int step_s);                                                   /*!< 快退 */
    int (*get_breakpoint)(void *, struct audio_dec_breakpoint *);						      /*!< 获取断点 */
    int (*start)(void *);                                                                     /*!< 解码开始 */
    int (*close)(void *);                                                                     /*!< 解码结束 */
    int (*channel_switch)(void *, u8);                                                        /*!< 通道选择 */
    int (*stream_info_scan)(void *, struct stream_codec_info *info, void *data, int len);     /*!< 扫描数据流信息 */
    int (*stop)(void *);                                                                      /*!< 解码停止 */
    int (*ioctl)(void *, u32 cmd, void *parm);                                                /*!< 解码命令控制 */
};
/// \endcond

/**
 * @brief 解码器控制命令
 */
enum {
    AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A = 0x08,	/*!< 设置复读A点 */
    AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B,			/*!< 设置复读B点 */
    AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE,		/*!< 设置AB点复读模式 */

    AUDIO_IOCTRL_CMD_REPEAT_PLAY = 0x90,		/*!< 设置循环播放 */
    AUDIO_IOCTRL_CMD_SET_DEC_SR = 0x91,         /*!< 设置采样率或者码率 */
    AUDIO_IOCTRL_CMD_SET_DEST_PLAYPOS = 0x93,	/*!< 设置指定位置播放 */
    AUDIO_IOCTRL_CMD_GET_PLAYPOS = 0x94,		/*!< 获取毫秒级时间 */
};

/**
 * @brief 解码请求参数
 */
struct audio_dec_req {
    u8 cmd;                                                                       /*!< 请求操作类型 */
    u8 status;																	  /*!< 请求后返回的解码状态 */
    u8 channel;                                                                   /*!< 解码通道数 */
    u8 volume;                                                                    /*!< 解码音量(0-100) */
    u8 priority;                                                                  /*!< 解码优先级，暂时没用到 */
    u8 speedV;                                                                    /*!< >80是变快，<80是变慢，建议范围：30到130 */
    u16 pitchV;															          /*!< >32768是音调变高，<32768音调变低，建议范围20000到50000 */
    u16 attr;                                                                     /*!< 解码附加属性 */
    u8 digital_gain_mul;                                                          /*!< 数字增益乘值 */
    u8 digital_gain_div;                                                          /*!< 数字增益除值 */
    u32 output_buf_len;                                                           /*!< 解码buffer大小 */
    u32 orig_sr;                                                                  /*!< 原始采样率，当使能强制变采样时才使用 */
    u32 sample_rate;                                                              /*!< 实际的解码采样率 */
    u32 ff_fr_step;                                                               /*!< 快进快退级数 */
    u32 total_time;                                                               /*!< 解码的总共时长 */
    u32 play_time;                                                                /*!< 断点恢复时的当前播放时间 */
    void *output_buf;                                                             /*!< 解码缓存buffer，默认填NULL，由解码器自己实现分配和释放 */
    FILE *file;                                                                   /*!< 需要解码的文件句柄 */
    const char *dec_type;                                                         /*!< 解码格式 */
    const char *sample_source;                                                    /*!< 播放源，支持"dac","iis0","iis1" */
    struct audio_dec_breakpoint *bp;                                              /*!< 断点播放信息句柄  */
    const struct audio_vfs_ops *vfs_ops;                                          /*!< 虚拟文件操作句柄 */
    void *eq_attr;                                                                /*!< eq属性设置  */
    void *eq_hdl;                                                                 /*!< 预先申请好的的eq句柄  */
    struct audio_cbuf_t *virtual_audio;                                           /*!< 虚拟解码句柄，供外部读写使用 */
    int (*dec_callback)(u8 *buf, u32 len, u32 sample_rate, u8 ch_num);            /*!< 解码后的PCM数据回调 */
    int (*dec_sync)(void *priv, u32 data_size, u16 *in_rate, u16 *out_rate);      /*!< 解码对端采样率同步，常用于蓝牙解码 */
};

/**
 * @brief 编码请求参数
 */
struct audio_enc_req {
    u8 cmd;                                   /*!< 请求操作类型 */
    u8 status;								  /*!< 编码器状态 */
    u8 channel;                               /*!< 同时编码的通道数 */
    u8 channel_bit_map;                       /*!< ADC通道选择 */
    u8 volume;                                /*!< ADC增益(0-100)，编码过程中可以通过AUDIO_ENC_SET_VOLUME动态调整增益 */
    u8 priority;                              /*!< 编码优先级，暂时没用到 */
    u8 use_vad : 2;                           /*!< 0:关闭vad功能 1:使用旧vad算法 2:使用JL新vad算法 */
    u8 vad_auto_refresh : 1;                  /*!< 是否自动刷新VAD状态，赋值1表示SPEAK_START->SPEAK_STOP- >SPEAK_START->SPEAK_STOP->....循环 */
    u8 direct2dac : 1;                        /*!< AUDIO_AD直通DAC功能 */
    u8 high_gain : 1;                         /*!< 直通DAC时是否打开模拟增益调整 */
    u8 amr_src : 1;                           /*!< amr编码时的强制16k变采样为8kpcm数据，因为amr编码器暂时只支持8k编码 */
    u8 aec_enable : 1;                        /*!< AEC回声消除功能开关，常用于蓝牙通话 */
    u8 ch_data_exchange : 1;                  /*!< 用于AEC差分回采时和MIC的通道数据交换  */

    u8 no_header : 1;                         /*!< 用于opus编码时是否需要添加头部格式 */
    u8 vir_data_wait : 1;                     /*!< 虚拟编码时是否允许丢失数据 */
    u8 no_auto_start : 1;                     /*!< 请求AUDIO_ENC_OPEN时不自动运行编码器，需要主动调用AUDIO_ENC_START */
    u8 sample_depth : 3;                      /*!< 采样深度16bit或者24bit */
    u8 dns_enable : 1;                        /*!< dns降噪算法 0:不使用 1:使用 */
    u8 reserve : 1;                           /*!< 保留位 */
    u16 vad_start_threshold;                  /*!< VAD连续检测到声音的阈值，表示开始说话，回调AUDIO_SERVER_EVENT_SPEAK_START，单位ms，填0使用库内默认值 */
    u16 vad_stop_threshold;                   /*!< VAD连续检测到静音的阈值, 表示停止说话，回调AUDIO_SERVER_EVENT_SPEAK_STOP，单位ms,填0使用库内默认值 */
    u16 frame_size;                           /*!< 编码器输出的每一帧帧长大小，只有pcm格式编码时才有效 */
    u16 frame_head_reserve_len;               /*!< 编码输出的帧预留头部的大小 */
    u32 bitrate;                              /*!< 编码码率大小 */
    u32 delay_ms;                             /*!< 当编码器读写不到数据后的延时等待 */
    u32 output_buf_len;                       /*!< 编码buffer大小 */
    u32 sample_rate;                          /*!< 编码采样率 */
    u32 msec;                                 /*!< 编码时长，填0表示一直编码，单位ms，编码结束会回调AUDIO_SERVER_EVENT_END消息 */
    FILE *file;                               /*!< 编码输出文件句柄 */
    u8 *output_buf;                           /*!< 编码buffer，默认填NULL，由编码器自动分配和释放资源 */
    const char *format;                       /*!< 编码格式 */
    const char *sample_source;                /*!< 采样源，支持"mic","linein","plnk0","plnk1"，"virtual"，"iis0"，"iis1"，"spdif" */
    const struct audio_vfs_ops *vfs_ops;      /*!< 虚拟文件操作句柄 */
    u32(*read_input)(u8 *buf, u32 len);       /*!< 用于虚拟采样源"virtual"编码时的数据读取操作读输入buf及其长度 */
    void *aec_attr;                           /*!< AEC回声消除算法配置参数 */
};

/**
 * @brief audio服务请求参数
 */
union audio_req {
    struct audio_dec_req dec;         /*!< 解码请求 */
    struct audio_enc_req enc;         /*!< 编码请求 */
    struct audio_ioctl ioctl;         /*!< 命令控制 */
    struct audio_finfo info;          /*!< 音频信息 */
};

///  \cond DO_NOT_DOCUMENT
struct audio_package_ops {
    const char *name;                                                                 /*!< 文件名称 */
    const char *data_fmt;                                                             /*!< 数据格式 */
    void *(*open)(void *file, const struct audio_vfs_ops *, struct audio_enc_req *);  /*!< 打开文件编码 */
    int (*start)(void *file, const struct audio_vfs_ops *vfs_ops);                    /*!< 开始数据包处理 */
    int (*input)(void *file, u8 *data, int len, const struct audio_vfs_ops *vfs_ops); /*!< 输入数据 */
    int (*close)(void *file, const struct audio_vfs_ops *vfs_ops);                    /*!< 关闭数据处理 */
};


#define REGISTER_AUDIO_DECODER(ops) \
        const struct audio_decoder_ops ops SEC_USED(.audio_decoder)

extern const struct audio_decoder_ops audio_decoder_begin[];
extern const struct audio_decoder_ops audio_decoder_end[];

#define list_for_each_audio_decoder(p) \
    for (p = audio_decoder_begin; p < audio_decoder_end; p++)


#define REGISTER_AUDIO_PACKAGE(ops) \
        const struct audio_package_ops ops SEC_USED(.audio_package)

extern const struct audio_package_ops audio_package_begin[];
extern const struct audio_package_ops audio_package_end[];

#define list_for_each_audio_package(p) \
    for (p = audio_package_begin; p < audio_package_end; p++)
/// \endcond

#endif

