#ifndef DEVICE_VIDEO_H
#define DEVICE_VIDEO_H

#include "video/video_ioctl.h"
#include "device/device.h"


#define VIDEO_DEFAULT_BUF_MODE	0x0
#define VIDEO_PPBUF_MODE		0x1
#define VIDEO_WL80_SPEC_PICTURE_MODE		BIT(0)
#define VIDEO_WL80_SPEC_DOUBLE_REC_MODE		BIT(1)
#define VIDEO_WL80_SPEC_720P_REC_MODE		BIT(2)

#define VIDEO_TAG(a,b,c,d)  (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))


enum {
    VIDREQ_GET_OVERLAY_BUFFER,
    VIDREQ_PUT_OVERLAY_BUFFER,
    VIDREQ_FREE_OVERLAY_BUFFER,
    VIDREQ_GET_CAPTURE_BUFFER,
    VIDREQ_SET_CAPTURE_BUFFER,
    VIDREQ_PUT_CAPTURE_BUFFER,
    VIDREQ_FREE_CAPTURE_BUFFER,
    VIDREQ_GET_IMAGE_CAPTURE_BUFFER,
    VIDREQ_PUT_IMAGE_CAPTURE_BUFFER,
    VIDREQ_IMAGE_CAPTURE_COMPLETED,
    VIDREQ_IMAGE_ZOOM, // 10
    /*VIDREQ_IMAGE_DISP_ZOOM,*/
    VIDREQ_IMAGE_ENC_START,
    /*VIDREQ_GET_DISP_CAPABILITY,*/
    VIDREQ_GET_ENCODER_CHANNEL,
    /*VIDREQ_PUT_IMAGE_TO_BUFFER, // 15*/
    /*VIDREQ_RESET_DISP_IMAGE,*/
    VIDREQ_RESET_CBUFFER,
    /*VIDREQ_PUT_DEC_DATA,*/
    VIDREQ_GET_DEC_DATA,
    VIDREQ_RESET_DOWN_CBUFFER, //20
    VIDREQ_RESET_UP_CBUFFER,
    VIDREQ_RESET_ENCODER,
    VIDREQ_REKSTART_ENCODER,
    VIDREQ_WAIT_ENC_END_STOP_IMC,
    VIDREQ_GET_FNUM,
    VIDREQ_PAUSE_ENC,
    VIDREQ_CONTINUE_ENC,
    VIDREQ_PAUSE_ENC_CHANNEL,
    VIDREQ_CONTINUE_ENC_CHANNEL,
    VIDREQ_GET_TIME_LABEL,
    VIDREQ_SET_SOFT_IMAGE_LABEL,
    VIDREQ_CAMERA_OUT,
    VIDREQ_RESET_UVC_ENC,
    VIDREQ_ENCODE_ONE_IMAGE,
    VIDREQ_SET_JPG_THUMBNAILS,
    VIDREQ_ASYNC_PUT_DEC_DATA,
    VIDREQ_BUFF_MOVE,
    VIDREQ_GET_MANUAL_ENC_YUV,
    VIDREQ_MANUAL_ENC_STREAM,
    VIDREQ_GET_SRC_FMT,
    VIDREQ_FLUSHINV_DEC_DATA,
    VIDREQ_INSERT_WATERMARKIGN,
    VIDREQ_MANUAL_DISP_PUTMAP,
    VIDREQ_KICK_UP_ONE_FRMAE,
    VIDREQ_GET_CORRECT_SIZE,
};

enum video_rec_quality {
    VIDEO_LOW_Q,
    VIDEO_MID_Q = 6,
    VIDEO_HIG_Q = 12,
};

enum video_camera_type {
    VIDEO_CAMERA_NORMAL = 0,
    VIDEO_CAMERA_UVC,
    VIDEO_CAMERA_VIRTUAL,
    VIDEO_CAMERA_JLC,
};

struct YUV_frame_data {
    u16 width;
    u16 height;
    u16 line_num;
    u16 data_height;
    int pixformat;
    u8  *y;
    u8  *u;
    u8  *v;
};

struct video_frame_yuv {
    u16 width;
    u16 height;
    u8 *y;
    u8 *u;
    u8 *v;
};

struct video_yuv_buffer {
    u8 *y;
    u8 *u;
    u8 *v;
};

