#ifndef __MESH_CONFIG_H__
#define __MESH_CONFIG_H__

/*******************************************************************/
/*
 *-------------------   SIG Mesh config
 */

/* Log debug config */
#define MESH_CODE_LOG_DEBUG_EN                  1
#define CONFIG_BT_DEBUG                         1
#define MESH_ADAPTATION_OPTIMIZE                1

/* Net buffer config */
#define NET_BUF_USE_MALLOC                      1
#define CONFIG_NET_BUF_USER_DATA_SIZE 		    4
#ifndef NET_BUF_USE_MALLOC
#define CONFIG_BT_MESH_ADV_BUF_COUNT 		    4
#endif /* NET_BUF_USE_MALLOC */

/* Buf Replace Config */
#define CONFIG_BUF_REPLACE_EN					0

/* Compile config */
#define ADAPTATION_COMPILE_DEBUG                0
#define MESH_RAM_AND_CODE_MAP_DETAIL            0
// #define CONFIG_ATOMIC_OPERATIONS_BUILTIN        1
#define CMD_DIRECT_TO_BTCTRLER_TASK_EN          1

/* Node features config */
#define CONFIG_BT_MESH_RELAY                    1
#define CONFIG_BT_MESH_PROXY                    1
#define CONFIG_BT_MESH_GATT_PROXY               1
#define CONFIG_BT_MESH_LOW_POWER                1
#define CONFIG_BT_MESH_FRIEND                   1

/* LPN config */
#ifdef CONFIG_BT_MESH_LOW_POWER
#define CONFIG_BT_MESH_LPN_ESTABLISHMENT        1
#define CONFIG_BT_MESH_LPN_AUTO                 1
#define CONFIG_BT_MESH_LPN_GROUPS               8
#define CONFIG_BT_MESH_LPN_AUTO_TIMEOUT         config_bt_mesh_lpn_auto_timeout // 15
#define CONFIG_BT_MESH_LPN_RETRY_TIMEOUT        config_bt_mesh_lpn_retry_timeout // 8
#define CONFIG_BT_MESH_LPN_SCAN_LATENCY         config_bt_mesh_lpn_scan_latency // 10
#define CONFIG_BT_MESH_LPN_MIN_QUEUE_SIZE       config_bt_mesh_lpn_min_queue_size // 1
#define CONFIG_BT_MESH_LPN_POLL_TIMEOUT         config_bt_mesh_lpn_poll_timeout // 300
#define CONFIG_BT_MESH_LPN_RECV_DELAY           config_bt_mesh_lpn_recv_delay // 100
#define CONFIG_BT_MESH_LPN_INIT_POLL_TIMEOUT    config_bt_mesh_lpn_init_poll_timeout // 300
#define CONFIG_BT_MESH_LPN_RSSI_FACTOR          config_bt_mesh_lpn_rssi_factor // 0
#define CONFIG_BT_MESH_LPN_RECV_WIN_FACTOR      config_bt_mesh_lpn_recv_win_factor // 0
#endif /* CONFIG_BT_MESH_LOW_POWER */

/* Net buffer config */
#define NET_BUF_TEST_EN                         0
#define NET_BUF_FREE_EN                         1
#define NET_BUF_USE_MALLOC                      1
#define CONFIG_NET_BUF_USER_DATA_SIZE 		    4
#ifndef NET_BUF_USE_MALLOC
#define CONFIG_BT_MESH_ADV_BUF_COUNT 		    4
#endif /* NET_BUF_USE_MALLOC */

/* Friend config */
#if NET_BUF_USE_MALLOC
#define CONFIG_BT_MESH_FRIEND_QUEUE_SIZE        config_bt_mesh_friend_queue_size
#define CONFIG_BT_MESH_FRIEND_SUB_LIST_SIZE     config_bt_mesh_friend_sub_list_size
#define CONFIG_BT_MESH_FRIEND_LPN_COUNT         config_bt_mesh_friend_lpn_count
#else
#define CONFIG_BT_MESH_FRIEND_QUEUE_SIZE        16
#define CONFIG_BT_MESH_FRIEND_SUB_LIST_SIZE     3
#define CONFIG_BT_MESH_FRIEND_LPN_COUNT         2
#endif /* NET_BUF_USE_MALLOC */
#define CONFIG_BT_MESH_FRIEND_SEG_RX            1
#define CONFIG_BT_MESH_FRIEND_RECV_WIN          config_bt_mesh_friend_recv_win // 255

/* Proxy config */
#define CONFIG_BT_MAX_CONN                      1
#define CONFIG_BT_MESH_PROXY_FILTER_SIZE        3

/* Net config */
#define CONFIG_BT_MESH_SUBNET_COUNT             2
#define CONFIG_BT_MESH_MSG_CACHE_SIZE           3
#define CONFIG_BT_MESH_IVU_DIVIDER              4

/* Transport config */
#define CONFIG_BT_MESH_TX_SEG_MAX 			    6
#define CONFIG_BT_MESH_TX_SEG_MSG_COUNT 	    1
#define CONFIG_BT_MESH_RX_SEG_MSG_COUNT 	    1
#define CONFIG_BT_MESH_RX_SDU_MAX 			    72

