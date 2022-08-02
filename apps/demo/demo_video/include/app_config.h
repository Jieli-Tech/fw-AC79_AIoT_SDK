#ifndef APP_CONFIG_H
#define APP_CONFIG_H



#define __FLASH_SIZE__    (4 * 1024 * 1024)
#define __SDRAM_SIZE__    (0 * 1024 * 1024)

#define CONFIG_DEBUG_ENABLE                     /* 打印开关 */


#define CONFIG_PCM_DEC_ENABLE
#define CONFIG_PCM_ENC_ENABLE
//#define CONFIG_OSD_ENABLE						/* 视频时间OSD */
#define CONFIG_VIDEO_REC_PPBUF_MODE

#define RTOS_STACK_CHECK_ENABLE //是否启用定时检查任务栈

//*********************************************************************************//
//                             编码图片分辨率                                      //
//*********************************************************************************//
//摄像头尺寸，此处需要和摄像头驱动可匹配
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
#define VIDEO_REC_AUDIO_SAMPLE_RATE		0//8000 //视频流的音频采样率,注意：硬件没MIC则为0
#define VIDEO_REC_FPS 					20   //视频帧率设置,0为默认

#define CONFIG_USR_VIDEO_ENABLE		//用户VIDEO使能
#ifdef CONFIG_USR_VIDEO_ENABLE
#define CONFIG_USR_PATH 	"192.168.1.1:8000" //用户路径，可随意设置，video_rt_usr.c的init函数看进行读取
#endif

//#define SDTAP_DEBUG


//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
#define TCFG_LOWPOWER_BTOSC_DISABLE			0
#define TCFG_LOWPOWER_LOWPOWER_SEL			0//SLEEP_EN
#define TCFG_LOWPOWER_VDDIOM_LEVEL			VDDIOM_VOL_32V
#define TCFG_LOWPOWER_VDDIOW_LEVEL			VDDIOW_VOL_21V
#define VDC14_VOL_SEL_LEVEL					VDC14_VOL_SEL_140V
#define SYSVDD_VOL_SEL_LEVEL				SYSVDD_VOL_SEL_126V


#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)




#endif

