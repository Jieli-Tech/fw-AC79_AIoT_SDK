/*************************************************************************
	> File Name: isp.h
	> Author:
	> Mail:
	> Created Time: Fri 24 Feb 2017 04:01:17 PM HKT
 ************************************************************************/

#ifndef _DEVICE_ISP_H_
#define _DEVICE_ISP_H_

#include "typedef.h"
#include "device/device.h"
#include "generic/ioctl.h"

enum isp_mode {
    ISP_MODE_NONE   = 0x0,
    ISP_MODE_REC,
    ISP_MODE_IMAGE_CAPTURE,
};

enum {
    ISP_CFG_MODE_GENERIC    = 0x0,
    ISP_CFG_MODE_CUSTOMIZE,
};


enum isp_generic_cmd {
    ISP_SET_INPUT_SIZE = 0x4A4C0000,
    ISP_SET_OUTPUT_SIZE,
    ISP_SET_MODE,
    ISP_SET_EV,
    ISP_SET_WB,
    ISP_SET_SHP,
    ISP_SET_DRC,


    ISP_GET_LV,
    ISP_GET_FREQ,
    ISP_GET_SEN_STATUS,
    ISP_GET_VISGNAL,
    ISP_GET_SEN_SIZE,
    ISP_GET_ISP_SIZE,
};

enum isp_special_cmd {
    ISP_SET_CCM,
    ISP_SET_NR,
    ISP_SET_SHPN,
    ISP_SET_SATURATION,
    ISP_SET_GAMMA,
    ISP_SET_BRIGHTNESS,
    ISP_SET_CONTRAST,
};

#define     ISP_CUSTOMIZE_MODE_TOOL     0x1
#define     ISP_CUSTOMIZE_MODE_FILE     0x2
#define     ISP_CUSTOMIZE_MODE_SPECIAL  0x3



struct isp_pix_format {
    u32 fps;
    u16 width;
    u16 height;
};

struct isp_generic_cfg {
    u32 id;
    u8 mode;
    u8 sen_status;
    u8 vsignal;
    s8 ev;
    u8 white_blance;
    u8 sharpness;
    u8 drc;
    s32 lv;

    struct isp_pix_format in_fmt;
    struct isp_pix_format out_fmt;
    struct isp_pix_format sen_fmt;
};

struct ispt_customize_cfg {
    //u32     cmd;
    u8      mode;
    u8      cmd;
    u8      *data;
    int     len;
    u16     version;
    u16     crc;
    void    *private;
};

#define ISP_IOCTL_SET_GENERIC_CFG 			_IOW('I', 0,  struct isp_generic_cfg)
#define ISP_IOCTL_GET_GENERIC_CFG           _IOR('I', 1,  struct isp_generic_cfg)
#define ISP_IOCTL_SET_CUSTOMIZE_CFG         _IOW('I', 2,  struct ispt_customize_cfg)
#define ISP_IOCTL_GET_CUSTOMIZE_CFG         _IOR('I', 3,  struct ispt_customize_cfg)
#define ISP_IOCTL_SET_FPS                   _IOW('I', 4,  int)


#endif
