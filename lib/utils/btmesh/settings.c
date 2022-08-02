/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adaptation.h"
#include "mesh_net.h"
#include "mesh_crypto.h"
#include "mesh_transport.h"
#include "access.h"
#include "foundation.h"
#include "proxy.h"
#include "mesh_settings.h"
#include "syscfg/syscfg_id.h"

#define LOG_TAG             "[MESH-settings]"
/* #define LOG_INFO_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_WARN_ENABLE */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DUMP_ENABLE */
#include "mesh_log.h"

#if MESH_RAM_AND_CODE_MAP_DETAIL
#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".ble_mesh_settings_bss")
#pragma data_seg(".ble_mesh_settings_data")
#pragma const_seg(".ble_mesh_settings_const")
#pragma code_seg(".ble_mesh_settings_code")
#endif
#else /* MESH_RAM_AND_CODE_MAP_DETAIL */
#pragma bss_seg(".ble_mesh_bss")
#pragma data_seg(".ble_mesh_data")
#pragma const_seg(".ble_mesh_const")
#pragma code_seg(".ble_mesh_code")
#endif /* MESH_RAM_AND_CODE_MAP_DETAIL */

#define MAX_MODEL_NUMS      6

typedef enum _NODE_INFO_SETTING_INDEX {
    /* NODE_MAC_ADDR_INDEX = 0, */
    NET_INDEX = VM_BLE_MESH_IDX_BEGIN,
    IV_INDEX,
    SEQ_INDEX,
    RPL_INDEX,
    NET_KEY_INDEX = RPL_INDEX + CONFIG_BT_MESH_CRPL,
    APP_KEY_INDEX = NET_KEY_INDEX + CONFIG_BT_MESH_SUBNET_COUNT,
    HB_PUB_INDEX = APP_KEY_INDEX + CONFIG_BT_MESH_APP_KEY_COUNT,
    CFG_INDEX,

    MOD_BIND_INDEX,
    MOD_SUB_INDEX = MOD_BIND_INDEX + MAX_MODEL_NUMS,
    MOD_PUB_INDEX = MOD_SUB_INDEX + MAX_MODEL_NUMS,
    VND_MOD_BIND_INDEX = MOD_PUB_INDEX + MAX_MODEL_NUMS,
    VND_MOD_SUB_INDEX = VND_MOD_BIND_INDEX + MAX_MODEL_NUMS,
    VND_MOD_PUB_INDEX = VND_MOD_SUB_INDEX + MAX_MODEL_NUMS,

#if defined(CONFIG_BT_MESH_CDB)
    /* CDB_INDEX = VND_MOD_PUB_INDEX + MAX_MODEL_NUMS, */
    CDB_INDEX = VM_BLE_MESH_CDB_IDX_BEGIN,
    CDB_NODE_INDEX,
    CDB_APP_KEY_INDEX = CDB_NODE_INDEX + CONFIG_BT_MESH_CDB_NODE_COUNT,
    CDB_SUBNET_INDEX = CDB_APP_KEY_INDEX + CONFIG_BT_MESH_CDB_SUBNET_COUNT + CONFIG_BT_MESH_CDB_APP_KEY_COUNT,
    CDB_MAX_INDEX = CDB_SUBNET_INDEX + CONFIG_BT_MESH_CDB_SUBNET_COUNT + CONFIG_BT_MESH_CDB_APP_KEY_COUNT,
#endif

} NODE_INFO_SETTING_INDEX;

/* Tracking of what storage changes are pending for App and Net Keys. We
 * track this in a separate array here instead of within the respective
 * bt_mesh_app_key and bt_mesh_subnet structs themselves, since once a key
 * gets deleted its struct becomes invalid and may be reused for other keys.
 */
static struct key_update {
    u16_t key_idx: 12,   /* AppKey or NetKey Index */
          valid: 1,      /* 1 if this entry is valid, 0 if not */
          app_key: 1,    /* 1 if this is an AppKey, 0 if a NetKey */
          clear: 1;      /* 1 if key needs clearing, 0 if storing */
} key_updates[CONFIG_BT_MESH_APP_KEY_COUNT + CONFIG_BT_MESH_SUBNET_COUNT];

struct net_val {
    u16_t primary_addr;
    u8_t  dev_key[16];
} __packed;

/* Sequence number storage */
struct seq_val {
    u8_t val[3];
} __packed;

/* Heartbeat Publication storage */
struct hb_pub_val {
    u16_t dst;
    u8_t  period;
    u8_t  ttl;
    u16_t feat;
    u16_t net_idx: 12,
          indefinite: 1;
};

/* Miscelaneous configuration server model states */
struct cfg_val {
    u8_t net_transmit;
    u8_t relay;
    u8_t relay_retransmit;
    u8_t beacon;
    u8_t gatt_proxy;
    u8_t frnd;
    u8_t default_ttl;
};

/* IV Index & IV Update storage */
struct iv_val {
    u32_t iv_index;
    u8_t  iv_update: 1,
          iv_duration: 7;
} __packed;

/* Replay Protection List storage */
struct rpl_val {
    u32_t seq: 24,
          old_iv: 1;
};

/* NetKey storage information */
struct net_key_val {
    u8_t net_idx;
    u8_t kr_flag: 1,
         kr_phase: 7;
    u8_t val[2][16];
} __packed;

/* AppKey storage information */
struct app_key_val {
    u16_t app_idx;
    u16_t net_idx;
    bool  updated;
    u8_t  val[2][16];
} __packed;

struct mod_pub_val {
    u16_t addr;
    u16_t key;
    u8_t  ttl;
    u8_t  retransmit;
    u8_t  period;
    u8_t  period_div: 4,
          cred: 1;
};

/* We need this so we don't overwrite app-hardcoded values in case FCB
 * contains a history of changes but then has a NULL at the end.
 */
static struct {
    bool valid;
    struct cfg_val cfg;
} stored_cfg;

struct __rpl_val {
    u16_t src;
    struct rpl_val rpl;
};

struct __mod_bind {
    u16_t keys[CONFIG_BT_MESH_MODEL_KEY_COUNT];
};

struct __mod_sub {
    u16_t groups[CONFIG_BT_MESH_MODEL_GROUP_COUNT];
};

struct __mod_pub {
    struct mod_pub_val pub;
};

static void store_pending(void);
extern void node_info_store(int index, void *buf, u16 len);
extern void node_info_clear(int index, u16 len);
extern bool node_info_load(int index, void *buf, u16 len);

#if CONFIG_BT_MESH_PROVISIONER

/* Node storage information */
struct node_val {
    u16_t net_idx;
    u8_t  num_elem;
    u8_t  flags;
#define F_NODE_CONFIGURED 0x01
    u8_t  uuid[16];
    u8_t  dev_key[16];
    u16_t addr;
} __packed;

struct node_update {
    u16_t addr;
    bool clear;
};

struct cdb_net_val {
    u32_t iv_index;
    bool  iv_update;
} __packed;

#if defined(CONFIG_BT_MESH_CDB)
static struct node_update cdb_node_updates[CONFIG_BT_MESH_CDB_NODE_COUNT];
static struct key_update cdb_key_updates[CONFIG_BT_MESH_CDB_SUBNET_COUNT +
                                            CONFIG_BT_MESH_CDB_APP_KEY_COUNT];
#else
static struct node_update cdb_node_updates[0];
static struct key_update cdb_key_updates[0];
#endif

static void cdb_set(void);
static void clear_cdb(void);
static void store_pending_cdb(void);
static void store_pending_cdb_nodes(void);
static void store_pending_cdb_keys(void);

#endif /* CONFIG_BT_MESH_PROVISIONER */

static u8 get_model_store_index(bool vnd, u8 elem_idx, u8 mod_idx)
{
    u8 i;
    u8 store_index = 0;

    for (i = 0; i < elem_idx; i++) {
        if (vnd) {
            store_index += bt_mesh_each_elem_vendor_model_numbers_get(i);
        } else {
            store_index += bt_mesh_each_elem_sig_model_numbers_get(i);
        }
    }
    store_index += mod_idx;

    return store_index;
}

