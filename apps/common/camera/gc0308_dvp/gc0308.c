#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gc0308.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"



static void *iic = NULL;
static u8 gc0308_reset_io[2] = {-1, -1};
static u8 gc0308_power_io[2] = {-1, -1};

#if (CONFIG_VIDEO_IMAGE_W > 640)
#define GC0308_DEVP_INPUT_W 	640
#define GC0308_DEVP_INPUT_H		480
#else
#define GC0308_DEVP_INPUT_W 	CONFIG_VIDEO_IMAGE_W
#define GC0308_DEVP_INPUT_H		CONFIG_VIDEO_IMAGE_H
#endif

#define GC0308_WRCMD 0x42
#define GC0308_RDCMD 0x43

#define CONFIG_INPUT_FPS	20 //0自动帧率 5/10/15/20/25固定帧率, 25帧需要关闭ppbuf, 加大实时流内存(cpu_config.h)

#define DELAY_TIME	10
static s32 GC0308_set_output_size(u16 *width, u16 *height, u8 *freq);
static unsigned char wrGC0308Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0308_WRCMD)) {
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

static unsigned char rdGC0308Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0308_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0308_RDCMD)) {
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

static void GC0308_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{

    wrGC0308Reg(0xfe, 0x80);
    wrGC0308Reg(0xfe, 0x00);        // set page0
    wrGC0308Reg(0xd2, 0x10);    // close AEC
    wrGC0308Reg(0x22, 0x55);    // close AWB
    wrGC0308Reg(0x5a, 0x56);
    wrGC0308Reg(0x5b, 0x40);
    wrGC0308Reg(0x5c, 0x4a);
    wrGC0308Reg(0x22, 0x57);   // Open AWB

#if (CONFIG_INPUT_FPS == 0)
    wrGC0308Reg(0x03, 0x01);
    wrGC0308Reg(0x04, 0x2c);
    wrGC0308Reg(0xe2, 0x00); 	//anti-flicker step [11:8]
    wrGC0308Reg(0xe3, 0x64);    //anti-flicker step [7:0]

    wrGC0308Reg(0xe4, 0x02);    //exp level 0  16.67fps
    wrGC0308Reg(0xe5, 0x58);
    wrGC0308Reg(0xe6, 0x03);    //exp level 1  12.5fps
    wrGC0308Reg(0xe7, 0x20);
    wrGC0308Reg(0xe8, 0x04);    //exp level 2  8.33fps
    wrGC0308Reg(0xe9, 0xb0);
    wrGC0308Reg(0xea, 0x09);    //exp level 3  4.00fps
    wrGC0308Reg(0xeb, 0xc4);

    wrGC0308Reg(0x05, 0x00);
    wrGC0308Reg(0x06, 0x00);
    wrGC0308Reg(0x07, 0x00);
    wrGC0308Reg(0x08, 0x00);
    GC0308_set_output_size(width, height, frame_freq);
    wrGC0308Reg(0x0d, 0x02);
    wrGC0308Reg(0x0e, 0x02);
    wrGC0308Reg(0x10, 0x22);
    wrGC0308Reg(0x11, 0xfd);
    wrGC0308Reg(0x12, 0x2a);
    wrGC0308Reg(0x13, 0x00);
    wrGC0308Reg(0x14, 0x10);
    wrGC0308Reg(0x15, 0x0a);
    wrGC0308Reg(0x16, 0x05);
    wrGC0308Reg(0x17, 0x01);
    wrGC0308Reg(0x18, 0x44);
    wrGC0308Reg(0x19, 0x44);
    wrGC0308Reg(0x1a, 0x1e);
    wrGC0308Reg(0x1b, 0x00);
    wrGC0308Reg(0x1c, 0xc1);
    wrGC0308Reg(0x1d, 0x08);
    wrGC0308Reg(0x1e, 0x60);
    wrGC0308Reg(0x1f, 0x16);


    wrGC0308Reg(0x20, 0xff);
    wrGC0308Reg(0x21, 0xf8);
    wrGC0308Reg(0x22, 0x57);
    wrGC0308Reg(0x24, 0xa0); //UYVY
    wrGC0308Reg(0x25, 0x0f);

    //output sync_mode
    wrGC0308Reg(0x26, 0x03);
    wrGC0308Reg(0x2f, 0x01);
    wrGC0308Reg(0x30, 0xf7);
    wrGC0308Reg(0x31, 0x50);
    wrGC0308Reg(0x32, 0x00);
    wrGC0308Reg(0x39, 0x04);
    wrGC0308Reg(0x3a, 0x18);
    wrGC0308Reg(0x3b, 0x20);
    wrGC0308Reg(0x3c, 0x00);
    wrGC0308Reg(0x3d, 0x00);
    wrGC0308Reg(0x3e, 0x00);
    wrGC0308Reg(0x3f, 0x00);
    wrGC0308Reg(0x50, 0x10);
    wrGC0308Reg(0x53, 0x82);
    wrGC0308Reg(0x54, 0x80);
    wrGC0308Reg(0x55, 0x80);
    wrGC0308Reg(0x56, 0x82);
    wrGC0308Reg(0x8b, 0x40);
    wrGC0308Reg(0x8c, 0x40);
    wrGC0308Reg(0x8d, 0x40);
    wrGC0308Reg(0x8e, 0x2e);
    wrGC0308Reg(0x8f, 0x2e);
    wrGC0308Reg(0x90, 0x2e);
    wrGC0308Reg(0x91, 0x3c);
    wrGC0308Reg(0x92, 0x50);
    wrGC0308Reg(0x5d, 0x12);
    wrGC0308Reg(0x5e, 0x1a);
    wrGC0308Reg(0x5f, 0x24);
    wrGC0308Reg(0x60, 0x07);
    wrGC0308Reg(0x61, 0x15);
    wrGC0308Reg(0x62, 0x08);
    wrGC0308Reg(0x64, 0x03);
    wrGC0308Reg(0x66, 0xe8);
    wrGC0308Reg(0x67, 0x86);
    wrGC0308Reg(0x68, 0xa2);
    wrGC0308Reg(0x69, 0x18);
    wrGC0308Reg(0x6a, 0x0f);
    wrGC0308Reg(0x6b, 0x00);
    wrGC0308Reg(0x6c, 0x5f);
    wrGC0308Reg(0x6d, 0x8f);
    wrGC0308Reg(0x6e, 0x55);
    wrGC0308Reg(0x6f, 0x38);
    wrGC0308Reg(0x70, 0x15);
    wrGC0308Reg(0x71, 0x33);
    wrGC0308Reg(0x72, 0xdc);
    wrGC0308Reg(0x73, 0x80);
    wrGC0308Reg(0x74, 0x02);
    wrGC0308Reg(0x75, 0x3f);
    wrGC0308Reg(0x76, 0x02);
    wrGC0308Reg(0x77, 0x36);
    wrGC0308Reg(0x78, 0x88);
    wrGC0308Reg(0x79, 0x81);
    wrGC0308Reg(0x7a, 0x81);
    wrGC0308Reg(0x7b, 0x22);
    wrGC0308Reg(0x7c, 0xff);
    wrGC0308Reg(0x93, 0x48);
    wrGC0308Reg(0x94, 0x00);
    wrGC0308Reg(0x95, 0x05);
    wrGC0308Reg(0x96, 0xe8);
    wrGC0308Reg(0x97, 0x40);
    wrGC0308Reg(0x98, 0xf0);
    wrGC0308Reg(0xb1, 0x38);
    wrGC0308Reg(0xb2, 0x38);
    wrGC0308Reg(0xbd, 0x38);
    wrGC0308Reg(0xbe, 0x36);
    wrGC0308Reg(0xd0, 0xc9);
    wrGC0308Reg(0xd1, 0x10);
    //wrGC0308Reg(0xd2 , 0x90);
    wrGC0308Reg(0xd3, 0x80);
    wrGC0308Reg(0xd5, 0xf2);
    wrGC0308Reg(0xd6, 0x16);
    wrGC0308Reg(0xdb, 0x92);
    wrGC0308Reg(0xdc, 0xa5);
    wrGC0308Reg(0xdf, 0x23);
    wrGC0308Reg(0xd9, 0x00);
    wrGC0308Reg(0xda, 0x00);
    wrGC0308Reg(0xe0, 0x09);
    wrGC0308Reg(0xec, 0x10);
    wrGC0308Reg(0xed, 0x04);
    wrGC0308Reg(0xee, 0xa0);
    wrGC0308Reg(0xef, 0x40);
    wrGC0308Reg(0x80, 0x03);
    wrGC0308Reg(0x80, 0x03);
    wrGC0308Reg(0x9F, 0x10);
    wrGC0308Reg(0xA0, 0x20);
    wrGC0308Reg(0xA1, 0x38);
    wrGC0308Reg(0xA2, 0x4E);
    wrGC0308Reg(0xA3, 0x63);
    wrGC0308Reg(0xA4, 0x76);
    wrGC0308Reg(0xA5, 0x87);
    wrGC0308Reg(0xA6, 0xA2);
    wrGC0308Reg(0xA7, 0xB8);
    wrGC0308Reg(0xA8, 0xCA);
    wrGC0308Reg(0xA9, 0xD8);
    wrGC0308Reg(0xAA, 0xE3);
    wrGC0308Reg(0xAB, 0xEB);
    wrGC0308Reg(0xAC, 0xF0);
    wrGC0308Reg(0xAD, 0xF8);
    wrGC0308Reg(0xAE, 0xFD);
    wrGC0308Reg(0xAF, 0xFF);
    wrGC0308Reg(0xc0, 0x00);
    wrGC0308Reg(0xc1, 0x10);
    wrGC0308Reg(0xc2, 0x1C);
    wrGC0308Reg(0xc3, 0x30);
    wrGC0308Reg(0xc4, 0x43);
    wrGC0308Reg(0xc5, 0x54);
    wrGC0308Reg(0xc6, 0x65);
    wrGC0308Reg(0xc7, 0x75);
    wrGC0308Reg(0xc8, 0x93);
    wrGC0308Reg(0xc9, 0xB0);
    wrGC0308Reg(0xca, 0xCB);
    wrGC0308Reg(0xcb, 0xE6);
    wrGC0308Reg(0xcc, 0xFF);
    wrGC0308Reg(0xf0, 0x02);
    wrGC0308Reg(0xf1, 0x01);
    wrGC0308Reg(0xf2, 0x01);
    wrGC0308Reg(0xf3, 0x30);
    wrGC0308Reg(0xf9, 0x9f);
    wrGC0308Reg(0xfa, 0x78);

    //---------------------------------------------------------------
    wrGC0308Reg(0xfe, 0x01);
    wrGC0308Reg(0x00, 0xf5);
    wrGC0308Reg(0x02, 0x1a);
    wrGC0308Reg(0x0a, 0xa0);
    wrGC0308Reg(0x0b, 0x60);
    wrGC0308Reg(0x0c, 0x08);
    wrGC0308Reg(0x0e, 0x4c);
    wrGC0308Reg(0x0f, 0x39);
    wrGC0308Reg(0x11, 0x3f);
    wrGC0308Reg(0x12, 0x72);
    wrGC0308Reg(0x13, 0x13);
    wrGC0308Reg(0x14, 0x42);
    wrGC0308Reg(0x15, 0x43);
    wrGC0308Reg(0x16, 0xc2);
    wrGC0308Reg(0x17, 0xa8);
    wrGC0308Reg(0x18, 0x18);
    wrGC0308Reg(0x19, 0x40);
    wrGC0308Reg(0x1a, 0xd0);
    wrGC0308Reg(0x1b, 0xf5);
    wrGC0308Reg(0x70, 0x40);
    wrGC0308Reg(0x71, 0x58);
    wrGC0308Reg(0x72, 0x30);
    wrGC0308Reg(0x73, 0x48);
    wrGC0308Reg(0x74, 0x20);
    wrGC0308Reg(0x75, 0x60);
    wrGC0308Reg(0x77, 0x20);
    wrGC0308Reg(0x78, 0x32);
    wrGC0308Reg(0x30, 0x03);
    wrGC0308Reg(0x31, 0x40);
    wrGC0308Reg(0x32, 0xe0);
    wrGC0308Reg(0x33, 0xe0);
    wrGC0308Reg(0x34, 0xe0);
    wrGC0308Reg(0x35, 0xb0);
    wrGC0308Reg(0x36, 0xc0);
    wrGC0308Reg(0x37, 0xc0);
    wrGC0308Reg(0x38, 0x04);
    wrGC0308Reg(0x39, 0x09);
    wrGC0308Reg(0x3a, 0x12);
    wrGC0308Reg(0x3b, 0x1C);
    wrGC0308Reg(0x3c, 0x28);
    wrGC0308Reg(0x3d, 0x31);
    wrGC0308Reg(0x3e, 0x44);
    wrGC0308Reg(0x3f, 0x57);
    wrGC0308Reg(0x40, 0x6C);
    wrGC0308Reg(0x41, 0x81);
    wrGC0308Reg(0x42, 0x94);
    wrGC0308Reg(0x43, 0xA7);
    wrGC0308Reg(0x44, 0xB8);
    wrGC0308Reg(0x45, 0xD6);
    wrGC0308Reg(0x46, 0xEE);
    wrGC0308Reg(0x47, 0x0d);
    wrGC0308Reg(0xfe, 0x00);
    wrGC0308Reg(0xd2, 0x90);   // Open AEC at last.


    //Registers of Page0
    wrGC0308Reg(0xfe, 0x00);

    wrGC0308Reg(0x10, 0x26);
    wrGC0308Reg(0x11, 0x0d);  	// fd,modified by mormo 2010/07/06
    wrGC0308Reg(0x1a, 0x2a);  	// 1e,modified by mormo 2010/07/06

    wrGC0308Reg(0x1c, 0x49); 	// c1,modified by mormo 2010/07/06
    wrGC0308Reg(0x1d, 0x9a);	// 08,modified by mormo 2010/07/06
    wrGC0308Reg(0x1e, 0x61);	// 60,modified by mormo 2010/07/06

    wrGC0308Reg(0x3a, 0x20);

    wrGC0308Reg(0x50, 0x14);  	// 10,modified by mormo 2010/07/06
    wrGC0308Reg(0x53, 0x80);
    wrGC0308Reg(0x56, 0x80);

    wrGC0308Reg(0x8b, 0x20); 	//LSC
    wrGC0308Reg(0x8c, 0x20);
    wrGC0308Reg(0x8d, 0x20);
    wrGC0308Reg(0x8e, 0x14);
    wrGC0308Reg(0x8f, 0x10);
    wrGC0308Reg(0x90, 0x14);

    wrGC0308Reg(0x94, 0x02);
    wrGC0308Reg(0x95, 0x07);
    wrGC0308Reg(0x96, 0xe0);

    wrGC0308Reg(0xb1, 0x40);  // YCPT
    wrGC0308Reg(0xb2, 0x40);
    wrGC0308Reg(0xb3, 0x40);
    wrGC0308Reg(0xb6, 0xe0);

    //wrGC0308Reg(0xd0 , 0xcb); // AECT  c9,modifed by mormo 2010/07/06
    //wrGC0308Reg(0xd3 , 0x48); // 80,modified by mormor 2010/07/06

    wrGC0308Reg(0xf2, 0x02);
    wrGC0308Reg(0xf7, 0x12);
    wrGC0308Reg(0xf8, 0x0a);
    wrGC0308Reg(0x57, 0x73);
    wrGC0308Reg(0x58, 0x80);
    wrGC0308Reg(0x59, 0x80);
    //Registers of Page1
    wrGC0308Reg(0xfe, 0x01); //select page1---------------------------------------------------------------

    wrGC0308Reg(0x02, 0x20);
    wrGC0308Reg(0x04, 0x10);
    wrGC0308Reg(0x05, 0x08);
    wrGC0308Reg(0x06, 0x20);
    wrGC0308Reg(0x08, 0x0a);

    wrGC0308Reg(0x0e, 0x44);
    wrGC0308Reg(0x0f, 0x32);
    wrGC0308Reg(0x10, 0x41);
    wrGC0308Reg(0x11, 0x37);
    wrGC0308Reg(0x12, 0x22);
    wrGC0308Reg(0x13, 0x19);
    wrGC0308Reg(0x14, 0x44);
    wrGC0308Reg(0x15, 0x44);

    wrGC0308Reg(0x19, 0x50);
    wrGC0308Reg(0x1a, 0xd8);

    wrGC0308Reg(0x32, 0x10);

    wrGC0308Reg(0x35, 0x00);
    wrGC0308Reg(0x36, 0x80);
    wrGC0308Reg(0x37, 0x00);
    //-----------Update the registers end---------//

    wrGC0308Reg(0xfe, 0x00);

    wrGC0308Reg(0x9F, 0x0E);
    wrGC0308Reg(0xA0, 0x1C);
    wrGC0308Reg(0xA1, 0x34);
    wrGC0308Reg(0xA2, 0x48);
    wrGC0308Reg(0xA3, 0x5A);
    wrGC0308Reg(0xA4, 0x6B);
    wrGC0308Reg(0xA5, 0x7B);
    wrGC0308Reg(0xA6, 0x95);
    wrGC0308Reg(0xA7, 0xAB);
    wrGC0308Reg(0xA8, 0xBF);
    wrGC0308Reg(0xA9, 0xCE);
    wrGC0308Reg(0xAA, 0xD9);
    wrGC0308Reg(0xAB, 0xE4);
    wrGC0308Reg(0xAC, 0xEC);
    wrGC0308Reg(0xAD, 0xF7);
    wrGC0308Reg(0xAE, 0xFD);
    wrGC0308Reg(0xAF, 0xFF);

#else
    wrGC0308Reg(0x01, 0x6a); //0xde /20    4e /25
    wrGC0308Reg(0x02, 0x0c);
    wrGC0308Reg(0x0f, 0x00);
    wrGC0308Reg(0x03, 0x01);
    wrGC0308Reg(0x04, 0x2c);
    wrGC0308Reg(0xe2, 0x00);
    wrGC0308Reg(0xe3, 0x83);
    wrGC0308Reg(0xe4, 0x02); //exp level
    wrGC0308Reg(0xe5, 0x8f);
    wrGC0308Reg(0xe6, 0x02);
    wrGC0308Reg(0xe7, 0x8f);
    wrGC0308Reg(0xe8, 0x02);
    wrGC0308Reg(0xe9, 0x8f);
    wrGC0308Reg(0xea, 0x03);
    wrGC0308Reg(0xeb, 0x95);

    wrGC0308Reg(0x05, 0x00); //window size and position
    wrGC0308Reg(0x06, 0x00);
    wrGC0308Reg(0x07, 0x00);
    wrGC0308Reg(0x08, 0x00);

    if (*format < SEN_IN_FORMAT_YUYV) {
        GC0308_set_output_size(width, height, frame_freq);
    } else {
        wrGC0308Reg(0x09, 0x01);
        wrGC0308Reg(0x0a, 0xe8);
        wrGC0308Reg(0x0b, 0x02);
        wrGC0308Reg(0x0c, 0x88);
        *frame_freq = 30;
    }

    wrGC0308Reg(0x0d, 0x02);
    wrGC0308Reg(0x0e, 0x02);
    wrGC0308Reg(0x10, 0x22);
    wrGC0308Reg(0x11, 0xfd);
    wrGC0308Reg(0x12, 0x2a);
    wrGC0308Reg(0x13, 0x00);
    wrGC0308Reg(0x14, 0x10);
    wrGC0308Reg(0x15, 0x0a);
    wrGC0308Reg(0x16, 0x05);
    wrGC0308Reg(0x17, 0x01);
    wrGC0308Reg(0x18, 0x44);
    wrGC0308Reg(0x19, 0x44);
    wrGC0308Reg(0x1a, 0x1e);
    wrGC0308Reg(0x1b, 0x00);
    wrGC0308Reg(0x1c, 0xc1); //current
    wrGC0308Reg(0x1d, 0x08);
    wrGC0308Reg(0x1e, 0x60);
    wrGC0308Reg(0x1f, 0x16);

    wrGC0308Reg(0x21, 0xf8);
    wrGC0308Reg(0x22, 0x57);
    wrGC0308Reg(0x24, 0xa0); //0xa0 1010 0000
    wrGC0308Reg(0x25, 0x0f);
    wrGC0308Reg(0x26, 0x03);
    wrGC0308Reg(0x2f, 0x01);
    wrGC0308Reg(0x30, 0xf7); //BLK
    wrGC0308Reg(0x31, 0x50);
    wrGC0308Reg(0x32, 0x00);
    wrGC0308Reg(0x39, 0x04);
    wrGC0308Reg(0x3a, 0x18);
    wrGC0308Reg(0x3b, 0x20);
    wrGC0308Reg(0x3c, 0x00);
    wrGC0308Reg(0x3d, 0x00);
    wrGC0308Reg(0x3e, 0x00);
    wrGC0308Reg(0x3f, 0x00);
    wrGC0308Reg(0x50, 0x10); //PREGAIN
    wrGC0308Reg(0x53, 0x82);
    wrGC0308Reg(0x54, 0x80);
    wrGC0308Reg(0x55, 0x80);
    wrGC0308Reg(0x56, 0x82);
    wrGC0308Reg(0x8b, 0x40); //LSC
    wrGC0308Reg(0x8c, 0x40);
    wrGC0308Reg(0x8d, 0x40);
    wrGC0308Reg(0x8e, 0x2e);
    wrGC0308Reg(0x8f, 0x2e);
    wrGC0308Reg(0x90, 0x2e);
    wrGC0308Reg(0x91, 0x3c);
    wrGC0308Reg(0x92, 0x50);
    wrGC0308Reg(0x5d, 0x12);
    wrGC0308Reg(0x5e, 0x1a);
    wrGC0308Reg(0x5f, 0x24);
    wrGC0308Reg(0x60, 0x07); //DNDD 去噪
    wrGC0308Reg(0x61, 0x22); //0x15
    wrGC0308Reg(0x62, 0x0c); //0x08
    wrGC0308Reg(0x64, 0x05); //0x03
    wrGC0308Reg(0x66, 0xe8);
    wrGC0308Reg(0x67, 0x86);
    wrGC0308Reg(0x68, 0xa2);
    wrGC0308Reg(0x69, 0x18); //ASDE
    wrGC0308Reg(0x6a, 0x0f);
    wrGC0308Reg(0x6b, 0x00);
    wrGC0308Reg(0x6c, 0x5f);
    wrGC0308Reg(0x6d, 0x8f);
    wrGC0308Reg(0x6e, 0xa5); //0x55
    wrGC0308Reg(0x6f, 0xa8); //0x38
    wrGC0308Reg(0x70, 0x15);
    wrGC0308Reg(0x71, 0x33);
    wrGC0308Reg(0x72, 0xdc); //0xdc INTPEE edge
    wrGC0308Reg(0x73, 0x80); //0x80
    wrGC0308Reg(0x74, 0x02);
    wrGC0308Reg(0x75, 0x3f);
    wrGC0308Reg(0x76, 0x02);
    wrGC0308Reg(0x77, 0x36);
    wrGC0308Reg(0x78, 0x88);
    wrGC0308Reg(0x79, 0x81);
    wrGC0308Reg(0x7a, 0x81);
    wrGC0308Reg(0x7b, 0x22);
    wrGC0308Reg(0x7c, 0xff);
    wrGC0308Reg(0x93, 0x48); //CC
    wrGC0308Reg(0x94, 0x00);
    wrGC0308Reg(0x95, 0x05);
    wrGC0308Reg(0x96, 0xe8);
    wrGC0308Reg(0x97, 0x40);
    wrGC0308Reg(0x98, 0xf0);
    wrGC0308Reg(0xb1, 0x38); //YCP
    wrGC0308Reg(0xb2, 0x38);
    wrGC0308Reg(0xbd, 0x38);
    wrGC0308Reg(0xbe, 0x36);
    wrGC0308Reg(0xd0, 0xc9); //AEC
    wrGC0308Reg(0xd1, 0x10);
    wrGC0308Reg(0xd3, 0x80); //0x80
    wrGC0308Reg(0xd5, 0xf2);
    wrGC0308Reg(0xd6, 0x16);
    wrGC0308Reg(0xdb, 0x92);
    wrGC0308Reg(0xdc, 0xa5);
    wrGC0308Reg(0xdf, 0x23);
    wrGC0308Reg(0xd9, 0x00);
    wrGC0308Reg(0xda, 0x00);
    wrGC0308Reg(0xe0, 0x09);
    wrGC0308Reg(0xec, 0x20);
    wrGC0308Reg(0xed, 0x04);
    wrGC0308Reg(0xee, 0xa0);
    wrGC0308Reg(0xef, 0x40);
    wrGC0308Reg(0x80, 0x03);
    wrGC0308Reg(0x80, 0x03);
    wrGC0308Reg(0x9F, 0x10);
    wrGC0308Reg(0xA0, 0x20);
    wrGC0308Reg(0xA1, 0x38);
    wrGC0308Reg(0xA2, 0x4E);
    wrGC0308Reg(0xA3, 0x63);
    wrGC0308Reg(0xA4, 0x76);
    wrGC0308Reg(0xA5, 0x87);
    wrGC0308Reg(0xA6, 0xA2);
    wrGC0308Reg(0xA7, 0xB8);
    wrGC0308Reg(0xA8, 0xCA);
    wrGC0308Reg(0xA9, 0xD8);
    wrGC0308Reg(0xAA, 0xE3);
    wrGC0308Reg(0xAB, 0xEB);
    wrGC0308Reg(0xAC, 0xF0);
    wrGC0308Reg(0xAD, 0xF8);
    wrGC0308Reg(0xAE, 0xFD);
    wrGC0308Reg(0xAF, 0xFF);
    wrGC0308Reg(0xc0, 0x00);
    wrGC0308Reg(0xc1, 0x10);
    wrGC0308Reg(0xc2, 0x1C);
    wrGC0308Reg(0xc3, 0x30);
    wrGC0308Reg(0xc4, 0x43);
    wrGC0308Reg(0xc5, 0x54);
    wrGC0308Reg(0xc6, 0x65);
    wrGC0308Reg(0xc7, 0x75);
    wrGC0308Reg(0xc8, 0x93);
    wrGC0308Reg(0xc9, 0xB0);
    wrGC0308Reg(0xca, 0xCB);
    wrGC0308Reg(0xcb, 0xE6);
    wrGC0308Reg(0xcc, 0xFF);
    wrGC0308Reg(0xf0, 0x02); //ABS
    wrGC0308Reg(0xf1, 0x01);
    wrGC0308Reg(0xf2, 0x01);
    wrGC0308Reg(0xf3, 0x30);
    wrGC0308Reg(0xf9, 0x9f); //measure window
    wrGC0308Reg(0xfa, 0x78);

    wrGC0308Reg(0xfe, 0x01); //select page1---------------------------------------------------------------
    wrGC0308Reg(0x00, 0xf5);
    wrGC0308Reg(0x02, 0x1a);
    wrGC0308Reg(0x0a, 0xa0);
    wrGC0308Reg(0x0b, 0x60);
    wrGC0308Reg(0x0c, 0x08);
    wrGC0308Reg(0x0e, 0x4c);
    wrGC0308Reg(0x0f, 0x39);
    wrGC0308Reg(0x11, 0x3f);
    wrGC0308Reg(0x12, 0x72);
    wrGC0308Reg(0x13, 0x13);
    wrGC0308Reg(0x14, 0x42);
    wrGC0308Reg(0x15, 0x43);
    wrGC0308Reg(0x16, 0xc2);
    wrGC0308Reg(0x17, 0xa8);
    wrGC0308Reg(0x18, 0x18);
    wrGC0308Reg(0x19, 0x40);
    wrGC0308Reg(0x1a, 0xd0);
    wrGC0308Reg(0x1b, 0xf5);
    wrGC0308Reg(0x70, 0x40);
    wrGC0308Reg(0x71, 0x58);
    wrGC0308Reg(0x72, 0x30);
    wrGC0308Reg(0x73, 0x48);
    wrGC0308Reg(0x74, 0x20);
    wrGC0308Reg(0x75, 0x60);
    wrGC0308Reg(0x77, 0x20);
    wrGC0308Reg(0x78, 0x32);
    wrGC0308Reg(0x30, 0x03);
    wrGC0308Reg(0x31, 0x40);
    wrGC0308Reg(0x32, 0xe0);
    wrGC0308Reg(0x33, 0xe0);
    wrGC0308Reg(0x34, 0xe0);
    wrGC0308Reg(0x35, 0xb0);
    wrGC0308Reg(0x36, 0xc0);
    wrGC0308Reg(0x37, 0xc0);
    wrGC0308Reg(0x38, 0x04);
    wrGC0308Reg(0x39, 0x09);
    wrGC0308Reg(0x3a, 0x12);
    wrGC0308Reg(0x3b, 0x1C);
    wrGC0308Reg(0x3c, 0x28);
    wrGC0308Reg(0x3d, 0x31);
    wrGC0308Reg(0x3e, 0x44);
    wrGC0308Reg(0x3f, 0x57);
    wrGC0308Reg(0x40, 0x6C);
    wrGC0308Reg(0x41, 0x81);
    wrGC0308Reg(0x42, 0x94);
    wrGC0308Reg(0x43, 0xA7);
    wrGC0308Reg(0x44, 0xB8);
    wrGC0308Reg(0x45, 0xD6);
    wrGC0308Reg(0x46, 0xEE);
    wrGC0308Reg(0x47, 0x0d);


    wrGC0308Reg(0xfe, 0x00); //select page0---------------------------------------------------------------
    wrGC0308Reg(0xd2, 0x90);
    wrGC0308Reg(0xfe, 0x00);
    wrGC0308Reg(0x10, 0x26);
    wrGC0308Reg(0x11, 0x0d);
    wrGC0308Reg(0x1a, 0x2a);
    wrGC0308Reg(0x1c, 0x49);
    wrGC0308Reg(0x1d, 0x9a);
    wrGC0308Reg(0x1e, 0x61);
    wrGC0308Reg(0x3a, 0x20);
    wrGC0308Reg(0x50, 0x14);
    wrGC0308Reg(0x53, 0x80);
    wrGC0308Reg(0x56, 0x80);
    wrGC0308Reg(0x8b, 0x20);
    wrGC0308Reg(0x8c, 0x20);
    wrGC0308Reg(0x8d, 0x20);
    wrGC0308Reg(0x8e, 0x14);
    wrGC0308Reg(0x8f, 0x10);
    wrGC0308Reg(0x90, 0x14);
    wrGC0308Reg(0x94, 0x02);
    wrGC0308Reg(0x95, 0x07);
    wrGC0308Reg(0x96, 0xe0);
    wrGC0308Reg(0xb1, 0x40);
    wrGC0308Reg(0xb2, 0x40);
    wrGC0308Reg(0xb3, 0x40);



    wrGC0308Reg(0xb6, 0xe0);
    /* wrGC0308Reg(0xd0, 0xcb); */
    /* wrGC0308Reg(0xd3, 0x48); */
    wrGC0308Reg(0xf2, 0x02);
    wrGC0308Reg(0xf7, 0x12);
    wrGC0308Reg(0xf8, 0x0a);
    wrGC0308Reg(0x57, 0x73);
    wrGC0308Reg(0x58, 0x80);
    wrGC0308Reg(0x59, 0x80);

    wrGC0308Reg(0xfe, 0x01); //select page1---------------------------------------------------------------
    wrGC0308Reg(0x02, 0x20);
    wrGC0308Reg(0x04, 0x10);
    wrGC0308Reg(0x05, 0x08);
    wrGC0308Reg(0x06, 0x20);
    wrGC0308Reg(0x08, 0x0a);
    wrGC0308Reg(0x0e, 0x44);
    wrGC0308Reg(0x0f, 0x32);
    wrGC0308Reg(0x10, 0x41);
    wrGC0308Reg(0x11, 0x37);
    wrGC0308Reg(0x12, 0x22);
    wrGC0308Reg(0x13, 0x19);
    wrGC0308Reg(0x14, 0x44);
    wrGC0308Reg(0x15, 0x44);
    wrGC0308Reg(0x19, 0x50);
    wrGC0308Reg(0x1a, 0xd8);
    wrGC0308Reg(0x32, 0x10);
    wrGC0308Reg(0x35, 0x00);
    wrGC0308Reg(0x36, 0x80);
    wrGC0308Reg(0x37, 0x00);

    wrGC0308Reg(0xfe, 0x00); //select page0---------------------------------------------------------------

    wrGC0308Reg(0xfe, 0x00);
    //wrGC0308Reg(0x28,0x10);//0 001 0000
    wrGC0308Reg(0x28, 0x00); //0 001 0000
    //wrGC0308Reg(0x20,0xff);
    //wrGC0308Reg(0x02,0x04);

#if (CONFIG_INPUT_FPS == 0)
    //5 fps
    wrGC0308Reg(0x0f, 0x24);
    wrGC0308Reg(0x01, 0x80);
    wrGC0308Reg(0x02, 0xff);
#elif (CONFIG_INPUT_FPS == 10)

    //10 fps
    wrGC0308Reg(0x0f, 0x11);
    wrGC0308Reg(0x01, 0xff);
    wrGC0308Reg(0x02, 0xff);
#elif (CONFIG_INPUT_FPS == 15)

    //15 fps
    wrGC0308Reg(0x0f, 0x01);
    wrGC0308Reg(0x01, 0x80);
    wrGC0308Reg(0x02, 0xf8);
#elif (CONFIG_INPUT_FPS == 20)

    //20 fps
    wrGC0308Reg(0x0f, 0x00);
    wrGC0308Reg(0x01, 0x80);
    wrGC0308Reg(0x02, 0xf8);
#else

    //25 fps
    wrGC0308Reg(0x12, 0x10);
    wrGC0308Reg(0x0f, 0x00);
    wrGC0308Reg(0x01, 0x40);
    wrGC0308Reg(0x02, 0x10);
#endif

    puts("\nGC0308 UYVY\n");
    *format = SEN_IN_FORMAT_UYVY;
#endif
}

static s32 GC0308_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width + 8;
    u16 liv_height = *height + 8;

    wrGC0308Reg(0x09, liv_height >> 8);
    wrGC0308Reg(0x0a, liv_height & 0xff);
    wrGC0308Reg(0x0b, liv_width >> 8);
    wrGC0308Reg(0x0c, liv_width & 0xff);

    printf("GC0308 : %d , %d \n", *width, *height);
    return 0;
}

