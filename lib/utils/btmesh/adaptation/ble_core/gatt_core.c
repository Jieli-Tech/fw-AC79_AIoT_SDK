/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adaptation.h"
#include "proxy.h"
#include "prov.h"
#include "ble/hci_ll.h"
#include "btstack/bluetooth.h"

#define LOG_TAG             "[MESH-gatt_core]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#include "mesh_log.h"

#if ADAPTATION_COMPILE_DEBUG

u8 *get_server_data_addr(void)
{
    return NULL;
}

void bt_gatt_service_register(u32 uuid) {}

void bt_gatt_service_unregister(u32 uuid) {}

int bt_gatt_notify(struct bt_conn *conn, const void *data, u16_t len)
{
    return 0;
}

u16 bt_gatt_get_mtu(struct bt_conn *conn)
{
    return 0;
}

void bt_conn_disconnect(struct bt_conn *conn, u8 reason) {}

void proxy_gatt_init(void) {}

#else /* ADAPTATION_COMPILE_DEBUG */

#define BYTE_LEN(x...)                      sizeof((u8 []) {x})

/** @brief Helper to declare elements of bt_data arrays
 *
 *  This macro is mainly for creating an array of struct bt_data
 *  elements which is then passed to bt_le_adv_start().
 *
 *  @param _type Type of advertising data field
 *  @param _data Pointer to the data field payload
 *  @param _data_len Number of bytes behind the _data pointer
 */
#define BT_DATA(_type, _data, _data_len) \
	{ \
		.type = (_type), \
		.data_len = (_data_len), \
		.data = (u8_t *)(_data), \
	}

/** @brief Helper to declare elements of bt_data arrays
 *
 *  This macro is mainly for creating an array of struct bt_data
 *  elements which is then passed to bt_le_adv_start().
 *
 *  @param _type Type of advertising data field
 *  @param _bytes Variable number of single-byte parameters
 */
#define BT_DATA_BYTES(_type, _bytes...) \
	BT_DATA(_type, ((u8_t []) { _bytes }), \
		sizeof((u8_t []) { _bytes }))

#define NODE_ID_LEN  19

#define BT_DATA_FLAGS                   0x01 /* AD flags */
#define BT_DATA_UUID16_ALL              0x03 /* 16-bit UUID, all listed */
#define BT_DATA_SVC_DATA16              0x16 /* Service data, 16-bit UUID */
#define BT_LE_AD_GENERAL                0x02 /* General Discoverable */
#define BT_LE_AD_NO_BREDR               0x04 /* BR/EDR not supported */

/** Description of different data types that can be encoded into
  * advertising data. Used to form arrays that are passed to the
  * bt_le_adv_start() function.
  */
struct bt_data {
    u8_t type;
    u8_t data_len;
    const u8_t *data;
};

static u8_t proxy_svc_data[NODE_ID_LEN] = { 0x28, 0x18, };

static const struct bt_data node_id_ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0x28, 0x18),
    BT_DATA(BT_DATA_SVC_DATA16, proxy_svc_data, NODE_ID_LEN),
};

#define UUID_16BIT_2_LSB_8BIT(x)            x & 0xff, x >> 8

#define UUID_MESH_PROVISIONING_SERVICE  UUID_16BIT_2_LSB_8BIT(BT_UUID_MESH_PROV)
#define UUID_MESH_PROVISIONING_DATA_IN  UUID_16BIT_2_LSB_8BIT(BT_UUID_MESH_PROV_DATA_IN)
#define UUID_MESH_PROVISIONING_DATA_OUT UUID_16BIT_2_LSB_8BIT(BT_UUID_MESH_PROV_DATA_OUT)
#define UUID_MESH_PROXY_SERVICE  		UUID_16BIT_2_LSB_8BIT(BT_UUID_MESH_PROXY)
#define UUID_MESH_PROXY_DATA_IN  		UUID_16BIT_2_LSB_8BIT(BT_UUID_MESH_PROXY_DATA_IN)
#define UUID_MESH_PROXY_DATA_OUT 		UUID_16BIT_2_LSB_8BIT(BT_UUID_MESH_PROXY_DATA_OUT)

#define MESH_DATA_IN_HANDLE             10
#define MESH_DATA_OUT_HANDLE            12
#define MESH_PROV_CONFIG_HANDLE         13
#define MESH_PROXY_CONFIG_HANDLE        13

#define APP_BT_MAC_ADDR                 0x11, 0x22, 0x33, 0x44, 0x55, 0x66

// Complete Local Name
#define BLE_DEV_NAME            'J', 'L', '_','M', 'E', 'S', 'H'
#define BLE_DEV_NAME_LEN        BYTE_LEN(BLE_DEV_NAME)

