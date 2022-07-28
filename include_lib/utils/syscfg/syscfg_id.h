#ifndef __JL_CFG_DEC_H__
#define __JL_CFG_DEC_H__

/// \cond DO_NOT_DOCUMENT
struct btif_item {
    u16 id;
    u16 data_len;
};
/// \endcond


//=================================================================================//
//                        系统配置项(VM, BTIF, cfg_bin)读写接口                    //
//接口说明:                                			   						       //
// 	1.输入参数                                			     					   //
// 		1)item_id: 配置项ID号, 由本文件统一分配;                                   //
// 		2)buf: 用于存储read/write数据内容;                                    	   //
// 		3)len: buf的长度(byte), buf长度必须大于等于read/write数据长度;             //
// 	2.返回参数:                                 			     				   //
// 		1)执行正确: 返回值等于实际上所读到的数据长度(大于0);                       //
//      2)执行错误: 返回值小于等于0, 小于0表示相关错误码;                          //
// 	3.读写接口使用注意事项:             										   //
// 		1)不能在中断里调用写(write)接口;                                   		   //
// 		2)调用本读写接口时应该习惯性判断返回值来检查read/write动作是否执行正确;    //
//=================================================================================//

/* --------------------------------------------------------------------------*/
/**
 * @brief 读取对应配置项的内容
 *
 * @param  [in] item_id 配置项ID号
 * @param  [out] buf 用于存储read数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于read数据长度
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_read(u16 item_id, void *buf, u16 len);

/* --------------------------------------------------------------------------*/
/**
 * @brief 写入对应配置项的内容
 *
 * @param  [in] item_id 配置项ID号
 * @param  [in] buf 用于存储write数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于write数据长度
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_write(u16 item_id, void *buf, u16 len);

/* --------------------------------------------------------------------------*/
/**
 * @brief 以dma的方式写入对应配置项的内容, 请注意buf地址需要按照4byte对齐
 *
 * @param  [in] item_id 配置项ID号
 * @param  [in] buf 用于存储write数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于write数据长度
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_dma_write(u16 item_id, void *buf, u16 len);

/* --------------------------------------------------------------------------*/
/**
 * @brief 读取同一个配置项存在多份数据中的某一份数据, ver读取表示第几份数据, ver从 0 开始;
 * @brief 典型应用: 读取配置项CFG_BT_NAME中多个蓝牙名中的某一个蓝牙名;
 *
 * @param  [in] item_id 配置项ID号
 * @param  [in] buf 用于存储read数据内容
 * @param  [in] len buf的长度(byte), buf长度必须大于等于read数据长度
 * @param  [in] ver 读取表示第几份数据
 *
 * @return 1)执行正确: 返回值等于实际上所读到的数据长度(大于0);
 *         2)执行错误: 返回值小于等于0, 小于0表示相关错误码;
 */
/* --------------------------------------------------------------------------*/
int syscfg_read_string(u16 item_id, void *buf, u16 len, u8 ver);

/* --------------------------------------------------------------------------*/
/**
 * @brief 获取配置项的地址
 * @brief 注: 只支持cfg_tools.bin文件中的配置项读取
 * @param  [in] item_id 配置项ID号
 * @param  [out] len 配置项长度
 *
 * @return 配置项地址指针(可以用cpu直接访问);
 */
/* --------------------------------------------------------------------------*/
u8 *syscfg_ptr_read(u16 item_id, u16 *len);