static s32 GC0308_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 GC0308_ID_check(void)
{
    u8 pid = 0x00;
    rdGC0308Reg(0x00, &pid);
    rdGC0308Reg(0x00, &pid);
    printf("GC0308 Sensor ID : 0x%x\n", pid);
    if (pid != 0x9b) {
        return -1;
    }

    return 0;
}

static void GC0308_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void GC0308_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("GC0308 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = (u8)gc0308_reset_io[0];
        powd_io = (u8)gc0308_power_io[0];
    } else {
        res_io = (u8)gc0308_reset_io[1];
        powd_io = (u8)gc0308_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        GC0308_powerio_ctl((u32)powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}


static s32 GC0308_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        gc0308_reset_io[isp_dev] = (u8)_reset_gpio;
        gc0308_power_io[isp_dev] = (u8)_power_gpio;
    }
    if (iic == NULL) {
        printf("GC0308 iic open err!!!\n\n");
        return -1;
    }
    GC0308_reset(isp_dev);

    if (0 != GC0308_ID_check()) {
        printf("-------not GC0308------\n\n");
        dev_close(iic);
        iic = NULL;
        return -1;
    }
    printf("-------hello GC0308------\n\n");
    return 0;
}


static s32 GC0308_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n GC0308_init \n\n");

    if (0 != GC0308_check(isp_dev, 0, 0)) {
        return -1;
    }
    GC0308_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_GC0308(u16 rev_flag)
{
    if (!rev_flag) {
        wrGC0308Reg(0x14, 0x13);
    } else {
        wrGC0308Reg(0x14, 0x10);
    }
}

