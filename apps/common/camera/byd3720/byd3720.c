#include "asm/iic.h"
#include "asm/isp_dev.h"
#include "byd3720.h"
#include "gpio.h"
#include "generic/jiffies.h"
#include "device/iic.h"
#include "device/device.h"
#include "app_config.h"


static void *iic = NULL;
static u8 BYD3720_reset_io[2] = {-1, -1};
static u8 BYD3720_power_io[2] = {-1, -1};


#if (CONFIG_VIDEO_IMAGE_W > 1600)
#define BYD3720_DEVP_INPUT_W 1600
#define BYD3720_DEVP_INPUT_H 1200
#else
#define BYD3720_DEVP_INPUT_W 		CONFIG_VIDEO_IMAGE_W
#define BYD3720_DEVP_INPUT_H		CONFIG_VIDEO_IMAGE_H
#endif

#define BYD3720_WRCMD 0xdc
#define BYD3720_RDCMD 0xdd

#define CONFIG_INPUT_FPS 15



#define DELAY_TIME	10
static s32 BYD3720_set_output_size(u16 *width, u16 *height, u8 *freq);
static unsigned char wrBYD3720Reg(unsigned char regID, unsigned char regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, BYD3720_WRCMD)) {
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

static unsigned char rdBYD3720Reg(unsigned char regID, unsigned char *regDat)
{
    u8 ret = 1;
    dev_ioctl(iic, IIC_IOCTL_START, 0);
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, BYD3720_WRCMD)) {
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
    if (dev_ioctl(iic, IIC_IOCTL_TX_WITH_START_BIT, BYD3720_RDCMD)) {
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

static void BYD3720_config_SENSOR(u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{

    //BF3720 YUV
    //XCLK:24M HZ; MCLK:24M HZ
    //行长：1920  帧长：1218
    //wrBYD3720Reg(0x12,0x80);
    wrBYD3720Reg(0x09, 0x11);
    wrBYD3720Reg(0x15, 0x02);
    wrBYD3720Reg(0x1b, 0x6f);
    wrBYD3720Reg(0x12, 0x00);
    wrBYD3720Reg(0x93, 0x00);
    wrBYD3720Reg(0xbb, 0x20);
    wrBYD3720Reg(0x2f, 0x10);
    wrBYD3720Reg(0x30, 0xc0);
    wrBYD3720Reg(0x27, 0xc1);
    wrBYD3720Reg(0x20, 0x40);
    wrBYD3720Reg(0xe1, 0x98);
    wrBYD3720Reg(0xe4, 0x0f);
    wrBYD3720Reg(0xe3, 0x04);
    wrBYD3720Reg(0xec, 0xa8);
    wrBYD3720Reg(0xeb, 0xd5);
    wrBYD3720Reg(0xd2, 0x19);
    wrBYD3720Reg(0x16, 0xc0);
    wrBYD3720Reg(0x3f, 0x28);
    wrBYD3720Reg(0xa9, 0x5a);
    wrBYD3720Reg(0x24, 0x70);
    wrBYD3720Reg(0x3b, 0x00);
    wrBYD3720Reg(0xb1, 0xf0);
    wrBYD3720Reg(0xb2, 0xf0);
    wrBYD3720Reg(0x3b, 0x01);
    wrBYD3720Reg(0xb1, 0xf0);
    wrBYD3720Reg(0xb2, 0xb0);
    wrBYD3720Reg(0xd4, 0x4a);
    wrBYD3720Reg(0x51, 0x39);
    wrBYD3720Reg(0x52, 0x98);
    wrBYD3720Reg(0x53, 0x5f);
    wrBYD3720Reg(0x54, 0x05);
    wrBYD3720Reg(0x57, 0xd9);
    wrBYD3720Reg(0x58, 0xd4);
    wrBYD3720Reg(0x59, 0x62);
    wrBYD3720Reg(0x5a, 0x84);
    wrBYD3720Reg(0x5b, 0x22);
    wrBYD3720Reg(0x5d, 0x95);
    wrBYD3720Reg(0xd3, 0x00);
    wrBYD3720Reg(0x98, 0x23);
    wrBYD3720Reg(0xee, 0x88);
    wrBYD3720Reg(0x3e, 0x4b);
    wrBYD3720Reg(0xd0, 0x00);
    wrBYD3720Reg(0xd1, 0x93);
    wrBYD3720Reg(0x40, 0x56);
    wrBYD3720Reg(0x41, 0x56);
    wrBYD3720Reg(0x42, 0x62);
    wrBYD3720Reg(0x43, 0x5a);
    wrBYD3720Reg(0x44, 0x52);
    wrBYD3720Reg(0x45, 0x4a);
    wrBYD3720Reg(0x46, 0x43);
    wrBYD3720Reg(0x47, 0x3c);
    wrBYD3720Reg(0x48, 0x36);
    wrBYD3720Reg(0x49, 0x32);
    wrBYD3720Reg(0x4b, 0x2e);
    wrBYD3720Reg(0x4c, 0x2b);
    wrBYD3720Reg(0x4e, 0x27);
    wrBYD3720Reg(0x4f, 0x23);
    wrBYD3720Reg(0x50, 0x20);
    wrBYD3720Reg(0x97, 0x70);
    wrBYD3720Reg(0x25, 0x88);
    wrBYD3720Reg(0xd4, 0x0a);
    wrBYD3720Reg(0x51, 0x06);
    wrBYD3720Reg(0x52, 0x0e);
    wrBYD3720Reg(0x53, 0x08);
    wrBYD3720Reg(0x54, 0x15);
    wrBYD3720Reg(0x57, 0x35);
    wrBYD3720Reg(0x58, 0x20);
    wrBYD3720Reg(0x59, 0x1b);
    wrBYD3720Reg(0x5a, 0x43);
    wrBYD3720Reg(0x5b, 0x28);
    wrBYD3720Reg(0x5d, 0x95);
    wrBYD3720Reg(0xd3, 0x00);
    wrBYD3720Reg(0x72, 0x72);
    wrBYD3720Reg(0x7a, 0x33);
    wrBYD3720Reg(0x23, 0x44);
    wrBYD3720Reg(0x70, 0x87);
    wrBYD3720Reg(0xa5, 0x30);
    wrBYD3720Reg(0xae, 0x33);
    wrBYD3720Reg(0xa7, 0x97);
    wrBYD3720Reg(0xa8, 0x19);
    wrBYD3720Reg(0xa9, 0x5a);
    wrBYD3720Reg(0xaa, 0x52);
    wrBYD3720Reg(0xab, 0x18);
    wrBYD3720Reg(0xac, 0x30);
    wrBYD3720Reg(0xad, 0xf0);
    wrBYD3720Reg(0x39, 0x0f);
    wrBYD3720Reg(0xa3, 0x27);
    wrBYD3720Reg(0x3b, 0x01);
    wrBYD3720Reg(0x5c, 0x00);
    wrBYD3720Reg(0x82, 0x2a);
    wrBYD3720Reg(0xb3, 0x44);
    wrBYD3720Reg(0x86, 0x90);
    wrBYD3720Reg(0x8a, 0x3e);
    wrBYD3720Reg(0xa4, 0x10);
    wrBYD3720Reg(0x3a, 0x02); //uyuv
    wrBYD3720Reg(0x89, 0x55); //\10帧
    wrBYD3720Reg(0x8a, 0x4e); //\10帧

#if (BYD3720_DEVP_INPUT_W == 1600)
    wrBYD3720Reg(0x4a, 0x00);//1600*1200
#else
    wrBYD3720Reg(0x4a, 0x70);//1280*720
    wrBYD3720Reg(0x17, 0x00);
    wrBYD3720Reg(0x18, 0xa0);
    wrBYD3720Reg(0x19, 0x00);
    wrBYD3720Reg(0x1a, 0x5a);
#endif
}
static s32 BYD3720_set_output_size(u16 *width, u16 *height, u8 *freq)
{
    u16 liv_width = *width;
    u16 liv_height = *height;

#if (BYD3720_DEVP_INPUT_W == 1600)
    wrBYD3720Reg(0x4a, 0x40);//1600*1200
#else
    wrBYD3720Reg(0x4a, 0x70);//1280*720,1600*1200不用设置下面的。
    wrBYD3720Reg(0x17, 0x00);
    wrBYD3720Reg(0x18, 0xa0);
    wrBYD3720Reg(0x19, 0x00);
    wrBYD3720Reg(0x1a, 0x5a);
#endif

    printf("BYD3720 : %d , %d \n", *width, *height);
    return 0;
}
static s32 BYD3720_power_ctl(u8 isp_dev, u8 is_work)
{
    return 0;
}

static s32 BYD3720_ID_check(void)
{
    u16 pid = 0x00;
    u8 id0, id1;
    rdBYD3720Reg(0xfc, &id0);
    rdBYD3720Reg(0xfd, &id1);
    pid = (u16)((id0 << 8) | id1);
    printf("BYD3720 Sensor ID : 0x%x\n", pid);
    if (pid != 0x3720) {
        return -1;
    }

    return 0;
}

static void BYD3720_powerio_ctl(u32 _power_gpio, u32 on_off)
{
    gpio_direction_output(_power_gpio, on_off);
}
static void BYD3720_reset(u8 isp_dev)
{
    u8 res_io;
    u8 powd_io;
    u8 id = 0;
    puts("BYD3720 reset\n");

    if (isp_dev == ISP_DEV_0) {
        res_io = (u8)BYD3720_reset_io[0];
        powd_io = (u8)BYD3720_power_io[0];
    } else {
        res_io = (u8)BYD3720_reset_io[1];
        powd_io = (u8)BYD3720_power_io[1];
    }

    if (powd_io != (u8) - 1) {
        BYD3720_powerio_ctl((u32)powd_io, 0);
    }
    if (res_io != (u8) - 1) {
        gpio_direction_output(res_io, 1);
        gpio_direction_output(res_io, 0);
        os_time_dly(1);
        gpio_direction_output(res_io, 1);
    }
}


static s32 BYD3720_check(u8 isp_dev, u32 _reset_gpio, u32 _power_gpio)
{
    if (!iic) {
        if (isp_dev == ISP_DEV_0) {
            iic = dev_open("iic0", 0);
        } else {
            iic = dev_open("iic1", 0);
        }
        BYD3720_reset_io[isp_dev] = (u8)_reset_gpio;
        BYD3720_power_io[isp_dev] = (u8)_power_gpio;
    }
    if (iic == NULL) {
        printf("BYD3720 iic open err!!!\n\n");
        return -1;
    }
    BYD3720_reset(isp_dev);

    if (0 != BYD3720_ID_check()) {
        dev_close(iic);
        iic = NULL;
        printf("-------not BYD3720------\n\n");
        return -1;
    }
    printf("-------hello BYD3720------\n\n");
    return 0;
}


static s32 BYD3720_init(u8 isp_dev, u16 *width, u16 *height, u8 *format, u8 *frame_freq)
{
    puts("\n\n BYD3720 \n\n");

    if (0 != BYD3720_check(isp_dev, 0, 0)) {
        return -1;
    }
    BYD3720_config_SENSOR(width, height, format, frame_freq);

    return 0;
}
void set_rev_sensor_BYD3720(u16 rev_flag)
{
    if (!rev_flag) {
        wrBYD3720Reg(0x1e, 0x30);
    } else {
        wrBYD3720Reg(0x1e, 0x00);
    }
}

u16 BYD3720_dvp_rd_reg(u16 addr)
{
    u8 val;
    rdBYD3720Reg((u8)addr, &val);
    return val;
}

void BYD3720_dvp_wr_reg(u16 addr, u16 val)
{
    wrBYD3720Reg((u8)addr, (u8)val);
}

// *INDENT-OFF*
REGISTER_CAMERA(BYD3720) = {
    .logo 				= 	"BYD3720",
    .isp_dev 			= 	ISP_DEV_NONE,
    .in_format 			= 	SEN_IN_FORMAT_UYVY,
    .mbus_type          =   SEN_MBUS_PARALLEL,
    .mbus_config        =   SEN_MBUS_DATA_WIDTH_8B  | SEN_MBUS_HSYNC_ACTIVE_HIGH | \
    						SEN_MBUS_PCLK_SAMPLE_FALLING | SEN_MBUS_VSYNC_ACTIVE_HIGH,
#if CONFIG_CAMERA_H_V_EXCHANGE
    .sync_config		=   SEN_MBUS_SYNC0_VSYNC_SYNC1_HSYNC,//WL82/AC791才可以H-V SYNC互换，请留意
#endif
    .fps         		= 	CONFIG_INPUT_FPS,
    .out_fps			=   CONFIG_INPUT_FPS,//输出帧率
    .sen_size 			= 	{BYD3720_DEVP_INPUT_W, BYD3720_DEVP_INPUT_H},
    .cap_fps         	= 	CONFIG_INPUT_FPS,
    .sen_cap_size 		= 	{BYD3720_DEVP_INPUT_W, BYD3720_DEVP_INPUT_H},

    .ops                =   {
        .avin_fps           =   NULL,
        .avin_valid_signal  =   NULL,
        .avin_mode_det      =   NULL,
        .sensor_check 		= 	BYD3720_check,
        .init 		        = 	BYD3720_init,
        .set_size_fps 		=	BYD3720_set_output_size,
        .power_ctrl         =   BYD3720_power_ctl,


        .sleep 		        =	NULL,
        .wakeup 		    =	NULL,
        .write_reg 		    =	BYD3720_dvp_wr_reg,
        .read_reg 		    =	BYD3720_dvp_rd_reg,
        .set_sensor_reverse =   set_rev_sensor_BYD3720,
    }
};


