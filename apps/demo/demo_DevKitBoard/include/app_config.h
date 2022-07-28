#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "demo_config.h"

#define __FLASH_SIZE__    (8 * 1024 * 1024)
#define __SDRAM_SIZE__    (8 * 1024 * 1024)


//*********************************************************************************//
//                             功能模块使能配置                                    //
//*********************************************************************************//
#define CONFIG_LOW_POWER_ENABLE                   //软关机/睡眠开关
#define RTOS_STACK_CHECK_ENABLE                   //是否启用定时检查任务栈
#define CONFIG_AUDIO_MIX_ENABLE                   //打开叠音功能，蓝牙同步需要打开此功能
#define CONFIG_DOUBLE_BANK_ENABLE                 0//双备份方式升级
#define CONFIG_IPERF_ENABLE       			      // iperf测试
#define CONFIG_AIRKISS_NET_CFG                    //AIRKISS配网
//#define MEM_LEAK_CHECK_ENABLE	                  //是否启用内存泄漏检查(需要包含mem_leak_test.h头文件)
//#define FINSH_ENABLE                            //FINSH功能开启
//#define CONFIG_ASSIGN_MACADDR_ENABLE            //第一次开机连上外网后，使用杰理服务器分配WIFI模块的MAC地址, 关闭则使用<蓝牙地址更新工具*.exe>或者随机数作为MAC地址
//#define SDTAP_DEBUG
//#define CONFIG_STATIC_IPADDR_ENABLE            //记忆路由器分配的IP,下次直接使用记忆IP节省DHCP时间
//**********************************END********************************************//


//*********************************************************************************//
//                             外设模块使能配置                                    //
//*********************************************************************************//
#define CONFIG_UART_ENABLE                        /* 打印开关 	*/
#ifndef USE_EDR_DEMO
#define CONFIG_WIFI_ENABLE  			          /* 无线WIFI 	*/
#endif
#define CONFIG_SPI_ENABLE                         /* SPI开关    */
#define CONFIG_KEY_ENABLE                         /* 按键开关   */
#define CONFIG_PWM_ENABLE                         /* PWM开关   	*/
#define CONFIG_IIC_ENABLE                         /* IIC开关   	*/
#define CONFIG_OSC_RTC_ENABLE                     /* RTC时钟开关*/
//**********************************END********************************************//


//*********************************************************************************//
//                             串口模块配置                                        //
//*********************************************************************************//
#define CONFIG_UART0_ENABLE                	      0   //模块没有使用时IO不会被占用
#define CONFIG_UART2_ENABLE                	      0
#define CONFIG_DEBUG_ENABLE                	      1
#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG                                 0
#else
#define LIB_DEBUG                                 1
#endif
#define CONFIG_DEBUG_LIB(x)                       (x & LIB_DEBUG)
//#define SDTAP_DEBUG
//**********************************END********************************************//

//*********************************************************************************//
//                             路径配置配置                                        //
//*********************************************************************************//
#define SDX_DEV					   			      "sd0"
#define CONFIG_STORAGE_PATH 	   			      "storage/sd0"  //定义对应SD0的路径
#define CONFIG_UDISK_STORAGE_PATH  			      "storage/udisk0"
#define CONFIG_MUSIC_PATH_SD       			      "storage/sd0/C/"
#define CONFIG_ROOT_PATH           			      "storage/sd0/C/" //定义对应SD文件系统的根目录路径
#define CONFIG_UDISK_ROOT_PATH     			      "storage/udisk0/C/" //定义对应U盘文件系统的根目录路径
#define CONFIG_MUSIC_PATH_UDISK    			      "storage/udisk0/C/"
#define CONFIG_MUSIC_PATH_FLASH                   "mnt/sdfile/res/"
#define CONFIG_UI_PATH_FLASH                      "mnt/sdfile/res/ui_res/"
#define CONFIG_VOICE_PROMPT_FILE_PATH             "mnt/sdfile/res/audlogo/"
#define CONFIG_EQ_FILE_NAME                       "mnt/sdfile/res/cfg/eq_cfg_hw.bin"
#define CONFIG_UPGRADE_PATH                       "storage/sd0/C/update.ufw"
#define CONFIG_UDISK_UPGRADE_PATH                 "storage/udisk0/C/update.ufw"
#define CONFIG_REC_PATH_0                         "storage/sd0/C/DCIM/1/"
//**********************************END*******************************************//

