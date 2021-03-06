/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adaptation.h"
#include "net/buf.h"
#include "mesh_net.h"
#include "adv.h"
#include "prov.h"
#include "beacon.h"
#include "lpn.h"
#include "friend.h"
#include "mesh_transport.h"
#include "access.h"
#include "foundation.h"
#include "proxy.h"
#include "mesh_settings.h"
#include "mesh.h"

#define LOG_TAG             "[MESH-main]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#include "mesh_log.h"

#if MESH_RAM_AND_CODE_MAP_DETAIL
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_mesh_main_bss")
#pragma data_seg(".ble_mesh_main_data")
#pragma const_seg(".ble_mesh_main_const")
#pragma code_seg(".ble_mesh_main_code")
#endif
#else /* MESH_RAM_AND_CODE_MAP_DETAIL */
#pragma bss_seg(".ble_mesh_bss")
#pragma data_seg(".ble_mesh_data")
#pragma const_seg(".ble_mesh_const")
#pragma code_seg(".ble_mesh_code")
#endif /* MESH_RAM_AND_CODE_MAP_DETAIL */

int bt_mesh_provision(const u8_t net_key[16], u16_t net_idx,
                      u8_t flags, u32_t iv_index, u16_t addr,
                      const u8_t dev_key[16])
{
    int err;

    BT_INFO("--func=%s", __FUNCTION__);
    BT_INFO("Primary Element: 0x%04x", addr);
    BT_DBG("net_idx 0x%04x flags 0x%02x iv_index 0x%04x",
           net_idx, flags, iv_index);

    if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
        bt_mesh_proxy_prov_disable(false);
    }

    /* create 'Secure Network beacon' PDU of 'Mesh beacons' */
    err = bt_mesh_net_create(net_idx, flags, net_key, iv_index);
    if (err) {
        if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
            bt_mesh_proxy_prov_enable();
        }

        return err;
    }

    bt_mesh.seq = 0;

    bt_mesh_comp_provision(addr);

    memcpy(bt_mesh.dev_key, dev_key, 16);

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        BT_DBG("Storing network information persistently");
        bt_mesh_store_net();
        bt_mesh_store_subnet(&bt_mesh.sub[0]);
        bt_mesh_store_iv(false);
    }

    bt_mesh_net_start();

    return 0;
}

void bt_mesh_reset(void)
{
    if (!bt_mesh.valid) {
        return;
    }

    bt_mesh.iv_index = 0;
    bt_mesh.seq = 0;
    bt_mesh.iv_update = 0;
    bt_mesh.pending_update = 0;
    bt_mesh.valid = 0;
    bt_mesh.ivu_duration = 0;
    bt_mesh.ivu_initiator = 0;

    k_delayed_work_cancel(&bt_mesh.ivu_timer);

    bt_mesh_cfg_reset();

    bt_mesh_rx_reset();
    bt_mesh_tx_reset();

    if (IS_ENABLED(CONFIG_BT_MESH_LOW_POWER)) {
        bt_mesh_lpn_disable(true);
    }

    if (IS_ENABLED(CONFIG_BT_MESH_FRIEND)) {
        bt_mesh_friend_clear_net_idx(BT_MESH_KEY_ANY);
    }

    if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY)) {
        bt_mesh_proxy_gatt_disable();
    }

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        bt_mesh_clear_net();
    }

    (void)memset(bt_mesh.dev_key, 0, sizeof(bt_mesh.dev_key));

    bt_mesh_scan_disable();
    bt_mesh_beacon_disable();

    bt_mesh_comp_unprovision();

    if (IS_ENABLED(CONFIG_BT_MESH_PROV)) {
        bt_mesh_prov_reset();
    }
}

bool bt_mesh_is_provisioned(void)
{
    return bt_mesh.valid;
}

int bt_mesh_prov_enable(bt_mesh_prov_bearer_t bearers)
{
    if (bt_mesh_is_provisioned()) {
        return -EALREADY;
    }

    if (IS_ENABLED(CONFIG_BT_DEBUG)) {
        const struct bt_mesh_prov *prov = bt_mesh_prov_get();
        struct bt_uuid_128 uuid = { .uuid.type = BT_UUID_TYPE_128 };

        memcpy(uuid.val, prov->uuid, 16);
        BT_INFO("Device UUID: %s", bt_uuid_str(&uuid.uuid));
    }

    if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV) &&
        (bearers & BT_MESH_PROV_ADV)) {
        /* Make sure we're scanning for provisioning inviations */
        bt_mesh_scan_enable();
        /* Enable unprovisioned beacon sending */
        if (bt_mesh_beacon_get() == BT_MESH_BEACON_ENABLED) {
            bt_mesh_beacon_enable();
        }
    }

    if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
        (bearers & BT_MESH_PROV_GATT)) {
        bt_mesh_proxy_prov_enable();
        bt_mesh_adv_update();
    }

    return 0;
}

int bt_mesh_prov_disable(bt_mesh_prov_bearer_t bearers)
{
    if (bt_mesh_is_provisioned()) {
        return -EALREADY;
    }

    if (IS_ENABLED(CONFIG_BT_MESH_PB_ADV) &&
        (bearers & BT_MESH_PROV_ADV)) {
        bt_mesh_beacon_disable();
        bt_mesh_scan_disable();
    }

    if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
        (bearers & BT_MESH_PROV_GATT)) {
        bt_mesh_proxy_prov_disable(true);
        bt_mesh_adv_update();
    }

    return 0;
}

int bt_mesh_init(const struct bt_mesh_prov *prov,
                 const struct bt_mesh_comp *comp)
{
    int err;

    err = bt_mesh_comp_register(comp);
    if (err) {
        return err;
    }

    if (IS_ENABLED(CONFIG_BT_MESH_PROV)) {
        err = bt_mesh_prov_init(prov);
        if (err) {
            return err;
        }
    }

    bt_mesh_net_init();
    bt_mesh_trans_init();
    bt_mesh_beacon_init();
    bt_mesh_adv_init();

    if (IS_ENABLED(CONFIG_BT_MESH_PROXY)) {
        bt_mesh_proxy_init();
    }

    if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
        bt_mesh_settings_init();
    }

    return 0;
}
