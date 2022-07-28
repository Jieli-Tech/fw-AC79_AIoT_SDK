#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "ov7670.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"



static void *iic = NULL;
static u8 OV7670_reset_io[2] = {-1, -1};
static u8 OV7670_power_io[2] = {-1, -1};

#if (CONFIG_VIDEO_IMAGE_W > 640)
#define OV7670_DEVP_INPUT_W 	640
#define OV7670_DEVP_INPUT_H		480
#else
#define OV7670_DEVP_INPUT_W 	CONFIG_VIDEO_IMAGE_W
#define OV7670_DEVP_INPUT_H		CONFIG_VIDEO_IMAGE_H
#endif



#define OV7670_WRCMD 0x42
#define OV7670_RDCMD 0x43


#define CONFIG_INPUT_FPS	20  //一般不改，INPUT_FPS为最高帧率

#define DELAY_TIME	10

static s32 OV7670_set_output_size(u16 *width, u16 *height, u8 *freq);

static unsigned char wrOV7670Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, OV7670_WRCMD)) {
        ret = 0;
        printf("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(DELAY_TIME);
    if (dev_ioctl(iic, IIC_IOCTL_TX, regID)) {
        ret = 0;
        printf("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(DELAY_TIME);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_STOP_BIT, regDat)) {
        ret = 0;
        printf("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(DELAY_TIME);

exit:
    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    return ret;
}


static unsigned char rdOV7670Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, OV7670_WRCMD)) {
        ret = 0;
        printf("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(DELAY_TIME);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_STOP_BIT, regID)) {
        ret = 0;
        printf("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(DELAY_TIME);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, OV7670_RDCMD)) {
        ret = 0;
        printf("iic write err!!! line : %d \n", __LINE__);
        goto exit;
    }
    delay(DELAY_TIME);
    dev_ioctl(iic, IIC_IOCTL_RX_WITH_STOP_BIT, (u32)regDat);

exit:
    dev_ioctl(iic, IIC_IOCTL_STOP, 0);
    return ret;
}

static void OV7670_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
//  ============杰理=============================

    wrOV7670Reg(0x12, 0x80);
    wrOV7670Reg(0x11, 0x80);
    wrOV7670Reg(0x3a, 0x04);
    wrOV7670Reg(0x12, 0x00);
    wrOV7670Reg(0x17, 0x13);
    wrOV7670Reg(0x18, 0x01);
    wrOV7670Reg(0x32, 0xb6);
    wrOV7670Reg(0x19, 0x02);
    wrOV7670Reg(0x1a, 0x7a);
    wrOV7670Reg(0x03, 0x0a);
    wrOV7670Reg(0x0c, 0x00);
    wrOV7670Reg(0x3e, 0x00);
    wrOV7670Reg(0x70, 0x3a);
    wrOV7670Reg(0x71, 0x35);
    wrOV7670Reg(0x72, 0x11);
    wrOV7670Reg(0x73, 0xf0);
    wrOV7670Reg(0xa2, 0x02);
    wrOV7670Reg(0x7a, 0x20);
    wrOV7670Reg(0x7b, 0x10);
    wrOV7670Reg(0x7c, 0x1e);
    wrOV7670Reg(0x7d, 0x35);
    wrOV7670Reg(0x7e, 0x5a);
    wrOV7670Reg(0x7f, 0x69);
    wrOV7670Reg(0x80, 0x76);
    wrOV7670Reg(0x81, 0x80);
    wrOV7670Reg(0x82, 0x88);
    wrOV7670Reg(0x83, 0x8f);
    wrOV7670Reg(0x84, 0x96);
    wrOV7670Reg(0x85, 0xa3);
    wrOV7670Reg(0x86, 0xaf);
    wrOV7670Reg(0x87, 0xc4);
    wrOV7670Reg(0x88, 0xd7);
    wrOV7670Reg(0x89, 0xe8);
    wrOV7670Reg(0x13, 0xe0);
    wrOV7670Reg(0x01, 0x58);
    wrOV7670Reg(0x02, 0x68);
    wrOV7670Reg(0x00, 0x00);
    wrOV7670Reg(0x10, 0x00);
    wrOV7670Reg(0x0d, 0x40);
    wrOV7670Reg(0x14, 0x18);
    wrOV7670Reg(0xa5, 0x05);
    wrOV7670Reg(0xab, 0x07);
    wrOV7670Reg(0x24, 0x95);
    wrOV7670Reg(0x25, 0x33);
    wrOV7670Reg(0x26, 0xe3);
    wrOV7670Reg(0x9f, 0x78);
    wrOV7670Reg(0xa0, 0x68);
    wrOV7670Reg(0xa1, 0x03);
    wrOV7670Reg(0xa6, 0xd8);
    wrOV7670Reg(0xa7, 0xd8);
    wrOV7670Reg(0xa8, 0xf0);
    wrOV7670Reg(0xa9, 0x90);
    wrOV7670Reg(0xaa, 0x94);
    wrOV7670Reg(0x13, 0xe5);
    wrOV7670Reg(0x0e, 0x61);
    wrOV7670Reg(0x0f, 0x4b);
    wrOV7670Reg(0x16, 0x02);
    wrOV7670Reg(0x1e, 0x0f);    // 0x07  0x0f
    wrOV7670Reg(0x21, 0x02);
    wrOV7670Reg(0x22, 0x91);
    wrOV7670Reg(0x29, 0x07);
    wrOV7670Reg(0x33, 0x0b);
    wrOV7670Reg(0x35, 0x0b);
    wrOV7670Reg(0x37, 0x1d);
    wrOV7670Reg(0x38, 0x71);
    wrOV7670Reg(0x39, 0x2a);
    wrOV7670Reg(0x3c, 0x78);
    wrOV7670Reg(0x4d, 0x40);
    wrOV7670Reg(0x4e, 0x20);
    wrOV7670Reg(0x69, 0x00);
    wrOV7670Reg(0x6b, 0x0a);
    wrOV7670Reg(0x74, 0x10);
    wrOV7670Reg(0x8d, 0x4f);
    wrOV7670Reg(0x8e, 0x00);
    wrOV7670Reg(0x8f, 0x00);
    wrOV7670Reg(0x90, 0x00);
    wrOV7670Reg(0x91, 0x00);
    wrOV7670Reg(0x92, 0x19);
    wrOV7670Reg(0x96, 0x00);
    wrOV7670Reg(0x9a, 0x80);
    wrOV7670Reg(0xb0, 0x84);
    wrOV7670Reg(0xb1, 0x0c);
    wrOV7670Reg(0xb2, 0x0e);
    wrOV7670Reg(0xb3, 0x82);
    wrOV7670Reg(0xb8, 0x0a);
    wrOV7670Reg(0x43, 0x14);
    wrOV7670Reg(0x44, 0xf0);
    wrOV7670Reg(0x45, 0x34);
    wrOV7670Reg(0x46, 0x58);
    wrOV7670Reg(0x47, 0x28);
    wrOV7670Reg(0x48, 0x3a);
    wrOV7670Reg(0x59, 0x88);
    wrOV7670Reg(0x5a, 0x88);
    wrOV7670Reg(0x5b, 0x44);
    wrOV7670Reg(0x5c, 0x67);
    wrOV7670Reg(0x5d, 0x49);
    wrOV7670Reg(0x5e, 0x0e);
    wrOV7670Reg(0x64, 0x04);
    wrOV7670Reg(0x65, 0x20);
    wrOV7670Reg(0x66, 0x05);
    wrOV7670Reg(0x94, 0x04);
    wrOV7670Reg(0x95, 0x08);
    wrOV7670Reg(0x6c, 0x0a);
    wrOV7670Reg(0x6d, 0x55);
    wrOV7670Reg(0x6e, 0x11);
    wrOV7670Reg(0x6f, 0x9f);
    wrOV7670Reg(0x6a, 0x40);
    wrOV7670Reg(0x01, 0x40);
    wrOV7670Reg(0x02, 0x40);
    wrOV7670Reg(0x4f, 0x80);
    wrOV7670Reg(0x50, 0x80);
    wrOV7670Reg(0x51, 0x00);
    wrOV7670Reg(0x52, 0x22);
    wrOV7670Reg(0x53, 0x5e);
    wrOV7670Reg(0x54, 0x80);
    wrOV7670Reg(0x58, 0x9e);
    wrOV7670Reg(0x41, 0x08);
    wrOV7670Reg(0x3f, 0x00);
    wrOV7670Reg(0x75, 0x04);
    wrOV7670Reg(0x76, 0xe1);
    wrOV7670Reg(0x4c, 0x00);
    wrOV7670Reg(0x77, 0x01);
    wrOV7670Reg(0x3d, 0xc2);
    wrOV7670Reg(0x4b, 0x09);
    wrOV7670Reg(0xc9, 0x60);
    wrOV7670Reg(0x41, 0x38);
    wrOV7670Reg(0x56, 0x40);
    wrOV7670Reg(0x34, 0x11);
    wrOV7670Reg(0x3b, 0x0a);
    wrOV7670Reg(0xa4, 0x88);
    wrOV7670Reg(0x96, 0x00);
    wrOV7670Reg(0x97, 0x30);
    wrOV7670Reg(0x98, 0x20);
    wrOV7670Reg(0x99, 0x30);
    wrOV7670Reg(0x9a, 0x84);
    wrOV7670Reg(0x9b, 0x29);
    wrOV7670Reg(0x9c, 0x03);
    wrOV7670Reg(0x9d, 0x4c);
    wrOV7670Reg(0x9e, 0x3f);
    wrOV7670Reg(0x78, 0x04);
    wrOV7670Reg(0x79, 0x01);
    wrOV7670Reg(0xc8, 0xf0);
    wrOV7670Reg(0x79, 0x0f);
    wrOV7670Reg(0xc8, 0x00);
    wrOV7670Reg(0x79, 0x10);
    wrOV7670Reg(0xc8, 0x7e);
    wrOV7670Reg(0x79, 0x0a);
    wrOV7670Reg(0xc8, 0x80);
    wrOV7670Reg(0x79, 0x0b);
    wrOV7670Reg(0xc8, 0x01);
    wrOV7670Reg(0x79, 0x0c);
    wrOV7670Reg(0xc8, 0x0f);
    wrOV7670Reg(0x79, 0x0d);
    wrOV7670Reg(0xc8, 0x20);
    wrOV7670Reg(0x79, 0x09);
    wrOV7670Reg(0xc8, 0x80);
    wrOV7670Reg(0x79, 0x02);
    wrOV7670Reg(0xc8, 0xc0);
    wrOV7670Reg(0x79, 0x03);
    wrOV7670Reg(0xc8, 0x40);
    wrOV7670Reg(0x79, 0x05);
    wrOV7670Reg(0xc8, 0x30);
    wrOV7670Reg(0x79, 0x26);
    wrOV7670Reg(0x2d, 0x00);
    wrOV7670Reg(0x2e, 0x00);
    wrOV7670Reg(0x13, 0xe7);
    wrOV7670Reg(0x09, 0x01);
    wrOV7670Reg(0x11, 0x00);

#if (CONFIG_INPUT_FPS == 30)
    wrOV7670Reg(0x11, 0x02);	 // 30fps
    wrOV7670Reg(0x6b, 0x8a);	// pclk*6
#elif (CONFIG_INPUT_FPS == 20)
    wrOV7670Reg(0x11, 0x05);    // 27fps
    wrOV7670Reg(0x6b, 0xca);	// pclk*8
#else
    wrOV7670Reg(0x11, 0x08);    // 15fps
    wrOV7670Reg(0x6b, 0xca);	// pclk*8
#endif

    *format = SEN_IN_FORMAT_YUYV;
    //=========================================

    /*
    //==========小徐===============================
     //   wrOV7670Reg(0x11, 0x02);	 // 30fps
     //   wrOV7670Reg(0x6b, 0x8a);	// pclk*6

       wrOV7670Reg(0x11, 0x04);    // 27fps
        wrOV7670Reg(0x6b, 0xca);	// pclk*8

     //   wrOV7670Reg(0x11, 0x08);    // 15fps
      //  wrOV7670Reg(0x6b, 0xca);	// pclk*8

      //   wrOV7670Reg(0x11, 0x06);    // 10fps
    //  wrOV7670Reg(0x6b, 0x4a);

        wrOV7670Reg(0x3A, 0x04);
    	wrOV7670Reg(0x12, 0x00);
    	wrOV7670Reg(0x17, 0x13);
    	wrOV7670Reg(0x18, 0x01);
    	wrOV7670Reg(0x32, 0xB6);
    	wrOV7670Reg(0x19, 0x02);
    	wrOV7670Reg(0x1A, 0x7A);
    	wrOV7670Reg(0x03, 0x0F);
    	wrOV7670Reg(0x0C, 0x00);
    	wrOV7670Reg(0x3E, 0x00);
    	wrOV7670Reg(0x70, 0x3A);
    	wrOV7670Reg(0x71, 0x35);
    	wrOV7670Reg(0x72, 0x11);
    	wrOV7670Reg(0x73, 0xF0);
    	wrOV7670Reg(0xA2, 0x3B);
    	wrOV7670Reg(0x1E, 0x0F);
    	wrOV7670Reg(0x7a, 0x20);
    	wrOV7670Reg(0x7b, 0x03);
    	wrOV7670Reg(0x7c, 0x0a);
    	wrOV7670Reg(0x7d, 0x1a);
    	wrOV7670Reg(0x7e, 0x3f);
    	wrOV7670Reg(0x7f, 0x4e);
    	wrOV7670Reg(0x80, 0x5b);
    	wrOV7670Reg(0x81, 0x68);
    	wrOV7670Reg(0x82, 0x75);
    	wrOV7670Reg(0x83, 0x7f);
    	wrOV7670Reg(0x84, 0x89);
    	wrOV7670Reg(0x85, 0x9a);
    	wrOV7670Reg(0x86, 0xa6);
    	wrOV7670Reg(0x87, 0xbd);
    	wrOV7670Reg(0x88, 0xd3);
    	wrOV7670Reg(0x89, 0xe8);
    	wrOV7670Reg(0x13, 0xE0);
    	wrOV7670Reg(0x00, 0x00);
    	wrOV7670Reg(0x10, 0x00);
    	wrOV7670Reg(0x0D, 0x50);
    	wrOV7670Reg(0x42, 0x40);
    	wrOV7670Reg(0x14, 0x28);
    	wrOV7670Reg(0xA5, 0x03);
    	wrOV7670Reg(0xAB, 0x03);
    	wrOV7670Reg(0x24, 0x50);
    	wrOV7670Reg(0x25, 0x43);
    	wrOV7670Reg(0x26, 0xa3);
    	wrOV7670Reg(0x9F, 0x78);
    	wrOV7670Reg(0xA0, 0x68);
    	wrOV7670Reg(0xA1, 0x03);
    	wrOV7670Reg(0xA6, 0xd2);
    	wrOV7670Reg(0xA7, 0xd2);
    	wrOV7670Reg(0xA8, 0xF0);
    	wrOV7670Reg(0xA9, 0x80);
    	wrOV7670Reg(0xAA, 0x14);
    	wrOV7670Reg(0x13, 0xE5);
    	wrOV7670Reg(0x0E, 0x61);
    	wrOV7670Reg(0x0F, 0x4B); 	// Flip (bit4) & Mirror (bit5)
    	wrOV7670Reg(0x16, 0x02);
    	wrOV7670Reg(0x21, 0x02);
    	wrOV7670Reg(0x22, 0x91);
    	wrOV7670Reg(0x29, 0x07);
    	wrOV7670Reg(0x33, 0x0B);
    	wrOV7670Reg(0x35, 0x0B);
    	wrOV7670Reg(0x37, 0x1D);
    	wrOV7670Reg(0x38, 0x71);
    	wrOV7670Reg(0x39, 0x2A);
    	wrOV7670Reg(0x3C, 0x78);
    	wrOV7670Reg(0x4D, 0x40);
    	wrOV7670Reg(0x4E, 0x20);
    	wrOV7670Reg(0x69, 0x00);
    	wrOV7670Reg(0x74, 0x10);
    	wrOV7670Reg(0x8D, 0x4F);
    	wrOV7670Reg(0x8E, 0x00);
    	wrOV7670Reg(0x8F, 0x00);
    	wrOV7670Reg(0x90, 0x00);
    	wrOV7670Reg(0x91, 0x00);
    	wrOV7670Reg(0x96, 0x00);
    	wrOV7670Reg(0x9A, 0x80);
    	wrOV7670Reg(0xB0, 0x84);
    	wrOV7670Reg(0xB1, 0x0C);
    	wrOV7670Reg(0xB2, 0x0E);
    	wrOV7670Reg(0xB3, 0x82);
    	wrOV7670Reg(0xB8, 0x0A);
    	wrOV7670Reg(0x43, 0x02);
    	wrOV7670Reg(0x44, 0xf2);
    	wrOV7670Reg(0x45, 0x46);
    	wrOV7670Reg(0x46, 0x63);
    	wrOV7670Reg(0x47, 0x32);
    	wrOV7670Reg(0x48, 0x3b);
    	wrOV7670Reg(0x59, 0x92);
    	wrOV7670Reg(0x5a, 0x9b);
    	wrOV7670Reg(0x5b, 0xa5);
    	wrOV7670Reg(0x5c, 0x7a);
    	wrOV7670Reg(0x5d, 0x4a);
    	wrOV7670Reg(0x5e, 0x0a);
    	wrOV7670Reg(0x6c, 0x0a);
    	wrOV7670Reg(0x6d, 0x55);
    	wrOV7670Reg(0x6e, 0x11);
    	wrOV7670Reg(0x6f, 0x9e);
    	wrOV7670Reg(0x6A, 0x40);
    	wrOV7670Reg(0x01, 0x40);
    	wrOV7670Reg(0x02, 0x40);
    	wrOV7670Reg(0x13, 0xf7);
    	wrOV7670Reg(0x4f, 0x9c);
    	wrOV7670Reg(0x50, 0x99);
    	wrOV7670Reg(0x51, 0x02);
    	wrOV7670Reg(0x52, 0x29);
    	wrOV7670Reg(0x53, 0x8b);
    	wrOV7670Reg(0x54, 0xb5);
    	wrOV7670Reg(0x58, 0x1e);
    	wrOV7670Reg(0x62, 0x08);
    	wrOV7670Reg(0x63, 0x10);
    	wrOV7670Reg(0x64, 0x04);
    	wrOV7670Reg(0x65, 0x00);
    	wrOV7670Reg(0x66, 0x05);
    	wrOV7670Reg(0x94, 0x04);
    	wrOV7670Reg(0x95, 0x06);
    	wrOV7670Reg(0x41, 0x08);
    	wrOV7670Reg(0x3F, 0x00);
    	wrOV7670Reg(0x75, 0x44);
    	wrOV7670Reg(0x76, 0xe1);
    	wrOV7670Reg(0x4C, 0x00);
    	wrOV7670Reg(0x77, 0x01);
    	wrOV7670Reg(0x3D, 0xC0);
    	wrOV7670Reg(0x4B, 0x09);
    	wrOV7670Reg(0xC9, 0x60);
    	wrOV7670Reg(0x41, 0x38);
    	wrOV7670Reg(0x56, 0x40);
    	wrOV7670Reg(0x34, 0x11);
    	wrOV7670Reg(0x3b, 0x02);
    	wrOV7670Reg(0xa4, 0x88);	//disable  night mode
    	wrOV7670Reg(0x92, 0x00);
    	wrOV7670Reg(0x96, 0x00);
    	wrOV7670Reg(0x97, 0x30);
    	wrOV7670Reg(0x98, 0x20);
    	wrOV7670Reg(0x99, 0x20);
    	wrOV7670Reg(0x9A, 0x84);
    	wrOV7670Reg(0x9B, 0x29);
    	wrOV7670Reg(0x9C, 0x03);
    	wrOV7670Reg(0x9D, 0x99);
    	wrOV7670Reg(0x9E, 0x7F);
    	wrOV7670Reg(0x78, 0x00);
    	wrOV7670Reg(0x79, 0x01);
    	wrOV7670Reg(0xc8, 0xf0);
    	wrOV7670Reg(0x79, 0x0f);
    	wrOV7670Reg(0xc8, 0x00);
    	wrOV7670Reg(0x79, 0x10);
    	wrOV7670Reg(0xc8, 0x7e);
    	wrOV7670Reg(0x79, 0x0a);
    	wrOV7670Reg(0xc8, 0x80);
    	wrOV7670Reg(0x79, 0x0b);
    	wrOV7670Reg(0xc8, 0x01);
    	wrOV7670Reg(0x79, 0x0c);
    	wrOV7670Reg(0xc8, 0x0f);
    	wrOV7670Reg(0x79, 0x0d);
    	wrOV7670Reg(0xc8, 0x20);
    	wrOV7670Reg(0x79, 0x09);
    	wrOV7670Reg(0xc8, 0x80);
    	wrOV7670Reg(0x79, 0x02);
    	wrOV7670Reg(0xc8, 0xc0);
    	wrOV7670Reg(0x79, 0x03);
    	wrOV7670Reg(0xc8, 0x40);
    	wrOV7670Reg(0x79, 0x05);
    	wrOV7670Reg(0xc8, 0x30);
    	wrOV7670Reg(0x79, 0x26);
    	wrOV7670Reg(0x3b, 0x02);
    	wrOV7670Reg(0x43, 0x02);
    	wrOV7670Reg(0x44, 0xf2);
    	wrOV7670Reg(0x30, 0x4F);
    */
    /*
    //=================================================
        //   wrOV7670Reg(0x11, 0x02);	 // 30fps
    	 //   wrOV7670Reg(0x6b, 0x8a);	// pclk*6

    //	   wrOV7670Reg(0x11, 0x04);    // 27fps
    //        wrOV7670Reg(0x6b, 0xca);	// pclk*8

      //      wrOV7670Reg(0x11, 0x08);    // 15fps
      //      wrOV7670Reg(0x6b, 0xca);	// pclk*8

          //   wrOV7670Reg(0x11, 0x06);    // 10fps
        //  wrOV7670Reg(0x6b, 0x4a);

        wrOV7670Reg(0x12, 0x80);
        wrOV7670Reg(0x11, 0x80);       //(0x11, 0x80);
        wrOV7670Reg(0x3a, 0x04);
        wrOV7670Reg(0x12, 0x00);
        wrOV7670Reg(0x17, 0x13);
        wrOV7670Reg(0x18, 0x01);
        wrOV7670Reg(0x32, 0xb6);
        wrOV7670Reg(0x19, 0x02);
        wrOV7670Reg(0x1a, 0x7a);
        wrOV7670Reg(0x03, 0xa);
        wrOV7670Reg(0x0c, 0x00);
        wrOV7670Reg(0x3e, 0x00);
        wrOV7670Reg(0x70, 0x3a);
        wrOV7670Reg(0x71, 0x35);
        wrOV7670Reg(0x72, 0x11);
        wrOV7670Reg(0x73, 0xf0);
        wrOV7670Reg(0xa2, 0x02);
        wrOV7670Reg(0x7a, 0x20);
        wrOV7670Reg(0x7b, 0x10);
        wrOV7670Reg(0x7c, 0x1e);
        wrOV7670Reg(0x7d, 0x35);
        wrOV7670Reg(0x7e, 0x5a);
        wrOV7670Reg(0x7f, 0x69);
        wrOV7670Reg(0x80, 0x76);
        wrOV7670Reg(0x81, 0x80);
        wrOV7670Reg(0x82, 0x88);
        wrOV7670Reg(0x83, 0x8f);
        wrOV7670Reg(0x84, 0x96);
        wrOV7670Reg(0x85, 0xa3);
        wrOV7670Reg(0x86, 0xaf);
        wrOV7670Reg(0x87, 0xc4);
        wrOV7670Reg(0x88, 0xd7);
        wrOV7670Reg(0x89, 0xe8);
        wrOV7670Reg(0x13, 0xe0);
        wrOV7670Reg(0x01, 0x58);
        wrOV7670Reg(0x02, 0x68);
        wrOV7670Reg(0x00, 0x00);
        wrOV7670Reg(0x10, 0x00);
        wrOV7670Reg(0x0d, 0x40);
        wrOV7670Reg(0x14, 0x18);
        wrOV7670Reg(0xa5, 0x05);
        wrOV7670Reg(0xab, 0x07);
        wrOV7670Reg(0x24, 0x95);
        wrOV7670Reg(0x25, 0x33);
        wrOV7670Reg(0x26, 0xe3);
        wrOV7670Reg(0x9f, 0x78);
        wrOV7670Reg(0xa0, 0x68);
        wrOV7670Reg(0xa1, 0x03);
        wrOV7670Reg(0xa6, 0xd8);
        wrOV7670Reg(0xa7, 0xd8);
        wrOV7670Reg(0xa8, 0xf0);
        wrOV7670Reg(0xa9, 0x90);
        wrOV7670Reg(0xaa, 0x94);
        wrOV7670Reg(0x13, 0xe5);
        wrOV7670Reg(0x0e, 0x61);
        wrOV7670Reg(0x0f, 0x4b);   // Flip (bit4) & Mirror (bit5)
        wrOV7670Reg(0x16, 0x02);
        wrOV7670Reg(0x1e, 0x07);
        wrOV7670Reg(0x21, 0x02);
        wrOV7670Reg(0x22, 0x91);
        wrOV7670Reg(0x29, 0x07);
        wrOV7670Reg(0x33, 0x0b);
        wrOV7670Reg(0x35, 0x0b);
        wrOV7670Reg(0x37, 0x1d);
        wrOV7670Reg(0x38, 0x71);
        wrOV7670Reg(0x39, 0x2a);
        wrOV7670Reg(0x3c, 0x78);
        wrOV7670Reg(0x4d, 0x40);
        wrOV7670Reg(0x4e, 0x20);
        wrOV7670Reg(0x69, 0x00);
        wrOV7670Reg(0x6b, 0x0a);    // 0x6b, 0x0a
        wrOV7670Reg(0x74, 0x10);
        wrOV7670Reg(0x8d, 0x4f);
        wrOV7670Reg(0x8e, 0x00);
        wrOV7670Reg(0x8f, 0x00);
        wrOV7670Reg(0x90, 0x00);
        wrOV7670Reg(0x91, 0x00);
        wrOV7670Reg(0x92, 0x19);
        wrOV7670Reg(0x96, 0x00);
        wrOV7670Reg(0x9a, 0x80);
        wrOV7670Reg(0xb0, 0x84);
        wrOV7670Reg(0xb1, 0x0c);
        wrOV7670Reg(0xb2, 0x0e);
        wrOV7670Reg(0xb3, 0x82);
        wrOV7670Reg(0xb8, 0x0a);
        wrOV7670Reg(0x43, 0x14);
        wrOV7670Reg(0x44, 0xf0);
        wrOV7670Reg(0x45, 0x34);
        wrOV7670Reg(0x46, 0x58);
        wrOV7670Reg(0x47, 0x28);
        wrOV7670Reg(0x48, 0x3a);
        wrOV7670Reg(0x59, 0x88);
        wrOV7670Reg(0x5a, 0x88);
        wrOV7670Reg(0x5b, 0x44);
        wrOV7670Reg(0x5c, 0x67);
        wrOV7670Reg(0x5d, 0x49);
        wrOV7670Reg(0x5e, 0x0e);
        wrOV7670Reg(0x64, 0x04);
        wrOV7670Reg(0x65, 0x20);
        wrOV7670Reg(0x66, 0x05);
        wrOV7670Reg(0x94, 0x04);
        wrOV7670Reg(0x95, 0x08);
        wrOV7670Reg(0x6c, 0x0a);
        wrOV7670Reg(0x6d,0x55);
        wrOV7670Reg(0x6e,0x11);
        wrOV7670Reg(0x6f,0x9f);
        wrOV7670Reg(0x6a,0x40);
        wrOV7670Reg(0x01,0x40);
        wrOV7670Reg(0x02,0x40);
        wrOV7670Reg(0x4f,0x80);
        wrOV7670Reg(0x50,0x80);
        wrOV7670Reg(0x51,0x00);
        wrOV7670Reg(0x52,0x22);
        wrOV7670Reg(0x53,0x5e);
        wrOV7670Reg(0x54,0x80);
        wrOV7670Reg(0x58,0x9e);    //disable  night mode
        wrOV7670Reg(0x41,0x08);
        wrOV7670Reg(0x3f,0x00);
        wrOV7670Reg(0x75,0x04);
        wrOV7670Reg(0x76,0xe1);
        wrOV7670Reg(0x4c,0x00);
        wrOV7670Reg(0x77,0x01);
        wrOV7670Reg(0x3d,0xc2);
        wrOV7670Reg(0x4b,0x09);
        wrOV7670Reg(0xc9,0x60);
        wrOV7670Reg(0x41,0x38);
        wrOV7670Reg(0x56,0x40);
        wrOV7670Reg(0x34,0x11);
        wrOV7670Reg(0x3b,0x0a);
        wrOV7670Reg(0xa4,0x88);
        wrOV7670Reg(0x96,0x00);
        wrOV7670Reg(0x97,0x30);
        wrOV7670Reg(0x98,0x20);
        wrOV7670Reg(0x99,0x30);
        wrOV7670Reg(0x9a,0x84);
        wrOV7670Reg(0x9b,0x29);
        wrOV7670Reg(0x9c,0x03);
        wrOV7670Reg(0x9d,0x4c);
        wrOV7670Reg(0x9e,0x3f);
        wrOV7670Reg(0x78,0x04);
        wrOV7670Reg(0x79,0x01);
        wrOV7670Reg(0xc8,0xf0);
        wrOV7670Reg(0x79,0x0f);
        wrOV7670Reg(0xc8,0x00);
        wrOV7670Reg(0x79,0x10);
        wrOV7670Reg(0xc8,0x7e);
        wrOV7670Reg(0x79,0x0a);
        wrOV7670Reg(0xc8,0x80);
        wrOV7670Reg(0x79,0x0b);
        wrOV7670Reg(0xc8,0x01);
        wrOV7670Reg(0x79,0x0c);
        wrOV7670Reg(0xc8,0x0f);
        wrOV7670Reg(0x79,0x0d);
        wrOV7670Reg(0xc8,0x20);
        wrOV7670Reg(0x79,0x09);
        wrOV7670Reg(0xc8,0x80);
        wrOV7670Reg(0x79,0x02);
        wrOV7670Reg(0xc8,0xc0);
        wrOV7670Reg(0x79,0x03);
        wrOV7670Reg(0xc8,0x40);
        wrOV7670Reg(0x79,0x05);
        wrOV7670Reg(0xc8,0x30);
        wrOV7670Reg(0x79,0x26);
        wrOV7670Reg(0x2d,0x00);
        wrOV7670Reg(0x2e,0x00);
        wrOV7670Reg(0x13,0xe7);
        wrOV7670Reg(0x09,0x01);
        wrOV7670Reg(0x11,0x00);
        *format = SEN_IN_FORMAT_YUYV;
    //=============================================
    */
}

static s32 OV7670_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    return 0;
    u16 liv_width = *width;
    u16 liv_height = *height;
    return 0;
}

static s32 OV7670_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}



