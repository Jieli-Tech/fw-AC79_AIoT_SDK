#include "app_config.h"

#include "system/includes.h"
#include "device/includes.h"
#include "asm/includes.h"
#include "server/audio_dev.h"

// *INDENT-OFF*

UART1_PLATFORM_DATA_BEGIN(uart1_data)
    .baudrate = 115200,
    .port = PORTUSB_A,
    .tx_pin = IO_PORT_USB_DPA,
    .rx_pin = IO_PORT_USB_DMA,
    .max_continue_recv_cnt = 512,
    .idle_sys_clk_cnt = 500000,
    .clk_src = PLL_48M,
    .flags = UART_DEBUG,
UART1_PLATFORM_DATA_END();

UART2_PLATFORM_DATA_BEGIN(uart2_data)
	.baudrate = 1000000,
	.port = PORT_REMAP,
	.output_channel = OUTPUT_CHANNEL0,
	.tx_pin = IO_PORTC_03,
	.rx_pin = -1,
	.max_continue_recv_cnt = 1024,
	.idle_sys_clk_cnt = 500000,
	.clk_src = PLL_48M,
	.flags = UART_DEBUG,
UART2_PLATFORM_DATA_END();


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
    .pa_mute_port = 0xff,
    .pa_mute_value = 0,
    .differ_output = 0,
    .hw_channel = 0x03,
    .ch_num = 2,	//差分只需开一个通道
    .vcm_init_delay_ms = 1000,
};
static const struct adc_platform_data adc_data = {
    .mic_channel = LADC_CH_MIC0_P_N | LADC_CH_MIC1_P | LADC_CH_MIC3_P, //P代表硬件MIC直接正端，N代表硬件MIC只接负端，P_N代表硬件MIC采用差分接法
    .mic_ch_num = 3, //其中MIC0作为差分MIC回采功放信号
    .all_channel_open = 1,
    .isel = 2,
};
static const struct audio_pf_data audio_pf_d = {
    .adc_pf_data = &adc_data,
    .dac_pf_data = &dac_data,
};
static const struct audio_platform_data audio_data = {
    .private_data = (void *)&audio_pf_d,
};


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

#if defined CONFIG_BT_ENABLE || defined CONFIG_WIFI_ENABLE
#include "wifi/wifi_connect.h"
const struct wifi_calibration_param wifi_calibration_param = {
    .xosc_l     = 0xb,// 调节晶振左电容
    .xosc_r     = 0xa,// 调节晶振右电容
    .pa_trim_data  ={7, 7, 1, 1, 5, 0, 7},// 根据MP测试生成PA TRIM值
};
#endif

REGISTER_DEVICES(device_table) = {
	{"uart1", &uart_dev_ops, (void *)&uart1_data },
	{"uart2", &uart_dev_ops, (void *)&uart2_data },
	{"audio", &audio_dev_ops, (void *)&audio_data },
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
    adc_init();
#if TCFG_ADKEY_ENABLE
    key_driver_init();
#endif
    void cfg_file_parse(void);
    cfg_file_parse();
}

