#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "gc0307.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"



static void *iic = NULL;
static u8 gc0307_reset_io[2] = {-1, -1};
static u8 gc0307_power_io[2] = {-1, -1};

#if (CONFIG_VIDEO_IMAGE_W > 640)
#define GC0307_DEVP_INPUT_W 	640
#define GC0307_DEVP_INPUT_H		480
#else
#define GC0307_DEVP_INPUT_W 	CONFIG_VIDEO_IMAGE_W
#define GC0307_DEVP_INPUT_H		CONFIG_VIDEO_IMAGE_H
#endif

#define GC0307_WRCMD 0x42
#define GC0307_RDCMD 0x43


#define CONFIG_INPUT_FPS	15

#define DELAY_TIME	10
struct reginfo {
    u8 reg;
    u8 val;
};

/* init 640X480 VGA */
static const struct reginfo sensor_init_data[] = {
//========= close output            return err;
    {0x43, 0x00},
    /*{0x44  ,0xa2}, */
    {0x44, 0xa0},

    //========= close some functions
    // open them after configure their parmameters
    {0x40, 0x10},
    {0x41, 0x00},
    {0x42, 0x10},
    {0x47, 0x00},  //mode1,
    {0x48, 0xc3},  //mode2,
    {0x49, 0x00},  //dither_mode
    {0x4a, 0x00},  //clock_gating_en
    {0x4b, 0x00},  //mode_reg3
    {0x4E, 0x22}, //0x23}, //sync mode yaowei
    {0x4F, 0x01},  //AWB, AEC, every N frame

    //========= frame timing
    /*{0xf0  ,0x00}, //select page0 */
#if (CONFIG_INPUT_FPS == 10)
    {0x01, 0xff},  //HB // 1
    {0x02, 0xff},  //HB // 1
    {0x10, 0x11},  //high 4 bits of VB, HB
#elif (CONFIG_INPUT_FPS == 15)
    {0x01, 0x80},  //HB // 1
    {0x02, 0xf8},  //VB
    {0x10, 0x01},  //high 4 bits of VB, HB
#elif (CONFIG_INPUT_FPS == 20)
    {0x01, 0x80},  //HB // 1
    {0x02, 0xf8},  //VB
    {0x10, 0x00},  //high 4 bits of VB, HB
#else
    {0x01, 0x40},  //HB // 1
    {0x02, 0x18},  //VB
    {0x10, 0x00},  //high 4 bits of VB, HB
#endif

    {0x1C, 0x04},  //Vs_st
    {0x1D, 0x04},  //Vs_et
    {0x11, 0x05},  //row_tail,  AD_pipe_number

    //========= windowing
    {0x05, 0x00},  //row_start
    {0x06, 0x00},
    {0x07, 0x00},  //col start
    {0x08, 0x00},
    {0x09, 0x01},  //win height
    {0x0A, 0xE8},
    {0x0B, 0x02},  //win width, pixel array only 640
    {0x0C, 0x80},

    //========= analog
    {0x0D, 0x22},  //rsh_width

    {0x0E, 0x02},  //CISCTL mode2,


    {0x12, 0x70},  //7 hrst, 6_4 darsg,
    {0x13, 0x00},  //7 CISCTL_restart, 0 apwd
    {0x14, 0x00},  //NA
    {0x15, 0xba},  //7_4 vref
    {0x16, 0x13},  //5to4 _coln_r,  __1to0__da18
    {0x17, 0x52},  //opa_r, ref_r, sRef_r
    //{0x18  ,0xc0}, //analog_mode, best case for left band.

    {0x1E, 0x0d},  //tsp_width
    {0x1F, 0x32},  //sh_delay

    //========= offset
    {0x47, 0x00},   //7__test_image, __6__fixed_pga, __5__auto_DN, __4__CbCr_fix,
    //__3to2__dark_sequence, __1__allow_pclk_vcync, __0__LSC_test_image
    {0x19, 0x06},   //pga_o
    {0x1a, 0x06},   //pga_e

    {0x31, 0x00},   //4	//pga_oFFset ,	 high 8bits of 11bits
    {0x3B, 0x00},   //global_oFFset, low 8bits of 11bits

    {0x59, 0x0f},   //offset_mode
    {0x58, 0x88},   //DARK_VALUE_RATIO_G,  DARK_VALUE_RATIO_RB
    {0x57, 0x08},   //DARK_CURRENT_RATE
    {0x56, 0x77},   //PGA_OFFSET_EVEN_RATIO, PGA_OFFSET_ODD_RATIO

    //========= blk
    {0x35, 0xd8},   //blk_mode

    {0x36, 0x40},

    {0x3C, 0x00},
    {0x3D, 0x00},
    {0x3E, 0x00},
    {0x3F, 0x00},

    {0xb5, 0x70},
    {0xb6, 0x40},
    {0xb7, 0x00},
    {0xb8, 0x38},
    {0xb9, 0xc3},
    {0xba, 0x0f},

    {0x7e, 0x50}, //0x45 ylz++
    {0x7f, 0x76},    //0x66

    {0x5c, 0x48},  //78
    {0x5d, 0x58},  //88


    //========= manual_gain
    {0x61, 0x80},  //manual_gain_g1
    {0x63, 0x80},  //manual_gain_r
    {0x65, 0x98},  //manual_gai_b, 0xa0=1.25, 0x98=1.1875
    {0x67, 0x80},  //manual_gain_g2
    {0x68, 0x18},  //global_manual_gain	 2.4bits

    //=========CC _R
    {0x69, 0x58},   //54
    {0x6A, 0xf6},   //ff
    {0x6B, 0xfb},   //fe
    {0x6C, 0xf4},   //ff
    {0x6D, 0x5a},   //5f
    {0x6E, 0xe6},   //e1

    {0x6f, 0x00},

    //=========lsc
    {0x70, 0x14},
    {0x71, 0x1c},
    {0x72, 0x20},

    {0x73, 0x10},
    {0x74, 0x3c},
    {0x75, 0x52},

    //=========dn
    {0x7d, 0x2f},   //dn_mode
    {0x80, 0x0c},  //when auto_dn, check 7e,7f
    {0x81, 0x0c},
    {0x82, 0x44},

    //dd
    {0x83, 0x18},   //DD_TH1
    {0x84, 0x18},   //DD_TH2
    {0x85, 0x04},   //DD_TH3
    {0x87, 0x34},   //32 b DNDD_low_range X16,  DNDD_low_range_C_weight_center


    //=========intp-ee
    {0x88, 0x04},
    {0x89, 0x01},
    {0x8a, 0x50}, //60
    {0x8b, 0x50}, //60
    {0x8c, 0x07},

    {0x50, 0x0c},
    {0x5f, 0x3c},

    {0x8e, 0x02},
    {0x86, 0x02},

    {0x51, 0x20},
    {0x52, 0x08},
    {0x53, 0x00},


    //========= YCP
    //contrast_center
    {0x77, 0x80},  //contrast_center //0x80 20120416
    {0x78, 0x00},  //fixed_Cb
    {0x79, 0x00},  //fixed_Cr
    {0x7a, 0x00},  //luma_offset
    {0x7b, 0x40},  //hue_cos
    {0x7c, 0x00},  //hue_sin

    //saturation
    {0xa0, 0x40},  //global_saturation
    {0xa1, 0x50},  //luma_contrast	// 0x42 20120416
    {0xa2, 0x40},  //saturation_Cb		//ylz  34
    {0xa3, 0x34},  //saturation_Cr

    {0xa4, 0xc8},
    {0xa5, 0x02},
    {0xa6, 0x28},
    {0xa7, 0x02},

    //skin
    {0xa8, 0xee},
    {0xa9, 0x12},
    {0xaa, 0x01},
    {0xab, 0x20},
    {0xac, 0xf0},
    {0xad, 0x10},

    //========= ABS
    {0xae, 0x18},
    {0xaf, 0x74},
    {0xb0, 0xe0},
    {0xb1, 0x20},
    {0xb2, 0x6c},
    {0xb3, 0x40},
    {0xb4, 0x04},

    //========= AWB
    {0xbb, 0x42},
    {0xbc, 0x60},
    {0xbd, 0x50},
    {0xbe, 0x50},

    {0xbf, 0x0c},
    {0xc0, 0x06},
    {0xc1, 0x60},
    {0xc2, 0xf1},   //f1
    {0xc3, 0x40},
    {0xc4, 0x1c},  //18//20
    {0xc5, 0x56},   //33
    {0xc6, 0x1d},

    {0xca, 0x70},
    {0xcb, 0x70},
    {0xcc, 0x78},

    {0xcd, 0x80},  //R_ratio
    {0xce, 0x80},  //G_ratio  , cold_white white
    {0xcf, 0x80},  //B_ratio

    //=========  aecT
    {0x20, 0x06}, //0x02
    {0x21, 0xc0},
    {0x22, 0x40},
    {0x23, 0x88},
    {0x24, 0x96},
    {0x25, 0x30},
    {0x26, 0xd0},
    {0x27, 0x00},

    {0x28, 0x02},  //AEC_exp_level_1bit11to8
    {0x29, 0x58},  //AEC_exp_level_1bit7to0
    {0x2a, 0x03},  //AEC_exp_level_2bit11to8
    {0x2b, 0x84},  //AEC_exp_level_2bit7to0
    {0x2c, 0x09},  //AEC_exp_level_3bit11to8   659 - 8FPS,  8ca - 6FPS  //
    {0x2d, 0x60},  //AEC_exp_level_3bit7to0
    {0x2e, 0x0a},  //AEC_exp_level_4bit11to8   4FPS
    {0x2f, 0x8c},  //AEC_exp_level_4bit7to0

    {0x30, 0x20},
    {0x31, 0x00},
    {0x32, 0x1c},
    {0x33, 0x90},
    {0x34, 0x10},

    {0xd0, 0x34},

    {0xd1, 0x58},  //AEC_target_Y	// 0x40_ 20120416
    {0xd2, 0x61}, //0xf2
    {0xd4, 0x96},
    {0xd5, 0x01},  // william 0318
    {0xd6, 0x96},  //antiflicker_step
    {0xd7, 0x03},  //AEC_exp_time_min ,william 20090312
    {0xd8, 0x02},

    {0xdd, 0x12}, //0x12

    //========= measure window	            return err;
    {0xe0, 0x03},
    {0xe1, 0x02},
    {0xe2, 0x27},
    {0xe3, 0x1e},
    {0xe8, 0x3b},
    {0xe9, 0x6e},
    {0xea, 0x2c},
    {0xeb, 0x50},
    {0xec, 0x73},

    //========= close_frame
    {0xed, 0x00},  //close_frame_num1 ,can be use to reduce FPS
    {0xee, 0x00},  //close_frame_num2
    {0xef, 0x00},  //close_frame_num

    // page1
    {0xf0, 0x01},  //select page1

    {0x00, 0x20},
    {0x01, 0x20},
    {0x02, 0x20},
    {0x03, 0x20},
    {0x04, 0x78},
    {0x05, 0x78},
    {0x06, 0x78},
    {0x07, 0x78},



    {0x10, 0x04},
    {0x11, 0x04},
    {0x12, 0x04},
    {0x13, 0x04},
    {0x14, 0x01},
    {0x15, 0x01},
    {0x16, 0x01},
    {0x17, 0x01},


    {0x20, 0x00},
    {0x21, 0x00},
    {0x22, 0x00},
    {0x23, 0x00},
    {0x24, 0x00},
    {0x25, 0x00},
    {0x26, 0x00},
    {0x27, 0x00},

    {0x40, 0x11},

    //=============================lscP
    {0x45, 0x06},
    {0x46, 0x06},
    {0x47, 0x05},

    {0x48, 0x04},
    {0x49, 0x03},
    {0x4a, 0x03},


    {0x62, 0xd8},
    {0x63, 0x24},
    {0x64, 0x24},
    {0x65, 0x24},
    {0x66, 0xd8},
    {0x67, 0x24},

    {0x5a, 0x00},
    {0x5b, 0x00},
    {0x5c, 0x00},
    {0x5d, 0x00},
    {0x5e, 0x00},
    {0x5f, 0x00},


    //============================= ccP

    {0x69, 0x03},  //cc_mode

    //CC_G
    {0x70, 0x5d},
    {0x71, 0xed},
    {0x72, 0xff},
    {0x73, 0xe5},
    {0x74, 0x5f},
    {0x75, 0xe6},

    //CC_B
    {0x76, 0x41},
    {0x77, 0xef},
    {0x78, 0xff},
    {0x79, 0xff},
    {0x7a, 0x5f},
    {0x7b, 0xfa},


    //============================= AGP

    {0x7e, 0x00},
    {0x7f, 0x20},   //x040
    {0x80, 0x48},
    {0x81, 0x06},
    {0x82, 0x08},

    {0x83, 0x23},
    {0x84, 0x38},
    {0x85, 0x4F},
    {0x86, 0x61},
    {0x87, 0x72},
    {0x88, 0x80},
    {0x89, 0x8D},
    {0x8a, 0xA2},
    {0x8b, 0xB2},
    {0x8c, 0xC0},
    {0x8d, 0xCA},
    {0x8e, 0xD3},
    {0x8f, 0xDB},
    {0x90, 0xE2},
    {0x91, 0xED},
    {0x92, 0xF6},
    {0x93, 0xFD},

    //about gamma1 is hex r oct
    {0x94, 0x04},
    {0x95, 0x0E},
    {0x96, 0x1B},
    {0x97, 0x28},
    {0x98, 0x35},
    {0x99, 0x41},
    {0x9a, 0x4E},
    {0x9b, 0x67},
    {0x9c, 0x7E},
    {0x9d, 0x94},
    {0x9e, 0xA7},
    {0x9f, 0xBA},
    {0xa0, 0xC8},
    {0xa1, 0xD4},
    {0xa2, 0xE7},
    {0xa3, 0xF4},
    {0xa4, 0xFA},

    //========= open functions
    {0xf0, 0x00},  //set back to page0
    {0x40, 0x7e},
    {0x41, 0x2F},

/////  请注意，调整GC0307的镜像和翻转，需要同时修改三个寄存器，如下:

    {0x0f, 0x82},
    {0x45, 0x24},
    {0x47, 0x20},
///banding setting
    /*
    {  0x01  ,0xfa}, // 24M
           {  0x02  ,0x70},
           {  0x10  ,0x01},
           {  0xd6  ,0x64},
           {  0x28  ,0x02},
           {  0x29  ,0x58},
           {  0x2a  ,0x02},
           {  0x2b  ,0x58},
           {  0x2c  ,0x02},
           {  0x2d  ,0x58},
           {  0x2e  ,0x06},
           {  0x2f  ,0x40},
    */
    /*{  0x01  ,0xfa}, // 24M  // 20120416*/
    /*{  0x02  ,0x70}, */
    /*{  0x10  ,0x01},   */
    {  0xd6, 0x64},
    {  0x28, 0x02},
    {  0x29, 0x58},
    {  0x2a, 0x02},
    {  0x2b, 0x58},
    {  0x2c, 0x02},
    {  0x2d, 0x58},
    {  0x2e, 0x06},
    {  0x2f, 0x40},

    /************
    {0x0f, 0x02},//82
    {0x45, 0x24},
    {0x47, 0x20},
    **************/
/////  四种不同的翻转和镜像设定，客户可直接复制!!!!!!


#if 0
//  IMAGE_NORMAL:
    {0x0f, 0xb2},
    {0x45, 0x27},
    {0x47, 0x2c},

// IMAGE_H_MIRROR:
    {0x0f, 0xa2},
    {0x45, 0x26},
    {0x47, 0x28},

// IMAGE_V_MIRROR:
    {0x0f, 0x92},
    {0x45, 0x25},
    {0x47, 0x24},

// IMAGE_HV_MIRROR:	   // 180
    {0x0f, 0x82},
    {0x45, 0x24},
    {0x47, 0x20},
#endif
    {0x43, 0x40},
    /*{0x44, 0xe2},	*/
    {0x44, 0xe0},
    {0xff, 0xff},
};