/* Element models config */
#define CONFIG_BT_MESH_CFG_CLI                  1
// #define CONFIG_BT_MESH_HEALTH_SRV               1
#define CONFIG_BT_MESH_APP_KEY_COUNT            2
#define CONFIG_BT_MESH_MODEL_KEY_COUNT          2
#define CONFIG_BT_MESH_MODEL_GROUP_COUNT        2
#define CONFIG_BT_MESH_CRPL                     10
#define CONFIG_BT_MESH_LABEL_COUNT              3

/* Provisioning config */
#define CONFIG_BT_MESH_PROV                     1
#define CONFIG_BT_MESH_PB_ADV                   1
#define CONFIG_BT_MESH_PB_GATT                  1

/* Store config */
#define CONFIG_BT_SETTINGS                      1
#define CONFIG_BT_MESH_STORE_TIMEOUT            2
#define CONFIG_BT_MESH_SEQ_STORE_RATE 		    128
#define CONFIG_BT_MESH_RPL_STORE_TIMEOUT        600

/* Provisioner config */
#define CONFIG_BT_MESH_PROVISIONER              1
#if CONFIG_BT_MESH_PROVISIONER
#define CONFIG_BT_MESH_CDB                      1
#define CONFIG_BT_MESH_CDB_NODE_COUNT           3
#define CONFIG_BT_MESH_CDB_SUBNET_COUNT         3
#define CONFIG_BT_MESH_CDB_APP_KEY_COUNT        3
#endif /* CONFIG_BT_MESH_PROVISIONER */

/* TODO */
// #define CONFIG_BT_MESH_HEALTH_CLI               1

/*******************************************************************/
/*
 *-------------------   SIG Mesh features
 */

#define BT_MESH_FEAT_RELAY                  BIT(0)
#define BT_MESH_FEAT_PROXY                  BIT(1)
#define BT_MESH_FEAT_FRIEND                 BIT(2)
#define BT_MESH_FEAT_LOW_POWER              BIT(3)

#define BT_MESH_FEATURES_GET(x)             (!!(BT_MESH_FEAT_SUPPORTED & x))

#define BT_MESH_FEATURES_IS_SUPPORT(x)              (config_bt_mesh_features & (x))

#define BT_MESH_FEATURES_IS_SUPPORT_OPTIMIZE(x)     if (BT_MESH_FEATURES_IS_SUPPORT(x) == 0x0) return


/*******************************************************************/
/*
 *-------------------   APP config
 */

/**
 * @brief Config current node features(Relay/Proxy/Friend/Low Power)
 */
/*-----------------------------------------------------------*/
extern const int config_bt_mesh_features;

/**
 * @brief Config adv bearer hardware param when node send messages
 */
/*-----------------------------------------------------------*/
extern const u16 config_bt_mesh_node_msg_adv_interval;
extern const u16 config_bt_mesh_node_msg_adv_duration;

/**
 * @brief Config proxy connectable adv hardware param
 */
/*-----------------------------------------------------------*/
extern const u16 config_bt_mesh_proxy_unprovision_adv_interval;
extern const u16 config_bt_mesh_proxy_node_adv_interval;
extern const u16 config_bt_mesh_proxy_pre_node_adv_interval;

/**
 * @brief Config lpn node character
 */
/*-----------------------------------------------------------*/
extern const u8 config_bt_mesh_lpn_auto_timeout;
extern const u8 config_bt_mesh_lpn_retry_timeout;
extern const int config_bt_mesh_lpn_scan_latency;
extern const u32 config_bt_mesh_lpn_init_poll_timeout;
extern const u8 config_bt_mesh_lpn_powerup_add_sub_list;
extern const u8 config_bt_mesh_lpn_recv_delay;
extern const u32 config_bt_mesh_lpn_poll_timeout;
extern const u8 config_bt_mesh_lpn_rssi_factor;
extern const u8 config_bt_mesh_lpn_recv_win_factor;
extern const u8 config_bt_mesh_lpn_min_queue_size;

/**
 * @brief Config friend node character
 */
/*-----------------------------------------------------------*/
extern const u8 config_bt_mesh_friend_lpn_count;
extern const u8 config_bt_mesh_friend_recv_win;
extern const u8 config_bt_mesh_friend_sub_list_size;
extern const u8 config_bt_mesh_friend_queue_size;

/**
 * @brief Config cache buffer
 */
/*-----------------------------------------------------------*/
extern const u8 config_bt_mesh_adv_buf_count;

/**
 * @brief Config PB-ADV param
 */
/*-----------------------------------------------------------*/
extern const u16 config_bt_mesh_pb_adv_interval;
extern const u16 config_bt_mesh_pb_adv_duration;
extern const u32 config_bt_mesh_prov_retransmit_timeout;
extern const u8 config_bt_mesh_prov_transaction_timeout;
extern const u8 config_bt_mesh_prov_link_close_timeout;
extern const u8 config_bt_mesh_prov_protocol_timeout;

/**
 * @brief Config beacon param
 */
/*-----------------------------------------------------------*/
extern const u32 config_bt_mesh_unprov_beacon_interval;
extern const u16 config_bt_mesh_secure_beacon_interval;

#endif /* __MESH_CONFIG_H__ */