struct image_scale_data {
    u8  zstate;
    u32 src_reg_data;
    u16 out_width;
    u16 out_height;
    u8 *output_buf;
    struct YUV_frame_data input_frame;
};

struct video_image_enc {
    u8  mkstart;
    u16 img_width;
    u16 img_height;
    u16 blk_width;
    u16 blk_height;
    struct video_yuv_buffer *blk_buf;
};

struct video_image_dec {
    u8  fkstart;
    u16 width;
    u16 height;
    u8  *buf;
    u32 size;
    struct video_yuv_buffer *blk_buf;
};

struct video_decode_frame {
    u8 type;
    unsigned int reset : 1;
    unsigned int mode : 7; //0 -- display, 1 -- encode
    u16 width;
    int line;
    u8 *buf;
    u8 *circle_y;
    u8 *circle_u;
    u8 *circle_v;
    int cnt;
};

struct video_encode_frame {
    u8 type;
    u8 reset;
    u8 *buf;
    int cnt;
    int lines;
};

struct video_cap_buffer {
    u8 num;
    u32 size;
    u8 *static_buf;
    u8 *buf;
    u8 *buf2;
    u8 ch_num;
};


struct video_subdevice;
struct video_var_param_info;

struct video_subdevice_data {
    int tag;
    void *data;
};

struct video_platform_data {
    int num;
    const struct video_subdevice_data *data;
};

struct roi_cfg {
    u32 roio_xy;
    u32 roi1_xy;
    u32 roi2_xy;
    u32 roi3_xy;
    u32 roio_ratio;
    u32 roio_ratio1;
    u32 roio_ratio2;
    u32 roio_ratio3;
    u32 roio_config;
};

struct jpg_q_table {
    u16  YQT_DCT[0x40] ;
    u16  UVQT_DCT[0x40];
    u8   DQT[138]; //file header
};

enum video_pix_format {
    VID_PIX_FMT_H264,
    VID_PIX_FMT_MJPG,
    VID_PIX_FMT_IMG,
    VID_PIX_FMT_YUV,
};

enum image_pix_format {
    IMG_PIX_FMT_JPG,
    IMG_PIX_FMT_YUV,
};

struct h264_user_data {
    u8 *data;
    u32 len;
};

struct h264_s_attr {
    u8 quality;
    u8 IP_interval;
    u8 std_head;
    u16 width;
    u16 height;
    /*u32 i_bitrate;*/
};

struct h264_d_attr {
    u32 abr_kbps;
    struct roi_cfg roi;
};

struct mjpg_s_attr {
    u8 enc_mode;//0 -- 整帧， 1 -- 帧分段
    u8 quality;
    u8 samp_fmt;
    u8 source;
    int head_len;
    u16 width;
    u16 height;
    u32 abr_kbps;
    u32 div_size;
};

struct mjpg_d_attr {
    struct jpg_q_table *qt;
    u8 *thumbnails;
    int thumb_len;
};

struct icap_auxiliary_mem {
    u8 *addr;
    u32 size;
};

struct image_s_attr {
    u8   quality;
    u16  width;
    u16  height;
    enum image_pix_format format;
    int    aligned_width;
    struct jpg_q_table *jpg_qt;
    struct icap_auxiliary_mem *aux_mem;
    struct jpg_thumbnail *thumb;
};

struct jpg_dec_mem {
    u8 *addr;
    u32 size;
};

struct jpg_dec_s_attr {
    u16 max_o_width;
    u16 max_o_height;
    struct jpg_dec_mem dec_mem;
};

struct video_enc_attr {
    enum video_pix_format format;
    void *attr;
};

struct video_dec_attr {
    enum video_pix_format format;
    void *attr;
};

struct yuv_image {
    u8 format;
    u16 width;
    u16 height;
    u8 *addr;
    u32 size;
};

struct video_dev_fps {
    u8 camera_fps;
    u8 real_fps;
    u8 target_fps;
};

struct video_osd {

