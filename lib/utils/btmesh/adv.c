/*  Bluetooth Mesh */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adaptation.h"
#include "net/buf.h"
#include "adv.h"
#include "mesh_net.h"
#include "foundation.h"
#include "beacon.h"
#include "prov.h"
#include "proxy.h"

#define LOG_TAG             "[MESH-adv]"
/* #define LOG_INFO_ENABLE */
#define LOG_DEBUG_ENABLE
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#include "mesh_log.h"

#if MESH_RAM_AND_CODE_MAP_DETAIL
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_mesh_adv_bss")
#pragma data_seg(".ble_mesh_adv_data")
#pragma const_seg(".ble_mesh_adv_const")
#pragma code_seg(".ble_mesh_adv_code")
#endif
#else /* MESH_RAM_AND_CODE_MAP_DETAIL */
#pragma bss_seg(".ble_mesh_bss")
#pragma data_seg(".ble_mesh_data")
#pragma const_seg(".ble_mesh_const")
#pragma code_seg(".ble_mesh_code")
#endif /* MESH_RAM_AND_CODE_MAP_DETAIL */

NET_BUF_POOL_DEFINE(adv_buf_pool, CONFIG_BT_MESH_ADV_BUF_COUNT,
                    BT_MESH_ADV_DATA_SIZE, BT_MESH_ADV_USER_DATA_SIZE, NULL);

extern void newbuf_replace(struct net_buf_pool *pool);

#if NET_BUF_USE_MALLOC
static struct bt_mesh_adv *adv_pool;
#else
static struct bt_mesh_adv adv_pool[CONFIG_BT_MESH_ADV_BUF_COUNT];
#endif /* NET_BUF_USE_MALLOC */

static struct bt_mesh_adv *adv_alloc(int id)
{
    return &adv_pool[id];
}

static inline void adv_send_start(u16_t duration, int err,
                                  const struct bt_mesh_send_cb *cb,
                                  void *cb_data)
{
    if (cb && cb->start) {
        cb->start(duration, err, cb_data);
    }
}

static inline void adv_send_end(int err, const struct bt_mesh_send_cb *cb,
                                void *cb_data)
{
    if (cb && cb->end) {
        cb->end(err, cb_data);
    }
}

struct net_buf *bt_mesh_adv_create_from_pool(struct net_buf_pool *pool,
        bt_mesh_adv_alloc_t get_id,
        enum bt_mesh_adv_type type,
        u8_t xmit, s32_t timeout)
{
    struct bt_mesh_adv *adv;
    struct net_buf *buf;

    buf = net_buf_alloc(pool, timeout);
    if (!buf) {
        return NULL;
    }

    adv = get_id(net_buf_id(buf));
    BT_MESH_ADV(buf) = adv;

    (void)memset(adv, 0, sizeof(*adv));

    adv->type         = type;
    adv->xmit         = xmit;

#if MESH_ADAPTATION_OPTIMIZE
    buf->__buf += BT_MESH_ADV_DATA_HEAD_SIZE;
    buf->data = buf->__buf;
#endif /* MESH_ADAPTATION_OPTIMIZE */

    BT_INFO("alloc buf addr=0x%x", buf);

    return buf;
}

struct net_buf *bt_mesh_adv_create(enum bt_mesh_adv_type type, u8_t xmit,
                                   s32_t timeout)
{
#if CONFIG_BUF_REPLACE_EN
    if (0 == adv_buf_pool.free_count) {
        newbuf_replace(&adv_buf_pool);
    }
#endif /* CONFIG_BUF_REPLACE_EN */

    return bt_mesh_adv_create_from_pool(&adv_buf_pool, adv_alloc, type,
                                        xmit, timeout);
}