u16 GC0308_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdGC0308Reg((u8)addr, &val);
    return val;
}

void GC0308_dvp_wr_reg(u16 addr, u16 val)
{
    wrGC0308Reg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA(GC0308) = {
    .logo 				= 	"GC0308",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_UYVY,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B  | SEN_MBUS_HSYNC_ACTIVE_HIGH | \
    						SEN_MBUS_PCLK_SAMPLE_FALLING | SEN_MBUS_VSYNC_ACTIVE_HIGH,
#if CONFIG_CAMERA_H_V_EXCHANGE
    .sync_config		=   SEN_MBUS_SYNC0_VSYNC_SYNC1_HSYNC,//WL82/AC791才可以H-V SYNC互换，请留意
#endif
    .fps         		= 	CONFIG_INPUT_FPS,
    .out_fps			=   CONFIG_INPUT_FPS,
    .sen_size 			= 	{GC0308_DEVP_INPUT_W, GC0308_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{GC0308_DEVP_INPUT_W, GC0308_DEVP_INPUT_H},


    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	GC0308_check,
        .init 		        = 	GC0308_init,
        .set_size_fps 		=	GC0308_set_output_size,
        .power_ctrl         =   GC0308_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	GC0308_dvp_wr_reg,
        .read_reg 		    =	GC0308_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_GC0308,
    }
};

