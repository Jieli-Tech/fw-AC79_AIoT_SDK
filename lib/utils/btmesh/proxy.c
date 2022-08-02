/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adaptation.h"
#include "mesh.h"
#include "net/buf.h"
#include "mesh_net.h"
#include "adv.h"
#include "prov.h"
#include "beacon.h"
#include "foundation.h"
#include "access.h"
#include "proxy.h"
#include "mesh_crypto.h"

#define LOG_TAG             "[MESH-proxy]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_WARN_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DUMP_ENABLE
#include "mesh_log.h"

#if MESH_RAM_AND_CODE_MAP_DETAIL
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_mesh_proxy_bss")
#pragma data_seg(".ble_mesh_proxy_data")
#pragma const_seg(".ble_mesh_proxy_const")
#pragma code_seg(".ble_mesh_proxy_code")
#endif
#else /* MESH_RAM_AND_CODE_MAP_DETAIL */
#pragma bss_seg(".ble_mesh_bss")
#pragma data_seg(".ble_mesh_data")
#pragma const_seg(".ble_mesh_const")
#pragma code_seg(".ble_mesh_code")
#endif /* MESH_RAM_AND_CODE_MAP_DETAIL */

#define PDU_TYPE(data)     (data[0] & BIT_MASK(6))
#define PDU_SAR(data)      (data[0] >> 6)

/* Mesh Profile 1.0 Section 6.6:
 * "The timeout for the SAR transfer is 20 seconds. When the timeout
 *  expires, the Proxy Server shall disconnect."
 */
#define PROXY_SAR_TIMEOUT  K_SECONDS(20)

#define SAR_COMPLETE       0x00
#define SAR_FIRST          0x01
#define SAR_CONT           0x02
#define SAR_LAST           0x03

#define CFG_FILTER_SET     0x00
#define CFG_FILTER_ADD     0x01
#define CFG_FILTER_REMOVE  0x02
#define CFG_FILTER_STATUS  0x03

#define PDU_HDR(sar, type) (sar << 6 | (type & BIT_MASK(6)))

#define CLIENT_BUF_SIZE 68

#define ID_TYPE_NODE 0x01

static struct bt_mesh_proxy_client {
    struct bt_conn *conn;
    u16_t filter[CONFIG_BT_MESH_PROXY_FILTER_SIZE];
    /* enum __packed { */
    enum __attribute__((packed))
    {
        NONE,
        WHITELIST,
        BLACKLIST,
        PROV,
    } filter_type;
    u8_t msg_type;
    struct k_delayed_work    sar_timer;
    struct net_buf_simple    buf;
} clients[CONFIG_BT_MAX_CONN];

static u8_t __noinit client_buf_data[CLIENT_BUF_SIZE * CONFIG_BT_MAX_CONN];

/* Track which service is enabled */
static enum {
    MESH_GATT_NONE,
    MESH_GATT_PROV,
    MESH_GATT_PROXY,
} gatt_svc = MESH_GATT_NONE;

static struct bt_mesh_proxy_client *find_client(struct bt_conn *conn)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(clients); i++) {
        if (clients[i].conn == conn) {
            return &clients[i];
        }
    }

    return NULL;
}

