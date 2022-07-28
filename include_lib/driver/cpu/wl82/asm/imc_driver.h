#ifndef IMC_DRIVER_H
#define IMC_DRIVER_H



#include "typedef.h"
#include "system/includes.h"



//#define IMC_CHANNEL_NUM  	3
#define IMC_ENC_CH_NUM  	2
#define IMC_DISP_CH_NUM  	2

enum imc_src_type {
    IMC_SRC0 = 1,
    IMC_SRC1 = 2,
    IMC_SRC_REP = 3,
};



enum imc_isp_sel {
    IMC_SRC_SEL_ISP0 = 1,
    IMC_SRC_SEL_ISP1,
    IMC_SRC_SEL_MANUAL,

};

enum imc_yuv_mode {
    IMC_YUV420 = 0,
    IMC_YUV422,
};

enum imc_axi_max_len {
    IMC_512B = 0,
    IMC_256B,
};

enum imc_event {
    IMC_EVENT_ERR_IRQ,
    IMC_EVENT_FRAME_IRQ,
    IMC_EVENT_LINE_IRQ,
    IMC_EVENT_PAUSE,
    IMC_EVENT_RESUME,
    IMC_EVENT_RESET,
};

enum imc_irq_mode_e {
    IMC_IRQ_M_DISABLE,
    IMC_IRQ_M_16LINE = 0x1,
    IMC_IRQ_M_FRAME  = 0x2,
    /*
     * 按位增加
     * 0x4,
     * 0x8,
     * ...
     */
};

enum output_mode {
    IMC_ONE_FRAME_MODE = 0,
    IMC_DOUBLE_BUF_MODE,
    IMC_CYCLE_BUF_MODE,
};

struct video_source_crop {
    u16 x_offset;
    u16 y_offset;
    u16 width;
    u16 height;
};

struct imc_s_attr {
    enum imc_src_type src;
    enum imc_isp_sel src_isp_sel;
    u8 down_smp;
    u8 mode;
    u8 camera_type;
    u8 fps;
    u32 real_fps; /*实际帧率(浮点)q16)*/
    u16 src_left;
    u16 src_top;
    u16 src_w;
    u16 src_h;
    u16 width;
    u16 height;
    u16 pixformat;
    u16 max_i_width;
    u16 max_o_width;
    enum imc_irq_mode_e irq_mode;
};


int imc_init(const char *name, const struct video_platform_data *data);

int imc_handl2ch(void *_hdl);

void *imc_enc_ch_open(struct video_format *);

int imc_enc_set_output_buf(void *fh, struct video_cap_buffer *b);

int imc_enc_set_output_module(void *_fh, int ch);

int imc_set_event_handler(void *_fh, void *priv,
                          int (*handler)(void *, enum imc_event, struct video_cap_buffer *));

int imc_enc_reset_buffer(void *_fh);
int imc_enc_reset_down_buffer(void *_fh);
int imc_enc_reset_up_buffer(void *_fh);

int imc_enc_ch_start(void *_fh);
int imc_enc_ch_stop(void *_fh);

int imc_enc_ch_close(void *_fh);

struct imc_d_attr {
    u8 output_fps;
};

int imc_set_d_attr(void *_fh, struct imc_d_attr *attr);

int imc_get_s_attr(void *_fh, struct imc_s_attr *attr);
int imc_set_s_attr(void *_fh, struct imc_s_attr *attr);

int imc_enc_set_output_fps(void *_fh, int fps);

int imc_enc_get_osd(void *_fh, struct video_text_osd *osd);
int imc_enc_set_osd(void *_fh, struct video_text_osd *osd);
int imc_enc_set_osd_enable(void *_fh);
int imc_enc_set_osd_disable(void *_fh);
int imc_enc_ch_pause(void *_fh);
int imc_enc_ch_resume(void *_fh, struct video_cap_buffer *b);
int imc_enc_get_osd_attr(void *_fh, struct video_osd_attr *attr);
int imc_enc_set_osd_attr(void *_fh, struct video_osd_attr *attr);

int imc_enc_ch_get_capability(void *_fh);
int imc_enc_ch_capture_one_frame(void *_fh, struct yuv_image *image);
int imc_enc_ch_image_capture(void *_fh, struct video_image_capture *icap);
int imc_enc_ch_stop_image_cap(void *_fh);
int imc_enc_image_scale(void *_fh, struct image_scale_data *scale_data);
int imc_get_image_text_osd(void *_fh, struct video_text_osd *text_osd);
int imc_enc_pause_image_osd(void *_fh);
int imc_enc_resume_image_osd(void *_fh);
/*
struct imc_manu_i_data {
    u8 proc_wait;
    u8 post_wait;
    struct YUV_frame_data *frame;
};
*/

int imc_dis_ch_max_width();
struct imc_manu_i_data {
    u8 restart_ch;
    u8 proc_wait;
    u8 post_wait;
    struct YUV_frame_data *frame;
};

void *imc_dis_ch_open(struct video_format *);

int imc_get_s_attr(void *, struct imc_s_attr *attr);

int imc_set_s_attr(void *, struct imc_s_attr *attr);

int imc_dis_set_output_buf(void *_fh, u8 *yaddr, u8 *uaddr, u8 *vaddr);

int imc_dis_set_pingpong_buf(void *_fh, struct fb_map_user *map0, struct fb_map_user *map1);

int imc_dis_set_output_module(void *_fh, int ch);

int imc_dis_ch_input_data(void *_fh, struct imc_manu_i_data *i_data);

int imc_set_irq_handler(void *_fh, void *priv, int (*handler)(void *));

int imc_dis_ch_start(void *_fh);

int imc_dis_ch_stop(void *_fh);

int imc_dis_ch_close(void *_ch);

int imc_manu_src_input_data(void *_fh, struct imc_manu_i_data *i_data);
int imc_dis_ch_input_data(void *_fh, struct imc_manu_i_data *i_data);


























#endif

