#include "app_config.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "btstack/btstack_task.h"
#include "btstack/le/att.h"
#include "btstack/le/le_user.h"
#include "btstack/avctp_user.h"
#include "bt_common.h"
#include "le_common.h"
#include "le_net_central.h"
#include "event/bt_event.h"
#include "syscfg/syscfg_id.h"
#include "../multi_demo/le_multi_common.h"

const int config_update_mode = 0;
void (*lmp_ch_update_resume_hdl)(void *priv) = NULL;

void bt_ble_module_init(void)
{
    void lmp_set_sniff_disable(void);
    lmp_set_sniff_disable();

    bt_max_pwr_set(3, 2, 2, 2);	//0-5

#if TCFG_USER_BLE_ENABLE
    u8 tmp_ble_addr[6];
    extern const u8 *bt_get_mac_addr(void);
    extern void lib_make_ble_address(u8 * ble_address, u8 * edr_address);
    extern int le_controller_set_mac(void *addr);
    lib_make_ble_address(tmp_ble_addr, (u8 *)bt_get_mac_addr());
    le_controller_set_mac((void *)tmp_ble_addr);
    printf("\n-----edr + ble 's address-----");
    put_buf((void *)bt_get_mac_addr(), 6);
    put_buf((void *)tmp_ble_addr, 6);
#if BT_NET_CENTRAL_EN || TRANS_MULTI_BLE_MASTER_NUMS
    extern void ble_client_config_init(void);
    ble_client_config_init();
#endif
#endif

    btstack_init();
}

static int bt_connction_status_event_handler(struct bt_event *bt)
{
    switch (bt->event) {
    case BT_STATUS_INIT_OK:
#if BT_NET_CENTRAL_EN
        extern void bt_master_ble_init(void);
        bt_master_ble_init();
#endif
        break;
    }

    return 0;
}

int ble_demo_bt_event_handler(struct sys_event *event)
{
    if (event->from == BT_EVENT_FROM_CON) {
        return bt_connction_status_event_handler((struct bt_event *)event->payload);
    }

    return 0;
}


static const struct ble_client_operation_t *ble_client_api;
static const u8 test_remoter_name1[] = "abc123(BLE)";//
static u16 ble_client_write_handle;

static const client_match_cfg_t match_dev01 = {
    .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
    .compare_data_len = sizeof(test_remoter_name1) - 1, //去结束符
    .compare_data = test_remoter_name1,
    .bonding_flag = 0,
};

static const unsigned char test_data[] = {
    0x4A, 0x4C, 0x00, 0x21, 0x10, 0x01,
    0x7B, 0x22, 0x73, 0x73, 0x69, 0x64, 0x22, 0x3A, 0x22, 0x49, 0x54,
    0x22, 0x2C, 0x22, 0x70, 0x61, 0x73, 0x73, 0x22, 0x3A, 0x22, 0x31, 0x32, 0x33, 0x34,
    0x35, 0x36, 0x37, 0x38, 0x22, 0x7D,
    0x64, 0xCF, 0xFF
};

//用户需要填写以下过滤规则，填写错误会出现一直连接不上的情况
//指定搜索uuid
static const target_uuid_t search_uuid_table[] = {

    {
        .services_uuid16 = 0x1800,
        .characteristic_uuid16 = 0x2a00,
        .opt_type = ATT_PROPERTY_READ,
    },

    {
        .services_uuid16 = 0xff00,
        .characteristic_uuid16 = 0xff01,
        .opt_type = ATT_PROPERTY_READ,
    },

    {
        .services_uuid16 = 0xff00,
        .characteristic_uuid16 = 0xff02,
        .opt_type = ATT_PROPERTY_WRITE,
    },

    {
        .services_uuid16 = 0xff00,
        .characteristic_uuid16 = 0xff02,
        .opt_type = ATT_PROPERTY_READ,
    },

    {
        .services_uuid16 = 0xff00,
        .characteristic_uuid16 = 0xff03,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },
};


static void ble_report_data_deal(att_data_report_t *report_data, const target_uuid_t *search_uuid)
{
    log_i("report_data:%02x,%02x,%d,len(%d)", report_data->packet_type,
          report_data->value_handle, report_data->value_offset, report_data->blob_length);

    put_buf(report_data->blob, report_data->blob_length);

    //处理接收数据

    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION:  //notify
        log_i("GATT_EVENT_NOTIFICATION");
        break;

    case GATT_EVENT_INDICATION://indicate
        log_i("GATT_EVENT_INDICATION");
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT://read
        break;

    case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT://read long
        break;

    default:
        break;
    }
}


static void write_test_timer(void *priv)
{
    //发送测试数据
    if (ble_client_write_handle) {
        int ret = ble_client_api->opt_comm_send(ble_client_write_handle, (u8 *)test_data, sizeof(test_data), ATT_OP_WRITE);
        log_i("ret = %d\n", ret);
    }
}


static void client_event_callback(le_client_event_e event, u8 *packet, int size)
{
    switch (event) {
    case CLI_EVENT_MATCH_DEV:
        log_i("match_name:%s\n", ((client_match_cfg_t *)packet)->compare_data);
        break;

    case CLI_EVENT_MATCH_UUID:
        log_i("CLI_EVENT_MATCH_UUID\n");
        opt_handle_t *opt_hdl = (opt_handle_t *)packet;
        if (opt_hdl->search_uuid == &search_uuid_table[2]) {
            ble_client_write_handle = opt_hdl->value_handle;
            log_i("match_uuid, %x\n", ble_client_write_handle);
        }
        break;

    case CLI_EVENT_SEARCH_PROFILE_COMPLETE:
        log_i("CLI_EVENT_SEARCH_PROFILE_COMPLETE\n");
        //可以开始发送数据
        u16 sys_timer_add(void *priv, void (*func)(void *priv), u32 msec);
        sys_timer_add(NULL, write_test_timer, 1000);
        break;

    case CLI_EVENT_CONNECTED:
        log_i("CLI_EVENT_CONNECTED\n");
        break;

    case CLI_EVENT_DISCONNECT:
        log_i("CLI_EVENT_DISCONNECT\n");
        ble_client_write_handle = 0;
        break;

    default:
        break;
    }
}

static const client_conn_cfg_t client_conn_config = {
    .security_en = 0,
    .match_dev_cfg[0] = &match_dev01,
    /* .search_uuid_cnt = 0, //配置不搜索profile，加快回连速度 */
    .search_uuid_cnt = (sizeof(search_uuid_table) / sizeof(target_uuid_t)),
    .search_uuid_table = search_uuid_table,
    .report_data_callback = ble_report_data_deal,
    .event_callback = client_event_callback,
};


void ble_client_config_init(void)
{
    ble_client_api = ble_get_client_operation_table();
    ble_client_api->init_config(0, (void *)&client_conn_config);
}


