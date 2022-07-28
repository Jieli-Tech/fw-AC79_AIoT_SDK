#include "app_config.h"
#include "init.h"
#include "os/os_api.h"
#include "device/uart.h"

#if !defined CONFIG_AUDIO_MIX_ENABLE
u8 audio_mix_en = 0;
#endif

#if !defined CONFIG_AUDIO_PS_ENABLE
void *get_ps_cal_api(void)
{
    return NULL;
}
#endif

#ifdef CONFIG_MP3_DEC_ENABLE
const int MP3_OUTPUT_LEN = 4;
const int MP3_TGF_TWS_EN = 0;
const int MP3_TGF_AB_EN  = 0;
const int MP3_TGF_FASTMO = 0;
const int MP3_SEARCH_MAX = 13;
const int MP3_TGF_POSPLAY_EN = 1;
#endif

#ifdef CONFIG_OPUS_DEC_ENABLE
const int silk_fsN_enable = 1;  //支持8-12k采样率
const int silk_fsW_enable = 1;  //支持16-24k采样率
#endif

#ifdef CONFIG_DNS_ENC_ENABLE
const u8 dns_enc_enable = 1;
#else
const u8 dns_enc_enable = 0;
#endif

#if (defined CONFIG_LC3_ENC_ENABLE) || (defined CONFIG_LC3_DEC_ENABLE)

const int  LC3_INT24bit_INOUT = 0;
//如果是1，则认为编码input的时候，读到的数据认为是int的类型，存放着24bit的数据。 解码output的时候也是int类型，存放24bit数据。
//如果为0，则 认为是short的。

const  	int  LC3_DMS_VAL = 100;        //配置: 25ms 50ms 100ms 帧
const  	int  LC3_DMS_FSINDEX = 5;    //采样率配置：<=8000:0, <=16000:1  ,<=24000:2,  <=32000:3,  <=48000:4,  可配采样率：5
const  	int  LC3_QUALTIY_CONFIG = 1;

const 	int  LC3_PLC_EN = 0;   //置1做plc，置0的效果类似补静音包
const  	int  LC3_HW_FFT = 0; //br28置1，br30置0

#endif

//audio part
const char log_tag_const_w_pcm AT(.LOG_TAG_CONST) = 0;

const char log_tag_const_w_audio AT(.LOG_TAG_CONST) = 0;


#ifdef CONFIG_CPU_WL82

static const unsigned short combined_vol_list[21][2] = {
    {	0,		0   	},		// 0:None
    {	2,		14530	},		// 1:-38.35db
    {	3,		15940	},		// 2:-36.35db
    {	5,		15100	},		// 3:-34.35db
    {	7,		14270	},		// 4:-32.35db
    {	8,		15480	},		// 5:-30.35db
    {	10,		14450	},		// 6:-28.35db
    {	11,		15700	},		// 7:-26.35db
    {	13,		14650	},		// 8:-24.35db
    {	14,		15870	},		// 9:-22.35db
    {	16,		15100	},		// 10:-20.35db
    {	17,		16320	},		// 11:-18.35db
    {	19,		15090	},		// 12:-16.35db
    {	20,		16230	},		// 13:-14.35db
    {	22,		14870	},		// 14:-12.35db
    {	23,		15970	},		// 15:-10.35db
    {	25,		14610	},		// 16:-8.35db
    {	26,		15640	},		// 17:-6.35db
    {	28,		14280	},		// 18:-4.35db
    {	29,		15300	},		// 19:-2.35db
    {	30,		16384	},		// 20:-0.35db
};

#else

