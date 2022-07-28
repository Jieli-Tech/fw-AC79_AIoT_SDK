#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gc0328.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"



#if (CONFIG_VIDEO_IMAGE_W > 640)
#define GC0328_DEVP_INPUT_W 640
#define GC0328_DEVP_INPUT_H 480
#else
#define GC0328_DEVP_INPUT_W CONFIG_VIDEO_IMAGE_W
#define GC0328_DEVP_INPUT_H CONFIG_VIDEO_IMAGE_H
#endif

#define CONFIG_INPUT_FPS 	20


static void *iic = NULL;
static u8 gc0328_reset_io[2] = {-1, -1};
static u8 gc0328_power_io[2] = {-1, -1};

#define GC0328_WRCMD 0x42
#define GC0328_RDCMD 0x43

#define DELAY_TIME	10

static s32 GC0328_set_output_size(u16 *width, u16 *height, u8 *freq);


static unsigned char wrGC0328Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0328_WRCMD)) {
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

static unsigned char rdGC0328Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0328_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0328_RDCMD)) {
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

static void GC0328_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("GC0328_config_SENSOR \n");
    wrGC0328Reg(0xfe, 0x80);
    wrGC0328Reg(0xfe, 0x80);
    wrGC0328Reg(0xfc, 0x16);
    wrGC0328Reg(0xfc, 0x16);
    wrGC0328Reg(0xfc, 0x16);
    wrGC0328Reg(0xfc, 0x16);
    wrGC0328Reg(0xf1, 0x00);
    wrGC0328Reg(0xf2, 0x00);
    wrGC0328Reg(0xfe, 0x00);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x03, 0x00);
    wrGC0328Reg(0x04, 0xc0);
    wrGC0328Reg(0x42, 0x00);
    wrGC0328Reg(0x77, 0x5a);
    wrGC0328Reg(0x78, 0x40);
    wrGC0328Reg(0x79, 0x56);

    wrGC0328Reg(0xfe, 0x00);
    /*wrGC0328Reg(0x0d , 0x01);*/
    /*wrGC0328Reg(0x0e , 0xe8);*/
    /*wrGC0328Reg(0x0f , 0x02);*/
    /*wrGC0328Reg(0x10 , 0x88);*/
    wrGC0328Reg(0x09, 0x00);
    wrGC0328Reg(0x0a, 0x00);
    wrGC0328Reg(0x0b, 0x00);
    wrGC0328Reg(0x0c, 0x00);
    wrGC0328Reg(0x16, 0x00);
    wrGC0328Reg(0x17, 0x14);
    wrGC0328Reg(0x18, 0x0e);
    wrGC0328Reg(0x19, 0x06);

    wrGC0328Reg(0x1b, 0x48);
    wrGC0328Reg(0x1f, 0xC8);
    wrGC0328Reg(0x20, 0x01);
    wrGC0328Reg(0x21, 0x78);
    wrGC0328Reg(0x22, 0xb0);
    wrGC0328Reg(0x23, 0x04); // 0x06 20140519 lanking GC0328C
    wrGC0328Reg(0x24, 0x11);
    wrGC0328Reg(0x26, 0x00);
    wrGC0328Reg(0x50, 0x01);  //crop mode

    //global gain for range
    wrGC0328Reg(0x70, 0x45);
#if (CONFIG_INPUT_FPS == 10)
    /////////////banding/////////////
    wrGC0328Reg(0x05, 0x01); //hb
    wrGC0328Reg(0x06, 0x32); //
    wrGC0328Reg(0x07, 0x00); //vb
    wrGC0328Reg(0x08, 0xe8); //
    wrGC0328Reg(0xfe, 0x01); //
    wrGC0328Reg(0x29, 0x00); //anti-flicker step [11:8]
    wrGC0328Reg(0x2a, 0x78); //anti-flicker step [7:0]
    //10fps
    wrGC0328Reg(0x2b, 0x04); //exp level 0  15fps
    wrGC0328Reg(0x2c, 0xd0); //
    wrGC0328Reg(0x2d, 0x04); //exp level 1
    wrGC0328Reg(0x2e, 0xd0); //
    wrGC0328Reg(0x2f, 0x04); //exp level 2
    wrGC0328Reg(0x30, 0xd0); //
    wrGC0328Reg(0x31, 0x04); //exp level 3
    wrGC0328Reg(0x32, 0xd0); //
