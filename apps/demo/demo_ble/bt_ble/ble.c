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

void (*lmp_ch_update_resume_hdl)(void *priv) = NULL;

void bt_ble_module_init(void)
{
    void lmp_set_sniff_disable(void);
    lmp_set_sniff_disable();

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
#if TRANS_MULTI_BLE_EN
        bt_ble_init();
#else

#if BT_NET_CENTRAL_EN
        extern void bt_master_ble_init(void);
        bt_master_ble_init();
#endif
#if TRANS_DATA_EN
        bt_ble_init();
#endif
#if BT_NET_HID_EN
        extern void ble_module_enable(u8 en);
        extern void ble_hid_set_config(void);
        ble_hid_set_config();
        bt_ble_init();
#endif
#if CONFIG_BLE_MESH_ENABLE
        extern void bt_ble_mesh_init(void);
        bt_ble_mesh_init();
#endif

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

#if BT_NET_CENTRAL_EN || TRANS_MULTI_BLE_MASTER_NUMS

//用户需要填写以下过滤规则，填写错误会出现一直连接不上的情况
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
        //发送测试数据
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

#if BT_NET_HID_EN

#include "standard_hid.h"

static const u8 hid_descriptor_keyboard_boot_mode[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0x65,        //   Usage Maximum (0x65)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

//----------------------------------
#define KEYBOARD_REPORT_MAP \
	USAGE_PAGE(CONSUMER_PAGE),              \
	USAGE(CONSUMER_CONTROL),                \
	COLLECTION(APPLICATION),                \
	REPORT_ID(1),      \
	USAGE(VOLUME_INC),                  \
	USAGE(VOLUME_DEC),                  \
	USAGE(PLAY_PAUSE),                  \
	USAGE(MUTE),                        \
	USAGE(SCAN_PREV_TRACK),             \
	USAGE(SCAN_NEXT_TRACK),             \
	USAGE(FAST_FORWARD),                \
	USAGE(REWIND),                      \
	LOGICAL_MIN(0),                     \
	LOGICAL_MAX(1),                     \
	REPORT_SIZE(1),                     \
	REPORT_COUNT(16),                   \
	INPUT(0x02),                        \
	END_COLLECTION,                         \

static const u8 hid_report_map[] = {KEYBOARD_REPORT_MAP};

void ble_hid_set_config(void)
{
    le_hogp_set_icon(0x03c1);//ble keyboard demo
    le_hogp_set_ReportMap(hid_descriptor_keyboard_boot_mode, sizeof(hid_descriptor_keyboard_boot_mode));
}
#endif


