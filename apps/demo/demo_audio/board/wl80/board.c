#include "app_config.h"

#include "system/includes.h"
#include "device/includes.h"
#include "asm/includes.h"
#include "server/audio_dev.h"
#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE
#include "otg.h"
#include "usb_host.h"
#include "usb_storage.h"
#endif

// *INDENT-OFF*


UART2_PLATFORM_DATA_BEGIN(uart2_data)
	.baudrate = 1000000,
	.port = PORT_REMAP,
	.output_channel = OUTPUT_CHANNEL1,
	.tx_pin = IO_PORTC_03,
	.rx_pin = -1,
	.max_continue_recv_cnt = 1024,
	.idle_sys_clk_cnt = 500000,
	.clk_src = PLL_48M,
	.flags = UART_DEBUG,
UART2_PLATFORM_DATA_END();


#if TCFG_SD0_ENABLE
//sd0
#define SD_PLATFORM_DATA_BEGIN() SD0_PLATFORM_DATA_BEGIN(sd0_data)
#define SD_PLATFORM_DATA_END() SD0_PLATFORM_DATA_END()
#define SD_CLK_DETECT_FUNC		sdmmc_0_clk_detect
#elif TCFG_SD1_ENABLE
//sd1
#define SD_PLATFORM_DATA_BEGIN() SD1_PLATFORM_DATA_BEGIN(sd1_data)
#define SD_PLATFORM_DATA_END() SD1_PLATFORM_DATA_END()
#define SD_CLK_DETECT_FUNC		sdmmc_1_clk_detect
#endif

#if TCFG_SD0_ENABLE || TCFG_SD1_ENABLE

SD_PLATFORM_DATA_BEGIN()
	.port 					= TCFG_SD_PORTS,
	.priority 				= 3,
	.data_width 			= TCFG_SD_DAT_WIDTH,
	.speed 					= TCFG_SD_CLK,
	.detect_mode 			= TCFG_SD_DET_MODE,
#if (TCFG_SD_DET_MODE == SD_CLK_DECT)
	.detect_func 			= SD_CLK_DETECT_FUNC,
#elif (TCFG_SD_DET_MODE == SD_IO_DECT)
	.detect_func 			= sdmmc_0_io_detect,
#else
	.detect_func 			= NULL,
#endif
#if TCFG_SD1_ENABLE
    .power                  = sd_set_power,
#endif
SD_PLATFORM_DATA_END()

#endif


#if TCFG_ADKEY_ENABLE

#define ADKEY_UPLOAD_R  22

#define ADC_VDDIO (0x3FF)
#define ADC_09   (0x3FF)
#define ADC_08   (0x3FF)
#define ADC_07   (0x3FF)
#define ADC_06   (0x3FF)
#define ADC_05   (0x3FF)
#define ADC_04   (0x3FF)
#define ADC_03   (0x3FF * 15  / (15  + ADKEY_UPLOAD_R))
#define ADC_02   (0x3FF * 10  / (10  + ADKEY_UPLOAD_R))
#define ADC_01   (0x3FF * 33  / (33  + ADKEY_UPLOAD_R * 10))
#define ADC_00   (0)

#define ADKEY_V_9      	((ADC_09 + ADC_VDDIO)/2)
#define ADKEY_V_8 		((ADC_08 + ADC_09)/2)
#define ADKEY_V_7 		((ADC_07 + ADC_08)/2)
#define ADKEY_V_6 		((ADC_06 + ADC_07)/2)
#define ADKEY_V_5 		((ADC_05 + ADC_06)/2)
#define ADKEY_V_4 		((ADC_04 + ADC_05)/2)
#define ADKEY_V_3 		((ADC_03 + ADC_04)/2)
#define ADKEY_V_2 		((ADC_02 + ADC_03)/2)
#define ADKEY_V_1 		((ADC_01 + ADC_02)/2)
#define ADKEY_V_0 		((ADC_00 + ADC_01)/2)

const struct adkey_platform_data adkey_data = {
    .enable     = 1,
	.adkey_pin  = IO_PORTB_01,
    .extern_up_en = 1,
	.ad_channel = 3,
    .ad_value = {
        ADKEY_V_0,
        ADKEY_V_1,
        ADKEY_V_2,
        ADKEY_V_3,
        ADKEY_V_4,
        ADKEY_V_5,
        ADKEY_V_6,
        ADKEY_V_7,
        ADKEY_V_8,
        ADKEY_V_9,
    },
    .key_value = {
        KEY_OK,
        KEY_VOLUME_INC,
        KEY_VOLUME_DEC,
        KEY_MODE,
        NO_KEY,
        NO_KEY,
        NO_KEY,
        NO_KEY,
        NO_KEY,
        NO_KEY,
    },
};

#endif

