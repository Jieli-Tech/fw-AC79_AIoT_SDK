#include "btstack/bluetooth.h"
#include "bt_common.h"
#include "crypto_toolbox/includes.h"
#include "api/sig_mesh_api.h"
#include "../model_api.h"
#include "../unix_timestamp.h"
#include "device/gpio.h"
#include "system/timer.h"
#include "event/key_event.h"

#define LOG_TAG             "[Mesh-TUYA]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if CONFIG_BLE_MESH_ENABLE && (CONFIG_MESH_MODEL == SIG_MESH_TUYA_LIGHT)

static int tuya_update(struct bt_mesh_model *mod);
extern u16_t primary_addr;
extern void mesh_setup(void (*init_cb)(void));
extern void gpio_pin_write(u8_t led_index, u8_t onoff);
extern void bt_mac_addr_set(u8 *bt_addr);
extern void prov_complete(u16_t net_idx, u16_t addr);
extern void prov_reset(void);
extern uint32_t btctler_get_rand_from_assign_range(uint32_t rand, uint32_t min, uint32_t max);
extern void pseudo_random_genrate(uint8_t *dest, unsigned size);

/*
 * @brief Config current node features(Relay/Proxy/Friend/Low Power)
 */
/*-----------------------------------------------------------*/
#define BT_MESH_FEAT_SUPPORTED_TEMP         ( \
                                                BT_MESH_FEAT_PROXY | \
                                                0 \
                                            )
#include "../feature_correct.h"
const int config_bt_mesh_features = BT_MESH_FEAT_SUPPORTED;

/*
 * @brief Config proxy connectable adv hardware param
 */
/*-----------------------------------------------------------*/
#if BT_MESH_FEATURES_GET(BT_MESH_FEAT_LOW_POWER)
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(3000); // unit: ms
#else
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(300); // unit: ms
#endif /* BT_MESH_FEATURES_GET(BT_MESH_FEAT_LOW_POWER) */

/*
 * @brief Conifg complete local name
 */
/*-----------------------------------------------------------*/
#define BLE_DEV_NAME        'T', 'Y', '-', 'L', 'i', 'g', 'h', 't'

const uint8_t mesh_name[] = {
    // Name
    BYTE_LEN(BLE_DEV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, BLE_DEV_NAME,
};

void get_mesh_adv_name(u8 *len, u8 **data)
{
    *len = sizeof(mesh_name);

    *data = mesh_name;
}

/*
 * @brief Conifg AliGenie 三元组
 *
 * detail on https://www.aligenie.com/doc/357554/gtgprq
 */
/*-----------------------------------------------------------*/
#define PID_TO_LITTLE_ENDIAN(x) \
    (x & 0xff), \
    ((x >> 8) & 0xff), \
    ((x >> 16) & 0xff), \
    ((x >> 24) & 0xff)

#define PID_TO_BIG_ENDIAN(x) \
    ((x >> 24) & 0xff), \
    ((x >> 16) & 0xff), \
    ((x >> 8) & 0xff), \
    (x & 0xff)

#define CATEGORY_TO_LITTLE_ENDIAN(x) \
    ((x >> 8) & 0xff), \
    (x & 0xff)

#define CUR_DEVICE_MAC_ADDR         0x112233445566
#define TUYA_CATEGORY               0x1011
#define PRODUCT_ID                  'y', 'j', 'f', 's', '5', '0', '6', 'f'

/*
 * @brief Publication Declarations
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
/*-----------------------------------------------------------*/
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv, tuya_update, 2 + 2);

/*
 * @brief Generic OnOff Model Operation Codes
 */
/*-----------------------------------------------------------*/
#define BT_MESH_MODEL_OP_GEN_ONOFF_GET			BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET			BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x04)

/*
 * @brief Light Lightness server Model Operation Codes
 */
/*-----------------------------------------------------------*/
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_GET				BT_MESH_MODEL_OP_2(0x82, 0x4B)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET				BT_MESH_MODEL_OP_2(0x82, 0x4C)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET_UNACK			BT_MESH_MODEL_OP_2(0x82, 0x4D)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS				BT_MESH_MODEL_OP_2(0x82, 0x4E)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_GET			BT_MESH_MODEL_OP_2(0x82, 0x4F)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_SET			BT_MESH_MODEL_OP_2(0x82, 0x50)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x51)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x52)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LAST_GET			BT_MESH_MODEL_OP_2(0x82, 0x53)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LAST_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x54)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_GET		BT_MESH_MODEL_OP_2(0x82, 0x55)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x56)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_GET			BT_MESH_MODEL_OP_2(0x82, 0x56)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x58)

