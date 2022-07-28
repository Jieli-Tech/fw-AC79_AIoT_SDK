#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include "board_config.h"

#ifdef CONFIG_NO_SDRAM_ENABLE					/* 封装不带sdram就打开该宏 */
#undef __SDRAM_SIZE__
#define __SDRAM_SIZE__    (0 * 1024 * 1024)
#endif

#define CONFIG_DATABASE_2_FLASH                 /* 系统配置存flash */
#define CONFIG_DEBUG_ENABLE                     /* 打印开关 */
//#define CONFIG_LOW_POWER_ENABLE            		//软关机/睡眠开关
//#define CONFIG_OSC_RTC_ENABLE					/* 外接RTC32768使能*/

#define CONFIG_PCM_DEC_ENABLE
#define CONFIG_PCM_ENC_ENABLE
// #define CONFIG_AEC_ENC_ENABLE

#define CONFIG_NET_TUTK_ENABLE

#ifdef CONFIG_NET_ENABLE
#define CONFIG_CTP_ENABLE
#define CONFIG_WIFI_ENABLE  					/* 无线WIFI */
// #define CONFIG_IPERF_ENABLE  				// iperf测试
// #define CONFIG_MASS_PRODUCTION_ENABLE //启用产测模式
#endif

// #define CONFIG_OSD_ENABLE
#define CONFIG_VIDEO_REC_PPBUF_MODE
/*#define CONFIG_VIDEO_SPEC_DOUBLE_REC_MODE*/

// #define CONFIG_QR_CODE_NET_CFG					//二维码配网

#ifdef CONFIG_MASS_PRODUCTION_ENABLE
#define ROUTER_SSID     "test"    //量产模式的路由器名称
#define ROUTER_PWD      "12345678"  //量产模式的路由器密码
//#define CONFIG_PRODUCTION_IO_PORT			IO_PORTB_01 //配置进入量产莫模式的IO
//#define CONFIG_PRODUCTION_IO_STATE		0 			//配置进入量产莫模式的IO状态：0低电平，1高电平
#endif

#ifndef TCFG_SD0_ENABLE
#define TCFG_SD0_ENABLE		0
#endif
#ifndef TCFG_SD1_ENABLE
#define TCFG_SD1_ENABLE		0
#endif

#define RTOS_STACK_CHECK_ENABLE //是否启用定时检查任务栈
//#define MEM_LEAK_CHECK_ENABLE	//是否启用内存泄漏检查(需要包含mem_leak_test.h头文件)


/*升级版本号*/
/*
版本    说明
主版本  用于重大升级，例如整体架构，接口都发生重大改变，导致版本不兼容，此时应该升级主版本
次版本  用于功能增加及升级，整体架构不变，接口完全兼容。次版本必须往下兼容，即1.5.x 必须兼容1.4.x、1.3.x、1.2.x、1.1.x、1.0.x
修订版本    用于内部发布及BUG修复
*/

#define OTA_MAJOR   1
#define OTA_MINOR   3
#define OTA_PATCH   0

#define CONFIG_REC_DIR_0       "DCIM/1/"
#define CONFIG_REC_DIR_1       "DCIM/2/"
#ifndef CONFIG_VIDEO1_ENABLE
#define CONFIG_REC_DIR_2       "DCIM/2/"
#else
#define CONFIG_REC_DIR_2       "DCIM/3/"
#endif

#define CONFIG_ROOT_PATH     	CONFIG_STORAGE_PATH"/C/"
#define CONFIG_REC_PATH_0       CONFIG_STORAGE_PATH"/C/"CONFIG_REC_DIR_0
#define CONFIG_REC_PATH_1       CONFIG_STORAGE_PATH"/C/"CONFIG_REC_DIR_1
#define CONFIG_REC_PATH_2       CONFIG_STORAGE_PATH"/C/"CONFIG_REC_DIR_2

#define CONFIG_UDISK_STORAGE_PATH	"storage/usb_disk"
#define CONFIG_UDISK_ROOT_PATH     	CONFIG_UDISK_STORAGE_PATH"/C/"

