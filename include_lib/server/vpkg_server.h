#ifndef VIDEO_PACKAGE_H
#define VIDEO_PACKAGE_H

#include "typedef.h"
#include "list.h"

#define VPKG_WRITE_END   250

/*****************net video***************/
enum vpkg_net_type {
    VPKG_NET_WRITE_ERR = 0xA0,
    VIDEO_SERVER_PKG_NET_ERR,
    VPKG_REQ_SET_NETVRT,
    VPKG_REQ_SET_NETART,
    VPKG_REQ_CYC_FILE,
    VPKG_REQ_CYC_TIME,
    VPKG_REQ_TOTAL_FRAME,
    VPKG_CMD_PKG_NETVRT_MUTE,
    VPKG_CMD_PKG_NETART_MUTE,
    VPKG_CMD_PKG_CYC_FILE,
    VPKG_CMD_PKG_CYC_TIME,
    VPKG_CMD_PKG_TOTAL_FRAME,
};
struct vpkg_net_ops {
    char *name;
    void *(*fopen)(u8 type);
    int (*fwrite)(void *file, void *buf, u32 len);
    int (*fclose)(void *file);
};
struct net_vpkg_info {
    u8 net_audrt_mute;
    u8 net_vidrt_mute;
    u32 total_frame;
    u8 *video_lbuf;
    u8 *aud_lbuf;
    u32 video_lbuf_len;
    u32 aud_lbuf_len;
    const char *netpath;
};
/****************************************/

enum VPKG_REQ_TYPE {
    VPKG_REQ_SET_INFO,
    VPKG_REQ_GET_INFO,
    VPKG_REQ_CTRL,
    VPKG_REQ_VID_FRAME,
    VPKG_REQ_AUD_FRAME,
    VPKG_REQ_AUD_MUTE,
    VPKG_REQ_GET_PKG_TIME,
    VPKG_REQ_DEL_FILE,
};

enum VPKG_CMD {
    VPKG_UNKNOW,
    VPKG_OPEN,
    VPKG_START,
    VPKG_STOP,
    VPKG_CLOSE,
    VPKG_VID_INPUT,
    VPKG_AUD_INPUT,
};

enum {
    VPKG_CMD_PKG_RUN,
    VPKG_CMD_PKG_START,
    VPKG_CMD_PKG_STOP,
    VPKG_CMD_PKG_SET_STOP,
    VPKG_CMD_PKG_STATUS,
    VPKG_CMD_PKG_ADD_SKIP_FRM,
    VPKG_CMD_PKG_VIN,
    VPKG_CMD_PKG_AIN,
    VPKG_CMD_PKG_AUD_LEN,
    VPKG_CMD_PKG_FILE,
    VPKG_CMD_PKG_CLR_FB,
    VPKG_CMD_PKG_SET_SKIP_INTERVAL,
    VPKG_CMD_PKG_SET_SKIP_CONTINUE,
    VPKG_CMD_PKG_MUTE,
    VPKG_CMD_PKG_WEXR_DATA,
    VPKG_CMD_PKG_TIME,
    VPKG_CMD_DEL_FILE,
};

enum VPKG_STATUS {
    VPKG_STATUS_IDLE,
    VPKG_STATUS_OPEN,
    VPKG_STATUS_RUNING,
    VPKG_STATUS_STOP,
    VPKG_STATUS_READY_STOP,
    VPKG_STATUS_STOPING,
};

enum {
    VPKG_ISTA_IDLE = 0,
    VPKG_ISTA_READY,
    VPKG_ISTA_RUN,
    VPKG_ISTA_SET_STOP,
    VPKG_ISTA_STOP,
    VPKG_ISTA_STOPING,
    VPKG_ISTA_OPEN,
    VPKG_ISTA_ERR,
};

enum {

    VPKG_AUD_TYPE_UNKNOW = 0,
    VPKG_AUD_TYPE_PCM,
    VPKG_AUD_TYPE_ALAW,
    VPKG_AUD_TYPE_ULAW,
    VPKG_AUD_TYPE_ADPCM_WAV,
    VPKG_AUD_TYPE_AAC,

