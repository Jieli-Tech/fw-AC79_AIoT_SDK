#ifndef  __HOST_UVC_H__
#define  __HOST_UVC_H__

#include "asm/usb.h"
#include "uvc.h"
#include "asm/uvc_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------
 * GUIDs
 */
#define UVC_GUID_UVC_CAMERA	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01}
#define UVC_GUID_UVC_OUTPUT	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02}
#define UVC_GUID_UVC_PROCESSING	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01}
#define UVC_GUID_UVC_SELECTOR	{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02}

#define UVC_GUID_UNIT_EXTENSION 0x92, 0x42, 0x39, 0x46, 0xD1, 0x0C, 0xE3, 0x4A, 0x87, 0x83, 0x31, 0x33, 0xF9, 0xEA, 0xAA, 0x3B
#if 0
#define UVC_GUID_UNIT_EXTENSION     0xAD, 0xCC, 0xB1, 0xC2,\
    0xF6, 0xAB, 0xB8, 0x48,\
    0x8E, 0x37, 0x32, 0xD4,\
    0xF3, 0xA3, 0xFE, 0xEC
#endif
#define UVC_GUID_FORMAT_MJPEG	'M',  'J',  'P',  'G', 0x00, 0x00, 0x10, 0x00, \
    0x80, 0x00, 0x00, 0xaa,0x00, 0x38, 0x9b, 0x71

#define UVC_GUID_FORMAT_YUY2	'Y',  'U',  'Y',  '2', 0x00, 0x00, 0x10, 0x00, \
    0x80, 0x00, 0x00, 0xaa,0x00, 0x38, 0x9b, 0x71

#define UVC_GUID_FORMAT_NV12	'N',  'V',  '1',  '2', 0x00, 0x00, 0x10, 0x00, \
    0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71


#define CAMERA_CLOSE_BY_IRQ     1


struct uvc_event_listener {
    void *priv;
    int (*handler)(void *priv, int event);
};

struct uvc_int_endponit {
    u8 enable;
    int host_ep;
    int epin;
    u32 interval;
    u32 wMaxPacketSize;
    u8 *buffer;
};

struct usb_uvc {
    u8  host_ep;
    u8  ep;
    u16 if_alt_num;
    u32 wMaxPacketSize;
    u32 interval;
    u8 *ep_buffer;
    u32 fps;
    u32 bfh: 1;
    u32 error_frame: 1;
    u32 cur_frame: 8;
    u32 mjpeg_format_index: 8;
    u32 reso_num: 8;
    u32 camera_open: 1;
    u32 xfer_type: 2;
    u32 ep_buf_ch: 1;
    u32 unused: 2;
    void *priv;
    struct uvc_int_endponit int_ep;
    UVC_STREAM_OUT stream_out;
#if !CAMERA_CLOSE_BY_IRQ
    UVC_STREAM_OUT bak_stream_out;
#endif
    int (*offline)(void *priv);
    void *event_priv;
    int (*event_handler)(void *priv, int event);
    struct uvc_frame_info pframe[0];
};
#define UVC_FORMAT_YUY2     0
#define UVC_FORMAT_NV12     0
#define UVC_FORMAT_MJPG     1



#if 0
#define FREAME_MAX_WIDTH        1280
#define FREAME_MAX_HEIGHT       720
#else
#define FREAME_MAX_WIDTH        640
#define FREAME_MAX_HEIGHT       480
#endif

#define UVC_FRAME_SZIE      (FREAME_MAX_WIDTH * FREAME_MAX_HEIGHT * 2)

#define MJPG_WWIDTH_0     (0x500)
#define MJPG_WHEIGHT_0    (0x2D0)

#define MJPG_WWIDTH_1     (0x0320)
#define MJPG_WHEIGHT_1    (0x0258)

#define MJPG_WWIDTH_2     (0x0280)
#define MJPG_WHEIGHT_2    (0x01E0)