static s32 OV7670_ID_check(void)
{
    u8 pid = 0x00;
    u8 ver = 0x00;

    rdOV7670Reg(0x0a, &pid);
    rdOV7670Reg(0x0b, &ver);
    printf("Sensor OV7670 PID %x %x\n", pid, ver);
    if (pid == 0x76/* && ver == 0x73*/) {
        puts("\n----helloOV7670-----\n");
        return 0;
    }
    puts("\n----notOV7670_DVP-----\n");
    return -1;
}

static void OV7670_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void OV7670_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("OV7670 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = (u8)OV7670_reset_io[0];
        powd_io = (u8)OV7670_power_io[0];
    } else {
        res_io = (u8)OV7670_reset_io[1];
        powd_io = (u8)OV7670_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        OV7670_powerio_ctl(powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}


s32 OV7670_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        OV7670_reset_io[isp_dev] = (u8)_reset_gpio;
        OV7670_power_io[isp_dev] = (u8)_power_gpio;
    }
    if (iic == NULL) {
        printf("OV7670 iic open err!!!\n\n");
        return -1;
    }
    OV7670_reset(isp_dev);

    if (0 != OV7670_ID_check()) {
        printf("-------not OV7670------\n\n");
        dev_close(iic);
        iic = NULL;
        return -1;
    }
    printf("-------hello OV7670------\n\n");
    return 0;
}


