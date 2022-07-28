/*********************************************************************************************
    *   Filename        : btctrler_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-18 14:39

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "btcontroller_config.h"
#include "app_config.h"

/**
 * @brief Bluetooth Module
 */
#if TCFG_USER_BT_CLASSIC_ENABLE && TCFG_USER_BLE_ENABLE
const int config_btctler_modules        = (BT_MODULE_CLASSIC | BT_MODULE_LE);
#elif TCFG_USER_BT_CLASSIC_ENABLE
const int config_btctler_modules        = (BT_MODULE_CLASSIC);
#elif TCFG_USER_BLE_ENABLE
const int config_btctler_modules        = (BT_MODULE_LE);
#else
const int config_btctler_modules        = 0;
#endif

#ifdef CONFIG_RF_TEST_ENABLE
int config_btctler_mode                 = BT_FCC;
int config_btctler_hci_standard         = 1;
#else
const int config_btctler_mode           = CONFIG_BT_MODE;
#if (CONFIG_BT_MODE != BT_NORMAL)
const int config_btctler_hci_standard   = 1;
#else
const int config_btctler_hci_standard   = 0;
#endif
#endif
#if TCFG_USER_EMITTER_ENABLE && defined CONFIG_NET_ENABLE
const int config_bt_function            = BT_MASTER_QOS;
#else
const int config_bt_function            = 0;
#endif
const int config_btctler_le_tws         = 0;
const int CONFIG_BTCTLER_TWS_ENABLE     = 0;
const int CONFIG_TWS_AFH_ENABLE         = 0;
const int CONFIG_WIFI_DETECT_ENABLE     = 0;
const int ESCO_FORWARD_ENABLE = 0;
const int CONFIG_LOW_LATENCY_ENABLE     = 0;
const int CONFIG_ESCO_MUX_RX_BULK_ENABLE  =  0;
const int CONFIG_BTCTLER_FAST_CONNECT_ENABLE     = 0;
const int CONFIG_LMP_NAME_REQ_ENABLE    = 1;
const int CONFIG_LMP_PASSKEY_ENABLE     = 1;
const int CONFIG_LMP_MASTER_ESCO_ENABLE = 1;
const int CONFIG_INQUIRY_PAGE_OFFSET_ADJUST = 0;
//固定使用正常发射功率的等级:0-使用不同模式的各自等级;1~10-固定发射功率等级
const int config_force_bt_pwr_tab_using_normal_level  = 0;
/*-----------------------------------------------------------*/

/**
 * @brief Bluetooth Classic setting
 */
const int config_bredr_afh_user           = 0;
const int config_btctler_bredr_master     = 0;
const int config_btctler_dual_a2dp		  = 0;
const u8 rx_fre_offset_adjust_enable      = 1;
const int config_fix_fre_enable           = 0;
const int ble_disable_wait_enable         = 1;
const int config_delete_link_key          = 1;	//配置是否连接失败返回PIN or Link Key Missing时删除linkKey
const int config_btstask_auto_exit_sniff  = 1;
const int CONFIG_TEST_DUT_CODE            = 1;
const int CONFIG_TEST_FCC_CODE            = 1;
const int CONFIG_TEST_DUT_ONLY_BOX_CODE   = 0;
#if TCFG_USER_EMITTER_ENABLE
const int CONFIG_BREDR_INQUIRY            = 1;
#else
const int CONFIG_BREDR_INQUIRY            = 0;
#endif

/*-----------------------------------------------------------*/
/**
 * @brief Bluetooth LE setting
 */
const uint64_t config_btctler_le_features = (LE_ENCRYPTION);
const int config_btctler_le_roles    = (LE_ADV | LE_SCAN | LE_INIT | LE_SLAVE | LE_MASTER);
// Master AFH
const int config_btctler_le_afh_en = 0;
// LE RAM Control
#ifdef CONFIG_NO_SDRAM_ENABLE
const int config_btctler_le_hw_ram_use_static = 0;
#else
const int config_btctler_le_hw_ram_use_static = 1;
#endif
#if CONFIG_BLE_MESH_ENABLE
const int config_btctler_le_hw_nums = 3;
const int config_btctler_le_rx_nums = 10;	//3
const int config_btctler_le_acl_packet_length = 27;	//251
const int config_btctler_le_acl_total_nums = 10;	//3
// Master multi-link
const int config_btctler_le_master_multilink = 0;
#else
#if TRANS_MULTI_BLE_EN
const int config_btctler_le_hw_nums = TRANS_MULTI_BLE_SLAVE_NUMS + TRANS_MULTI_BLE_MASTER_NUMS;
const int config_btctler_le_rx_nums = 5 * config_btctler_le_hw_nums;
const int config_btctler_le_acl_packet_length = 27;
const int config_btctler_le_acl_total_nums = 5 * config_btctler_le_hw_nums;
// Master multi-link
const int config_btctler_le_master_multilink = 1;
#else
const int config_btctler_le_hw_nums = 1;
const int config_btctler_le_rx_nums = 5;	//3
const int config_btctler_le_acl_packet_length = 27;	//251
const int config_btctler_le_acl_total_nums = 5;	//3
// Master multi-link
const int config_btctler_le_master_multilink = 0;
#endif
#endif

