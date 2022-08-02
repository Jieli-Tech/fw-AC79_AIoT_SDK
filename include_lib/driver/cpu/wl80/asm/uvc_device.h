/*****************************************************************
>file name : uvc_device.h
>author : lichao
>create time : Sat 02 Sep 2017 03:22:12 PM HKT
*****************************************************************/
#ifndef _UVC_DEVICE_H_
#define _UVC_DEVICE_H_
#include "video/video.h"
#include "video/camera.h"

#define UVC_CMD_BASE        0x00010000

#define UVC_SET_CUR_FPS         (UVC_CMD_BASE + 0)
#define UVC_GET_CUR_FPS         (UVC_CMD_BASE + 1)
#define UVC_GET_CUR_BITS_RATE   (UVC_CMD_BASE + 2)

#define UVC_CAMERA_FMT_YUY2     0x1
#define UVC_CAMERA_FMT_MJPG     0x2
#define UVC_CAMERA_FMT_H264     0x3

#define USBIOC_MASS_STORAGE_CONNECT     _IOW('U', 0, sizeof(struct usb_mass_storage))
#define USBIOC_UVC_CAMERA_CONNECT      _IOW('U', 1, sizeof(struct uvc_format))
#define USBIOC_UAC_MICROPHONE_CONNECT      _IOW('U', 2, sizeof(struct uvc_format))
//#define USBIOC_UVC_CAMERA1_CONNECT      _IOW('U', 2, sizeof(struct uvc_format))
#define USBIOC_SLAVE_MODE_START         _IOW('U', 3, sizeof(unsigned int))
#define USBIOC_SLAVE_DISCONNECT         _IOW('U', 4, sizeof(unsigned int))
#define UVCIOC_QUERYCAP                 _IOR('U', 5, sizeof(struct uvc_capability))
#define UVCIOC_SET_CAP_SIZE             _IOW('U', 5, sizeof(unsigned int))
#define UVCIOC_STREAM_ON                _IOW('U', 7, sizeof(unsigned int))
#define UVCIOC_STREAM_OFF               _IOW('U', 8, sizeof(unsigned int))
#define UVCIOC_REQBUFS                  _IOW('U', 9, sizeof(unsigned int))
#define UVCIOC_DQBUF                    _IOW('U', 10, sizeof(unsigned int))
#define UVCIOC_QBUF                     _IOW('U', 11, sizeof(unsigned int))
#define UVCIOC_RESET                    _IOW('U', 12, sizeof(unsigned int))
#define UVCIOC_REQ_PROCESSING_UNIT      _IOR('U', 13, sizeof(struct uvc_processing_unit))
#define UVCIOC_SET_PROCESSING_UNIT      _IOW('U', 14, sizeof(struct uvc_processing_unit))
#define UVCIOC_GET_DEVICE_ID            _IOR('U', 15, sizeof(struct usb_device_id))

#define USBIOC_SLAVE_RESET              _IOW('U', 16, sizeof(unsigned int))

/***********2018-06-21************/
#define USBIOC_GET_DEVICE_ID            _IOR('U', 17, sizeof(struct usb_device_id))
#define USBIOC_GET_MANUFACTURER         _IOR('U', 18, sizeof(struct usb_string))
#define USBIOC_GET_PRODUCT_NAME         _IOR('U', 19, sizeof(struct usb_string))
/*********************************/
/***********2018-07-30************/
#define UVCIOC_SET_EVENT_LISTENER       _IOW('U', 19, sizeof(struct uvc_event_listener))
/*********************************/

#define USBIOC_HID_CONNECT              _IOW('U', 20, sizeof(struct usb_hid_arg))
#define USBIOC_HID_CONTROL              _IOW('U', 21, sizeof(struct usb_hid_arg))
#define USBIOC_CDC_CONNECT              _IOW('U', 22, sizeof(struct usb_cdc_arg))
#define USBIOC_CDC_CONTROL              _IOW('U', 23, sizeof(unsigned int))

