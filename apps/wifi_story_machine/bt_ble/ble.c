#include "app_config.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "btstack/btstack_task.h"
#include "btstack/le/att.h"
#include "btstack/le/le_user.h"
#include "bt_common.h"
#include "le_net_central.h"
#include "../multi_demo/le_multi_common.h"

#if BT_NET_CENTRAL_EN || TRANS_MULTI_BLE_MASTER_NUMS

//指定搜索uuid
//指定搜索uuid
static const target_uuid_t search_uuid_table[] = {

    // for uuid16
    // PRIMARY_SERVICE, ae80
    // CHARACTERISTIC,  ae81, WRITE_WITHOUT_RESPONSE | DYNAMIC,
    // CHARACTERISTIC,  ae82, NOTIFY,

    {
        .services_uuid16 = 0xae80,
        .characteristic_uuid16 = 0xae81,
        .opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
    },

    {
        .services_uuid16 = 0xae80,
        .characteristic_uuid16 = 0xae82,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },

    {
        .services_uuid16 = 0xae00,  //  6901A
        .characteristic_uuid16 = 0xae01,
        .opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
    },

    {
        .services_uuid16 = 0xae00,  //  6901A
        .characteristic_uuid16 = 0xae01,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },

    //for uuid128,sample
    //	PRIMARY_SERVICE, 0000F530-1212-EFDE-1523-785FEABCD123
    //	CHARACTERISTIC,  0000F531-1212-EFDE-1523-785FEABCD123, NOTIFY,
    //	CHARACTERISTIC,  0000F532-1212-EFDE-1523-785FEABCD123, WRITE_WITHOUT_RESPONSE | DYNAMIC,
    /*
    	{
    		.services_uuid16 = 0,
    		.services_uuid128 =       {0x00,0x00,0xF5,0x30 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23} ,
    		.characteristic_uuid16 = 0,
    		.characteristic_uuid128 = {0x00,0x00,0xF5,0x31 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23},
    		.opt_type = ATT_PROPERTY_NOTIFY,
    	},

    	{
    		.services_uuid16 = 0,
    		.services_uuid128 =       {0x00,0x00,0xF5,0x30 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23} ,
    		.characteristic_uuid16 = 0,
    		.characteristic_uuid128 = {0x00,0x00,0xF5,0x32 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23},
    		.opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
    	},
    */
};

static void ble_report_data_deal(att_data_report_t *report_data, const target_uuid_t *search_uuid)
{
    log_i("report_data:%02x,%02x,%d,len(%d)", report_data->packet_type,
          report_data->value_handle, report_data->value_offset, report_data->blob_length);

    put_buf(report_data->blob, report_data->blob_length);

    //处理接收数据

    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION:  //notify
        break;

    case GATT_EVENT_INDICATION://indicate
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT://read
        break;

    case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT://read long
        break;

    default:
        break;
    }
}

static const struct ble_client_operation_t *ble_client_api;
static const u8 test_remoter_name1[] = "JL-AC79XX-AF0B(BLE)";//
static const u8 test_remoter_name2[] = "6901toy211-ble";//
static u16 ble_client_write_handle;

static const client_match_cfg_t match_dev01 = {
    .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
    .compare_data_len = sizeof(test_remoter_name1) - 1, //去结束符
    .compare_data = test_remoter_name1,
    .bonding_flag = 0,
};

static const client_match_cfg_t match_dev02 = {
    .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
    .compare_data_len = sizeof(test_remoter_name2) - 1, //去结束符
    .compare_data = test_remoter_name2,
    .bonding_flag = 0,
};

static const unsigned char test_data[] = {
    0x4A, 0x4C, 0x00, 0x21, 0x10, 0x01,
    0x7B, 0x22, 0x73, 0x73, 0x69, 0x64, 0x22, 0x3A, 0x22, 0x49, 0x54,
    0x22, 0x2C, 0x22, 0x70, 0x61, 0x73, 0x73, 0x22, 0x3A, 0x22, 0x31, 0x32, 0x33, 0x34,
    0x35, 0x36, 0x37, 0x38, 0x22, 0x7D,
    0x64, 0xCF, 0xFF
};

static void client_event_callback(le_client_event_e event, u8 *packet, int size)
{
    switch (event) {
    case CLI_EVENT_MATCH_DEV:
        log_i("match_name:%s\n", ((client_match_cfg_t *)packet)->compare_data);
        break;

    case CLI_EVENT_MATCH_UUID:
        ;
        opt_handle_t *opt_hdl = (opt_handle_t *)packet;
        if (opt_hdl->search_uuid == &search_uuid_table[0]) {
            ble_client_write_handle = opt_hdl->value_handle;
            log_i("match_uuid\n");
        }
        break;

    case CLI_EVENT_SEARCH_PROFILE_COMPLETE:
        log_i("CLI_EVENT_SEARCH_PROFILE_COMPLETE\n");
        //可以开始发送数据
#if TRANS_MULTI_BLE_MASTER_NUMS
        for (int i = 0; i < TRANS_MULTI_BLE_MASTER_NUMS; i++) {
            u16 tmp_handle = mul_dev_get_conn_handle(i, MULTI_ROLE_CLIENT);
            if (tmp_handle) {
                ble_client_api->opt_comm_send_ext(tmp_handle, ble_client_write_handle, (u8 *)test_data, sizeof(test_data), ATT_OP_WRITE_WITHOUT_RESPOND);
            }
        }
#else
        if (ble_client_write_handle) {
            ble_client_api->opt_comm_send(ble_client_write_handle, (u8 *)test_data, sizeof(test_data), ATT_OP_WRITE_WITHOUT_RESPOND);
        }
#endif
        break;

    case CLI_EVENT_CONNECTED:
        break;

    case CLI_EVENT_DISCONNECT:
        ble_client_write_handle = 0;
        break;

    default:
        break;
    }
}

static const client_conn_cfg_t client_conn_config = {
    .match_dev_cfg[0] = &match_dev01,
    .match_dev_cfg[1] = &match_dev02,
    .report_data_callback = ble_report_data_deal,
    /* .search_uuid_cnt = 0, //配置不搜索profile，加快回连速度 */
    .search_uuid_cnt = (sizeof(search_uuid_table) / sizeof(target_uuid_t)),
    .search_uuid_table = search_uuid_table,
    .security_en = 0,
    .event_callback = client_event_callback,
};

void ble_client_config_init(void)
{
    ble_client_api = ble_get_client_operation_table();
    ble_client_api->init_config(0, (void *)&client_conn_config);
    /* client_clear_bonding_info();//for test */
}

#endif

