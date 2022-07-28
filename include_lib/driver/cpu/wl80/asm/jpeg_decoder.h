/*****************************************************************
>file name : lib/system/cpu/dv16/video-dev/jpeg/jpeg_decoder.h
>author : lichao
>create time : Thu 08 Jun 2017 04:12:49 PM HKT
*****************************************************************/
#ifndef _JPEG_DECODER_H_
#define _JPEG_DECODER_H_

#include "typedef.h"
#include "spinlock.h"
#include "jpeg_codec.h"

extern u16 QT_TBL[0x80];
extern u16 STD_HUFFMAN_TBL[258];

#define QTAB_ADDR       ((s16 *)(QT_TBL))
#define HTAB_DC0_ADDR   ((u16 *)((u8*)STD_HUFFMAN_TBL))                         //  0x30
#define HTAB_AC0_ADDR   ((u16 *)((u8*)STD_HUFFMAN_TBL + 0x30))                  //  0xd2
#define HTAB_DC1_ADDR   ((u16 *)((u8*)STD_HUFFMAN_TBL + 0x30 + 0xd2))           //  0x30
#define HTAB_AC1_ADDR   ((u16 *)((u8*)STD_HUFFMAN_TBL + 0x30 + 0xd2 + 0x30))    //  0xd2

enum std_markers {
    DQT  = 0xDB,
    SOF  = 0xC0,
    DHT  = 0xC4,
    SOI  = 0xD8,
    SOS  = 0xDA,
    RST  = 0xD0,
    RST7 = 0xD7,
    EOI  = 0xD9,
    DRI  = 0xDD,
    APP0 = 0xE0,
};

struct decoder_info {
    u8 *data_out;
    u32 cur_pos;
    u16 x_pos;
    u16 y_pos;
    u16 x;
    u16 y;
    u16 old_x;
    u16 old_y;
    u16 old_x_mcu_num;
    u16 old_y_mcu_num;
    u16 x_mcu_num;
    u16 y_mcu_num;
    u16 x_mcu_cnt;
    u16 y_mcu_cnt;
    u8 samp_Y;
    u8 htab_Y;
    u8 htab_Cr;
    u8 htab_Cb;
    u8 qtab_Y;
    u8 qtab_Cr;
    u8 qtab_Cb;
    u8 y_cnt;
};

struct jpeg_decoder_fd {
    void *parent;
    struct decoder_info info;
    struct jpeg_yuv yuv[2];
    u32 static_yuv_size;
    u8 *static_yuv_addr;
    u32 mcu_num;
    u32 mcu_len;
    u32 head_len;
    u8  *stream;
    u8  *stream_alige4byte;
    u8  *stream_begin;
    u8  *stream_end;
    u32 bits_cnt;
    spinlock_t lock;
    volatile u8 active;
    u8  bits_mode;
    u8  dec_query_mode;
    u8  mode;
    u8  old_mode;
    u8  state;
    u8  yuv_type;
    u8  obuf_index;
    u8  manual_en;//control
    u8  frame_end;
    u8  last_rst_marker_seen;
    u8  DRI_enable;
    u8  next_rst;
    u32 next_rst_bits_cnt;
    u32 restart_interval;
    u32 rst_mcu_cnt;
    u32 frame_num;

    void *priv;
    int (*decoder_yuv_out)(void *priv, void *arg);
    int (*reset_output)(void *priv);
    u8 *fb;
    int cb_ylen;
    int	cb_ulen;
    int	cb_vlen;

    struct list_head entry;
    void *event_priv;
    int (*event_handler)(void *priv, enum jpg_dec_event event, void *arg);
    OS_SEM sem_bits;
};

int jpeg_decoder_init(struct jpeg_decoder_fd *fd, void *arg);
int jpeg_decoder_reset_param(struct jpeg_decoder_fd *fd, void *arg);
int jpeg_decoder_close(void *_fd);
int jpeg_decoder_reset(void *_fd, void *arg);
int jpeg_parse_header(void *_fd, void *info, u8 *buf, int len);
int jpeg_decoder_start(void *_fd, u8 *jframe, u32 len, u8 manual_en);
int jpeg_decoder_manual_start(void *_fd);
int jpeg_decoder_release(void *_fd);
int jpeg_decoder_change_omode(void *_fd, u8 *obuf, u8 omode);

int jpeg_decoder_set_yuv(void *_fd, void *yuv_addr, u32 len);
int decoder_bits_irq_handler(struct jpeg_decoder_fd *fd);
int decoder_mcu_irq_handler(struct jpeg_decoder_fd *fd);
int jpeg_decoder_input_data(void *_fd, void *data, int len);
int jpeg_decoder_set_event_handler(void *_fd, void *priv,
                                   int (*handler)(void *, enum jpg_dec_event, void *));


//subdevice_djpeg.c API
void *jpg_dec_open(struct video_format *f);
int jpg_dec_get_s_attr(void *_fh, struct jpg_dec_s_attr *attr);
int jpg_dec_set_yuv(void *_fh, void *yuv, u32 len);//yuv420/422图片的yuv长度：(w*16*3/2+64)*2以上，yuv422/444图片的yuv长度：(w*8*3/2+64)*2以上
int jpg_dec_set_s_attr(void *_fh, struct jpg_dec_s_attr *attr);
int jpg_dec_set_scaler_handler(void *_fh, void *priv,
                               int (*handler)(void *, struct image_scale_data *));
int jpg_dec_set_output_handler(void *_fh, void *priv,
                               int (*handler)(void *, struct YUV_frame_data *));
int jpg_dec_set_event_handler(void *_fh, void *priv,
                              int (*handler)(void *, enum jpg_dec_event, struct jpg_dec_mem *));
int jpg_dec_start(void *_fh);
int jpg_dec_stop(void *_fh);
int jpg_dec_input_data(void *_fh, void *data, u32 len);
int jpg_dec_close(void *_fh);


#endif