/*
 * @brief Vendor Model ID
 *
 * Company Identifiers (see Bluetooth Assigned Numbers)
 * detail on Mesh_v1.0 <3.7.2 Model identifier>
 * detail on https://www.aligenie.com/doc/357554/iv5it7
 */
/*-----------------------------------------------------------*/
#define BT_COMP_ID_LF                           0xD007      // Alibaba
#define BT_PRODUCT_ID_LF                        0x0000      // PID
#define BT_VERSION_ID_LF                        0x3533      // VERSION
#define BT_MESH_VENDOR_MODEL_ID_SRV             0x07D00004  // Tuya vendor model

/*
 * @brief AliGenie Vendor Model Operation Codes
 *
 * detail on Mesh_v1.0 <3.7.3.1 Operation codes>
 * 扩展消息 detail on https://www.aligenie.com/doc/357554/iv5it7
 */
/*-----------------------------------------------------------*/
#define VENDOR_CMD_ATTR_WRITE			        BT_MESH_MODEL_OP_3(0xC9, BT_COMP_ID_LF)
#define VENDOR_CMD_ATTR_WRITE_UNACK			    BT_MESH_MODEL_OP_3(0xCA, BT_COMP_ID_LF)
#define VENDOR_CMD_ATTR_SET_READ			    BT_MESH_MODEL_OP_3(0xCC, BT_COMP_ID_LF)
#define VENDOR_CMD_ATTR_STATUS			        BT_MESH_MODEL_OP_3(0xCB, BT_COMP_ID_LF)
#define VENDOR_CMD_ATTR_DATA			        BT_MESH_MODEL_OP_3(0xCD, BT_COMP_ID_LF)

struct light_state {
    u16 lightness_actual;
    u16 lightness_linear;
    u16 lightness_default;
    u16 lightness_last;
    u16 lightness_range_min;
    u16 lightness_range_max;
};

static struct light_state light = {
    .lightness_actual = 0,
    .lightness_default = 10000,
    .lightness_range_min = 0,
    .lightness_range_max = 65535,
    .lightness_last = 0,
};

static bool led_switch = 0;
struct bt_mesh_model vendor_server_models[];
static bool indicate_flag[256];

/*
 * @brief Access Payload Fields
 *
 * detail on Mesh_v1.0 <3.7.3 Access payload>
 */
/*-----------------------------------------------------------*/
#define TRANSMIC_SIZE                           4
#define MAX_USEFUL_ACCESS_PAYLOAD_SIZE          11 // 32 bit TransMIC (unsegmented)
#define ACCESS_OP_SIZE                          3
#define ACCESS_PARAM_SIZE                       (MAX_USEFUL_ACCESS_PAYLOAD_SIZE - ACCESS_OP_SIZE)

/*
 * @brief Server Configuration Declaration
 */
/*-----------------------------------------------------------*/
static struct bt_mesh_cfg_srv cfg_srv = {
    .relay          = BT_MESH_FEATURES_GET(BT_MESH_FEAT_RELAY),
    .frnd           = BT_MESH_FEATURES_GET(BT_MESH_FEAT_FRIEND),
    .gatt_proxy     = BT_MESH_FEATURES_GET(BT_MESH_FEAT_PROXY),
    .beacon         = BT_MESH_BEACON_ENABLED,
    .default_ttl    = 7,
};

/*
 * @brief Generic OnOff State Set
 */
/*-----------------------------------------------------------*/
#define LED0_GPIO_PIN           0
#define LED1_GPIO_PIN           1

struct onoff_state {
    u8_t current;
    u8_t previous;
    u8_t led_gpio_pin;
};

struct _switch {
    u8_t sw_num;
    u8_t onoff_state;
};

static struct onoff_state onoff_state[] = {
    { .led_gpio_pin = LED0_GPIO_PIN },
};

const u8 led_use_port[] = {
    IO_PORTA_03,
};

static int tuya_update(struct bt_mesh_model *mod)
{
    printf("tuya_update, mod = 0x%x", (u32)mod);
    return 0;
}

/*
 * @brief Generic OnOff Model Server Message Handlers
 *
 * Mesh Model Specification 3.1.1
 */