#ifdef CONFIG_EMR_DIR_ENABLE
#define CONFIG_EMR_REC_DIR         "EMR/"
#define CONFIG_EMR_REC_DIR_0       "DCIM/1/"CONFIG_EMR_REC_DIR
#define CONFIG_EMR_REC_DIR_1       "DCIM/2/"CONFIG_EMR_REC_DIR
#ifndef CONFIG_VIDEO1_ENABLE
#define CONFIG_EMR_REC_DIR_2       "DCIM/2/"CONFIG_EMR_REC_DIR
#else
#define CONFIG_EMR_REC_DIR_2       "DCIM/3/"CONFIG_EMR_REC_DIR
#endif
#define CONFIG_EMR_REC_PATH_0   CONFIG_STORAGE_PATH"/C/"CONFIG_EMR_REC_DIR_0
#define CONFIG_EMR_REC_PATH_1   CONFIG_STORAGE_PATH"/C/"CONFIG_EMR_REC_DIR_1
#define CONFIG_EMR_REC_PATH_2   CONFIG_STORAGE_PATH"/C/"CONFIG_EMR_REC_DIR_2
#endif


#define CAMERA0_CAP_PATH        CONFIG_REC_PATH_0
#define CAMERA1_CAP_PATH        CONFIG_REC_PATH_1
#define CAMERA2_CAP_PATH        CONFIG_REC_PATH_2

#define CONFIG_DEC_ROOT_PATH 	CONFIG_ROOT_PATH"DCIM/"
#define CONFIG_DEC_PATH_1 	    CONFIG_REC_PATH_0
#define CONFIG_DEC_PATH_2 	    CONFIG_REC_PATH_1
#define CONFIG_DEC_PATH_3 	    CONFIG_REC_PATH_2

#define MAX_FILE_NAME_LEN       64
#define FILE_SHOW_NUM           12  /* 一页显示文件数 */

#if (defined CONFIG_VIDEO1_ENABLE) && (defined CONFIG_VIDEO2_ENABLE)
#define THREE_WAY_ENABLE		1
#define CONFIG_VIDEO_REC_NUM    3
#else
#define THREE_WAY_ENABLE		0
#define CONFIG_VIDEO_REC_NUM    3
#endif

#define VM_MAGIC_VERSION 0X129
#define VM_STORAGE_MODE  0 //2:双64K,1:64K+4K,0:双4K
#define FLASH_END_RESERVE_SPACE  ((VM_STORAGE_MODE==0?8*1024:(VM_STORAGE_MODE==1?68*1024:128*1024))+4*1024) //FLASH末尾预留空间不纳入文件系统管理,VM区域加上其他用到区域例如4K profile, 4k mac

//*********************************************************************************//
//                            AUDIO_ADC应用的通道配置                              //
//*********************************************************************************//
//#define CONFIG_ASR_ALGORITHM ROOBO_ALGORITHM
#ifdef CONFIG_ASR_ALGORITHM
#define CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE          //四路ADC硬件全开
#endif

#ifdef CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE
#define CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN       //用差分mic代替aec回采
#define CONFIG_ASR_CLOUD_ADC_CHANNEL        1		//云端识别mic通道
#define CONFIG_VOICE_NET_CFG_ADC_CHANNEL    1		//声波配网mic通道
#define CONFIG_AISP_MIC0_ADC_CHANNEL        1		//本地唤醒左mic通道
#define CONFIG_REVERB_ADC_CHANNEL           1		//混响mic通道
#define CONFIG_PHONE_CALL_ADC_CHANNEL       1		//通话mic通道
#define CONFIG_AISP_MIC_ADC_GAIN            80		//本地唤醒mic增益
#ifdef CONFIG_AISP_DIFFER_MIC_REPLACE_LINEIN
#define CONFIG_AISP_LINEIN_ADC_CHANNEL      2		//本地唤醒LINEIN回采DAC通道
#define CONFIG_AISP_MIC1_ADC_CHANNEL        0		//本地唤醒右mic通道
#define CONFIG_AISP_LINEIN_ADC_GAIN         40		//本地唤醒LINEIN增益
#endif
#endif


