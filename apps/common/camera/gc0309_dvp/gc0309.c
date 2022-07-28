#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gc0309.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"



static void *iic = NULL;
static u8 gc0309_reset_io[2] = {-1, -1};
static u8 gc0309_power_io[2] = {-1, -1};

#if (CONFIG_VIDEO_IMAGE_W > 640)
#define GC0309_DEVP_INPUT_W 	640
#define GC0309_DEVP_INPUT_H		480
#else
#define GC0309_DEVP_INPUT_W 	CONFIG_VIDEO_IMAGE_W
#define GC0309_DEVP_INPUT_H		CONFIG_VIDEO_IMAGE_H
#endif

#define GC0309_WRCMD 0x42
#define GC0309_RDCMD 0x43


#define CONFIG_INPUT_FPS	10

#define DELAY_TIME	10
struct reginfo {
    u8 reg;
    u8 val;
};


static s32 GC0309_set_output_size(u16 *width, u16 *height, u8 *freq);
static unsigned char wrGC0309Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0309_WRCMD)) {
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

static unsigned char rdGC0309Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0309_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0309_RDCMD)) {
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

static void GC0309_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    int i;
    wrGC0309Reg(0xfe, 0x80);  // soft reset
    delay(DELAY_TIME);
    wrGC0309Reg(0xfe, 0x00);      // set page0
    delay(DELAY_TIME);
    wrGC0309Reg(0x1a, 0x16);
    wrGC0309Reg(0xd2, 0x10);  // close AEC
    wrGC0309Reg(0x22, 0x55);  // close AWB
    wrGC0309Reg(0x5a, 0x56);
    wrGC0309Reg(0x5b, 0x40);
    wrGC0309Reg(0x5c, 0x4a);
    wrGC0309Reg(0x22, 0x57);

    GC0309_set_output_size(width, height, frame_freq);


    /*wrGC0309Reg(0x01,0x26); 	*/
    /*wrGC0309Reg(0x02,0x98); */
    /*wrGC0309Reg(0x0f,0x03);*/
    wrGC0309Reg(0xe2, 0x00);
    wrGC0309Reg(0xe3, 0x50);
    wrGC0309Reg(0x03, 0x01);
    wrGC0309Reg(0x04, 0x40);
    wrGC0309Reg(0x05, 0x00);
    wrGC0309Reg(0x06, 0x00);
    wrGC0309Reg(0x07, 0x00);
    wrGC0309Reg(0x08, 0x00);
    /*wrGC0309Reg(0x09,0x01); */
    /*wrGC0309Reg(0x0a,0xe8); */
    /*wrGC0309Reg(0x0b,0x02); */
    /*wrGC0309Reg(0x0c,0x88); */

    wrGC0309Reg(0x0d, 0x02);
    wrGC0309Reg(0x0e, 0x02);
    wrGC0309Reg(0x10, 0x22);
    wrGC0309Reg(0x11, 0x0d);
    wrGC0309Reg(0x12, 0x2a);
    wrGC0309Reg(0x13, 0x00);
    wrGC0309Reg(0x14, 0x10);
    wrGC0309Reg(0x15, 0x0a);
    wrGC0309Reg(0x16, 0x05);
    wrGC0309Reg(0x17, 0x01);

    wrGC0309Reg(0x1b, 0x03);
    wrGC0309Reg(0x1c, 0xc1);
    wrGC0309Reg(0x1d, 0x08);
    wrGC0309Reg(0x1e, 0x20);
    wrGC0309Reg(0x1f, 0x16);

    wrGC0309Reg(0x20, 0xff);
    wrGC0309Reg(0x21, 0xf8);
    /*wrGC0309Reg(0x24,0xa2); */
    wrGC0309Reg(0x24, 0xa0);
    wrGC0309Reg(0x25, 0x0f);
    //output sync_mode
    wrGC0309Reg(0x26, 0x02);
    wrGC0309Reg(0x2f, 0x01);
    /////////////////////////////////////////////////////////////////////
    /////////////////////////// grab_t ////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    wrGC0309Reg(0x30, 0xf7);
    wrGC0309Reg(0x31, 0x40);
    wrGC0309Reg(0x32, 0x00);
    wrGC0309Reg(0x39, 0x04);
    wrGC0309Reg(0x3a, 0x20);
    wrGC0309Reg(0x3b, 0x20);
    wrGC0309Reg(0x3c, 0x02);
    wrGC0309Reg(0x3d, 0x02);
    wrGC0309Reg(0x3e, 0x02);
    wrGC0309Reg(0x3f, 0x02);

    //gain
    wrGC0309Reg(0x50, 0x24);

    wrGC0309Reg(0x53, 0x82);
    wrGC0309Reg(0x54, 0x80);
    wrGC0309Reg(0x55, 0x80);
    wrGC0309Reg(0x56, 0x82);

    /////////////////////////////////////////////////////////////////////
    /////////////////////////// LSC_t  ////////////////////////////////
    /////////////////////////////////////////////////////////////////////
    wrGC0309Reg(0x8b, 0x20);
    wrGC0309Reg(0x8c, 0x20);
    wrGC0309Reg(0x8d, 0x20);
    wrGC0309Reg(0x8e, 0x10);
    wrGC0309Reg(0x8f, 0x10);
    wrGC0309Reg(0x90, 0x10);
    wrGC0309Reg(0x91, 0x3c);
    wrGC0309Reg(0x92, 0x50);
    wrGC0309Reg(0x5d, 0x12);
    wrGC0309Reg(0x5e, 0x1a);
    wrGC0309Reg(0x5f, 0x24);
    /////////////////////////////////////////////////////////////////////
    /////////////////////////// DNDD_t  ///////////////////////////////
    /////////////////////////////////////////////////////////////////////
    wrGC0309Reg(0x60, 0x07);
    wrGC0309Reg(0x61, 0x0e);
    wrGC0309Reg(0x62, 0x0c);
    wrGC0309Reg(0x64, 0x03);
    wrGC0309Reg(0x66, 0xe8);
    wrGC0309Reg(0x67, 0x86);
    wrGC0309Reg(0x68, 0xa2);

    /////////////////////////////////////////////////////////////////////
    /////////////////////////// asde_t ///////////////////////////////
    /////////////////////////////////////////////////////////////////////
    wrGC0309Reg(0x69, 0x20);
    wrGC0309Reg(0x6a, 0x0f);
    wrGC0309Reg(0x6b, 0x00);
    wrGC0309Reg(0x6c, 0x53);
    wrGC0309Reg(0x6d, 0x83);
    wrGC0309Reg(0x6e, 0xac);
    wrGC0309Reg(0x6f, 0xac);
    wrGC0309Reg(0x70, 0x15);
    wrGC0309Reg(0x71, 0x33);
    /////////////////////////////////////////////////////////////////////
    /////////////////////////// eeintp_t///////////////////////////////
    /////////////////////////////////////////////////////////////////////
    wrGC0309Reg(0x72, 0xdc);
    wrGC0309Reg(0x73, 0x80);
    //for high resolution in light scene
    wrGC0309Reg(0x74, 0x02);
    wrGC0309Reg(0x75, 0x3f);
    wrGC0309Reg(0x76, 0x02);
    wrGC0309Reg(0x77, 0x54);
    wrGC0309Reg(0x78, 0x88);
    wrGC0309Reg(0x79, 0x81);
    wrGC0309Reg(0x7a, 0x81);
    wrGC0309Reg(0x7b, 0x22);
    wrGC0309Reg(0x7c, 0xff);

    /////////////////////////////////////////////////////////////////////
    ///////////////////////////CC_t///////////////////////////////
    /////////////////////////////////////////////////////////////////////
    wrGC0309Reg(0x93, 0x45);
    wrGC0309Reg(0x94, 0x00);
    wrGC0309Reg(0x95, 0x00);
    wrGC0309Reg(0x96, 0x00);
    wrGC0309Reg(0x97, 0x45);
    wrGC0309Reg(0x98, 0xf0);
    wrGC0309Reg(0x9c, 0x00);
    wrGC0309Reg(0x9d, 0x03);
    wrGC0309Reg(0x9e, 0x00);

    /////////////////////////////////////////////////////////////////////
    ///////////////////////////YCP_t///////////////////////////////
    /////////////////////////////////////////////////////////////////////
    wrGC0309Reg(0xb1, 0x40);
    wrGC0309Reg(0xb2, 0x40);
    wrGC0309Reg(0xb8, 0x20);
    wrGC0309Reg(0xbe, 0x36);
    wrGC0309Reg(0xbf, 0x00);
    /////////////////////////////////////////////////////////////////////
    ///////////////////////////AEC_t///////////////////////////////
    /////////////////////////////////////////////////////////////////////
    wrGC0309Reg(0xd0, 0xc9);
    wrGC0309Reg(0xd1, 0x10);
//	wrGC0309Reg(0xd2,0x90);
    wrGC0309Reg(0xd3, 0x80);
    wrGC0309Reg(0xd5, 0xf2);
    wrGC0309Reg(0xd6, 0x16);
    wrGC0309Reg(0xdb, 0x92);
    wrGC0309Reg(0xdc, 0xa5);
    wrGC0309Reg(0xdf, 0x23);
    wrGC0309Reg(0xd9, 0x00);
    wrGC0309Reg(0xda, 0x00);
    wrGC0309Reg(0xe0, 0x09);

    wrGC0309Reg(0xec, 0x20);
    wrGC0309Reg(0xed, 0x04);
    wrGC0309Reg(0xee, 0xa0);
    wrGC0309Reg(0xef, 0x40);
    ///////////////////////////////////////////////////////////////////
    ///////////////////////////GAMMA//////////////////////////////////
    ///////////////////////////////////////////////////////////////////
    wrGC0309Reg(0x8b, 0x22);  // lsc r
    wrGC0309Reg(0x71, 0x43);  // auto sat limit

    //cc
    wrGC0309Reg(0x93, 0x48);
    wrGC0309Reg(0x94, 0x00);
    wrGC0309Reg(0x95, 0x05);
    wrGC0309Reg(0x96, 0xe8);
    wrGC0309Reg(0x97, 0x40);
    wrGC0309Reg(0x98, 0xf8);
    wrGC0309Reg(0x9c, 0x00);
    wrGC0309Reg(0x9d, 0x00);
    wrGC0309Reg(0x9e, 0x00);

    wrGC0309Reg(0xd0, 0xcb); // aec before gamma
    wrGC0309Reg(0xd3, 0x50); // ae target
    wrGC0309Reg(0x31, 0x60);

    wrGC0309Reg(0x1c, 0x49);
    wrGC0309Reg(0x1d, 0x98);
    wrGC0309Reg(0x10, 0x26);
    wrGC0309Reg(0x1a, 0x26);

    wrGC0309Reg(0x9F, 0x15);
    wrGC0309Reg(0xA0, 0x2A);
    wrGC0309Reg(0xA1, 0x4A);
    wrGC0309Reg(0xA2, 0x67);
    wrGC0309Reg(0xA3, 0x79);
    wrGC0309Reg(0xA4, 0x8C);
    wrGC0309Reg(0xA5, 0x9A);
    wrGC0309Reg(0xA6, 0xB3);
    wrGC0309Reg(0xA7, 0xC5);
    wrGC0309Reg(0xA8, 0xD5);
    wrGC0309Reg(0xA9, 0xDF);
    wrGC0309Reg(0xAA, 0xE8);
    wrGC0309Reg(0xAB, 0xEE);
    wrGC0309Reg(0xAC, 0xF3);
    wrGC0309Reg(0xAD, 0xFA);
    wrGC0309Reg(0xAE, 0xFD);
    wrGC0309Reg(0xAF, 0xFF);

    //Y_gamma
    wrGC0309Reg(0xc0, 0x00);
    wrGC0309Reg(0xc1, 0x0B);
    wrGC0309Reg(0xc2, 0x15);
    wrGC0309Reg(0xc3, 0x27);
    wrGC0309Reg(0xc4, 0x39);
    wrGC0309Reg(0xc5, 0x49);
    wrGC0309Reg(0xc6, 0x5A);
    wrGC0309Reg(0xc7, 0x6A);
    wrGC0309Reg(0xc8, 0x89);
    wrGC0309Reg(0xc9, 0xA8);
    wrGC0309Reg(0xca, 0xC6);
    wrGC0309Reg(0xcb, 0xE3);
    wrGC0309Reg(0xcc, 0xFF);

    /////////////////////////////////////////////////////////////////
    /////////////////////////// ABS_t ///////////////////////////////
    /////////////////////////////////////////////////////////////////
    wrGC0309Reg(0xf0, 0x02);
    wrGC0309Reg(0xf1, 0x01);
    wrGC0309Reg(0xf2, 0x00);
    wrGC0309Reg(0xf3, 0x30);

    /////////////////////////////////////////////////////////////////
    /////////////////////////// Measure Window ///////////////////////
    /////////////////////////////////////////////////////////////////
    wrGC0309Reg(0xf7, 0x04);
    wrGC0309Reg(0xf8, 0x02);
    wrGC0309Reg(0xf9, 0x9f);
    wrGC0309Reg(0xfa, 0x78);

    //---------------------------------------------------------------
    wrGC0309Reg(0xfe, 0x01);

    /////////////////////////////////////////////////////////////////
    ///////////////////////////AWB_p/////////////////////////////////
    /////////////////////////////////////////////////////////////////
    wrGC0309Reg(0x00, 0xf5);
    //wrGC0309Reg(0x01,0x0a);
    wrGC0309Reg(0x02, 0x1a);
    wrGC0309Reg(0x0a, 0xa0);
    wrGC0309Reg(0x0b, 0x60);
    wrGC0309Reg(0x0c, 0x08);
    wrGC0309Reg(0x0e, 0x4c);
    wrGC0309Reg(0x0f, 0x39);
    wrGC0309Reg(0x11, 0x3f);
    wrGC0309Reg(0x12, 0x72);
    wrGC0309Reg(0x13, 0x13);
    wrGC0309Reg(0x14, 0x42);
    wrGC0309Reg(0x15, 0x43);
    wrGC0309Reg(0x16, 0xc2);
    wrGC0309Reg(0x17, 0xa8);
    wrGC0309Reg(0x18, 0x18);
    wrGC0309Reg(0x19, 0x40);
    wrGC0309Reg(0x1a, 0xd0);
    wrGC0309Reg(0x1b, 0xf5);

    wrGC0309Reg(0x70, 0x40);
    wrGC0309Reg(0x71, 0x58);
    wrGC0309Reg(0x72, 0x30);
    wrGC0309Reg(0x73, 0x48);
    wrGC0309Reg(0x74, 0x20);
    wrGC0309Reg(0x75, 0x60);
    wrGC0309Reg(0xfe, 0x00);
    wrGC0309Reg(0xd2, 0x90); // Open AEC at last.

#if 0
#if (CONFIG_INPUT_FPS == 10)
    //10 fps
    wrGC0309Reg(0x01, 0xff);
    wrGC0309Reg(0x02, 0xff);
    wrGC0309Reg(0x0f, 0x11);
#elif (CONFIG_INPUT_FPS == 15)
    //15 fps
    wrGC0309Reg(0x01, 0x80);
    wrGC0309Reg(0x02, 0xf8);
    wrGC0309Reg(0x0f, 0x01);
#elif (CONFIG_INPUT_FPS == 20)
    //20 fps
    wrGC0309Reg(0x01, 0x80);
    wrGC0309Reg(0x02, 0xf8);
    wrGC0309Reg(0x0f, 0x00);
#elif (CONFIG_INPUT_FPS == 25)
    //25 fps
    wrGC0309Reg(0x12, 0x10);
    wrGC0309Reg(0x01, 0x40);
    wrGC0309Reg(0x02, 0x10);
    wrGC0309Reg(0x0f, 0x00);
#else
    //25 fps
    wrGC0309Reg(0x12, 0x10);
    wrGC0309Reg(0x0f, 0x00);
    wrGC0309Reg(0x01, 0x40);
    wrGC0309Reg(0x02, 0x10);
#endif
#endif

    puts("\nGC0309 UYVY\n");
    *format = SEN_IN_FORMAT_UYVY;
}