/*-----------------------------------------------------------*/
static void respond_messsage_schedule(u16 *delay, u16 *duration, void *cb_data)
{
    /*  Mesh_v1.0 <3.7.4.1 Transmitting an access message>
     *
     *	  If the message is sent in response to a received message
     *  that was sent to a unicast address, the node should transmit
     *  the response message with a random delay between 20 and 50 milliseconds.
     *
     *    If the message is sent in response to a received message
     *  that was sent to a group address or a virtual address,
     *  the node should transmit the response message with
     *  a random delay between 20 and 500 milliseconds.
     */
    u16 delay_ms;
    //struct bt_mesh_msg_ctx *ctx = cb_data;
    u16 dst_addr = (u16)cb_data;

    pseudo_random_genrate((u8 *)&delay_ms, 2);
    if (BT_MESH_ADDR_IS_UNICAST(dst_addr)) {
        delay_ms = btctler_get_rand_from_assign_range(delay_ms, 20, 50);
    } else {
        delay_ms = btctler_get_rand_from_assign_range(delay_ms, 20, 200);
    }

    *delay = delay_ms;
    log_info("respond_messsage delay =%u ms", delay_ms);
}

static const struct bt_mesh_send_cb rsp_msg_cb = {
    .user_intercept = respond_messsage_schedule,
    /* .user_intercept = NULL, */
};

static void gen_onoff_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + 4);
    struct onoff_state *onoff_state = model->user_data;

    log_info("gen_onoff_get addr 0x%04x onoff 0x%02x\n",
             bt_mesh_model_elem(model)->addr, onoff_state->current);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
    buffer_add_u8_at_tail(&msg, onoff_state->current);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send On Off Status response\n");
    }
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
    struct net_buf_simple *msg = model->pub->msg;
    struct onoff_state *onoff_state = model->user_data;
    int err;

    onoff_state->current = buffer_pull_u8_from_head(buf);
    log_info("gen_onoff_set_unack addr 0x%02x state 0x%02x\n", bt_mesh_model_elem(model)->addr, onoff_state->current);
    /* log_info_hexdump((u8 *)onoff_state, sizeof(*onoff_state)); */

    led_switch = onoff_state->current;
}

static void gen_onoff_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    log_info("gen_onoff_set\n");

    gen_onoff_set_unack(model, ctx, buf);
    gen_onoff_get(model, ctx, buf);
}

static void lightness_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    printf("lightness_get\n");
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
    struct light_state *light_state = model->user_data;

    log_info("addr 0x%04x actual lightness 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_actual);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS);
    buffer_add_le16_at_tail(&msg, light_state->lightness_actual);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send lightness Status response\n");
    }
}

static void lightness_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
    struct light_state *light_state = model->user_data;

    light_state->lightness_actual = buffer_pull_le16_from_head(buf);
    printf("light set to %d", light_state->lightness_actual);

    if (light_state->lightness_actual > 0) {
        onoff_state[0].current = 1;
    } else {
        onoff_state[0].current = 0;
    }

    light_state->lightness_last = light_state->lightness_actual;
    u32 bright = (light_state->lightness_actual) * 100 / 65535;

    //need to make a func to set bright here

    log_info("actual lightness set to %d%%\n", bright);
}

static void lightness_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    printf("lightness_set\n");
    lightness_set_unack(model, ctx, buf);
    lightness_get(model, ctx, buf);
}

static void vendor_cmd_test(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    printf("receive tuya_vendor_msg, len except opcode =0x%x", buf->len);
    printf_buf(buf->data, buf->len);
}

/*
 * @brief OnOff Model Server Op Dispatch Table
 */
/*-----------------------------------------------------------*/
static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
    { BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, gen_onoff_get },
    { BT_MESH_MODEL_OP_GEN_ONOFF_SET, 2, gen_onoff_set },
    { BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, gen_onoff_set_unack },
    BT_MESH_MODEL_OP_END,
};

/*
 * @brief Vendor Model Server Op Dispatch Table
 */
/*-----------------------------------------------------------*/
static const struct bt_mesh_model_op vendor_srv_op[] = {
    { VENDOR_CMD_ATTR_WRITE,        ACCESS_OP_SIZE, vendor_cmd_test},
    { VENDOR_CMD_ATTR_WRITE_UNACK,  ACCESS_OP_SIZE, vendor_cmd_test},
    { VENDOR_CMD_ATTR_SET_READ,     ACCESS_OP_SIZE, vendor_cmd_test},
    { VENDOR_CMD_ATTR_STATUS,       ACCESS_OP_SIZE, vendor_cmd_test},
    { VENDOR_CMD_ATTR_DATA,         ACCESS_OP_SIZE, vendor_cmd_test},
    BT_MESH_MODEL_OP_END,
};

