#ifndef _IMAGE_ENC_H_
#define _IMAGE_ENC_H_
#include "video/video.h"
#include "asm/jpeg_codec.h"

/*

#define JPEG_ENC_CMD_BASE                0x00300000
#define JPEG_ENC_SET_Q_VAL               (JPEG_ENC_CMD_BASE + 1)
#define JPEG_ENC_SET_ABR                 (JPEG_ENC_CMD_BASE + 2)
#define SET_IMAGE_AUXILIARY_BUF          (JPEG_ENC_CMD_BASE + 3)
#define JPEG_ENC_SET_CYC_TIME            (JPEG_ENC_CMD_BASE + 4)
#define JPEG_ENC_SET_Q_TABLE             (JPEG_ENC_CMD_BASE + 5)


struct icap_auxiliary_mem {
    u8 *addr;
    u32 size;
};
*/

/*
struct jpg_q_table {
    u16  YQT_DCT[0x40] ;
    u16  UVQT_DCT[0x40];
    u8   DQT[138]; //file header
};
*/
struct image_enc_s_attr {
    u16 width;
    u16 height;
    u8 *buf;
    u16 src_width;
    u16 src_height;
    u8 *src_addr;
    u32 src_size;
};


void *image_enc_open(struct video_format *f, u8 multi_scale, int max_width);
void *image_enc_force_open(struct video_format *f, u8 multi_scale, int max_width, int id);
int image_enc_set_fmt(void *_fh, struct video_format *f, u8 multi_scale, int max_width);
int image_handl2ch(void *_fh);
int image_enc_set_scale_handler(void *_fh, void *priv, int (*handler)(void *, struct image_scale_data *));
int image_enc_get_s_attr(void *_fh, struct image_s_attr *attr);
int image_enc_set_s_attr(void *_fh, struct image_s_attr *attr);
int image_capture_enc_start(void *_fh, struct image_capture_info *info);

bool image_enc_other_streamon(void *_fh);
int image_enc_pause_other_stream(void *_fh);
int image_enc_resume_other_stream(void *_fh);
int image_enc_set_scale_mode(void *_fh, u8 mode);
int image_enc_get_source_attr(void *_fh, struct yuv_image *image);
int image_enc_set_source_attr(void *_fh, struct yuv_image *image);
int image_enc_set_input_buf(void *_fh, struct video_cap_buffer *b);
void image_enc_set_output_buf_ops(void *_fh, void *priv, const struct mjpg_user_ops *ops);
int image_enc_get_osd_attr(void *_fh, struct video_osd_attr *attr);
int image_enc_set_osd_attr(void *_fh, struct video_osd_attr *attr);
int image_enc_close(void *_fh);
#endif
