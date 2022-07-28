
#ifndef __RTP_PKG_IF_H__
#define __RTP_PKG_IF_H__


#include "typedef.h"
#include "list.h"
#include "server/vpkg_server.h"
#include "system/spinlock.h"
#include "system/task.h"


#define ENC_FORMAT_H264  0x1001
#define ENC_FORMAT_JPEG  0x1002

struct h264_info_t {
    const char *path;
    const char *fname;

    int i_profile_idc;		//与H264编码级别相关的参数
    int i_level_idc;		//与H264编码级别相关的参数

    u8 vid_fps;//帧率
    u16 vid_width;
    u16 vid_heigh;
    u16 vid_real_width;
    u16 vid_real_heigh;
    u32 IP_interval;//每隔多少P帧有一个I帧

    u8 aud_en;//是否需要封装音频
    u8 aud_type;
    u8 aud_bits;//采样位深
    u8 aud_ch;//采样通道
    u32 aud_sr;//采样率
    u8 interval;//每隔多少视频帧插入一帧音频帧
    u32 offset_size;//一级映射表大小，单位是4BYTE, 注意：这个值*4之后的值必须是512的倍数!

    u8 cycle_time;
    u32 seek_len;
    int channel;

};

struct jpeg_info_t {
    const char *path;
    const char *fname;

    unsigned char vid_fps;//帧率
    unsigned short vid_width;
    unsigned short vid_heigh;
    unsigned short fake_w;
    unsigned short fake_h;

    unsigned char aud_en;//是否需要封装音频
    unsigned char aud_type;
    unsigned char aud_ch;//采样通道
    unsigned char aud_bits;//采样位深
    unsigned char interval;//每隔多少视频帧插入一帧音频帧
    unsigned int aud_sr;//采样率
};

struct jpeg_pkg_fd {
    unsigned int aud_fsize;
    volatile u32 pkg_status;
    unsigned int v_offset;
    unsigned int a_offset;
    struct vpkg_vframe *vframe;
    struct vpkg_aframe *aframe;
    unsigned char vframe_open_flag;
    unsigned char aframe_open_flag;
    unsigned char aud_clear_zero;

    OS_SEM strm_pkg_sem;
    OS_SEM strm_sync_sem;
    OS_SEM strm_schedule_stop_sem;//关闭RTSP需要保证不再用其他信号量
    OS_MUTEX mutex;
    spinlock_t lock;

    struct vpkg_sys_ops *sys_ops;
    struct jpeg_info_t jpeg_info;
    struct list_head alist_head;
    struct list_head vlist_head;
    int (*set_free)(void *priv, void *buf);
    void *priv;
};

struct h264_pkg_fd {
    unsigned int aud_fsize;
    volatile u32 pkg_status;
    unsigned int a_offset;
    unsigned int v_offset;
    struct vpkg_vframe *vframe;
    struct vpkg_aframe *aframe;
    unsigned char vframe_open_flag;
    unsigned char aframe_open_flag;
    unsigned char aud_clear_zero;

    OS_SEM strm_pkg_sem;
    OS_SEM strm_sync_sem;
    OS_SEM strm_schedule_stop_sem;//关闭RTSP需要保证不再用其他信号量
    OS_MUTEX mutex;
    spinlock_t lock;

    struct vpkg_sys_ops *sys_ops;
    struct h264_info_t h264_info;
    struct list_head alist_head;
    struct list_head vlist_head;
    int (*set_free)(void *priv, void *buf);
    void *priv;
};

struct strm_pkg_fd {
    void *file;
    unsigned int stream_open_type;  //0 close , 1 open
    unsigned int vdframe_cnt;
    unsigned int adframe_cnt;
    unsigned int afbuf_cnt;
    unsigned int vfbuf_cnt;
    unsigned int pkt_time_len;  //rtp每20ms取一次数据
    unsigned char *alaw_buf;  //pcm数据编码为alaw格式后数据存放的buf

    struct vpkg_sys_ops *sys_ops;
    struct h264_pkg_fd h264_pkg;
    struct jpeg_pkg_fd jpeg_pkg;
};

const struct vpkg_ops *get_strm_jpeg_pkg_ops(void);
const struct vpkg_ops *get_strm_h264_pkg_ops(void);
int stream_media_server_init(struct fenice_config *conf);
void stream_media_server_uninit(void);
int is_live();

#endif

