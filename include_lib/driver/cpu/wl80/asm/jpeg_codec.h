/*****************************************************************
>file name : lib/system/cpu/dv16/video-dev/jpeg/jpeg_codec.h
>author : lichao
>create time : Thu 08 Jun 2017 04:13:07 PM HKT
*****************************************************************/

#ifndef _JPEG_CODEC_H_
#define _JPEG_CODEC_H_

#include "typedef.h"
#include "video/video.h"



#define JPEG0_SFR_BEGIN  &JPG0_CON0
#define JPEG1_SFR_BEGIN  &JPG1_CON0

/*
#define JPEG_FMT_YUV444    0
#define JPEG_FMT_YUV422    1
#define JPEG_FMT_YUV420    3
*/

struct jpg_dec_attr {
    u16 max_o_width;
    u16 max_o_height;
};

enum {
    SOURCE_FROM_IMC,
    SOURCE_FROM_DDR,
};

enum codec_state {
    JPEG_CODEC_IDLE = 0x0,
    JPEG_CODEC_ENC_STREAM, //编码视频流 -- 优先级最高解码不可打断，除非外部手动打断
    JPEG_CODEC_ENC_IMAGE,  //编码图片
    JPEG_CODEC_MANUAL_ENC,
    JPEG_CODEC_DEC_STREAM,
    JPEG_CODEC_DEC_IMAGE,  //解码图片
};
enum jpeg_enc_mode {
    JPEG_ENC_TYPE_NONE = 0x0,
    JPEG_ENC_TYPE_STREAM,
    JPEG_ENC_TYPE_IMAGE,
    JPEG_ENC_AUTO_STREAM,
    JPEG_ENC_MANU_STREAM,
    JPEG_ENC_AUTO_IMAGE,
    JPEG_ENC_MANU_IMAGE,
};

enum jpg_dec_event {
    JPG_DEC_EVENT_GET_MEM = 0x0, //向外部请求sdram/ddr的buffer用于解码缓冲解码+缩放
    JPG_DEC_EVENT_PUT_MEM,
    JPG_DEC_EVENT_BITS_RUN_OUT,//jpg解码已经消耗尽位流，需要重新设置下一段位流
    JPG_DEC_EVENT_TRY_ALLOC_VRAM,//获取video ram的buffer用于YUV输出
    JPG_DEC_EVENT_FREE_VRAM,
};

struct jpg_dec_vram {
    u8 got;
    u8 smpfmt;
    u16 width;
    u8 *buf[3];
    int num;
    int size;
};

#define AUTO_STREAM_CAPTURE     0x0
#define MANU_STREAM_CAPTURE     0x1

enum mjpg_irq_code {
    MJPG_IRQ_PRECNT,
    MJPG_IRQ_BUFFER_FULL,
    MJPG_IRQ_SPEED_INTEN,
    MJPG_IRQ_NO_BUFF,
    MJPG_IRQ_MCU_PEND,
};

enum mjpg_frame_type {
    MJPG_STREAM_FRAME,
    MJPG_SINGLE_FRAME,
};

struct mjpg_user_ops {
    void   *(*malloc)(void *_video, u32 size);
    void   *(*realloc)(void *_video, void *fb, int newsize);
    void (*free)(void *_video, void *fb);
    int (*size)(void *fb);
    int (*free_size)(void *_video);
    void (*output_frame_end)(void *_video, void *buf);
};

struct jpeg_yuv {
    u8 *y;
    u8 *u;
    u8 *v;
};

/*
struct jpeg_encoder_param {
    u16 width;        //图像宽度
    u16 height;       //图像高度
    u32 kbps; //目标码率
    u8  fps; //帧率
    u8  format; //编码格式
    u8  source;    //数据输入方式，从摄像头或者指定地址
    u8  q;    //编码质量，0-8 9级，越大质量越好，码流越大
    u8  enable_abr;
    u8  enable_dyhuffman;
    u8  prio;
    u8  vbuf_num;//circle video buffer number
    u8  type;
    u32 file_head_len;
    u8  *thumbnails;
    int thumb_len;
    struct jpeg_yuv yuv;
    struct jpg_q_table *qt;
    void *priv;
    void *fbpipe;
    int (*insert_frame)(void *priv, u8 insert_type);
    int (*insert_watermarking)(void *priv);
    void (*reset_source)(void *priv);
    int (*output_frame_buffer)(void *priv, void *ptr);
    struct mjpg_fb_ops *fb_ops;

};
*/