static s32 GC0309_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width + 8;
    u16 liv_height = *height + 8;

    wrGC0309Reg(0x09, liv_height >> 8);
    wrGC0309Reg(0x0a, liv_height & 0xff);
    wrGC0309Reg(0x0b, liv_width >> 8);
    wrGC0309Reg(0x0c, liv_width & 0xff);

    printf("GC0309 : %d , %d \n", *width, *height);
    return 0;
}

static s32 GC0309_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 GC0309_ID_check(void)
{
    u8 pid = 0x00;
    rdGC0309Reg(0x00, &pid);
    rdGC0309Reg(0x00, &pid);
    printf("GC0309 Sensor ID : 0x%x\n", pid);
    if (pid != 0xa0) {
        return -1;
    }

    return 0;
}

static void GC0309_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void GC0309_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("GC0309 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = (u8)gc0309_reset_io[0];
        powd_io = (u8)gc0309_power_io[0];
    } else {
        res_io = (u8)gc0309_reset_io[1];
        powd_io = (u8)gc0309_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        GC0309_powerio_ctl((u32)powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}


static s32 GC0309_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        gc0309_reset_io[isp_dev] = (u8)_reset_gpio;
        gc0309_power_io[isp_dev] = (u8)_power_gpio;
    }
    if (iic == NULL) {
        printf("GC0309 iic open err!!!\n\n");
        return -1;
    }
    GC0309_reset(isp_dev);

    if (0 != GC0309_ID_check()) {
        dev_close(iic);
        iic = NULL;
        printf("-------not GC0309------\n\n");
        return -1;
    }
    printf("-------hello GC0309------\n\n");
    return 0;
}