//=================================================================================//
//                               配置项出错码                                      //
//=================================================================================//
typedef enum JL_CONFIG_ERR_TABLE {
    JL_CONFIG_ERR_NONE = 0,         			//0: 执行正确
    VM_ERR_NONE = 0,
//BIN
    JL_CONFIG_BIN_FILE_OPEN_ERR = -0x400,  		//-1024: bin文件打开失败
    JL_CONFIG_BIN_FILE_READ_ERR,				//-1023: bin文件读数据出错
    JL_CONFIG_BIN_INPUT_BUF_TOO_LESS_ERR, 		//-1022: 输出buf小于配置项大小
    JL_CONFIG_BIN_DATA_CRC_ERR, 				//-1021: 配置crc校验出错
    JL_CONFIG_BIN_MALLOC_STR_BUF_ERR, 			//-1020: malloc出错
    JL_CONFIG_BIN_ITEM_NOT_EXIST_ERR, 			//-1019: 所读配置项不存在
    JL_CONFIG_BIN_GROUP_VER_ERR, 				//-1018: 所读配置项版本不对
    JL_CONFIG_BIN_ITEM_HEAD_ERR, 				//-1017: 配置项HEAD数据错误
//BTIF
    JL_CONFIG_BTIF_READ_ITEM_NOT_WRITE_ERR = -0x300, 	//-768: 所读配置项数据未写入
    JL_CONFIG_BTIF_WRITE_ITEM_EXIST_ERR,  	 	//-767: 所写配置项已存在
    JL_CONFIG_BTIF_FILE_NOT_EXIST_ERR,    		//-766: BTIF文件不存在
    JL_CONFIG_BTIF_READ_ITEM_NOT_EXIST_ERR, 	//-765: 所读配置项未注册
    JL_CONFIG_BTIF_WRITE_ITEM_NOT_EXIST_ERR, 	//-764: 所写配置项未注册
    JL_CONFIG_BTIF_ITEM_DATA_CRC_ERR, 			//-763: 所读配置项数据校验出错
    JL_CONFIG_BTIF_WRITE_ITEM_ERR,  			//-762: 写配置项到BTIF出错
    JL_CONFIG_BTIF_INPUT_BUF_ERR,  	 			//-761: 输入buf为长度错误
    JL_CONFIG_BTIF_MALLOC_BUF_ERR,  	 		//-760: 申请buf出错
    JL_CONFIG_BTIF_WRITE_ITEM_LEN_ERR,			//-759: 写配置项长度出错
//VM
    VM_INDEX_ERR = -0x200,                      //-512: 配置项ID出错
    VM_DATA_LEN_ERR,                            //-511: 配置项数据长度超出最大支持长度
    VM_READ_DATA_ERR, 							//-510: 读取数据校验出错
    VM_WRITE_OVERFLOW, 							//-509: 配置项写溢出
    VM_NOT_INIT, 								//-508: VM未初始化
    VM_INIT_ALREADY, 							//-507: VM重复初始化
    VM_DEFRAG_ERR, 								//-506: VM整理出错
    VM_ERR_INIT, 								//-505: VM初始化出错
    VM_ERR_PROTECT,								//-504: VM区域flash被保护
} CFG_ERR;

/// \cond DO_NOT_DOCUMENT


//==================================================================================================//
//                                 配置项ID分配说明                                					//
//	1.配置项ID号根据存储区域进行分配;                                			   					//
//	2.存储区域有3个:  1)VM区域; 2)sys_cfg.bin; 3)BTIF区域                      	   					//
//	3.配置项ID号分配如下:  												   		   					//
//		0)[0]: 配置项ID号0为配置项工具保留ID号; 			   	   									//
//		1)[  1 ~  59]: 共59项, 预留给用户自定义, 只存于VM区域; 			   	   						//
//		2)[ 60 ~  99]: 共40项, sdk相关配置项, 只存于VM区域; 			   		   					//
//		3)[100 ~ 255]: 共156项, sdk相关配置项, 可以存于VM区域, sys_cfg.bin(作为默认值)和BTIF区域;	//
//		4)[512 ~ 700]: 共188项, sdk相关配置项, 只存于sys_cfg.bin; 		   		   					//
//==================================================================================================//

//=================================================================================//
//                             用户自定义配置项[1 ~ 59]                            //
//=================================================================================//
#define    CFG_USER_DEFINE_BEGIN        1
#define    CFG_USER_DEFINE_END          59

//=================================================================================//
//         可以存于VM, sys_cfg.bin(默认值)和BTIF区域的配置项[100 ~ 127]            //
//		   (VM支持扩展到511)    												   //
//=================================================================================//
#define    CFG_STORE_VM_BIN_BTIF_BEGIN  100
#define    CFG_STORE_VM_BIN_BTIF_END    (VM_ITEM_MAX_NUM - 1) //在app_cfg文件中配置128/256