static s32 OV7670_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n OV7670_init \n\n");

    if (0 != OV7670_check(isp_dev, 0, 0)) {
        return -1;
    }
    puts("\n\n OV7670_config_SENSOR \n\n");
    OV7670_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_OV7670(u16 rev_flag)
{
    u8 val;
    rdOV7670Reg(0x1e, &val);
    val &= ~0x30;
    if (!rev_flag) {
        wrOV7670Reg(0x1e, val);
    } else {
        val |= 0x30;
        wrOV7670Reg(0x1e, val);
    }
}

void OV7670_DVP_W_Reg(u16 addr, u16 val)
{
    wrOV7670Reg((u8) addr, (u8) val);
}

u16 OV7670_DVP_R_Reg(u16 addr)
{
    u8 val;
    rdOV7670Reg((u8) addr, &val);
    return val;
}

// *INDENT-OFF*
REGISTER_CAMERA(OV7670) = {
    .logo 				= 	"OV7670",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_YUYV,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B  | SEN_MBUS_HSYNC_ACTIVE_HIGH | \
    						SEN_MBUS_PCLK_SAMPLE_FALLING | SEN_MBUS_VSYNC_ACTIVE_HIGH,
#if CONFIG_CAMERA_H_V_EXCHANGE
    .sync_config		=   SEN_MBUS_SYNC0_VSYNC_SYNC1_HSYNC,//WL82/AC791才可以H-V SYNC互换，请留意
#endif
    .fps         		= 	CONFIG_INPUT_FPS,
    .out_fps			=   CONFIG_INPUT_FPS,
    .sen_size 			= 	{OV7670_DEVP_INPUT_W, OV7670_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{OV7670_DEVP_INPUT_W, OV7670_DEVP_INPUT_H},


    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	OV7670_check,
        .init 		        = 	OV7670_init,
        .set_size_fps 		=	OV7670_set_output_size,
        .power_ctrl         =   OV7670_power_ctl,

        .get_ae_params 	    =	NULL,
        .get_awb_params 	=	NULL,
        .get_iq_params 	    =	NULL,

        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	OV7670_DVP_W_Reg,
        .read_reg 		    =	OV7670_DVP_R_Reg,
        .set_sensor_reverse =   set_rev_sensor_OV7670,
    }
};