//*********************************************************************************//
//                            按键配置                                             //
//*********************************************************************************//
#ifdef CONFIG_KEY_ENABLE
#define TCFG_IRKEY_ENABLE       				  1 //红外接收头
#define TCFG_RDEC_KEY_ENABLE    				  0 //旋转编码器
#define TCFG_IOKEY_ENABLE       				  0 //io按键
#define TCFG_TOUCH_KEY_ENABLE   				  0 //触摸按键
#define TCFG_CTMU_TOUCH_KEY_ENABLE  			  0 //触摸按键
#define TCFG_ADKEY_ENABLE           			  1 //AD按键
#endif
//**********************************END*******************************************//

//*********************************************************************************//
//                            音频模块配置                                         //
//*********************************************************************************//
#ifdef CONFIG_NET_ENABLE

#ifdef CONFIG_NET_MUSIC_MODE_ENABLE
#define CONFIG_DLNA_SDK_ENABLE                      //打开DLNA音乐播放功能
#endif

#ifdef CONFIG_ASR_ALGORITHM_ENABLE
#define AISP_ALGORITHM                            1 //思必驰双mic唤醒,未授权版本只支持运行10分钟
#define ROOBO_ALGORITHM                           2 //ROOBO 单/双MIC唤醒 ROOBO_DUAL_MIC_ALGORITHM ,,,测试版本只支持运行10分钟
#define WANSON_ALGORITHM                          3 //华镇算法,测试版只能够正确识别1000次
#define TEST_ALGORITHM                            4
#define CONFIG_ASR_ALGORITHM  AISP_ALGORITHM        //本地打断唤醒算法选择
#endif
#endif

//编解码器使能
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
#define CONFIG_AAC_DEC_ENABLE
#define CONFIG_SBC_DEC_ENABLE
#define CONFIG_SBC_ENC_ENABLE
#define CONFIG_MSBC_DEC_ENABLE
#define CONFIG_MSBC_ENC_ENABLE
#define CONFIG_CVSD_DEC_ENABLE
#define CONFIG_CVSD_ENC_ENABLE
#define CONFIG_NEW_M4A_DEC_ENABLE

#define AUDIO_ENC_SAMPLE_SOURCE_MIC               0     //录音输入源：MIC
#define AUDIO_ENC_SAMPLE_SOURCE_PLNK0             1     //录音输入源：数字麦PLNK0
#define AUDIO_ENC_SAMPLE_SOURCE_PLNK1             2     //录音输入源：数字麦PLNK1
#define AUDIO_ENC_SAMPLE_SOURCE_IIS0              3     //录音输入源：IIS0
#define AUDIO_ENC_SAMPLE_SOURCE_IIS1              4     //录音输入源：IIS1
#define AUDIO_ENC_SAMPLE_SOURCE_LINEIN            5     //录音输入源：LINEIN

#define CONFIG_AUDIO_ENC_SAMPLE_SOURCE            AUDIO_ENC_SAMPLE_SOURCE_MIC    //录音输入源选择
#define CONFIG_AUDIO_DEC_PLAY_SOURCE              "dac"                          //播放输出源选择
#define CONFIG_AUDIO_RECORDER_SAMPLERATE          16000                          //录音采样率
#define CONFIG_AUDIO_RECORDER_CHANNEL             1                              //录音通道数
#define CONFIG_AUDIO_RECORDER_DURATION            (30 * 1000)                    //录音时长ms

#define CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE              //四路ADC硬件全开
#define CONFIG_AUDIO_ADC_CHANNEL_L          	  1     //左mic通道
#define CONFIG_AUDIO_ADC_CHANNEL_R          	  3     //右mic通道
#define CONFIG_PHONE_CALL_ADC_CHANNEL       	  1     //通话mic通道
#define CONFIG_AEC_ADC_CHANNEL              	  0     //回采通道
#define CONFIG_AEC_AUDIO_ADC_GAIN           	  80    //回采通道增益