static void proxy_sar_timeout(struct k_work *work)
{
    struct bt_mesh_proxy_client *client;

    BT_WARN("Proxy SAR timeout");

    client = CONTAINER_OF(work, struct bt_mesh_proxy_client, sar_timer);
    if (client->conn) {
        bt_conn_disconnect(client->conn,
                           BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    }
}

#if defined(CONFIG_BT_MESH_GATT_PROXY)

static int proxy_segment_and_send(struct bt_conn *conn, u8_t type,
                                  struct net_buf_simple *msg);

static int filter_set(struct bt_mesh_proxy_client *client,
                      struct net_buf_simple *buf)
{
    u8_t type;

    if (buf->len < 1) {
        BT_WARN("Too short Filter Set message");
        return -EINVAL;
    }

    type = net_buf_simple_pull_u8(buf);
    BT_DBG("type 0x%02x", type);

    switch (type) {
    case 0x00:
        (void)memset(client->filter, 0, sizeof(client->filter));
        client->filter_type = WHITELIST;
        break;
    case 0x01:
        (void)memset(client->filter, 0, sizeof(client->filter));
        client->filter_type = BLACKLIST;
        break;
    default:
        BT_WARN("Prohibited Filter Type 0x%02x", type);
        return -EINVAL;
    }

    return 0;
}

static void filter_add(struct bt_mesh_proxy_client *client, u16_t addr)
{
    int i;

    BT_DBG("addr 0x%04x", addr);

    if (addr == BT_MESH_ADDR_UNASSIGNED) {
        return;
    }

    for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
        if (client->filter[i] == addr) {
            return;
        }
    }

    for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
        if (client->filter[i] == BT_MESH_ADDR_UNASSIGNED) {
            client->filter[i] = addr;
            return;
        }
    }
}

static void filter_remove(struct bt_mesh_proxy_client *client, u16_t addr)
{
    int i;

    BT_DBG("addr 0x%04x", addr);

    if (addr == BT_MESH_ADDR_UNASSIGNED) {
        return;
    }

    for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
        if (client->filter[i] == addr) {
            client->filter[i] = BT_MESH_ADDR_UNASSIGNED;
            return;
        }
    }
}

static void send_filter_status(struct bt_mesh_proxy_client *client,
                               struct bt_mesh_net_rx *rx,
                               struct net_buf_simple *buf)
{
    struct bt_mesh_net_tx tx = {
        .sub = rx->sub,
        .ctx = &rx->ctx,
        .src = bt_mesh_primary_addr(),
    };
    u16_t filter_size;
    int i, err;

    /* Configuration messages always have dst unassigned */
    tx.ctx->addr = BT_MESH_ADDR_UNASSIGNED;

    net_buf_simple_reset(buf);
    net_buf_simple_reserve(buf, 10);

    net_buf_simple_add_u8(buf, CFG_FILTER_STATUS);

    if (client->filter_type == WHITELIST) {
        net_buf_simple_add_u8(buf, 0x00);
    } else {
        net_buf_simple_add_u8(buf, 0x01);
    }

    for (filter_size = 0U, i = 0; i < ARRAY_SIZE(client->filter); i++) {
        if (client->filter[i] != BT_MESH_ADDR_UNASSIGNED) {
            filter_size++;
        }
    }

    net_buf_simple_add_be16(buf, filter_size);

    BT_DBG("%u bytes: %s", buf->len, bt_hex(buf->data, buf->len));

    err = bt_mesh_net_encode(&tx, buf, true);
    if (err) {
        BT_ERR("Encoding Proxy cfg message failed (err %d)", err);
        return;
    }

    err = proxy_segment_and_send(client->conn, BT_MESH_PROXY_CONFIG, buf);
    if (err) {
        BT_ERR("Failed to send proxy cfg message (err %d)", err);
    }
}

static void proxy_cfg(struct bt_mesh_proxy_client *client)
{
    NET_BUF_SIMPLE_DEFINE(buf, 29);
    struct bt_mesh_net_rx rx;
    u8_t opcode;
    int err;

    err = bt_mesh_net_decode(&client->buf, BT_MESH_NET_IF_PROXY_CFG,
                             &rx, &buf);
    if (err) {
        BT_ERR("Failed to decode Proxy Configuration (err %d)", err);
        return;
    }

    /* Remove network headers */
    net_buf_simple_pull(&buf, BT_MESH_NET_HDR_LEN);

    BT_DBG("%u bytes: %s", buf.len, bt_hex(buf.data, buf.len));

    if (buf.len < 1) {
        BT_WARN("Too short proxy configuration PDU");
        return;
    }

    opcode = net_buf_simple_pull_u8(&buf);
    switch (opcode) {
    case CFG_FILTER_SET:
        filter_set(client, &buf);
        send_filter_status(client, &rx, &buf);
        break;
    case CFG_FILTER_ADD:
        while (buf.len >= 2) {
            u16_t addr;

            addr = net_buf_simple_pull_be16(&buf);
            filter_add(client, addr);
        }
        send_filter_status(client, &rx, &buf);
        break;
    case CFG_FILTER_REMOVE:
        while (buf.len >= 2) {
            u16_t addr;

            addr = net_buf_simple_pull_be16(&buf);
            filter_remove(client, addr);
        }
        send_filter_status(client, &rx, &buf);
        break;
    default:
        BT_WARN("Unhandled configuration OpCode 0x%02x", opcode);
        break;
    }
}

