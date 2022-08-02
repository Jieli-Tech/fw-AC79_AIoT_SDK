#ifndef _USB_VIDEO_H_
#define _USB_VIDEO_H_

#include "host/uvc.h"
#include "asm/usb.h"

#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 * UVC constants
 */

#define SC_UNDEFINED                    0x00
#define SC_VIDEOCONTROL                 0x01
#define SC_VIDEOSTREAMING               0x02
#define SC_VIDEO_INTERFACE_COLLECTION   0x03


/* VideoControl class specific interface descriptor */
#define VC_DESCRIPTOR_UNDEFINED         0x00
#define VC_HEADER                       0x01
#define VC_INPUT_TERMINAL               0x02
#define VC_OUTPUT_TERMINAL              0x03
#define VC_SELECTOR_UNIT                0x04
#define VC_PROCESSING_UNIT              0x05
#define VC_EXTENSION_UNIT               0x06

/* VideoStreaming class specific interface descriptor */
#define VS_UNDEFINED                    0x00
#define VS_INPUT_HEADER                 0x01
#define VS_OUTPUT_HEADER                0x02
#define VS_STILL_IMAGE_FRAME            0x03
#define VS_FORMAT_UNCOMPRESSED          0x04
#define VS_FRAME_UNCOMPRESSED           0x05
#define VS_FORMAT_MJPEG                 0x06
#define VS_FRAME_MJPEG                  0x07
#define VS_FORMAT_MPEG2TS               0x0a
#define VS_FORMAT_DV                    0x0c
#define VS_COLORFORMAT                  0x0d
#define VS_FORMAT_FRAME_BASED           0x10
#define VS_FRAME_FRAME_BASED            0x11
#define VS_FORMAT_STREAM_BASED          0x12

/* Endpoint type */
#define EP_UNDEFINED                    0x00
#define EP_GENERAL                      0x01
#define EP_ENDPOINT                     0x02
#define EP_INTERRUPT                    0x03


/* VideoControl interface controls */
#define VC_CONTROL_UNDEFINED            0x00
#define VC_VIDEO_POWER_MODE_CONTROL     0x01
#define VC_REQUEST_ERROR_CODE_CONTROL   0x02

/* Terminal controls */
#define TE_CONTROL_UNDEFINED            0x00

/* Selector Unit controls */
#define SU_CONTROL_UNDEFINED            0x00
#define SU_INPUT_SELECT_CONTROL         0x01

/* Camera Terminal controls */
#define CT_CONTROL_UNDEFINED            		0x00
#define CT_SCANNING_MODE_CONTROL        		0x01
#define CT_AE_MODE_CONTROL              		0x02
#define CT_AE_PRIORITY_CONTROL          		0x03
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL               0x04
#define CT_EXPOSURE_TIME_RELATIVE_CONTROL               0x05
#define CT_FOCUS_ABSOLUTE_CONTROL       		0x06
#define CT_FOCUS_RELATIVE_CONTROL       		0x07
#define CT_FOCUS_AUTO_CONTROL           		0x08
#define CT_IRIS_ABSOLUTE_CONTROL        		0x09
#define CT_IRIS_RELATIVE_CONTROL        		0x0a
#define CT_ZOOM_ABSOLUTE_CONTROL        		0x0b
#define CT_ZOOM_RELATIVE_CONTROL        		0x0c
#define CT_PANTILT_ABSOLUTE_CONTROL     		0x0d
#define CT_PANTILT_RELATIVE_CONTROL     		0x0e
#define CT_ROLL_ABSOLUTE_CONTROL        		0x0f
#define CT_ROLL_RELATIVE_CONTROL        		0x10
#define CT_PRIVACY_CONTROL              		0x11

