#include "btstack/bluetooth.h"
#include "bt_common.h"
#include "crypto_toolbox/includes.h"
#include "api/sig_mesh_api.h"
#include "../model_api.h"
#include "../unix_timestamp.h"
#include "device/gpio.h"
#include "system/timer.h"
#include "event/key_event.h"

#define LOG_TAG             "[Mesh-AliFan]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if CONFIG_BLE_MESH_ENABLE && (CONFIG_MESH_MODEL == SIG_MESH_ALIGENIE_FAN)

extern u16_t primary_addr;
extern void mesh_setup(void (*init_cb)(void));
extern void gpio_pin_write(u8_t led_index, u8_t onoff);
extern void bt_mac_addr_set(u8 *bt_addr);
extern void prov_complete(u16_t net_idx, u16_t addr);
extern void prov_reset(void);
extern uint32_t btctler_get_rand_from_assign_range(uint32_t rand, uint32_t min, uint32_t max);
extern void pseudo_random_genrate(uint8_t *dest, unsigned size);

_WEAK_ int timer_pwm_init(JL_TIMER_TypeDef *JL_TIMERx, u32 fre, u32 duty, u32 port, int output_ch)
{
    return 0;
}

_WEAK_ void set_led_duty(u16 duty)
{

}

/*
 * @brief Config current node features(Relay/Proxy/Friend/Low Power)
 */
/*-----------------------------------------------------------*/
#define BT_MESH_FEAT_SUPPORTED_TEMP         ( \
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
#define BLE_DEV_NAME        'A', 'G', '-', 'S', 'o', 'c', 'k', 'e', 't'

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

#define PRODUCT_ID_STRING_SIZE      (sizeof(Product_ID) * 2)
#define MAC_ADDRESS_STRING_SIZE     (sizeof(Mac_Address) * 2)
#define SECRET_STRING_SIZE          (sizeof(Secret) - 1)

#define CUR_DEVICE_MAC_ADDR         0x27fa7af002a0
#define PRODUCT_ID                  7809508
#define DEVICE_SECRET               "d2729d5f3898079fa7b697c76a7bfe8e"

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
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv, NULL, 2 + 2);

/*
 * @brief Generic OnOff Model Operation Codes
 */
/*-----------------------------------------------------------*/
#define BT_MESH_MODEL_OP_GEN_ONOFF_GET			BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET			BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x04)

/*
 * @brief Vendor Model ID
 *
 * Company Identifiers (see Bluetooth Assigned Numbers)
 * detail on Mesh_v1.0 <3.7.2 Model identifier>
 * detail on https://www.aligenie.com/doc/357554/iv5it7
 */
/*-----------------------------------------------------------*/
#define BT_COMP_ID_LF                           0x01A8      // Alibaba
#define BT_MESH_VENDOR_MODEL_ID_SRV             0x01A80000      //0x0000
#define BT_MESH_VENDOR_MODEL_ID_CLI             0x01A80001      //0x0001

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

/*
 * @brief AliGenie Vendor Model Message Struct
 *
 * 定时功能 detail on https://www.aligenie.com/doc/357554/ovzn6v
 */
/*-----------------------------------------------------------*/
#define ATTR_TYPE_UNIX_TIME                     0xF01F
#define ATTR_TYPE_SET_TIMEOUT                   0xF010
#define ATTR_TYPE_SET_PERIOD_TIMEOUT            0xF011
#define ATTR_TYPE_DELETE_TIMEOUT                0xF012
#define ATTR_TYPE_WIND_SPEED					0x010A

struct __unix_time {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    u32 time;
} _GNU_PACKED_;

struct __set_timeout {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    struct __param {
        u8 index;
        u32 time;
        u16 attr_type;
        u8 attr_para;
    } _GNU_PACKED_ param[1] ;
} _GNU_PACKED_;

struct __set_period_timeout {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    struct __period_param {
        u8  index;
        u16 _24h_timer;
        u8  schedule;
        u16 attr_type;
        u8  attr_para;
    } _GNU_PACKED_ period_param[1] ;
} _GNU_PACKED_;

