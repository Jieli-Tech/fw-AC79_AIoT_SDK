/**
 * @file tuya_os_adapt_bt.h
 * @brief BLE操作接口
 *
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 *
 */

#ifndef TUYA_OS_ADAPT_BT_H__
#define TUYA_OS_ADAPT_BT_H__


#include <stdbool.h>
#include <stdint.h>
#include "tuya_cloud_types.h"
// #include "tuya_ble_type.h"
#include "tuya_os_adapter.h"


//hz-低功耗时，只支持蓝牙配网，不支持蓝牙控制，该宏的作用只是框住相关代码，便于管理
#define ONLY_SUPPORT_BT_CONFIG      (1)

/**
 * @brief tuya_os_adapt_bt_port_init
 *
 * @param[in] p
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_port_init(ty_bt_param_t *p);

/**
 * @brief tuya_os_adapt_bt_port_deinit
 *
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_port_deinit(void);

/**
 * @brief tuya_os_adapt_bt_gap_disconnect
 *
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_gap_disconnect(void);

/**
 * @brief tuya_os_adapt_bt_send
 *
 * @param[in] data: send buffer
 * @param[in] len: send buffer length
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_send(const unsigned char *data, const unsigned char len);

/**
 * @brief tuya_os_adapt_bt_reset_adv
 *
 * @param[out] adv
 * @param[out] scan_resp
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_reset_adv(tuya_ble_data_buf_t *adv, tuya_ble_data_buf_t *scan_resp);

/**
 * @brief tuya_os_adapt_bt_get_rssi
 *
 * @param[out] rssi
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_get_rssi(SCHAR_T *rssi);

/**
 * @brief tuya_os_adapt_bt_start_adv
 *
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_start_adv(void);

/**
 * @brief tuya_os_adapt_bt_stop_adv
 *
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_stop_adv(void);

/**
 * @brief
 *
 * @param ty_bt_scan_info_t
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_assign_scan(INOUT ty_bt_scan_info_t *info);


/**
 * @brief
 *
 * @param
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_get_ability(VOID_T);

//给sdk使用：
OPERATE_RET tuya_os_adapt_bt_scan_init(IN TY_BT_SCAN_ADV_CB scan_adv_cb);
OPERATE_RET tuya_os_adapt_bt_start_scan(VOID_T);
OPERATE_RET tuya_os_adapt_bt_stop_scan(VOID_T);

/* add begin: by sunkz, interface regist */
OPERATE_RET tuya_os_adapt_reg_bt_intf(void);
/* add end */


#endif