static u8 bt_mesh_model_numbers_get(bool vnd)
{
    u8 model_count = 0;

    for (u8 i = 0; i < bt_mesh_elem_count(); i++) {
        if (vnd) {
            model_count += bt_mesh_each_elem_vendor_model_numbers_get(i);
        } else {
            model_count += bt_mesh_each_elem_sig_model_numbers_get(i);
        }
    }
    ASSERT(model_count <= MAX_MODEL_NUMS, "real model numbers:%d", model_count);

    return model_count;
}

static void get_elem_and_mod_idx(bool vnd, u8 model_count, u8 *elem_idx, u8 *mod_idx)
{
    u8 i;
    u8 each_elem_model_numbers = 0;

    *elem_idx = 0;
    *mod_idx = 0;
    for (i = 0; i < bt_mesh_elem_count(); i++) {
        if (vnd) {
            each_elem_model_numbers = bt_mesh_each_elem_vendor_model_numbers_get(i);
        } else {
            each_elem_model_numbers = bt_mesh_each_elem_sig_model_numbers_get(i);
        }
        if (model_count >= each_elem_model_numbers) {
            model_count -= each_elem_model_numbers;
        } else {
            break;
        }
    }
    *elem_idx = i;
    *mod_idx = model_count;
}

static void net_set(void)
{
    BT_INFO("\n < --%s-- >", __FUNCTION__);

    struct net_val net;
    int err;

    err = node_info_load(NET_INDEX, &net, sizeof(net));
    if (err) {
        bt_mesh_comp_unprovision();
        (void)memset(bt_mesh.dev_key, 0, sizeof(bt_mesh.dev_key));
        BT_ERR("<net_set> load not exist");
        return;
    }

    memcpy(bt_mesh.dev_key, net.dev_key, sizeof(bt_mesh.dev_key));
    bt_mesh_comp_provision(net.primary_addr);

    BT_DBG("Provisioned with primary address 0x%04x", net.primary_addr);
    BT_DBG("Recovered DevKey %s", bt_hex(bt_mesh.dev_key, 16));
}

static void iv_set(void)
{
    struct iv_val iv;
    int err;

    BT_INFO("\n < --%s-- >", __FUNCTION__);
    err = node_info_load(IV_INDEX, &iv, sizeof(iv));
    if (err) {
        bt_mesh.iv_index = 0;
        bt_mesh.iv_update = 0;
        BT_ERR("<iv_set> load not exist");
        return;
    }

    bt_mesh.iv_index = iv.iv_index;
    bt_mesh.iv_update = iv.iv_update;
    bt_mesh.ivu_duration = iv.iv_duration;

    BT_DBG("IV Index 0x%04x (IV Update Flag %u) duration %u hours",
           bt_mesh.iv_index, bt_mesh.iv_update, bt_mesh.ivu_duration);
}

static void seq_set(void)
{
    struct seq_val seq;
    int err;

    BT_INFO("\n < --%s-- >", __FUNCTION__);
    err = node_info_load(SEQ_INDEX, &seq, sizeof(seq));
    if (err) {
        bt_mesh.seq = 0;
        BT_ERR("<seq_set> load not exist");
        return;
    }

    bt_mesh.seq = ((u32_t)seq.val[0] | ((u32_t)seq.val[1] << 8) |
                   ((u32_t)seq.val[2] << 16));

    if (CONFIG_BT_MESH_SEQ_STORE_RATE > 0) {
        /* Make sure we have a large enough sequence number. We
         * subtract 1 so that the first transmission causes a write
         * to the settings storage.
         */
        bt_mesh.seq += (CONFIG_BT_MESH_SEQ_STORE_RATE -
                        (bt_mesh.seq % CONFIG_BT_MESH_SEQ_STORE_RATE));
        bt_mesh.seq--;
    }

    BT_DBG("Sequence Number 0x%06x", bt_mesh.seq);
}

static struct bt_mesh_rpl *rpl_find(u16_t src)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
        if (bt_mesh.rpl[i].src == src) {
            return &bt_mesh.rpl[i];
        }
    }

    return NULL;
}

static struct bt_mesh_rpl *rpl_alloc(u16_t src)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
        if (!bt_mesh.rpl[i].src) {
            bt_mesh.rpl[i].src = src;
            return &bt_mesh.rpl[i];
        }
    }

    return NULL;
}

static void rpl_set(void)
{
    struct bt_mesh_rpl *entry;
    struct __rpl_val __rpl;
    int err;
    u8 index;

    BT_INFO("\n < --%s-- >", __FUNCTION__);

    for (index = 0; index < CONFIG_BT_MESH_CRPL; index++) {
        err = node_info_load(RPL_INDEX + index, &__rpl, sizeof(__rpl));
        if (err) {
            BT_ERR("<rpl_set> memory load fail for index:0x%x", index);
            continue;
        }

        entry = rpl_find(__rpl.src);
        if (!entry) {
            entry = rpl_alloc(__rpl.src);
            if (!entry) {
                BT_ERR("Unable to allocate RPL entry for 0x%04x", __rpl.src);
                return;
            }
        }

        entry->seq = __rpl.rpl.seq;
        entry->old_iv = __rpl.rpl.old_iv;

        BT_DBG("RPL entry for 0x%04x: Seq 0x%06x old_iv %u", entry->src,
               entry->seq, entry->old_iv);
    }
}

static void net_key_set(void)
{
    struct bt_mesh_subnet *sub;
    struct net_key_val key;
    int i, err;
    u16_t net_idx;

    BT_INFO("\n < --%s-- >", __FUNCTION__);

    for (net_idx = 0; net_idx < CONFIG_BT_MESH_SUBNET_COUNT; net_idx++) {
        err = node_info_load(NET_KEY_INDEX + net_idx, &key, sizeof(key));
        if (err) {
            BT_ERR("<net_key_set> memory load fail for net_idx:0x%x", net_idx);
            continue;
        }

        sub = bt_mesh_subnet_get(net_idx);
        if (sub) {
            BT_DBG("Updating existing NetKeyIndex 0x%03x", net_idx);

            sub->kr_flag = key.kr_flag;
            sub->kr_phase = key.kr_phase;
            memcpy(sub->keys[0].net, &key.val[0], 16);
            memcpy(sub->keys[1].net, &key.val[1], 16);

            continue;
        }

        for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
            if (bt_mesh.sub[i].net_idx == BT_MESH_KEY_UNUSED) {
                sub = &bt_mesh.sub[i];
                break;
            }
        }

        if (!sub) {
            BT_ERR("No space to allocate a new subnet");
            return;
        }

        sub->net_idx = net_idx;
        sub->kr_flag = key.kr_flag;
        sub->kr_phase = key.kr_phase;
        memcpy(sub->keys[0].net, &key.val[0], 16);
        memcpy(sub->keys[1].net, &key.val[1], 16);

        BT_DBG("NetKeyIndex 0x%03x recovered from storage", net_idx);
    }
}

static void app_key_set(void)
{
    struct bt_mesh_app_key *app;
    struct app_key_val key;
    u16_t app_idx;
    int err;

    BT_INFO("\n < --%s-- >", __FUNCTION__);

    for (app_idx = 0; app_idx < CONFIG_BT_MESH_SUBNET_COUNT; app_idx++) {
        err = node_info_load(APP_KEY_INDEX + app_idx, &key, sizeof(key));
        if (err) {
            BT_ERR("<app_key_set> memory load fail for net_idx:0x%x", app_idx);
            continue;
        }

        app = bt_mesh_app_key_find(app_idx);
        if (!app) {
            app = bt_mesh_app_key_alloc(app_idx);
        }

        if (!app) {
            BT_ERR("No space for a new app key");
            return;
        }

        app->net_idx = key.net_idx;
        app->app_idx = app_idx;
        app->updated = key.updated;
        memcpy(app->keys[0].val, key.val[0], 16);
        memcpy(app->keys[1].val, key.val[1], 16);

        bt_mesh_app_id(app->keys[0].val, &app->keys[0].id);
        bt_mesh_app_id(app->keys[1].val, &app->keys[1].id);

        BT_DBG("AppKeyIndex 0x%03x recovered from storage", app_idx);
    }
}