    u16 x ;//起始地址
    u16 y ;//结束地址
    u32 osd_yuv;//osd颜色

//注意：下面的字符串地址必须是全局的,然后年是yyyy，月是nn，日是dd，时是hh，分是mm，秒是ss,其他字符是英文字母&&符号&&汉字
    char *osd_str; //用户自定义格式，例如 "yyyy-nn-dd\hh:mm:ss" 或者 "hh:mm:ss"
    char *osd_matrix_str; //用户自定义字模字符串,例如“abcd....0123..”
    u8 *osd_matrix_base; //用户自定义字模的起始地址
    u32 osd_matrix_len;//用户自定义字模数组的长度,no str len!!!
    u8 osd_w;//用户自定义字体大小,8的倍数
    u8 osd_h;//8的倍数
};

struct image_capture_info {
    u8 multi_scale;
    struct video_image_capture *icap;
};

#if 1
enum video_osd_mode {
    VID_OSD_TEXT = 0,
    VID_OSD_GRAPH,
};

enum video_osd_type {
    VIDEO_STREAM_OSD,
    VIDEO_IMAGE_OSD,
};

struct video_text_osd {
    u16     x;//起始地址
    u16     y;//结束地址
    u32     osd_yuv;//osd颜色
//注意：下面的字符串地址必须是全局的,然后年是yyyy，月是nn，日是dd，时是hh，分是mm，秒是ss,其他字符是英文字母&&符号&&汉字
    char    *text_format; //用户自定义格式，例如 "yyyy-nn-dd\hh:mm:ss" 或者 "hh:mm:ss"
    char    *font_matrix_table; //用户自定义字模字符串,例如“abcd....0123..”
    u8      *font_matrix_base; //用户自定义字模的起始地址
    u32     font_matrix_len;//用户自定义字模数组的长度,no str len!!!
    u8      font_w;//用户自定义字体大小,8的倍数
    u8      font_h;//8的倍数
    u8      direction; //0 -- 顺向, 1 -- 逆向
};

struct video_graph_osd {
    u8      bit_mode; //2 - 2bit, 16 - 16bit
    u16     x;
    u16     y;
    u32     color[3]; //2bit图像yuv颜色配置
    u16     width;
    u16     height;
    u8      *icon; //图形内容buffer
    int     icon_size;
};

struct video_osd_config {
    struct video_text_osd text_osd;
    struct video_graph_osd icon_osd;
};

struct video_osd_attr {
    u32 enable;
    u32 ability;
    enum video_osd_type type;
    struct video_text_osd *text_osd;
    struct video_graph_osd *graph_osd;
};
#endif

#define VIDEO_REC_NUM       2       //double rec or single rec

#define VIDEO_PLATFORM_DATA_BEGIN(vdata) \
	static const struct video_platform_data vdata = { \

#define VIDEO_PLATFORM_DATA_END() \
	};

struct image_capability {
    u8  zoom;
    u16 width;
    u16 height;
    struct video_image_capture *icap;
};

struct video_subdevice_ops {
    int (*init)(const char *name, const struct video_platform_data *);

    bool (*online)(const char *name);

    int (*get_fmt)(struct video_format *f);

    int (*set_fmt)(struct video_format *f);

    struct video_endpoint *(*open)(struct video_var_param_info *);

    int (*overlay)(struct video_endpoint *, int i);

    int (*streamon)(struct video_endpoint *);

    int (*streamoff)(struct video_endpoint *);

    int (*get_image_capability)(struct video_endpoint *, struct image_capability *);
    int (*image_capture)(struct video_endpoint *, struct image_capability *);

    int (*response)(struct video_endpoint *, int cmd, void *);

    int (*write)(struct video_endpoint *, void *buf, u32 len);

    int (*close)(struct video_endpoint *);

    int (*querycap)(struct video_endpoint *, struct video_capability *cap);

    int (*ioctrl)(struct video_endpoint *, u32 cmd, void *arg);
};


#define MANUAL_CHANNEL  3
struct video_var_param_info {
    u32 fps;
    u16 width;
    u16 height;

    u8 ch;
    u8 channel;
    int source;
    struct video_format *f;
    void *priv;
};

struct video_subdevice {
    u8 subid;
    const char *name;
//    u32 input_pixelformat;
    u32 output_pixelformat;
    const struct video_subdevice_ops *ops;
    int (*request)(struct video_endpoint *, int cmd, void *);
};

struct video_endpoint {
    struct list_head entry;
    struct video_subdevice *dev;
    int inused;
    void *parent;
    void *private_data;
    void *imc_data;
};