#define JPG_SAMP_FMT_INVALID     0x0 //
#define JPG_SAMP_FMT_YUV444      0x1 //
#define JPG_SAMP_FMT_YUV422      0x2 //
#define JPG_SAMP_FMT_YUV420      0x3 //

#define BITS_MODE_UNCACHE  0
#define BITS_MODE_CACHE    1

#define SINGLE_BUF_MODE    0x0
#define DOUBLE_BUF_MODE    0x1
#define CIRCLE_BUF_MODE    0x2
#define WHOLE_BUF_MODE     0x3

#define INSERT_EMPTY_FRAME      0x1
#define INSERT_SPECAIL_FRAME    0x2
#define INVALID_Q_VAL           0xff

#define BITS_ONE_FRAME      0
#define BITS_DIV_DATA       1
struct jpeg_decoder_param {
    u8  bits_mode;
    u8  out_mode;
    u8  yuv_type;
    void *priv;
    int (*yuv_out_dest)(void *priv, void *arg);
    int (*reset_output)(void *priv);
    u8  *cbuf;
    //int (*jpg_info_wait_data)(void *priv);
};

enum {
    JPEG_INPUT_TYPE_FILE,
    JPEG_INPUT_TYPE_DATA,
};

enum {
    JPEG_DECODE_TYPE_YUV420 = 0,
    JPEG_DECODE_TYPE_DEFAULT,
};

#define DEC_YUV_ALIGN_SIZE(size)  (((size) + 32 - 1) & ~(32 - 1))
struct decoder_yuv_out {
    int line;
    int width;
    int total_line;
    struct jpeg_yuv *yuv;
    u8 frame_begin;
    u8 mode;//
    u8 yuv_type;
};

struct jpeg_file {
    const char *name;
};

struct jpeg_data {
    u8 *buf;
    u32 len;
};

struct jpeg_dec_hdl {
    u8 start;
    u32 yuv_size;
    u8 *yuv;
    struct jpeg_decoder_param *param;
    struct jpeg_decode_req *req;
    struct jpeg_decode_image *dimg;
    struct jpeg_decoder_fd *decoder_fd;
    struct jpeg_codec_handle *codec;
};

struct jpeg_decode_req {
    u8 input_type;
    u8 output_type; //0 -- yuv420, 1 -- original mode
    u8 bits_mode; //BITS_MODE_CACHE / BITS_MODE_UNCACHE
    u8 dec_query_mode;//查询法
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
    u16 out_width;
    u16 out_height;
    u16 out_xoffset;
    u16 out_yoffset;
    void *priv;
    void (*stream_end)(void *priv);
};

struct jpeg_image_info {
    union {
        struct jpeg_data data;
    } input;
    int sample_fmt;
    u16 width;
    u16 height;
};

struct jpeg_encode_req {
    u8 format;
    u8 q;
    struct jpeg_data data;
    u8 *y;
    u8 *u;
    u8 *v;
    u16 width;
    u16 height;
};

struct jpeg_codec_handle {
    int id;
    volatile int state;
    int timer;
    JL_JPG_TypeDef *reg;
    /*
    struct jpeg_encoder_fd *encoder_fd;
    struct jpeg_encoder_fd *auto_enc_fd;
    struct jpeg_encoder_fd *manual_enc_fd;
    struct jpeg_decoder_fd *decode_fd;
    */
    struct list_head encoder;
    struct list_head decoder;
    int (*frame_end)(void *priv, void *fb);
    OS_SEM sem;
};

#define jpg_be16_to_cpu(x) (((x)[0] << 8) | (x)[1])


int jpeg_codec_init(void);
/*
 * jpeg encode
 * JPEG编码使用函数
 */
int jpeg_encode_one_image(struct jpeg_encode_req *req);
int jpeg_manual_encode_frame(void *fd, struct jpeg_encode_req *req, u8 wait_complet);
int jpeg_manual_encode_frame_wait_complet(void *fd, struct jpeg_encode_req *req);
int jpeg_manual_encode_frame_close(void *fd);
void *jpeg_manual_encode_frame_init(void);
/*
int mjpg_stream_cap_resume(void *fd);
int mjpg_stream_cap_pause(void *fd);
int mjpg_stream_cap_start(void *fd);
int mjpg_stream_cap_stop(void *fd);
int mjpg_stream_cap_reset(void *fd);
int mjpg_stream_cap_next_frame(void *fd);
int mjpg_get_stream_fnum(void *fd);
int mjpg_image_cap_start(void *fd, struct video_image_enc *info);
int mjpg_capture_stop(void *fd);
int mjpg_capture_close(void *fd, u8 auto_stream);
int mjpg_stream_set_source(void *fd, void *arg);
int mjpg_stream_reset_bits_rate(void *fd, u32 bits_rate);
void *mjpg_image_cap_open(void *arg, int type);
void *mjpg_stream_cap_open(void *arg, int type);
int mjpg_capture_reset_param(void *fd, void *arg);
*/