static s32 GC0307_set_output_size(u16 *width, u16 *height, u8 *freq);
static unsigned char wrGC0307Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0307_WRCMD)) {
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

static unsigned char rdGC0307Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0307_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, GC0307_RDCMD)) {
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

static void GC0307_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    int i;
    for (i = 0; i < sizeof(sensor_init_data) / sizeof(sensor_init_data[0]); i++) {
        wrGC0307Reg(sensor_init_data[i].reg, sensor_init_data[i].val);
    }
    puts("\nGC0307 UYVY\n");
    *format = SEN_IN_FORMAT_UYVY;
}



static s32 GC0307_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width;
    u16 liv_height = *height + 8;

    wrGC0307Reg(0x09, liv_height >> 8);
    wrGC0307Reg(0x0a, liv_height & 0xff);
    wrGC0307Reg(0x0b, liv_width >> 8);
    wrGC0307Reg(0x0c, liv_width & 0xff);

    printf("GC0307 : %d , %d \n", *width, *height);
    return 0;
}

static s32 GC0307_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 GC0307_ID_check(void)
{
    u8 pid = 0x00;
    rdGC0307Reg(0x00, &pid);
    rdGC0307Reg(0x00, &pid);
    printf("GC0307 Sensor ID : 0x%x\n", pid);
    if (pid != 0x99) {
        return -1;
    }

    return 0;
}

