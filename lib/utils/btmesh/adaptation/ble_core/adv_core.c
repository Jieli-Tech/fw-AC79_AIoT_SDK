/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adaptation.h"
#include "net/buf.h"
#include "adv.h"
#include "timer.h"
#include "ble/hci_ll.h"
#include "btstack/bluetooth.h"

#define LOG_TAG             "[MESH-adv_core]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#include "mesh_log.h"

#if ADAPTATION_COMPILE_DEBUG

void bt_mesh_adv_update(void) {}

void bt_mesh_adv_send(struct net_buf *buf, const struct bt_mesh_send_cb *cb, void *cb_data) {}

void bt_mesh_adv_init(void) {}

#else /* ADAPTATION_COMPILE_DEBUG */

#define MESH_ADV_SEND_USE_HI_TIMER          1

#if MESH_ADV_SEND_USE_HI_TIMER

#define sys_timer_add               sys_hi_timer_add
#define sys_timer_change_period     sys_hi_timer_modify
#define sys_timer_remove            sys_hi_timer_del

/* #undef BT_INFO */
/* #define BT_INFO */

#endif /* MESH_ADV_SEND_USE_HI_TIMER */

#define CUR_DEBUG_IO_0(i,x)         //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define CUR_DEBUG_IO_1(i,x)         //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

#define ADV_HW_START()              CUR_DEBUG_IO_1(A, 2)
#define ADV_HW_END()                CUR_DEBUG_IO_0(A, 2)

#define ADV_INT_FAST_MS         10
#define ADV_INT_DEFAULT_MS      100

#define USER_ADV_SEND_INTERVAL \
    (bt_mesh_is_provisioned()? config_bt_mesh_node_msg_adv_interval : config_bt_mesh_pb_adv_interval)
#define USER_ADV_SEND_DURATION \
    (bt_mesh_is_provisioned()? config_bt_mesh_node_msg_adv_duration : config_bt_mesh_pb_adv_duration)

static const u8_t adv_type[] = {
    [BT_MESH_ADV_PROV]   = BT_DATA_MESH_PROV,
    [BT_MESH_ADV_DATA]   = BT_DATA_MESH_MESSAGE,
    [BT_MESH_ADV_BEACON] = BT_DATA_MESH_BEACON,
    [BT_MESH_ADV_URI]    = BT_DATA_URI,
};

static sys_timer mesh_AdvSend_timer;

static sys_slist_t adv_list = {
    .head = NULL,
    .tail = NULL,
};

static void ble_adv_enable(bool en);
static bool adv_send(struct net_buf *buf);
static void fresh_adv_info(struct net_buf *buf);
extern void resume_mesh_gatt_proxy_adv_thread(void);
extern void unprovision_connectable_adv(void);
extern void proxy_connectable_adv(void);
extern void proxy_fast_connectable_adv(void);
extern int bt_mesh_get_if_app_key_add(void);
extern bool get_if_connecting(void);
extern void bt_mesh_adv_buf_alloc(void);
extern void ble_set_scan_enable(bool en);

static u16 mesh_adv_send_start(void *param)
{
    struct net_buf *buf = param;

    if (BT_MESH_ADV(buf)->delay) {
        BT_MESH_ADV(buf)->delay = 0;
        fresh_adv_info(buf);

        return USER_ADV_SEND_DURATION;
    }

    return 0;
}

static void mesh_adv_send_end(void *param)
{
    struct net_buf *buf = param;
    const struct bt_mesh_send_cb *cb = BT_MESH_ADV(buf)->cb;
    void *cb_data = BT_MESH_ADV(buf)->cb_data;

    ble_adv_enable(0);
    /* ble_set_scan_enable(1); */

    if (cb && cb->end) {
        cb->end(0, cb_data);
    }

    BT_MESH_ADV(buf)->busy = 0;

    net_buf_unref(buf);

    BT_INFO("adv_list.head=0x%x", adv_list.head);
    BT_INFO("adv_list.tail=0x%x", adv_list.tail);

    buf = net_buf_slist_simple_get(&adv_list);

    BT_INFO("adv_list.head=0x%x", adv_list.head);
    BT_INFO("adv_list.tail=0x%x", adv_list.tail);

    if (buf && BT_MESH_ADV(buf) && BT_MESH_ADV(buf)->busy) {
        bool send_busy = adv_send(buf);

        BT_INFO("adv_list.head=0x%x", adv_list.head);
        BT_INFO("adv_list.tail=0x%x", adv_list.tail);
        BT_DBG("adv_send %s", send_busy ? "busy" : "succ");
    } else {
        resume_mesh_gatt_proxy_adv_thread();
    }
}

