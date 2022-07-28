#ifndef VIDEO_DECODE_SERVER_H
#define VIDEO_DECODE_SERVER_H


#include "server/server_core.h"
#include "fs/fs.h"


#define VIDEO_DEC_EVENT_END          0x20
#define VIDEO_DEC_EVENT_CURR_TIME    0x21
#define VIDEO_DEC_EVENT_ERR          0x22


/*struct jpeg_file {
    const char *name;
};

struct jpeg_data {
    u8 *data;
    u32 data_len;
};

enum {
    JPEG_INPUT_TYPE_FILE,
    JPEG_INPUT_TYPE_DATA,
};

struct jpeg_decode_req {
    u8 input_type;
    union {
        struct jpeg_file file;
        struct jpeg_data data;
    } input;
    u8 *buf_y;
    u8 *buf_u;
    u8 *buf_v;
    u16 buf_width;
    u16 buf_height;
    u16 buf_xoffset;
    u16 buf_yoffset;
    u16 output_width;
    u16 output_height;
    u16 output_xoffset;
    u16 output_yoffset;
    void *priv;
    void (*stream_end)(void *priv);
};

enum {
    VIDEO_DECODE_REQ_START,
};


union video_decode_req  {
    struct jpeg_decode_req jpeg;
};*/



struct vds_video_info {
    u16 width;
    u16 height;
    u32 total_time;
};

struct vds_audio_param {
    void *buf;
    int buf_len;
    u32 volume;
};

struct vds_image_info {
    u8 *buf;
    u32 size;
    u8 *aux_buf;
    u32 aux_buf_size;
};

struct video_dec_param {
    u16 left;
    u16 top;
    u16 width;
    u16 height;
    u16 border_left;
    u16 border_top;
    u16 border_right;
    u16 border_bottom;
    u8 preview;
    u8 thm_first;
    u8 volume;
    FILE *file;
    const char *ftype;
    const char *fb;
    struct vds_image_info image;
    struct vds_video_info info;
    struct imc_presca_ctl *pctl;
    int (*get_yuv)(void *hdl, void *frame);
};

struct video_dec_preview {
    u8  show_mode;      // 0: 裁剪， 1: 缩放
    u8  rotate;
    u16 left;
    u16 top;
    u16 width;
    u16 height;
    u16 src_width;
    u16 src_height;
    u16 border_left;
    u16 border_top;
    u16 border_right;
    u16 border_bottom;
    u16 dst_left;
    u16 dst_top;
    u32 backcolor;
    FILE *file;
    const char *fb;
    struct vds_image_info image;
    struct vds_video_info info;
    struct imc_presca_ctl *pctl;
};

struct video_dec_ff_fr {
    u8 on;
    u8 once;
    u8 times;
    u16 msec_step;
};

/*
 * video_decoder server支持的请求列表
 */
enum {
    VIDEO_REQ_DEC_STOP,
    VIDEO_REQ_DEC_START,
    VIDEO_REQ_DEC_PLAY_PAUSE,
    VIDEO_REQ_DEC_RESUME,
    VIDEO_REQ_DEC_FF,
    VIDEO_REQ_DEC_FR,
    VIDEO_REQ_DEC_IMAGE,
    VIDEO_REQ_DEC_PREVIEW_OPEN,
    VIDEO_REQ_DEC_PREVIEW,
    VIDEO_REQ_DEC_PREVIEW_CLEAR,
    VIDEO_REQ_DEC_PREVIEW_MOVE,
    VIDEO_REQ_DEC_PREVIEW_FLUSH,
    VIDEO_REQ_DEC_PREVIEW_CLOSE,
};

union video_dec_req {
    struct video_dec_param dec;
    struct video_dec_ff_fr f;
    struct video_dec_preview preview;
};

struct dec_ram_device {
    u8  enable;
    const char *name;
    u32 video_addr;
    u32 video_size;
    u32 audio_addr;
    u32 audio_size;
};

struct video_dec_arg {
    const char *dev_name;
    int audio_buf_size;
    int video_buf_size;
    u8 *audio_buf;
    u8 *video_buf;
    struct dec_ram_device ram_dev;
};























#endif