struct __delete_time {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    u8 index;
} _GNU_PACKED_;

struct __timer_success {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    u8  event;
    u8  index;
} _GNU_PACKED_;

struct __timer_param {
    u8 tid;
    u8 index;
    u8 timer_cnt;
    bool onoff;
};

struct __onoff_repo {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    bool OnOff;
} _GNU_PACKED_;

struct __fan_speed {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    u8 level;
} _GNU_PACKED_;

struct __indicate_msg {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    u8 Event;
    u8 EventParam;
} _GNU_PACKED_;

struct __comfirm_check_param {
    u8 resend_cnt;
    u8 timer_cnt;
    u8 indicate_tid;
    void *buf;
    u16 len;
};

static bool led_flag = 0;
static u8 indicate_tid = 0x80;
static u16 timer_index[20];
static struct __timer_param timer_param[20];
static struct __comfirm_check_param comfirm_check_param[20];
static u8 timer_cnt = -1;
static struct bt_mesh_model vendor_server_models[];
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
    log_info("ready to send ATTR_TYPE_SET status");

    NET_BUF_SIMPLE_DEFINE(msg, len + TRANSMIC_SIZE);

    buffer_memcpy(&msg, buf, len);

    log_info_hexdump(msg.data, msg.len);

    if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
        log_error("Unable to send Status response\n");
    }
}

static void indicate_tid_get(u8 *indicate_tid)
{
    if (*indicate_tid >= 0x80 && *indicate_tid <= 0xbf) {
        *indicate_tid += 1;
    } else {
        *indicate_tid = 0x80;
    }
}

static void timer_cnt_get(u8 *timer_cnt)
{
    if (*timer_cnt == (u8) - 1 && *timer_cnt <= 18) {
        *timer_cnt += 1;
    } else {
        *timer_cnt = 0;
    }
}

static void comfirm_check(void *p)
{
    struct __comfirm_check_param *param = (struct __comfirm_check_param *)p;
    struct bt_mesh_msg_ctx ctx = {
        .addr = 0xf000,
    };
    if (!indicate_flag[param->indicate_tid]) {
        param->resend_cnt += 1;
        if (param->resend_cnt >= 10) {  //最多重传75次，即30秒重传时间
            sys_timer_remove(timer_index[param->timer_cnt]);
            printf("resend msg more than 75 times\r\n");
        }
        printf("indicate_flag[ %d ] = %d\r\n", param->indicate_tid, indicate_flag[param->indicate_tid]);
        printf("\n  param->buf.tid = %d, timer_cnt = %d \n", ((struct __onoff_repo *)(param->buf))->TID, param->timer_cnt);
        vendor_attr_status_send(&vendor_server_models[1], &ctx, param->buf, param->len);
    } else {
        sys_timer_remove(timer_index[param->timer_cnt]);
    }
}