#elif (CONFIG_INPUT_FPS == 15)
    /////////////banding/////////////
    wrGC0328Reg(0x05, 0x01); //hb
    wrGC0328Reg(0x06, 0x32); //
    wrGC0328Reg(0x07, 0x00); //vb
    wrGC0328Reg(0x08, 0xe8); //
    wrGC0328Reg(0xfe, 0x01); //
    wrGC0328Reg(0x29, 0x00); //anti-flicker step [11:8]
    wrGC0328Reg(0x2a, 0x78); //anti-flicker step [7:0]
    //15fps
    wrGC0328Reg(0x2b, 0x02); //exp level 0  15fps
    wrGC0328Reg(0x2c, 0xd0); //
    wrGC0328Reg(0x2d, 0x02); //exp level 1
    wrGC0328Reg(0x2e, 0xd0); //
    wrGC0328Reg(0x2f, 0x02); //exp level 2
    wrGC0328Reg(0x30, 0xd0); //
    wrGC0328Reg(0x31, 0x02); //exp level 3
    wrGC0328Reg(0x32, 0xd0); //
#elif (CONFIG_INPUT_FPS == 20)
    /////////////banding/////////////
    wrGC0328Reg(0x05, 0x01); //hb
    wrGC0328Reg(0x06, 0x32); //
    wrGC0328Reg(0x07, 0x00); //vb
    wrGC0328Reg(0x08, 0x5c); //
    wrGC0328Reg(0xfe, 0x01); //
    wrGC0328Reg(0x29, 0x00); //anti-flicker step [11:8]
    wrGC0328Reg(0x2a, 0x78); //anti-flicker step [7:0]
    //==================20FPS=====================
    //20fps
    wrGC0328Reg(0x2b, 0x02); //exp level 0  20fps
    wrGC0328Reg(0x2c, 0x58); //
    wrGC0328Reg(0x2d, 0x02); //exp level 1
    wrGC0328Reg(0x2e, 0x58); //
    wrGC0328Reg(0x2f, 0x02); //exp level 2
    wrGC0328Reg(0x30, 0x58); //
    wrGC0328Reg(0x31, 0x02); //exp level 3
    wrGC0328Reg(0x32, 0x58); //
    //===================20fps-end===================
#elif (CONFIG_INPUT_FPS == 25)
    /////////////banding/////////////
    wrGC0328Reg(0x05, 0x00); //hb
    wrGC0328Reg(0x06, 0xde); //
    wrGC0328Reg(0x07, 0x00); //vb
    wrGC0328Reg(0x08, 0x24); //
    wrGC0328Reg(0xfe, 0x01); //
    wrGC0328Reg(0x29, 0x00); //anti-flicker step [11:8]
    wrGC0328Reg(0x2a, 0x83); //anti-flicker step [7:0]
    //=========25FPS=====================
    wrGC0328Reg(0x2b, 0x02); //exp level 0  25fps
    wrGC0328Reg(0x2c, 0x0c); //
    wrGC0328Reg(0x2d, 0x02); //exp level 1
    wrGC0328Reg(0x2e, 0x0c); //
    wrGC0328Reg(0x2f, 0x02); //exp level 2
    wrGC0328Reg(0x30, 0x0c); //
    wrGC0328Reg(0x31, 0x02); //exp level 3
    wrGC0328Reg(0x32, 0x0c); //