struct video_crop_ctrl {
    u16 crop_sx;
    u16 crop_ex;
    u16 crop_sy;
    u16 crop_ey;
};

struct video_crop_sca {
    u16 src_w;
    u16 src_h;
    u16 crop_w;
    u16 crop_h;
};

struct video_crop_tri {
    int (*do_crop)(void *priv, void *parm);
    void *priv;
    void *parm;
};


extern const struct video_subdevice video_subdev_begin[];
extern const struct video_subdevice video_subdev_end[];

extern const struct device_operations video_dev_ops;
extern const struct device_operations usb_cam_dev_ops;

#define list_for_each_video_subdevice(dev) \
	for (dev=video_subdev_begin; dev<video_subdev_end; dev++)

#define REGISTER_VIDEO_SUBDEVICE(dev, id) \
const struct video_subdevice dev SEC_USED(.video_subdev.##id) = { \
		.subid = id, \
        .name = #dev, \
        .request = video_dec_request, \

struct video_device;

struct video_device_ops {
    const char *name;
    bool (*online)();
    int (*init)(const char *name, const struct video_platform_data *data);
    int (*open)(struct video_device *);
    int (*set_fmt)(struct video_device *, struct video_format *);
    int (*overlay)(struct video_device *, int on);
    int (*streamon)(struct video_device *);
    int (*streamoff)(struct video_device *);
    int (*image_capture)(struct video_device *, struct video_image_capture *icap);
    int (*start_I_frame)(struct video_device *);
    int (*adjust_fps)(struct video_device *, void *);
    int (*ioctl)(struct video_device *, int cmd, u32 arg);
    int (*write)(struct video_device *, void *data, u32 len);
    int (*close)(struct video_device *);
};

extern const struct video_device_ops video_dev_begin[];
extern const struct video_device_ops video_dev_end[];

#define REGISTER_VIDEO_DEVICE(dev, _name) \
	static const struct video_device_ops __device_##dev SEC_USED(.video_device) = { \
        .name = _name, \

struct video_fill_frames {
    u8    reset_timer;
    u8    time_cnt;
    char  fps_remain;
    spinlock_t lock;
    volatile int   o_frame_cnt;
    short interval;
    short fill_dly;
    int   lost_frames;
    int   timer;
    int   timer_one_frame;
};

struct video_device {
    struct device device;
    struct list_head endpoint;
    struct list_head entry;
    struct videobuf_queue video_q;
    struct video_var_param_info info;
    struct video_subdevice *subdev[4];
    struct video_image_capture *icap;
    const struct video_device_ops *dev_ops;
    struct video_dev_fps fps;
    struct video_fill_frames ff;
    void   *private_data;
    OS_SEM sem;
    u8 ref;
    u8 time_base;
    u8 streamon;
    u8 subdev_num;
    u8 major, mijor;
    u32 frame_cnt;
    u32 stop_frame_cnt;
    u32 stop_frame_interval;
    u32 pixelformat;
    u16 src_width;
    u16 src_height;

    u8 bfmode;//输出BUFF模式 , pingpong_buffer lbuffer
    u8 picture_mode;
    u32 wl80_spec_mode;//wl80硬件限制的特殊模式
    void *priv_wl80_spec;
    u8 jpeg_yuv_format;
    void *ppbuf;
    void *isc_sbuf;
    u8 isc_ppbuf;
    u32 sbuf_size;
    int (*block_done_cb)(void *info);
};

int video_subdevice_register(struct video_subdevice *dev);

u32 video_buf_free_space(struct video_device *);

void *video_buf_malloc(struct video_device *, u32 size);

void *video_buf_realloc(struct video_device *, void *buf, int size);

void video_buf_free(struct video_device *, void *buf);

void *video_buf_ptr(void *buf);

u32 video_buf_size(void *buf);

void video_buf_stream_finish(struct video_device *, void *buf);

int video_buf_query(struct video_device *ep, struct videobuf_state *sta);

int video_subdev_request(struct video_endpoint *ep, int req, void *arg);

int video_dev_need_fill_frame(struct video_device *);

void video_dev_real_frame_dec(struct video_device *);

void video_dev_reset_frame_interval_timer(struct video_device *);

int video_dec_request(struct video_endpoint *ep, int req, void *arg);

#endif