static void gen_onoff_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + 4);
    struct onoff_state *onoff_state = model->user_data;

    log_info("addr 0x%04x onoff 0x%02x\n",
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
    log_info("addr 0x%02x state 0x%02x\n",
             bt_mesh_model_elem(model)->addr, onoff_state->current);
    /* log_info_hexdump((u8 *)onoff_state, sizeof(*onoff_state)); */

    if (onoff_state->current) {
        set_led_duty(10000);
    } else {
        set_led_duty(0);
    }

    led_flag = onoff_state->current;
    printf("\n   tmall set led to %d    \n", led_flag);

    indicate_tid_get(&indicate_tid);
    timer_cnt_get(&timer_cnt);
    indicate_flag[indicate_tid] = 0;

    static struct __onoff_repo onoff_repo_set[10];
    static u8 onoff_repo_set_cnt = 0;

    if (onoff_repo_set_cnt < 9) {
        onoff_repo_set_cnt ++;
    } else {
        onoff_repo_set_cnt = 0;
    }

    onoff_repo_set[onoff_repo_set_cnt].Opcode = buffer_head_init(VENDOR_MSG_ATTR_INDICAT);
    onoff_repo_set[onoff_repo_set_cnt].TID = indicate_tid;
    onoff_repo_set[onoff_repo_set_cnt].Attr_Type = 0x0100;
    onoff_repo_set[onoff_repo_set_cnt].OnOff = led_flag;

    comfirm_check_param[timer_cnt].resend_cnt = 0;
    comfirm_check_param[timer_cnt].timer_cnt = timer_cnt;
    comfirm_check_param[timer_cnt].indicate_tid = onoff_repo_set[onoff_repo_set_cnt].TID;
    comfirm_check_param[timer_cnt].buf = &onoff_repo_set[onoff_repo_set_cnt];
    comfirm_check_param[timer_cnt].len = sizeof(onoff_repo_set[onoff_repo_set_cnt]);

    printf("\n  set unack tid = %d, timer_cnt = %d, param.tid = %d  \n", onoff_repo_set[onoff_repo_set_cnt].TID, timer_cnt, comfirm_check_param[timer_cnt].indicate_tid);

    vendor_attr_status_send(&vendor_server_models[1], ctx, &onoff_repo_set[onoff_repo_set_cnt], sizeof(onoff_repo_set[onoff_repo_set_cnt]));
    timer_index[timer_cnt] = sys_timer_add(&comfirm_check_param[timer_cnt], comfirm_check, 400);
}

static void gen_onoff_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    log_info("gen_onoff_set\n");

    gen_onoff_set_unack(model, ctx, buf);
    gen_onoff_get(model, ctx, buf);
}

static void timer_handler(void *p)
{
    struct __timer_param *param = (struct __timer_param *)p;
    indicate_tid_get(&indicate_tid);

    static struct __timer_success timer_success;
    timer_success.Opcode = buffer_head_init(VENDOR_MSG_ATTR_INDICAT);
    timer_success.TID = indicate_tid;
    timer_success.Attr_Type = 0xf009;
    timer_success.event = 0x11;
    timer_success.index = param->index;

    indicate_flag[timer_success.TID] = 0;

    indicate_tid_get(&indicate_tid);
    static struct __onoff_repo onoff_repo_handle;
    onoff_repo_handle.Opcode = buffer_head_init(VENDOR_MSG_ATTR_INDICAT);
    onoff_repo_handle.TID = indicate_tid;
    onoff_repo_handle.Attr_Type = 0x0100;
    onoff_repo_handle.OnOff = param->onoff;

    indicate_flag[onoff_repo_handle.TID] = 0;

    struct bt_mesh_msg_ctx ctx = {
        .addr = 0xf000,
    };

    gpio_pin_write(LED0_GPIO_PIN, param->onoff);
    led_flag = param->onoff;

    printf("timer_cnt = %d, timer_index = 0x%x, onoff = %d", param->timer_cnt, timer_index[param->timer_cnt], param->onoff);

    sys_timer_remove(timer_index[param->timer_cnt]);

    //timing success msg send and check comfirm
    timer_cnt_get(&timer_cnt);
    comfirm_check_param[timer_cnt].resend_cnt = 0;
    comfirm_check_param[timer_cnt].timer_cnt = timer_cnt;
    comfirm_check_param[timer_cnt].indicate_tid = indicate_tid;
    comfirm_check_param[timer_cnt].buf = &timer_success;
    comfirm_check_param[timer_cnt].len = sizeof(timer_success);
    vendor_attr_status_send(&vendor_server_models[1], &ctx, &timer_success, sizeof(timer_success));
    timer_index[timer_cnt] = sys_timer_add(&comfirm_check_param[timer_cnt], comfirm_check, 400);

    //onoff_state msg send and check comfirm
    timer_cnt_get(&timer_cnt);
    comfirm_check_param[timer_cnt].resend_cnt = 0;
    comfirm_check_param[timer_cnt].timer_cnt = timer_cnt;
    comfirm_check_param[timer_cnt].indicate_tid = indicate_tid;
    comfirm_check_param[timer_cnt].buf = &onoff_repo_handle;
    comfirm_check_param[timer_cnt].len = sizeof(onoff_repo_handle);
    vendor_attr_status_send(&vendor_server_models[1], &ctx, &onoff_repo_handle, sizeof(onoff_repo_handle));
    timer_index[timer_cnt] = sys_timer_add(&comfirm_check_param[timer_cnt], comfirm_check, 400);
}