const u32 config_vendor_le_bb = 0;
const int config_btctler_le_slave_conn_update_winden = 2500;//range:100 to 2500
const int config_btctler_single_carrier_en = 0;
const int sniff_support_reset_anchor_point = 0;
const int config_bt_security_vulnerability = 0;


//le 配置,可以优化代码和RAM
#if TRANS_MULTI_BLE_EN
const int config_le_hci_connection_num = TRANS_MULTI_BLE_SLAVE_NUMS + TRANS_MULTI_BLE_MASTER_NUMS;//支持同时连接个数
const int config_le_sm_support_enable  = 1; //是否支持加密配对
const int config_le_gatt_server_num    = TRANS_MULTI_BLE_SLAVE_NUMS;   //支持server角色个数
const int config_le_gatt_client_num    = TRANS_MULTI_BLE_MASTER_NUMS;   //支持client角色个数
#else
const int config_le_hci_connection_num = 1;//支持同时连接个数
#if CONFIG_BLE_MESH_ENABLE
const int config_le_sm_support_enable  = 0; //是否支持加密配对
#else
const int config_le_sm_support_enable  = 1; //是否支持加密配对
#endif
const int config_le_gatt_server_num    = 1;   //支持server角色个数
const int config_le_gatt_client_num    = 1;   //支持client角色个数
#endif

/*-----------------------------------------------------------*/

/**
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 */
/*-----------------------------------------------------------*/
//RF part
const char log_tag_const_v_Analog AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_Analog AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_Analog AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_Analog AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_e_Analog AT(.LOG_TAG_CONST) = 0;

const char log_tag_const_v_RF AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_RF AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_RF AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_RF AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_e_RF AT(.LOG_TAG_CONST) = 0;

//Classic part
const char log_tag_const_v_HCI_LMP AT(.LOG_TAG_CONST)  = 0;
const char log_tag_const_i_HCI_LMP AT(.LOG_TAG_CONST)  = 1;
const char log_tag_const_d_HCI_LMP AT(.LOG_TAG_CONST)  = 0;
const char log_tag_const_w_HCI_LMP AT(.LOG_TAG_CONST)  = 1;
const char log_tag_const_e_HCI_LMP AT(.LOG_TAG_CONST)  = 1;

const char log_tag_const_v_LMP AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_i_LMP AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_LMP AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LMP AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LMP AT(.LOG_TAG_CONST) = 1;

//LE part
const char log_tag_const_v_LE_BB AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LE_BB AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LE_BB AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_LE_BB AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LE_BB AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LE5_BB AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LE5_BB AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LE5_BB AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LE5_BB AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LE5_BB AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_HCI_LL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_HCI_LL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_HCI_LL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_HCI_LL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_HCI_LL AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_LL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_E AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_E AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_E AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_E AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_E AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_M AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_M AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_M AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_M AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_M AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_ADV AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_ADV AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_ADV AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_ADV AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_ADV AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_SCAN AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_SCAN AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_SCAN AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_SCAN AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_SCAN AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_INIT AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_INIT AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_INIT AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_INIT AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_INIT AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_EXT_ADV AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_EXT_ADV AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_EXT_ADV AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_EXT_ADV AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_EXT_ADV AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_EXT_SCAN AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_EXT_SCAN AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_EXT_SCAN AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_EXT_SCAN AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_EXT_SCAN AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_EXT_INIT AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_EXT_INIT AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_EXT_INIT AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_EXT_INIT AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_EXT_INIT AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_TWS_ADV AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_TWS_ADV AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_TWS_ADV AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_TWS_ADV AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_TWS_ADV AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_TWS_SCAN AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_TWS_SCAN AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_TWS_SCAN AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_TWS_SCAN AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_TWS_SCAN AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_S AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_S AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_LL_S AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_S AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_S AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_RL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_RL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_LL_RL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_RL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_RL AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_WL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_WL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_LL_WL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_WL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_WL AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_PADV AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_PADV AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_PADV AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_PADV AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_PADV AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_DX AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_DX AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_DX AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_DX AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_DX AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_PHY AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_PHY AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_LL_PHY AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_LL_PHY AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_PHY AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_LL_AFH AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_LL_AFH AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_LL_AFH AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_LL_AFH AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_LL_AFH AT(.LOG_TAG_CONST) = 1;

//HCI part
const char log_tag_const_v_H4_USB AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_H4_USB AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_H4_USB AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_H4_USB AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_H4_USB AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_Thread AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_Thread AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_Thread AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_Thread AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_Thread AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_AES AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_AES AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_AES AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_AES AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_AES AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_HCI_STD AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_HCI_STD AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_HCI_STD AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_HCI_STD AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_HCI_STD AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_HCI_LL5 AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_HCI_LL5 AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_HCI_LL5 AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_HCI_LL5 AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_HCI_LL5 AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_BL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_BL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_BL AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_BL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_BL AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_c_BL AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_TWS_LE AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_i_TWS_LE AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_TWS_LE AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_TWS_LE AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_e_TWS_LE AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_c_TWS_LE AT(.LOG_TAG_CONST) = 1;

const char log_tag_const_v_TWS_LMP AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_i_TWS_LMP AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_TWS_LMP AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_TWS_LMP AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_e_TWS_LMP AT(.LOG_TAG_CONST) = 0;