static int beacon_send(struct bt_conn *conn, struct bt_mesh_subnet *sub)
{
    BT_INFO("--func=%s", __FUNCTION__);

    NET_BUF_SIMPLE_DEFINE(buf, 23);

    net_buf_simple_reserve(&buf, 1);
    bt_mesh_beacon_create(sub, &buf);

    return proxy_segment_and_send(conn, BT_MESH_PROXY_BEACON, &buf);
}

static void proxy_send_beacons(struct bt_mesh_proxy_client *client)
{
    BT_INFO("--func=%s", __FUNCTION__);

    int i;

    for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
        struct bt_mesh_subnet *sub = &bt_mesh.sub[i];

        if (sub->net_idx != BT_MESH_KEY_UNUSED) {
            beacon_send(client->conn, sub);
        }
    }
}

void bt_mesh_proxy_beacon_send(struct bt_mesh_subnet *sub)
{
    int i;

    if (!sub) {
        /* NULL means we send on all subnets */
        for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
            if (bt_mesh.sub[i].net_idx != BT_MESH_KEY_UNUSED) {
                bt_mesh_proxy_beacon_send(&bt_mesh.sub[i]);
            }
        }

        return;
    }

    for (i = 0; i < ARRAY_SIZE(clients); i++) {
        if (clients[i].conn) {
            beacon_send(clients[i].conn, sub);
        }
    }
}

static int node_id_adv(struct bt_mesh_subnet *sub)
{
    u8_t tmp[16];
    int err;

    BT_DBG("");

    u8 *proxy_svc_data = get_server_data_addr();

    proxy_svc_data[2] = ID_TYPE_NODE;

    err = bt_rand(proxy_svc_data + 11, 8);
    if (err) {
        return err;
    }

    (void)memset(tmp, 0, 6);
    memcpy(tmp + 6, proxy_svc_data + 11, 8);
    sys_put_be16(bt_mesh_primary_addr(), tmp + 14);

    err = bt_encrypt_be(sub->keys[sub->kr_flag].identity, tmp, tmp);
    if (err) {
        return err;
    }

    memcpy(proxy_svc_data + 3, tmp + 8, 8);

    return 0;
}

void bt_mesh_proxy_identity_start(struct bt_mesh_subnet *sub)
{
    sub->node_id = BT_MESH_NODE_IDENTITY_RUNNING;

    node_id_adv(sub);
}

void bt_mesh_proxy_identity_stop(struct bt_mesh_subnet *sub)
{
    sub->node_id = BT_MESH_NODE_IDENTITY_STOPPED;
    sub->node_id_start = 0U;
}

int bt_mesh_proxy_identity_enable(void)
{
    int i, count = 0;

    BT_DBG("");

    if (!bt_mesh_is_provisioned()) {
        return -EAGAIN;
    }

    for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
        struct bt_mesh_subnet *sub = &bt_mesh.sub[i];

        if (sub->net_idx == BT_MESH_KEY_UNUSED) {
            continue;
        }

        if (sub->node_id == BT_MESH_NODE_IDENTITY_NOT_SUPPORTED) {
            continue;
        }

        bt_mesh_proxy_identity_start(sub);
        count++;
    }

    if (count) {
        bt_mesh_adv_update();
    }

    return 0;
}

#endif /* GATT_PROXY */

