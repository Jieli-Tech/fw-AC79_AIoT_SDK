
#ifndef __NET_PKG_IF_H__
#define __NET_PKG_IF_H__


#include "typedef.h"
#include "list.h"
#include "server/vpkg_server.h"
#include "system/spinlock.h"
#include "system/task.h"




/* nal */
enum nal_unit_type_e {
    NAL_UNKNOWN = 0,
    NAL_SLICE   = 1,
    NAL_SLICE_DPA   = 2,
    NAL_SLICE_DPB   = 3,
    NAL_SLICE_DPC   = 4,
    NAL_SLICE_IDR   = 5,    /* ref_idc != 0 */
    NAL_SEI         = 6,    /* ref_idc == 0 */
    NAL_SPS         = 7,
    NAL_PPS         = 8,
    NAL_AUD         = 9,
    /* ref_idc == 0 for 6,9,10,11,12 */
};

enum {
    NET_PKG_ERR_NONE,
    NET_PKG_ERR_FS,			//文件系统的句柄或者函数为空
    NET_PKG_ERR_WRITE,		//文件系统写失败
    NET_PKG_ERR_READ,		//文件系统读失败
    NET_PKG_ERR_NBUF,		//申请索引表或者MOV表头的空间失败
    NET_PKG_ERR_TYPE,		//索引类型出错
    NET_PKG_ERR_STREAM,		//位流出错
    NET_PKG_ERR_STATUS,		//状态出错
    NET_PKG_ERR_TAB_EXT,	//表溢出
    NET_PKG_ERR_CMD,		//输入了一个没有的命令
    NET_PKG_ERR_SEM,        //信号量出错
    NET_PKG_ERR_MUTEX,        //信号量出错
    NET_PKG_ERR_MIX_VAL,        //没有获取信息的函数或者获取信息失败
    NET_PKG_ERR_ADPCM_NOT_ALIGN,//adpcm_wav 没有对齐
    NET_PKG_WRITE_END,
    NET_PKG_ERR_NET,		//网络写失败
};

typedef struct _NET_PKG_INFO_T {
    const char *path;
    const char *fname;

    u8 *pre_pic_buf;
    u32 pre_pic_len;

    // int i_profile_idc;		//与H264编码级别相关的参数
    // int i_level_idc;		//与H264编码级别相关的参数

    u16 vid_width;
    u16 vid_heigh;
    u16 vid_real_width;
    u16 vid_real_heigh;
    u32 IP_interval;//每隔多少P帧有一个I帧
    u32 offset_size;//一级映射表大小，单位是4BYTE, 注意：这个值*4之后的值必须是512的倍数!
    u32 seek_len;
    u32 aud_sr;//采样率

    u8 aud_bits;//采样位深
    u8 aud_ch;//采样通道
    u8 aud_type;
    u8 aud_en;//是否需要封装音频
    u8 interval;//每隔多少视频帧插入一帧音频帧
    u8 cycle_time;
    u8 vid_fps;//帧率
    u8 channel;
} NET_PKG_INFO_T;

typedef struct _NET_PKG_FD {
    struct vpkg_sys_ops *sys_ops;
    int (*set_free)(void *priv, void *buf);
    void *priv;
    struct list_head vlist_head;
    struct list_head alist_head;
    NET_PKG_INFO_T input_info;
    volatile u32 status;
    OS_SEM pkg_sem;
    OS_MUTEX mutex;

    // mov_idx_ctl_t idx_ctl;
    u32 aud_interval_size;//音视频交叉时音频chunk大小
    u8 net_vidrt_onoff;//实时流开关标记
    u8 net_audrt_onoff;
    const char *netpath;
    u32 total_time;
    void *file;
    spinlock_t lock;
    u8 cycle;
    u8 first_time;
    u8 aud_clear_zero;
    u8 pkg_err;
    u32 total_frame;
    u32 total_frame_timeout;
    u32 adframe_cnt;
    u32 vdframe_cnt;
} NET_PKG_FD_T;


const struct vpkg_ops *get_net_priv_rtp_jpeg_pkg(void);
const struct vpkg_ops *get_net_priv_rtp_264_pkg(void);

#endif