const struct bt_mesh_model_op light_lightness_srv_op[] = {
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_GET, 				0, lightness_get},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET, 				0, lightness_set},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET_UNACK, 			0, lightness_set_unack},
    BT_MESH_MODEL_OP_END,
};
/*
 * @brief Element Model Declarations
 *
 * Element 0 Root Models
 */
/*-----------------------------------------------------------*/
static struct bt_mesh_model root_models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_pub_srv, &onoff_state[0]),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV, light_lightness_srv_op, &gen_onoff_pub_srv, &light),
};

static struct bt_mesh_model vendor_server_models[] = {
    BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_SRV, vendor_srv_op, NULL, &onoff_state[0]),
};


/*
 * @brief LED to Server Model Assigmnents
 */
/*-----------------------------------------------------------*/
static struct bt_mesh_model *mod_srv_sw[] = {
    &root_models[1],
};

/*
 * @brief Root and Secondary Element Declarations
 */
/*-----------------------------------------------------------*/
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, root_models, vendor_server_models),
};

static const struct bt_mesh_comp composition = {
    .cid = BT_COMP_ID_LF,
    .pid = BT_PRODUCT_ID_LF,
    .vid = BT_VERSION_ID_LF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static const u8 dev_uuid[16] = {
    MAC_TO_BIG_ENDIAN(CUR_DEVICE_MAC_ADDR), // MAC
    CATEGORY_TO_LITTLE_ENDIAN(TUYA_CATEGORY), // mesh category
    PRODUCT_ID, // ProductID
};

static const struct bt_mesh_prov prov = {
    .uuid = dev_uuid,
    .output_size = 0,
    .output_actions = 0,
    .output_number = 0,
    .complete = prov_complete,
    .reset = prov_reset,
};

/*
 * @brief Button Pressed Worker Task
 */
/*-----------------------------------------------------------*/
static bool server_publish(struct _switch *sw)
{
    struct bt_mesh_model *mod_srv;
    struct bt_mesh_model_pub *pub_srv;

    mod_srv = mod_srv_sw[sw->sw_num];
    pub_srv = mod_srv->pub;

    /* If unprovisioned, just call the set function.
     * The intent is to have switch-like behavior
     * prior to provisioning. Once provisioned,
     * the button and its corresponding led are no longer
     * associated and act independently. So, if a button is to
     * control its associated led after provisioning, the button
     * must be configured to either publish to the led's unicast
     * address or a group to which the led is subscribed.
     */
    if (primary_addr == BT_MESH_ADDR_UNASSIGNED) {
        NET_BUF_SIMPLE_DEFINE(msg, 1);
        struct bt_mesh_msg_ctx ctx = {
            .addr = sw->sw_num + primary_addr,
        };

        /* This is a dummy message sufficient
         * for the led server
         */
        buffer_add_u8_at_tail(&msg, sw->onoff_state);
        gen_onoff_set_unack(mod_srv, &ctx, &msg);
        return TRUE;
    }
    return FALSE;
}

static void button_pressed_worker(struct _switch *sw)
{
    if (sw->sw_num >= composition.elem_count) {
        log_info("sw_num over elem_count");
        return;
    }

    if (server_publish(sw)) {
        return;
    }
}

static void iot_reset(void *p)
{
    bt_mesh_reset();
    extern void p33_soft_reset(void);
    p33_soft_reset();
}

void input_key_handler(u8 key_status, u8 key_number)
{
    struct _switch press_switch;
    log_info("key_number=0x%x", key_number);

    if ((key_number == 2) && (key_status == KEY_EVENT_CLICK)) {
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

/*
 * @brief Mesh Profile Setup
 */
/*-----------------------------------------------------------*/
static void mesh_init(void)
{
    int err = bt_mesh_init(&prov, &composition);
    if (err) {
        log_error("Initializing mesh failed (err %d)\n", err);
        return;
    }

    settings_load();

    bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
}

void bt_ble_mesh_init(void)
{
    u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

    bt_mac_addr_set(bt_addr);

    mesh_setup(mesh_init);
}

#endif /* (CONFIG_MESH_MODEL == SIG_MESH_ALIGENIE_SOCKET) */