static void bt_mesh_scan_cb(const bt_addr_le_t *addr, s8_t rssi,
                            u8_t adv_type, struct net_buf_simple *buf)
{
    if (adv_type != BT_LE_ADV_NONCONN_IND) {
        return;
    }

    /* BT_DBG("len %u: %s", buf->len, bt_hex(buf->data, buf->len)); */

    while (buf->len > 1) {
        struct net_buf_simple_state state;
        u8_t len, type;

        len = net_buf_simple_pull_u8(buf);
        /* Check for early termination */
        if (len == 0) {
            return;
        }

        if (len > buf->len) {
            BT_WARN("AD malformed");
            return;
        }

        net_buf_simple_save(buf, &state);

        type = net_buf_simple_pull_u8(buf);

        buf->len = len - 1;

        switch (type) {
        case BT_DATA_MESH_MESSAGE:
            BT_INFO("\n< ADV-BT_DATA_MESH_MESSAGE >\n");
            bt_mesh_net_recv(buf, rssi, BT_MESH_NET_IF_ADV);
            break;
#if defined(CONFIG_BT_MESH_PB_ADV)
        case BT_DATA_MESH_PROV:
            BT_INFO("\n< ADV-BT_DATA_MESH_PROV >\n");
#if defined(CONFIG_BT_MESH_PROVISIONER)
            bt_mesh_pb_adv_recv(buf);
#endif
            break;
#endif
        case BT_DATA_MESH_BEACON:
            BT_INFO("\n< ADV-BT_DATA_MESH_BEACON>\n");
            bt_mesh_beacon_recv(buf);
            break;
        default:
            break;
        }

        net_buf_simple_restore(buf, &state);
        net_buf_simple_pull(buf, len);
    }
}

int bt_mesh_scan_enable(void)
{
    BT_DBG("");

    return bt_le_scan_start(bt_mesh_scan_cb);
}

int bt_mesh_scan_disable(void)
{
    BT_DBG("");

    return bt_le_scan_stop();
}

#if NET_BUF_USE_MALLOC

#include "system/malloc.h"

void bt_mesh_adv_buf_alloc(void)
{
    BT_DBG("--func=%s, adv buffer cnt = %d", __FUNCTION__, config_bt_mesh_adv_buf_count);

    u32 buf_size;
    u32 net_buf_p, net_buf_data_p, adv_pool_p;

    buf_size = sizeof(struct net_buf) * config_bt_mesh_adv_buf_count;
    BT_DBG("net_buf size=0x%x", buf_size);
    net_buf_data_p = buf_size;
    buf_size += ALIGN_4BYTE(config_bt_mesh_adv_buf_count * BT_MESH_ADV_DATA_SIZE);
    BT_DBG("net_buf_data size=0x%x", ALIGN_4BYTE(config_bt_mesh_adv_buf_count * BT_MESH_ADV_DATA_SIZE));
    adv_pool_p = buf_size;
    buf_size += (sizeof(struct bt_mesh_adv) * config_bt_mesh_adv_buf_count);
    BT_DBG("adv_pool size=0x%x", sizeof(struct bt_mesh_adv) * config_bt_mesh_adv_buf_count);

    net_buf_p = (u32)malloc(buf_size);
    ASSERT(net_buf_p);
    memset((u8 *)net_buf_p, 0, buf_size);

    net_buf_data_p += net_buf_p;
    adv_pool_p += net_buf_p;

    NET_BUF_MALLOC(adv_buf_pool,
                   net_buf_p, net_buf_data_p, (struct bt_mesh_adv *)adv_pool_p,
                   config_bt_mesh_adv_buf_count);

    BT_DBG("total buf_size=0x%x", buf_size);
    BT_DBG("net_buf addr=0x%x", net_buf_p);
    BT_DBG("net_buf_data addr=0x%x", net_buf_data_p);
    BT_DBG("adv_pool addr=0x%x", adv_pool_p);
    BT_DBG("*fixed->data_pool=0x%x", *((u32 *)net_buf_fixed_adv_buf_pool.data_pool));
    BT_DBG("net_buf_adv_buf_pool user_data addr=0x%x\r\n", net_buf_adv_buf_pool->user_data);
}

void bt_mesh_adv_buf_free(void)
{
    free(NET_BUF_FREE(adv_buf_pool));
}

#endif /* NET_BUF_USE_MALLOC */

