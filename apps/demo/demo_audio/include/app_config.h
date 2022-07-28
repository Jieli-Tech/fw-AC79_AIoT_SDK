#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define __FLASH_SIZE__    (4 * 1024 * 1024)
#define __SDRAM_SIZE__    (2 * 1024 * 1024)

#ifdef CONFIG_NO_SDRAM_ENABLE
#undef  __SDRAM_SIZE__
#define __SDRAM_SIZE__    (0 * 1024 * 1024)
#endif

#define TCFG_ADKEY_ENABLE             1         //AD按键
// #define TCFG_IRKEY_ENABLE             1         //红外遥控
// #define TCFG_SD0_ENABLE               1
#define TCFG_SD1_ENABLE               1

#define CONFIG_DEBUG_ENABLE                     // 打印开关
// #define CONFIG_DEC_DIGITAL_VOLUME_ENABLE     //数字音量淡入淡出功能
// #define CONFIG_DEC_ANALOG_VOLUME_ENABLE      //模拟音量淡入淡出功能

#define CONFIG_LOCAL_MUSIC_MODE_ENABLE      //mode:本地播放模式使能
#define CONFIG_RECORDER_MODE_ENABLE         //mode:录音模式使能

#ifdef CONFIG_NET_ENABLE
#define CONFIG_NET_MUSIC_MODE_ENABLE        //mode:网络播放模式使能
// #define CONFIG_ASR_ALGORITHM_ENABLE         //mode:打断唤醒模式使能

#define CONFIG_WIFI_ENABLE                  //无线WIFI
#define INIT_SSID "GJ1"                     //WIFI名称
#define INIT_PWD  "8888888899"              //WIFI密码
#define CONFIG_SERVER_ASSIGN_PROFILE		//第三方平台的profile由杰理服务器分配
#define CONFIG_ASSIGN_MACADDR_ENABLE        //第一次开机连上外网后，使用杰理服务器分配WIFI模块的MAC地址, 关闭则使用<蓝牙地址更新工具*.exe>或者随机数作为MAC地址
#define CONFIG_DLNA_SDK_ENABLE              //打开DLNA音乐播放功能

#ifdef CONFIG_ASR_ALGORITHM_ENABLE
#define AISP_ALGORITHM 1 //思必驰双mic唤醒,未授权版本只支持运行10分钟
#define ROOBO_ALGORITHM 2 //ROOBO 单/双MIC唤醒 ROOBO_DUAL_MIC_ALGORITHM ,,,测试版本只支持运行10分钟
#define WANSON_ALGORITHM 3 //华镇算法,测试版只能够正确识别1000次
#define TEST_ALGORITHM 4
#define CONFIG_ASR_ALGORITHM  AISP_ALGORITHM //本地打断唤醒算法选择
#endif
#endif

//编解码器使能
#define CONFIG_LC3_ENC_ENABLE
#define CONFIG_LC3_DEC_ENABLE
#define CONFIG_PCM_DEC_ENABLE
#define CONFIG_PCM_ENC_ENABLE
#define CONFIG_DTS_DEC_ENABLE
#define CONFIG_ADPCM_DEC_ENABLE
#define CONFIG_MP3_DEC_ENABLE
#define CONFIG_MP3_ENC_ENABLE
#define CONFIG_WMA_DEC_ENABLE
#define CONFIG_M4A_DEC_ENABLE
#define CONFIG_WAV_DEC_ENABLE
#define CONFIG_AMR_DEC_ENABLE
#define CONFIG_APE_DEC_ENABLE
#define CONFIG_FLAC_DEC_ENABLE
#define CONFIG_SPEEX_DEC_ENABLE
#define CONFIG_ADPCM_ENC_ENABLE
#define CONFIG_WAV_ENC_ENABLE
#define CONFIG_VAD_ENC_ENABLE
#define CONFIG_VIRTUAL_DEV_ENC_ENABLE
#define CONFIG_OPUS_ENC_ENABLE
#define CONFIG_OPUS_DEC_ENABLE
#define CONFIG_SPEEX_ENC_ENABLE
#define CONFIG_AMR_ENC_ENABLE
#define CONFIG_AEC_ENC_ENABLE
#define CONFIG_DNS_ENC_ENABLE
#define CONFIG_AAC_ENC_ENABLE

#define CONFIG_REVERB_MODE_ENABLE            //打开混响功能
#define CONFIG_AUDIO_MIX_ENABLE              //打开叠音功能
#define CONFIG_AUDIO_PS_ENABLE               //打开变调变速功能