static void mesh_adv_timer_handler(void *param)
{
    u16 duration;

    BT_DBG("TO - adv_timer_cb 0x%x", param);

    if (NULL == param) {
        BT_ERR("param is NULL");
        return;
    }

    duration = mesh_adv_send_start(param);

    if (duration) {
        sys_timer_change_period(mesh_AdvSend_timer, duration);
        BT_INFO("start duration= %dms", duration);
    } else {
        sys_timer_remove(mesh_AdvSend_timer);
        BT_INFO("remove id =%d", mesh_AdvSend_timer);
        mesh_AdvSend_timer = 0;
        mesh_adv_send_end(param);
    }

    BT_DBG("adv_t_cb end");
}

static void mesh_adv_timeout_start(u16 delay, u16 duration, void *param)
{
    if (0 == mesh_AdvSend_timer) {
        /* mesh_AdvSend_timer = sys_timer_register(delay? delay : duration, mesh_adv_timer_handler); */
        /* sys_timer_set_context(mesh_AdvSend_timer, param); */
        mesh_AdvSend_timer = sys_timer_add(param, mesh_adv_timer_handler, delay ? delay : duration);
        BT_INFO("mesh_AdvSend_timer id = %d, %s= %dms",
                mesh_AdvSend_timer,
                delay ? "delay first" : "only duration",
                delay ? delay : duration);
        BT_INFO("param addr=0x%x", param);
    }

    ASSERT(mesh_adv_timer_handler);
}

bool mesh_adv_send_timer_busy(void)
{
    return !!mesh_AdvSend_timer;
}

void ble_set_adv_param(u16 interval_min, u16 interval_max, u8 type, u8 direct_addr_type, u8 *direct_addr,
                       u8 channel_map, u8 filter_policy)
{
#if CMD_DIRECT_TO_BTCTRLER_TASK_EN
    ll_hci_adv_set_params(interval_min, interval_max, type, direct_addr_type, direct_addr,
                          channel_map, filter_policy);
#else
    ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, interval_min, type, channel_map);
#endif /* CMD_DIRECT_TO_BTCTRLER_TASK_EN */
}

void ble_set_adv_data(u8 data_length, u8 *data)
{
#if CMD_DIRECT_TO_BTCTRLER_TASK_EN
    ll_hci_adv_set_data(data_length, data);
#else
    ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, data_length, data);
#endif /* CMD_DIRECT_TO_BTCTRLER_TASK_EN */
}

void ble_set_scan_rsp_data(u8 data_length, u8 *data)
{
#if CMD_DIRECT_TO_BTCTRLER_TASK_EN
    ll_hci_adv_scan_response_set_data(data_length, data);
#else
    ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, data_length, data);
#endif /* CMD_DIRECT_TO_BTCTRLER_TASK_EN */
}

static void ble_adv_enable(bool en)
{
    if (en) {
        ADV_HW_START();
    } else {
        ADV_HW_END();
    }

#if CMD_DIRECT_TO_BTCTRLER_TASK_EN
    ll_hci_adv_enable(en);
#else
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, en);
#endif /* CMD_DIRECT_TO_BTCTRLER_TASK_EN */
}

static void fresh_adv_info(struct net_buf *buf)
{
    u8 direct_addr[6] = {0};
    u16 adv_interval;
    struct advertising_data_header *adv_data_head;
    u16 total_adv_data_len;

#if 0
    const s32_t adv_int_min = ADV_INT_FAST_MS;
    adv_interval = max(adv_int_min, BT_MESH_TRANSMIT_INT(BT_MESH_ADV(buf)->xmit));
    adv_interval = ADV_SCAN_UNIT(adv_interval);
#else
    adv_interval = USER_ADV_SEND_INTERVAL;
#endif

    adv_data_head = (struct advertising_data_header *)(buf->data - BT_MESH_ADV_DATA_HEAD_SIZE);
    adv_data_head->Len = buf->len + 1;
    adv_data_head->Type = adv_type[BT_MESH_ADV(buf)->type];
    total_adv_data_len = buf->len + BT_MESH_ADV_DATA_HEAD_SIZE;

    /* ble_set_scan_enable(0); */
    ble_adv_enable(0);
    ble_set_adv_param(adv_interval, adv_interval, 0x03, 0, direct_addr, 0x07, 0x00);
    ble_set_adv_data(total_adv_data_len, (u8 *)adv_data_head);
    ble_adv_enable(1);
}

