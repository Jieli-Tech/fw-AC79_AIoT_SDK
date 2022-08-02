/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adaptation.h"
#include "ble/hci_ll.h"
#include "btstack/bluetooth.h"

#define LOG_TAG             "[MESH-scan_core]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#include "mesh_log.h"


#if ADAPTATION_COMPILE_DEBUG

int bt_le_scan_start(bt_le_scan_cb_t cb)
{
    return 0;
}

int bt_le_scan_stop(void)
{
    return 0;
}

#else /* ADAPTATION_COMPILE_DEBUG */

/* Window and Interval are equal for continuous scanning */
#define MESH_SCAN_INTERVAL_MS 10
#define MESH_SCAN_WINDOW_MS   10
#define MESH_SCAN_INTERVAL    ADV_SCAN_UNIT(MESH_SCAN_INTERVAL_MS)
#define MESH_SCAN_WINDOW      ADV_SCAN_UNIT(MESH_SCAN_WINDOW_MS)

/* Scan types */
#define BT_HCI_LE_SCAN_PASSIVE                  0x00
#define BT_HCI_LE_SCAN_ACTIVE                   0x01
#define BT_HCI_LE_SCAN_FILTER_DUP_DISABLE       0x00
#define BT_HCI_LE_SCAN_FILTER_DUP_ENABLE        0x01

#define CUR_DEBUG_IO_0(i,x)         //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define CUR_DEBUG_IO_1(i,x)         //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

#define SCAN_HW_START()             CUR_DEBUG_IO_1(A, 4)
#define SCAN_HW_END()               CUR_DEBUG_IO_0(A, 4)

/** LE scan parameters */
struct bt_le_scan_param {
    /** Scan type (BT_HCI_LE_SCAN_ACTIVE or BT_HCI_LE_SCAN_PASSIVE) */
    u8_t  type;

    /** Duplicate filtering (BT_HCI_LE_SCAN_FILTER_DUP_ENABLE or
     *  BT_HCI_LE_SCAN_FILTER_DUP_DISABLE)
     */
    u8_t  filter_dup;

    /** Scan interval (N * 0.625 ms) */
    u16_t interval;

    /** Scan window (N * 0.625 ms) */
    u16_t window;
};

static bt_le_scan_cb_t *scan_dev_found_cb;


static void ble_set_scan_param(u8 scan_type, u16 scan_interval, u16 scan_window)
{
#if CMD_DIRECT_TO_BTCTRLER_TASK_EN
    ll_hci_scan_set_params(scan_type, scan_interval, scan_window);
#else
    ble_user_cmd_prepare(BLE_CMD_SCAN_PARAM, 3, scan_type, scan_interval, scan_window);
#endif /* CMD_DIRECT_TO_BTCTRLER_TASK_EN */
}

void ble_set_scan_enable(bool en)
{
    SCAN_HW_START();

#if CMD_DIRECT_TO_BTCTRLER_TASK_EN
    ll_hci_scan_enable(en, 1);
#else
    ble_user_cmd_prepare(BLE_CMD_SCAN_ENABLE, 1, en);
#endif /* CMD_DIRECT_TO_BTCTRLER_TASK_EN */
}

void handle_scan_callback(uint8_t *packet, uint16_t size)
{
    bt_addr_le_t addr;
    s8_t rssi;
    u8_t adv_type;
    struct net_buf_simple buf = {
        .data = &packet[12],
        .len = packet[11],
        .__buf = buf.data,
    };

    /* BT_INFO("--func=%s", __FUNCTION__); */
    /* BT_INFO_HEXDUMP(packet, size); */

    reverse_bytes(&packet[4], addr.a.val, 6);

    addr.type = packet[3];

    rssi = packet[10];

    adv_type = packet[2];

    scan_dev_found_cb(&addr, rssi, adv_type, &buf);
}

int bt_le_scan_start(bt_le_scan_cb_t cb)
{
    struct bt_le_scan_param scan_param = {
        .type       = BT_HCI_LE_SCAN_PASSIVE,
        .filter_dup = BT_HCI_LE_SCAN_FILTER_DUP_DISABLE,
        .interval   = MESH_SCAN_INTERVAL,
        .window     = MESH_SCAN_WINDOW
    };

    scan_dev_found_cb = cb;

    BT_INFO("--func=%s", __FUNCTION__);

    ble_set_scan_param(scan_param.type, scan_param.interval, scan_param.window);

    ble_set_scan_enable(1);

    return 0;
}

int bt_le_scan_stop(void)
{
    BT_INFO("--func=%s", __FUNCTION__);

    ble_set_scan_enable(0);

    return 0;
}

#endif /* ADAPTATION_COMPILE_DEBUG */
