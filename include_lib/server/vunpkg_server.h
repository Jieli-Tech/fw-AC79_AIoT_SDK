
#ifndef __VUNPKG_SERVER_H__
#define __VUNPKG_SERVER_H__

#include "typedef.h"
#include "list.h"

enum {
    UNPKG_AUD_TYPE_UNKNOW = 0,
    UNPKG_AUD_TYPE_PCM_SOWT,
    UNPKG_AUD_TYPE_PCM_TWOS,
    UNPKG_AUD_TYPE_ALAW,
    UNPKG_AUD_TYPE_ULAW,
    UNPKG_AUD_TYPE_ADPCM_WAV,
    UNPKG_AUD_TYPE_AAC,


    UNPKG_AUD_TYPE_END,
};



enum VUNPKG_ISTA {

    VUNPKG_ISTA_IDLE = 0,
    VUNPKG_ISTA_READY,
    VUNPKG_ISTA_RUN,
    VUNPKG_ISTA_START,
    VUNPKG_ISTA_PAUSE,
    VUNPKG_ISTA_SET_STOP,
    VUNPKG_ISTA_STOP,
    VUNPKG_ISTA_STOPING,
    VUNPKG_ISTA_ERR,
};

enum VUNPKG_ICMD {
    VUNPKG_CMD_UNPKG_RUN,
    VUNPKG_CMD_UNPKG_START,
    VUNPKG_CMD_UNPKG_STOP,
    VUNPKG_CMD_UNPKG_SET_STOP,
    VUNPKG_CMD_UNPKG_STATUS,
    VUNPKG_CMD_UNPKG_PAUSE,
    VUNPKG_CMD_UNPKG_RESUME,
    VUNPKG_CMD_UNPKG_FF_FR,
    VUNPKG_CMD_UNPKG_FF_FR_END,
    VUNPKG_CMD_UNPKG_NOTIFY,
    VUNPKG_CMD_UNPKG_GET_MDIF,
};

enum VUNPKG_REQ_TYPE {
    VUNPKG_REQ_SET_INFO,
    VUNPKG_REQ_GET_INFO,
    VUNPKG_REQ_CTRL,
    VUNPKG_REQ_FF_FR,
    VUNPKG_REQ_GET_STATUS,
    VUNPKG_REQ_SET_FILE,
    VUNPKG_REQ_GET_PREVIEW_PIC,
};


enum VUNPKG_STATUS {
    VUNPKG_STATUS_OPEN,
    VUNPKG_STATUS_START,
    VUNPKG_STATUS_PAUSE,
    VUNPKG_STATUS_STOP,
    VUNPKG_STATUS_USER_STOP,
    VUNPKG_STATUS_CLOSE,
    VUNPKG_STATUS_ERROR,
};

enum VUNPKG_CMD {
    VUNPKG_UNKNOW,
    VUNPKG_OPEN,
    VUNPKG_START,
    VUNPKG_STOP,
    VUNPKG_CLOSE,

    VUNPKG_NOTIFY,
    VUNPKG_PAUSE,
    VUNPKG_RESUME,

};






struct vunpkg_sys_ops {
    void *(*fopen)(const char *path, const char *mode);
    int (*fread)(void *buf, u32 size, u32 count, void *file);
    int (*fseek)(void *file, u32 offset, int seek_mode);
    int (*ftell)(void *file);

    void *(*fb_malloc)(void *hdl, u32 size);
    void (*fb_free)(void *hdl, void *fb_buf);
    void (*fb_put)(void *hdl, void *fb_buf);
    void *(*fb_ptr)(void *fb_buf);
    u32(*fb_size)(void *fb_buf);

};


struct vunpkg_ops {
    char *name ;
    s32(*init)();
    s32(*open)(void *priv, void *arg);
    s32(*close)(void *priv);
    s32(*read)(void *priv, u8 *buf, u32 addr, u32 len);
    s32(*write)(void *priv, u8 *buf, u32 addr, u32 len);
    s32(*ioctrl)(void *priv, void *parm, u32 cmd);
};


struct vunpkg_media_info {

    /** VPKG_CHG_AUD **/
    u8 aud_en;//是否需要封装音频
    u8 aud_type;
    u8 aud_ch;//采样通道
    u8 aud_bits;//采样位深
    u32 aud_sr;//采样率
    u32 aud_flen;

    /** VPKG_CHG_VID **/
    u16 vid_width;
    u16 vid_heigh;
    u16 fake_w;
    u16 fake_h;

    int i_profile_idc;		//与H264编码级别相关的参数
    int i_level_idc;		//与H264编码级别相关的参数

    u32 extra_len;
    u8 *extra_data;

    u32 cur_vframe;
    u32 cur_aframe;
    u32 vframe_sum;
    u8 destroy_flag;        //if is imperfect file
    u8 vid_fps;//帧率
    u8 IP_interval;         //每隔多少P帧有一个I帧
};

struct vunpkg_info {
//	const char * path;
//	const char * fname;
    struct vunpkg_sys_ops *sys_ops;
    void *fb_vhdl;
    void *fb_ahdl;
    int channel;

};

struct vunpkg_file_info {
    /*const char *path;
    const char *fname;*/
    void *file;
};


#define VUNPKG_MODE_FF		    0x01		//快进
#define VUNPKG_MODE_FR		    0x02		//快退
#define VUNPKG_MODE_FF_FR_END	0x03

struct vunpkg_ff_fr_info {
    u8 seek_mode ;		//快进快退
    u32 frame_base;		//需要偏移的帧号基值,从1开始
    u32 frame_offset;	//需要向前或者向后定位的帧偏移(以1帧为单位)
};



struct vunpkg_ctrl {
    enum VUNPKG_CMD cmd;
};

struct vunpkg_preview {
    struct vunpkg_info info;
    struct vunpkg_file_info file_info;
};


union vunpkg_req {

    struct vunpkg_info info;
    struct vunpkg_file_info file_info;
    struct vunpkg_ctrl ctrl;
    struct vunpkg_media_info minfo;
    struct vunpkg_ff_fr_info ff_fr_ctrl;
    enum VUNPKG_STATUS status;
    struct vunpkg_preview prev;
};


#endif // __VUNPKG_SERVER_H__