#endif

    wrGC0328Reg(0xfe, 0x00); //

    ///////////////AWB//////////////
    wrGC0328Reg(0xfe, 0x01);
    wrGC0328Reg(0x50, 0x00);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4c, 0x01);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4d, 0x30);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4d, 0x40);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4d, 0x50);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4d, 0x60);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4d, 0x70);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4f, 0x01);
    wrGC0328Reg(0x50, 0x88);
    wrGC0328Reg(0xfe, 0x00);

    //////////// BLK//////////////////////
    wrGC0328Reg(0xfe, 0x00);
    wrGC0328Reg(0x27, 0xb7);
    wrGC0328Reg(0x28, 0x7F);
    wrGC0328Reg(0x29, 0x20);
    wrGC0328Reg(0x33, 0x20);
    wrGC0328Reg(0x34, 0x20);
    wrGC0328Reg(0x35, 0x20);
    wrGC0328Reg(0x36, 0x20);
    wrGC0328Reg(0x32, 0x08);
    wrGC0328Reg(0x3b, 0x00);
    wrGC0328Reg(0x3c, 0x00);
    wrGC0328Reg(0x3d, 0x00);
    wrGC0328Reg(0x3e, 0x00);
    wrGC0328Reg(0x47, 0x00);
    wrGC0328Reg(0x48, 0x00);

    //////////// block enable/////////////
    wrGC0328Reg(0x40, 0x7f);
    wrGC0328Reg(0x41, 0x26);
    wrGC0328Reg(0x42, 0xfb);
    wrGC0328Reg(0x44, 0x00);  //uyvy
    wrGC0328Reg(0x45, 0x00);
    wrGC0328Reg(0x46, 0x03);
    wrGC0328Reg(0x4f, 0x01);
    wrGC0328Reg(0x4b, 0x01);
    wrGC0328Reg(0x50, 0x01);

    /////DN & EEINTP/////
    wrGC0328Reg(0x7e, 0x0a);
    wrGC0328Reg(0x7f, 0x03);
    wrGC0328Reg(0x80, 0x27);  //  20140915
    wrGC0328Reg(0x81, 0x15);
    wrGC0328Reg(0x82, 0x90);
    wrGC0328Reg(0x83, 0x02);
    wrGC0328Reg(0x84, 0x23);  // 0x22 20140915
    wrGC0328Reg(0x90, 0x2c);
    wrGC0328Reg(0x92, 0x02);
    wrGC0328Reg(0x94, 0x02);
    wrGC0328Reg(0x95, 0x35);

    ////////////YCP///////////
    wrGC0328Reg(0xd1, 0x24); // 0x30 for front
    wrGC0328Reg(0xd2, 0x24); // 0x30 for front
    wrGC0328Reg(0xd3, 0x40);
    wrGC0328Reg(0xdd, 0xd3);
    wrGC0328Reg(0xde, 0x38);
    wrGC0328Reg(0xe4, 0x88);
    wrGC0328Reg(0xe5, 0x40);
    wrGC0328Reg(0xd7, 0x0e);

    ///////////rgb gamma ////////////
    wrGC0328Reg(0xfe, 0x00);
    wrGC0328Reg(0xbf, 0x0e);
    wrGC0328Reg(0xc0, 0x1c);
    wrGC0328Reg(0xc1, 0x34);
    wrGC0328Reg(0xc2, 0x48);
    wrGC0328Reg(0xc3, 0x5a);
    wrGC0328Reg(0xc4, 0x6e);
    wrGC0328Reg(0xc5, 0x80);
    wrGC0328Reg(0xc6, 0x9c);
    wrGC0328Reg(0xc7, 0xb4);
    wrGC0328Reg(0xc8, 0xc7);
    wrGC0328Reg(0xc9, 0xd7);
    wrGC0328Reg(0xca, 0xe3);
    wrGC0328Reg(0xcb, 0xed);
    wrGC0328Reg(0xcc, 0xf2);
    wrGC0328Reg(0xcd, 0xf8);
    wrGC0328Reg(0xce, 0xfd);
    wrGC0328Reg(0xcf, 0xff);

    /////////////Y gamma//////////
    wrGC0328Reg(0xfe, 0x00);
    wrGC0328Reg(0x63, 0x00);
    wrGC0328Reg(0x64, 0x05);
    wrGC0328Reg(0x65, 0x0b);
    wrGC0328Reg(0x66, 0x19);
    wrGC0328Reg(0x67, 0x2e);
    wrGC0328Reg(0x68, 0x40);
    wrGC0328Reg(0x69, 0x54);
    wrGC0328Reg(0x6a, 0x66);
    wrGC0328Reg(0x6b, 0x86);
    wrGC0328Reg(0x6c, 0xa7);
    wrGC0328Reg(0x6d, 0xc6);
    wrGC0328Reg(0x6e, 0xe4);
    wrGC0328Reg(0x6f, 0xff);

    //////////////ASDE/////////////
    wrGC0328Reg(0xfe, 0x01);
    wrGC0328Reg(0x18, 0x02);
    wrGC0328Reg(0xfe, 0x00);
    wrGC0328Reg(0x97, 0x30);
    wrGC0328Reg(0x98, 0x00);
    wrGC0328Reg(0x9b, 0x60);
    wrGC0328Reg(0x9c, 0x60);
    wrGC0328Reg(0xa4, 0x50);
    wrGC0328Reg(0xa8, 0x80);
    wrGC0328Reg(0xaa, 0x40);
    wrGC0328Reg(0xa2, 0x23);
    wrGC0328Reg(0xad, 0x28);

    //////////////abs///////////
    wrGC0328Reg(0xfe, 0x01);
    wrGC0328Reg(0x9c, 0x00);
    wrGC0328Reg(0x9e, 0xc0);
    wrGC0328Reg(0x9f, 0x40);

    ////////////// AEC////////////
    wrGC0328Reg(0xfe, 0x01);
    wrGC0328Reg(0x08, 0xa0);
    wrGC0328Reg(0x09, 0xe8);
    wrGC0328Reg(0x10, 0x08);
    wrGC0328Reg(0x11, 0x21);
    wrGC0328Reg(0x12, 0x11);
    wrGC0328Reg(0x13, 0x45);
    wrGC0328Reg(0x15, 0xfc);
    wrGC0328Reg(0x18, 0x02);
    wrGC0328Reg(0x21, 0xf0);
    wrGC0328Reg(0x22, 0x60);
    wrGC0328Reg(0x23, 0x30);
    wrGC0328Reg(0x25, 0x00);
    wrGC0328Reg(0x24, 0x14);
    wrGC0328Reg(0x3d, 0x80);
    wrGC0328Reg(0x3e, 0x40);

    ////////////////AWB///////////
    wrGC0328Reg(0xfe, 0x01);
    wrGC0328Reg(0x51, 0x88);
    wrGC0328Reg(0x52, 0x12);
    wrGC0328Reg(0x53, 0x80);
    wrGC0328Reg(0x54, 0x60);
    wrGC0328Reg(0x55, 0x01);
    wrGC0328Reg(0x56, 0x02);
    wrGC0328Reg(0x58, 0x00);
    wrGC0328Reg(0x5b, 0x02);
    wrGC0328Reg(0x5e, 0xa4);
    wrGC0328Reg(0x5f, 0x8a);
    wrGC0328Reg(0x61, 0xdc);
    wrGC0328Reg(0x62, 0xdc);
    wrGC0328Reg(0x70, 0xfc);
    wrGC0328Reg(0x71, 0x10);
    wrGC0328Reg(0x72, 0x30);
    wrGC0328Reg(0x73, 0x0b);
    wrGC0328Reg(0x74, 0x0b);
    wrGC0328Reg(0x75, 0x01);
    wrGC0328Reg(0x76, 0x00);
    wrGC0328Reg(0x77, 0x40);
    wrGC0328Reg(0x78, 0x70);
    wrGC0328Reg(0x79, 0x00);
    wrGC0328Reg(0x7b, 0x00);
    wrGC0328Reg(0x7c, 0x71);
    wrGC0328Reg(0x7d, 0x00);
    wrGC0328Reg(0x80, 0x70);
    wrGC0328Reg(0x81, 0x58);
    wrGC0328Reg(0x82, 0x98);
    wrGC0328Reg(0x83, 0x60);
    wrGC0328Reg(0x84, 0x58);
    wrGC0328Reg(0x85, 0x50);
    wrGC0328Reg(0xfe, 0x00);

    ////////////////LSC////////////////
    wrGC0328Reg(0xfe, 0x01);
    wrGC0328Reg(0xc0, 0x10);
    wrGC0328Reg(0xc1, 0x0c);
    wrGC0328Reg(0xc2, 0x0a);
    wrGC0328Reg(0xc6, 0x0e);
    wrGC0328Reg(0xc7, 0x0b);
    wrGC0328Reg(0xc8, 0x0a);
    wrGC0328Reg(0xba, 0x26);
    wrGC0328Reg(0xbb, 0x1c);
    wrGC0328Reg(0xbc, 0x1d);
    wrGC0328Reg(0xb4, 0x23);
    wrGC0328Reg(0xb5, 0x1c);
    wrGC0328Reg(0xb6, 0x1a);
    wrGC0328Reg(0xc3, 0x00);
    wrGC0328Reg(0xc4, 0x00);
    wrGC0328Reg(0xc5, 0x00);
    wrGC0328Reg(0xc9, 0x00);
    wrGC0328Reg(0xca, 0x00);
    wrGC0328Reg(0xcb, 0x00);
    wrGC0328Reg(0xbd, 0x00);
    wrGC0328Reg(0xbe, 0x00);
    wrGC0328Reg(0xbf, 0x00);
    wrGC0328Reg(0xb7, 0x07);
    wrGC0328Reg(0xb8, 0x05);
    wrGC0328Reg(0xb9, 0x05);
    wrGC0328Reg(0xa8, 0x07);
    wrGC0328Reg(0xa9, 0x06);
    wrGC0328Reg(0xaa, 0x00);
    wrGC0328Reg(0xab, 0x04);
    wrGC0328Reg(0xac, 0x00);
    wrGC0328Reg(0xad, 0x02);
    wrGC0328Reg(0xae, 0x0d);
    wrGC0328Reg(0xaf, 0x05);
    wrGC0328Reg(0xb0, 0x00);
    wrGC0328Reg(0xb1, 0x07);
    wrGC0328Reg(0xb2, 0x03);
    wrGC0328Reg(0xb3, 0x00);
    wrGC0328Reg(0xa4, 0x00);
    wrGC0328Reg(0xa5, 0x00);
    wrGC0328Reg(0xa6, 0x00);
    wrGC0328Reg(0xa7, 0x00);
    wrGC0328Reg(0xa1, 0x3c);
    wrGC0328Reg(0xa2, 0x50);
    wrGC0328Reg(0xfe, 0x00);

    ///////////////CCT ///////////
    wrGC0328Reg(0xb1, 0x12);
    wrGC0328Reg(0xb2, 0xf5);
    wrGC0328Reg(0xb3, 0xfe);
    wrGC0328Reg(0xb4, 0xe0);
    wrGC0328Reg(0xb5, 0x15);
    wrGC0328Reg(0xb6, 0xc8);

    /*   /////skin CC for front //////
    wrGC0328Reg(0xb1 , 0x00);
    wrGC0328Reg(0xb2 , 0x00);
    wrGC0328Reg(0xb3 , 0x00);
    wrGC0328Reg(0xb4 , 0xf0);
    wrGC0328Reg(0xb5 , 0x00);
    wrGC0328Reg(0xb6 , 0x00);
    */

    ///////////////AWB////////////////
    wrGC0328Reg(0xfe, 0x01);
    wrGC0328Reg(0x50, 0x00);
    wrGC0328Reg(0xfe, 0x01);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4c, 0x01);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4f, 0x00);
    wrGC0328Reg(0x4d, 0x34);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x02);
    wrGC0328Reg(0x4e, 0x02);
    wrGC0328Reg(0x4d, 0x44);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4d, 0x53);
    wrGC0328Reg(0x4e, 0x00);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4d, 0x65);
    wrGC0328Reg(0x4e, 0x04);
    wrGC0328Reg(0x4d, 0x73);
    wrGC0328Reg(0x4e, 0x20);
    wrGC0328Reg(0x4d, 0x83);
    wrGC0328Reg(0x4e, 0x20);
    wrGC0328Reg(0x4f, 0x01);
    wrGC0328Reg(0x50, 0x88);
    /////////output////////
    wrGC0328Reg(0xfe, 0x00);
    wrGC0328Reg(0xf1, 0x07);
    wrGC0328Reg(0xf2, 0x01);
}
static s32 GC0328_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width + 8;
    u16 liv_height = *height + 8;

    wrGC0328Reg(0x0d, liv_height >> 8);
    wrGC0328Reg(0x0e, liv_height & 0xff);
    wrGC0328Reg(0x0f, liv_width >> 8);
    wrGC0328Reg(0x10, liv_width & 0xff);
    return 0;
}

