#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "h7375s.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"


static void *iic = NULL;
static u8 H7375S_reset_io[2] = {-1, -1};
static u8 H7375S_power_io[2] = {-1, -1};

#if (CONFIG_VIDEO_IMAGE_W > 640)
#define H7375S_DEVP_INPUT_W 	640
#define H7375S_DEVP_INPUT_H		480
#else
#define H7375S_DEVP_INPUT_W 	CONFIG_VIDEO_IMAGE_W
#define H7375S_DEVP_INPUT_H		CONFIG_VIDEO_IMAGE_H
#endif

#define H7375S_WRCMD 0xd0
#define H7375S_RDCMD 0xd1

#define CONFIG_INPUT_FPS	25

#define DELAY_TIME	10

struct reginfo {
    u8 reg;
    u8 val;
};
static const struct reginfo sensor_init_data[] = {
    {0xf0, 0x01},
    {0xd4, 0x11},
    {0xf0, 0x00},
    {0x02, 0x69},
    {0x0b, 0x0c},
    {0x28, 0x06},
    {0x30, 0xa9},
    {0x35, 0xa8},
    {0x38, 0x80},
    {0x50, 0x00},
    {0x70, 0x4f},
    {0x72, 0x44},
    {0x76, 0x1c},
    {0x7c, 0x2a},
    {0x7e, 0x11},
    {0x8b, 0x10},
    {0x9e, 0xb2},
    {0xb0, 0x81},
    {0xb2, 0x14},
    {0xb3, 0x25},
    {0xb4, 0x0b},
    {0xb5, 0x20},
    {0xbc, 0x1c},
    {0xbd, 0x11},
    {0xc0, 0x03},
    {0xc1, 0xcc},
    {0xc4, 0x2c},
    {0xc7, 0x00},
    {0xe0, 0x0f},
    {0xe7, 0xef},
    {0xea, 0xb0},
    {0xf0, 0x01},
    {0x20, 0x10},
    {0x21, 0x98},
    {0x22, 0x08},
    {0x23, 0x8c},
    {0x24, 0x20},
    {0x25, 0x94},
    {0x26, 0x18},
    {0x27, 0xc8},
    {0x28, 0x30},
    {0x29, 0x11},
    {0x48, 0xe6},
    {0x49, 0xc0},
    {0x4a, 0xd0},
    {0x4b, 0x48},
    {0x4d, 0x00},
    {0x4f, 0x88},
    {0x70, 0x02},
    {0x71, 0x82},
    {0x72, 0x08},
    {0x73, 0x02},
    {0x74, 0xe2},
    {0x75, 0x10},
    {0x77, 0x08},
    {0x79, 0x04},
    {0x7a, 0x0e},
    {0xd1, 0xc1},
    {0xd2, 0x00},
    {0xd3, 0x00},
    {0xd6, 0x10},
    {0xd7, 0x10},
    {0xf0, 0x00},
};

static s32 H7375S_set_output_size(u16 *width, u16 *height, u8 *freq);
static unsigned char wrH7375SReg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, H7375S_WRCMD)) {
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

static unsigned char rdH7375SReg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, H7375S_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, H7375S_RDCMD)) {
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

static void H7375S_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    int i;
    for (i = 0; i < sizeof(sensor_init_data) / sizeof(sensor_init_data[0]); i++) {
        wrH7375SReg(sensor_init_data[i].reg, sensor_init_data[i].val);
    }
    *format = SEN_IN_FORMAT_UYVY;
    *frame_freq = CONFIG_INPUT_FPS;
}
static s32 H7375S_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width;
    u16 liv_height = *height;
    return 0;
}
static s32 H7375S_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 H7375S_ID_check(void)
{
    int ret;
    u16 pid = 0x00;
    u8 id0, id1;
    ret = rdH7375SReg(0xfc, &id0);
    rdH7375SReg(0xfd, &id1);
    pid = (u16)((id0 << 8) | id1);
    printf("H7375S Sensor ID : 0x%x\n", pid);
    if (pid != 0x1703) {
        return -1;
    }

    return 0;
}

static void H7375S_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void H7375S_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("H7375S reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = (u8)H7375S_reset_io[0];
        powd_io = (u8)H7375S_power_io[0];
    } else {
        res_io = (u8)H7375S_reset_io[1];
        powd_io = (u8)H7375S_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        H7375S_powerio_ctl((u32)powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}


static s32 H7375S_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        H7375S_reset_io[isp_dev] = (u8)_reset_gpio;
        H7375S_power_io[isp_dev] = (u8)_power_gpio;
    }
    if (iic == NULL) {
        printf("H7375S iic open err!!!\n\n");
        return -1;
    }
    H7375S_reset(isp_dev);

    if (0 != H7375S_ID_check()) {
        dev_close(iic);
        iic = NULL;
        printf("-------not H7375S------\n\n");
        return -1;
    }
    printf("-------hello H7375S------\n\n");
    return 0;
}


static s32 H7375S_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("H7375S \n\n");
    if (0 != H7375S_check(isp_dev, 0, 0)) {
        return -1;
    }
    H7375S_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_H7375S(u16 rev_flag)
{
    if (!rev_flag) {
        wrH7375SReg(0x0b, 0x0c);
    } else {
        wrH7375SReg(0x0b, 0x04);
    }
}

u16 H7375S_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdH7375SReg((u8)addr, &val);
    return val;
}

void H7375S_dvp_wr_reg(u16 addr, u16 val)
{
    wrH7375SReg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA(H7375S) = {
    .logo 				= 	"H7375S",
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
    .sen_size 			= 	{H7375S_DEVP_INPUT_W, H7375S_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{H7375S_DEVP_INPUT_W, H7375S_DEVP_INPUT_H},


    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	H7375S_check,
        .init 		        = 	H7375S_init,
        .set_size_fps 		=	H7375S_set_output_size,
        .power_ctrl         =   H7375S_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	H7375S_dvp_wr_reg,
        .read_reg 		    =	H7375S_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_H7375S,
    }
};