int mjpg_handl2ch(void *_fh);
void *mjpg_enc_open(void *_info, enum jpeg_enc_mode mode, u8 bfmode);
int mjpg_enc_get_s_attr(void *_fh, struct mjpg_s_attr *attr);
int mjpg_enc_set_s_attr(void *_fh, struct mjpg_s_attr *attr);

int mjpg_enc_get_d_attr(void *_fh, struct mjpg_d_attr *attr);
int mjpg_enc_set_d_attr(void *_fh, struct mjpg_d_attr *attr);

int mjpg_enc_set_output_fps(void *_fh, int fps);

int mjpg_enc_set_input_buf(void *_fh, struct video_cap_buffer *b);

void mjpg_enc_set_output_buf_ops(void *_fh, void *, const struct mjpg_user_ops *ops);

int mjpg_enc_set_irq_handler(void *_fh, void *priv,
                             int (*handler)(void *, enum mjpg_irq_code, enum mjpg_frame_type));
int mjpg_enc_start(void *_fh);

int mjpg_enc_pause(void *fd);

int mjpg_enc_resume(void *fd);

int mjpg_enc_kstart(void *_fh);
int mjpg_enc_mline_kick(void *_fh);

int mjpg_enc_wait_stop(void *_fh);

int mjpg_enc_stop(void *_fh);
int mjpg_enc_stop_force(void *_fh);
int mjpg_start_next_frame(void *_fh);
int mjpg_enc_close(void *_fh);
int mjpg_enc_inirq_stop(void *_fh);
int mjpg_enc_yuv_use_line(void *_fh, int line);
int mjpg_enc_next_line(void *_fh);

void *mjpg_image_enc_open(void *_info, enum jpeg_enc_mode mode);
void *mjpg_image_enc_force_open(void *_info, enum jpeg_enc_mode mode, int id);
int mjpg_image_enc_start(void *_fh, struct YUV_frame_data *input_frame, u8 *bits_buf, int buf_len, u8 q_val);
int mjpg_image_enc_close(void *_fh);
int mjpg_enc_set_thumbnails(void *_fh, void *thumbnail, int size);
void *mjpg_get_auto_stream_encoder(void *_fh);
int mjpg_manu_enc_check_space(void *fd, u32 size);
/*
 * jpeg decode
 * JPEG解码使用函数
 */
//u8 *find_jpg_thumbnails(u8 *buf, int len, int *thm_len);
u8 *find_jpg_frame(u8 *buf, int limit);
void *jpeg_decode_open(void *arg);
int jpeg_decode_reset_param(void *fd, void *arg);
int jpeg_decode_close(void *fd);
int jpeg_decode_reset(void *fd, void *arg);
int jpeg_decode_start(void *fd, u8 *buf, int len);
int jpeg_decode_release(void *fd);
int jpeg_dec_change_omode(void *fd, void *buf, u8 mode);
int jpeg_dec_manual_start(void *fd, u8 *buf, int len);
int jpeg_dec_write_data(void *fd, u8 *data, int len);
int jpeg_dec_set_event_handler(void *fd, void *priv,
                               int (*handler)(void *, enum jpg_dec_event, void *));

int jpeg_yuv_to_yuv420(struct jpeg_yuv *src_yuv, struct jpeg_yuv *dst_yuv, u16 stride, u16 image_w, u16 out_w, u8 yuv_type, u16 lines);
int jpeg_decode_image_info(struct jpeg_image_info *info);
int jpeg_decode_one_image(struct jpeg_decode_req *req);
int jpeg_yuv_to_yuv420(struct jpeg_yuv *src_yuv, struct jpeg_yuv *dst_yuv, u16 stride, u16 image_w, u16 out_w, u8 yuv_type, u16 lines);
int jpeg_decode_cyc(struct jpeg_dec_hdl *jpeg_dec_hdl, u8 *jpg_buf, u32 jpg_size);

#endif