static s32 GC0309_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n GC0309_init \n\n");

    if (0 != GC0309_check(isp_dev, 0, 0)) {
        return -1;
    }
    GC0309_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_GC0309(u16 rev_flag)
{
    if (!rev_flag) {
        wrGC0309Reg(0x14, 0x13);
    } else {
        wrGC0309Reg(0x14, 0x10);
    }
}

u16 GC0309_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdGC0309Reg((u8)addr, &val);
    return val;
}

void GC0309_dvp_wr_reg(u16 addr, u16 val)
{
    wrGC0309Reg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA(GC0309) = {
    .logo 				= 	"GC0309",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_UYVY,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B  | SEN_MBUS_HSYNC_ACTIVE_HIGH | \
    						SEN_MBUS_PCLK_SAMPLE_FALLING | SEN_MBUS_VSYNC_ACTIVE_LOW,
#if CONFIG_CAMERA_H_V_EXCHANGE
    .sync_config		=   SEN_MBUS_SYNC0_VSYNC_SYNC1_HSYNC,//WL82/AC791才可以H-V SYNC互换，请留意
#endif
    .fps         		= 	CONFIG_INPUT_FPS,
    .out_fps			=   CONFIG_INPUT_FPS,
    .sen_size 			= 	{GC0309_DEVP_INPUT_W, GC0309_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{GC0309_DEVP_INPUT_W, GC0309_DEVP_INPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	GC0309_check,
        .init 		        = 	GC0309_init,
        .set_size_fps 		=	GC0309_set_output_size,
        .power_ctrl         =   GC0309_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	GC0309_dvp_wr_reg,
        .read_reg 		    =	GC0309_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_GC0309,
    }
};

