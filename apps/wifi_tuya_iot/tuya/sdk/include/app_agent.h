/**
* @file app_agent.h
* @brief Common process - app agent
* @version 0.1
* @date 2015-06-18
*
* @copyright Copyright 2015-2021 Tuya Inc. All Rights Reserved.
*
*/

#ifndef _APP_AGENT_H
#define _APP_AGENT_H

#include "tuya_cloud_types.h"
#include "uni_network.h"

#ifdef __cplusplus
extern "C" {
#endif

//group test
#define FRM_GRP_OPER_ENGR 0xd0
#define FRM_GRP_CMD 0xd1

/**
 * @brief lan protocol init
 *
 * @param[in] wechat true/false
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_pro_cntl_init(BOOL_T wechat);

/**
 * @brief lan protocol exit
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_pro_cntl_exit(VOID);

/**
 * @brief lan protocol diable
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_pro_cntl_disable(VOID);

/**
 * @brief lan protocol enable
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_pro_cntl_enable(VOID);

/**
 * @brief lan dp report
 *
 * @param[in] data data buf
 * @param[in] len buf length
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_dp_sata_report(IN CONST VOID *data, IN CONST UINT_T len);

/**
 * @brief lan dp report callback
 *
 * @param[in] fr_type refer to LAN_PRO_HEAD_APP_S
 * @param[in] ret_code refer to LAN_PRO_HEAD_APP_S
 * @param[in] data refer to LAN_PRO_HEAD_APP_S
 * @param[in] len refer to LAN_PRO_HEAD_APP_S
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_data_report_cb(IN CONST UINT_T fr_type, IN CONST UINT_T ret_code, \
                               IN CONST BYTE_T *data, IN CONST UINT_T len);

/**
 * @brief get vaild connect count
 *
 * @return vaild count
 */
INT_T lan_pro_cntl_get_valid_connect_cnt(VOID);

/**
 * @brief disconnect all sockets
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_disconnect_all_sockets(VOID_T);

typedef enum {
    CFG_UDP_DISCOVERY_FORCE,    // send upd discovery even if clients exceed(BOOL_T)
    CFG_UDP_EXT_CONTENT,        // deprecated(reserved for gw/ipc)
    CFG_UDP_EXT_UPDATE,         // add/update new key/value(ty_cJSON)
    CFG_UDP_EXT_DELETE,         // delete key/value(ty_cJSON)
    CFG_SET_CLT_NUM,            // set clinet number(UINT_T)
    CFG_UDP_DISCOVERY_INTERVAL, // set udp discovery interval(UINT_T, unit:s, default:5)
    CFG_REV_BUF_SIZE,           // receive buffer size(UINT, default:512)
    CFG_MAX
} Lan_Cfg_e;

/**
 * @brief lan configure
 *
 * @param[in] cfg refer to Lan_Cfg_e
 * @param[in] data buf
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_pro_cntl_cfg(Lan_Cfg_e cfg, VOID *data);

/**
 * @brief lan cmd extersion, caller will free out
 *
 * @param[in] data data
 * @param[out] out buf
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
typedef OPERATE_RET(*lan_cmd_handler_cb)(IN CONST BYTE_T *data, OUT BYTE_T **out);

/**
 * @brief register callback
 *
 * @param[in] frame_type refer to LAN_PRO_HEAD_APP_S
 * @param[in] frame_type refer to lan_cmd_handler_cb
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_pro_cntl_register_cb(UINT_T frame_type, lan_cmd_handler_cb handler);

/**
 * @brief unregister callback
 *
 * @param[in] frame_type refer to LAN_PRO_HEAD_APP_S
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET lan_pro_cntl_unregister_cb(UINT_T frame_type);

/**
 * @brief judge if lan connect
 *
 * @return TRUE/FALSE
 */
BOOL_T is_lan_connected(VOID_T);

/**
 * @brief get lan client number
 *
 * @return client number
 */
UINT_T tuya_svc_lan_get_client_num(VOID);

#ifdef __cplusplus
}
#endif
#endif