#ifndef TCFG_SD0_ENABLE
#define TCFG_SD0_ENABLE         0
#endif

#ifndef TCFG_SD1_ENABLE
#define TCFG_SD1_ENABLE         0
#endif

#if TCFG_SD0_ENABLE
#define CONFIG_STORAGE_PATH 	"storage/sd0"
#define SDX_DEV					"sd0"
#endif

#if TCFG_SD1_ENABLE
#define CONFIG_STORAGE_PATH 	"storage/sd1"
#define SDX_DEV					"sd1"
#endif

#ifndef CONFIG_STORAGE_PATH
#define CONFIG_STORAGE_PATH 	"storage/sdx"
#define SDX_DEV					"sdx"
#endif

#define FAT_CACHE_NUM   32

#define CONFIG_UDISK_STORAGE_PATH	"storage/udisk0"

#define CONFIG_ROOT_PATH     	    CONFIG_STORAGE_PATH"/C/"
#define CONFIG_UDISK_ROOT_PATH     	CONFIG_UDISK_STORAGE_PATH"/C/"

#define CONFIG_MUSIC_PATH_SD        CONFIG_ROOT_PATH
#define CONFIG_MUSIC_PATH_UDISK     CONFIG_UDISK_ROOT_PATH

#define CONFIG_MUSIC_PATH_FLASH             "mnt/sdfile/res/"
#define CONFIG_VOICE_PROMPT_FILE_PATH       "mnt/sdfile/res/audlogo/"
#define CONFIG_EQ_FILE_NAME                 "mnt/sdfile/res/cfg/eq_cfg_hw.bin"

#define RTOS_STACK_CHECK_ENABLE //是否启用定时检查任务栈
// #define MEM_LEAK_CHECK_ENABLE	//是否启用内存泄漏检查(需要包含mem_leak_test.h头文件)

//*********************************************************************************//
//                            AUDIO_ADC应用的通道配置                              //
//*********************************************************************************//
#define AUDIO_ENC_SAMPLE_SOURCE_MIC         0  //录音输入源：MIC
#define AUDIO_ENC_SAMPLE_SOURCE_PLNK0       1  //录音输入源：数字麦PLNK0
#define AUDIO_ENC_SAMPLE_SOURCE_PLNK1       2  //录音输入源：数字麦PLNK1
#define AUDIO_ENC_SAMPLE_SOURCE_IIS0        3  //录音输入源：IIS0
#define AUDIO_ENC_SAMPLE_SOURCE_IIS1        4  //录音输入源：IIS1
#define AUDIO_ENC_SAMPLE_SOURCE_LINEIN      5  //录音输入源：LINEIN

#define CONFIG_AUDIO_ENC_SAMPLE_SOURCE      AUDIO_ENC_SAMPLE_SOURCE_MIC    //录音输入源选择
#define CONFIG_AUDIO_DEC_PLAY_SOURCE        "dac"                          //播放输出源选择
#define CONFIG_AUDIO_RECORDER_SAMPLERATE    16000                          //录音采样率
#define CONFIG_AUDIO_RECORDER_CHANNEL       1                              //录音通道数
#define CONFIG_AUDIO_RECORDER_DURATION      (30 * 1000)                    //录音时长ms

#define CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE      //四路ADC硬件全开

#define CONFIG_AUDIO_ADC_CHANNEL_L          1        //左mic/aux通道
#define CONFIG_AUDIO_ADC_CHANNEL_R          3        //右mic/aux通道
#define CONFIG_REVERB_ADC_CHANNEL           1        //混响mic通道
#define CONFIG_UAC_MIC_ADC_CHANNEL          1        //UAC mic通道
#define CONFIG_AUDIO_ADC_GAIN               100      //mic/aux增益

#define CONFIG_AISP_MIC0_ADC_CHANNEL        1		//本地唤醒左mic通道
#define CONFIG_AISP_MIC1_ADC_CHANNEL        3		//本地唤醒右mic通道
#define CONFIG_AISP_LINEIN_ADC_CHANNEL      2		//本地唤醒LINEIN回采DAC通道
#define CONFIG_AISP_MIC_ADC_GAIN            80		//本地唤醒mic增益
#define CONFIG_AISP_LINEIN_ADC_GAIN         60		//本地唤醒LINEIN增益