static void hb_pub_set(void)
{
    struct bt_mesh_hb_pub *pub = bt_mesh_hb_pub_get();
    struct hb_pub_val hb_val;
    int err;

    BT_INFO("\n < --%s-- >", __FUNCTION__);
    if (!pub) {
        BT_ERR("no heartbeat will set");
        return;
    }

    err = node_info_load(HB_PUB_INDEX, &hb_val, sizeof(hb_val));
    if (err) {
        pub->dst = BT_MESH_ADDR_UNASSIGNED;
        pub->count = 0;
        pub->ttl = 0;
        pub->period = 0;
        pub->feat = 0;

        BT_ERR("Cleared heartbeat publication");
        return;
    }

    pub->dst = hb_val.dst;
    pub->period = hb_val.period;
    pub->ttl = hb_val.ttl;
    pub->feat = hb_val.feat;
    pub->net_idx = hb_val.net_idx;

    if (hb_val.indefinite) {
        pub->count = 0xffff;
    } else {
        pub->count = 0;
    }

    BT_DBG("Restored heartbeat publication");
}

static void cfg_set(void)
{
    struct bt_mesh_cfg_srv *cfg = bt_mesh_cfg_get();
    int err;

    BT_INFO("\n < --%s-- >", __FUNCTION__);
    if (!cfg) {
        BT_ERR("no cfg will set");
        return;
    }

    err = node_info_load(CFG_INDEX, &stored_cfg.cfg, sizeof(stored_cfg.cfg));
    if (err) {
        stored_cfg.valid = false;
        BT_ERR("Cleared configuration state");
        return;
    }

    stored_cfg.valid = true;
    BT_DBG("Restored configuration state");
}

static void mod_set_bind(bool vnd, u8 model_count)
{
    int err;
    struct bt_mesh_model *mod;
    u8_t elem_idx, mod_idx;
    struct __mod_bind _mod_bind;

    u8 load_index = vnd ? VND_MOD_BIND_INDEX : MOD_BIND_INDEX;

    BT_INFO("< --%s-- >", __FUNCTION__);

    for (u8 i = 0; i < model_count; i++) {
        err = node_info_load(load_index + i, &_mod_bind, sizeof(_mod_bind));
        if (err) {
            BT_ERR("%s <mod bind> load not exist", vnd ? "Vendor" : "SIG");
            continue;
        }

        get_elem_and_mod_idx(vnd, i, &elem_idx, &mod_idx);
        BT_DBG("Decoded elem_idx:%u; mod_idx:%u",
               elem_idx, mod_idx);

        mod = bt_mesh_model_get(vnd, elem_idx, mod_idx);
        memcpy((u8 *)mod->keys, (u8 *)_mod_bind.keys, sizeof(mod->keys));

        BT_DBG("Decoded %u bound keys for model", sizeof(mod->keys) / sizeof(mod->keys[0]));
        BT_DBG("Restored model bind, keys[0]:0x%x, keys[1]:0x%x ",
               _mod_bind.keys[0], _mod_bind.keys[1]);
    }
}

static void mod_set_sub(bool vnd, u8 model_count)
{
    int err;
    struct bt_mesh_model *mod;
    u8_t elem_idx, mod_idx;
    struct __mod_sub _mod_sub;

    BT_INFO("< --%s-- >", __FUNCTION__);

    u8 load_index = vnd ? VND_MOD_SUB_INDEX : MOD_SUB_INDEX;

    for (u8 i = 0; i < model_count; i++) {
        err = node_info_load(load_index + i, &_mod_sub, sizeof(_mod_sub));
        if (err) {
            BT_ERR("%s <mod sub> load not exist", vnd ? "Vendor" : "SIG");
            continue;
        }

        get_elem_and_mod_idx(vnd, i, &elem_idx, &mod_idx);
        BT_INFO("Decoded elem_idx:%u; mod_idx:%u",
                elem_idx, mod_idx);

        mod = bt_mesh_model_get(vnd, elem_idx, mod_idx);
        memcpy((u8 *)mod->groups, (u8 *)_mod_sub.groups, sizeof(mod->groups));

        BT_INFO("Decoded %u subscribed group addresses for model",
                sizeof(mod->groups) / sizeof(mod->groups[0]));
        BT_INFO("Restored model subscribed, groups[0]:0x%x, groups[1]:0x%x ",
                _mod_sub.groups[0], _mod_sub.groups[1]);
    }
}

static void mod_set_pub(bool vnd, u8 model_count)
{
    int len, err;
    struct bt_mesh_model *mod;
    u8_t elem_idx, mod_idx;
    struct __mod_pub _mod_pub;

    u8 load_index = vnd ? VND_MOD_PUB_INDEX : MOD_PUB_INDEX;

    BT_INFO("< --%s-- >", __FUNCTION__);

    for (u8 i = 0; i < model_count; i++) {
        err = node_info_load(load_index + i, &_mod_pub, sizeof(_mod_pub));
        if (err) {
            BT_ERR("%s <mod pub> load not exist", vnd ? "Vendor" : "SIG");
            continue;
        }

        get_elem_and_mod_idx(vnd, i, &elem_idx, &mod_idx);
        BT_INFO("Decoded elem_idx:%u; mod_idx:%u",
                elem_idx, mod_idx);

        mod = bt_mesh_model_get(vnd, elem_idx, mod_idx);

        mod->pub->addr          = _mod_pub.pub.addr;
        mod->pub->key           = _mod_pub.pub.key;
        mod->pub->cred          = _mod_pub.pub.cred;
        mod->pub->ttl           = _mod_pub.pub.ttl;
        mod->pub->period        = _mod_pub.pub.period;
        mod->pub->retransmit    = _mod_pub.pub.retransmit;
        mod->pub->count         = 0;

        BT_INFO("Restored model publication, dst 0x%04x app_idx 0x%03x",
                _mod_pub.pub.addr, _mod_pub.pub.key);
    }
}

static void mod_set(bool vnd, u8 model_count)
{
    mod_set_bind(vnd, model_count);

    mod_set_sub(vnd, model_count);

    mod_set_pub(vnd, model_count);
}

static void sig_mod_set(void)
{
    BT_INFO("\n < --%s-- >", __FUNCTION__);

    u8 model_count = bt_mesh_model_numbers_get(false);
    if (model_count) {
        mod_set(false, model_count);
    } else {
        BT_INFO("no sig model exist");
    }
}

static void vnd_mod_set(void)
{
    BT_INFO("\n < --%s-- >", __FUNCTION__);

    u8 model_count = bt_mesh_model_numbers_get(true);
    if (model_count) {
        mod_set(true, model_count);
    } else {
        BT_INFO("no vendor model exist");
    }
}

const struct mesh_setting {
    void (*func)(void);
} settings[] = {
    net_set,
    iv_set,
    seq_set,
    rpl_set,
    net_key_set,
    app_key_set,
    hb_pub_set,
    cfg_set,
    sig_mod_set,
    vnd_mod_set,

#if defined(CONFIG_BT_MESH_CDB)
    cdb_set,
#endif
};

static void mesh_set(void)
{
    BT_INFO("--func=%s", __FUNCTION__);

    for (int i = 0; i < ARRAY_SIZE(settings); i++) {
        settings[i].func();
    }
}