//=================================================================================//
//                             SDK库保留配置项[60 ~ 240]                           //
//=================================================================================//
#define    CFG_REMOTE_DB_INFO          61
#define    CFG_REMOTE_DB_00            62
#define    CFG_REMOTE_DB_01            63
#define    CFG_REMOTE_DB_02            64
#define    CFG_REMOTE_DB_03            65
#define    CFG_REMOTE_DB_04            66
#define    CFG_REMOTE_DB_05            67
#define    CFG_REMOTE_DB_06            68
#define    CFG_REMOTE_DB_07            69
#define    CFG_REMOTE_DB_08            70
#define    CFG_REMOTE_DB_09            71
#define    CFG_REMOTE_DB_10            72
#define    CFG_REMOTE_DB_11            73
#define    CFG_REMOTE_DB_12            74
#define    CFG_REMOTE_DB_13            75
#define    CFG_REMOTE_DB_14            76
#define    CFG_REMOTE_DB_15            77
#define    CFG_REMOTE_DB_16            78
#define    CFG_REMOTE_DB_17            79
#define    CFG_REMOTE_DB_18            80
#define    CFG_REMOTE_DB_19            81
#define    CFG_BLE_MODE_INFO           82
#define    CFG_MUSIC_VOL               83
#define    CFG_TWS_PAIR_AA             84
#define    CFG_TWS_CONNECT_AA          85
#define    CFG_TWS_LOCAL_ADDR          86
#define    CFG_TWS_REMOTE_ADDR         87
#define    CFG_TWS_COMMON_ADDR         88
#define    CFG_TWS_CHANNEL             89
#define    CFG_DAC_TRIM_INFO           90
#define    CFG_REMOTE_DN_00            91
#define    CFG_REMOTE_DN_END           100

//=========== btif & cfg_tool.bin & vm ============//
#define    CFG_BT_NAME                 101
#define    CFG_BT_MAC_ADDR             102
#define    CFG_BLE_NAME                103
#define    CFG_BLE_MAC_ADDR            104
#define    CFG_RESERVED_UPDATE_INFO    105 //保存预留区升级信息
#define    VM_XOSC_INDEX               106
#define    VM_WIFI_PA_DATA             107
#define    VM_WIFI_PA_MCS_DGAIN        108
#define    VM_BLE_LOCAL_INFO           109
#define    CFG_BT_FRE_OFFSET           110
#define    CFG_WIFI_FRE_OFFSET         111
#define    VM_TME_AUTH_COOKIE          112

#define    VM_BLE_REMOTE_DB_INFO       113
#define    VM_BLE_REMOTE_DB_00         114
#define    VM_BLE_REMOTE_DB_01         115
#define    VM_BLE_REMOTE_DB_02         116
#define    VM_BLE_REMOTE_DB_03         117
#define    VM_BLE_REMOTE_DB_04         118
#define    VM_BLE_REMOTE_DB_05         119
#define    VM_BLE_REMOTE_DB_06         120
#define    VM_BLE_REMOTE_DB_07         121
#define    VM_BLE_REMOTE_DB_08         122
#define    VM_BLE_REMOTE_DB_09         123

//蓝牙mesh协议栈使用了124-179，若需要mesh功能请勿使用此范围index
#define    VM_BLE_MESH_IDX_BEGIN       124
#define    VM_BLE_MESH_IDX_END         179

//WIFI类配置项[180 ~ 250]
#define    NETWORK_SSID_INFO_CNT       5 //配置VM里面保存多少个路由器信息, 如果不需要写0
#define    WIFI_STA_INFO_IDX_START     180
#define    WIFI_STA_INFO_IDX_END       (WIFI_STA_INFO_IDX_START+NETWORK_SSID_INFO_CNT)
#define    VM_WIFI_RF_INIT_INFO        187
#define    WIFI_BSS_TABLE              188
#define    WIFI_SCAN_INFO              189
#define    WIFI_PMK_INFO               190
#if WIFI_STA_INFO_IDX_END >= VM_WIFI_RF_INIT_INFO
#error "NETWORK_SSID_INFO_CNT TOO LARGE"
#endif
#define    VM_VERSION_INDEX             192
#define    VM_STA_IPADDR_INDEX          193
#define    VM_USER_ID_INDEX             194
#define    WIFI_INFO_IDX                195
#define    VM_FLASH_BREAKPOINT_INDEX    196
#define    VM_SD0_BREAKPOINT_INDEX      197
#define    VM_USB0_BREAKPOINT_INDEX     198
#define    VM_FM_INFO_INDEX             199
#define    VM_SD_LOG_INDEX              200
#define    VM_AG_KEY_INFO_IDX_START     201
#define    VM_AG_KEY_INFO_IDX_END       214
#define    VM_UPG_INDEX                 215
#define    VM_FIR_INDEX                 216
#define    VM_MIC_INDEX                 217
#define    VM_RES_INDEX                 218
#define    VM_CYC_INDEX                 219
#define    VM_DAT_INDEX                 220