//*********************************************************************************//
//                                  EQ配置                                         //
//*********************************************************************************//
//EQ配置，使用在线EQ时，EQ文件和EQ模式无效。有EQ文件时，默认不用EQ模式切换功能
#define TCFG_EQ_ENABLE                            1     //支持EQ功能
#define TCFG_EQ_ONLINE_ENABLE                     0     //支持在线EQ调试
#define TCFG_HW_SOFT_EQ_ENABLE                    1     //前3段使用软件运算
#define TCFG_LIMITER_ENABLE                       1     //限幅器
#define TCFG_EQ_FILE_ENABLE                       1     //从bin文件读取eq配置数据
#define TCFG_DRC_ENABLE                           TCFG_LIMITER_ENABLE
#define TCFG_ONLINE_ENABLE                        TCFG_EQ_ONLINE_ENABLE

//*********************************************************************************//
//                        SD 配置（暂只支持打开一个SD外设）                        //
//*********************************************************************************//
//SD0 	cmd,  clk,  data0, data1, data2, data3
//A     PB6   PB7   PB5    PB5    PB3    PB2
//B     PA7   PA8   PA9    PA10   PA5    PA6
//C     PH1   PH2   PH0    PH3    PH4    PH5
//D     PC9   PC10  PC8    PC7    PC6    PC5

//SD1 	cmd,  clk,  data0, data1, data2, data3
//A     PH6   PH7   PH5    PH4    PH3    PH2
//B     PC0   PC1   PC2    PC3    PC4    PC5

#if TCFG_SD0_ENABLE
#define TCFG_SD_PORTS                      'D'			//SD0/SD1的ABCD组
#define TCFG_SD_DAT_WIDTH                  1			//1:单线模式, 4:四线模式
#define TCFG_SD_DET_MODE                   SD_CLK_DECT	//检测模式
#define TCFG_SD_DET_IO                     IO_PORTA_01	//SD_DET_MODE为SD_IO_DECT时有效
#define TCFG_SD_DET_IO_LEVEL               0			//IO检卡上线的电平(0/1),SD_DET_MODE为SD_IO_DECT时有效
#define TCFG_SD_CLK                        30000000		//SD时钟
#endif

#if TCFG_SD1_ENABLE
#define TCFG_SD_PORTS                      'B'			//SD0/SD1的ABCD组
#define TCFG_SD_DAT_WIDTH                  1			//1:单线模式, 4:四线模式
#define TCFG_SD_DET_MODE                   SD_CLK_DECT	//检测模式
#define TCFG_SD_DET_IO                     IO_PORTA_01	//SD_DET_MODE为SD_IO_DECT时有效
#define TCFG_SD_DET_IO_LEVEL               0			//IO检卡上线的电平(0/1),SD_DET_MODE为SD_IO_DECT时有效
#define TCFG_SD_CLK                        30000000		//SD时钟
#endif

//*********************************************************************************//
//                                  USB配置                                        //
//*********************************************************************************//
#define TCFG_PC_ENABLE                      0           //使能USB从机模式
#define USB_PC_NO_APP_MODE                  2
#define USB_MALLOC_ENABLE                   1
#define USB_DEVICE_CLASS_CONFIG             (MASSSTORAGE_CLASS | AUDIO_CLASS)   //读卡器和UAC从机模式
#define TCFG_UDISK_ENABLE                   0           //U盘主机模式
#define TCFG_HOST_AUDIO_ENABLE              0           //UAC主机模式

#include "usb_std_class_def.h"
#include "usb_common_def.h"


//*********************************************************************************//
//                                    电源配置                                     //
//*********************************************************************************//
#define TCFG_LOWPOWER_BTOSC_DISABLE			0
#ifdef CONFIG_LOW_POWER_ENABLE
#define TCFG_LOWPOWER_LOWPOWER_SEL			RF_SLEEP_EN
#else
#define TCFG_LOWPOWER_LOWPOWER_SEL			0
#endif
#define TCFG_LOWPOWER_VDDIOM_LEVEL			VDDIOM_VOL_32V       //强VDDIO电压档位，不要高于外部DCDC的电压
#define TCFG_LOWPOWER_VDDIOW_LEVEL			VDDIOW_VOL_21V       //弱VDDIO电压档位
#define VDC14_VOL_SEL_LEVEL			        VDC14_VOL_SEL_140V   //RF1.4V电压档位
#define SYSVDD_VOL_SEL_LEVEL				SYSVDD_VOL_SEL_126V  //内核电压档位值

//#define SDTAP_DEBUG

#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)

#endif

