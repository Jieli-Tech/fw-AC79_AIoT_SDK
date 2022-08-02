#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "byd3901.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"


static void *iic = NULL;
static u8 BYD3901_reset_io[2] = {-1, -1};
static u8 BYD3901_power_io[2] = {-1, -1};

#define BYD3901_DEVP_INPUT_W 		240
#define BYD3901_DEVP_INPUT_H		320

#define BYD3901_WRCMD 0xdc
#define BYD3901_RDCMD 0xdd

#define CONFIG_INPUT_FPS	15

#define BYD3901_OUT_CLK		24 //24M 48M

#define DELAY_TIME	10
struct reginfo {
    u8 reg;
    u8 val;
};
static const struct reginfo sensor_init_data[] = {
    //MTK  SPI Mode
    //XCLK:24M  SCLK:48M  MCLK:6M
    //行长：320  帧长：240
    //max fps:30fps
    {0x12, 0x80},
    {0x11, 0x30},

#if BYD3901_OUT_CLK == 48
    {0x1b, 0x06},//SCLK:48M.MCLK:6M
#else
    {0x1b, 0x80},//SCLK:24M.MCLK:6M
#endif

    {0x0b, 0x03},

    /*{0x6b, 0x01},//0x6b[6]=1,打开CCIR656编码方式。*/
    {0x6b, 0x71},//0x6b[6]=1,打开CCIR656编码方式。
    {0x12, 0x10},//0x12[5:4]=10

    {0x3a, 0x00},
    {0x15, 0x22},
    {0x62, 0x81},//0x62[1:0]:2'01,SPI mode
    {0x08, 0xa0},
    {0x06, 0x68},
    {0x2b, 0x00},
    {0x27, 0x97},

    /******240 * 320**********/
    {0x17, 0x00},
    {0x18, (u8)((BYD3901_DEVP_INPUT_W >> 1) & 0xFF)},
    {0x19, 0x00},
    {0x1a, (u8)((BYD3901_DEVP_INPUT_H >> 1) & 0xFF)},
    {0x03, (u8)((((BYD3901_DEVP_INPUT_W & 0x1) << 1) | ((BYD3901_DEVP_INPUT_H & 0x1) << 3)) & 0xFF)},
    /****************/

    {0x13, 0x00},
    {0x01, 0x13},
    {0x02, 0x20},
    {0x87, 0x16},
    {0x8c, 0x01},
    {0x8d, 0xcc},
    {0x13, 0x07},
    {0x33, 0x10},
    {0x34, 0x1d},
    {0x35, 0x46},
    {0x36, 0x40},
    {0x37, 0xa4},
    {0x38, 0x7c},
    {0x65, 0x46},
    {0x66, 0x46},
    {0x6e, 0x20},
    {0x9b, 0xa4},
    {0x9c, 0x7c},
    {0xbc, 0x0c},
    {0xbd, 0xa4},
    {0xbe, 0x7c},
    {0x20, 0x09},
    /*{0x09, 0x03},*/
    {0x72, 0x54},
    {0x73, 0x27},
    {0x74, 0x87},
    {0x75, 0x12},
    {0x79, 0x8c},
    {0x7a, 0x00},
    {0x7e, 0xfa},
    {0x70, 0x0f},
    {0x7c, 0x84},
    {0x7d, 0xba},
    {0x5b, 0xc2},
    {0x76, 0x90},
    {0x7b, 0x55},
    {0x71, 0x46},
    {0x77, 0xdd},
    {0x13, 0x0f},
    {0x8a, 0x10},
    {0x8b, 0x20},
    {0x8e, 0x21},
    {0x8f, 0x40},
    {0x94, 0x41},
    {0x95, 0x7e},
    {0x96, 0x7f},
    {0x97, 0xf3},
    {0x13, 0x07},
    {0x24, 0x50},
    {0x97, 0x48},
    {0x25, 0x08},
    {0x94, 0xb5},
    {0x95, 0xc0},
    {0x80, 0xf6},
    {0x81, 0xe0},
    {0x82, 0x1b},
    {0x83, 0x37},
    {0x84, 0x39},
    {0x85, 0x58},
    {0x86, 0xc0},
    {0x8a, 0x5c},//33
    {0xf1, 0x00},//33
#if CONFIG_INPUT_FPS == 15
    //3d 、16 fps   0x2d=20fps 0x25=25fps
    {0x89, 0x25},
#else
    //10 fps
    {0x89, 0x55},
    {0x92, 0x6d},//10 fps
#endif

    {0x8b, 0x4c},
    {0x39, 0x80},
    {0x3f, 0x80},
    {0x90, 0xa0},
    {0x91, 0xe0},
    {0x40, 0x20},
    {0x41, 0x28},
    {0x42, 0x26},
    {0x43, 0x25},
    {0x44, 0x1f},
    {0x45, 0x1a},
    {0x46, 0x16},
    {0x47, 0x12},
    {0x48, 0x0f},
    {0x49, 0x0d},
    {0x4b, 0x0b},
    {0x4c, 0x0a},
    {0x4e, 0x08},
    {0x4f, 0x06},
    {0x50, 0x06},
    {0x40, 0x17},
    {0x41, 0x27},
    {0x42, 0x25},
    {0x43, 0x23},
    {0x44, 0x20},
    {0x45, 0x18},
    {0x46, 0x16},
    {0x47, 0x13},
    {0x48, 0x0f},
    {0x49, 0x0c},
    {0x4b, 0x0b},
    {0x4c, 0x0a},
    {0x4e, 0x08},
    {0x4f, 0x08},
    {0x50, 0x06},
    {0x5a, 0x56},
    {0x51, 0x1b},
    {0x52, 0x04},
    {0x53, 0x4a},
    {0x54, 0x26},
    {0x57, 0x75},
    {0x58, 0x2b},
    {0x5a, 0xd6},
    {0x51, 0x28},
    {0x52, 0x1e},
    {0x53, 0x9e},
    {0x54, 0x70},
    {0x57, 0x50},
    {0x58, 0x07},
    {0x5c, 0x28},
    {0xb0, 0xe0},
    {0xb1, 0x90},
    {0xb2, 0x80},
    {0xb3, 0x4f},
    {0xb4, 0x63},
    {0xb4, 0xe3},
    {0xb1, 0x90},
    {0xb2, 0xa0},
    {0x55, 0x00},
    {0x56, 0x40},
    {0x96, 0x50},
    {0x9a, 0x30},
    {0x6a, 0x81},
    {0x23, 0x11},
    {0xa0, 0xd0},
    {0xa1, 0x31},
    {0xa6, 0x04},
    {0xa2, 0x0f},
    {0xa3, 0x2b},
    {0xa4, 0x08},
    {0xa5, 0x25},
    {0xa7, 0x9a},
    {0xa8, 0x1c},
    {0xa9, 0x11},
    {0xaa, 0x16},
    {0xab, 0x16},
    {0xac, 0x3c},
    {0xad, 0xf0},
    {0xae, 0x57},
    {0xc6, 0xaa},
    {0xd2, 0x78},
    {0xd0, 0xb4},
    {0xd1, 0x00},
    {0xc8, 0x10},
    {0xc9, 0x12},
    {0xd3, 0x09},
    {0xd4, 0x2a},
    {0xee, 0x4c},
    {0x7e, 0xfa},
    {0x74, 0xa3},
    {0x78, 0x4e},
    {0x60, 0xe7},
    {0x61, 0xc8},
    {0x6d, 0x70},
    {0x1e, 0x09},   //0x39  0x09  修改图像正反
    {0x98, 0x1a},
    {0xbb, 0x20},

    {0x09, 0x03},
};

