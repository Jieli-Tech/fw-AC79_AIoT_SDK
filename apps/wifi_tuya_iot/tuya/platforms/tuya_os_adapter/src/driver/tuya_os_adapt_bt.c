/**
 * @file tuya_os_adapt_bt.c
 * @brief bt操作接口
 *
 * @copyright Copyright (c) {2018-2020} 涂鸦科技 www.tuya.com
 *
 */
#include "tuya_os_adapt_bt.h"
#include "tuya_os_adapter.h"
#include "tuya_os_adapter_error_code.h"


/***********************************************************
*************************micro define***********************
***********************************************************/
/** 涂鸦接收回调*/
TY_BT_MSG_CB ty_bt_msg_cb;
typedef unsigned char u8;
extern void tuya_bt_send_data(const unsigned char *data, const unsigned char len);
extern void ble_app_disconnect(void);
extern void bt_ble_exit(void);
extern void bt_ble_adv_enable(u8 enable);
extern int tuya_set_adv_data(unsigned char *data, unsigned int len);
extern int tuya_set_rsp_data(unsigned char *data, unsigned int len);
extern void bt_ble_module_init(void);
/***********************************************************
*************************variable define********************
***********************************************************/
static const TUYA_OS_BT_INTF m_tuya_os_bt_intfs = {
    .port_init      = tuya_os_adapt_bt_port_init,
    .port_deinit    = tuya_os_adapt_bt_port_deinit,
    .gap_disconnect = tuya_os_adapt_bt_gap_disconnect,
    .send           = tuya_os_adapt_bt_send,
    .reset_adv      = tuya_os_adapt_bt_reset_adv,
    .get_rssi       = tuya_os_adapt_bt_get_rssi,
    .start_adv      = tuya_os_adapt_bt_start_adv,
    .stop_adv       = tuya_os_adapt_bt_stop_adv,
    .assign_scan    = tuya_os_adapt_bt_assign_scan,
    .scan_init      = tuya_os_adapt_bt_scan_init,
    .start_scan     = tuya_os_adapt_bt_start_scan,
    .stop_scan      = tuya_os_adapt_bt_stop_scan,
    .get_ability    = tuya_os_adapt_bt_get_ability,
};

/***********************************************************
*************************function define********************
***********************************************************/


/**
 * @brief tuya_os_adapt_bt 蓝牙初始化
 * @return int
 */
int tuya_os_adapt_bt_port_init(ty_bt_param_t *p)
{
    printf("tuya_os_adapt_bt_port_init");
    if ((NULL == p) && (NULL == p->cb)) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    /** 注册bt数据接收回调*/
    ty_bt_msg_cb = p->cb;

    /** 初始化蓝牙模块*/
    bt_ble_module_init();

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 蓝牙断开关闭
 * @return int
 */
int tuya_os_adapt_bt_port_deinit(void)
{
    puts("tuya_os_adapt_bt_port_deinit");
    /** 关闭蓝牙模块*/
    bt_ble_exit();

    if (ty_bt_msg_cb) {
        ty_bt_msg_cb = NULL;
    }

    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief tuya_os_adapt_bt 蓝牙断开
 * @return int
 */
int tuya_os_adapt_bt_gap_disconnect(void)
{
    puts("tuya_os_adapt_bt_gap_disconnect");
    /** 断开蓝牙连接*/
    ble_app_disconnect();

    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief tuya_os_adapt_bt 蓝牙发送
 * @return int
 */
int tuya_os_adapt_bt_send(const unsigned char *data, const unsigned char len)
{
    puts("tuya_os_adapt_bt_send");
    if (0 == len || NULL == data) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    tuya_bt_send_data(data, len);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 广播包重置
 * @return int
 */
int tuya_os_adapt_bt_reset_adv(tuya_ble_data_buf_t *adv, tuya_ble_data_buf_t *scan_resp)
{
    puts("tuya_os_adapt_bt_reset_adv");
    /** 关闭广播*/
    bt_ble_adv_enable(0);

    /** 设置adv data*/
    tuya_set_adv_data(adv->data, adv->len);

    /** 设置rsp data*/
    tuya_set_rsp_data(scan_resp->data, scan_resp->len);

    /** 打开广播*/
    bt_ble_adv_enable(1);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 获取rssi信号值
 * @return int
 */
int tuya_os_adapt_bt_get_rssi(signed char *rssi)
{
    //todo
    puts("tuya_os_adapt_bt_get_rssi : Not supported yet!\n");

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 停止广播
 * @return int
 */
int tuya_os_adapt_bt_start_adv(void)
{
    puts("tuya_os_adapt_bt_start_adv");
    bt_ble_adv_enable(1);

    return OPRT_OS_ADAPTER_BT_ADV_START_FAILED;
}

/**
 * @brief tuya_os_adapt_bt 停止广播
 * @return int
 */
int tuya_os_adapt_bt_stop_adv(void)
{
    puts("tuya_os_adapt_bt_stop_adv");
    bt_ble_adv_enable(0);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 主动扫描
 * @return int
 */
int tuya_os_adapt_bt_assign_scan(IN OUT ty_bt_scan_info_t *info)
{
    if (NULL == info) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    //todo
    puts("tuya_os_adapt_bt_assign_scan : Not supported yet!\n");

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt scan初始化
 * @return int
 */
int tuya_os_adapt_bt_scan_init(IN TY_BT_SCAN_ADV_CB scan_adv_cb)
{
    //todo
    puts("tuya_os_adapt_bt_assign_scan : Not supported yet!\n");
    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 开始scan接收
 * @return int
 */
int tuya_os_adapt_bt_start_scan(void)
{
    //todo
    puts("tuya_os_adapt_bt_start_scan : Not supported yet!\n");

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_bt 停止scan接收
 * @return int
 */
int tuya_os_adapt_bt_stop_scan(void)
{
    //todo
    puts("tuya_os_adapt_bt_stop_scan : Not supported yet!\n");
    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief  get Bluetooth capability
 *
 * @param
 * @return OPERATE_RET
 */
OPERATE_RET tuya_os_adapt_bt_get_ability(VOID_T)
{
    //todo
    puts("tuya_os_adapt_bt_get_ability : Not supported yet!\n");
    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief tuya_os_adapt_reg_bt_intf 接口注册
 * @return int
 */
int tuya_os_adapt_reg_bt_intf(void)
{
    return tuya_os_adapt_reg_intf(INTF_BT, (void *)&m_tuya_os_bt_intfs);
}

/*********************************************************************************/
void tuya_ble_write_callback(unsigned char *buf, unsigned short buf_size)
{
    if (NULL == buf) {
        printf("tuya_ble_write_callback : PARAM ERROR!\n");
        return;
    }

    tuya_ble_data_buf_t data;
    data.data = buf;
    data.len = buf_size;

    if (NULL != ty_bt_msg_cb) {
        ty_bt_msg_cb(0, TY_BT_EVENT_RX_DATA, &data);
    }
}

void tuya_bt_state_connected(void)
{
    puts("tuya_bt_state_connected");
    if (NULL != ty_bt_msg_cb) {
        ty_bt_msg_cb(0, TY_BT_EVENT_CONNECTED, NULL);
    }
}

void tuya_bt_state_disconnected(void)
{
    puts("tuya_bt_state_disconnected");
    if (NULL != ty_bt_msg_cb) {
        ty_bt_msg_cb(0, TY_BT_EVENT_DISCONNECTED, NULL);
    }
}
