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
#include "device/gpio.h"
#include "event/key_event.h"

#define LOG_TAG         "[Mesh-vendor_cli]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ERROR_ENABLE
#include "debug.h"

#if CONFIG_BLE_MESH_ENABLE && (CONFIG_MESH_MODEL == SIG_MESH_VENDOR_CLIENT)

extern u16_t primary_addr;
extern void mesh_setup(void (*init_cb)(void));
extern void gpio_pin_write(u8_t led_index, u8_t onoff);
extern void bt_mac_addr_set(u8 *bt_addr);
extern void prov_complete(u16_t net_idx, u16_t addr);
extern void prov_reset(void);
static void vendor_set(struct bt_mesh_model *model,
                       struct bt_mesh_msg_ctx *ctx,
                       struct net_buf_simple *buf);
static void vendor_status(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf);

#define CLIENT_SUBSCRIBE_EN     0

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
 * @brief Config adv cache buffer
 */
/*-----------------------------------------------------------*/
#define MESH_ADV_BUFFER_COUNT           6
const u8 config_bt_mesh_adv_buf_count = MESH_ADV_BUFFER_COUNT;
#if (MESH_ADV_BUFFER_COUNT < 6) // base on "config_bt_mesh_node_msg_adv_duration = 100"
#error " current MESH_ADV_BUFFER_COUNT must >= 6 "
#endif

/**
 * @brief Conifg complete local name
 */
/*-----------------------------------------------------------*/
#define BLE_DEV_NAME        'V', 'd', '_','c', 'l', 'i'

const uint8_t mesh_name[] = {
    // Name
    BYTE_LEN(BLE_DEV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, BLE_DEV_NAME,
};

void get_mesh_adv_name(u8 *len, u8 **data)
{
    *len = sizeof(mesh_name);

    *data = (u8 *)mesh_name;
}

/**
 * @brief Conifg MAC of current demo
 */
/*-----------------------------------------------------------*/
#define CUR_DEVICE_MAC_ADDR         0x332233445566

struct _switch {
    u8_t sw_num;
    u8_t onoff_state;
};

/* Company Identifiers (see Bluetooth Assigned Numbers) */
#define BT_COMP_ID_LF           0x05D6 // Zhuhai Jieli technology Co.,Ltd

/*
 * Vendor Model ID
 * detail on Mesh_v1.0 <3.7.2 Model identifier>
 */
#define BT_MESH_VENDOR_MODEL_ID_CLI             0x0002

/*
 * Vendor Model Operation Codes
 * detail on Mesh_v1.0 <3.7.3.1 Operation codes>
 */
#define BT_MESH_VENDOR_MODEL_OP_SET			    BT_MESH_MODEL_OP_3(0x01, BT_COMP_ID_LF)
#define BT_MESH_VENDOR_MODEL_OP_STATUS			BT_MESH_MODEL_OP_3(0x02, BT_COMP_ID_LF)

/*
 * Access payload fields
 * detail on Mesh_v1.0 <3.7.3 Access payload>
 */
#define TRANSMIC_SIZE                           4
#define MAX_USEFUL_ACCESS_PAYLOAD_SIZE          11 // 32 bit TransMIC (unsegmented)
#define ACCESS_OP_SIZE      3
#define ACCESS_PARAM_SIZE   (MAX_USEFUL_ACCESS_PAYLOAD_SIZE - ACCESS_OP_SIZE)

/* test data */
#define LED_STATE_LEN       1
#define REMAIN_DATA_LEN     (ACCESS_PARAM_SIZE - LED_STATE_LEN)
#define REMAIN_DATA_VALUE   0x02

/* LED NUMBER */
#define LED0_GPIO_PIN       0

struct onoff_state {
    u8_t onoff;
    u8_t led_gpio_pin;
};

static struct onoff_state onoff_state[] = {
    { .led_gpio_pin = LED0_GPIO_PIN },
};

const u8 led_use_port[1] = {
    IO_PORTA_03,
};

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
BT_MESH_MODEL_PUB_DEFINE(vendor_pub_cli, NULL, MAX_USEFUL_ACCESS_PAYLOAD_SIZE);

/*
 * Models in an element must have unique op codes.
 *
 * The mesh stack dispatches a message to the first model in an element
 * that is also bound to an app key and supports the op code in the
 * received message.
 *
 */
/*
 * Vendor Model Server Op Dispatch Table
 *
 */
static const struct bt_mesh_model_op vendor_srv_op[] = {
    { BT_MESH_VENDOR_MODEL_OP_SET, ACCESS_OP_SIZE, vendor_set },
    BT_MESH_MODEL_OP_END,
};

/*
 * Vendor Model Client Op Dispatch Table
 */

static const struct bt_mesh_model_op vendor_cli_op[] = {
    { BT_MESH_VENDOR_MODEL_OP_STATUS, ACCESS_OP_SIZE, vendor_status },
#if CLIENT_SUBSCRIBE_EN
    { BT_MESH_VENDOR_MODEL_OP_SET, ACCESS_OP_SIZE, vendor_set },
#endif /* CLIENT_SUBSCRIBE_EN */
    BT_MESH_MODEL_OP_END,
};

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

/*
 * Client Configuration Declaration
 */
static struct bt_mesh_cfg_cli cfg_cli;

/*
 *
 * Element Model Declarations
 *
 * Element 0 Root Models
 */
static struct bt_mesh_model root_models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv), // default for root model
    BT_MESH_MODEL_CFG_CLI(&cfg_cli), // default for self-configuration network
};