static void proxy_complete_pdu(struct bt_mesh_proxy_client *client)
{
    switch (client->msg_type) {
#if defined(CONFIG_BT_MESH_GATT_PROXY)
    case BT_MESH_PROXY_NET_PDU:
        BT_DBG("Mesh Network PDU");
        if (BT_MESH_FEATURES_IS_SUPPORT(BT_MESH_FEAT_PROXY)) {
            bt_mesh_net_recv(&client->buf, 0, BT_MESH_NET_IF_PROXY);
        }
        break;
    case BT_MESH_PROXY_BEACON:
        BT_DBG("Mesh Beacon PDU");
        if (BT_MESH_FEATURES_IS_SUPPORT(BT_MESH_FEAT_PROXY)) {
            bt_mesh_beacon_recv(&client->buf);
        }
        break;
    case BT_MESH_PROXY_CONFIG:
        BT_DBG("Mesh Configuration PDU");
        if (BT_MESH_FEATURES_IS_SUPPORT(BT_MESH_FEAT_PROXY)) {
            proxy_cfg(client);
        }
        break;
#endif
#if defined(CONFIG_BT_MESH_PB_GATT)
    case BT_MESH_PROXY_PROV:
        BT_DBG("Mesh Provisioning PDU");
        bt_mesh_pb_gatt_recv(client->conn, &client->buf);
        break;
#endif
    default:
        BT_WARN("Unhandled Message Type 0x%02x", client->msg_type);
        break;
    }

    net_buf_simple_reset(&client->buf);
}

ssize_t proxy_recv(struct bt_conn *conn,
                   const void *buf,
                   u16_t len, u16_t offset, u8_t flags)
{
    struct bt_mesh_proxy_client *client = find_client(conn);
    const u8_t *data = buf;

    if (!client) {
        return -ENOTCONN;
    }

    if (len < 1) {
        BT_WARN("Too small Proxy PDU");
        return -EINVAL;
    }

    if (len - 1 > net_buf_simple_tailroom(&client->buf)) {
        BT_WARN("Too big proxy PDU");
        return -EINVAL;
    }

    BT_INFO("PDU_SAR(data)=0x%x\r\n", PDU_SAR(data));
    switch (PDU_SAR(data)) {
    case SAR_COMPLETE:
        if (client->buf.len) {
            BT_WARN("Complete PDU while a pending incomplete one");
            return -EINVAL;
        }

        client->msg_type = PDU_TYPE(data);
        net_buf_simple_add_mem(&client->buf, data + 1, len - 1);
        proxy_complete_pdu(client);
        break;

    case SAR_FIRST:
        if (client->buf.len) {
            BT_WARN("First PDU while a pending incomplete one");
            return -EINVAL;
        }

        k_delayed_work_submit(&client->sar_timer, PROXY_SAR_TIMEOUT);
        client->msg_type = PDU_TYPE(data);
        net_buf_simple_add_mem(&client->buf, data + 1, len - 1);
        break;

    case SAR_CONT:
        if (!client->buf.len) {
            BT_WARN("Continuation with no prior data");
            return -EINVAL;
        }

        if (client->msg_type != PDU_TYPE(data)) {
            BT_WARN("Unexpected message type in continuation");
            return -EINVAL;
        }

        k_delayed_work_submit(&client->sar_timer, PROXY_SAR_TIMEOUT);
        net_buf_simple_add_mem(&client->buf, data + 1, len - 1);
        break;

    case SAR_LAST:
        if (!client->buf.len) {
            BT_WARN("Last SAR PDU with no prior data");
            return -EINVAL;
        }

        if (client->msg_type != PDU_TYPE(data)) {
            BT_WARN("Unexpected message type in last SAR PDU");
            return -EINVAL;
        }

        k_delayed_work_cancel(&client->sar_timer);
        net_buf_simple_add_mem(&client->buf, data + 1, len - 1);
        proxy_complete_pdu(client);
        break;
    }

    return len;
}

static int conn_count;

