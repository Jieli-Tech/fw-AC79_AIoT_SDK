#include "app_config.h"
#include "device/iic.h"
#include "server/audio_dev.h"
#include "system/includes.h"
#include "device/includes.h"
#include "asm/includes.h"

// *INDENT-OFF*

/*
 *debug打印
 */
UART2_PLATFORM_DATA_BEGIN(uart2_data)
	.baudrate = 460800,
	.port = PORT_REMAP,
	.output_channel = OUTPUT_CHANNEL0,
	.tx_pin = IO_PORTC_06,
	.rx_pin = -1,
	.max_continue_recv_cnt = 1024,
	.idle_sys_clk_cnt = 500000,
	.clk_src = PLL_48M,
	.flags = UART_DEBUG,
UART2_PLATFORM_DATA_END();

/*
 *摄像头IIC
 */
SW_IIC_PLATFORM_DATA_BEGIN(sw_iic0_data)
	/*.clk_pin = IO_PORTA_03,*/
	/*.dat_pin = IO_PORTA_04,*/
	.clk_pin = IO_PORTC_01,
	.dat_pin = IO_PORTC_02,
	.sw_iic_delay = 50,
SW_IIC_PLATFORM_DATA_END()


/*
 *摄像头IO-PORT
 */
#ifdef CONFIG_VIDEO_ENABLE
#define CAMERA_GROUP_PORT	ISC_GROUPA
/* #define CAMERA_GROUP_PORT	ISC_GROUPC */
static const struct camera_platform_data camera0_data = {
    .xclk_gpio      = IO_PORTC_00,//注意： 如果硬件xclk接到芯片IO，则会占用OUTPUT_CHANNEL1
    .reset_gpio     = IO_PORTC_03,
    .online_detect  = NULL,
    .pwdn_gpio      = -1,
    .power_value    = 0,
    .interface      = SEN_INTERFACE0,//SEN_INTERFACE_CSI2,
    .dvp={
#if (CAMERA_GROUP_PORT == ISC_GROUPA)
        .pclk_gpio   = IO_PORTA_08,
        .hsync_gpio  = IO_PORTA_09,
        .vsync_gpio  = IO_PORTA_10,
#else
        .pclk_gpio   = IO_PORTC_08,
        .hsync_gpio  = IO_PORTC_09,
        .vsync_gpio  = IO_PORTC_10,
#endif
		.group_port  = CAMERA_GROUP_PORT,
        .data_gpio={
#if (CAMERA_GROUP_PORT == ISC_GROUPA)
                IO_PORTA_07,
                IO_PORTA_06,
                IO_PORTA_05,
                IO_PORTA_04,
                IO_PORTA_03,
                IO_PORTA_02,
                IO_PORTA_01,
                IO_PORTA_00,
				-1,
				-1,
#else
                IO_PORTC_07,
                IO_PORTC_06,
                IO_PORTC_05,
                IO_PORTC_04,
                IO_PORTC_03,
                IO_PORTC_02,
                IO_PORTC_01,
                IO_PORTC_00,
				-1,
				-1,
#endif
        },
    }
};
static const struct video_subdevice_data video0_subdev_data[] = {
    { VIDEO_TAG_CAMERA, (void *)&camera0_data },
};
static const struct video_platform_data video0_data = {
    .data = video0_subdev_data,
    .num = ARRAY_SIZE(video0_subdev_data),
};
#endif

#ifdef CONFIG_VIDEO1_ENABLE //SPI_VIDEO
static const struct camera_platform_data camera1_data = {
    .xclk_gpio      = -1,//IO_PORTC_08,//IO_PORTC_06,//注意： 如果硬件xclk接到芯片IO，则会占用OUTPUT_CHANNEL1
    .reset_gpio     = -1,
    .online_detect  = NULL,
    .pwdn_gpio      = -1,
    .power_value    = 0,
    .interface      = -1,
    .dvp={
		.group_port  = -1,
		.pclk_gpio   = -1,
		.hsync_gpio  = -1,
		.vsync_gpio  = -1,
		.data_gpio={-1},
    }
};
static const struct video_subdevice_data video1_subdev_data[] = {
    { VIDEO_TAG_CAMERA, (void *)&camera1_data },
};
static const struct video_platform_data video1_data = {
    .data = video1_subdev_data,
    .num = ARRAY_SIZE(video1_subdev_data),
};
#endif
/*
 *MIC
 */
static const struct adc_platform_data adc_data = {
    .mic_channel = LADC_CH_MIC1_P_N,// | LADC_CH_MIC3_P_N
    .mic_ch_num = 1,//硬件总共MIC个数,默认1个
    .isel = 2,
};
static const struct audio_pf_data audio_pf_d = {
    .adc_pf_data = &adc_data,
};
static const struct audio_platform_data audio_data = {
    .private_data = (void *) &audio_pf_d,
};


REGISTER_DEVICES(device_table) = {
	{ "iic0",  &iic_dev_ops, (void *)&sw_iic0_data },
	{ "audio", &audio_dev_ops, (void *)&audio_data },
#ifdef CONFIG_VIDEO_ENABLE
    { "video0.*",  &video_dev_ops, (void *)&video0_data },
#endif
#ifdef CONFIG_VIDEO1_ENABLE //SPI_VIDEO
    { "video1.*",  &video_dev_ops, (void *)&video1_data },
#endif
	{"uart2", &uart_dev_ops, (void *)&uart2_data },
};


//电源配置以及低功耗相关配置
static const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,// | SLEEP_HW_EN,          //SLEEP_HW_EN:使能wifi和蓝牙硬件休眠，SLEEP_EN:使能系统休眠
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.4V  3.6V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
	.vdc14_dcdc 	= TRUE,	   							   //打开内部1.4VDCDC，关闭则用外部
    .vdc14_lev		= VDC14_VOL_SEL_LEVEL, 				   //VDD1.4V配置
	.sysvdd_lev		= SYSVDD_VOL_SEL_LEVEL,				   //内核、sdram电压配置
	.vlvd_enable	= TRUE,                                //TRUE电压复位使能
	.vlvd_value		= VLVD_SEL_25V,                        //低电压复位电压值
};


static void board_power_init(void)
{
    power_init(&power_param);
}

#ifdef CONFIG_DEBUG_ENABLE
void debug_uart_init()
{
    uart_init(&uart2_data);
}
#endif

void board_early_init()
{
    dac_early_init(0, 0, 1000);
    devices_init();
}

void board_init()
{
	board_power_init();
}


