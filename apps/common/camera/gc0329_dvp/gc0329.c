#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gc0329.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "os/os_api.h"
#include "app_config.h"


#if (CONFIG_VIDEO_IMAGE_W > 640)
#define GC0329_DEVP_INPUT_W 640
#define GC0329_DEVP_INPUT_H 480
#else
#define GC0329_DEVP_INPUT_W CONFIG_VIDEO_IMAGE_W
#define GC0329_DEVP_INPUT_H CONFIG_VIDEO_IMAGE_H
#endif

#define CONFIG_INPUT_FPS 	20


//////////////////
#define BRIGHTNES 6 //亮度调节
#define CONTRAST 4 //对比度调节
#define AWD_TAB 0 //特效颜色输出  0正常
/////////////////

static void *iic = NULL;
static u8 gc0329_reset_io[2] = {-1, -1};
static u8 gc0329_power_io[2] = {-1, -1};

#define GC0329_WRCMD 0x62
#define GC0329_RDCMD 0x63

#define DELAY_TIME	10

static s32 GC0329_set_output_size(u16 *width, u16 *height, u8 *freq);
static unsigned char wrGC0329Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0329_WRCMD)) {
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

static unsigned char rdGC0329Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0329_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0329_RDCMD)) {
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

static void GC0329_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("GC0329_config_SENSOR \n");

    GC0329_set_output_size(width, height, frame_freq);

#if (CONFIG_INPUT_FPS == 20)
    wrGC0329Reg(0xfe, 0x00); //**************
    wrGC0329Reg(0x05, 0x01);
    wrGC0329Reg(0x06, 0xfa);
    wrGC0329Reg(0x07, 0x01);
    wrGC0329Reg(0x08, 0x38);
    wrGC0329Reg(0xfe, 0x01);
    wrGC0329Reg(0x29, 0x00);
    wrGC0329Reg(0x2a, 0x64);
    wrGC0329Reg(0x2b, 0x02);
    wrGC0329Reg(0x2c, 0x58);
    wrGC0329Reg(0x2d, 0x02);
    wrGC0329Reg(0x2e, 0x58);
    wrGC0329Reg(0x2f, 0x02);
    wrGC0329Reg(0x30, 0x58);
    wrGC0329Reg(0x31, 0x05);
    wrGC0329Reg(0x32, 0xdc);
    wrGC0329Reg(0xfe, 0x00);
#elif (CONFIG_INPUT_FPS == 25)
    wrGC0329Reg(0xfe, 0x00); //************
    wrGC0329Reg(0x05, 0x00);
    wrGC0329Reg(0x06, 0xa8);
    wrGC0329Reg(0x07, 0x00);
    wrGC0329Reg(0x08, 0x5c);
    wrGC0329Reg(0xfe, 0x01);
    wrGC0329Reg(0x29, 0x00);
    wrGC0329Reg(0x2a, 0x3a);
    wrGC0329Reg(0x2b, 0x02);
    wrGC0329Reg(0x2c, 0x44);
    wrGC0329Reg(0x2d, 0x02);
    wrGC0329Reg(0x2e, 0x44);
    wrGC0329Reg(0x2f, 0x02);
    wrGC0329Reg(0x30, 0x44);
    wrGC0329Reg(0x31, 0x05);
    wrGC0329Reg(0x32, 0x70);
    wrGC0329Reg(0xfe, 0x00);
#endif


    wrGC0329Reg(0xfe, 0x80);
    wrGC0329Reg(0xfe, 0x80);
    wrGC0329Reg(0xfc, 0x12); //clock_en
    wrGC0329Reg(0xfc, 0x12); //clock_en
    wrGC0329Reg(0xfe, 0x00);
    wrGC0329Reg(0xf0, 0x07); //vsync_en
    wrGC0329Reg(0xf1, 0x01); //data_en

    wrGC0329Reg(0x73, 0x90);//R channle gain
    wrGC0329Reg(0x74, 0x80);//G1 channle gain
    wrGC0329Reg(0x75, 0x80);//G2 channle gain
    wrGC0329Reg(0x76, 0x94);//B channle gain

    wrGC0329Reg(0x42, 0x00);
    wrGC0329Reg(0x77, 0x57);
    wrGC0329Reg(0x78, 0x4d);
    wrGC0329Reg(0x79, 0x45);
    wrGC0329Reg(0x42, 0xfc);
    wrGC0329Reg(0x03, 0x02);
    wrGC0329Reg(0x04, 0x58);

    /*
    wrGC0329Reg(0x05, 0x01);
    wrGC0329Reg(0x06, 0xfa);
    wrGC0329Reg(0x07, 0x01);
    wrGC0329Reg(0x08, 0x38);
    wrGC0329Reg(0xfe, 0x01);
    wrGC0329Reg(0x29, 0x00);
    wrGC0329Reg(0x2a, 0x64);
    wrGC0329Reg(0x2b, 0x02);
    wrGC0329Reg(0x2c, 0x58);
    wrGC0329Reg(0x2d, 0x02);
    wrGC0329Reg(0x2e, 0x58);
    wrGC0329Reg(0x2f, 0x02);
    wrGC0329Reg(0x30, 0x58);
    wrGC0329Reg(0x31, 0x05);
    wrGC0329Reg(0x32, 0xdc);
    wrGC0329Reg(0xfe, 0x00);
    */
    ////////////////////nalog////////////////////
    wrGC0329Reg(0xfc, 0x16);
    wrGC0329Reg(0x0a, 0x02); //row_s);tart_low
    wrGC0329Reg(0x0c, 0x02); //col_start_low
    wrGC0329Reg(0x17, 0x14);//cisctl_mode1//[7]hsync_always ,[6] NA,  [5:4] CFA sequence [3:2]NA,  [1]upside_down,  [0] mirror
    wrGC0329Reg(0x19, 0x05);
    wrGC0329Reg(0x1b, 0x24);
    wrGC0329Reg(0x1c, 0x04);
    wrGC0329Reg(0x1e, 0x15);//Analog_mode1//[7:6]rsv1,rsv0[5:3] Column bias(coln_r)[1] clk_delay_en
    wrGC0329Reg(0x1f, 0xc0);//Analog_mode2//[7:6] comv_r
    wrGC0329Reg(0x20, 0x00);//Analog_mode3//[6:4] cap_low_r for MPW [3:2] da18_r [1] rowclk_mode [0]adclk_mode
    wrGC0329Reg(0x21, 0x48);//Hrst_rsg//[7] hrst[6:4] da_rsg[3]txhigh_en
    wrGC0329Reg(0x23, 0x22);//ADC_r//[6:5]opa_r [1:0]sRef
    wrGC0329Reg(0x24, 0x16);//PAD_drv//[7:6]NA,[5:4]sync_drv [3:2]data_drv [1:0]pclk_drv

    ////////////////////blk////////////////////
    wrGC0329Reg(0x26, 0xf7);
    wrGC0329Reg(0x32, 0x04);
    wrGC0329Reg(0x33, 0x20); //曝光亮度 0x30最大 0x20 0x10 0x00最小
    wrGC0329Reg(0x34, 0x20);
    wrGC0329Reg(0x35, 0x20);
    wrGC0329Reg(0x36, 0x20);

    ////////////////////ISP BLOCK ENABLE////////////////////
    wrGC0329Reg(0x40, 0xff);
    wrGC0329Reg(0x41, 0x00);
    wrGC0329Reg(0x42, 0xfe);
    wrGC0329Reg(0x46, 0x03);//sync mode
    wrGC0329Reg(0x4b, 0xca);
    wrGC0329Reg(0x4d, 0x01);//[1]In_buf
    wrGC0329Reg(0x4f, 0x01);
    wrGC0329Reg(0x70, 0x48);//global gain

    //wrGC0329Reg(0xb0, 0x00);
    //wrGC0329Reg(0xbc, 0x00);
    //wrGC0329Reg(0xbd, 0x00);
    //wrGC0329Reg(0xbe, 0x00);

    ////////////////////DNDD////////////////////
    wrGC0329Reg(0x80, 0xe7);//87[7]auto_en [6]one_pixel [5]two_pixel
    wrGC0329Reg(0x82, 0x55);//DN_inc
    wrGC0329Reg(0x87, 0x4a);

    /////////////////ASDE////////////////////
    wrGC0329Reg(0xfe, 0x01);
    wrGC0329Reg(0x18, 0x22);//[7:4]AWB LUMA X  [3:0]ASDE LUMA X
    wrGC0329Reg(0xfe, 0x00);
    wrGC0329Reg(0x9c, 0x0a);//ASDE dn b slope
    wrGC0329Reg(0xa4, 0x50);//Auto Sa slope
    wrGC0329Reg(0xa5, 0x21);//[7:4]Saturation limit x10
    wrGC0329Reg(0xa7, 0x35);//low luma value th
    wrGC0329Reg(0xdd, 0x54);//edge dec sat enable & slopes
    wrGC0329Reg(0x95, 0x35);//Edge effect


    ////////////////////RGB gamma////////////////////
    wrGC0329Reg(0xfe, 0x00);
    wrGC0329Reg(0xbf, 0x06);
    wrGC0329Reg(0xc0, 0x14);
    wrGC0329Reg(0xc1, 0x27);
    wrGC0329Reg(0xc2, 0x3b);
    wrGC0329Reg(0xc3, 0x4f);
    wrGC0329Reg(0xc4, 0x62);
    wrGC0329Reg(0xc5, 0x72);
    wrGC0329Reg(0xc6, 0x8d);
    wrGC0329Reg(0xc7, 0xa4);
    wrGC0329Reg(0xc8, 0xb8);
    wrGC0329Reg(0xc9, 0xc9);
    wrGC0329Reg(0xca, 0xd6);
    wrGC0329Reg(0xcb, 0xe0);
    wrGC0329Reg(0xcc, 0xe8);
    wrGC0329Reg(0xcd, 0xf4);
    wrGC0329Reg(0xce, 0xfc);
    wrGC0329Reg(0xcf, 0xff);

    //////////////////CC///////////////////
    wrGC0329Reg(0xfe, 0x00);
    wrGC0329Reg(0xb3, 0x44);
    wrGC0329Reg(0xb4, 0xfd);
    wrGC0329Reg(0xb5, 0x02);
    wrGC0329Reg(0xb6, 0xfa);
    wrGC0329Reg(0xb7, 0x48);
    wrGC0329Reg(0xb8, 0xf0);

    ////default CC
    /*wrGC0329Reg(0xfe, 0x00);
    wrGC0329Reg(0xb3, 0x45);
    wrGC0329Reg(0xb4, 0x00);
    wrGC0329Reg(0xb5, 0x00);
    wrGC0329Reg(0xb6, 0x00);
    wrGC0329Reg(0xb7, 0x45);
    wrGC0329Reg(0xb8, 0xf0);*/

    // crop
    wrGC0329Reg(0x50, 0x01);
    /*
    wrGC0329Reg(0x51, 0x00);
    wrGC0329Reg(0x52, 0x00);
    wrGC0329Reg(0x53, 0x00);
    wrGC0329Reg(0x54, 0x00);
    wrGC0329Reg(0x55, 0x01);
    wrGC0329Reg(0x56, 0x40);
    wrGC0329Reg(0x57, 0x01);
    wrGC0329Reg(0x58, 0x60);
    */
    wrGC0329Reg(0x19, 0x05);
    wrGC0329Reg(0x20, 0x01);
    wrGC0329Reg(0x22, 0xba);
    wrGC0329Reg(0x21, 0x48);

    ////////////////////YCP////////////////////
    wrGC0329Reg(0xfe, 0x00);
    wrGC0329Reg(0xd1, 0x34);//saturation Cb
    wrGC0329Reg(0xd2, 0x34);//saturation Cr

    ////////////////////AEC////////////////////
    wrGC0329Reg(0xfe, 0x01);
    wrGC0329Reg(0x10, 0x40);
    wrGC0329Reg(0x11, 0xa1);
    wrGC0329Reg(0x12, 0x07);
    wrGC0329Reg(0x13, 0x50);//Y target
    wrGC0329Reg(0x17, 0x88);
    wrGC0329Reg(0x21, 0xb0);
    wrGC0329Reg(0x22, 0x48);
    wrGC0329Reg(0x3c, 0x95);
    wrGC0329Reg(0x3d, 0x50);
    wrGC0329Reg(0x3e, 0x48);

    ////////////////////AWB////////////////////
    wrGC0329Reg(0xfe, 0x01);
    wrGC0329Reg(0x06, 0x06);
    wrGC0329Reg(0x07, 0x06);
    wrGC0329Reg(0x08, 0xa6);
    wrGC0329Reg(0x09, 0xee);
    wrGC0329Reg(0x50, 0xfc);//RGB high
    wrGC0329Reg(0x51, 0x28);//Y2C diff
    wrGC0329Reg(0x52, 0x10);
    wrGC0329Reg(0x53, 0x1d);
    wrGC0329Reg(0x54, 0x16); //0xC inter
    wrGC0329Reg(0x55, 0x20);
    wrGC0329Reg(0x56, 0x60);
// wrGC0329Reg(0x57, 0x40);
    wrGC0329Reg(0x58, 0x60);//number limit, 0xX4
    wrGC0329Reg(0x59, 0x28);//AWB adjust temp curve
    wrGC0329Reg(0x5a, 0x02);//light gain range
    wrGC0329Reg(0x5b, 0x63);
    wrGC0329Reg(0x5c, 0x37);//show and mode [2]dark mode
    wrGC0329Reg(0x5d, 0x73);//AWB margin
    wrGC0329Reg(0x5e, 0x11);//temp curve_enable
    wrGC0329Reg(0x5f, 0x40);
    wrGC0329Reg(0x60, 0x40);//5K gain
    wrGC0329Reg(0x61, 0xc8);//sinTxfe, 0x01
    wrGC0329Reg(0x62, 0xa0);//cosT
    wrGC0329Reg(0x63, 0x40);//AWB X1 cut
    wrGC0329Reg(0x64, 0x50);//AWB X2 cut
    wrGC0329Reg(0x65, 0x98);//AWB Y1 cut
    wrGC0329Reg(0x66, 0xfa);//AWB Y2 cut
    wrGC0329Reg(0x67, 0x70);//AWB R gain limit
    wrGC0329Reg(0x68, 0x58);//AWB G gain Limit
    wrGC0329Reg(0x69, 0x85);//AWB B gain limit
    wrGC0329Reg(0x6a, 0x40);
    wrGC0329Reg(0x6b, 0x39);
    wrGC0329Reg(0x6c, 0x40);
    wrGC0329Reg(0x6d, 0x40);
    wrGC0329Reg(0x6e, 0x41);//outdoor gain limit enable [7]use exp or luma value to adjust outdoor
    wrGC0329Reg(0x70, 0x50);
    wrGC0329Reg(0x71, 0x00);//when outdoor , add high luma gray pixel weight
    wrGC0329Reg(0x72, 0x10);
    wrGC0329Reg(0x73, 0x40);//when exp < th, outdoor mode open
    wrGC0329Reg(0x74, 0x32);
    wrGC0329Reg(0x75, 0x40);
    wrGC0329Reg(0x76, 0x30);
    wrGC0329Reg(0x77, 0x48);
    wrGC0329Reg(0x7a, 0x50);
    wrGC0329Reg(0x7b, 0x20); //Yellow R2B, 0xB2G limit, >it, as Yellow
    wrGC0329Reg(0x80, 0x70);//R gain high limit
    wrGC0329Reg(0x81, 0x58);//G gain high limit
    wrGC0329Reg(0x82, 0x42);//B gain high limit
    wrGC0329Reg(0x83, 0x40);//R gain low limit
    wrGC0329Reg(0x84, 0x40);//G gain low limit
    wrGC0329Reg(0x85, 0x40);//B gaixfe, 0x01n low limit

    ////////////////////CC-WB////////////////////
    wrGC0329Reg(0xd0, 0x00);
    wrGC0329Reg(0xd2, 0x2c); //D Xn
    wrGC0329Reg(0xd3, 0x80);

    /////////////////ABS////////////////////
    wrGC0329Reg(0x9c, 0x02);
    wrGC0329Reg(0x9d, 0x10);

    wrGC0329Reg(0xfe, 0x00);

    /////////////////ASDE///////////////////
    wrGC0329Reg(0xa0, 0xaf);//[7:4]bright_slope for special point
    wrGC0329Reg(0xa2, 0xff);//for special point

    wrGC0329Reg(0x44, 0xa2);//out format

    /////////////END/////////////


    /////////////USER chance/////

    ////////////////BRIGHTNES///////////
    switch (BRIGHTNES) {
    case 0:
        wrGC0329Reg(0xd5, 0xd0);
        break;
    case 1:
        wrGC0329Reg(0xd5, 0xe0);
        break;
    case 2:
        wrGC0329Reg(0xd5, 0xf0);
        break;
    case 3:
        wrGC0329Reg(0xd5, 0x00);
        break;
    case 4:
        wrGC0329Reg(0xd5, 0x20);
        break;
    case 5:
        wrGC0329Reg(0xd5, 0x30);
        break;
    case 6:
        wrGC0329Reg(0xd5, 0x40);
        break;
    }
    wrGC0329Reg(0xff, 0xff);

    ////////////////CONTRAST//////////
    switch (CONTRAST) {
    case 0:
        wrGC0329Reg(0xd3, 0x34);
        break;
    case 1:
        wrGC0329Reg(0xd3, 0x38);
        break;
    case 2:
        wrGC0329Reg(0xd3, 0x3d);
        break;
    case 3:
        wrGC0329Reg(0xd3, 0x40);
        break;
    case 4:
        wrGC0329Reg(0xd3, 0x44);
        break;
    case 5:
        wrGC0329Reg(0xd3, 0x48);
        break;
    case 6:
        wrGC0329Reg(0xd3, 0x50);
        break;
    }
    wrGC0329Reg(0xff, 0xff);

    //////////////AWD_TAB//////////
    switch (AWD_TAB) {
    case 0://AUTO
        wrGC0329Reg(0x77, 0x57);
        wrGC0329Reg(0x78, 0x4d);
        wrGC0329Reg(0x79, 0x45);
        wrGC0329Reg(0x42, 0xfe);
        break;
    case 1://INCANDESCENCE
        wrGC0329Reg(0x42, 0xfd);
        wrGC0329Reg(0x77, 0x48);
        wrGC0329Reg(0x78, 0x40);
        wrGC0329Reg(0x79, 0x5c);
        break;
    case 2://U30
        wrGC0329Reg(0x42, 0xfd);
        wrGC0329Reg(0x77, 0x40);
        wrGC0329Reg(0x78, 0x54);
        wrGC0329Reg(0x79, 0x70);
        break;
    case 3://CWF
        wrGC0329Reg(0x42, 0xfd);
        wrGC0329Reg(0x77, 0x40);
        wrGC0329Reg(0x78, 0x54);
        wrGC0329Reg(0x79, 0x70);
        break;
    case 4://FLUORESCENT
        wrGC0329Reg(0x42, 0xfd);
        wrGC0329Reg(0x77, 0x40);
        wrGC0329Reg(0x78, 0x54);
        wrGC0329Reg(0x79, 0x50);
        break;
    case 5://SUN
        wrGC0329Reg(0x42, 0xfd);
        wrGC0329Reg(0x77, 0x50);
        wrGC0329Reg(0x78, 0x45);
        wrGC0329Reg(0x79, 0x40);
        break;
    case 6://CLOUD
        wrGC0329Reg(0x42, 0xfd);
        wrGC0329Reg(0x77, 0x5a);
        wrGC0329Reg(0x78, 0x42);
        wrGC0329Reg(0x79, 0x40);
        break;
    }
    wrGC0329Reg(0xff, 0xff);





}
static s32 GC0329_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width + 8;
    u16 liv_height = *height + 8;

    wrGC0329Reg(0x0d, liv_height >> 8);
    wrGC0329Reg(0x0e, liv_height & 0xff);
    wrGC0329Reg(0x0f, liv_width >> 8);
    wrGC0329Reg(0x10, liv_width & 0xff);
    return 0;
}