static int subnet_init(struct bt_mesh_subnet *sub)
{
    int err;

    err = bt_mesh_net_keys_create(&sub->keys[0], sub->keys[0].net);
    if (err) {
        BT_ERR("Unable to generate keys for subnet");
        return -EIO;
    }

    if (sub->kr_phase != BT_MESH_KR_NORMAL) {
        err = bt_mesh_net_keys_create(&sub->keys[1], sub->keys[1].net);
        if (err) {
            BT_ERR("Unable to generate keys for subnet");
            (void)memset(&sub->keys[0], 0, sizeof(sub->keys[0]));
            return -EIO;
        }
    }

    if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) &&
        BT_MESH_FEATURES_IS_SUPPORT(BT_MESH_FEAT_PROXY)) {
        sub->node_id = BT_MESH_NODE_IDENTITY_STOPPED;
    } else {
        sub->node_id = BT_MESH_NODE_IDENTITY_NOT_SUPPORTED;
    }

    /* Make sure we have valid beacon data to be sent */
    bt_mesh_net_beacon_update(sub);

    return 0;
}

static void commit_mod(struct bt_mesh_model *mod, struct bt_mesh_elem *elem,
                       bool vnd, bool primary, void *user_data)
{
    if (mod->pub && mod->pub->update &&
        mod->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
        s32_t ms = bt_mesh_model_pub_period_get(mod);
        if (ms) {
            BT_DBG("Starting publish timer (period %u ms)", ms);
        }
    }
}

static int mesh_commit(void)
{
    struct bt_mesh_hb_pub *hb_pub;
    struct bt_mesh_cfg_srv *cfg;
    int i;

    BT_INFO("--func=%s", __FUNCTION__);
    BT_DBG("sub[0].net_idx=0x%x", bt_mesh.sub[0].net_idx);

    if (bt_mesh.sub[0].net_idx == BT_MESH_KEY_UNUSED) {
        /* Nothing to do since we're not yet provisioned */
        return 0;
    }

    if (IS_ENABLED(CONFIG_BT_MESH_PB_GATT)) {
        bt_mesh_proxy_prov_disable(true);
    }

    for (i = 0; i < ARRAY_SIZE(bt_mesh.sub); i++) {
        struct bt_mesh_subnet *sub = &bt_mesh.sub[i];
        int err;

        if (sub->net_idx == BT_MESH_KEY_UNUSED) {
            continue;
        }

        err = subnet_init(sub);
        if (err) {
            BT_ERR("Failed to init subnet 0x%03x", sub->net_idx);
        }
    }

    if (bt_mesh.ivu_duration < BT_MESH_IVU_MIN_HOURS) {
        // ....to do
    }

    bt_mesh_model_foreach(commit_mod, NULL);

    hb_pub = bt_mesh_hb_pub_get();
    if (hb_pub && hb_pub->dst != BT_MESH_ADDR_UNASSIGNED &&
        hb_pub->count && hb_pub->period) {
        BT_DBG("Starting heartbeat publication");
        // ....to do
    }

    cfg = bt_mesh_cfg_get();
    if (cfg && stored_cfg.valid) {
        cfg->net_transmit = stored_cfg.cfg.net_transmit;
        cfg->relay = stored_cfg.cfg.relay;
        cfg->relay_retransmit = stored_cfg.cfg.relay_retransmit;
        cfg->beacon = stored_cfg.cfg.beacon;
        cfg->gatt_proxy = stored_cfg.cfg.gatt_proxy;
        cfg->frnd = stored_cfg.cfg.frnd;
        cfg->default_ttl = stored_cfg.cfg.default_ttl;
    }

    bt_mesh.valid = 1;

    if (IS_ENABLED(CONFIG_BT_MESH_GATT_PROXY) &&
        BT_MESH_FEATURES_IS_SUPPORT(BT_MESH_FEAT_PROXY)) {
        bt_mesh_proxy_identity_enable();
    }
    bt_mesh_net_start();

    return 0;
}

static void schedule_store(int flag)
{
    s32_t timeout;

    atomic_set_bit(bt_mesh.flags, flag);

#if 0
    if (atomic_test_bit(bt_mesh.flags, BT_MESH_NET_PENDING) ||
        atomic_test_bit(bt_mesh.flags, BT_MESH_IV_PENDING) ||
        atomic_test_bit(bt_mesh.flags, BT_MESH_SEQ_PENDING)) {
        timeout = K_NO_WAIT;
    } else if (atomic_test_bit(bt_mesh.flags, BT_MESH_RPL_PENDING) &&
               (CONFIG_BT_MESH_RPL_STORE_TIMEOUT <
                CONFIG_BT_MESH_STORE_TIMEOUT)) {
        timeout = K_SECONDS(CONFIG_BT_MESH_RPL_STORE_TIMEOUT);
    } else {
        timeout = K_SECONDS(CONFIG_BT_MESH_STORE_TIMEOUT);
    }

    BT_DBG("Waiting %d seconds", timeout / MSEC_PER_SEC);
#endif

    /* OS_ENTER_CRITICAL(); */
    store_pending();
    /* OS_EXIT_CRITICAL(); */
}

static void clear_iv(void)
{
    BT_INFO("--func=%s", __FUNCTION__);

    node_info_clear(IV_INDEX, sizeof(struct iv_val));
}

static void clear_net(void)
{
    BT_INFO("--func=%s", __FUNCTION__);

    node_info_clear(NET_INDEX, sizeof(struct net_val));
}

static void store_pending_net(void)
{
    struct net_val net;

    BT_INFO("--func=%s", __FUNCTION__);
    BT_DBG("addr 0x%04x DevKey %s", bt_mesh_primary_addr(),
           bt_hex(bt_mesh.dev_key, 16));

    net.primary_addr = bt_mesh_primary_addr();
    memcpy(net.dev_key, bt_mesh.dev_key, 16);

    node_info_store(NET_INDEX, &net, sizeof(net));
}

void bt_mesh_store_net(void)
{
    schedule_store(BT_MESH_NET_PENDING);
}

static void store_pending_iv(void)
{
    BT_INFO("--func=%s", __FUNCTION__);
    struct iv_val iv;

    iv.iv_index = bt_mesh.iv_index;
    iv.iv_update = bt_mesh.iv_update;
    iv.iv_duration = bt_mesh.ivu_duration;

    node_info_store(IV_INDEX, &iv, sizeof(iv));
}

void bt_mesh_store_iv(bool only_duration)
{
    schedule_store(BT_MESH_IV_PENDING);

    if (!only_duration) {
        /* Always update Seq whenever IV changes */
        schedule_store(BT_MESH_SEQ_PENDING);
    }
}

static void store_pending_seq(void)
{
    BT_INFO("--func=%s", __FUNCTION__);
    struct seq_val seq;

    seq.val[0] = bt_mesh.seq;
    seq.val[1] = bt_mesh.seq >> 8;
    seq.val[2] = bt_mesh.seq >> 16;

    node_info_store(SEQ_INDEX, &seq, sizeof(seq));
}

void bt_mesh_store_seq(void)
{
    if (CONFIG_BT_MESH_SEQ_STORE_RATE &&
        (bt_mesh.seq % CONFIG_BT_MESH_SEQ_STORE_RATE)) {
        return;
    }

    schedule_store(BT_MESH_SEQ_PENDING);
}

static void store_rpl(struct bt_mesh_rpl *entry, u8 index)
{
    struct __rpl_val __rpl;

    BT_DBG("src 0x%04x seq 0x%06x old_iv %u", entry->src, entry->seq,
           entry->old_iv);

    __rpl.rpl.seq = entry->seq;
    __rpl.rpl.old_iv = entry->old_iv;
    __rpl.src = entry->src;

    node_info_store(RPL_INDEX + index, &__rpl, sizeof(__rpl));
}

static void clear_rpl(void)
{
    int i;

    BT_INFO("--func=%s", __FUNCTION__);

    for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
        struct bt_mesh_rpl *rpl = &bt_mesh.rpl[i];

        if (!rpl->src) {
            continue;
        }

        node_info_clear(RPL_INDEX + i, sizeof(struct __rpl_val));

        (void)memset(rpl, 0, sizeof(*rpl));
    }
}