static void proxy_connected(struct bt_conn *conn, u8_t err)
{
    struct bt_mesh_proxy_client *client;
    int i;

    BT_DBG("conn %x err 0x%02x", conn, err);

    conn_count++;

    /* Try to re-enable advertising in case it's possible */
    if (conn_count < CONFIG_BT_MAX_CONN) {
        bt_mesh_adv_update();
    }

    for (client = NULL, i = 0; i < ARRAY_SIZE(clients); i++) {
        if (!clients[i].conn) {
            client = &clients[i];
            break;
        }
    }

    if (!client) {
        BT_ERR("No free Proxy Client objects");
        return;
    }

    client->conn = bt_conn_ref(conn);
    client->filter_type = NONE;
    (void)memset(client->filter, 0, sizeof(client->filter));
    net_buf_simple_reset(&client->buf);
}

static void proxy_disconnected(struct bt_conn *conn, u8_t reason)
{
    int i;

    BT_DBG("conn %x reason 0x%02x", conn, reason);

    conn_count--;

    for (i = 0; i < ARRAY_SIZE(clients); i++) {
        struct bt_mesh_proxy_client *client = &clients[i];

        if (client->conn == conn) {
            if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT) &&
                client->filter_type == PROV) {
                bt_mesh_pb_gatt_close(conn);
            }

            k_delayed_work_cancel(&client->sar_timer);
            bt_conn_unref(client->conn);
            client->conn = NULL;
            break;
        }
    }

    bt_mesh_adv_update();
}

struct net_buf_simple *bt_mesh_proxy_get_buf(void)
{
    struct net_buf_simple *buf = &clients[0].buf;

    net_buf_simple_reset(buf);

    return buf;
}