static s32 GC0329_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 GC0329_ID_check(void)
{
    u8 pid = 0;

    rdGC0329Reg(0xfb, &pid);
    rdGC0329Reg(0xfb, &pid);
    printf("GC0329 Sensor ID : 0x%x\n", pid);
    if (pid != 0x62) {
        return -1;
    }
    return 0;
}

static void GC0329_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void GC0329_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("GC0329 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = gc0329_reset_io[0];
        powd_io = gc0329_power_io[0];
    } else {
        res_io = gc0329_reset_io[1];
        powd_io = gc0329_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        GC0329_powerio_ctl(powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}

s32 GC0329_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        gc0329_reset_io[isp_dev] = (u8)_reset_gpio;
        gc0329_power_io[isp_dev] = (u8)_power_gpio;
    }

    if (iic == NULL) {
        printf("GC0329 iic open err!!!\n\n");
        return -1;
    }
    GC0329_reset(isp_dev);

    if (0 != GC0329_ID_check()) {
        printf("-------not GC0329------\n\n");
        dev_close(iic);
        iic = NULL;
        return -1;
    }

    printf("-------hello GC0329------\n\n");
    return 0;
}


static s32 GC0329_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n GC0329_init \n\n");

    if (0 != GC0329_check(isp_dev, 0, 0)) {
        return -1;
    }

    GC0329_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_GC0329(u16 rev_flag)
{
    if (!iic) {
        printf("no iic \n");
        return;
    }
    if (!rev_flag) {
        wrGC0329Reg(0x17, 0x12);
    } else {
        wrGC0329Reg(0x17, 0x11);
    }
}

