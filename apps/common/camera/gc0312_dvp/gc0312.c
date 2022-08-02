#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gc0312.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"



#if (CONFIG_VIDEO_IMAGE_W > 640)
#define GC0312_DEVP_INPUT_W 640
#define GC0312_DEVP_INPUT_H 480
#else
#define GC0312_DEVP_INPUT_W CONFIG_VIDEO_IMAGE_W
#define GC0312_DEVP_INPUT_H CONFIG_VIDEO_IMAGE_H
#endif

#define CONFIG_INPUT_FPS 	12


static void *iic = NULL;
static u8 gc0312_reset_io[2] = {-1, -1};
static u8 gc0312_power_io[2] = {-1, -1};

#define GC0312_WRCMD 0x42
#define GC0312_RDCMD 0x43


#define DELAY_TIME	10
static s32 GC0312_set_output_size(u16 *width, u16 *height, u8 *freq);


static unsigned char wrGC0312Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0312_WRCMD)) {
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

static unsigned char rdGC0312Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0312_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0312_RDCMD)) {
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

static void GC0312_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("GC0312_config_SENSOR \n");
    wrGC0312Reg(0xfe, 0xf0);
    wrGC0312Reg(0xfe, 0xf0);
    wrGC0312Reg(0xfe, 0x00);
    wrGC0312Reg(0xfc, 0x0e);
    wrGC0312Reg(0xfc, 0x0e);
    wrGC0312Reg(0xf2, 0x07);
    wrGC0312Reg(0xf3, 0x00); // output_disable
    wrGC0312Reg(0xf7, 0x1b);
    wrGC0312Reg(0xf8, 0x04);
    wrGC0312Reg(0xf9, 0x0e);
    wrGC0312Reg(0xfa, 0x11);

    /////////////////////////////////////////////////
    /////////////////  CISCTL reg	/////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0x00, 0x2f);
    wrGC0312Reg(0x01, 0x0f);//06
    wrGC0312Reg(0x02, 0x04);
    wrGC0312Reg(0x03, 0x03);
    wrGC0312Reg(0x04, 0x50);
    wrGC0312Reg(0x09, 0x00);
    wrGC0312Reg(0x0a, 0x00);
    wrGC0312Reg(0x0b, 0x00);
    wrGC0312Reg(0x0c, 0x04);
    GC0312_set_output_size(width, height, frame_freq);
    /*wrGC0312Reg(0x0d, 0x01);*/
    /*wrGC0312Reg(0x0e, 0xe8);*/
    /*wrGC0312Reg(0x0f, 0x02);*/
    /*wrGC0312Reg(0x10, 0x88);*/
    wrGC0312Reg(0x16, 0x00);
    wrGC0312Reg(0x17, 0x14);
    wrGC0312Reg(0x18, 0x1a);
    wrGC0312Reg(0x19, 0x14);
    wrGC0312Reg(0x1b, 0x48);
    wrGC0312Reg(0x1c, 0x1c);
    wrGC0312Reg(0x1e, 0x6b);
    wrGC0312Reg(0x1f, 0x28);
    wrGC0312Reg(0x20, 0x8b);//89 travis 20140801
    wrGC0312Reg(0x21, 0x49);
    wrGC0312Reg(0x22, 0xb0);
    wrGC0312Reg(0x23, 0x04);
    wrGC0312Reg(0x24, 0x16);
    wrGC0312Reg(0x34, 0x20);

    /////////////////////////////////////////////////
    ////////////////////   BLK	 ////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0x26, 0x23);
    wrGC0312Reg(0x28, 0xff);
    wrGC0312Reg(0x29, 0x00);
    wrGC0312Reg(0x32, 0x00);
    wrGC0312Reg(0x33, 0x10);
    wrGC0312Reg(0x37, 0x20);
    wrGC0312Reg(0x38, 0x10);
    wrGC0312Reg(0x47, 0x80);
    wrGC0312Reg(0x4e, 0x66);
    wrGC0312Reg(0xa8, 0x02);
    wrGC0312Reg(0xa9, 0x80);

    /////////////////////////////////////////////////
    //////////////////	ISP reg   ///////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0x40, 0xff);
    wrGC0312Reg(0x41, 0x21);
    wrGC0312Reg(0x42, 0xcf);
    wrGC0312Reg(0x44, 0x00);
    wrGC0312Reg(0x45, 0xa8);
    wrGC0312Reg(0x46, 0x03); //sync
    wrGC0312Reg(0x4a, 0x11);
    wrGC0312Reg(0x4b, 0x01);
    wrGC0312Reg(0x4c, 0x20);
    wrGC0312Reg(0x4d, 0x05);
    wrGC0312Reg(0x4f, 0x01);
    wrGC0312Reg(0x50, 0x01);
    wrGC0312Reg(0x55, 0x01);
    wrGC0312Reg(0x56, 0xe0);
    wrGC0312Reg(0x57, 0x02);
    wrGC0312Reg(0x58, 0x80);

    /////////////////////////////////////////////////
    ///////////////////   GAIN   ////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0x70, 0x70);
    wrGC0312Reg(0x5a, 0x84);
    wrGC0312Reg(0x5b, 0xc9);
    wrGC0312Reg(0x5c, 0xed);
    wrGC0312Reg(0x77, 0x74);
    wrGC0312Reg(0x78, 0x40);
    wrGC0312Reg(0x79, 0x5f);

    /////////////////////////////////////////////////
    ///////////////////   DNDD  /////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0x82, 0x14);
    wrGC0312Reg(0x83, 0x0b);
    wrGC0312Reg(0x89, 0xf0);

    /////////////////////////////////////////////////
    //////////////////   EEINTP  ////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0x8f, 0xaa);
    wrGC0312Reg(0x90, 0x8c);
    wrGC0312Reg(0x91, 0x90);
    wrGC0312Reg(0x92, 0x03);
    wrGC0312Reg(0x93, 0x03);
    wrGC0312Reg(0x94, 0x05);
    wrGC0312Reg(0x95, 0x65);
    wrGC0312Reg(0x96, 0xf0);

    /////////////////////////////////////////////////
    /////////////////////  ASDE  ////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0xfe, 0x00);

    wrGC0312Reg(0x9a, 0x20);
    wrGC0312Reg(0x9b, 0x80);
    wrGC0312Reg(0x9c, 0x40);
    wrGC0312Reg(0x9d, 0x80);

    wrGC0312Reg(0xa1, 0x30);
    wrGC0312Reg(0xa2, 0x32);
    wrGC0312Reg(0xa4, 0x80);//30 travis 20140929
    wrGC0312Reg(0xa5, 0x28);//30 travis 20140929
    wrGC0312Reg(0xaa, 0x30);//10 travis 20140929
    wrGC0312Reg(0xac, 0x22);

    /////////////////////////////////////////////////
    ///////////////////   GAMMA   ///////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0xfe, 0x00);//default
    wrGC0312Reg(0xbf, 0x08);
    wrGC0312Reg(0xc0, 0x16);
    wrGC0312Reg(0xc1, 0x28);
    wrGC0312Reg(0xc2, 0x41);
    wrGC0312Reg(0xc3, 0x5a);
    wrGC0312Reg(0xc4, 0x6c);
    wrGC0312Reg(0xc5, 0x7a);
    wrGC0312Reg(0xc6, 0x96);
    wrGC0312Reg(0xc7, 0xac);
    wrGC0312Reg(0xc8, 0xbc);
    wrGC0312Reg(0xc9, 0xc9);
    wrGC0312Reg(0xca, 0xd3);
    wrGC0312Reg(0xcb, 0xdd);
    wrGC0312Reg(0xcc, 0xe5);
    wrGC0312Reg(0xcd, 0xf1);
    wrGC0312Reg(0xce, 0xfa);
    wrGC0312Reg(0xcf, 0xff);

    /*
    	wrGC0312Reg(0xfe, 0x00);//big gamma
    	wrGC0312Reg(0xbf, 0x08);
    	wrGC0312Reg(0xc0, 0x1d);
    	wrGC0312Reg(0xc1, 0x34);
    	wrGC0312Reg(0xc2, 0x4b);
    	wrGC0312Reg(0xc3, 0x60);
    	wrGC0312Reg(0xc4, 0x73);
    	wrGC0312Reg(0xc5, 0x85);
    	wrGC0312Reg(0xc6, 0x9f);
    	wrGC0312Reg(0xc7, 0xb5);
    	wrGC0312Reg(0xc8, 0xc7);
    	wrGC0312Reg(0xc9, 0xd5);
    	wrGC0312Reg(0xca, 0xe0);
    	wrGC0312Reg(0xcb, 0xe7);
    	wrGC0312Reg(0xcc, 0xec);
    	wrGC0312Reg(0xcd, 0xf4);
    	wrGC0312Reg(0xce, 0xfa);
    	wrGC0312Reg(0xcf, 0xff);
    */

    /*
    	wrGC0312Reg(0xfe, 0x00);//small gamma
    	wrGC0312Reg(0xbf, 0x08);
    	wrGC0312Reg(0xc0, 0x18);
    	wrGC0312Reg(0xc1, 0x2c);
    	wrGC0312Reg(0xc2, 0x41);
    	wrGC0312Reg(0xc3, 0x59);
    	wrGC0312Reg(0xc4, 0x6e);
    	wrGC0312Reg(0xc5, 0x81);
    	wrGC0312Reg(0xc6, 0x9f);
    	wrGC0312Reg(0xc7, 0xb5);
    	wrGC0312Reg(0xc8, 0xc7);
    	wrGC0312Reg(0xc9, 0xd5);
    	wrGC0312Reg(0xca, 0xe0);
    	wrGC0312Reg(0xcb, 0xe7);
    	wrGC0312Reg(0xcc, 0xec);
    	wrGC0312Reg(0xcd, 0xf4);
    	wrGC0312Reg(0xce, 0xfa);
    	wrGC0312Reg(0xcf, 0xff);
    */
    /////////////////////////////////////////////////
    ///////////////////   YCP  //////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0xd0, 0x40);
    wrGC0312Reg(0xd1, 0x34);
    wrGC0312Reg(0xd2, 0x34);
    wrGC0312Reg(0xd3, 0x40);
    wrGC0312Reg(0xd6, 0xf2);
    wrGC0312Reg(0xd7, 0x1b);
    wrGC0312Reg(0xd8, 0x18);
    wrGC0312Reg(0xdd, 0x03);

    /////////////////////////////////////////////////
    ////////////////////   AEC   ////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0xfe, 0x01);
    wrGC0312Reg(0x05, 0x30);
    wrGC0312Reg(0x06, 0x75);
    wrGC0312Reg(0x07, 0x40);
    wrGC0312Reg(0x08, 0xb0);
    wrGC0312Reg(0x0a, 0xc5);
    wrGC0312Reg(0x0b, 0x11);
    wrGC0312Reg(0x0c, 0x00);
    wrGC0312Reg(0x12, 0x52);
    wrGC0312Reg(0x13, 0x38);
    wrGC0312Reg(0x18, 0x95);
    wrGC0312Reg(0x19, 0x96);
    wrGC0312Reg(0x1f, 0x20);
    wrGC0312Reg(0x20, 0xc0); //80
    wrGC0312Reg(0x3e, 0x40);
    wrGC0312Reg(0x3f, 0x57);
    wrGC0312Reg(0x40, 0x7d);
    wrGC0312Reg(0x03, 0x60);
    wrGC0312Reg(0x44, 0x02);

    /////////////////////////////////////////////////
    ////////////////////   AWB   ////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0xfe, 0x01);
    wrGC0312Reg(0x1c, 0x91);
    wrGC0312Reg(0x21, 0x15);
    wrGC0312Reg(0x50, 0x80);
    wrGC0312Reg(0x56, 0x04);
    wrGC0312Reg(0x59, 0x08);
    wrGC0312Reg(0x5b, 0x02);
    wrGC0312Reg(0x61, 0x8d);
    wrGC0312Reg(0x62, 0xa7);
    wrGC0312Reg(0x63, 0xd0);
    wrGC0312Reg(0x65, 0x06);
    wrGC0312Reg(0x66, 0x06);
    wrGC0312Reg(0x67, 0x84);
    wrGC0312Reg(0x69, 0x08);
    wrGC0312Reg(0x6a, 0x25);
    wrGC0312Reg(0x6b, 0x01);
    wrGC0312Reg(0x6c, 0x00);
    wrGC0312Reg(0x6d, 0x02);
    wrGC0312Reg(0x6e, 0xf0);
    wrGC0312Reg(0x6f, 0x80);
    wrGC0312Reg(0x76, 0x80);
    wrGC0312Reg(0x78, 0xaf);
    wrGC0312Reg(0x79, 0x75);
    wrGC0312Reg(0x7a, 0x40);
    wrGC0312Reg(0x7b, 0x50);
    wrGC0312Reg(0x7c, 0x0c);


    wrGC0312Reg(0x90, 0xc9); //stable AWB
    wrGC0312Reg(0x91, 0xbe);
    wrGC0312Reg(0x92, 0xe2);
    wrGC0312Reg(0x93, 0xc9);
    wrGC0312Reg(0x95, 0x1b);
    wrGC0312Reg(0x96, 0xe2);
    wrGC0312Reg(0x97, 0x49);
    wrGC0312Reg(0x98, 0x1b);
    wrGC0312Reg(0x9a, 0x49);
    wrGC0312Reg(0x9b, 0x1b);
    wrGC0312Reg(0x9c, 0xc3);
    wrGC0312Reg(0x9d, 0x49);
    wrGC0312Reg(0x9f, 0xc7);
    wrGC0312Reg(0xa0, 0xc8);
    wrGC0312Reg(0xa1, 0x00);
    wrGC0312Reg(0xa2, 0x00);
    wrGC0312Reg(0x86, 0x00);
    wrGC0312Reg(0x87, 0x00);
    wrGC0312Reg(0x88, 0x00);
    wrGC0312Reg(0x89, 0x00);
    wrGC0312Reg(0xa4, 0xb9);
    wrGC0312Reg(0xa5, 0xa0);
    wrGC0312Reg(0xa6, 0xba);
    wrGC0312Reg(0xa7, 0x92);
    wrGC0312Reg(0xa9, 0xba);
    wrGC0312Reg(0xaa, 0x80);
    wrGC0312Reg(0xab, 0x9d);
    wrGC0312Reg(0xac, 0x7f);
    wrGC0312Reg(0xae, 0xbb);
    wrGC0312Reg(0xaf, 0x9d);
    wrGC0312Reg(0xb0, 0xc8);
    wrGC0312Reg(0xb1, 0x97);
    wrGC0312Reg(0xb3, 0xb7);
    wrGC0312Reg(0xb4, 0x7f);
    wrGC0312Reg(0xb5, 0x00);
    wrGC0312Reg(0xb6, 0x00);
    wrGC0312Reg(0x8b, 0x00);
    wrGC0312Reg(0x8c, 0x00);
    wrGC0312Reg(0x8d, 0x00);
    wrGC0312Reg(0x8e, 0x00);
    wrGC0312Reg(0x94, 0x55);
    wrGC0312Reg(0x99, 0xa6);
    wrGC0312Reg(0x9e, 0xaa);
    wrGC0312Reg(0xa3, 0x0a);
    wrGC0312Reg(0x8a, 0x00);
    wrGC0312Reg(0xa8, 0x55);
    wrGC0312Reg(0xad, 0x55);
    wrGC0312Reg(0xb2, 0x55);
    wrGC0312Reg(0xb7, 0x05);
    wrGC0312Reg(0x8f, 0x00);
    wrGC0312Reg(0xb8, 0xcb);
    wrGC0312Reg(0xb9, 0x9b);

    /*
    wrGC0312Reg(0xa4,0xb9); //default AWB
    wrGC0312Reg(0xa5,0xa0);
    wrGC0312Reg(0x90,0xc9);
    wrGC0312Reg(0x91,0xbe);
    wrGC0312Reg(0xa6,0xb8);
    wrGC0312Reg(0xa7,0x95);
    wrGC0312Reg(0x92,0xe6);
    wrGC0312Reg(0x93,0xca);
    wrGC0312Reg(0xa9,0xbc);
    wrGC0312Reg(0xaa,0x95);
    wrGC0312Reg(0x95,0x23);
    wrGC0312Reg(0x96,0xe7);
    wrGC0312Reg(0xab,0x9d);
    wrGC0312Reg(0xac,0x80);
    wrGC0312Reg(0x97,0x43);
    wrGC0312Reg(0x98,0x24);
    wrGC0312Reg(0xae,0xb7);
    wrGC0312Reg(0xaf,0x9e);
    wrGC0312Reg(0x9a,0x43);
    wrGC0312Reg(0x9b,0x24);
    wrGC0312Reg(0xb0,0xc8);
    wrGC0312Reg(0xb1,0x97);
    wrGC0312Reg(0x9c,0xc4);
    wrGC0312Reg(0x9d,0x44);
    wrGC0312Reg(0xb3,0xb7);
    wrGC0312Reg(0xb4,0x7f);
    wrGC0312Reg(0x9f,0xc7);
    wrGC0312Reg(0xa0,0xc8);
    wrGC0312Reg(0xb5,0x00);
    wrGC0312Reg(0xb6,0x00);
    wrGC0312Reg(0xa1,0x00);
    wrGC0312Reg(0xa2,0x00);
    wrGC0312Reg(0x86,0x60);
    wrGC0312Reg(0x87,0x08);
    wrGC0312Reg(0x88,0x00);
    wrGC0312Reg(0x89,0x00);
    wrGC0312Reg(0x8b,0xde);
    wrGC0312Reg(0x8c,0x80);
    wrGC0312Reg(0x8d,0x00);
    wrGC0312Reg(0x8e,0x00);
    wrGC0312Reg(0x94,0x55);
    wrGC0312Reg(0x99,0xa6);
    wrGC0312Reg(0x9e,0xaa);
    wrGC0312Reg(0xa3,0x0a);
    wrGC0312Reg(0x8a,0x0a);
    wrGC0312Reg(0xa8,0x55);
    wrGC0312Reg(0xad,0x55);
    wrGC0312Reg(0xb2,0x55);
    wrGC0312Reg(0xb7,0x05);
    wrGC0312Reg(0x8f,0x05);
    wrGC0312Reg(0xb8,0xcc);
    wrGC0312Reg(0xb9,0x9a);
    */
    /////////////////////////////////////////////////
    ////////////////////   CC    ////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0xfe, 0x01);

    wrGC0312Reg(0xd0, 0x38);//skin red
    wrGC0312Reg(0xd1, 0x00);
    wrGC0312Reg(0xd2, 0x02);
    wrGC0312Reg(0xd3, 0x04);
    wrGC0312Reg(0xd4, 0x38);
    wrGC0312Reg(0xd5, 0x12);
    /*
    	wrGC0312Reg(0xd0, 0x38);//skin white
    	wrGC0312Reg(0xd1, 0xfd);
    	wrGC0312Reg(0xd2, 0x06);
    	wrGC0312Reg(0xd3, 0xf0);
    	wrGC0312Reg(0xd4, 0x40);
    	wrGC0312Reg(0xd5, 0x08);
    */

    /*
    	wrGC0312Reg(0xd0, 0x38);//guodengxiang
    	wrGC0312Reg(0xd1, 0xf8);
    	wrGC0312Reg(0xd2, 0x06);
    	wrGC0312Reg(0xd3, 0xfd);
    	wrGC0312Reg(0xd4, 0x40);
    	wrGC0312Reg(0xd5, 0x00);
    */
    wrGC0312Reg(0xd6, 0x30);
    wrGC0312Reg(0xd7, 0x00);
    wrGC0312Reg(0xd8, 0x0a);
    wrGC0312Reg(0xd9, 0x16);
    wrGC0312Reg(0xda, 0x39);
    wrGC0312Reg(0xdb, 0xf8);

    /////////////////////////////////////////////////
    ////////////////////   LSC   ////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0xfe, 0x01);
    wrGC0312Reg(0xc1, 0x3c);
    wrGC0312Reg(0xc2, 0x50);
    wrGC0312Reg(0xc3, 0x00);
    wrGC0312Reg(0xc4, 0x40);
    wrGC0312Reg(0xc5, 0x30);
    wrGC0312Reg(0xc6, 0x30);
    wrGC0312Reg(0xc7, 0x10);
    wrGC0312Reg(0xc8, 0x00);
    wrGC0312Reg(0xc9, 0x00);
    wrGC0312Reg(0xdc, 0x20);
    wrGC0312Reg(0xdd, 0x10);
    wrGC0312Reg(0xdf, 0x00);
    wrGC0312Reg(0xde, 0x00);

    /////////////////////////////////////////////////
    ///////////////////  Histogram	/////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0x01, 0x10);
    wrGC0312Reg(0x0b, 0x31);
    wrGC0312Reg(0x0e, 0x50);
    wrGC0312Reg(0x0f, 0x0f);
    wrGC0312Reg(0x10, 0x6e);
    wrGC0312Reg(0x12, 0xa0);
    wrGC0312Reg(0x15, 0x60);
    wrGC0312Reg(0x16, 0x60);
    wrGC0312Reg(0x17, 0xe0);

    /////////////////////////////////////////////////
    //////////////	Measure Window	  ///////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0xcc, 0x0c);
    wrGC0312Reg(0xcd, 0x10);
    wrGC0312Reg(0xce, 0xa0);
    wrGC0312Reg(0xcf, 0xe6);

    /////////////////////////////////////////////////
    /////////////////	dark sun   //////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0x45, 0xf7);
    wrGC0312Reg(0x46, 0xff);
    wrGC0312Reg(0x47, 0x15);
    wrGC0312Reg(0x48, 0x03);
    wrGC0312Reg(0x4f, 0x60);

    //////////////////banding//////////////////////
    wrGC0312Reg(0xfe, 0x00);
    /*wrGC0312Reg(0x05, 0x02);*/
    /*wrGC0312Reg(0x06, 0xd1); //HB*/
    /*wrGC0312Reg(0x07, 0x00);*/
    /*wrGC0312Reg(0x08, 0x22); //VB*/

#if (CONFIG_INPUT_FPS == 10)
    //10 fps
    wrGC0312Reg(0x05, 0x01);//HB-H
    wrGC0312Reg(0x06, 0x80);//HB-L
    wrGC0312Reg(0x07, 0x00);//VB-H
    wrGC0312Reg(0x08, 0xf8);//VB-L
#else
    //10 fps
    wrGC0312Reg(0x05, 0x00);//HB-H
    wrGC0312Reg(0x06, 0x90);//HB-L
    wrGC0312Reg(0x07, 0x00);//VB-H
    wrGC0312Reg(0x08, 0xa8);//VB-L
#endif

    wrGC0312Reg(0xfe, 0x01);
    wrGC0312Reg(0x25, 0x00);   //anti-flicker step [11:8]
    wrGC0312Reg(0x26, 0x6a);   //anti-flicker step [7:0]
    wrGC0312Reg(0x27, 0x02); //20fps
    wrGC0312Reg(0x28, 0x12);
    wrGC0312Reg(0x29, 0x03); //12.5fps
    wrGC0312Reg(0x2a, 0x50);
    wrGC0312Reg(0x2b, 0x05); //7.14fps
    wrGC0312Reg(0x2c, 0xcc);
    wrGC0312Reg(0x2d, 0x07); //5.55fps
    wrGC0312Reg(0x2e, 0x74);
    wrGC0312Reg(0x3c, 0x20);
    wrGC0312Reg(0xfe, 0x00);

    /////////////////////////////////////////////////
    /////////////////////  DVP   ////////////////////
    /////////////////////////////////////////////////
    wrGC0312Reg(0xfe, 0x03);
    wrGC0312Reg(0x01, 0x00);
    wrGC0312Reg(0x02, 0x00);
    wrGC0312Reg(0x10, 0x00);
    wrGC0312Reg(0x15, 0x00);
    wrGC0312Reg(0xfe, 0x00);
    ///////////////////OUTPUT//////////////////////
    wrGC0312Reg(0xf3, 0xff);// output_enable


}
static s32 GC0312_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width + 8;
    u16 liv_height = *height + 8;

    wrGC0312Reg(0x0d, liv_height >> 8);
    wrGC0312Reg(0x0e, liv_height & 0xff);
    wrGC0312Reg(0x0f, liv_width >> 8);
    wrGC0312Reg(0x10, liv_width & 0xff);

    return 0;
}