#if defined(CONFIG_BT_MESH_PB_GATT)
ssize_t prov_ccc_write(struct bt_conn *conn,
                       const void *buf, u16_t len,
                       u16_t offset, u8_t flags)
{
    struct bt_mesh_proxy_client *client;
    u16_t value;

    BT_DBG("len %u: %s", len, bt_hex(buf, len));

    if (len != sizeof(value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    value = sys_get_le16(buf);
    if (value != BT_GATT_CCC_NOTIFY) {
        BT_WARN("Client wrote 0x%04x instead enabling notify", value);
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }
    /* If a connection exists there must be a client */
    client = find_client(conn);
    __ASSERT(client, "No client for connection");

    if (client->filter_type == NONE) {
        client->filter_type = PROV;
        bt_mesh_pb_gatt_open(conn);
    }

    return len;
}

int bt_mesh_proxy_prov_enable(void)
{
    int i;

    BT_INFO("--func=%s", __FUNCTION__);

    if (gatt_svc == MESH_GATT_PROV) {
        return -EALREADY;
    }

    if (gatt_svc != MESH_GATT_NONE) {
        return -EBUSY;
    }

    bt_gatt_service_register(BT_UUID_MESH_PROV);
    gatt_svc = MESH_GATT_PROV;

    for (i = 0; i < ARRAY_SIZE(clients); i++) {
        if (clients[i].conn) {
            clients[i].filter_type = PROV;
        }
    }

    return 0;
}

int bt_mesh_proxy_prov_disable(bool disconnect)
{
    int i;

    BT_INFO("--func=%s", __FUNCTION__);

    if (gatt_svc == MESH_GATT_NONE) {
        return -EALREADY;
    }

    if (gatt_svc != MESH_GATT_PROV) {
        return -EBUSY;
    }

    bt_gatt_service_unregister(BT_UUID_MESH_PROV);
    gatt_svc = MESH_GATT_NONE;

    for (i = 0; i < ARRAY_SIZE(clients); i++) {
        struct bt_mesh_proxy_client *client = &clients[i];

        if (!client->conn || client->filter_type != PROV) {
            continue;
        }

        if (disconnect) {
            bt_conn_disconnect(client->conn,
                               BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        } else {
            bt_mesh_pb_gatt_close(client->conn);
            client->filter_type = NONE;
        }
    }

    bt_mesh_adv_update();

    return 0;
}

#endif /* CONFIG_BT_MESH_PB_GATT */

#if defined(CONFIG_BT_MESH_GATT_PROXY)
ssize_t proxy_ccc_write(struct bt_conn *conn,
                        const void *buf, u16_t len,
                        u16_t offset, u8_t flags)
{
    struct bt_mesh_proxy_client *client;
    u16_t value;

    BT_DBG("len %u: %s", len, bt_hex(buf, len));

    if (len != sizeof(value)) {
        return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
    }

    value = sys_get_le16(buf);
    if (value != BT_GATT_CCC_NOTIFY) {
        BT_WARN("Client wrote 0x%04x instead enabling notify", value);
        return BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED);
    }

    /* If a connection exists there must be a client */
    client = find_client(conn);
    __ASSERT(client, "No client for connection");

    if (client->filter_type == NONE) {
        client->filter_type = WHITELIST;
        // send beacons
        proxy_send_beacons(client);
    }

    return len;
}

int bt_mesh_proxy_gatt_enable(void)
{
    int i;

    BT_INFO("--func=%s", __FUNCTION__);

    if (gatt_svc == MESH_GATT_PROXY) {
        return -EALREADY;
    }

    if (gatt_svc != MESH_GATT_NONE) {
        return -EBUSY;
    }
    bt_gatt_service_register(BT_UUID_MESH_PROXY);
    gatt_svc = MESH_GATT_PROXY;

    for (i = 0; i < ARRAY_SIZE(clients); i++) {
        if (clients[i].conn) {
            clients[i].filter_type = WHITELIST;
        }
    }

    return 0;
}

void bt_mesh_proxy_gatt_disconnect(void)
{
    int i;

    BT_INFO("--func=%s", __FUNCTION__);

    for (i = 0; i < ARRAY_SIZE(clients); i++) {
        struct bt_mesh_proxy_client *client = &clients[i];

        if (client->conn && (client->filter_type == WHITELIST ||
                             client->filter_type == BLACKLIST)) {
            client->filter_type = NONE;
            bt_conn_disconnect(client->conn,
                               BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        }
    }
}

int bt_mesh_proxy_gatt_disable(void)
{
    BT_MESH_FEATURES_IS_SUPPORT_OPTIMIZE(BT_MESH_FEAT_PROXY) 0;

    BT_INFO("--func=%s", __FUNCTION__);

    if (gatt_svc == MESH_GATT_NONE) {
        return -EALREADY;
    }

    if (gatt_svc != MESH_GATT_PROXY) {
        return -EBUSY;
    }

    bt_mesh_proxy_gatt_disconnect();

    bt_gatt_service_unregister(BT_UUID_MESH_PROXY);
    gatt_svc = MESH_GATT_NONE;

    return 0;
}

void bt_mesh_proxy_addr_add(struct net_buf_simple *buf, u16_t addr)
{
    struct bt_mesh_proxy_client *client =
        CONTAINER_OF(buf, struct bt_mesh_proxy_client, buf);

    BT_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

    if (client->filter_type == WHITELIST) {
        filter_add(client, addr);
    } else if (client->filter_type == BLACKLIST) {
        filter_remove(client, addr);
    }
}

static bool client_filter_match(struct bt_mesh_proxy_client *client,
                                u16_t addr)
{
    int i;

    BT_DBG("filter_type %u addr 0x%04x", client->filter_type, addr);

    if (client->filter_type == BLACKLIST) {
        for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
            if (client->filter[i] == addr) {
                return false;
            }
        }

        return true;
    }

    if (addr == BT_MESH_ADDR_ALL_NODES) {
        return true;
    }

    if (client->filter_type == WHITELIST) {
        for (i = 0; i < ARRAY_SIZE(client->filter); i++) {
            if (client->filter[i] == addr) {
                return true;
            }
        }
    }

    return false;
}

bool bt_mesh_proxy_relay(struct net_buf_simple *buf, u16_t dst)
{
    bool relayed = false;
    int i;

    BT_DBG("%u bytes to dst 0x%04x", buf->len, dst);

    for (i = 0; i < ARRAY_SIZE(clients); i++) {
        struct bt_mesh_proxy_client *client = &clients[i];
        NET_BUF_SIMPLE_DEFINE(msg, 32);

        if (!client->conn) {
            continue;
        }

        if (!client_filter_match(client, dst)) {
            continue;
        }

        /* Proxy PDU sending modifies the original buffer,
         * so we need to make a copy.
         */
        net_buf_simple_reserve(&msg, 1);
        net_buf_simple_add_mem(&msg, buf->data, buf->len);

        bt_mesh_proxy_send(client->conn, BT_MESH_PROXY_NET_PDU, &msg);
        relayed = true;
    }

    return relayed;
}

#endif /* CONFIG_BT_MESH_GATT_PROXY */

static int proxy_send(struct bt_conn *conn, const void *data, u16_t len)
{
    BT_DBG("%u bytes: %s", len, bt_hex(data, len));

#if defined(CONFIG_BT_MESH_GATT_PROXY)
    if (BT_MESH_FEATURES_IS_SUPPORT(BT_MESH_FEAT_PROXY)) {
        if (gatt_svc == MESH_GATT_PROXY) {
            return bt_gatt_notify(conn, data, len);
        }
    }
#endif

#if defined(CONFIG_BT_MESH_PB_GATT)
    if (gatt_svc == MESH_GATT_PROV) {
        return bt_gatt_notify(conn, data, len);
    }
#endif

    return 0;
}

static int proxy_segment_and_send(struct bt_conn *conn, u8_t type,
                                  struct net_buf_simple *msg)
{
    u16_t mtu;

    BT_INFO("--func=%s", __FUNCTION__);

    BT_DBG("conn %p type 0x%02x len %u: %s", conn, type, msg->len,
           bt_hex(msg->data, msg->len));

    /* ATT_MTU - OpCode (1 byte) - Handle (2 bytes) */
    mtu = bt_gatt_get_mtu(conn) - 3;
    if (mtu > msg->len) {
        net_buf_simple_push_u8(msg, PDU_HDR(SAR_COMPLETE, type));
        BT_INFO("SAR_COMPLETE");
        return proxy_send(conn, msg->data, msg->len);
    }

    net_buf_simple_push_u8(msg, PDU_HDR(SAR_FIRST, type));
    BT_INFO("SAR_FIRST");
    proxy_send(conn, msg->data, mtu);
    net_buf_simple_pull(msg, mtu);

    while (msg->len) {
        if (msg->len + 1 < mtu) {
            net_buf_simple_push_u8(msg, PDU_HDR(SAR_LAST, type));
            BT_INFO("SAR_LAST");
            proxy_send(conn, msg->data, msg->len);
            break;
        }

        net_buf_simple_push_u8(msg, PDU_HDR(SAR_CONT, type));
        BT_INFO("SAR_CONT");
        proxy_send(conn, msg->data, mtu);
        net_buf_simple_pull(msg, mtu);
    }

    return 0;
}

int bt_mesh_proxy_send(struct bt_conn *conn, u8_t type,
                       struct net_buf_simple *msg)
{
    struct bt_mesh_proxy_client *client = find_client(conn);

    if (!client) {
        BT_ERR("No Proxy Client found");
        return -ENOTCONN;
    }

    if ((client->filter_type == PROV) != (type == BT_MESH_PROXY_PROV)) {
        BT_ERR("Invalid PDU type for Proxy Client");
        return -EINVAL;
    }

    return proxy_segment_and_send(conn, type, msg);
}

static struct bt_conn_cb conn_callbacks = {
    .connected = proxy_connected,
    .disconnected = proxy_disconnected,
};

int bt_mesh_proxy_init(void)
{
    int i;

    BT_INFO("--func=%s", __FUNCTION__);

    /* Initialize the client receive buffers */
    for (i = 0; i < ARRAY_SIZE(clients); i++) {
        struct bt_mesh_proxy_client *client = &clients[i];

        client->buf.size = CLIENT_BUF_SIZE;
        client->buf.__buf = client_buf_data + (i * CLIENT_BUF_SIZE);

        k_delayed_work_init(&client->sar_timer, proxy_sar_timeout);
    }

    bt_conn_cb_register(&conn_callbacks);

    proxy_gatt_init();

    return 0;
}