static void store_pending_rpl(void)
{
    int i;

    BT_INFO("--func=%s", __FUNCTION__);

    for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
        struct bt_mesh_rpl *rpl = &bt_mesh.rpl[i];

        if (rpl->store) {
            rpl->store = false;
            store_rpl(rpl, i);
        }
    }
}

static void store_pending_hb_pub(void)
{
    BT_INFO("--func=%s", __FUNCTION__);
    struct bt_mesh_hb_pub *pub = bt_mesh_hb_pub_get();
    struct hb_pub_val val;

    if (!pub) {
        return;
    }

    if (pub->dst == BT_MESH_ADDR_UNASSIGNED) {
        node_info_clear(HB_PUB_INDEX, sizeof(struct hb_pub_val));
    } else {
        val.indefinite = (pub->count = 0xffff);
        val.dst = pub->dst;
        val.period = pub->period;
        val.ttl = pub->ttl;
        val.feat = pub->feat;
        val.net_idx = pub->net_idx;

        node_info_store(HB_PUB_INDEX, &val, sizeof(val));
    }
}

static void store_pending_cfg(void)
{
    BT_INFO("--func=%s", __FUNCTION__);
    struct bt_mesh_cfg_srv *cfg = bt_mesh_cfg_get();
    struct cfg_val val;

    if (!cfg) {
        return;
    }

    val.net_transmit = cfg->net_transmit;
    val.relay = cfg->relay;
    val.relay_retransmit = cfg->relay_retransmit;
    val.beacon = cfg->beacon;
    val.gatt_proxy = cfg->gatt_proxy;
    val.frnd = cfg->frnd;
    val.default_ttl = cfg->default_ttl;

    node_info_store(CFG_INDEX, &val, sizeof(val));
}

static void clear_cfg(void)
{
    BT_INFO("--func=%s", __FUNCTION__);

    node_info_clear(CFG_INDEX, sizeof(struct cfg_val));
}

static void clear_app_key(u16_t app_idx)
{
    BT_INFO("--func=%s", __FUNCTION__);

    BT_DBG("AppKeyIndex 0x%03x", app_idx);

    node_info_clear(APP_KEY_INDEX + app_idx, sizeof(struct app_key_val));
}

static void clear_net_key(u16_t net_idx)
{
    BT_INFO("--func=%s", __FUNCTION__);
    BT_DBG("NetKeyIndex 0x%03x", net_idx);

    node_info_clear(NET_KEY_INDEX + net_idx, sizeof(struct net_key_val));
}

static void store_net_key(struct bt_mesh_subnet *sub)
{
    struct net_key_val key;

    BT_INFO("--func=%s", __FUNCTION__);
    BT_DBG("NetKeyIndex 0x%03x NetKey %s", sub->net_idx,
           bt_hex(sub->keys[0].net, 16));

    memcpy(&key.val[0], sub->keys[0].net, 16);
    memcpy(&key.val[1], sub->keys[1].net, 16);
    key.kr_flag = sub->kr_flag;
    key.kr_phase = sub->kr_phase;

    BT_INFO("sub->net_idx=0x%x", sub->net_idx);
    node_info_store(NET_KEY_INDEX + sub->net_idx, &key, sizeof(key));
}

static void store_app_key(struct bt_mesh_app_key *app)
{
    struct app_key_val key;

    BT_INFO("--func=%s", __FUNCTION__);
    key.net_idx = app->net_idx;
    key.updated = app->updated;
    memcpy(key.val[0], app->keys[0].val, 16);
    memcpy(key.val[1], app->keys[1].val, 16);

    BT_INFO("app->net_idx=0x%x", app->net_idx);
    node_info_store(APP_KEY_INDEX + app->net_idx, &key, sizeof(key));
}

static void store_pending_keys(void)
{
    int i;

    BT_INFO("--func=%s", __FUNCTION__);

    for (i = 0; i < ARRAY_SIZE(key_updates); i++) {
        struct key_update *update = &key_updates[i];

        if (!update->valid) {
            continue;
        }

        if (update->clear) {
            if (update->app_key) {
                clear_app_key(update->key_idx);
            } else {
                clear_net_key(update->key_idx);
            }
        } else {
            if (update->app_key) {
                struct bt_mesh_app_key *key;

                key = bt_mesh_app_key_find(update->key_idx);
                if (key) {
                    store_app_key(key);
                } else {
                    BT_WARN("AppKeyIndex 0x%03x not found",
                            update->key_idx);
                }

            } else {
                struct bt_mesh_subnet *sub;

                sub = bt_mesh_subnet_get(update->key_idx);
                if (sub) {
                    store_net_key(sub);
                } else {
                    BT_WARN("NetKeyIndex 0x%03x not found",
                            update->key_idx);
                }
            }
        }

        update->valid = 0;
    }
}

static void store_pending_mod_bind(struct bt_mesh_model *mod, bool vnd)
{
    int i, count;
    struct __mod_bind _mod_bind;
    u8 store_index;

    BT_INFO("--func=%s", __FUNCTION__);
    for (i = 0, count = 0; i < ARRAY_SIZE(mod->keys); i++) {
        _mod_bind.keys[i] = BT_MESH_KEY_UNUSED;
        if (mod->keys[i] != BT_MESH_KEY_UNUSED) {
            _mod_bind.keys[count++] = mod->keys[i];
        }
    }
    store_index = get_model_store_index(vnd, mod->elem_idx, mod->mod_idx);
    store_index += vnd ? VND_MOD_BIND_INDEX : MOD_BIND_INDEX;
    if (count) {
        node_info_store(store_index, &_mod_bind, sizeof(_mod_bind));
    } else {
        node_info_clear(store_index, sizeof(struct __mod_bind));
    }
}

static void store_pending_mod_sub(struct bt_mesh_model *mod, bool vnd)
{
    int i, count;
    struct __mod_sub _mod_sub;
    u8 store_index;

    BT_INFO("--func=%s", __FUNCTION__);
    for (i = 0, count = 0; i < ARRAY_SIZE(mod->groups); i++) {
        _mod_sub.groups[i] = BT_MESH_ADDR_UNASSIGNED;
        if (mod->groups[i] != BT_MESH_ADDR_UNASSIGNED) {
            _mod_sub.groups[count++] = mod->groups[i];
        }
    }

    store_index = get_model_store_index(vnd, mod->elem_idx, mod->mod_idx);
    store_index += vnd ? VND_MOD_SUB_INDEX : MOD_SUB_INDEX;
    if (count) {
        node_info_store(store_index, &_mod_sub, sizeof(_mod_sub));
    } else {
        node_info_clear(store_index, sizeof(struct __mod_sub));
    }
}

static void store_pending_mod_pub(struct bt_mesh_model *mod, bool vnd)
{
    struct __mod_pub _mod_pub;
    u8 store_index;

    BT_INFO("--func=%s", __FUNCTION__);
    store_index = get_model_store_index(vnd, mod->elem_idx, mod->mod_idx);
    store_index += vnd ? VND_MOD_PUB_INDEX : MOD_PUB_INDEX;
    if (!mod->pub || mod->pub->addr == BT_MESH_ADDR_UNASSIGNED) {
        node_info_clear(store_index, sizeof(struct __mod_pub));
    } else {
        _mod_pub.pub.addr = mod->pub->addr;
        _mod_pub.pub.key = mod->pub->key;
        _mod_pub.pub.ttl = mod->pub->ttl;
        _mod_pub.pub.retransmit = mod->pub->retransmit;
        _mod_pub.pub.period = mod->pub->period;
        _mod_pub.pub.period_div = mod->pub->period_div;
        _mod_pub.pub.cred = mod->pub->cred;

        node_info_store(store_index, &_mod_pub, sizeof(_mod_pub));
    }
}

