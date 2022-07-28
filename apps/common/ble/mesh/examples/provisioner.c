/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "btstack/bluetooth.h"
#include "bt_common.h"
#include "api/sig_mesh_api.h"
#include "../model_api.h"
#include "adaptation.h"
#include "mesh_net.h"
#include "event/key_event.h"
#include "system/timer.h"

#define LOG_TAG             "[Mesh-Provisioner]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if CONFIG_BLE_MESH_ENABLE && (CONFIG_MESH_MODEL == SIG_MESH_PROVISIONER)

extern u16_t primary_addr;
extern void mesh_setup(void (*init_cb)(void));
extern void bt_mac_addr_set(u8 *bt_addr);
extern void prov_complete(u16_t net_idx, u16_t addr);
extern void prov_reset(void);
extern void prov_node_del(u16_t addr);

/**
 * @brief Config current node features(Relay/Proxy/Friend/Low Power)
 */
/*-----------------------------------------------------------*/
#define BT_MESH_FEAT_SUPPORTED_TEMP         ( \
                                                0 \
                                            )
#include "../feature_correct.h"
const int config_bt_mesh_features = BT_MESH_FEAT_SUPPORTED;

/**
 * @brief Config proxy connectable adv hardware param
 */
/*-----------------------------------------------------------*/
#if BT_MESH_FEATURES_GET(BT_MESH_FEAT_LOW_POWER)
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(3000); // unit: ms
#else
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(300); // unit: ms
#endif /* BT_MESH_FEATURES_GET(BT_MESH_FEAT_LOW_POWER) */

/**
 * @brief Conifg complete local name
 */
/*-----------------------------------------------------------*/
#define BLE_DEV_NAME        'P', 'r', 'o', 'v', 'i', 't', 'i', 'o', 'n', 'e', 'r'