/*
 * Mesh_v1.0 7.1 Mesh Provisioning Service
 */
#define Provisioning_Service(x) provisioning_service_##x

static u8 Provisioning_Service(adv_data)[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,

    0x03, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS, UUID_MESH_PROVISIONING_SERVICE,

    0x15, BLUETOOTH_DATA_TYPE_SERVICE_DATA_16_BIT_UUID, UUID_MESH_PROVISIONING_SERVICE, APP_BT_MAC_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8 Provisioning_Service(profile_data)[] = {
//  len-------------------------Properties----  Handle--------------------------UUID16----------Value------------------------------------
    ///--Primary Service Declaration:Generic Access
    10, 0x00,                    0x02, 0x00,    0x01, 0x00,                     0x00, 0x28,     0x00, 0x18,                                     //primary service declaration
    //< characteristic declaration:Device Name
    13, 0x00,                    0x02, 0x00,    0x02, 0x00,                     0x03, 0x28,     0x02, 0x03, 0x00, 0x00, 0x2A,                   //characteristic declaration
    8 + BLE_DEV_NAME_LEN, 0x00,  0x02, 0x00,    0x03, 0x00,                     0x00, 0x2A,     BLE_DEV_NAME,                                   //device name

    ///--Primary Service Declaration:Generic Attribute
    10, 0x00,                    0x02, 0x00,    0x04, 0x00,                     0x00, 0x28,     0x01, 0x18,                                     //primary service declaration
    //< characteristic declaration:Service Changed
    13, 0x00,                    0x02, 0x00,    0x05, 0x00,                     0x03, 0x28,     0x20, 0x06, 0x00, 0x05, 0x2A,                   //characteristic declaration
    8, 0x00,                     0x20, 0x00,    0x06, 0x00,                     0x05, 0x2A,                                                     //service changed
    10, 0x00,                    0x0A, 0x01,    0x07, 0x00,                     0x02, 0x29,     0x00, 0x00,                                     //client characteristic configuration

    ///--Primary Service Declaration:Mesh Provisioning Service
    10, 0x00,                    0x02, 0x00,    0x08, 0x00,                     0x00, 0x28,     UUID_MESH_PROVISIONING_SERVICE,
    //< characteristic declaration
    13, 0x00,                    0x02, 0x00,    0x09, 0x00,                     0x03, 0x28,     0x04, MESH_DATA_IN_HANDLE, 0x00, UUID_MESH_PROVISIONING_DATA_IN,
    8, 0x00,                     0x04, 0x01,    MESH_DATA_IN_HANDLE, 0x00,      UUID_MESH_PROVISIONING_DATA_IN,
    //< characteristic declaration
    13, 0x00,                    0x02, 0x00,    0x0b, 0x00,                     0x03, 0x28,     0x10, MESH_DATA_OUT_HANDLE, 0x00, UUID_MESH_PROVISIONING_DATA_OUT,
    8, 0x00,                     0x10, 0x00,    MESH_DATA_OUT_HANDLE, 0x00,     UUID_MESH_PROVISIONING_DATA_OUT,
    //< client characteristic configuration
    10, 0x00,                    0x0A, 0x01,    MESH_PROV_CONFIG_HANDLE, 0x00,  0x02, 0x29,     0x00, 0x00,

    // END
    0x00, 0x00,
};

/*
 * Mesh_v1.0 7.2 Mesh Proxy Service
 */
#define Proxy_Service(x) proxy_service_##x

static const uint8_t Proxy_Service(profile_data)[] = {
//  len-------------------------Properties----  Handle--------------------------UUID16----------Value------------------------------------
    ///--Primary Service Declaration:Generic Access
    10, 0x00,                    0x02, 0x00,    0x01, 0x00,                     0x00, 0x28,     0x00, 0x18,                                     //primary service declaration
    //< characteristic declaration:Device Name
    13, 0x00,                    0x02, 0x00,    0x02, 0x00,                     0x03, 0x28,     0x02, 0x03, 0x00, 0x00, 0x2A,                   //characteristic declaration
    8 + BLE_DEV_NAME_LEN, 0x00,  0x02, 0x00,    0x03, 0x00,                     0x00, 0x2A,     BLE_DEV_NAME,                                   //device name

    ///--Primary Service Declaration:Generic Attribute
    10, 0x00,                    0x02, 0x00,    0x04, 0x00,                     0x00, 0x28,     0x01, 0x18,                                     //primary service declaration
    //< characteristic declaration:Service Changed
    13, 0x00,                    0x02, 0x00,    0x05, 0x00,                     0x03, 0x28,     0x20, 0x06, 0x00, 0x05, 0x2A,                   //characteristic declaration
    8, 0x00,                     0x20, 0x00,    0x06, 0x00,                     0x05, 0x2A,                                                     //service changed
    10, 0x00,                    0x0A, 0x01,    0x07, 0x00,                     0x02, 0x29,     0x00, 0x00,                                     //client characteristic configuration

    ///--Primary Service Declaration:Mesh Provisioning Service
    10, 0x00,                    0x02, 0x00,    0x08, 0x00,                     0x00, 0x28,     UUID_MESH_PROXY_SERVICE,
    //< characteristic declaration
    13, 0x00,                    0x02, 0x00,    0x09, 0x00,                     0x03, 0x28,     0x04, MESH_DATA_IN_HANDLE, 0x00, UUID_MESH_PROXY_DATA_IN,
    8, 0x00,                     0x04, 0x01,    MESH_DATA_IN_HANDLE, 0x00,      UUID_MESH_PROXY_DATA_IN,
    //< characteristic declaration
    13, 0x00,                    0x02, 0x00,    0x0b, 0x00,                     0x03, 0x28,     0x10, MESH_DATA_OUT_HANDLE, 0x00, UUID_MESH_PROXY_DATA_OUT,
    8, 0x00,                     0x10, 0x00,    MESH_DATA_OUT_HANDLE, 0x00,     UUID_MESH_PROXY_DATA_OUT,
    //< client characteristic configuration
    10, 0x00,                    0x0A, 0x01,    MESH_PROXY_CONFIG_HANDLE, 0x00, 0x02, 0x29,     0x00, 0x00,

    // END
    0x00, 0x00,
};

static u8 mesh_gatt_buf[0x100];

extern struct bt_conn bt_mesh_conn;
extern int mesh_gatt_notify(u16 conn_handle, u16 att_handle, const void *data, u16_t len);
extern void mesh_gatt_set_callback(void *read_cb, void *write_cb);
extern void mesh_gatt_change_profile(void *data);
extern void mesh_gatt_init(u8 *buf, u16 len);
extern bool mesh_adv_send_timer_busy(void);
extern void ble_set_adv_param(u16 interval_min, u16 interval_max, u8 type, u8 direct_addr_type, u8 *direct_addr,
                              u8 channel_map, u8 filter_policy);
extern void ble_set_adv_data(u8 data_length, u8 *data);
extern void ble_set_scan_rsp_data(u8 data_length, u8 *data);
extern void get_mesh_adv_name(u8 *len, u8 **data);
extern int le_controller_get_mac(void *addr);

static void get_prov_properties_capabilities(u8 *adv_data)
{
    const struct bt_mesh_prov *prov;
    u8 offset = 11;

    prov = bt_mesh_prov_get();

    if (prov == NULL) {
        BT_ERR("get prov is NULL");
        return;
    }

    memcpy(adv_data + offset, (u8 *)prov->uuid, 16);

    offset += 16;

    memcpy(adv_data + offset, (u8 *)&prov->oob_info, 2);
}

static void Provisioning_Service(change_adv_info)(void)
{
    if (TRUE == mesh_adv_send_timer_busy()) {
        return;
    }

    // setup proxy connectable advertisements
    u16 adv_int = config_bt_mesh_proxy_unprovision_adv_interval;
    u8 adv_type = 0;
    u8 null_addr[6] = {0};
    u8 rsp_len;
    u8 *rsp_data;

    ble_set_adv_param(adv_int, adv_int, adv_type, 0, null_addr, 0x07, 0x00);

    // add user data
    get_prov_properties_capabilities(Provisioning_Service(adv_data));

    ble_set_adv_data(sizeof(Provisioning_Service(adv_data)), (uint8_t *)Provisioning_Service(adv_data));

    get_mesh_adv_name(&rsp_len, &rsp_data);

    ble_set_scan_rsp_data(rsp_len, rsp_data);
}

static void Proxy_Service(change_adv_info)(u16 interval_ms)
{
    if (TRUE == mesh_adv_send_timer_busy()) {
        return;
    }

    // setup proxy connectable advertisements
    u16 adv_int = interval_ms;
    u8 adv_type = 0;
    u8 null_addr[6];
    u8 data_len = 0;
    static u8 adv_data[31];
    u8 rsp_len;
    u8 *rsp_data;

    for (u8 i = 0; i < ARRAY_SIZE(node_id_ad); i++) {
        adv_data[data_len++] = node_id_ad[i].data_len + 1;
        adv_data[data_len++] = node_id_ad[i].type;
        memcpy(adv_data + data_len, node_id_ad[i].data, node_id_ad[i].data_len);
        data_len += node_id_ad[i].data_len;
    }

    BT_INFO("--func=%s", __FUNCTION__);
    memset(null_addr, 0, 6);
    BT_INFO_HEXDUMP(proxy_svc_data, NODE_ID_LEN);
    ble_set_adv_param(adv_int, adv_int, adv_type, 0, null_addr, 0x07, 0x00);
    ble_set_adv_data(data_len, (uint8_t *)adv_data);
    get_mesh_adv_name(&rsp_len, &rsp_data);
    ble_set_scan_rsp_data(rsp_len, rsp_data);
}

u8 *get_server_data_addr(void)
{
    return proxy_svc_data;
}

static u16 gatt_read_callback(u16 conn_handle, u16 att_handle, u16 offset, u8 *buf, u16 buf_len)
{
    BT_INFO("read att_handle %04x", att_handle);
    BT_INFO_HEXDUMP(buf, buf_len);

    return 0;
}

static int gatt_write_callback(u16 conn_handle, u16 att_handle, u16 mode, u16 offset, u8 *buf, u16 buf_len)
{
    BT_INFO("write att_handle 0x%04x, mode %u", att_handle, mode);
    BT_INFO_HEXDUMP(buf, buf_len);

    if (FALSE == bt_mesh_is_provisioned()) {
        if (MESH_PROV_CONFIG_HANDLE == att_handle) {
            BT_INFO("MESH_PROV_CONFIG_HANDLE");
            prov_ccc_write(&bt_mesh_conn,
                           buf, buf_len,
                           offset, 0);
            return 0;
        }
    }

    switch (att_handle) {
    case MESH_PROXY_CONFIG_HANDLE:
        BT_INFO("MESH_PROXY_CONFIG_HANDLE");
        proxy_ccc_write(&bt_mesh_conn,
                        buf, buf_len,
                        offset, 0);
        break;
    case MESH_DATA_IN_HANDLE:
        BT_INFO("MESH_DATA_IN_HANDLE");
        proxy_recv(&bt_mesh_conn,
                   buf,
                   buf_len, offset, 0);
        break;
    }

    return 0;
}

void bt_gatt_service_register(u32 uuid)
{
    BT_INFO("--func=%s", __FUNCTION__);

    //< setup GATT callback
    mesh_gatt_set_callback(gatt_read_callback, gatt_write_callback);

    switch (uuid) {
    case BT_UUID_MESH_PROV:
        BT_INFO("BT_UUID_MESH_PROV");
        mesh_gatt_change_profile((void *)Provisioning_Service(profile_data));
        Provisioning_Service(change_adv_info)();
        break;
    case BT_UUID_MESH_PROXY:
        BT_INFO("BT_UUID_MESH_PROXY");
        mesh_gatt_change_profile((void *)Proxy_Service(profile_data));
        break;
    default :
        break;
    }
}

void bt_gatt_service_unregister(u32 uuid)
{
    BT_INFO("--func=%s", __FUNCTION__);

    mesh_gatt_set_callback(NULL, NULL);
}

int bt_gatt_notify(struct bt_conn *conn, const void *data, u16_t len)
{
    BT_INFO("--func=%s", __FUNCTION__);

    return mesh_gatt_notify(conn->handle, MESH_DATA_OUT_HANDLE, data, len);
}

u16 bt_gatt_get_mtu(struct bt_conn *conn)
{
    return conn->mtu;
}

static u8 ble_disconnect(u16 handle)
{
#if CMD_DIRECT_TO_BTCTRLER_TASK_EN
    ll_hci_disconnect(handle, 0x13);
#else
    ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, handle);
#endif /* CMD_DIRECT_TO_BTCTRLER_TASK_EN */

    return 0;
}

void bt_conn_disconnect(struct bt_conn *conn, u8 reason)
{
    BT_INFO("--func=%s", __FUNCTION__);
    BT_INFO("conn->handle=0x%x", conn->handle);

    ble_disconnect(conn->handle);
}

void unprovision_connectable_adv(void)
{
    BT_INFO(RedBoldBlink "--func=%s" Reset, __FUNCTION__);
    Provisioning_Service(change_adv_info)();
}

void proxy_connectable_adv(void)
{
    BT_INFO(RedBoldBlink "--func=%s" Reset, __FUNCTION__);
    Proxy_Service(change_adv_info)(config_bt_mesh_proxy_node_adv_interval);
}

void proxy_fast_connectable_adv(void)
{
    BT_INFO(RedBoldBlink "--func=%s" Reset, __FUNCTION__);
    Proxy_Service(change_adv_info)(config_bt_mesh_proxy_pre_node_adv_interval);
}

void proxy_gatt_init(void)
{
    BT_INFO("--func=%s", __FUNCTION__);

    mesh_gatt_init(mesh_gatt_buf, sizeof(mesh_gatt_buf));
}

#endif /* ADAPTATION_COMPILE_DEBUG */