static s32 BYD3901_set_output_size(u16 *width, u16 *height, u8 *freq);
static unsigned char wrBYD3901Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, BYD3901_WRCMD)) {
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

static unsigned char rdBYD3901Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, BYD3901_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, BYD3901_RDCMD)) {
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

static void BYD3901_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    int i;
    for (i = 0; i < sizeof(sensor_init_data) / sizeof(sensor_init_data[0]); i++) {
        wrBYD3901Reg(sensor_init_data[i].reg, sensor_init_data[i].val);
    }
#if BYD3901_OUT_CLK == 48
    os_time_dly(8);//24M务必延时130ms以上 ,48M务必延时80ms以上
#else
    os_time_dly(13);//24M务必延时130ms以上 ,48M务必延时80ms以上
#endif

    *format = SEN_IN_FORMAT_UYVY;
    *frame_freq = CONFIG_INPUT_FPS;
}
static s32 BYD3901_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width;
    u16 liv_height = *height;

    return 0;
}
static s32 BYD3901_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 BYD3901_ID_check(void)
{
    int ret;
    u16 pid = 0x00;
    u8 id0, id1;
    ret = rdBYD3901Reg(0xfc, &id0);
    rdBYD3901Reg(0xfd, &id1);
    pid = (u16)((id0 << 8) | id1);
    printf("BYD3901 Sensor ID : 0x%x\n", pid);
    if (pid != 0x3901) {
        return -1;
    }

    return 0;
}