static void store_pending_mod(struct bt_mesh_model *mod,
                              struct bt_mesh_elem *elem, bool vnd,
                              bool primary, void *user_data)
{
    if (!mod->flags) {
        return;
    }

    if (mod->flags & BT_MESH_MOD_BIND_PENDING) {
        mod->flags &= ~BT_MESH_MOD_BIND_PENDING;
        store_pending_mod_bind(mod, vnd);
    }

    if (mod->flags & BT_MESH_MOD_SUB_PENDING) {
        mod->flags &= ~BT_MESH_MOD_SUB_PENDING;
        store_pending_mod_sub(mod, vnd);
    }

    if (mod->flags & BT_MESH_MOD_PUB_PENDING) {
        mod->flags &= ~BT_MESH_MOD_PUB_PENDING;
        store_pending_mod_pub(mod, vnd);
    }
}

static void store_pending(void)
{
    BT_INFO("--func=%s", __FUNCTION__);

    if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_RPL_PENDING)) {
        if (bt_mesh.valid) {
            store_pending_rpl();
        } else {
            clear_rpl();
        }
    }

    if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_KEYS_PENDING)) {
        store_pending_keys();
    }

    if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_NET_PENDING)) {
        if (bt_mesh.valid) {
            store_pending_net();
        } else {
            clear_net();
        }
    }

    if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_IV_PENDING)) {
        if (bt_mesh.valid) {
            store_pending_iv();
        } else {
            clear_iv();
        }
    }

    if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_SEQ_PENDING)) {
        store_pending_seq();
    }

    if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_HB_PUB_PENDING)) {
        store_pending_hb_pub();
    }

    if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_CFG_PENDING)) {
        if (bt_mesh.valid) {
            store_pending_cfg();
        } else {
            clear_cfg();
        }
    }

    if (atomic_test_and_clear_bit(bt_mesh.flags, BT_MESH_MOD_PENDING)) {
        bt_mesh_model_foreach(store_pending_mod, NULL);
    }

#if CONFIG_BT_MESH_PROVISIONER
    if (IS_ENABLED(CONFIG_BT_MESH_CDB)) {
        if (atomic_test_and_clear_bit(bt_mesh_cdb.flags, BT_MESH_CDB_SUBNET_PENDING)) {
            if (atomic_test_bit(bt_mesh_cdb.flags, BT_MESH_CDB_VALID)) {
                store_pending_cdb();
            } else {
                clear_cdb();
            }
        }

        if (atomic_test_and_clear_bit(bt_mesh_cdb.flags, BT_MESH_CDB_NODES_PENDING)) {
            store_pending_cdb_nodes();
        }

        if (atomic_test_and_clear_bit(bt_mesh_cdb.flags, BT_MESH_CDB_KEYS_PENDING)) {
            store_pending_cdb_keys();
        }
    }
#endif /* CONFIG_BT_MESH_PROVISIONER */
}

void bt_mesh_store_rpl(struct bt_mesh_rpl *entry)
{
    entry->store = true;
    schedule_store(BT_MESH_RPL_PENDING);
}

static struct key_update *key_update_find(bool app_key, u16_t key_idx,
        struct key_update **free_slot)
{
    struct key_update *match;
    int i;

    match = NULL;
    *free_slot = NULL;

    for (i = 0; i < ARRAY_SIZE(key_updates); i++) {
        struct key_update *update = &key_updates[i];

        if (!update->valid) {
            *free_slot = update;
            continue;
        }

        if (update->app_key != app_key) {
            continue;
        }

        if (update->key_idx == key_idx) {
            match = update;
        }
    }

    return match;
}

void bt_mesh_store_subnet(struct bt_mesh_subnet *sub)
{
    struct key_update *update, *free_slot;

    BT_DBG("NetKeyIndex 0x%03x", sub->net_idx);

    update = key_update_find(false, sub->net_idx, &free_slot);
    if (update) {
        update->clear = 0;
        schedule_store(BT_MESH_KEYS_PENDING);
        return;
    }

    if (!free_slot) {
        store_net_key(sub);
        return;
    }

    free_slot->valid = 1;
    free_slot->key_idx = sub->net_idx;
    free_slot->app_key = 0;
    free_slot->clear = 0;

    schedule_store(BT_MESH_KEYS_PENDING);
}

void bt_mesh_store_app_key(struct bt_mesh_app_key *key)
{
    struct key_update *update, *free_slot;

    BT_DBG("AppKeyIndex 0x%03x", key->app_idx);

    update = key_update_find(true, key->app_idx, &free_slot);
    if (update) {
        update->clear = 0;
        schedule_store(BT_MESH_KEYS_PENDING);
        return;
    }

    if (!free_slot) {
        store_app_key(key);
        return;
    }

    free_slot->valid = 1;
    free_slot->key_idx = key->app_idx;
    free_slot->app_key = 1;
    free_slot->clear = 0;

    schedule_store(BT_MESH_KEYS_PENDING);
}

void bt_mesh_store_hb_pub(void)
{
    schedule_store(BT_MESH_HB_PUB_PENDING);
}

void bt_mesh_store_cfg(void)
{
    schedule_store(BT_MESH_CFG_PENDING);
}

void bt_mesh_clear_net(void)
{
    schedule_store(BT_MESH_NET_PENDING);
    schedule_store(BT_MESH_IV_PENDING);
    schedule_store(BT_MESH_CFG_PENDING);
}

void bt_mesh_clear_subnet(struct bt_mesh_subnet *sub)
{
    struct key_update *update, *free_slot;

    BT_DBG("NetKeyIndex 0x%03x", sub->net_idx);

    update = key_update_find(false, sub->net_idx, &free_slot);
    if (update) {
        update->clear = 1;
        schedule_store(BT_MESH_KEYS_PENDING);
        return;
    }

    if (!free_slot) {
        clear_net_key(sub->net_idx);
        return;
    }

    free_slot->valid = 1;
    free_slot->key_idx = sub->net_idx;
    free_slot->app_key = 0;
    free_slot->clear = 1;

    schedule_store(BT_MESH_KEYS_PENDING);
}

void bt_mesh_clear_app_key(struct bt_mesh_app_key *key)
{
    struct key_update *update, *free_slot;

    BT_DBG("AppKeyIndex 0x%03x", key->app_idx);

    update = key_update_find(true, key->app_idx, &free_slot);
    if (update) {
        update->clear = 1;
        schedule_store(BT_MESH_KEYS_PENDING);
        return;
    }

    if (!free_slot) {
        clear_app_key(key->app_idx);
        return;
    }

    free_slot->valid = 1;
    free_slot->key_idx = key->app_idx;
    free_slot->app_key = 1;
    free_slot->clear = 1;

    schedule_store(BT_MESH_KEYS_PENDING);
}

void bt_mesh_clear_rpl(void)
{
    schedule_store(BT_MESH_RPL_PENDING);
}

void bt_mesh_store_mod_bind(struct bt_mesh_model *mod)
{
    mod->flags |= BT_MESH_MOD_BIND_PENDING;
    schedule_store(BT_MESH_MOD_PENDING);
}

void bt_mesh_store_mod_sub(struct bt_mesh_model *mod)
{
    mod->flags |= BT_MESH_MOD_SUB_PENDING;
    schedule_store(BT_MESH_MOD_PENDING);
}

void bt_mesh_store_mod_pub(struct bt_mesh_model *mod)
{
    mod->flags |= BT_MESH_MOD_PUB_PENDING;
    schedule_store(BT_MESH_MOD_PENDING);
}

void bt_mesh_settings_init(void)
{
}

void settings_load(void)
{
    mesh_set();

    mesh_commit();
}

#if CONFIG_BT_MESH_PROVISIONER

static void cdb_net_set(void)
{
    BT_INFO("\n < --%s-- >", __FUNCTION__);

    struct cdb_net_val net;
    int err;

    err = node_info_load(CDB_INDEX, &net, sizeof(net));
    if (err) {
        BT_ERR("<cdb_net_set> load fail, index:0x%x", index);
        return;
    }

    bt_mesh_cdb.iv_index = net.iv_index;

    if (net.iv_update) {
        atomic_set_bit(bt_mesh_cdb.flags, BT_MESH_CDB_IVU_IN_PROGRESS);
    }

    atomic_set_bit(bt_mesh_cdb.flags, BT_MESH_CDB_VALID);
}