    VPKG_AUD_TYPE_END,
};



struct vpkg_sys_ops {
    char *name;
    void *(*fopen)(const char *path, const char *mode);
    int (*fread)(void *file, void *buf, u32 len);
    int (*fwrite)(void *file, void *buf, u32 len);
    int (*fseek)(void *file, u32 offset, int seek_mode);
    int (*fcat)(void *file_name, void *cat_fname, u8 cat_mode);
    int (*ftell)(void *file);
    int (*fclose)(void *file);
    int (*fdelete)(void *file);
};


struct vpkg_ops {
    char *name ;
    s32 pkg_fd_len;
    s32(*init)();
    s32(*open)(void *priv, void *arg);
    s32(*close)(void *priv);
    s32(*read)(void *priv, u8 *buf, u32 addr, u32 len);
    s32(*write)(void *priv, u8 *buf, u32 addr, u32 len);
    s32(*ioctrl)(void *priv, void *parm, u32 cmd);
};

extern struct vpkg_sys_ops video_pkg_sys_begin[];
extern struct vpkg_sys_ops video_pkg_sys_end[];

#define VIDEO_PKG_SYS_OPS_REG(ops) \
const struct vpkg_sys_ops ops SEC_USED(.video_pkg_sys)

#define list_for_each_video_pkg_ops_sys(ops) \
	for (ops = video_pkg_sys_begin; ops < video_pkg_sys_end; ops++)

extern struct vpkg_ops video_pkg_begin[];
extern struct vpkg_ops video_pkg_end[];

#define VIDEO_PKG_OPS_REG(ops) \
const struct vpkg_ops ops SEC_USED(.video_pkg)

#define list_for_each_video_pkg_ops(ops) \
	for (ops = video_pkg_begin; ops < video_pkg_end; ops++)


struct vpkg_media_info {

    u8 aud_en;//是否需要封装音频
    u8 aud_type;
    u8 aud_ch;//采样通道
    u8 aud_bits;//采样位深
    u32 aud_sr;//采样率

    u16 vid_width;
    u16 vid_heigh;
    u16 fake_w;
    u16 fake_h;

    int i_profile_idc;		//与H264编码级别相关的参数
    int i_level_idc;		//与H264编码级别相关的参数
    u32 offset_size;//一级映射表大小，单位是4BYTE, 注意：这个值*4之后的值必须是512的倍数!
    u8 IP_interval;//每隔多少P帧有一个I帧
    u8 interval;//每隔多少视频帧插入一帧音频帧
    u8 vid_fps;//帧率

};


struct vpkg_vframe {
    struct list_head entry;
    u8 *buf;
    u32 len;
    u32 test_num;
};

struct vpkg_aframe {
    struct list_head entry;
    u8 *buf;
    u32 len;
};

struct vpkg_ctrl {
    enum VPKG_CMD cmd;
};

struct vpkg_info {
    void *file;
    struct vpkg_sys_ops *sys_ops;
    struct vpkg_media_info minfo;
    int channel;
    int (*set_free)(void *priv, void *buf);
    void *priv;
    u32 seek_len;

    u8 *pre_pic_buf;
    u32 pre_pic_len;

    u32 cycle_time;
    u8 aud_mute;

    /*net video*/
    const char *path;
    const char *fname;
    struct net_vpkg_info net_info;

};

struct vpkg_get_info {
    int aud_size;
    void *file;
};

struct vpkg_get_pkg_time {
    int secs;
};

struct vpkg_aud_mute {
    int aud_mute;
};



union vpkg_req {

    struct vpkg_info info;
    struct vpkg_ctrl ctrl;

    struct vpkg_vframe vframe;
    struct vpkg_aframe aframe;

    struct vpkg_get_info get_info;

    struct vpkg_aud_mute pkg_mute;

    struct vpkg_get_pkg_time pkg_time;
};























#endif

