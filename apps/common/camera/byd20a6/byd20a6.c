#include "device/iic.h"
#include "asm/isp_dev.h"
#include "byd20a6.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "app_config.h"


static void *iic = NULL;
static u8 BYD20a6_reset_io[2] = {-1, -1};
static u8 BYD20a6_power_io[2] = {-1, -1};


#if (CONFIG_VIDEO_IMAGE_W > 640)
#define BYD20A6_DEVP_INPUT_W 640
#define BYD20A6_DEVP_INPUT_H 480
#else
#define BYD20A6_DEVP_INPUT_W 		CONFIG_VIDEO_IMAGE_W
#define BYD20A6_DEVP_INPUT_H		CONFIG_VIDEO_IMAGE_H
#endif
#define BYD20A6_WRCMD 0xdc
#define BYD20A6_RDCMD 0xdd

#define CONFIG_INPUT_FPS	15



#define BYD20A6_OUT_DVP		1//0:spi-bt656 , 1:dvp-yuv

#if !BYD20A6_OUT_DVP //spi-bt656
#define BYD20A6_OUTPUT_MODE_CCIR656_1BIT
/*#define BYD20A6_OUTPUT_MODE_CCIR656_2BIT*/
/*#define BYD20A6_OUTPUT_MODE_CCIR656_4BIT*/
#endif

struct reginfo {
    u8 reg;
    u8 val;
};
#define DELAY_TIME	10

static const struct reginfo sensor_init_data[] = {
    {0xf2, 0x01},
    {0x12, 0xa0},
#ifdef BYD20A6_OUTPUT_MODE_CCIR656_1BIT
    {0x3a, 0x01},
    {0xe1, 0xd3},
    {0xe3, 0x02},
#elif defined(BYD20A6_OUTPUT_MODE_CCIR656_2BIT)
    {0x3a, 0x02},
    {0xe1, 0x92},
    {0xe3, 0x02},
#elif defined(BYD20A6_OUTPUT_MODE_CCIR656_4BIT)
    {0x3a, 0x03},
    {0xe1, 0x51},
    {0xe3, 0x12},
#else
    //DVP
    {0x15, 0x28},
    {0x16, 0x61},
    {0x3a, 0x00},
    {0xe1, 0x93},
    {0xe3, 0x82},
#endif

#define START_ADDRW 0x0
#define START_ADDRH 0x0
#define END_ADDRW (BYD20A6_DEVP_INPUT_W + START_ADDRW)
#define END_ADDRH (BYD20A6_DEVP_INPUT_H + START_ADDRH)
    //(END_ADDRW > 640 || END_ADDRH > 480)

    {0x17, (START_ADDRW >> 2)},
    {0x18, (END_ADDRW >> 2)},
    {0x19, (START_ADDRH >> 2)},
    {0x1a, (END_ADDRH >> 2)},
    {0x1b, (START_ADDRW & 0x3) | ((END_ADDRW & 0x3) << 2) | ((START_ADDRH & 0x3) << 4) | ((END_ADDRH & 0x3) << 6)},
    {0xe0, 0x00},
    {0x2a, 0x98},
    {0xcd, 0x17},
    {0xc0, 0x10},
    {0xc6, 0x1d},
    {0x10, 0x35},
    {0xe2, 0x09},
    {0xe4, 0x72},
    {0xe5, 0x22},
    {0xe6, 0x24},
    {0xe7, 0x64},
    {0xe8, 0xf6}, //VDDIO=1.8v 0xe8[2]=1,VDDIO=2.8v 0xe8[2]=0
    {0x4a, 0x00},
    {0x00, 0x03},
    {0x1f, 0x02},
    {0x22, 0x02},
    {0x0c, 0x31},
    {0x00, 0x00},
    {0x60, 0x81},
    {0x61, 0x81},
    {0xa0, 0x08},
    {0x01, 0x1a},
    {0x01, 0x1a},
    {0x01, 0x1a},
    {0x02, 0x15},
    {0x02, 0x15},
    {0x02, 0x15},
    {0x13, 0x08},
    {0x8a, 0x96},
    {0x8b, 0x06},
    {0x87, 0x18},
    {0x34, 0x48}, //lens
    {0x35, 0x40},
    {0x36, 0x40},
    {0x71, 0x44}, //add 10.27
    {0x72, 0x48},
    {0x74, 0xa2},
    {0x75, 0xa9},
    {0x78, 0x12},
    {0x79, 0xa0},
    {0x7a, 0x94},
    {0x7c, 0x97},
    {0x40, 0x30},
    {0x41, 0x30},
    {0x42, 0x28},
    {0x43, 0x1f},
    {0x44, 0x1c},
    {0x45, 0x16},
    {0x46, 0x13},
    {0x47, 0x10},
    {0x48, 0x0D},
    {0x49, 0x0C},
    {0x4B, 0x0A},
    {0x4C, 0x0B},
    {0x4E, 0x09},
    {0x4F, 0x08},
    {0x50, 0x08},
    {0x5f, 0x29},
    {0x23, 0x33},
    {0xa1, 0x10}, //AWB
    {0xa2, 0x0d},
    {0xa3, 0x30},
    {0xa4, 0x06},
    {0xa5, 0x22},
    {0xa6, 0x56},
    {0xa7, 0x18},
    {0xa8, 0x1a},
    {0xa9, 0x12},
    {0xaa, 0x12},
    {0xab, 0x16},
    {0xac, 0xb1},
    {0xba, 0x12},
    {0xbb, 0x12},
    {0xad, 0x12},
    {0xae, 0x56}, //56
    {0xaf, 0x0a},
    {0x3b, 0x30},
    {0x3c, 0x12},
    {0x3d, 0x22},
    {0x3e, 0x3f},
    {0x3f, 0x28},
    {0xb8, 0xc3},
    {0xb9, 0xA3},
    {0x39, 0x47}, //pure color threshold
    {0x26, 0x13},
    {0x27, 0x16},
    {0x28, 0x14},
    {0x29, 0x18},
    {0xee, 0x0d},
    {0x13, 0x05},
    {0x24, 0x3C},
    {0x81, 0x20},
    {0x82, 0x40},
    {0x83, 0x30},
    {0x84, 0x58},
    {0x85, 0x30},
    {0x92, 0x08},
    {0x86, 0xA0},
#ifdef BYD20A6_OUTPUT_MODE_CCIR656_1BIT
    {0x8a, 0x4b},
#else
    {0x8a, 0x96},
//    {0x8a, 0x66},
#endif
    {0x91, 0xff},
    {0x94, 0x62},
    {0x9a, 0x18}, //outdoor threshold
    {0xf0, 0x4e},
    {0x51, 0x17}, //color normal
    {0x52, 0x03},
    {0x53, 0x5F},
    {0x54, 0x47},
    {0x55, 0x66},
    {0x56, 0x0F},
    {0x7e, 0x14},
    {0x57, 0x36}, //A光color
    {0x58, 0x2A},
    {0x59, 0xAA},
    {0x5a, 0xA8},
    {0x5b, 0x43},
    {0x5c, 0x10},
    {0x5d, 0x00},
    {0x7d, 0x36},
    {0x5e, 0x10},
    {0xd6, 0x88}, //contrast
    {0xd5, 0x20}, //低光加亮度
    {0xb0, 0x84}, //灰色区域降饱和度
    {0xb5, 0x08}, //低光降饱和度阈值
    {0xb1, 0xc8}, //saturation
    {0xb2, 0xc0},
    {0xb3, 0xd0},
    {0xb4, 0xB0},
    {0x32, 0x10},
    {0xa0, 0x09},
    {0x00, 0x03},
    {0x0b, 0x02},
};