const uint8_t mesh_name[] = {
    // Name
    BYTE_LEN(BLE_DEV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, BLE_DEV_NAME,
};

void get_mesh_adv_name(u8 *len, u8 **data)
{
    *len = sizeof(mesh_name);

    *data = mesh_name;
}

#define TRANSMIC_SIZE                           4
#define MAX_USEFUL_ACCESS_PAYLOAD_SIZE          11 // 32 bit TransMIC (unsegmented)
#define ACCESS_OP_SIZE                          3
#define ACCESS_PARAM_SIZE                       (MAX_USEFUL_ACCESS_PAYLOAD_SIZE - ACCESS_OP_SIZE)

/**
 * @brief Conifg MAC of current demo
 */
/*-----------------------------------------------------------*/
#define CUR_DEVICE_MAC_ADDR         		0x112233440000
#define BT_MESH_VENDOR_MODEL_ID_CLI         0x01A80001      //0x0001
#define BT_MESH_VENDOR_MODEL_ID_SRV         0x01A80000      //0x0000

/* provisioner configuraion */
#define NODE_ADDR 0x7fff
#define GROUP_ADDR 0xf000

static const u8_t net_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static const u8_t dev_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static const u8_t app_key[16] = {
    0x06, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x06, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static const u16_t net_idx = 0;
static const u16_t app_idx = 0;
static const u32_t iv_index = 0;
static u8_t flags;
static u16_t node_addr = NODE_ADDR;
/*
 * @brief AliGenie Vendor Model Operation Codes
 *
 * detail on Mesh_v1.0 <3.7.3.1 Operation codes>
 * 扩展消息 detail on https://www.aligenie.com/doc/357554/iv5it7
 */
/*-----------------------------------------------------------*/
#define VENDOR_MSG_ATTR_GET			            BT_MESH_MODEL_OP_3(0xD0, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_SET			            BT_MESH_MODEL_OP_3(0xD1, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_SET_UNACK			    BT_MESH_MODEL_OP_3(0xD2, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_STATUS			        BT_MESH_MODEL_OP_3(0xD3, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_INDICAT			        BT_MESH_MODEL_OP_3(0xD4, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_CONFIRM			        BT_MESH_MODEL_OP_3(0xD5, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_TRANSPARENT			    BT_MESH_MODEL_OP_3(0xCF, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_DEL						BT_MESH_MODEL_OP_3(0xD9, BT_COMP_ID_LF)

#define ATTR_TYPE_UNIX_TIME                     0xF01F
#define ATTR_TYPE_SET_TIMEOUT                   0xF010
#define ATTR_TYPE_SET_PERIOD_TIMEOUT            0xF011
#define ATTR_TYPE_DELETE_TIMEOUT                0xF012
#define ATTR_TYPE_WIND_SPEED					0x010A
#define ATTR_TYPE_EVENT_TRIGGER					0xf009

#define EVENT_TYPE_HARD_RESET					0x23

#define STATUS_APP_ADD_KEY						0x01
#define STATUS_APP_BIND_KEY						0x02
#define STATUS_ADD_SUB_ADDR						0x03
#define STATUS_FINISH							0xff
/*
 * Publication Declarations
 *
 * The publication messages are initialized to the
 * the size of the opcode + content
 *
 * For publication, the message must be in static or global as
 * it is re-transmitted several times. This occurs
 * after the function that called bt_mesh_model_publish() has
 * exited and the stack is no longer valid.
 *
 * Note that the additional 4 bytes for the AppMIC is not needed
 * because it is added to a stack variable at the time a
 * transmission occurs.
 *
 */
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_cli, NULL, 2 + 2);

/* Company Identifiers (see Bluetooth Assigned Numbers) */
#define BT_COMP_ID_LF           0x01A8// Zhuhai Jieli technology Co.,Ltd

struct prov_list {
    struct list_head node_list;
    struct bt_mesh_cdb_node node;
};

static struct prov_list prov_head;

/*
 * @brief AliGenie Vendor Model Message Handlers
 *
 * 定时功能 detail on https://www.aligenie.com/doc/357554/ovzn6v
 */
/*-----------------------------------------------------------*/
static void vendor_attr_status_send(struct bt_mesh_model *model,
                                    struct bt_mesh_msg_ctx *ctx,
                                    void *buf, u16 len)
{
    log_info("ready to send ATTR_TYPE_SET_TIMEOUT status");

    NET_BUF_SIMPLE_DEFINE(msg, len + TRANSMIC_SIZE);

    buffer_memcpy(&msg, buf, len);

    log_info_hexdump(msg.data, msg.len);

    if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
        log_error("Unable to send Status response\n");
    }
}

