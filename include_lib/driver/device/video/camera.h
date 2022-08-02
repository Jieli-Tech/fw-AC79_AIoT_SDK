#ifndef DEVICE_CAMERA_H
#define DEVICE_CAMERA_H




#include "device/device.h"
#include "asm/isp_dev.h"
#include "video/video.h"


#define VIDEO_TAG_CAMERA        VIDEO_TAG('c', 'a', 'm', 'e')
#define VIDEO_TAG_UVC           VIDEO_TAG('u', 'v', 'c', ' ')
#define VIDEO_TAG_MASS           VIDEO_TAG('m', 'a', 's', 's')


#define CAMERA_DEVICE_NUM  2

#define CSI2_X0_LANE    0
#define CSI2_X1_LANE    1
#define CSI2_X2_LANE    2
#define CSI2_X3_LANE    3
#define CSI2_X4_LANE    4

struct camera_platform_data {
    u8 xclk_gpio;
    u8 xclk_hz;// 0/24-->xclk out:24MHz, 12-->xclk out:12MHz
    u8 reset_gpio;
    u8 pwdn_gpio;
    u8 power_value;
    u32 interface;
    bool (*online_detect)();
    union {
        struct {
            u8  pclk_gpio;
            u8  hsync_gpio;
            u8  vsync_gpio;
            u8  group_port;
            u8 data_gpio[10];
        } dvp;
        struct {
            u8 data_lane_num;
            u8 clk_inv;
            u8 d0_rmap;
            u8 d0_inv;
            u8 d1_rmap;
            u8 d1_inv;
            u8 d2_rmap;
            u8 d2_inv;
            u8 d3_rmap;
            u8 d3_inv;
            u8 tval_hstt;
            u8 tval_stto;
        } csi2;
    };
};

struct camera_device_info {
    u16 fps;
    u16 width;
    u16 height;
    u32 real_fps;
    u8 camera_logo[8];
    void *priv;
};


#define CAMERA_PLATFORM_DATA_BEGIN(data) \
	static const struct camera_platform_data data = { \


#define CAMERA_PLATFORM_DATA_END() \
	};

#define CAMERA_CMD_BASE		0x00400000
#define CAMERA_GET_ISP_SRC_SIZE		(CAMERA_CMD_BASE + 1)
#define CAMERA_GET_ISP_SIZE			(CAMERA_CMD_BASE + 2)
#define CAMERA_SET_CROP_SIZE		(CAMERA_CMD_BASE + 3)
#define CAMERA_CROP_TRIG			(CAMERA_CMD_BASE + 4)
#define CAMERA_NEED_REMOUNT			(CAMERA_CMD_BASE + 5)
#define CAMERA_GET_SENSOR_ID    	(CAMERA_CMD_BASE + 6)
#define CAMERA_GET_SENSOR_INFO   	(CAMERA_CMD_BASE + 7)
#define CAMERA_SET_SENSOR_REVSER   	(CAMERA_CMD_BASE + 8)//图片翻转
#define CAMERA_SET_SENSOR_RESET   	(CAMERA_CMD_BASE + 9)//镜头复位重新检测
#define CAMERA_GET_SENSOR_TYPE   	(CAMERA_CMD_BASE + 10)//获取摄像头型号

#define  CAMERA_WITE_BIT BIT(31)
#define  CAMERA_READ_BIT BIT(30)


extern const struct device_operations camera_dev_ops;

//dvp sensor io sel
#define DVP_SENSOR0(sel)   (((sel?1:0) << 24) | (22))//sel 0 PC3~PC13  1 PA6~PA15
#define DVP_SENSOR1(sel)   (((sel?1:0) << 24) | (23))//sel 0 PH1~PH11  1 PD0~PD10


int camera_init(const char *name, const struct video_platform_data *data);

void *camera_driver_open(int id, struct camera_device_info *info);

int camera_driver_ioctl(void *_camera, u32 cmd, void *arg);

int camera_driver_close(void *_camera);

#endif