static s32 GC0312_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 GC0312_ID_check(void)
{
    u8 pid = 0x00;
    u16 id = 0;
    rdGC0312Reg(0xf0, &pid);
    id |= (pid << 8) & 0xff00;
    rdGC0312Reg(0xf1, &pid);
    id |= pid;
    printf("GC0312 Sensor ID : 0x%x\n", id);
    if (id != 0xb310) {
        return -1;
    }

    return 0;
}

static void GC0312_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void GC0312_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("GC0312 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = gc0312_reset_io[0];
        powd_io = gc0312_power_io[0];
    } else {
        res_io = gc0312_reset_io[1];
        powd_io = gc0312_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        GC0312_powerio_ctl(powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}

s32 GC0312_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        gc0312_reset_io[isp_dev] = (u8)_reset_gpio;
        gc0312_power_io[isp_dev] = (u8)_power_gpio;
    }

    if (iic == NULL) {
        printf("GC0312 iic open err!!!\n\n");
        return -1;
    }
    GC0312_reset(isp_dev);

    if (0 != GC0312_ID_check()) {
        printf("-------not GC0312------\n\n");
        dev_close(iic);
        iic = NULL;
        return -1;
    }

    printf("-------hello GC0312------\n\n");
    return 0;
}


static s32 GC0312_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n GC0312_init \n\n");

    if (0 != GC0312_check(isp_dev, 0, 0)) {
        return -1;
    }

    GC0312_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_GC0312(u16 rev_flag)
{
    if (!rev_flag) {
        wrGC0312Reg(0x17, 0x17);
    } else {
        wrGC0312Reg(0x17, 0x14);
    }
}

u16 GC0312_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdGC0312Reg((u8)addr, &val);
    return val;
}

void GC0312_dvp_wr_reg(u16 addr, u16 val)
{
    wrGC0312Reg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA(GC0312) = {
    .logo 				= 	"GC0312",
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
    .sen_size 			= 	{GC0312_DEVP_INPUT_W, GC0312_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{GC0312_DEVP_INPUT_W, GC0312_DEVP_INPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	GC0312_check,
        .init 		        = 	GC0312_init,
        .set_size_fps 		=	GC0312_set_output_size,
        .power_ctrl         =   GC0312_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	GC0312_dvp_wr_reg,
        .read_reg 		    =	GC0312_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_GC0312,
    }
};