#define CONFIG_AUDIO_ADC_GAIN                     100   //mic/aux增益

#define CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN           //用差分mic代替aec回采
#define CONFIG_ASR_CLOUD_ADC_CHANNEL        	  1		//云端识别mic通道
#define CONFIG_VOICE_NET_CFG_ADC_CHANNEL    	  1		//声波配网mic通道
#define CONFIG_AISP_MIC0_ADC_CHANNEL        	  1		//本地唤醒左mic通道
#define CONFIG_AISP_MIC_ADC_GAIN            	  80    //本地唤醒mic增益
#define CONFIG_AISP_LINEIN_ADC_CHANNEL      	  3		//本地唤醒LINEIN回采DAC通道
#define CONFIG_AISP_MIC1_ADC_CHANNEL        	  0		//本地唤醒右mic通道
#define CONFIG_REVERB_ADC_CHANNEL           	  1		//混响mic通道
#define CONFIG_PHONE_CALL_ADC_CHANNEL       	  1		//通话mic通道
#define CONFIG_UAC_MIC_ADC_CHANNEL          	  1		//UAC mic通道
#define CONFIG_AISP_LINEIN_ADC_GAIN         	  10    //本地唤醒LINEIN增益

//**********************************END*******************************************//

//*********************************************************************************//
//                             SD模块配置                                          //
//*********************************************************************************//
#ifdef CONFIG_SD_ENABLE
#define TCFG_SD_PORTS                     	     'A'
#define TCFG_SD_DAT_WIDTH                 	      1		//开发板只接了一线 所有只能为一线模式
#define TCFG_SD_DET_MODE                  	      SD_CMD_DECT	//CMD模式
#define TCFG_SD_DET_IO_LEVEL              	      0		//IO检卡上线的电平(0/1),SD_DET_MODE为SD_IO_DECT时有效
#define TCFG_SD_CLK                       	      20000000		//SD时钟
#endif
//**********************************END********************************************//

//*********************************************************************************//
//                                  EQ配置                                         //
//*********************************************************************************//
//EQ配置，使用在线EQ时，EQ文件和EQ模式无效。有EQ文件时，默认不用EQ模式切换功能
#define TCFG_EQ_ENABLE                            1     //支持EQ功能
#define TCFG_EQ_ONLINE_ENABLE                     0     //支持在线EQ调试
#define TCFG_HW_SOFT_EQ_ENABLE                    1     //前3段使用软件运算
#define TCFG_LIMITER_ENABLE                       1     //限幅器
#define TCFG_EQ_FILE_ENABLE                       1     //从bin文件读取eq配置数据
#define TCFG_DRC_ENABLE                           0
#define TCFG_ONLINE_ENABLE                        TCFG_EQ_ONLINE_ENABLE
#define CONFIG_EQ_FILE_NAME                       "mnt/sdfile/res/cfg/eq_cfg_hw.bin"
//**********************************END*******************************************//


//*********************************************************************************//
//                                    电源配置                                     //
//*********************************************************************************//
#define TCFG_LOWPOWER_BTOSC_DISABLE			      0
#ifdef CONFIG_LOW_POWER_ENABLE
#define TCFG_LOWPOWER_LOWPOWER_SEL			      RF_SLEEP_EN
#else
#define TCFG_LOWPOWER_LOWPOWER_SEL			      0
#endif
#define TCFG_LOWPOWER_VDDIOM_LEVEL			      VDDIOM_VOL_32V       //强VDDIO电压档位，不要高于外部DCDC的电压
#define TCFG_LOWPOWER_VDDIOW_LEVEL			      VDDIOW_VOL_21V       //弱VDDIO电压档位
#define VDC14_VOL_SEL_LEVEL			              VDC14_VOL_SEL_140V   //RF1.4V电压档位
#define SYSVDD_VOL_SEL_LEVEL				      SYSVDD_VOL_SEL_126V  //内核电压档位值
//**********************************END*******************************************//


//*********************************************************************************//
//                                  BT_BLE配置                                     //
//*********************************************************************************//
#define BT_EMITTER_EN                             1
#define BT_RECEIVER_EN                            2