static bool adv_send(struct net_buf *buf)
{
    BT_INFO("--func=%s", __FUNCTION__);
    BT_INFO("entry_node addr=0x%x", &buf->entry_node);

    buf->entry_node.next = 0;

    local_irq_disable();

    if (TRUE == mesh_adv_send_timer_busy()) {

        net_buf_slist_simple_put(&adv_list, &buf->entry_node);

        local_irq_enable();

        return 1;
    }

    local_irq_enable();

    const struct bt_mesh_send_cb *cb = BT_MESH_ADV(buf)->cb;
    void *cb_data = BT_MESH_ADV(buf)->cb_data;
    u16 delay = 0;
    u16 duration;

    /* duration = (MESH_SCAN_WINDOW_MS + */
    /*             ((BT_MESH_TRANSMIT_COUNT(BT_MESH_ADV(buf)->xmit) + 1) * */
    /*              (adv_int + 10))); */
    duration = USER_ADV_SEND_DURATION;
    if (cb) {
        if (cb->start) {
            cb->start(duration, 0, cb_data);
        }

        if (cb->user_intercept) {
            cb->user_intercept(&delay, &duration, cb_data);
        }
    }

    if (0 == delay) {
        fresh_adv_info(buf);
    } else {
        BT_MESH_ADV(buf)->delay = 1;
    }

    mesh_adv_timeout_start(delay, duration, buf);

    return 0;
}

void resume_mesh_gatt_proxy_adv_thread(void)
{
    BT_MESH_FEATURES_IS_SUPPORT_OPTIMIZE(BT_MESH_FEAT_PROXY);

    if (!(IS_ENABLED(CONFIG_BT_MESH_PROXY) & IS_ENABLED(CONFIG_BT_MESH_PB_GATT))) {
        return;
    }

    BT_INFO("--func=%s", __FUNCTION__);

    if (TRUE == mesh_adv_send_timer_busy()) {
        return;
    }

    ble_adv_enable(0);

    //< send period connectable adv ( unprovision adv || proxy adv )
    if (FALSE == bt_mesh_is_provisioned()) {
        unprovision_connectable_adv();
    } else if (false == bt_mesh_get_if_app_key_add()) {
        proxy_fast_connectable_adv();
    } else {
        proxy_connectable_adv();
    }

    if (FALSE == get_if_connecting()) {
        ble_adv_enable(1);
    }
}

void bt_mesh_adv_update(void)
{
    BT_INFO("--func=%s", __FUNCTION__);

    resume_mesh_gatt_proxy_adv_thread();
}

void newbuf_replace(struct net_buf_pool *pool)
{
    struct net_buf *buf;

    buf = net_buf_slist_simple_get(&adv_list);

    BT_MESH_ADV(buf)->busy = 0;

    buf->ref = 0;

    buf->flags = 0;

    pool->free_count++;
}

void bt_mesh_adv_send(struct net_buf *buf, const struct bt_mesh_send_cb *cb,
                      void *cb_data)
{
    BT_DBG("--func=%s", __FUNCTION__);
    /* BT_DBG("type 0x%02x len %u: %s", BT_MESH_ADV(buf)->type, buf->len, */
    /* bt_hex(buf->data, buf->len)); */

    BT_MESH_ADV(buf)->cb = cb;
    BT_MESH_ADV(buf)->cb_data = cb_data;
    BT_MESH_ADV(buf)->busy = 1U;

    net_buf_ref(buf);

    BT_INFO("adv_list.head=0x%x", adv_list.head);
    BT_INFO("adv_list.tail=0x%x", adv_list.tail);

    bool send_busy = adv_send(buf);

    BT_INFO("adv_list.head=0x%x", adv_list.head);
    BT_INFO("adv_list.tail=0x%x", adv_list.tail);
    BT_INFO("mesh_adv_send %s", send_busy ? "busy" : "succ");
}

void bt_mesh_adv_init(void)
{
#if NET_BUF_USE_MALLOC
    bt_mesh_adv_buf_alloc();
#endif /* NET_BUF_USE_MALLOC */
}

#endif /* ADAPTATION_COMPILE_DEBUG */