static void cdb_node_set(void)
{
    BT_INFO("\n < --%s-- >", __FUNCTION__);

    struct bt_mesh_cdb_node *node;
    struct node_val val;
    u16_t addr;
    int err;

    for (u8 index = 0; index < CONFIG_BT_MESH_CDB_NODE_COUNT; index++) {
        err = node_info_load(CDB_NODE_INDEX + index, &val, sizeof(val));
        addr = val.addr;
        if (err) {
            /* BT_ERR("Failed to set \'node\'"); */
            /* BT_DBG("Deleting node 0x%04x", addr); */
            /* node = bt_mesh_cdb_node_get(addr); */
            /* if (node) { */
            /*     bt_mesh_cdb_node_del(node, false); */
            /* } */
            BT_ERR("<cdb_node_set> load fail, index:0x%x", index);
            continue;
        }

        node = bt_mesh_cdb_node_get(addr);
        if (!node) {
            node = bt_mesh_cdb_node_alloc(val.uuid, addr, val.num_elem, val.net_idx);
        }

        if (!node) {
            BT_ERR("No space for a new node");
            return;
        }

        if (val.flags & F_NODE_CONFIGURED) {
            atomic_set_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED);
        }

        memcpy(node->uuid, val.uuid, 16);
        memcpy(node->dev_key, val.dev_key, 16);

        BT_DBG("Node 0x%04x recovered from storage", addr);
    }
}

static void cdb_subnet_set(void)
{
    struct bt_mesh_cdb_subnet *sub;
    struct net_key_val key;
    u16_t net_idx;
    int err;

    for (u8 index = 0; index < (CONFIG_BT_MESH_CDB_SUBNET_COUNT + CONFIG_BT_MESH_CDB_APP_KEY_COUNT); index++) {
        err = node_info_load(CDB_SUBNET_INDEX + index, &key, sizeof(key));
        if (err) {
            /* BT_ERR("Failed to set \'net-key\'"); */
            /* BT_DBG("Deleting NetKeyIndex 0x%03x", net_idx); */
            /* bt_mesh_cdb_subnet_del(sub, false); */
            BT_ERR("<cdb_subnet_set> load fail, index:0x%x", index);
            continue;
        }

        net_idx = key.net_idx;

        sub = bt_mesh_cdb_subnet_get(net_idx);

        if (sub) {
            BT_DBG("Updating existing NetKeyIndex 0x%03x", net_idx);

            sub->kr_flag = key.kr_flag;
            sub->kr_phase = key.kr_phase;
            memcpy(sub->keys[0].net_key, &key.val[0], 16);
            memcpy(sub->keys[1].net_key, &key.val[1], 16);

            continue;
        }

        sub = bt_mesh_cdb_subnet_alloc(net_idx);
        if (!sub) {
            BT_ERR("No space to allocate a new subnet");
            return;
        }

        sub->kr_flag = key.kr_flag;
        sub->kr_phase = key.kr_phase;
        memcpy(sub->keys[0].net_key, &key.val[0], 16);
        memcpy(sub->keys[1].net_key, &key.val[1], 16);

        BT_DBG("NetKeyIndex 0x%03x recovered from storage", net_idx);
    }
}

static void cdb_app_key_set(void)
{
    struct bt_mesh_cdb_app_key *app;
    struct app_key_val key;
    u16_t app_idx;
    int err;

    for (u8 index = 0; index < (CONFIG_BT_MESH_CDB_SUBNET_COUNT + CONFIG_BT_MESH_CDB_APP_KEY_COUNT); index++) {
        err = node_info_load(CDB_APP_KEY_INDEX + index, &key, sizeof(key));
        if (err) {
            /* BT_ERR("Failed to set \'app-key\'"); */
            /* BT_DBG("val (null)"); */
            /* BT_DBG("Deleting AppKeyIndex 0x%03x", app_idx); */
            /* app = bt_mesh_cdb_app_key_get(app_idx); */
            /* if (app) { */
            /*     bt_mesh_cdb_app_key_del(app, false); */
            /* } */
            BT_ERR("<cdb_app_key_set> load fail, index:0x%x", index);
            continue;
        }

        app_idx = key.app_idx;

        app = bt_mesh_cdb_app_key_get(app_idx);
        if (!app) {
            app = bt_mesh_cdb_app_key_alloc(key.net_idx, app_idx);
        }

        if (!app) {
            BT_ERR("No space for a new app key");
            return;
        }

        memcpy(app->keys[0].app_key, key.val[0], 16);
        memcpy(app->keys[1].app_key, key.val[1], 16);

        BT_DBG("AppKeyIndex 0x%03x recovered from storage", app_idx);
    }
}

static void cdb_set(void)
{
    cdb_net_set();

    cdb_node_set();

    cdb_subnet_set();

    cdb_app_key_set();
}

static void clear_cdb(void)
{
    BT_INFO("--func=%s", __FUNCTION__);

    node_info_clear(CDB_INDEX, sizeof(struct cdb_net_val));
}

static void store_pending_cdb(void)
{
    struct cdb_net_val net;

    BT_INFO("--func=%s", __FUNCTION__);

    net.iv_index = bt_mesh_cdb.iv_index;
    net.iv_update = atomic_test_bit(bt_mesh_cdb.flags,
                                    BT_MESH_CDB_IVU_IN_PROGRESS);

    node_info_store(CDB_INDEX, &net, sizeof(net));

    BT_DBG("Stored Network value");
}

static void clear_cdb_node(u16_t addr)
{
    BT_INFO("--func=%s", __FUNCTION__);

    u8 index = bt_mesh_cdb_node_get(addr) - bt_mesh_cdb.nodes;

    BT_DBG("clear index 0x%x", index);

    node_info_clear(CDB_NODE_INDEX + index, sizeof(struct node_val));

    BT_DBG("Cleared Node 0x%04x", addr);
}

static void store_cdb_node(const struct bt_mesh_cdb_node *node)
{
    BT_INFO("--func=%s", __FUNCTION__);

    struct node_val val;
    u8 index = node - bt_mesh_cdb.nodes;

    BT_DBG("store index 0x%x", index);

    val.net_idx = node->net_idx;
    val.num_elem = node->num_elem;
    val.flags = 0;
    val.addr = node->addr;

    if (atomic_test_bit(node->flags, BT_MESH_CDB_NODE_CONFIGURED)) {
        val.flags |= F_NODE_CONFIGURED;
    }

    memcpy(val.uuid, node->uuid, 16);
    memcpy(val.dev_key, node->dev_key, 16);

    node_info_store(CDB_NODE_INDEX + index, &val, sizeof(val));

    BT_DBG("Store Node 0x%04x", val.addr);
}

static void store_pending_cdb_nodes(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(cdb_node_updates); ++i) {
        struct node_update *update = &cdb_node_updates[i];

        if (update->addr == BT_MESH_ADDR_UNASSIGNED) {
            continue;
        }

        BT_DBG("addr: 0x%04x, clear: %d", update->addr, update->clear);

        if (update->clear) {
            clear_cdb_node(update->addr);
        } else {
            struct bt_mesh_cdb_node *node;

            node = bt_mesh_cdb_node_get(update->addr);
            if (node) {
                store_cdb_node(node);
            } else {
                BT_WARN("Node 0x%04x not found", update->addr);
            }
        }

        update->addr = BT_MESH_ADDR_UNASSIGNED;
    }
}

static void clear_cdb_app_key(u16_t app_idx)
{
    BT_INFO("--func=%s", __FUNCTION__);

    node_info_clear(CDB_APP_KEY_INDEX + app_idx, sizeof(struct app_key_val));

    BT_DBG("Cleared AppKeyIndex 0x%03x", app_idx);
}