/* Processing Unit controls */
#define PU_CONTROL_UNDEFINED            		0x00
#define PU_BACKLIGHT_COMPENSATION_CONTROL               0x01
#define PU_BRIGHTNESS_CONTROL           		0x02
#define PU_CONTRAST_CONTROL             		0x03
#define PU_GAIN_CONTROL                 		0x04
#define PU_POWER_LINE_FREQUENCY_CONTROL 		0x05
#define PU_HUE_CONTROL                  		0x06
#define PU_SATURATION_CONTROL           		0x07
#define PU_SHARPNESS_CONTROL            		0x08
#define PU_GAMMA_CONTROL                		0x09
#define PU_WHITE_BALANCE_TEMPERATURE_CONTROL            0x0a
#define PU_WHITE_BALANCE_TEMPERATURE_AUTO_CONTROL       0x0b
#define PU_WHITE_BALANCE_COMPONENT_CONTROL              0x0c
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL         0x0d
#define PU_DIGITAL_MULTIPLIER_CONTROL   		0x0e
#define PU_DIGITAL_MULTIPLIER_LIMIT_CONTROL             0x0f
#define PU_HUE_AUTO_CONTROL             		0x10
#define PU_ANALOG_VIDEO_STANDARD_CONTROL                0x11
#define PU_ANALOG_LOCK_STATUS_CONTROL   		0x12

#define LXU_MOTOR_PANTILT_RELATIVE_CONTROL		0x01
#define LXU_MOTOR_PANTILT_RESET_CONTROL			0x02

/* VideoStreaming interface controls */
#define VS_CONTROL_UNDEFINED            0x00
#define VS_PROBE_CONTROL                0x01
#define VS_COMMIT_CONTROL               0x02
#define VS_STILL_PROBE_CONTROL          0x03
#define VS_STILL_COMMIT_CONTROL         0x04
#define VS_STILL_IMAGE_TRIGGER_CONTROL  0x05
#define VS_STREAM_ERROR_CODE_CONTROL    0x06
#define VS_GENERATE_KEY_FRAME_CONTROL   0x07
#define VS_UPDATE_FRAME_SEGMENT_CONTROL 0x08
#define VS_SYNC_DELAY_CONTROL           0x09

#define TT_VENDOR_SPECIFIC              0x0100
#define TT_STREAMING                    0x0101

/* Input Terminal types */
#define ITT_VENDOR_SPECIFIC             0x0200
#define ITT_CAMERA                      0x0201
#define ITT_MEDIA_TRANSPORT_INPUT       0x0202

/* Output Terminal types */
#define OTT_VENDOR_SPECIFIC             0x0300
#define OTT_DISPLAY                     0x0301
#define OTT_MEDIA_TRANSPORT_OUTPUT      0x0302

#define EXTERNAL_VENDOR_SPECIFIC        0x0400
#define COMPOSITE_CONNECTOR             0x0401
#define SVIDEO_CONNECTOR                0x0402
#define COMPONENT_CONNECTOR             0x0403

#define UVC_ENTITY_IS_UNIT(entity)	((entity->type & 0xff00) == 0)
#define UVC_ENTITY_IS_TERM(entity)	((entity->type & 0xff00) != 0)
#define UVC_ENTITY_IS_ITERM(entity)	((entity->type & 0xff00) == ITT_VENDOR_SPECIFIC)
#define UVC_ENTITY_IS_OTERM(entity)	((entity->type & 0xff00) == OTT_VENDOR_SPECIFIC)

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

#if 0
#define UVC_GUID_UNIT_EXTENSION 0x92, 0x42, 0x39, 0x46,\
   								0xD1, 0x0C,\
   								0xE3, 0x4A,\
   								0x87, 0x83,\
   								0x31, 0x33, 0xF9, 0xEA, 0xAA, 0x3B
#else

#define UVC_GUID_UNIT_EXTENSION1 0x70, 0x33, 0xf0, 0x28,\
	0x11, 0x63,\
	0x2e, 0x4A,\
	0xba, 0x2c,\
	0x68, 0x90, 0xeb, 0x33, 0x40, 0x16
