#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define __FLASH_SIZE__    (4 * 1024 * 1024)

#ifdef CONFIG_NO_SDRAM_ENABLE
#define __SDRAM_SIZE__    (0 * 1024 * 1024)
#else
#define __SDRAM_SIZE__    (2 * 1024 * 1024)
#endif

#define TCFG_ADKEY_ENABLE             1      //AD按键

#define CONFIG_DEBUG_ENABLE                  //打印开关
#define CONFIG_LOW_POWER_ENABLE              //软关机/睡眠开关
#define CONFIG_AUDIO_MIX_ENABLE              //打开叠音功能，蓝牙同步需要打开此功能

#define RTOS_STACK_CHECK_ENABLE //是否启用定时检查任务栈
// #define MEM_LEAK_CHECK_ENABLE	//是否启用内存泄漏检查(需要包含mem_leak_test.h头文件)

#define CONFIG_PCM_ENC_ENABLE
#define CONFIG_AEC_ENC_ENABLE
#define CONFIG_DNS_ENC_ENABLE
#define CONFIG_AAC_DEC_ENABLE
#define CONFIG_SBC_DEC_ENABLE
#define CONFIG_SBC_ENC_ENABLE
#define CONFIG_MSBC_DEC_ENABLE
#define CONFIG_MSBC_ENC_ENABLE
#define CONFIG_CVSD_DEC_ENABLE
#define CONFIG_CVSD_ENC_ENABLE
#define CONFIG_VIRTUAL_DEV_ENC_ENABLE
#define CONFIG_NEW_M4A_DEC_ENABLE

#define CONFIG_ALL_ADC_CHANNEL_OPEN_ENABLE          //四路ADC硬件全开
#define CONFIG_AUDIO_ADC_CHANNEL_L          1       //左mic通道
#define CONFIG_AUDIO_ADC_CHANNEL_R          3       //右mic通道
#define CONFIG_PHONE_CALL_ADC_CHANNEL       1       //通话mic通道
#define CONFIG_AEC_ADC_CHANNEL              0       //回采通道
#define CONFIG_AEC_AUDIO_ADC_GAIN           80      //回采通道增益


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


//*********************************************************************************//
//                                  BT_BLE配置                                     //
//*********************************************************************************//
#define BT_EMITTER_EN     1
#define BT_RECEIVER_EN    2

#define CONFIG_BT_RX_BUFF_SIZE  (12 * 1024)
#define CONFIG_BT_TX_BUFF_SIZE  (6 * 1024)

#define CONFIG_POWER_ON_ENABLE_BT                 1     //开机自动打开经典蓝牙
#define TCFG_USER_BT_CLASSIC_ENABLE               1     //经典蓝牙功能
#define TCFG_USER_BLE_ENABLE                      0     //BLE功能使能
#define TCFG_USER_EMITTER_ENABLE                  1     //蓝牙发射功能
#ifdef CONFIG_LOW_POWER_ENABLE
#define TCFG_BT_SNIFF_ENABLE                      1
#else
#define TCFG_BT_SNIFF_ENABLE                      0
#endif
#define BT_SUPPORT_MUSIC_VOL_SYNC                 0     //音量同步
#define BT_SUPPORT_DISPLAY_BAT                    1     //电池电量同步显示功能
#define SPP_TRANS_DATA_EN                         1     //SPP数传demo
#define TCFG_BT_SUPPORT_AAC                       0     //蓝牙AAC格式支持

//#define SDTAP_DEBUG

#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)

#endif