static void GC0307_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void GC0307_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("GC0307 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = (u8)gc0307_reset_io[0];
        powd_io = (u8)gc0307_power_io[0];
    } else {
        res_io = (u8)gc0307_reset_io[1];
        powd_io = (u8)gc0307_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        GC0307_powerio_ctl((u32)powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}


static s32 GC0307_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        gc0307_reset_io[isp_dev] = (u8)_reset_gpio;
        gc0307_power_io[isp_dev] = (u8)_power_gpio;
    }
    if (iic == NULL) {
        printf("GC0307 iic open err!!!\n\n");
        return -1;
    }
    GC0307_reset(isp_dev);

    if (0 != GC0307_ID_check()) {
        dev_close(iic);
        iic = NULL;
        printf("-------not GC0307------\n\n");
        return -1;
    }
    printf("-------hello GC0307------\n\n");
    return 0;
}


static s32 GC0307_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n GC0307_init \n\n");

    if (0 != GC0307_check(isp_dev, 0, 0)) {
        return -1;
    }
    GC0307_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_GC0307(u16 rev_flag)
{
    if (!rev_flag) {
        wrGC0307Reg(0x14, 0x13);
    } else {
        wrGC0307Reg(0x14, 0x10);
    }
}

u16 GC0307_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdGC0307Reg((u8)addr, &val);
    return val;
}

void GC0307_dvp_wr_reg(u16 addr, u16 val)
{
    wrGC0307Reg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA(GC0307) = {
    .logo 				= 	"GC0307",
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
    .sen_size 			= 	{GC0307_DEVP_INPUT_W, GC0307_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{GC0307_DEVP_INPUT_W, GC0307_DEVP_INPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	GC0307_check,
        .init 		        = 	GC0307_init,
        .set_size_fps 		=	GC0307_set_output_size,
        .power_ctrl         =   GC0307_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	GC0307_dvp_wr_reg,
        .read_reg 		    =	GC0307_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_GC0307,
    }
};

