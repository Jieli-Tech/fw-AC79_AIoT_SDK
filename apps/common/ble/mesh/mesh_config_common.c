#include "app_config.h"
#include "bt_common.h"
#include "api/sig_mesh_api.h"

#if CONFIG_BLE_MESH_ENABLE

/**
 * @brief Config adv bearer hardware param when node send messages
 */
/*-----------------------------------------------------------*/
#if (CONFIG_MESH_MODEL == SIG_MESH_PROVISIONER)
const u16 config_bt_mesh_node_msg_adv_interval = ADV_SCAN_UNIT(15); // unit: ms
const u16 config_bt_mesh_node_msg_adv_duration = 40; // unit: ms
#else
const u16 config_bt_mesh_node_msg_adv_interval = ADV_SCAN_UNIT(10); // unit: ms
const u16 config_bt_mesh_node_msg_adv_duration = 100; // unit: ms
#endif

/**
 * @brief Config proxy connectable adv hardware param
 */
/*-----------------------------------------------------------*/
const u16 config_bt_mesh_proxy_unprovision_adv_interval = ADV_SCAN_UNIT(30); // unit: ms
const u16 config_bt_mesh_proxy_pre_node_adv_interval = ADV_SCAN_UNIT(10); // unit: ms
_WEAK_
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(300); // unit: ms

/**
 * @brief Config lpn node character
 */
/*-----------------------------------------------------------*/
const u8 config_bt_mesh_lpn_auto_timeout = 0; // unit: s
const u8 config_bt_mesh_lpn_retry_timeout = 10; // unit: s
const int config_bt_mesh_lpn_scan_latency = 10; // unit: ms
const u32 config_bt_mesh_lpn_init_poll_timeout = 300; // unit: 100ms
const u8 config_bt_mesh_lpn_powerup_add_sub_list = 1;
//< 3.6.5.3 Friend Request
const u8 config_bt_mesh_lpn_recv_delay = 100; // unit: ms
const u32 config_bt_mesh_lpn_poll_timeout = 200; // unit: 100ms
const u8 config_bt_mesh_lpn_rssi_factor = 0;
const u8 config_bt_mesh_lpn_recv_win_factor = 0;
const u8 config_bt_mesh_lpn_min_queue_size = 1;

/**
 * @brief Config friend node character
 */
/*-----------------------------------------------------------*/
const u8 config_bt_mesh_friend_lpn_count = 1;
//< 3.6.5.4 Friend Offer
const u8 config_bt_mesh_friend_recv_win = 250;
const u8 config_bt_mesh_friend_sub_list_size = 2;
const u8 config_bt_mesh_friend_queue_size = 2;

/**
 * @brief Config adv cache buffer
 */
/*-----------------------------------------------------------*/
#define MESH_ADV_BUFFER_COUNT           10
_WEAK_
const u8 config_bt_mesh_adv_buf_count = MESH_ADV_BUFFER_COUNT; // must >= 3

#if (MESH_ADV_BUFFER_COUNT < 3)
#error " MESH_ADV_BUFFER_COUNT must >= 3 "
#endif

/**
 * @brief Config PB-ADV param
 */
/*-----------------------------------------------------------*/
const u16 config_bt_mesh_pb_adv_interval = ADV_SCAN_UNIT(15); // unit: ms
const u16 config_bt_mesh_pb_adv_duration = 40; // unit: ms
const u32 config_bt_mesh_prov_retransmit_timeout = 300; // unit: ms
const u8 config_bt_mesh_prov_transaction_timeout = 30; // unit: s
const u8 config_bt_mesh_prov_link_close_timeout = 3; // unit: s
const u8 config_bt_mesh_prov_protocol_timeout = 60; // unit: s

/**
 * @brief Config beacon param
 */
/*-----------------------------------------------------------*/
const u32 config_bt_mesh_unprov_beacon_interval = 200; // unit: ms
const u16 config_bt_mesh_secure_beacon_interval = 10; // unit: s

/**
 * @brief Ble Mesh Log
 */
/*-----------------------------------------------------------*/
const char log_tag_const_v_MESH_HCI AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_MESH_HCI AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_MESH_HCI AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_MESH_HCI AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_e_MESH_HCI AT(.LOG_TAG_CONST) = 0;

const char log_tag_const_v_MESH_GATT AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_MESH_GATT AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_d_MESH_GATT AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_w_MESH_GATT AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_e_MESH_GATT AT(.LOG_TAG_CONST) = 0;

#endif