static void vendor_msg_status(struct bt_mesh_model *model,
                              struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf)
{
    log_info("receive vendor_attr_status, len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);

    u8 tid = buffer_pull_u8_from_head(buf);
    u16 Attr_Type = buffer_pull_le16_from_head(buf);
    log_info("ATTR_TYPE = 0x%x", Attr_Type);

    switch (Attr_Type) {
    case ATTR_TYPE_WIND_SPEED:
        u8 level = buffer_pull_u8_from_head(buf);
        printf("get fan speed level: %d\n", level);
        break;
    }
}

static void vendor_indicat_handler(struct bt_mesh_model *model,
                                   struct bt_mesh_msg_ctx *ctx,
                                   struct net_buf_simple *buf)
{
    log_info("receive vendor_attr_indicat, len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);

    u8 tid = buffer_pull_u8_from_head(buf);
    u16 Attr_Type = buffer_pull_le16_from_head(buf);
    log_info("INDICATE ATTR_TYPE = 0x%x", Attr_Type);

    switch (Attr_Type) {
    case ATTR_TYPE_EVENT_TRIGGER:
        u8 event = buffer_pull_u8_from_head(buf);
        printf("event = 0x%x\n", event);
        if (event == EVENT_TYPE_HARD_RESET) {
            log_info("node addr 0x%x hardreset\n");
            prov_node_del(ctx->addr);
        }
        break;
    }
}

/*
 * Server Configuration Declaration
 */
static struct bt_mesh_cfg_srv cfg_srv = {
    .relay          = BT_MESH_FEATURES_GET(BT_MESH_FEAT_RELAY),
    .frnd           = BT_MESH_FEATURES_GET(BT_MESH_FEAT_FRIEND),
    .gatt_proxy     = BT_MESH_FEATURES_GET(BT_MESH_FEAT_PROXY),
    .beacon         = BT_MESH_BEACON_DISABLED,
    .default_ttl    = 7,
};

static const struct bt_mesh_model_op vendor_cli_op[] = {
    { VENDOR_MSG_ATTR_STATUS, 1, vendor_msg_status},
    { VENDOR_MSG_ATTR_INDICAT, 1, vendor_indicat_handler},
    BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_cfg_cli cfg_cli;

/*
 *
 * Element Model Declarations
 *
 * Element 0 Root Models
 */
static struct bt_mesh_model root_models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
    BT_MESH_MODEL_CFG_CLI(&cfg_cli), // default for self-configuration network
};

static struct bt_mesh_model vendor_server_models[] = {
    BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_CLI, vendor_cli_op, NULL, NULL),
};
/*
 * Root and Secondary Element Declarations
 */
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, root_models, vendor_server_models),
};