static s32 BYD20a6_set_output_size(u16 *width, u16 *height, u8 *freq);
static unsigned char wrBYD20a6Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, BYD20A6_WRCMD)) {
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

static unsigned char rdBYD20a6Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, BYD20A6_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, BYD20A6_RDCMD)) {
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

static void BYD20a6_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    int i;
    for (i = 0; i < sizeof(sensor_init_data) / sizeof(sensor_init_data[0]); i++) {
        wrBYD20a6Reg(sensor_init_data[i].reg, sensor_init_data[i].val);
    }
}
static s32 BYD20a6_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width;
    u16 liv_height = *height;

    return 0;
}
static s32 BYD20a6_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 BYD20a6_ID_check(void)
{
    int ret;
    u16 pid = 0x00;
    u8 id0, id1;
    ret = rdBYD20a6Reg(0xfc, &id0);
    printf("BYD20a6 Sensor ID : 0x%x\n", pid);
    rdBYD20a6Reg(0xfd, &id1);
    pid = (u16)((id0 << 8) | id1);
    printf("BYD20a6 DVP Sensor ID : 0x%x\n", pid);
    if (pid != 0x20a6) {
        return -1;
    }

    return 0;
}

static void BYD20a6_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void BYD20a6_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("BYD20a6 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = (u8)BYD20a6_reset_io[0];
        powd_io = (u8)BYD20a6_power_io[0];
    } else {
        res_io = (u8)BYD20a6_reset_io[1];
        powd_io = (u8)BYD20a6_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        BYD20a6_powerio_ctl((u32)powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}


static s32 BYD20a6_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        BYD20a6_reset_io[isp_dev] = (u8)_reset_gpio;
        BYD20a6_power_io[isp_dev] = (u8)_power_gpio;
    }
    if (iic == NULL) {
        printf("BYD20a6 iic open err!!!\n\n");
        return -1;
    }
    BYD20a6_reset(isp_dev);

    if (0 != BYD20a6_ID_check()) {
        dev_close(iic);
        iic = NULL;
        printf("-------not BYD20a6------\n\n");
        return -1;
    }
    printf("-------hello BYD20a6------\n\n");
    return 0;
}


static s32 BYD20a6_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n BYD20a6 \n\n");
    if (0 != BYD20a6_check(isp_dev, 0, 0)) {
        return -1;
    }
    BYD20a6_config_SENSOR(width, height, format, frame_freq);
    return 0;
}

void set_rev_sensor_BYD20a6(u16 rev_flag)
{
    if (!rev_flag) {
        wrBYD20a6Reg(0x1e, 0x30);
    } else {
        wrBYD20a6Reg(0x1e, 0x00);
    }
}

u16 BYD20a6_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdBYD20a6Reg((u8)addr, &val);
    return val;
}

void BYD20a6_dvp_wr_reg(u16 addr, u16 val)
{
    wrBYD20a6Reg((u8)addr, (u8)val);
}
// *INDENT-OFF*
#if BYD20A6_OUT_DVP
REGISTER_CAMERA(BYD20a6) = {
#else
REGISTER_CAMERA1(BYD20a6) = {
#endif
    .logo 				= 	"BYD20a6",
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
    .sen_size 			= 	{BYD20A6_DEVP_INPUT_W, BYD20A6_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{BYD20A6_DEVP_INPUT_W, BYD20A6_DEVP_INPUT_H},


    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	BYD20a6_check,
        .init 		        = 	BYD20a6_init,
        .set_size_fps 		=	BYD20a6_set_output_size,
        .power_ctrl         =   BYD20a6_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	BYD20a6_dvp_wr_reg,
        .read_reg 		    =	BYD20a6_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_BYD20a6,
    }
};


