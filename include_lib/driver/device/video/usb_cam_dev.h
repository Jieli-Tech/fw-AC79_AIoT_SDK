/*****************************************************************
>file name : usb_cam_dev.h
>author : lichao
>create time : Mon 28 Aug 2017 10:49:01 AM HKT
*****************************************************************/

#ifndef _USB_CAM_DEV_H
#define _USB_CAM_DEV_H
#include "typedef.h"
#include "ioctl.h"

struct usb_camera_platform_data {
    u8  open_log;
};

#define USB_CAMERA_PLATFORM_DATA_BEGIN(data) \
    static const struct usb_camera_platform_data data = {\

#define USB_CAMERA_PLATFORM_DATA_END()  \
};

#define USB_CAM_TYPE_JPEG		0x01
#define USB_CAM_TYPE_H264		0x02
#define USB_CAM_TYPE_YUV        0x03
#define USB_CAM_TYPE_IMAGE      0x04

struct usb_camera_vframe {
    u8   type;
    void *data;
    u32  len;
    int  timeout;
};

#define USBCAM_STATE_FRAME_START            0x0
#define USBCAM_STATE_FRAME_TRANS            0x80
#define USBCAM_STATE_FRAME_END              0xff

#define USBCAM_IOCTL_GET_FRAME_STATE        _IOR('U', 0, sizeof(int))
#define USBCAM_IOCTL_SET_READ_MODE          _IOW('U', 1, sizeof(int))

extern const struct device_operations usb_cam_dev_ops;
#endif
