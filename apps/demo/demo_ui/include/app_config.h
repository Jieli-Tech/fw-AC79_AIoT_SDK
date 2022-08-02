#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*#define CONFIG_BOARD_7916A  //需要注意791系列摄像头驱动多了一个配置参数 .sync_config*/
#define CONFIG_JL_AC79_DevKitboard

#ifdef CONFIG_BOARD_7916A
#define __FLASH_SIZE__    (8 * 1024 * 1024)
#define __SDRAM_SIZE__    (8 * 1024 * 1024)
#endif

#ifdef CONFIG_JL_AC79_DevKitboard
#define __FLASH_SIZE__    (8 * 1024 * 1024)
#define __SDRAM_SIZE__    (8 * 1024 * 1024)
#endif

//*********************************************************************************//
//                                测试例子配置                                    //
//*********************************************************************************//
//注意:不同UI工程的ename.h文件不一样 使用不同UI工程时需要确认ename.h文件
#define UI_DEMO_1_0              //ui基本控件测试demo        //使用UI_DEMO_1 ui工程
//#define UI_DEMO_1_1              //帧数播放测试              //使用UI_DEMO_1 ui工程
//#define UI_DEMO_1_2              //摄像头数据显示帧数据测试  //使用UI_DEMO_1 ui工程
//#define UI_DEMO_1_3              //GIJ播放测试               //使用UI_DEMO_1 ui工程
//#define UI_DEMO_2_0              //主界面窗口测试            //使用UI_DEMO_2 中ui_128x128_double_1   ui工程
//#define UI_DEMO_2_1              //动态垂直列表测试          //使用UI_DEMO_2 中ui_128x128_double_1   ui工程
//#define UI_DEMO_2_2              //双UI工程测试              //使用UI_DEMO_2 中ui_128x128_double_1和ui_128x128_double_2   ui工程
//#define UI_DEMO_2_3              //双ui_text测试             //使用UI_DEMO_2 中ui_128x128_double_1   ui工程
//#define UI_DEMO_2_4              //显示不同字体大小中文测试  //使用UI_DEMO_2 中ui_128x128_double_1   ui工程


//*********************************************************************************//
//                                  SD卡配置                                       //
//*********************************************************************************//
#define TCFG_SD0_ENABLE                     1
#if (TCFG_SD0_ENABLE || TCFG_SD1_ENABLE)
#define TCFG_SD_PORTS                     'A'           //如果使用了Ui不能使用D口建议使用A口	//SD0/SD1的ABCD组
#define TCFG_SD_DAT_WIDTH                  1			//1:单线模式, 4:四线模式
#define TCFG_SD_DET_MODE                   SD_CMD_DECT	//CMD模式
#define TCFG_SD_DET_IO_LEVEL               0			//IO检卡上线的电平(0/1),SD_DET_MODE为SD_IO_DECT时有效
#define TCFG_SD_CLK                        20000000		//SD时钟
#endif

#if TCFG_SD0_ENABLE
#define CONFIG_STORAGE_PATH 	"storage/sd0"
#define SDX_DEV					"sd0"
#endif
#if TCFG_SD1_ENABLE
#define CONFIG_STORAGE_PATH 	"storage/sd1"
#define SDX_DEV					"sd1"
#endif

#define CONFIG_ROOT_PATH     	    CONFIG_STORAGE_PATH"/C/"
#define CONFIG_UI_PATH_FLASH        "mnt/sdfile/res/ui_res/"

//*********************************************************************************//
//                             摄像头相关配置                                      //
//*********************************************************************************//
//摄像头尺寸，此处需要和摄像头驱动可匹配
#define CONFIG_CAMERA_H_V_EXCHANGE         1
#define CONFIG_VIDEO_720P
#ifdef CONFIG_VIDEO_720P
#define CONFIG_VIDEO_IMAGE_W    1280
#define CONFIG_VIDEO_IMAGE_H    720
#else
#define CONFIG_VIDEO_IMAGE_W    640
#define CONFIG_VIDEO_IMAGE_H    480
#endif//CONFIG_VIDEO_720P

//#define SDTAP_DEBUG
#endif

//*********************************************************************************//
//                                  电源配置                                       //
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
#define TCFG_USE_SD_ADD_UI_FILE             0  //使用SD卡加载资源文件

#if CONFIG_BOARD_7916A
#define TCFG_LCD_ST7789S_ENABLE			    1
#define TCFG_LCD_ST7789V_ENABLE			    0
#define TCFG_LCD_ST7735S_ENABLE			    0
#define TCFG_LCD_480x272_8BITS			    0
#endif
#ifdef CONFIG_JL_AC79_DevKitboard
#define TCFG_TOUCH_GT911_ENABLE             1
#define TCFG_LCD_ILI9341_ENABLE	    	    1
#endif

#if TCFG_LCD_480x272_8BITS || TCFG_LCD_ST7789V_ENABLE
#define HORIZONTAL_SCREEN                   0//0为使用竖屏
#else
#define HORIZONTAL_SCREEN                   1//1为使能横屏配置
#endif

#if TCFG_LCD_ST7789S_ENABLE ||TCFG_LCD_ILI9341_ENABLE
#define USE_LCD_TE                          1
#endif

#endif//CONFIG_UI_ENABLE

//*********************************************************************************//
//                                  库打印配置                                         //
//*********************************************************************************//
#define CONFIG_DEBUG_ENABLE            /* 打印开关 */
#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)