static const unsigned short combined_vol_list[21][2] = {
    {	0,		0   	},		// 0:None
    {	2,		14630	},		// 1:-40.5db
    {	3,		16020	},		// 2:-38.5db
    {	5,		15130	},		// 3:-36.5db
    {	7,		14280	},		// 4:-34.5db
    {	8,		15500	},		// 5:-32.5db
    {	10,		14430	},		// 6:-30.5db
    {	11,		15650	},		// 7:-28.5db
    {	13,		14660	},		// 8:-26.5db
    {	14,		15950	},		// 9:-24.5db
    {	16,		14680	},		// 10:-22.5db
    {	17,		15850	},		// 11:-20.5db
    {	19,		14650	},		// 12:-18.5db
    {	20,		15750	},		// 13:-16.5db
    {	22,		14450	},		// 14:-14.5db
    {	23,		15480	},		// 15:-12.5db
    {	25,		14150	},		// 16:-10.5db
    {	26,		15180	},		// 17:-8.5db
    {	27,		16260	},		// 18:-6.5db
    {	29,		14850	},		// 19:-4.5db
    {	30,		15920	},		// 20:-2.5db
};

#endif

const unsigned short **get_user_vol_tab(unsigned char *vol_tab_step)
{
    *vol_tab_step = sizeof(combined_vol_list) / sizeof(combined_vol_list[0]);
    return (const unsigned short **)&combined_vol_list;
}

#if CONFIG_VOLUME_TAB_TEST_ENABLE

__attribute__((weak)) const char *mp_test_get_uart(void)
{
    return "uart1";
}

static void volume_test_task(void *p)
{
    int parm;
    u8 uart_circlebuf[1 * 1024 + 32];
    u8 *temp = ((u32)uart_circlebuf % 32) ? (u8 *)((32 - (u32)uart_circlebuf % 32) + (u32)uart_circlebuf) : uart_circlebuf;
    char uart_data[64];
    int len = 0;
    void *uart_dev_handle = NULL;
    int analog_vol, digital_vol;

    if (NULL == mp_test_get_uart()) {
        return;
    }

    os_time_dly(100);

    uart_dev_handle = dev_open(mp_test_get_uart(), 0);
    if (!uart_dev_handle) {
        return;
    }

    dev_ioctl(uart_dev_handle, UART_SET_CIRCULAR_BUFF_ADDR, (u32)temp);

    parm = sizeof(uart_circlebuf) - 32;

    dev_ioctl(uart_dev_handle, UART_SET_CIRCULAR_BUFF_LENTH, (u32)parm);

    parm = 1;
    dev_ioctl(uart_dev_handle, UART_SET_RECV_BLOCK, (u32)parm);

    parm = 1000;
    dev_ioctl(uart_dev_handle, UART_SET_RECV_TIMEOUT, (u32)parm);

    dev_ioctl(uart_dev_handle, UART_START, (u32)0);

    while (1) {
        memset(uart_data, 0, sizeof(uart_data));

        len = dev_read(uart_dev_handle, uart_data, sizeof(uart_data));
        if (len > 0) {
            sscanf(uart_data, "d:%d,a:%d", &digital_vol, &analog_vol);
            printf("set digital_vol : %d, analog_vol : %d\n", digital_vol, analog_vol);
            CPU_CRITICAL_ENTER();
            SFR(JL_ANA->DAA_CON1, 15,  5, analog_vol);             // R_RG_SEL_12v[4:0]
            SFR(JL_ANA->DAA_CON1, 10,  5, analog_vol);             // R_LG_SEL_12v[4:0]
            SFR(JL_ANA->DAA_CON1,  5,  5, analog_vol);             // F_RG_SEL_12v[4:0]
            SFR(JL_ANA->DAA_CON1,  0,  5, analog_vol);             // F_LG_SEL_12v[4:0]
            SFR(JL_AUDIO->DAC_VL0, 16, 16, digital_vol);       //dac vol left
            SFR(JL_AUDIO->DAC_VL0,  0, 16, digital_vol);       //dac vol right
            SFR(JL_AUDIO->DAC_VL1, 16, 16, digital_vol);       //dac vol left
            SFR(JL_AUDIO->DAC_VL1,  0, 16, digital_vol);       //dac vol right
            CPU_CRITICAL_EXIT();
        }
    }

    dev_close(uart_dev_handle);
}

static int volume_test_task_create(void)
{
    return thread_fork("volume_test_task", 12, 1024, 0, 0, volume_test_task, NULL);
}
late_initcall(volume_test_task_create);

#endif