/***********2020-05-11，设置uvc摄像头黑白切换************/
#define UVCIOC_SET_CUR_GRAY             _IOW('U', 24, sizeof(unsigned int))
#define UVCIOC_GET_IMAMGE               _IOR('U', 24, sizeof(unsigned int))
#define UVCIOC_SET_CUR_FPS              _IOW('U', 25, sizeof(unsigned int))
/*********************************/

/* ------------------------------------------------------------------------
 * Driver specific constants.
 */
typedef int (*UVC_STREAM_OUT)(void *, int, void *, int);
struct uvc_parm {
    s16 brightness_min;
    s16 brightness_max;
    s16 brightness_def;
    s16 brightness_res;
    s16 brightness_cur;

    s16 contrast_min;
    s16 contrast_max;
    s16 contrast_def;
    s16 contrast_res;
    s16 contrast_cur;

    s16 hue_min;
    s16 hue_max;
    s16 hue_def;
    s16 hue_res;
    s16 hue_cur;

    s16 saturation_min;
    s16 saturation_max;
    s16 saturation_def;
    s16 saturation_res;
    s16 saturation_cur;

    s16 sharpness_min;
    s16 sharpness_max;
    s16 sharpness_def;
    s16 sharpness_res;
    s16 sharpness_cur;

    s16 gamma_min;
    s16 gamma_max;
    s16 gamma_def;
    s16 gamma_res;
    s16 gamma_cur;

    s16 white_balance_temp_min;
    s16 white_balance_temp_max;
    s16 white_balance_temp_def;
    s16 white_balance_temp_res;
    s16 white_balance_temp_cur;

    s16 power_line_freq_min;
    s16 power_line_freq_max;
    s16 power_line_freq_def;
    s16 power_line_freq_res;
    s16 power_line_freq_cur;
};
struct uvc_host_param {
    char *name;
    void *priv;
    UVC_STREAM_OUT uvc_stream_out;
    int (*uvc_out)(void *priv);
};

struct uvc_stream_list {
    void *addr;
    u32 length;
};
struct uvc_frame_info {
    u16 width;
    u16 height;
};
struct uvc_reqbufs {
    void *buf;
    int size;
};

struct uvc_capability {
    int fmt;
    int fps;
    int reso_num;
    struct uvc_frame_info reso[8];
};

struct uvc_processing_unit {
    u8 request;
    u8 type;
    u16 value;
    u16 index;
    u8 buf[4];
    int len;
};
enum trans_mode {
    UVC_PUSH_PHY_MODE,
    UVC_PUSH_VIRTUAL_MODE,
};

struct usb_camera_info {
    u16 width;
    u16 height;
    int fps;
    int sample_fmt;
    enum trans_mode  mode;
};

struct uvc_platform_data {
    u16 width;
    u16 height;
    int fps;
    int fmt;
    int mem_size;
    int timeout;
    u8  put_msg;
};

#define UVC_PLATFORM_DATA_BEGIN(data) \
    static const struct uvc_platform_data data = {\

#define UVC_PLATFORM_DATA_END()  \
};
extern const struct device_operations uvc_dev_ops;

int uvc_get_src_pixformat(void *fh);
int uvc_set_output_buf(void *fh, void *buf, int size);
void set_uvc_gray(u32 flag);
void *uvc_output_open(u8 mijor, struct camera_device_info *info);
int uvc_output_set_fmt(void *fh, struct video_format *f);
int uvc_get_real_fps(void *fh);
int uvc_output_one_frame(void *fh);
int uvc_output_stop(void *fh);
int uvc_output_close(void *fh);
int uvc_set_output_buffer(void *fh, void *buf, int num);
int uvc_set_scaler_handler(void *fh, void *priv, int (*handler)(void *, struct YUV_frame_data *));
int uvc_output_set_reso(void *fh, struct video_format *f, u16 *width, u16 *height);
#endif