u16 GC0329_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdGC0329Reg((u8)addr, &val);
    return val;
}

void GC0329_dvp_wr_reg(u16 addr, u16 val)
{
    wrGC0329Reg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA(GC0329) = {
    .logo 				= 	"GC0329",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_VYUY,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B  | SEN_MBUS_HSYNC_ACTIVE_HIGH | \
    						SEN_MBUS_PCLK_SAMPLE_FALLING | SEN_MBUS_VSYNC_ACTIVE_HIGH,
#if CONFIG_CAMERA_H_V_EXCHANGE
    .sync_config		=   SEN_MBUS_SYNC0_VSYNC_SYNC1_HSYNC,//WL82/AC791才可以H-V SYNC互换，请留意
#endif
    .fps         		= 	CONFIG_INPUT_FPS,
    .out_fps			=   CONFIG_INPUT_FPS,
    .sen_size 			= 	{GC0329_DEVP_INPUT_W, GC0329_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{GC0329_DEVP_INPUT_W, GC0329_DEVP_INPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	GC0329_check,
        .init 		        = 	GC0329_init,
        .set_size_fps 		=	GC0329_set_output_size,
        .power_ctrl         =   GC0329_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	GC0329_dvp_wr_reg,
        .read_reg 		    =	GC0329_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_GC0329,
    }
};