static const struct bt_mesh_comp composition = {
    .cid = BT_COMP_ID_LF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static void prov_list_init(void)
{
    prov_head.node.net_idx = 0;
    prov_head.node.addr = 0x0;
    prov_head.node.num_elem = 0;


    INIT_LIST_HEAD(&prov_head.node_list);
}

static struct prov_list *new_node(u16_t net_idx, u8_t uuid[16], u16_t addr, u8_t num_elem, u8_t dev_key[16])
{
    struct prov_list *new = malloc(sizeof(struct prov_list));

    new->node.net_idx = net_idx;
    //new->node.uuid = uuid;
    strcpy(new->node.uuid, uuid);
    new->node.addr = addr;
    new->node.num_elem = num_elem;
    strcpy(new->node.dev_key, dev_key);

    list_add(&new->node_list, &prov_head.node_list);
    return new;
}

extern struct bt_mesh_net bt_mesh;
static void prov_node_del(u16_t addr)
{
    struct prov_list *node_del, *n, *pos;

    list_for_each_entry_safe(pos, n, &prov_head.node_list, node_list) {
        if (pos->node.addr == addr) {
            node_del = pos;
            list_del(&pos->node_list);
            printf("del node addr 0x%x\n", node_del->node.addr);
            free(node_del);
        }
    }

    struct bt_mesh_cdb_node *mesh_node = bt_mesh_cdb_node_get(addr);
    if (!mesh_node) {
        log_info("No find node for addr 0x%x\n", addr);
        return;
    }

    int i;
    for (i = 0; i < ARRAY_SIZE(bt_mesh.rpl); i++) {
        struct bt_mesh_rpl *rpl = &bt_mesh.rpl[i];
        if (rpl->src == addr) {
            rpl->seq = 0;
        }
    }
    bt_mesh_cdb_node_del(mesh_node, true);
}

struct __init_node {
    u16 net_idx;
    u16_t addr;
    u16_t mod_id;
    u8_t dev_key[16];
} machine_param;

static u8 g_state;

static void node_bind_machine(void)
{
    log_info("node_bind_machine, g_state = 0x%02x", g_state);
    u8_t status;
    int err;

    u16 m_net_idx = machine_param.net_idx;
    u16_t m_addr = machine_param.addr;
    u16_t m_mod_id = machine_param.mod_id;
    u8_t m_dev_key[16];

    switch (g_state) {
    case STATUS_APP_ADD_KEY:
        g_state = STATUS_APP_BIND_KEY;
        log_info("add node app key\n");
        err = bt_mesh_cfg_app_key_add(m_net_idx, m_addr, net_idx, app_idx, app_key, &status);
        if (err) {
            log_info("Failed to add app-key (err %d, status %d)\n", err, status);
            return;
        }
        break;

    case STATUS_APP_BIND_KEY:
        log_info("bind node app key\n");
        g_state = STATUS_ADD_SUB_ADDR;
        err = bt_mesh_cfg_mod_app_bind_vnd(m_net_idx, m_addr, m_addr, app_idx, m_mod_id, BT_COMP_ID_LF, &status);
        if (err) {
            log_info("Failed to bing app-key (err %d, status %d)\n", err, status);
            return;
        }
        break;

    case STATUS_ADD_SUB_ADDR:
        log_info("add node sub addr\n");
        g_state = STATUS_FINISH;
        err = bt_mesh_cfg_mod_sub_add_vnd(m_net_idx, m_addr, m_addr, GROUP_ADDR, m_mod_id, BT_COMP_ID_LF, &status);
        if (err) {
            log_info("Failed to add subcribe addr (err %d, status %d)\n", err, status);
            return;
        }
        break;

    case STATUS_FINISH:
        log_info("node init success for addr 0x%x\n", m_addr);
        m_net_idx = 0;
        m_addr = 0;
        memset(m_dev_key, 0, sizeof(m_dev_key));
        g_state = 0;
        break;

    default:
        log_info("unknown g_state: 0x%x\n", g_state);
    }
}

static void node_init(u16_t net_idx, u16_t addr, u8_t dev_key[16])
{
    log_info("<provisioned node_init begin>\n");
    g_state = STATUS_APP_ADD_KEY;
    machine_param.net_idx = net_idx;
    machine_param.addr = addr;
    machine_param.mod_id = BT_MESH_VENDOR_MODEL_ID_SRV;
    memcpy(machine_param.dev_key, dev_key, sizeof(dev_key[16]));
    node_bind_machine();
}

static void prov_node_added(u16_t net_idx, u8_t uuid[16], u16_t addr, u8_t num_elem, u8_t dev_key[16])
{
    new_node(net_idx, uuid, addr, num_elem, dev_key);
    node_init(net_idx, addr, dev_key);
}

static const u8_t dev_uuid[16] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

static const struct bt_mesh_prov prov = {
    .uuid = dev_uuid,
#if 0
    .output_size = 6,
    .output_actions = (BT_MESH_DISPLAY_NUMBER | BT_MESH_DISPLAY_STRING),
    .output_number = output_number,
    .output_string = output_string,
#else
    .output_size = 0,
    .output_actions = 0,
    .output_number = 0,
#endif
    .complete = prov_complete,
    .reset = prov_reset,
    .node_added = prov_node_added,
};

static void step_set_fan_speed_level()
{
    struct __fan_speed {
        u32 Opcode: 24,
            TID: 8;
        u16 Attr_Type;
        u8 level;
    } _GNU_PACKED_;

    static u8 level = 0;
    if (level <= 4 && level >= 1) {
        level += 1;
    } else {
        level = 1;
    }

    struct __fan_speed fan_speed = {
        .Opcode = buffer_head_init(VENDOR_MSG_ATTR_SET),
        .TID = 1,
        .Attr_Type = ATTR_TYPE_WIND_SPEED,
        .level = level,
    };
    log_info("set fan speed level = %d\n", level);

    struct bt_mesh_msg_ctx ctx = {
        .addr = 0xf000,
    };

    vendor_attr_status_send(&vendor_server_models[0], &ctx, &fan_speed, sizeof(fan_speed));
}

static void provisioner_reset(void *p)
{
    bt_mesh_reset();
    cpu_reset();
}

static void del_msg_send(u16_t addr)
{
    struct __del_msg {
        u32 Opcode: 24,
            TID: 8;
    } _GNU_PACKED_;

    struct __del_msg del_msg = {
        .Opcode = buffer_head_init(VENDOR_MSG_ATTR_DEL),
        .TID = 1,
    };

    struct bt_mesh_msg_ctx ctx = {
        .addr = addr,
    };

    vendor_attr_status_send(&vendor_server_models[0], &ctx, &del_msg, sizeof(del_msg));
}

void input_key_handler(u8 key_status, u8 key_number)
{
    log_info("key_number=0x%x", key_number);

    switch (key_status) {
    case KEY_EVENT_CLICK:
        log_info("  [KEY_EVENT_CLICK]  ");
        break;
    case KEY_EVENT_LONG:
        log_info("  [KEY_EVENT_LONG]  ");
        break;
    case KEY_EVENT_HOLD:
        log_info("  [KEY_EVENT_HOLD]  ");
        break;
    default :
        return;
    }

    if ((key_number == 2) && (key_status == KEY_EVENT_CLICK)) {
        log_info("\n  <bt_mesh_reset> \n");
        sys_timeout_add(NULL, provisioner_reset, 1000 * 3);
        return;
    }

    if (key_status == KEY_EVENT_CLICK) {
        switch (key_number) {
        case 0:
            /*struct prov_list *pos;
            list_for_each_entry(pos, &prov_head.node_list, node_list) {
            	printf("addr = 0x%x", pos->node.addr);
            }*/
            u16_t node = 0x1;
            while (1) {
                struct bt_mesh_cdb_node *mesh_node = bt_mesh_cdb_node_get(node);
                if (!mesh_node) {
                    break;
                }
                printf("node addr = 0x%x\n", mesh_node->addr);
                node++;
            }
            break;
        case 1:
            u16_t node_del_cycle = 0x1;
            while (bt_mesh_cdb_node_get(node_del_cycle)) {
                node_del_cycle += 1;
            }
            node_del_cycle -= 1;
            if (node_del_cycle == 0) {
                log_info("node_list is empty!\n");
                break;
            }
            prov_node_del(node_del_cycle);

            del_msg_send(node_del_cycle);

            break;
        case 3:
            step_set_fan_speed_level();
            break;
        default:
            log_info("Undefinded key: %d\n", key_number);
            return;
        }
    }
}

static void configure(void)
{
    u16_t elem_addr = node_addr;
    u16 dst_addr = GROUP_ADDR;

    u8 *status = NULL;
    g_state = 0;
    bt_mesh_cfg_app_key_add(net_idx, node_addr, net_idx, app_idx, app_key, status);
    bt_mesh_cfg_mod_app_bind_vnd(net_idx, node_addr, elem_addr, app_idx, BT_MESH_VENDOR_MODEL_ID_CLI, BT_COMP_ID_LF, status);
    //bt_mesh_cfg_mod_sub_add_vnd(net_idx, node_addr, elem_addr, dst_addr, BT_MESH_VENDOR_MODEL_ID_CLI, BT_COMP_ID_LF, NULL);
}

static void mesh_init(void)
{
    log_info("--func=%s", __FUNCTION__);

    int err = bt_mesh_init(&prov, &composition);
    if (err) {
        log_error("Initializing mesh failed (err %d)\n", err);
        return;
    }

    settings_load();

    err = bt_mesh_provision(net_key, net_idx, flags, iv_index, node_addr, dev_key);
    if (err) {
        log_error("Using stored settings\n");
    } else {
        log_info("Provisioning completed\n");

        cfg_cli_machine(node_bind_machine);
        configure();
    }

    bt_mesh_cdb_create(net_key);

    bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);

    prov_list_init();
}

void bt_ble_mesh_init(void)
{
    u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

    bt_mac_addr_set(bt_addr);

    mesh_setup(mesh_init);
}

#endif /* (CONFIG_MESH_MODEL == SIG_MESH_PROVISIONER) */