#define MJPG_WWIDTH_3     (0x0160)
#define MJPG_WHEIGHT_3    (0x0120)

#define MJPG_WWIDTH_4     (0x0140)
#define MJPG_WHEIGHT_4    (0x00F0)

#define YUV_WWIDTH_0     (0x500)
#define YUV_WHEIGHT_0    (0x2D0)

#define YUV_WWIDTH_1    (0x0280)
#define YUV_WHEIGHT_1    (0x01E0)

#define YUV_WWIDTH_2     (0x0320)
#define YUV_WHEIGHT_2    (0x0258)

#define YUV_WWIDTH_3     (0x0160)
#define YUV_WHEIGHT_3    (0x0120)

#define YUV_WWIDTH_4     (0x0140)
#define YUV_WHEIGHT_4    (0x00F0)

#define FRAME_FPS_0         (10000000/30)
#define FRAME_FPS_1         (10000000/30)
#define FRAME_FPS_2         (10000000/30)
#define FRAME_FPS_3         (10000000/30)
#define FRAME_FPS_4         (10000000/30)

#if UVC_FORMAT_YUY2||UVC_FORMAT_NV12
#define ISO_EP_INTERVAL     1
#else
#define ISO_EP_INTERVAL     1
#endif

#define UVC_DWCLOCKFREQUENCY    0x01c9c380

#define UVC_IT_TERMINALID       1
#define UVC_OT_TERMINALID       2
#define UVC_PU_TERMINALID       3
#define UVC_EXT_TERMINALID      4



///////////Video Class
/* USB VIDEO INTERFACE TYPE */
#define VIDEO_CTL_INTFACE_TYPE         1
#define VIDEO_STEAM_INTFACE_TYPE       2

/* Video BFH */
#define UVC_FID     0x01
#define UVC_EOF     0x02
#define UVC_PTS     0x04
#define UVC_SCR     0x08
#define UVC_RES     0x10
#define UVC_STI     0x20
#define UVC_ERR     0x40
#define UVC_EOH     0x80

enum stream_state {
    STREAM_NO_ERR = 0x0,
    STREAM_ERR = 0x1,
    STREAM_EOF = 0x2,
    STREAM_SOF = 0x3,
};

#define 	VIDEO_INF_NUM                   2

#define SINGLE_UVC	1

//#define ISO_INTR_MODE 0 //1 iso_intr_cnt
#define ISO_INTR_MODE 1 //1 iso_intr_cnt
//#define ISO_INTR_MODE 2 //2 iso_intr_cnt

#if (ISO_INTR_MODE == 2)
#if (SINGLE_UVC== 1)
#define ISO_INTR_CNT	8
#else
#define ISO_INTR_CNT	4
#endif
#else
#define ISO_INTR_CNT	1
#endif

extern int usb_uvc_parser(struct usb_host_device *host_dev, u8 interface_num, u8 *pBuf);
extern void *uvc_host_open(void *arg);
extern int uvc_force_reset(void *fd);
extern int uvc_host_close(void *fd);
extern int uvc_host_open_camera(void *fd);
extern int uvc_host_close_camera(void *fd);
extern int uvc_host_online(void);
extern int uvc_host_get_backlight_compensation(void *fd, u32 *iostatus);
extern int uvc_host_req_processing_unit(void *fd, struct uvc_processing_unit *pu);
extern int uvc_host_get_pix_table(void *fd, struct uvc_frame_info **pix_table);
extern int uvc_host_set_pix(void *fd, int index);
extern int uvc_host_camera_out(const usb_dev usb_id);
extern int uvc2usb_ioctl(void *fd, u32 cmd, void *arg);
extern int uvc_host_get_fps(void *fd);
//extern int uvc_get_device_id(void *fd, struct usb_device_id *id);
extern int usb_host_video_init(const usb_dev usb_id);
#ifdef __cplusplus
}
#endif




#endif  /*HOST_UVC_H*/