static void set_timer_start(u8      tid,
                            u32     delay_s,
                            u8      index,
                            bool    onoff)
{

    timer_cnt_get(&timer_cnt);

    timer_param[timer_cnt].tid       = tid;
    timer_param[timer_cnt].index     = index;
    timer_param[timer_cnt].onoff     = onoff;
    timer_param[timer_cnt].timer_cnt = timer_cnt;


    printf("time_cnt = %d\r\n", timer_cnt);
    timer_index[timer_cnt] = sys_timer_add(&timer_param[timer_cnt], timer_handler, delay_s * 1000);

    log_info("timer set delay %d second\r\n", delay_s);
}

static void vendor_attr_cfm(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    u8 cfm_tid = buffer_pull_u8_from_head(buf);
    indicate_flag[cfm_tid] = 1;
    log_info("receice vendor_attr_confirm, indicate_tid = %d\r\n", cfm_tid);
}

static void vendor_attr_get(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    log_info("receive vendor_attr_get, len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);
}

static void vendor_attr_set(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    log_info("receive vendor_attr_set, len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);

    static struct UTC_TIME cur_utc;
    u8 tid = buffer_pull_u8_from_head(buf);
    u16 Attr_Type = buffer_pull_le16_from_head(buf);
    switch (Attr_Type) {
    case ATTR_TYPE_UNIX_TIME: {
        u32 time = buffer_pull_le32_from_head(buf);

        cur_utc = unix32_to_UTC_beijing(time);
        log_info("\n        __unix_time BeiJing time: %d/%d/%d %02d:%02d:%02d, weekday %d   \n",
                 cur_utc.year, cur_utc.month, cur_utc.day,
                 cur_utc.hour, cur_utc.minute, cur_utc.second,
                 cur_utc.weekday);

        struct __unix_time unix_time = {
            .Opcode = buffer_head_init(VENDOR_MSG_ATTR_STATUS),
            .TID = tid,
            .Attr_Type = Attr_Type,
            .time = time,
        };

        vendor_attr_status_send(model, ctx, &unix_time, sizeof(unix_time));
    }
    break;

    case ATTR_TYPE_SET_TIMEOUT: {
        u8 index = buffer_pull_u8_from_head(buf);
        u32 time = buffer_pull_le32_from_head(buf);
        u16 attr_type = buffer_pull_le16_from_head(buf);
        u8 attr_para = buffer_pull_u8_from_head(buf);

        struct UTC_TIME set_cur_utc = unix32_to_UTC_beijing(time);
        log_info("\n    __set_timeout BeiJinG time: %d/%d/%d %02d:%02d:%02d, weekday %d\n",
                 set_cur_utc.year, set_cur_utc.month, set_cur_utc.day,
                 set_cur_utc.hour, set_cur_utc.minute, set_cur_utc.second,
                 set_cur_utc.weekday);

        u32 delay_s = ((set_cur_utc.hour - cur_utc.hour) * 3600) + ((set_cur_utc.minute - cur_utc.minute) * 60) + (set_cur_utc.second - cur_utc.second);

        printf("\n  delay_s = %d, switch set to %d\n", delay_s, attr_para);

        set_timer_start(tid, delay_s, index, attr_para);

        struct __set_timeout set_timeout = {
            .Opcode = buffer_head_init(VENDOR_MSG_ATTR_STATUS),
            .TID = tid,
            .Attr_Type = Attr_Type,
            .param[0] = {
                .index = index,
                .time = time,
                .attr_type = attr_type,
                .attr_para = attr_para,
            },
        };
        vendor_attr_status_send(model, ctx, &set_timeout, sizeof(set_timeout));
    }
    break;

    case ATTR_TYPE_SET_PERIOD_TIMEOUT: {
        u8 index = buffer_pull_u8_from_head(buf);
        u16 _24h_timer = buffer_pull_le16_from_head(buf);
        u8 schedule = buffer_pull_u8_from_head(buf);
        u16 attr_type = buffer_pull_le16_from_head(buf);
        u8 attr_para = buffer_pull_u8_from_head(buf);

        log_info("\n    set_period_timeout BeiJinG time: %02d:%02d, weekday %d\n",
                 (_24h_timer & 0x0fff) / 60, (_24h_timer & 0x0fff) % 60, schedule);

        struct __set_period_timeout set_period_timeout = {
            .Opcode = buffer_head_init(VENDOR_MSG_ATTR_STATUS),
            .TID = tid,
            .Attr_Type = Attr_Type,
            .period_param[0] = {
                .index = index,
                ._24h_timer = _24h_timer,
                .schedule = schedule,
                .attr_type = attr_type,
                .attr_para = attr_para,
            },
        };
        printf("\n  time = 0x%x\n", _24h_timer);
        printf("\n          set_period opcode = 0x%x, Attr_Type = 0x%x, para = 0x%x, index = 0x%x       \n", set_period_timeout.Opcode, Attr_Type, attr_para, index);
        vendor_attr_status_send(model, ctx, &set_period_timeout, sizeof(set_period_timeout));
    }
    break;

    case ATTR_TYPE_DELETE_TIMEOUT: {
        u8 index = buffer_pull_u8_from_head(buf);
        struct __delete_time delete_time = {
            .Opcode = buffer_head_init(VENDOR_MSG_ATTR_STATUS),
            .TID = tid,
            .Attr_Type = Attr_Type,
            .index = index,
        };
        printf("delete timeout for index = 0x%x\r\n", index);
        vendor_attr_status_send(model, ctx, &delete_time, sizeof(delete_time));
    }
    break;

    case ATTR_TYPE_WIND_SPEED: {
        u8 level = buffer_pull_u8_from_head(buf);
        struct __fan_speed fan_speed = {
            .Opcode = buffer_head_init(VENDOR_MSG_ATTR_STATUS),
            .TID = tid,
            .Attr_Type = Attr_Type,
            .level = level,
        };

        if (level > 5) {
            printf("max level = 5\n");
            level = 5;
        }
        printf("fan level set to %d\r\n", level);

        set_led_duty(level * 2000);

        vendor_attr_status_send(model, ctx, &fan_speed, sizeof(fan_speed));
    }
    break;

    default :
        printf("\n\n\n\n      default attr = 0x%x \n\n\n\n", Attr_Type);
        break;
    }
}

static void iot_reset(void *p)
{
    bt_mesh_reset();
    extern void p33_soft_reset(void);
    p33_soft_reset();
}

static void vendor_attr_del(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    u8 tid = buffer_pull_u8_from_head(buf);
    u16 Attr_Type = buffer_pull_le16_from_head(buf);

    log_info("Node was del, reset myself\n");
    sys_timeout_add(NULL, iot_reset, 3000);
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
    { VENDOR_MSG_ATTR_GET,     ACCESS_OP_SIZE, vendor_attr_get },
    { VENDOR_MSG_ATTR_SET,     ACCESS_OP_SIZE, vendor_attr_set },
    { VENDOR_MSG_ATTR_DEL,     1, vendor_attr_del },
    { VENDOR_MSG_ATTR_CONFIRM, 1, vendor_attr_cfm },
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
};

static struct bt_mesh_model vendor_server_models[] = {
    BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_CLI, vendor_srv_op, NULL, NULL),
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
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

/*
 * @brief AliGenie UUID格式
 *
 * detail on https://www.aligenie.com/doc/357554/gtgprq <表1：Device UUID格式>
 */
/*-----------------------------------------------------------*/
static const u8 dev_uuid[16] = {
    0xA8, 0x01,  // CID
    0x01 | BIT(4) | BIT(6), // PID
    PID_TO_LITTLE_ENDIAN(PRODUCT_ID), // ProductID
    MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR), // MAC
    0x00,//BIT(1), // FeatureFlag
    0x00, 0x00 // RFU
};

/*
 * @brief AliGenie 设备配网流程
 *
 * detail on https://www.aligenie.com/doc/357554/gtgprq
 */
/*-----------------------------------------------------------*/
static u8 auth_data[16];

static void hex_to_string(void *in, int in_len, void *out, int out_len)
{
    static const char hex[] = "0123456789abcdef";
    u8 *a = in;
    u8 *b = out;
    int i;

    if (in_len > (out_len / 2)) {
        log_error("\"error: in_len > (out_len / 2)\"");
        return;
    }

    for (i = 0; i < in_len; i++) {
        b[i * 2]     = hex[a[i] >> 4];
        b[i * 2 + 1] = hex[a[i] & 0xf];
    }
}

static void static_auth_value_calculate(void)
{
    const u8 Product_ID[] = {
        PID_TO_BIG_ENDIAN(PRODUCT_ID)
    };
    const u8 Mac_Address[] = {
        MAC_TO_BIG_ENDIAN(CUR_DEVICE_MAC_ADDR)
    };
    const u8 Secret[] = {
        DEVICE_SECRET
    };

    u8 string_buf[PRODUCT_ID_STRING_SIZE + MAC_ADDRESS_STRING_SIZE + SECRET_STRING_SIZE + 2];
    u8 *string_p = string_buf;
    u8 digest[SHA256_DIGEST_SIZE];

    //< Product ID
    hex_to_string((void *)Product_ID, sizeof(Product_ID), string_p, PRODUCT_ID_STRING_SIZE);
    string_p += PRODUCT_ID_STRING_SIZE;
    *string_p++ = ',';
    //< Mac Address
    hex_to_string((void *)Mac_Address, sizeof(Mac_Address), string_p, MAC_ADDRESS_STRING_SIZE);
    string_p += MAC_ADDRESS_STRING_SIZE;
    *string_p++ = ',';
    //< Secert
    memcpy(string_p, Secret, SECRET_STRING_SIZE);

    sha256Compute(string_buf, sizeof(string_buf), digest);

    memcpy(auth_data, digest, sizeof(auth_data));

    log_info("string_buf : %s", string_buf);
    log_info_hexdump(string_buf, sizeof(string_buf));
    log_info_hexdump(auth_data, sizeof(auth_data));
}

static const struct bt_mesh_prov prov = {
    .uuid = dev_uuid,
    .static_val = auth_data,
    .static_val_len = sizeof(auth_data),
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

static void led_set(void)
{
    led_flag = !led_flag;

    indicate_tid_get(&indicate_tid);
    timer_cnt_get(&timer_cnt);

    set_led_duty(led_flag ? 10000 : 0);
    log_info("state set to %d, indicate_tid now = %d\r\n", led_flag, indicate_tid);

    if (!bt_mesh_is_provisioned()) {
        printf("no provision, not send state indicate\r\n");
        return;
    }
    struct bt_mesh_msg_ctx ctx = {
        .addr = 0xf000,
    };

    static struct __onoff_repo onoff_repo;
    onoff_repo.Opcode = buffer_head_init(VENDOR_MSG_ATTR_INDICAT);
    onoff_repo.TID = indicate_tid;
    onoff_repo.Attr_Type = 0x0100;
    onoff_repo.OnOff = led_flag;

    indicate_flag[indicate_tid] = 0;

    comfirm_check_param[timer_cnt].resend_cnt = 0;
    comfirm_check_param[timer_cnt].timer_cnt = timer_cnt;
    comfirm_check_param[timer_cnt].indicate_tid = indicate_tid;
    comfirm_check_param[timer_cnt].buf = &onoff_repo;
    comfirm_check_param[timer_cnt].len = sizeof(onoff_repo);

    vendor_attr_status_send(&vendor_server_models[1], &ctx, &onoff_repo, sizeof(onoff_repo));
    timer_index[timer_cnt] = sys_timer_add(&comfirm_check_param[timer_cnt], comfirm_check, 400);
}

static u16 blink_id;

static void led_blink(void *p)
{
    static bool blink_flag = 0;
    static u8 blink_cnt = 0;
    blink_flag = ~blink_flag;
    blink_cnt += 1;
    gpio_pin_write(LED1_GPIO_PIN, blink_flag);
    if (blink_cnt == 6) {
        sys_timer_remove(blink_id);
    }
}

void input_key_handler(u8 key_status, u8 key_number)
{
    struct _switch press_switch;
    log_info("key_number=0x%x", key_number);

    if ((key_number == 2) && (key_status == KEY_EVENT_CLICK)) {
        log_info("\n  <bt_mesh_reset> \n");
        struct __indicate_msg HardReset_msg = {
            .Opcode     = buffer_head_init(VENDOR_MSG_ATTR_INDICAT),
            .TID        = indicate_tid,
            .Attr_Type  = 0xf009,
            .Event      = 0x23,
        };
        struct bt_mesh_msg_ctx ctx = {
            .addr = 0x7fff,
        };
        vendor_attr_status_send(&vendor_server_models[1], &ctx, &HardReset_msg, sizeof(HardReset_msg));
        blink_id = sys_timer_add(NULL, led_blink, 300);
        sys_timer_add(NULL, iot_reset, 1000 * 3);
        return;
    }

    if ((key_number == 1) && (key_status == KEY_EVENT_LONG)) {
        extern void power_set_soft_poweroff(void);
        power_set_soft_poweroff();
        return;
    }

    if ((key_number == 0) && (key_status == KEY_EVENT_CLICK)) {
        led_set();
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

static void period_msg(void *empty)
{
    struct bt_mesh_msg_ctx ctx = {
        .addr = 0xf000,
    };
    struct __onoff_repo period_indicat = {
        .Opcode = buffer_head_init(VENDOR_MSG_ATTR_INDICAT),
        .TID = indicate_tid,
        .Attr_Type = 0x0100,    //设备开关状态，与generic onoff绑定
        .OnOff = led_flag,
    };

    if (bt_mesh_is_provisioned()) {
        vendor_attr_status_send(&vendor_server_models[1], &ctx, &period_indicat, sizeof(period_indicat));
    }
}

static void iot_init(void)
{
    if (bt_mesh_is_provisioned()) {
        indicate_tid_get(&indicate_tid);
        struct bt_mesh_msg_ctx ctx = {
            .addr = 0xf000,
        };
        struct __onoff_repo state_msg = {
            .Opcode = buffer_head_init(VENDOR_MSG_ATTR_INDICAT),
            .TID = indicate_tid,
            .Attr_Type = 0x0100,    //设备开关状态，与generic onoff绑定
            .OnOff = led_flag,
        };
        vendor_attr_status_send(&vendor_server_models[1], &ctx, &state_msg, sizeof(state_msg));

        indicate_tid_get(&indicate_tid);
        struct __indicate_msg indicate_msg = {
            .Opcode     = buffer_head_init(VENDOR_MSG_ATTR_INDICAT),
            .TID        = indicate_tid,
            .Attr_Type  = 0xf009,
            .Event      = 0x03,
        };
        vendor_attr_status_send(&vendor_server_models[1], &ctx, &indicate_msg, sizeof(indicate_msg));
    }
    sys_timer_add(NULL, period_msg, 180 * 1000);
}

/*
 * @brief Mesh Profile Setup
 */
/*-----------------------------------------------------------*/
static void mesh_init(void)
{
    static_auth_value_calculate();

    int err = bt_mesh_init(&prov, &composition);
    if (err) {
        log_error("Initializing mesh failed (err %d)\n", err);
        return;
    }

    settings_load();

    bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);

    printf("\nThis board had not set up timer_pwm\n");

    iot_init();     //indicate_state
}

void bt_ble_mesh_init(void)
{
    u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

    bt_mac_addr_set(bt_addr);

    mesh_setup(mesh_init);
}

#endif /* (CONFIG_MESH_MODEL == SIG_MESH_ALIGENIE_SOCKET) */