//蓝牙mesh协议栈使PROVISIONER功能用了221-237，若需要mesh的PROVISIONER功能请勿使用此范围index
#define    VM_BLE_MESH_CDB_IDX_BEGIN    221
#define    VM_BLE_MESH_CDB_IDX_END      237

//238-255可使用
#define    CFG_UPDATE_VERSION_INFO      238
#define    VM_RTC_LRC_TRIM_INDEX        244

//=================================================================================//
//                   只存于sys_cfg.bin的配置项[512 ~ 700]                		   //
//=================================================================================//
#define 	CFG_STORE_BIN_ONLY_BEGIN	512
//硬件类配置项[513 ~ 600]
#define		CFG_UART_ID		    		513
#define		CFG_HWI2C_ID				514
#define		CFG_SWI2C_ID				515
#define		CFG_HWSPI_ID				516
#define		CFG_SWSPI_ID				517
#define		CFG_SD_ID					518
#define		CFG_USB_ID					519
#define		CFG_LCD_ID					520
#define		CFG_TOUCH_ID				521
#define		CFG_IOKEY_ID				522
#define		CFG_ADKEY_ID				523
#define		CFG_AUDIO_ID				524
#define		CFG_VIDEO_ID				525
#define		CFG_WIFI_ID					526
#define		CFG_NIC_ID					527
#define		CFG_LED_ID					528
#define		CFG_POWER_MANG_ID			529
#define		CFG_IRFLT_ID				530
#define		CFG_PLCNT_ID				531
#define		CFG_PWMLED_ID				532
#define		CFG_RDEC_ID					533
#define		CFG_CHARGE_STORE_ID			534
#define		CFG_CHARGE_ID				535
#define    	CFG_LOWPOWER_V_ID   	    536
#define    	CFG_MIC_TYPE_ID   			537
#define    	CFG_COMBINE_SYS_VOL_ID 		538
#define    	CFG_COMBINE_CALL_VOL_ID 	539
#define    	CFG_LP_TOUCH_KEY_ID   		540

//蓝牙类配置项[601 ~ 650]
#define     CFG_BT_RF_POWER_ID			601
#define    	CFG_TWS_PAIR_CODE_ID   		602
#define    	CFG_AUTO_OFF_TIME_ID   	    603
#define    	CFG_AEC_ID   	            604
#define    	CFG_UI_TONE_STATUS_ID   	605
#define    	CFG_KEY_MSG_ID   			606
#define    	CFG_LRC_ID   				607
#define    	CFG_BLE_ID   				608
#define    	CFG_DMS_ID   	            609
#define    	CFG_ANC_ID   	            610
#define    	CFG_ANC_COEFF_CONFIG_ID     611
#define    	CFG_SMS_DNS_ID 	            612	//单mic神经网络降噪
#define    	CFG_DMS_DNS_ID 	            613	//双mic神经网络降噪
#define    	CFG_BLE_RF_POWER_ID         615

//WIFI类配置项[651 ~ 660]
#define    	CFG_WIFI_TEST_CFG_ID        651
#define    	CFG_WIFI_POWER_CFG_ID       652
#define    	CFG_WIFI_SSID_ID            653
#define    	CFG_WIFI_PWD_ID             654
#define    	CFG_WIFI_RF_CFG_ID          655

//其它类配置项[651 ~ 700]
#define 	CFG_STORE_BIN_ONLY_END		700
/// \endcond

#endif