/*------------网络配置----------*/
#define ACCESS_NUM 				1
//#define CONFIG_ENABLE_VLIST
#define WIFI_CAM_PREFIX    "GJ12"
#define WIFI_CAM_WIFI_PWD  "123456789"

//默认网络视频为JPEG格式
#define CONFIG_NET_JPEG

#ifndef CONFIG_USR_VIDEO_ENABLE
//#define CONFIG_NET_TCP_ENABLE
// #define CONFIG_NET_UDP_ENABLE
#endif

/*-------------网络端口----------------*/
#define CTP_CTRL_PORT   	3333
#define CDP_CTRL_PORT   	2228
#define NET_SCR_PORT    	2230
#define VIDEO_PREVIEW_PORT 	2226
#define VIDEO_PLAYBACK_PORT 2223
#define HTTP_PORT           8080
#define RTSP_PORT           554

/*--------视频库内使用----------------*/
#define _DUMP_PORT          2229
#define _FORWARD_PORT    	2224
#define _BEHIND_PORT     	2225


/*--------录像和实时流帧率配置--------*/
//实时流BUFF允许缓存帧数(延时大则需要打开，例如录像中720P),写0无效,注意：该宏大于1时，会引起实时流实际帧率比配置低，但是实时性好
#define  NET_VIDEO_BUFF_FRAME_CNT		2

//实时流前后视帧率设置
#define  NET_VIDEO_REC_FPS0   			20  //不使用0作为默认值，写具体数值
#define  NET_VIDEO_REC_FPS1   			20   //不使用0作为默认值，写具体数值

//录像模式实时流丢帧配置(单路720录像且实时流720或双路录像有效),只能配置以下3个宏数值>=1其中一个,配置必须小于等于摄像头输出帧率,全0则输出录像帧率
#define  NET_VIDEO_REC_DROP_REAl_FP		15	//(实际输出帧率，单路720录像且实时流720或双路录像有效,写0无效,不录像请用NET_VIDEO_REC_FPS0,NET_VIDEO_REC_FPS1)
#define  NET_VIDEO_REC_LOSE_FRAME_CNT	0	//(每隔n帧丢一帧,单路720录像且实时流720或双路录像有效,写0无效)30p摄像头,需求>=15p/s则配置>=1;25p摄像头,需求>=12p/s则配置>=1.
#define  NET_VIDEO_REC_SEND_FRAME_CNT	0	//(每隔n帧发一帧,单路720录像且实时流720或双路录像有效,写0无效)

//RTSP实时流帧率设置
#define  STRM_VIDEO_REC_FPS0   			5  //不使用0作为默认值，写具体数值
#define  STRM_VIDEO_REC_DROP_REAl_FP 	5  //(实际输出帧率，单路720录像且实时流720或双路录像有效,写0无效)

//RTSP实时流BUFF允许缓存帧数(延时大则需要打开，例如录像中720P),写0无效,注意：该宏大于1时，会引起实时流实际帧率比配置低，但是实时性好
#define  STRM_VIDEO_BUFF_FRAME_CNT		2
/*--------------------------------------*/


//*********************************************************************************//
//                             编码图片分辨率                                      //
//*********************************************************************************//
//#define CONFIG_VIDEO_720P
#ifdef CONFIG_VIDEO_720P
#define CONFIG_VIDEO_IMAGE_W    1280
#define CONFIG_VIDEO_IMAGE_H    720
#else
#define CONFIG_VIDEO_IMAGE_W    640
#define CONFIG_VIDEO_IMAGE_H    480
#endif

//*********************************************************************************//
//                             视频流相关配置                                      //
//*********************************************************************************//
#define VIDEO_REC_AUDIO_SAMPLE_RATE		8000 //视频流的音频采样率,注意：硬件没MIC则为0
#define VIDEO_REC_FPS 					20   //录像SD卡视频帧率设置,0为默认

//#define CONFIG_USR_VIDEO_ENABLE		//用户VIDEO使能
#ifdef CONFIG_USR_VIDEO_ENABLE
#define CONFIG_USR_PATH 	"192.168.1.1:8000" //用户路径，可随意设置，video_rt_usr.c的init函数看进行读取
#endif