static void BYD3901_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void BYD3901_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("BYD3901 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = (u8)BYD3901_reset_io[0];
        powd_io = (u8)BYD3901_power_io[0];
    } else {
        res_io = (u8)BYD3901_reset_io[1];
        powd_io = (u8)BYD3901_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        BYD3901_powerio_ctl((u32)powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}


static s32 BYD3901_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        BYD3901_reset_io[isp_dev] = (u8)_reset_gpio;
        BYD3901_power_io[isp_dev] = (u8)_power_gpio;
    }
    if (iic == NULL) {
        printf("BYD3901 iic open err!!!\n\n");
        return -1;
    }
    BYD3901_reset(isp_dev);

    if (0 != BYD3901_ID_check()) {
        dev_close(iic);
        iic = NULL;
        printf("-------not BYD3901------\n\n");
        return -1;
    }
    printf("-------hello BYD3901------\n\n");
    return 0;
}


static s32 BYD3901_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n BYD3901 \n\n");
    if (0 != BYD3901_check(isp_dev, 0, 0)) {
        return -1;
    }
    BYD3901_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_BYD3901(u16 rev_flag)
{
    if (!rev_flag) {
        wrBYD3901Reg(0x1e, 0x30);
    } else {
        wrBYD3901Reg(0x1e, 0x00);
    }
}

u16 BYD3901_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdBYD3901Reg((u8)addr, &val);
    return val;
}

void BYD3901_dvp_wr_reg(u16 addr, u16 val)
{
    wrBYD3901Reg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA1(BYD3901) = {
    .logo 				= 	"BYD3901",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_YUYV,
    .mbus_type          =   SEN_MBUS_BT656,
#ifdef CONFIG_CPU_WL82
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_1B | SEN_MBUS_PCLK_SAMPLE_FALLING,
#else
	.mbus_config        =  0,
#endif
    .sync_config		=   0,//WL82/AC791才可以H-V SYNC互换，请留意
    .fps         		= 	CONFIG_INPUT_FPS,
    .out_fps			=   CONFIG_INPUT_FPS,
    .sen_size 			= 	{BYD3901_DEVP_INPUT_W, BYD3901_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{BYD3901_DEVP_INPUT_W, BYD3901_DEVP_INPUT_H},


    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	BYD3901_check,
        .init 		        = 	BYD3901_init,
        .set_size_fps 		=	BYD3901_set_output_size,
        .power_ctrl         =   BYD3901_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	BYD3901_dvp_wr_reg,
        .read_reg 		    =	BYD3901_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_BYD3901,
    }
};