static s32 GC0328_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 GC0328_ID_check(void)
{
    u8 pid = 0x00;
    rdGC0328Reg(0xf0, &pid);
    rdGC0328Reg(0xf0, &pid);
    printf("GC0328 Sensor ID : 0x%x\n", pid);
    if (pid != 0x9d) {
        return -1;
    }

    return 0;
}

static void GC0328_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void GC0328_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("GC0328 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = gc0328_reset_io[0];
        powd_io = gc0328_power_io[0];
    } else {
        res_io = gc0328_reset_io[1];
        powd_io = gc0328_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        GC0328_powerio_ctl(powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}

s32 GC0328_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        gc0328_reset_io[isp_dev] = (u8)_reset_gpio;
        gc0328_power_io[isp_dev] = (u8)_power_gpio;
    }

    if (iic == NULL) {
        printf("GC0328 iic open err!!!\n\n");
        return -1;
    }
    GC0328_reset(isp_dev);

    if (0 != GC0328_ID_check()) {
        printf("-------not GC0328------\n\n");
        dev_close(iic);
        iic = NULL;
        return -1;
    }

    printf("-------hello GC0328------\n\n");
    return 0;
}


static s32 GC0328_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n GC0328_init \n\n");

    if (0 != GC0328_check(isp_dev, 0, 0)) {
        return -1;
    }

    GC0328_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_GC0328(u16 rev_flag)
{
    if (!iic) {
        printf("no iic \n");
        return;
    }
    if (!rev_flag) {
        wrGC0328Reg(0x17, 0x12);
    } else {
        wrGC0328Reg(0x17, 0x11);
    }
}

u16 GC0328_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdGC0328Reg((u8)addr, &val);
    return val;
}

void GC0328_dvp_wr_reg(u16 addr, u16 val)
{
    wrGC0328Reg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA(GC0328) = {
    .logo 				= 	"GC0328",
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
    .sen_size 			= 	{GC0328_DEVP_INPUT_W, GC0328_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{GC0328_DEVP_INPUT_W, GC0328_DEVP_INPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	GC0328_check,
        .init 		        = 	GC0328_init,
        .set_size_fps 		=	GC0328_set_output_size,
        .power_ctrl         =   GC0328_power_ctl,
        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	GC0328_dvp_wr_reg,
        .read_reg 		    =	GC0328_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_GC0328,
    }
};