//*********************************************************************************//
//                        升级：单双备份和名称路径配置                             //
//*********************************************************************************//
#define CONFIG_DOUBLE_BANK_ENABLE			1//双备份方式升级
#define CONFIG_UPGRADE_FILE_NAME			"update.ufw"
#define CONFIG_UPGRADE_PATH       	        CONFIG_ROOT_PATH\
											CONFIG_UPGRADE_FILE_NAME	//备份方式升级
#define CONFIG_NO_BACKUP_UPGRADE_PATH     	CONFIG_ROOT_PATH\
											CONFIG_UPGRADE_FILE_NAME	//不备份的网络固件升级专用，建议采用备份方式升级


//*********************************************************************************//
//                                  USB配置                                        //
//*********************************************************************************//
#ifdef CONFIG_UVC_VIDEO2_ENABLE
#define TCFG_HOST_UVC_ENABLE               	1   //打开USB主机UVC
#define TCFG_USB_SLAVE_ENABLE               0   //打开usb从机功能
#else
#define TCFG_USB_SLAVE_ENABLE               0   //打开usb从机功能
#endif


#if TCFG_USB_SLAVE_ENABLE
#define TCFG_PC_ENABLE 						1	//打开连接电脑模式(从机模式)
#endif

#define TCFG_UDISK_ENABLE     				0	//打开读写U盘功能

#include "usb_std_class_def.h"
#include "usb_common_def.h"

//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
#define TCFG_LOWPOWER_BTOSC_DISABLE			0
#define TCFG_LOWPOWER_LOWPOWER_SEL			0//(RF_SLEEP_EN|RF_FORCE_SYS_SLEEP_EN|SYS_SLEEP_EN) //该宏在睡眠低功耗才用到，此处设置为0
#define TCFG_LOWPOWER_VDDIOM_LEVEL			VDDIOM_VOL_32V //正常工作的内部vddio电压值，一般使用外部3.3V，内部设置需比外部3.3V小
#define TCFG_LOWPOWER_VDDIOW_LEVEL			VDDIOW_VOL_21V //软关机或睡眠的内部vddio最低电压值
#define VDC14_VOL_SEL_LEVEL					VDC14_VOL_SEL_140V //内部的1.4V默认1.4V
#define SYSVDD_VOL_SEL_LEVEL				SYSVDD_VOL_SEL_126V //系统内核电压，默认1.26V


//*********************************************************************************//
//                                  UI配置                                         //
//*********************************************************************************//
#ifdef CONFIG_UI_ENABLE

#define CONFIG_VIDEO_DEC_ENABLE             1  //打开视频解码器
#define TCFG_LCD_ENABLE                     1  //使用lcd屏幕
#define TCFG_USE_SD_ADD_UI_FILE             0  //使用SD卡加载资源文件

#if TCFG_LCD_ENABLE
#define TCFG_LCD_ST7789S_ENABLE			    1
#define TCFG_LCD_ST7789V_ENABLE			    0
#define TCFG_LCD_ST7735S_ENABLE			    0
#define TCFG_LCD_480x272_8BITS			    0
#endif

#if TCFG_LCD_480x272_8BITS || TCFG_LCD_ST7789V_ENABLE
#define HORIZONTAL_SCREEN                   0//0为使用竖屏
#else
#define HORIZONTAL_SCREEN                   1//1为使能横屏配置
#endif

#if TCFG_LCD_ST7789S_ENABLE
#define USE_LCD_TE                          1
#endif

#endif//CONFIG_UI_ENABLE


#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)

#include "video_buf_config.h"

#ifndef TCFG_ADKEY_ENABLE
#define TCFG_ADKEY_ENABLE             0         //AD按键
#endif

#ifndef TCFG_IOKEY_ENABLE
#define TCFG_IOKEY_ENABLE             0         //IO按键
#define TCFG_IO_MULTIPLEX_WITH_SD     0
#endif

#ifndef TCFG_IRKEY_ENABLE
#define TCFG_IRKEY_ENABLE             0         //红外遥控按键
#endif

#ifndef TCFG_RDEC_KEY_ENABLE
#define TCFG_RDEC_KEY_ENABLE          0         //旋转编码器
#endif

#endif