#define CONFIG_BT_RX_BUFF_SIZE                    (12 * 1024)
#define CONFIG_BT_TX_BUFF_SIZE                    (6 * 1024)

#define CONFIG_POWER_ON_ENABLE_BT                 1     //开机自动打开经典蓝牙
#define TCFG_USER_BT_CLASSIC_ENABLE               1     //经典蓝牙功能
#define TCFG_USER_BLE_ENABLE                      0     //BLE功能使能
#define TCFG_USER_EMITTER_ENABLE                  0     //蓝牙发射功能
#ifdef CONFIG_LOW_POWER_ENABLE
#define TCFG_BT_SNIFF_ENABLE                      1
#else
#define TCFG_BT_SNIFF_ENABLE                      0
#endif
#define BT_SUPPORT_MUSIC_VOL_SYNC                 0     //音量同步
#define BT_SUPPORT_DISPLAY_BAT                    1     //电池电量同步显示功能
#define SPP_TRANS_DATA_EN                         1     //SPP数传demo
#define TCFG_BT_SUPPORT_AAC                       1     //蓝牙AAC格式支持
//**********************************END*******************************************//


//*********************************************************************************//
//                                  USB配置                                         //
//*********************************************************************************//
#ifdef CONFIG_USB_ENABLE
#define TCFG_PC_ENABLE                      	  0
#define USB_PC_NO_APP_MODE                  	  0
#define USB_MALLOC_ENABLE                   	  0
#define USB_DEVICE_CLASS_CONFIG             	  (MASSSTORAGE_CLASS)
#define TCFG_HOST_AUDIO_ENABLE              	  0
#define TCFG_HOST_UVC_ENABLE                	  0   //打开USB 后拉摄像头功能，需要使能住机模式
#define TCFG_UDISK_ENABLE                   	  0   //U盘功能
#define TCFG_USB_SLAVE_ENABLE                     0
#define TCFG_USB_HOST_ENABLE                      0
#include "usb_std_class_def.h"
#include "usb_common_def.h"
#endif
//**********************************END*******************************************//

//*********************************************************************************//
//                             摄像头相关配置                                      //
//*********************************************************************************//
#ifdef CONFIG_VIDEO_ENABLE
//摄像头尺寸，此处需要和摄像头驱动可匹配
#define CONFIG_CAMERA_H_V_EXCHANGE                1

#define CONFIG_VIDEO_720P
#ifdef CONFIG_VIDEO_720P
#define CONFIG_VIDEO_IMAGE_W    		          1280
#define CONFIG_VIDEO_IMAGE_H    		          720
#else
#define CONFIG_VIDEO_IMAGE_W    		          640
#define CONFIG_VIDEO_IMAGE_H    		          480
#endif//CONFIG_VIDEO_720P
#endif
//**********************************END*******************************************//

//*********************************************************************************//
//                                  UI配置                                         //
//*********************************************************************************//
#ifdef CONFIG_UI_ENABLE
#define CONFIG_VIDEO_DEC_ENABLE             	  1  //打开视频解码器
#define TCFG_USE_SD_ADD_UI_FILE             	  0  //使用SD卡加载资源文件

#define USE_LCD_240X320_ILI9341                   0
#define USE_LCD_320X480_ILI9481                   1

#if USE_LCD_240X320_ILI9341
#define TCFG_TOUCH_GT911_ENABLE             	  1
#define TCFG_LCD_ILI9341_ENABLE	    	    	  1
#endif //USE_LCD_240X320_ILI9341

#if USE_LCD_320X480_ILI9481
#define TCFG_TOUCH_FT6236_ENABLE                  1
#define TCFG_LCD_ILI9481_ENABLE	    	    	  1
#endif //USE_LCD_320X480_ILI9481


#ifdef USE_CAMERA_DVP_SHOW_TO_LCD_DEMO
#define HORIZONTAL_SCREEN                   	  1//0为使用竖屏  //1为使能横屏配置
#else
#define HORIZONTAL_SCREEN                   	  0//0为使用竖屏  //1为使能横屏配置
#endif

#define USE_LCD_TE                          	  1
#endif//CONFIG_UI_ENABLE
//**********************************END*******************************************//

#endif