#define UVC_GUID_UNIT_EXTENSION2 0x94, 0x73, 0xdf, 0xdd,\
	0x3e, 0x97,\
	0x27, 0x47,\
	0xbe, 0xd9,\
	0x04, 0xed, 0x64, 0x26, 0xdc, 0x67
#endif
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

#define UVC_GUID_FORMAT_I420	'I',  '4',  '2',  '0', 0x00, 0x00, 0x10, 0x00, \
    0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71

#define UVC_GUID_FORMAT_H264	'H',  '2',  '6',  '4', 0x00, 0x00, 0x10, 0x00, \
    0x80, 0x00, 0x00, 0xaa,0x00, 0x38, 0x9b, 0x71

/* ------------------------------------------------------------------------
 * Driver specific constants.
 */


#define UVC_FORMAT_YUY2     0
#define UVC_FORMAT_NV12     0
#define UVC_FORMAT_I420     1
#define UVC_FORMAT_MJPG     1



#if 1
#define FREAME_MAX_WIDTH        1280
#define FREAME_MAX_HEIGHT       720
#else
#define FREAME_MAX_WIDTH        640
#define FREAME_MAX_HEIGHT       480
#endif

#define UVC_FRAME_SZIE      (FREAME_MAX_WIDTH * FREAME_MAX_HEIGHT * 2)

#define MJPG_WWIDTH_0     (1280)
#define MJPG_WHEIGHT_0    (720)

#define MJPG_WWIDTH_1     (0)
#define MJPG_WHEIGHT_1    (0)

#define MJPG_WWIDTH_2     (0)
#define MJPG_WHEIGHT_2    (0)

#define MJPG_WWIDTH_3     (0)
#define MJPG_WHEIGHT_3    (0)

#define MJPG_WWIDTH_4     (0)
#define MJPG_WHEIGHT_4    (0)

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

#define FRAME_FPS         (10000000/30)

#if UVC_FORMAT_YUY2||UVC_FORMAT_NV12
#define ISO_EP_INTERVAL     1
#else
#define ISO_EP_INTERVAL     1
#endif

#define UVC_DWCLOCKFREQUENCY    0x01c9c380

#define UVC_IT_TERMINALID       1
#define UVC_PU_TERMINALID       2
#define UVC_EXT_TERMINALID1     3
#define UVC_EXT_TERMINALID2     4
#define UVC_OT_TERMINALID1      5


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


#define	VIDEO_INF_NUM                   2

#define VS_IT_CONTROLS_ENABLE  1
#define VS_PU_CONTROLS_ENABLE  1

#if 1
#define FMT_MAX_RESO_NUM    5

struct uvc_unit_ctrl {
    u8  request;
    u8  unit;
    u32 len;
    u8 *data;
};

struct uvc_reso {
    u16 width;
    u16 height;
};

struct uvc_reso_info {
    int num;
    struct uvc_reso reso[5];
};

struct usb_camera {
    u8  quality;
    u32 bits_rate;
    int (*video_open)(int idx, int fmt, int frame_id, int fps);
    int (*video_reqbuf)(int idx, void *buf, u32 len, u32 *frame_end);
    int (*video_close)(int idx);
    int (*processing_unit_response)(struct uvc_unit_ctrl *ctl_req);
    int (*private_cmd_response)(u16 cmd, u16 data);
    const struct uvc_reso_info *jpg_info;
    const struct uvc_reso_info *yuv_info;
};
#endif


void usb_video_shutter(usb_dev usb_id);
u32 uvc_desc_config(usb_dev usb_id, u8 *ptr, u32 *inf);
void uvc_set_camera_info(usb_dev usb_id, const struct usb_camera *info);
u32 uvc_register(usb_dev usb_id);
u32 uvc_release(usb_dev usb_id);


#ifdef __cplusplus
}
#endif
#endif