static void store_cdb_app_key(const struct bt_mesh_cdb_app_key *app)
{
    BT_INFO("--func=%s", __FUNCTION__);

    struct app_key_val key;

    key.net_idx = app->net_idx;
    key.updated = false;
    key.app_idx = app->app_idx;
    memcpy(key.val[0], app->keys[0].app_key, 16);
    memcpy(key.val[1], app->keys[1].app_key, 16);

    node_info_store(CDB_APP_KEY_INDEX + app->app_idx, &key, sizeof(key));
}

static void clear_cdb_subnet(u16_t net_idx)
{
    BT_INFO("--func=%s", __FUNCTION__);

    node_info_clear(CDB_SUBNET_INDEX + net_idx, sizeof(struct net_key_val));

    BT_DBG("Cleared NetKeyIndex 0x%03x", net_idx);
}

static void store_cdb_subnet(const struct bt_mesh_cdb_subnet *sub)
{
    BT_INFO("--func=%s", __FUNCTION__);

    struct net_key_val key;

    BT_DBG("NetKeyIndex 0x%03x NetKey %s", sub->net_idx,
           bt_hex(sub->keys[0].net_key, 16));

    memcpy(&key.val[0], sub->keys[0].net_key, 16);
    memcpy(&key.val[1], sub->keys[1].net_key, 16);
    key.kr_flag = sub->kr_flag;
    key.kr_phase = sub->kr_phase;
    key.net_idx = sub->net_idx;

    node_info_store(CDB_SUBNET_INDEX + sub->net_idx, &key, sizeof(key));
}

static void store_pending_cdb_keys(void)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(cdb_key_updates); i++) {
        struct key_update *update = &cdb_key_updates[i];

        if (!update->valid) {
            continue;
        }

        if (update->clear) {
            if (update->app_key) {
                clear_cdb_app_key(update->key_idx);
            } else {
                clear_cdb_subnet(update->key_idx);
            }
        } else {
            if (update->app_key) {
                struct bt_mesh_cdb_app_key *key;

                key = bt_mesh_cdb_app_key_get(update->key_idx);
                if (key) {
                    store_cdb_app_key(key);
                } else {
                    BT_WARN("AppKeyIndex 0x%03x not found", update->key_idx);
                }
            } else {
                struct bt_mesh_cdb_subnet *sub;

                sub = bt_mesh_cdb_subnet_get(update->key_idx);
                if (sub) {
                    store_cdb_subnet(sub);
                } else {
                    BT_WARN("NetKeyIndex 0x%03x not found", update->key_idx);
                }
            }
        }

        update->valid = 0U;
    }
}

static void schedule_cdb_store(int flag)
{
    atomic_set_bit(bt_mesh_cdb.flags, flag);

    store_pending();
}

void bt_mesh_store_cdb(void)
{
    schedule_cdb_store(BT_MESH_CDB_SUBNET_PENDING);
}

/* TODO: Could be shared with key_update_find? */
static struct key_update *cdb_key_update_find(bool app_key, u16_t key_idx,
        struct key_update **free_slot)
{
    struct key_update *match;
    int i;

    match = NULL;
    *free_slot = NULL;

    for (i = 0; i < ARRAY_SIZE(cdb_key_updates); i++) {
        struct key_update *update = &cdb_key_updates[i];

        if (!update->valid) {
            *free_slot = update;
            continue;
        }

        if (update->app_key != app_key) {
            continue;
        }

        if (update->key_idx == key_idx) {
            match = update;
        }
    }

    return match;
}

void bt_mesh_store_cdb_subnet(const struct bt_mesh_cdb_subnet *sub)
{
    struct key_update *update, *free_slot;

    BT_DBG("NetKeyIndex 0x%03x", sub->net_idx);

    update = cdb_key_update_find(false, sub->net_idx, &free_slot);
    if (update) {
        update->clear = 0U;
        schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
        return;
    }

    if (!free_slot) {
        store_cdb_subnet(sub);
        return;
    }

    free_slot->valid = 1U;
    free_slot->key_idx = sub->net_idx;
    free_slot->app_key = 0U;
    free_slot->clear = 0U;

    schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
}

void bt_mesh_clear_cdb_subnet(struct bt_mesh_cdb_subnet *sub)
{
    struct key_update *update, *free_slot;

    BT_DBG("NetKeyIndex 0x%03x", sub->net_idx);

    update = cdb_key_update_find(false, sub->net_idx, &free_slot);
    if (update) {
        update->clear = 1U;
        schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
        return;
    }

    if (!free_slot) {
        clear_cdb_subnet(sub->net_idx);
        return;
    }

    free_slot->valid = 1U;
    free_slot->key_idx = sub->net_idx;
    free_slot->app_key = 0U;
    free_slot->clear = 1U;

    schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
}

static struct node_update *cdb_node_update_find(u16_t addr,
        struct node_update **free_slot)
{
    struct node_update *match;
    int i;

    match = NULL;
    *free_slot = NULL;

    for (i = 0; i < ARRAY_SIZE(cdb_node_updates); i++) {
        struct node_update *update = &cdb_node_updates[i];

        if (update->addr == BT_MESH_ADDR_UNASSIGNED) {
            *free_slot = update;
            continue;
        }

        if (update->addr == addr) {
            match = update;
        }
    }

    return match;
}

void bt_mesh_clear_cdb_node(struct bt_mesh_cdb_node *node)
{
    struct node_update *update, *free_slot;

    BT_DBG("Node 0x%04x", node->addr);

    update = cdb_node_update_find(node->addr, &free_slot);
    if (update) {
        update->clear = true;
        schedule_cdb_store(BT_MESH_CDB_NODES_PENDING);
        return;
    }

    if (!free_slot) {
        clear_cdb_node(node->addr);
        return;
    }

    free_slot->addr = node->addr;
    free_slot->clear = true;

    schedule_cdb_store(BT_MESH_CDB_NODES_PENDING);
}

void bt_mesh_store_cdb_node(const struct bt_mesh_cdb_node *node)
{
    struct node_update *update, *free_slot;

    BT_DBG("Node 0x%04x", node->addr);

    update = cdb_node_update_find(node->addr, &free_slot);
    if (update) {
        update->clear = false;
        schedule_cdb_store(BT_MESH_CDB_NODES_PENDING);
        return;
    }

    if (!free_slot) {
        store_cdb_node(node);
        return;
    }

    free_slot->addr = node->addr;
    free_slot->clear = false;

    schedule_cdb_store(BT_MESH_CDB_NODES_PENDING);
}

void bt_mesh_clear_cdb_app_key(struct bt_mesh_cdb_app_key *key)
{
    struct key_update *update, *free_slot;

    BT_DBG("AppKeyIndex 0x%03x", key->app_idx);

    update = cdb_key_update_find(true, key->app_idx, &free_slot);
    if (update) {
        update->clear = 1U;
        schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
        return;
    }

    if (!free_slot) {
        clear_cdb_app_key(key->app_idx);
        return;
    }

    free_slot->valid = 1U;
    free_slot->key_idx = key->app_idx;
    free_slot->app_key = 1U;
    free_slot->clear = 1U;

    schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
}

void bt_mesh_store_cdb_app_key(const struct bt_mesh_cdb_app_key *key)
{
    struct key_update *update, *free_slot;

    BT_DBG("AppKeyIndex 0x%03x", key->app_idx);

    update = cdb_key_update_find(true, key->app_idx, &free_slot);
    if (update) {
        update->clear = 0U;
        schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
        return;
    }

    if (!free_slot) {
        store_cdb_app_key(key);
        return;
    }

    free_slot->valid = 1U;
    free_slot->key_idx = key->app_idx;
    free_slot->app_key = 1U;
    free_slot->clear = 0U;

    schedule_cdb_store(BT_MESH_CDB_KEYS_PENDING);
}

#endif /* CONFIG_BT_MESH_PROVISIONER */