static const struct dac_platform_data dac_data = {
    .pa_auto_mute = 1,
    .pa_mute_port = 0xff, //功放的mute脚
    .pa_mute_value = 0, //0低电平mute，高电平mute
    .differ_output = 0, //选择是否采用差分输出模式
    .hw_channel = 0x03, //BIT(0)使用DACL | BIT(1)使用DACR
    .ch_num = 2,	//差分只需开一个通道
    .vcm_init_delay_ms = 1000,
#ifdef CONFIG_DEC_ANALOG_VOLUME_ENABLE
    .fade_enable = 1, //模拟音量淡入淡出功能
    .fade_delay_ms = 40, //每级模拟音量延时40ms调整
#endif
};
static const struct adc_platform_data adc_data = {
	//整个代码运行过程中会用到多少路通道AUDIO MIC
    .mic_channel = LADC_CH_MIC1_P | LADC_CH_MIC3_P, //P代表硬件MIC直接正端，N代表硬件MIC只接负端，P_N代表硬件MIC采用差分接法
    .linein_channel = LADC_CH_AUX0,
    .mic_ch_num = 2, //整个代码运行过程中会用到多少路通道AUDIO MIC
    .linein_ch_num = 1, //整个代码运行过程中会用到多少路通道AUDIO LINEIN
    .all_channel_open = 1,
    .isel = 2,
};
static const struct iis_platform_data iis0_data = {
    .channel_in = BIT(0), //通道0设为输入
    .channel_out = BIT(1), //通道1设为输出
    .port_sel = IIS_PORTA,
    .data_width = 0, //BIT(x)代表通道x使用24bit模式
    .mclk_output = 0, //1:输出mclk 0:不输出mclk
    .slave_mode = 0, //1:从机模式 0:主机模式
};
static void plnk0_port_remap_cb(void)
{
	//重映射PDM DAT-PH4   PDM CLK-PC0
    extern int gpio_plnk_rx_input(u32 gpio, u8 index, u8 data_sel);
    gpio_plnk_rx_input(IO_PORTH_04, 0, 0);
    gpio_output_channle(IO_PORTC_00, CH0_PLNK0_SCLK_OUT);	//SCLK0使用outputchannel0
    JL_IOMAP->CON3 |= BIT(18);
}
static void plnk0_port_unremap_cb(void)
{
    JL_IOMAP->CON3 &= ~BIT(18);
    gpio_clear_output_channle(IO_PORTC_00, CH0_PLNK0_SCLK_OUT);
    gpio_set_die(IO_PORTH_04, 0);
}
//plnk的时钟和数据引脚都采用重映射的使用例子
static const struct plnk_platform_data plnk0_data = {
    .hw_channel = PLNK_CH_MIC_L,
    .clk_out = 1,
    .port_remap_cb = plnk0_port_remap_cb,
    .port_unremap_cb = plnk0_port_unremap_cb,
    .sample_edge = 0,   //在CLK的下降沿采样左MIC
    .share_data_io = 1, //两个数字MIC共用一个DAT脚
    .high_gain = 1,
    .dc_cancelling_filter = 14,
};
static const struct audio_pf_data audio_pf_d = {
    .adc_pf_data = &adc_data,
    .dac_pf_data = &dac_data,
    .iis0_pf_data = &iis0_data,
    .plnk0_pf_data = &plnk0_data,
};
static const struct audio_platform_data audio_data = {
    .private_data = (void *)&audio_pf_d,
};


#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE
static const struct otg_dev_data otg_data = {
    .usb_dev_en = 0x01,
#if TCFG_USB_SLAVE_ENABLE
    .slave_online_cnt = 10,
    .slave_offline_cnt = 10,
#endif
#if TCFG_USB_HOST_ENABLE
    .host_online_cnt = 10,
    .host_offline_cnt = 10,
#endif
    .detect_mode = OTG_HOST_MODE | OTG_SLAVE_MODE | OTG_CHARGE_MODE,
    .detect_time_interval = 20,
};
#endif

#if defined CONFIG_BT_ENABLE || defined CONFIG_WIFI_ENABLE
#include "wifi/wifi_connect.h"
const struct wifi_calibration_param wifi_calibration_param = {
    .xosc_l     = 0xb,// 调节晶振左电容
    .xosc_r     = 0xa,// 调节晶振右电容
    .pa_trim_data  = {7, 7, 1, 1, 5, 0, 7},// 根据MP测试生成PA TRIM值
    .mcs_dgain     = {
        64,//11B_1M
        64,//11B_2.2M
        64,//11B_5.5M
        64,//11B_11M

        64,//11G_6M
        64,//11G_9M
        64,//11G_12M
        64,//11G_18M
        64,//11G_24M
        38,//11G_36M
        34,//11G_48M
        27,//11G_54M

        64,//11N_MCS0
        64,//11N_MCS1
        64,//11N_MCS2
        64,//11N_MCS3
        40,//11N_MCS4
        34,//11N_MCS5
        29,//11N_MCS6
        25,//11N_MCS7
	}
};
#endif


/************************** LOW POWER config ****************************/
static const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.4V  3.6V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
	.vdc14_dcdc 	= TRUE,	   							   //打开内部1.4VDCDC，关闭则用外部
    .vdc14_lev		= VDC14_VOL_SEL_LEVEL, 				   //VDD1.4V配置
	.sysvdd_lev		= SYSVDD_VOL_SEL_LEVEL,				   //内核、sdram电压配置
	.vlvd_enable	= TRUE,                                //TRUE电压复位使能
	.vlvd_value		= VLVD_SEL_25V,                        //低电压复位电压值
};

REGISTER_DEVICES(device_table) = {
	{"uart2", &uart_dev_ops, (void *)&uart2_data },
	{"audio", &audio_dev_ops, (void *)&audio_data },
#if TCFG_SD0_ENABLE
	{ "sd0",  &sd_dev_ops, (void *)&sd0_data },
#endif
#if TCFG_SD1_ENABLE
	{ "sd1",  &sd_dev_ops, (void *)&sd1_data },
#endif
#if TCFG_USB_SLAVE_ENABLE || TCFG_USB_HOST_ENABLE
	{ "otg", &usb_dev_ops, (void *)&otg_data},
#endif
#if TCFG_UDISK_ENABLE
    { "udisk0", &mass_storage_ops, NULL },
#endif
};

#ifdef CONFIG_DEBUG_ENABLE
void debug_uart_init()
{
    uart_init(&uart2_data);
}
#endif

void board_early_init()
{
    dac_early_init(1, 0x3, 1000);
    devices_init();
}

static void board_power_init(void)
{
    power_init(&power_param);
}

void board_init()
{
    board_power_init();
#if TCFG_ADKEY_ENABLE
    adc_init();
    key_driver_init();
#endif
}

