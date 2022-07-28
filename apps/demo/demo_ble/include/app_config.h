#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#define __FLASH_SIZE__    (4 * 1024 * 1024)

#ifdef CONFIG_NO_SDRAM_ENABLE
#define __SDRAM_SIZE__    (0 * 1024 * 1024)
#else
#define __SDRAM_SIZE__    (2 * 1024 * 1024)
#endif

#define CONFIG_DEBUG_ENABLE                  //打印开关
#define CONFIG_LOW_POWER_ENABLE              //软关机/睡眠开关

#define RTOS_STACK_CHECK_ENABLE //是否启用定时检查任务栈
// #define MEM_LEAK_CHECK_ENABLE	//是否启用内存泄漏检查(需要包含mem_leak_test.h头文件)


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
#ifdef CONFIG_BT_ENABLE

#define CONFIG_BT_RX_BUFF_SIZE  0
#define CONFIG_BT_TX_BUFF_SIZE  0

#define TCFG_USER_BLE_ENABLE                      1     //BLE功能使能
#define TCFG_USER_BT_CLASSIC_ENABLE               0     //经典蓝牙功能

#if TCFG_USER_BLE_ENABLE
//BLE DEMO选择
#define BT_NET_HID_EN                             0     //从机 hid
#define TRANS_DATA_EN                             0     //从机 传输数据
#define BT_NET_CENTRAL_EN                         0     //主机 client角色
#define TRANS_MULTI_BLE_EN                        0     //多机通讯
#define CONFIG_BLE_MESH_ENABLE                    1     //mesh测试demo

#if (TRANS_MULTI_BLE_EN + BT_NET_HID_EN + BT_NET_CENTRAL_EN + TRANS_DATA_EN  + CONFIG_BLE_MESH_ENABLE > 1)
#error "they can not enable at the same time,just select one!!!"
#endif

#define TCFG_BLE_SECURITY_EN                      1     //配对加密使能

#if TRANS_MULTI_BLE_EN
#define TRANS_MULTI_BLE_SLAVE_NUMS                2
#define TRANS_MULTI_BLE_MASTER_NUMS               2
#endif

#endif

#endif

#if CONFIG_BLE_MESH_ENABLE
#define TCFG_ADKEY_ENABLE             1      //AD按键
#else
#define TCFG_ADKEY_ENABLE             0      //AD按键
#endif
//#define SDTAP_DEBUG

#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif
#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)

#endif