static struct bt_mesh_model vendor_client_models[] = {
    BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_CLI,
    vendor_cli_op, &vendor_pub_cli, &onoff_state[0]),
};

/*
 * Button to Client Model Assignments
 */
static struct bt_mesh_model *mod_cli_sw[] = {
    &vendor_client_models[0],
};

/*
 * Root and Secondary Element Declarations
 */
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, root_models, vendor_client_models),
};

static const struct bt_mesh_comp composition = {
    .cid = BT_COMP_ID_LF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static u8_t dev_uuid[16] = { 0xdd, 0xdd };

static const struct bt_mesh_prov prov = {
    .uuid = dev_uuid,
    .output_size = 0,
    .output_actions = 0,
    .output_number = 0,
    .complete = prov_complete,
    .reset = prov_reset,
};

static void vendor_set(struct bt_mesh_model *model,
                       struct bt_mesh_msg_ctx *ctx,
                       struct net_buf_simple *buf)
{
    log_info("receive message len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);

    struct net_buf_simple *msg = model->pub->msg;
    struct onoff_state *onoff_state = model->user_data;

    //< set led onoff
    onoff_state->onoff = buffer_pull_u8_from_head(buf);
    log_info("Local Node 0x%02x shoult set led to 0x%02x\n",
             bt_mesh_model_elem(model)->addr, onoff_state->onoff);
    gpio_pin_write(onoff_state->led_gpio_pin, onoff_state->onoff);

    //< Ack to client with the same receive data
    NET_BUF_SIMPLE_DEFINE(ack_msg, MAX_USEFUL_ACCESS_PAYLOAD_SIZE + TRANSMIC_SIZE);
    bt_mesh_model_msg_init(&ack_msg, BT_MESH_VENDOR_MODEL_OP_STATUS); // Opcode: 3 octets
    buffer_add_u8_at_tail(&ack_msg, onoff_state->onoff); // onoff state: 1 octets
    buffer_memset(&ack_msg, buffer_pull_u8_from_head(buf) + 1, REMAIN_DATA_LEN);

    log_info_hexdump(ack_msg.data, MAX_USEFUL_ACCESS_PAYLOAD_SIZE);
    if (bt_mesh_model_send(model, ctx, &ack_msg, NULL, NULL)) {
        log_info("Unable to send Status response\n");
    }
}

static void vendor_status(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    log_info("receive vendor server message len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);

    u8_t state;

    state = buffer_pull_u8_from_head(buf);
    log_info("Local Node 0x%04x receive ACK from Remote Node 0x%04x with led 0x%02x\n",
             bt_mesh_model_elem(model)->addr, ctx->addr, state);
}

static void client_publish(struct _switch *sw)
{
    int err;
    struct bt_mesh_model *mod_cli;
    struct bt_mesh_model_pub *pub_cli;

    mod_cli = mod_cli_sw[sw->sw_num];
    pub_cli = mod_cli->pub;

    if (pub_cli->addr == BT_MESH_ADDR_UNASSIGNED) {
        log_info("pub_cli->addr == BT_MESH_ADDR_UNASSIGNED");
        return;
    }

    log_info("publish to Remote 0x%04x onoff 0x%04x sw_num 0x%04x\n",
             pub_cli->addr, sw->onoff_state, sw->sw_num);

    bt_mesh_model_msg_init(pub_cli->msg, BT_MESH_VENDOR_MODEL_OP_SET);
    buffer_add_u8_at_tail(pub_cli->msg, sw->onoff_state);
    buffer_memset(pub_cli->msg, REMAIN_DATA_VALUE, REMAIN_DATA_LEN);

    err = bt_mesh_model_publish(mod_cli);
    if (err) {
        log_info("bt_mesh_model_publish err %d\n", err);
    }
    log_info_hexdump(pub_cli->msg->data, MAX_USEFUL_ACCESS_PAYLOAD_SIZE);
}

/*
 * Button Pressed Worker Task
 */
static void button_pressed_worker(struct _switch *sw)
{
    if (sw->sw_num >= composition.elem_count) {
        log_info("sw_num over elem_count");
        return;
    }

    client_publish(sw);
}

#define NODE_ADDR 0x0001

#define GROUP_ADDR 0xc000

#define OP_VENDOR_BUTTON BT_MESH_MODEL_OP_3(0x00, BT_COMP_ID_LF)

static const u8_t net_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static const u8_t dev_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static const u8_t app_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static const u16_t net_idx;
static const u16_t app_idx;
static const u32_t iv_index;
static u8_t flags;
static u16_t node_addr = NODE_ADDR;

static void configure(void)
{
    log_info("Configuring...");

    u16_t elem_addr = node_addr;
    log_info("node_addr=0x%x, net_idx=0x%x, app_idx=0x%x", node_addr, net_idx, app_idx);

    /* Add Application Key */
    log_info("bt_mesh_cfg_app_key_add");
    bt_mesh_cfg_app_key_add(net_idx, node_addr, net_idx, app_idx, app_key, NULL);

    u16 dst_addr = GROUP_ADDR;

    /* Bind to vendor client model */
    log_info("bt_mesh_cfg_mod_app_bind_vnd client");
    bt_mesh_cfg_mod_app_bind_vnd(net_idx, node_addr, elem_addr, app_idx,
                                 BT_MESH_VENDOR_MODEL_ID_CLI,
                                 BT_COMP_ID_LF,
                                 NULL);
    /* Add model publish */
    struct bt_mesh_cfg_mod_pub pub;
    pub.addr = dst_addr;
    pub.app_idx = app_idx;
    pub.cred_flag = 0;
    pub.ttl = 7;
    pub.period = 0;
    /* pub.transmit = 0b00100001; */
    pub.transmit = 0;
    log_info("bt_mesh_cfg_mod_pub_set_vnd client");
    bt_mesh_cfg_mod_pub_set_vnd(net_idx, node_addr, elem_addr,
                                BT_MESH_VENDOR_MODEL_ID_CLI,
                                BT_COMP_ID_LF,
                                &pub, NULL);

#if CLIENT_SUBSCRIBE_EN
    /* Add model subscription */
    log_info("bt_mesh_cfg_mod_sub_add_vnd server");
    bt_mesh_cfg_mod_sub_add_vnd(net_idx, node_addr, elem_addr, dst_addr,
                                BT_MESH_VENDOR_MODEL_ID_CLI,
                                BT_COMP_ID_LF,
                                NULL);
#endif /* CLIENT_SUBSCRIBE_EN */

    log_info("Configuration complete");
}

void input_key_handler(u8 key_status, u8 key_number)
{
    struct _switch press_switch;

    log_info("key_number=0x%x", key_number);

    if ((key_number == 2) && (key_status == KEY_EVENT_LONG)) {
        log_info("\n  <bt_mesh_reset> \n");
        bt_mesh_reset();
        return;
    }

    switch (key_status) {
    case KEY_EVENT_CLICK:
        log_info("  [KEY_EVENT_CLICK]  ");
        press_switch.sw_num = key_number;
        press_switch.onoff_state = 1;
        button_pressed_worker(&press_switch);
        break;
    case KEY_EVENT_LONG:
        log_info("  [KEY_EVENT_LONG]  ");
        press_switch.sw_num = key_number;
        press_switch.onoff_state = 0;
        button_pressed_worker(&press_switch);
        break;
    case KEY_EVENT_HOLD:
        log_info("  [KEY_EVENT_HOLD]  ");
        break;
    default :
        return;
    }
}

static void mesh_init(void)
{
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

        configure();
    }
}

void bt_ble_mesh_init(void)
{
    u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

    bt_mac_addr_set(bt_addr);

    mesh_setup(mesh_init);
}

/********************************************************************************/
/*
 *                  Mesh API Example
 */

/*
 * note:    Each model only support one publish address.
 *          But support more subscribe address list, base on CONFIG_BT_MESH_MODEL_GROUP_COUNT.
 */
void example_node_publish_address_change(void)
{
    //< change root element pubilsh address to 0xc001 (group address)

    u16_t elem_addr = node_addr;
    struct bt_mesh_model_pub *pub_cli = mod_cli_sw[0]->pub;
    struct bt_mesh_cfg_mod_pub pub;

    pub.addr = 0xc001; // dst address
    pub.app_idx = app_idx;
    pub.cred_flag = pub_cli->cred;
    pub.ttl = pub_cli->cred;
    pub.period = pub_cli->period;
    pub.transmit = pub_cli->retransmit;

    bt_mesh_cfg_mod_pub_set_vnd(net_idx, node_addr, elem_addr,
                                BT_MESH_VENDOR_MODEL_ID_CLI,
                                BT_COMP_ID_LF,
                                &pub, NULL);
}

void example_node_relay_character_set(void)
{
    //< BT_MESH_RELAY_DISABLED / BT_MESH_RELAY_ENABLED

    u8 relay_attr = BT_MESH_RELAY_DISABLED;
    u8 relay_retransmit = BT_MESH_TRANSMIT(2, 20);

    bt_mesh_cfg_relay_set(net_idx, node_addr, relay_attr,
                          relay_retransmit, NULL, NULL);
}

void example_node_reset(void)
{
    //< reset the node to an unprovisioned device

    bt_mesh_reset();
}

#endif /* (CONFIG_MESH_MODEL == SIG_MESH_VENDOR_CLIENT) */
