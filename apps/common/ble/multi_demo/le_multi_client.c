/*********************************************************************************************
    *   Filename        : le_multi_client.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2017-01-17 11:14

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/

// *****************************************************************************
/* EXAMPLE_START(le_counter): LE Peripheral - Heartbeat Counter over GATT
 *
 * @text All newer operating systems provide GATT Client functionality.
 * The LE Counter examples demonstrates how to specify a minimal GATT Database
 * with a custom GATT Service and a custom Characteristic that sends periodic
 * notifications.
 */
// *****************************************************************************

#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btcontroller_modules.h"
#include "bt_common.h"

#include "le_net_central.h"
#include "le_common.h"
#include "ble_user.h"
#include "le_multi_common.h"
#include "syscfg/syscfg_id.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_MULTI && SUPPORT_MAX_CLIENT)

#define SUPPORT_TEST_BOX_BLE_MASTER_TEST_EN	1
#define SHOW_RX_DATA_RATE           0
#define EXT_ADV_MODE_EN             0

#if LE_DEBUG_PRINT_EN
//#define log_info            g_printf
#define log_info(x, ...)    g_printf("[LE_MUL_CLIENT]" x " ", ## __VA_ARGS__)
#define log_info_hexdump    put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//---------------
#define SEARCH_PROFILE_BUFSIZE    (512)                   //note:
static u8 search_ram_buffer[SEARCH_PROFILE_BUFSIZE] __attribute__((aligned(4)));
#define scan_buffer search_ram_buffer
//---------------
#define BASE_INTERVAL_MIN         (6)//最小的interval
#if (SUPPORT_MAX_CLIENT > 2)
#define BASE_INTERVAL_VALUE       (BASE_INTERVAL_MIN*4)
#else
#define BASE_INTERVAL_VALUE       (BASE_INTERVAL_MIN)
#endif

//搜索类型
#define SET_SCAN_TYPE       SCAN_ACTIVE
//搜索 周期大小
#define SET_SCAN_INTERVAL   ADV_SCAN_MS(24) // unit: ms
#define SET_SCAN_WINDOW     ADV_SCAN_MS(8) // unit: ms
//连接周期
#define SET_CONN_INTERVAL   (BASE_INTERVAL_MIN*8) //(unit:1.25ms)
//连接latency
#define SET_CONN_LATENCY    0  //(unit:conn_interval)
//连接超时
#define SET_CONN_TIMEOUT    400 //(unit:10ms)

//----------------------------------------------------------------------------
static s8 cur_dev_cid; // 当前操作的多机id
static u8 scan_ctrl_en;
static u8 ble_work_state[SUPPORT_MAX_CLIENT];
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static u32 channel_priv;

#define BLE_VM_HEAD_TAG           (0xB95C)
#define BLE_VM_TAIL_TAG           (0x5CB9)
struct pair_info_t {
    u16 head_tag;
    u8  pair_flag: 2;
    u8  match_dev_id: 6;
    u8  peer_address_info[7];
    u16 conn_handle;
    u16 conn_interval;
    u16 conn_latency;
    u16 conn_timeout;
    u16 tail_tag;
};
struct conn_info_t {
    u16 conn_handle;
    u16 conn_interval;
    u16 conn_latency;
    u16 conn_timeout;
};

static struct conn_info_t  reconnect_info;
static struct pair_info_t  conn_pair_info;
static u8 pair_bond_enable = 0;
static u8 match_dev_id = 0;
static struct pair_info_t conn_pair_info_table[SUPPORT_MAX_CLIENT];
static u8 encryption_process; //0--not pair,1--first pair,2--reconnect_encryption
static u8 pair_reconnect_search_profile = 1;//回连是否搜索profile
static u8 sm_master_pair_redo = 0;//keymiss,是否重新执行配对
//-------------------------------------------------------------------------------
typedef struct {
    uint16_t read_handle;
    uint16_t read_long_handle;
    uint16_t write_handle;
    uint16_t write_no_respond;
    uint16_t notify_handle;
    uint16_t indicate_handle;
} target_hdl_t;

//记录handle 使用
static target_hdl_t target_handle[SUPPORT_MAX_CLIENT];
static opt_handle_t opt_handle_table[OPT_HANDLE_MAX];
static u8 opt_handle_used_cnt;
static u8 force_seach_onoff = 0;
static s8 force_seach_rssi = -127;
static u8 send_param_index = 3;

static void bt_ble_create_connection(u8 *conn_addr, u8 addr_type);
static int bt_ble_scan_enable(void *priv, u32 en);
static int client_write_send(void *priv, u8 *data, u16 len);
static int client_operation_send(u16 handle, u8 *data, u16 len, u8 att_op_type);
int ble_enable_new_dev_scan(void);
//---------------------------------------------------------------------------
static const client_conn_cfg_t *client_config = NULL;
//----------------------------------------------------------------------------

static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 0, 600},//11
    {12, 28, 0, 600},//3.7
    {8,  20, 0, 600},
    {50, 60, 0, 600},
};

static int client_create_connect_api(u8 *addr, u8 addr_type, u8 mode);
static int client_create_cannel_api(void);
//----------------------------------------------------------------------------

static void client_event_report(le_client_event_e event, u8 *packet, int size)
{
    if (client_config->event_callback) {
        client_config->event_callback(event, packet, size);
    }
}

static bool check_device_is_match(u8 info_type, u8 *data, int size)
{
    int i;
    u8  conn_mode = BIT(info_type);
    const client_match_cfg_t *cfg;

    /* log_info_hexdump(data,size); */

    for (i = 0; i < CLIENT_MATCH_CONN_MAX; i++) {
        cfg = client_config->match_dev_cfg[i];
        if (cfg == NULL) {
            continue;
        }
        /* log_info("cfg = %08x\n",cfg);	 */
        /* log_info_hexdump(cfg,sizeof(client_match_cfg_t)); */
        if (cfg->create_conn_mode == conn_mode && size == cfg->compare_data_len) {
            log_info("match check\n");
            /* log_info_hexdump(data, size); */
            /* log_info_hexdump(cfg->compare_data, size); */
            if (0 == memcmp(data, cfg->compare_data, cfg->compare_data_len)) {
                log_info("match ok:%d\n", cfg->bonding_flag);
                pair_bond_enable = cfg->bonding_flag;
                match_dev_id = i;
                client_event_report(CLI_EVENT_MATCH_DEV, (u8 *)cfg, sizeof(client_match_cfg_t));
                return true;
            }
        }
    }
    return false;
}

static void conn_pair_vm_do(struct pair_info_t *info, u8 rw_flag)
{
    /* return; */

    int ret;
    int unit_len = sizeof(struct pair_info_t);
    int i;

    log_info("-conn_pair_info vm_do:%d\n", rw_flag);

    if (rw_flag == 0) {
        if (unit_len * SUPPORT_MAX_CLIENT != syscfg_read(CFG_BLE_MODE_INFO, (u8 *)conn_pair_info_table, unit_len * SUPPORT_MAX_CLIENT)) {
            log_info("-table null--\n");
            memset(conn_pair_info_table, 0, sizeof(conn_pair_info_table));
        }

        for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
            if (conn_pair_info_table[i].head_tag != BLE_VM_HEAD_TAG
                || conn_pair_info_table[i].tail_tag != BLE_VM_TAIL_TAG) {
                //invail pair
                log_info("-invail,table %d!!!--\n", i);
                memset(&conn_pair_info_table[i], 0, unit_len);
            }
            log_info("read,pair_id= %d\n", i);
            put_buf(conn_pair_info_table[i].peer_address_info, 7);
        }

        /* if (info) { */
        /* memcpy(info, &conn_pair_info_table[0], unit_len); */
        /* } */
    } else {
        int fill_index = -1;

        if (!info) {
            log_info("-table clean--\n");
            memset(conn_pair_info_table, 0, sizeof(conn_pair_info_table));
        } else {
            for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
                if (0 == memcmp(info, &conn_pair_info_table[i], unit_len)) {
                    log_info("dev in table\n");
                    return;
                }
            }

            /* put_buf(conn_pair_info_table, unit_len * SUPPORT_MAX_CLIENT); */
            /* put_buf(info, unit_len); */

            r_printf("========write new dev=======\n");
            log_info("find table\n");

            /*find same address*/
            if (fill_index == -1) {
                for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
                    if (0 == memcmp(info->peer_address_info, &conn_pair_info_table[i].peer_address_info, 7)) {
                        fill_index = i;
                        break;
                    }
                }
            }

            /*find idle*/
            if (fill_index == -1) {
                for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
                    if (0 == conn_pair_info_table[i].pair_flag) {
                        fill_index = i;
                        break;
                    }
                }
            }

            /*find old match_dev_id*/
            if (fill_index == -1) {
                for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
                    if (conn_pair_info_table[i].match_dev_id == info->match_dev_id) {
                        log_info("replace match_dev_id= %d\n", info->match_dev_id);
                        fill_index = i;
                        break;
                    }
                }
            }

            /*find old*/
            if (fill_index == -1) {
                for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
                    memcpy(&conn_pair_info_table[i - 1], &conn_pair_info_table[i], unit_len);
                }
                fill_index = SUPPORT_MAX_CLIENT - 1;
            }

            /*destroy same handle*/
            for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
                if (conn_pair_info_table[i].conn_handle == info->conn_handle) {
                    conn_pair_info_table[i].conn_handle = 0;
                    break;
                }
            }

            log_info("write,pair_id= %d\n", fill_index);
            put_buf(info->peer_address_info, 7);
            memcpy(&conn_pair_info_table[fill_index], info, unit_len);
            conn_pair_info_table[fill_index].head_tag = BLE_VM_HEAD_TAG;
            conn_pair_info_table[fill_index].tail_tag = BLE_VM_TAIL_TAG;
        }
        syscfg_write(CFG_BLE_MODE_INFO, (u8 *)conn_pair_info_table, unit_len * SUPPORT_MAX_CLIENT);
    }
    /* put_buf(conn_pair_info_table, unit_len * SUPPORT_MAX_CLIENT); */
}

void client_clear_bonding_info(void)
{
    log_info("client_clear_bonding_info\n");
    //del pair bond
    memset(&conn_pair_info, 0, sizeof(struct pair_info_t));
    conn_pair_info.head_tag = BLE_VM_HEAD_TAG;
    conn_pair_info.tail_tag = BLE_VM_TAIL_TAG;
    conn_pair_vm_do(NULL, 1);
}

//------------------------------------------------------------
static void set_ble_work_state(u8 cid, ble_state_e state)
{
    if (state != ble_work_state[cid]) {
        log_info("ble_client_work_st[%d]:%x->%x\n", cid, ble_work_state[cid], state);
        ble_work_state[cid] = state;
        if (app_ble_state_callback) {
            app_ble_state_callback((void *)channel_priv, state);
        }
    }
}
//------------------------------------------------------------
static ble_state_e get_ble_work_state(u8 cid)
{
    return ble_work_state[cid];
}

//-------------------------------------------------------------------------------
static void check_target_uuid_match(search_result_t *result_info)
{
    u32 i;
    const target_uuid_t *t_uuid;

    for (i = 0; i < client_config->search_uuid_cnt; i++) {
        t_uuid = &client_config->search_uuid_table[i];
        if (result_info->services.uuid16) {
            if (result_info->services.uuid16 != t_uuid->services_uuid16) {
                /* log_info("b1"); */
                continue;
            }
        } else {
            if (memcmp(result_info->services.uuid128, t_uuid->services_uuid128, 16)) {
                /* log_info("b2"); */
                continue;
            }
        }

        if (result_info->characteristic.uuid16) {
            if (result_info->characteristic.uuid16 != t_uuid->characteristic_uuid16) {
                /* log_info("b3"); */
                /* log_info("%d: %04x--%04x",result_info->characteristic.uuid16,t_uuid->characteristic_uuid16); */
                continue;
            }
        } else {
            if (memcmp(result_info->characteristic.uuid128, t_uuid->characteristic_uuid128, 16)) {
                /* log_info("b4"); */
                continue;
            }
        }

        break;//match one
    }

    if (i >= client_config->search_uuid_cnt) {
        return;
    }

    if (opt_handle_used_cnt >= OPT_HANDLE_MAX) {
        log_info("opt_handle is full!!!\n");
        return;
    }

    if ((t_uuid->opt_type & result_info->characteristic.properties) != t_uuid->opt_type) {
        log_info("properties not match!!!\n");
        return;
    }

    log_info("match one uuid\n");

    opt_handle_t *opt_get = &opt_handle_table[opt_handle_used_cnt++];
    opt_get->value_handle = result_info->characteristic.value_handle;
    opt_get->search_uuid = t_uuid;

    switch (t_uuid->opt_type) {
    case ATT_PROPERTY_READ:
        target_handle[cur_dev_cid].read_handle = result_info->characteristic.value_handle;
        break;

    case ATT_PROPERTY_WRITE_WITHOUT_RESPONSE:
        target_handle[cur_dev_cid].write_no_respond = result_info->characteristic.value_handle;
        break;

    case ATT_PROPERTY_WRITE:
        target_handle[cur_dev_cid].write_handle = result_info->characteristic.value_handle;
        break;

    case ATT_PROPERTY_NOTIFY:
        target_handle[cur_dev_cid].notify_handle  = result_info->characteristic.value_handle;
        break;

    case ATT_PROPERTY_INDICATE:
        target_handle[cur_dev_cid].indicate_handle = result_info->characteristic.value_handle;
        break;

    default:
        break;
    }

    client_event_report(CLI_EVENT_MATCH_UUID, (u8 *)opt_get, sizeof(opt_handle_t));
}

//操作handle，完成 write ccc
static void do_operate_search_handle(void)
{
    u16 tmp_16;
    u16 i, cur_opt_type;
    opt_handle_t *opt_hdl_pt;

    log_info("opt_handle_used_cnt= %d\n", opt_handle_used_cnt);

    log_info("find target_handle:");
    log_info_hexdump((const u8 *)&target_handle[cur_dev_cid], sizeof(target_hdl_t));

    if (0 == opt_handle_used_cnt) {
        return;
        /* goto opt_end; */
    }

    /* test_send_conn_update();//for test */

    for (i = 0; i < opt_handle_used_cnt; i++) {
        opt_hdl_pt = &opt_handle_table[i];
        log_info("do opt:service_uuid16:%04x,charactc_uuid16:%04x\n", opt_hdl_pt->search_uuid->services_uuid16, opt_hdl_pt->search_uuid->characteristic_uuid16);
        cur_opt_type = opt_hdl_pt->search_uuid->opt_type;
        switch ((u8)cur_opt_type) {
        case ATT_PROPERTY_READ:
            if (opt_hdl_pt->search_uuid->read_long_enable) {
                tmp_16  = 0x55A2;//fixed
                log_info("read_long:%04x\n", opt_hdl_pt->value_handle);
                client_operation_send(opt_hdl_pt->value_handle, (u8 *)&tmp_16, 2, ATT_OP_READ_LONG);
            } else {
                tmp_16  = 0x55A1;//fixed
                log_info("read:%04x\n", opt_hdl_pt->value_handle);
                client_operation_send(opt_hdl_pt->value_handle, (u8 *)&tmp_16, 2, ATT_OP_READ);
            }
            break;

        case ATT_PROPERTY_NOTIFY:
            tmp_16  = 0x01;//fixed
            log_info("write_ntf_ccc:%04x\n", opt_hdl_pt->value_handle);
            client_operation_send(opt_hdl_pt->value_handle + 1, (u8 *)&tmp_16, 2, ATT_OP_WRITE);
            if (opt_hdl_pt->search_uuid->read_report_reference) {
                tmp_16  = 0x55A1;//fixed
                log_info("read_report_reference:%04x\n", opt_hdl_pt->value_handle + 2);
                client_operation_send(opt_hdl_pt->value_handle + 2, (u8 *)&tmp_16, 2, ATT_OP_READ);
            }
            break;

        case ATT_PROPERTY_INDICATE:
            tmp_16  = 0x02;//fixed
            log_info("write_ind_ccc:%04x\n", opt_hdl_pt->value_handle);
            client_operation_send(opt_hdl_pt->value_handle + 1, (u8 *)&tmp_16, 2, ATT_OP_WRITE);
            break;

        default:
            break;
        }
    }

opt_end:
    set_ble_work_state(cur_dev_cid, BLE_ST_SEARCH_COMPLETE);
}

//协议栈内部调用
//return: 0--accept,1--reject
int l2cap_connection_update_request_just(u8 *packet, hci_con_handle_t handle)
{
    log_info("slave request conn_update:\n-conn_handle= %04x\n-interval_min= %d,\n-interval_max= %d,\n-latency= %d,\n-timeout= %d\n",
             handle,
             little_endian_read_16(packet, 0), little_endian_read_16(packet, 2),
             little_endian_read_16(packet, 4), little_endian_read_16(packet, 6));

#if (SUPPORT_MAX_CLIENT > 1)
    u16 cfg_interval = BASE_INTERVAL_VALUE;
    u16 max_interval = little_endian_read_16(packet, 2);
    while (max_interval >= (cfg_interval + BASE_INTERVAL_VALUE)) {
        cfg_interval += BASE_INTERVAL_VALUE;
    }
    extern void little_endian_store_16(uint8_t *buffer, uint16_t pos, uint16_t value);
    little_endian_store_16(packet, 0, cfg_interval);
    little_endian_store_16(packet, 2, cfg_interval);
    r_printf("conn_handle:%04x,confirm_interval:%d", handle, cfg_interval);
    log_info("conn_handle:%04x,confirm_interval:%d\n", handle, cfg_interval);
#endif

    //change param
    /* little_endian_store_16(packet, 4,0);//disable latency */
    /* little_endian_store_16(packet, 6,400);//change timeout */
    return 0;
    /* return 1; */
}

/*************************************************************************************************/
/*!
 *  \brief      协议栈回调搜索descriptor结果
 *
 *  \param      [in]    搜索结果
 *
 *  \return
 *
 *  \note      搜索charactc包含descriptor
 */
/*************************************************************************************************/
void user_client_report_descriptor_result(charact_descriptor_t *result_descriptor)
{
    log_info("report_descriptor,handle= %04x ,uuid16: %04x\n", result_descriptor->handle, result_descriptor->uuid16);
    if (result_descriptor->uuid16 == 0) {
        log_info("uuid128:");
        log_info_hexdump(result_descriptor->uuid128, 16);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      协议栈回调搜索 service & charactc的结果
 *
 *  \param      [in]    搜索结果
 *
 *  \return
 *
 *  \note      每搜索到一个server 或 charactc uuid 都会调用,直到搜索结束
 */
/*************************************************************************************************/
//协议栈内部调用
void user_client_report_search_result(search_result_t *result_info)
{
    if (result_info == (void *) - 1) {
        log_info("client_report_search_result finish!!!\n");
        do_operate_search_handle();
        client_event_report(CLI_EVENT_SEARCH_PROFILE_COMPLETE, (u8 *)&client_con_handle[cur_dev_cid], 2);

        //搜索完profile,尝试开新设备scan
        ble_enable_new_dev_scan();
        return;
    }

    log_info("\n*** services, uuid16:%04x,index=%d ***\n", result_info->services.uuid16, result_info->service_index);
    log_info("{charactc, uuid16:%04x,index=%d,handle:%04x~%04x,value_handle=%04x}\n",
             result_info->characteristic.uuid16, result_info->characteristic_index,
             result_info->characteristic.start_handle, result_info->characteristic.end_handle,
             result_info->characteristic.value_handle
            );

    if (!result_info->services.uuid16) {
        log_info("######services_uuid128:");
        log_info_hexdump(result_info->services.uuid128, 16);
    }

    if (!result_info->characteristic.uuid16) {
        log_info("######charact_uuid128:");
        log_info_hexdump(result_info->characteristic.uuid128, 16);
    }

    check_target_uuid_match(result_info);
}

#if SHOW_RX_DATA_RATE

static u32 client_timer_handle = 0;
static u32 test_data_count;

static void client_timer_handler(void *priv)
{
    if (!client_con_handle[cur_dev_cid]) {
        test_data_count = 0;
        return;
    }
    log_info("peer_rssi = %d\n", ble_vendor_get_peer_rssi(client_con_handle[cur_dev_cid]));

    if (test_data_count) {
        /* log_info("\n-ble_data_rate: %d bps-\n", test_data_count * 8); */
        log_info("\n%d bytes receive: %d.%02d KB/s \n", test_data_count, test_data_count / 1000, test_data_count % 1000);
        test_data_count = 0;
    }
}

static void client_timer_start(void)
{
    client_timer_handle  = sys_timer_add(NULL, client_timer_handler, 1000);
}

#endif /* SHOW_RX_DATA_RATE */

static const target_uuid_t *get_match_handle_target(u16 handle)
{
    for (int i = 0; i < opt_handle_used_cnt; i++) {
        if (opt_handle_table[i].value_handle == handle) {
            return opt_handle_table[i].search_uuid;
        }
    }
    return NULL;
}

//协议栈内部调用
void user_client_report_data_callback(att_data_report_t *report_data)
{
    /* log_info("\n-report_data:type %02x,handle %04x,offset %d,len %d:",report_data->packet_type, */
    /* 		report_data->value_handle,report_data->value_offset,report_data->blob_length); */
    /* log_info_hexdump(report_data->blob,report_data->blob_length); */

    if ((s8) - 1 == mul_get_dev_index(report_data->conn_handle, MULTI_ROLE_CLIENT)) {
        log_info("unknown_handle:%04x\n", report_data->conn_handle);
        log_info_hexdump(report_data->blob, report_data->blob_length);
        return;
    }

#if SHOW_RX_DATA_RATE
    test_data_count += report_data->blob_length;
#endif /* SHOW_RX_DATA_RATE */

    const target_uuid_t *search_uuid = get_match_handle_target(report_data->value_handle);

    if (client_config->report_data_callback) {
        client_config->report_data_callback(report_data, search_uuid);
        return;
    }

    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION://notify
//        log_info("\n-notify_rx(%d):",report_data->blob_length);
    case GATT_EVENT_INDICATION://indicate
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT://read
    case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT://read long
        break;
    default:
        break;
    }
}

//	PRIMARY_SERVICE,AE30
/* static const u16 test_services_uuid16 = 0xae30; */

//	PRIMARY_SERVICE, 0000F530-1212-EFDE-1523-785FEABCD123
/* static const u8  test_services_uuid128[16] = {0x00,0x00,0xF5,0x30 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23};  */

static void client_search_profile_start(u16 conn_handle)
{
    s8 tmp_cid = mul_get_dev_index(conn_handle, MULTI_ROLE_CLIENT);

    if (tmp_cid == MULTI_INVAIL_INDEX) {
        log_info("error search_cid!!! :%04x\n", conn_handle);
        return;
    }

    opt_handle_used_cnt = 0;
    memset(&target_handle[tmp_cid], 0, sizeof(target_hdl_t));
    user_client_init(client_con_handle[tmp_cid], search_ram_buffer, SEARCH_PROFILE_BUFSIZE);

    if (encryption_process == 2 && pair_reconnect_search_profile == 0) {
        log_info("reconnect not search_profile:%04x\n", client_con_handle[tmp_cid]);
        user_client_set_search_complete();
        return;
    }

    if (client_config->search_uuid_cnt) {
        log_info("start search_profile_all:%04x\n", client_con_handle[tmp_cid]);
        ble_op_search_profile_all();
    } else {
        log_info("skip search_profile:%04x\n", client_con_handle[tmp_cid]);
        user_client_set_search_complete();
    }
}

//------------------------------------------------------------
static bool resolve_adv_report(u8 *adv_address, u8 data_length, u8 *data, s8 rssi)
{
    u8 i, lenght, ad_type;
    u8 *adv_data_pt;
    u8 find_remoter = 0;
    /* u8 tmp_addr[6]; */
    u32 tmp32;

    if (check_device_is_match(CLI_CREAT_BY_ADDRESS, adv_address, 6)) {
        find_remoter = 1;
    }

    adv_data_pt = data;
    for (i = 0; i < data_length;) {
        if (*adv_data_pt == 0) {
            /* log_info("analyze end\n"); */
            break;
        }

        lenght = *adv_data_pt++;
        ad_type = *adv_data_pt++;
        i += (lenght + 1);

        switch (ad_type) {
        case HCI_EIR_DATATYPE_FLAGS:
            /* log_info("flags:%02x\n",adv_data_pt[0]); */
            break;

        case HCI_EIR_DATATYPE_MORE_16BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_MORE_32BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_32BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_MORE_128BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_128BIT_SERVICE_UUIDS:
            /* log_info("service uuid:"); */
            /* log_info_hexdump(adv_data_pt, lenght - 1); */
            break;

        case HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME:
        case HCI_EIR_DATATYPE_SHORTENED_LOCAL_NAME:
            tmp32 = adv_data_pt[lenght - 1];
            adv_data_pt[lenght - 1] = 0;;
            g_printf("ble remoter_name: %s,rssi:%d\n", adv_data_pt, rssi);
            /* log_info_hexdump(adv_address, 6); */
            adv_data_pt[lenght - 1] = tmp32;
            //-------
#if SUPPORT_TEST_BOX_BLE_MASTER_TEST_EN
#define TEST_BOX_BLE_NAME		"JLBT_TESTBOX"
#define TEST_BOX_BLE_NAME_LEN	0xc
            if (0 == memcmp(adv_data_pt, TEST_BOX_BLE_NAME, TEST_BOX_BLE_NAME_LEN)) {
                find_remoter = 1;
                break;
            }
#endif
            /* log_info("target name:%s", client_config->compare_data); */
            if (check_device_is_match(CLI_CREAT_BY_NAME, adv_data_pt, lenght - 1)) {
                find_remoter = 1;
                log_info("catch name ok\n");
            }
            break;

        case HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA:
            if (check_device_is_match(CLI_CREAT_BY_TAG, adv_data_pt, lenght - 1)) {
                log_info("get_tag_string!\n");
                find_remoter = 1;
            }
            break;

        case HCI_EIR_DATATYPE_APPEARANCE_DATA:
            /* log_info("get_class_type:%04x\n",little_endian_read_16(adv_data_pt,0)); */
            break;

        default:
            /* log_info("unknow ad_type:"); */
            break;
        }

        if (find_remoter) {
            log_info_hexdump(adv_data_pt, lenght - 1);
        }
        adv_data_pt += (lenght - 1);
    }

    return find_remoter;
}

static void client_report_adv_data(adv_report_t *report_pt, u16 len)
{
    bool find_tag = 0;
    int i;
    /* log_info("event_type,addr_type: %x,%x; ",report_pt->event_type,report_pt->address_type); */
    /* log_info_hexdump(report_pt->address,6); */

    /* log_info("adv_data_display:"); */
    /* log_info_hexdump(report_pt->data,report_pt->length); */

//    log_info("rssi:%d\n",report_pt->rssi);

    for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
        if (conn_pair_info_table[i].pair_flag) {
            /* log_info("check paired dev\n"); */
            /* put_buf(report_pt->address, 6); */
            /* put_buf(&conn_pair_info_table[i].peer_address_info[1], 6); */
            if (report_pt->address_type == conn_pair_info_table[i].peer_address_info[0]
                && 0 == memcmp(report_pt->address, &conn_pair_info_table[i].peer_address_info[1], 6)) {
                log_info("find paired dev to connect\n");
                //set tag
                find_tag = 2;
                pair_bond_enable = 0x55;
                match_dev_id = conn_pair_info_table[i].match_dev_id;
                memcpy(&reconnect_info, &conn_pair_info_table[i].conn_handle, sizeof(struct conn_info_t));
            }
        }
    }

    if (!find_tag) {
        find_tag = resolve_adv_report(report_pt->address, report_pt->length, report_pt->data, report_pt->rssi);
    }

    if (find_tag) {
        if (force_seach_onoff && force_seach_rssi > report_pt->rssi) {
            log_info("match but rssi fail!!!:%d,%d\n", force_seach_rssi, report_pt->rssi);
            return;
        }

        log_info("rssi:%d\n", report_pt->rssi);
        log_info("\n*********create_connection***********\n");
        log_info("***remote type %d,addr:", report_pt->address_type);
        log_info_hexdump(report_pt->address, 6);
        bt_ble_scan_enable(0, 0);
        client_create_connect_api(report_pt->address, report_pt->address_type, 0);
        log_info("*create_finish\n");
    }
}

#if EXT_ADV_MODE_EN

#define GET_STRUCT_MEMBER_OFFSET(type, member) \
    (u32)&(((struct type*)0)->member)

struct __ext_adv_report_event {
    u8  Subevent_Code;
    u8  Num_Reports;
    u16 Event_Type;
    u8  Address_Type;
    u8  Address[6];
    u8  Primary_PHY;
    u8  Secondary_PHY;
    u8  Advertising_SID;
    u8  Tx_Power;
    u8  RSSI;
    u16 Periodic_Advertising_Interval;
    u8  Direct_Address_Type;
    u8  Direct_Address[6];
    u8  Data_Length;
    u8  Data[0];
} _GNU_PACKED_;

static void client_report_ext_adv_data(u8 *report_pt, u16 len)
{
    bool find_remoter;
    u8 address_type = report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, Address_Type)];
    u8 address[6];

    /* log_info("--func=%s", __FUNCTION__); */
    /* log_info_hexdump(report_pt, len); */

    memcpy(address, &report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, Address)], 6);

    find_remoter = resolve_adv_report(\
                                      address, \
                                      report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, Data_Length)], \
                                      &report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, Data)], 0 \
                                     );

    if (find_remoter) {
        if (force_seach_onoff && force_seach_rssi > report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, RSSI)]) {
            log_info("match but rssi fail!!!:%d,%d\n", force_seach_rssi, report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, RSSI)]);
            return;
        }

        log_info("\n*********ext create_connection***********\n");
        log_info("***remote type %d, addr:", address_type);
        log_info_hexdump(address, 6);
        bt_ble_scan_enable(0, 0);
        client_create_connect_api(address, address_type, 0);
        log_info("*create_finish\n");
    }
}

struct __ext_init {
    u8 Initiating_Filter_Policy;
    u8 Own_Address_Type;
    u8 Peer_Address_Type;
    u8 Peer_Address[6];
    u8 Initiating_PHYs;
    u16 Scan_Interval;
    u16 Scan_Window;
    u16 Conn_Interval_Min;
    u16 Conn_Interval_Max;
    u16 Conn_Latency;
    u16 Supervision_Timeout;
    u16 Minimum_CE_Length;
    u16 Maximum_CE_Length;
} _GNU_PACKED_;

static void client_ext_create_connection(u8 *conn_addr, u8 addr_type)
{
    struct __ext_init *create_conn_par = (struct __ext_init *)scan_buffer;

    if (get_ble_work_state(cur_dev_cid) == BLE_ST_CREATE_CONN) {
        log_info("already ext_create conn:%d!!!\n", cur_dev_cid);
        return;
    }

    memset(create_conn_par, 0, sizeof(*create_conn_par));
    create_conn_par->Conn_Interval_Min = SET_CONN_INTERVAL;
    create_conn_par->Conn_Interval_Max = SET_CONN_INTERVAL;
    create_conn_par->Conn_Latency = SET_CONN_LATENCY;
    create_conn_par->Supervision_Timeout = SET_CONN_TIMEOUT;
    create_conn_par->Peer_Address_Type = addr_type;
    create_conn_par->Initiating_PHYs = INIT_SET_1M_PHY;
    memcpy(create_conn_par->Peer_Address, conn_addr, 6);

    set_ble_work_state(cur_dev_cid, BLE_ST_CREATE_CONN);

    log_info_hexdump(create_conn_par, sizeof(*create_conn_par));

    ble_op_ext_create_conn(create_conn_par, sizeof(*create_conn_par));
}

#endif /* EXT_ADV_MODE_EN */

static void client_create_connection(u8 *conn_addr, u8 addr_type)
{
    struct create_conn_param_t *create_conn_par = (struct create_conn_param_t *)scan_buffer;
    if (get_ble_work_state(cur_dev_cid) == BLE_ST_CREATE_CONN) {
        log_info("already create conn:%d!!!\n", cur_dev_cid);
        return;
    }

    if (pair_bond_enable == 0x55) {
        log_info("pair reconnect to creat\n");
        create_conn_par->conn_interval = reconnect_info.conn_interval;
        create_conn_par->conn_latency = reconnect_info.conn_latency;
        create_conn_par->supervision_timeout = reconnect_info.conn_timeout;
    } else {
        create_conn_par->conn_interval = SET_CONN_INTERVAL;
        create_conn_par->conn_latency = SET_CONN_LATENCY;
        create_conn_par->supervision_timeout = SET_CONN_TIMEOUT;
    }

    memcpy(create_conn_par->peer_address, conn_addr, 6);
    create_conn_par->peer_address_type = addr_type;

    set_ble_work_state(cur_dev_cid, BLE_ST_CREATE_CONN);
    ble_op_create_connection(create_conn_par);
}

static void bt_ble_create_connection(u8 *conn_addr, u8 addr_type)
{
#if EXT_ADV_MODE_EN
    client_ext_create_connection(conn_addr, addr_type);
#else
    client_create_connection(conn_addr, addr_type);
#endif /* EXT_ADV_MODE_EN */
}

static void client_create_connection_cannel(void)
{
    if (get_ble_work_state(cur_dev_cid) == BLE_ST_CREATE_CONN) {
        set_ble_work_state(cur_dev_cid, BLE_ST_SEND_CREATE_CONN_CANNEL);
        ble_op_create_connection_cancel();
    }
}

static int client_disconnect(void *priv)
{
    u8 cid = (u8) priv;

    if (cid >= SUPPORT_MAX_CLIENT) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (client_con_handle[cid]) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state(cid)) {
            log_info(">>>ble(%d) send disconnect\n", cid);
            set_ble_work_state(cid, BLE_ST_SEND_DISCONN);
            ble_op_disconnect(client_con_handle[cid]);
        } else {
            log_info(">>>ble(%d) wait disconnect...\n", cid);
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

void ble_multi_client_disconnect(void)
{
    int count;

    for (u32 i = 0; i < SUPPORT_MAX_CLIENT; i++) {
        count = 150;
        if (client_con_handle[i]) {
            client_disconnect((void *)i);
            while (count-- && client_con_handle[i]) {
                os_time_dly(1);
                putchar('w');
            }
        }
    }
}

static void connection_update_complete_success(u16 conn_handle, u8 *packet, u8 connect_start)
{
    int conn_interval, conn_latency, conn_timeout;

    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_handle = %04x\n", conn_handle);
    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);

    if (connect_start) {
        conn_pair_info.conn_handle = conn_handle;
        conn_pair_info.conn_interval = conn_interval;
        conn_pair_info.conn_latency = conn_latency;
        conn_pair_info.conn_timeout = conn_timeout;
    } else {
        if (conn_pair_info.conn_handle ==  conn_handle) {
            log_info("cur_conn_pair_info\n");
            conn_pair_info.conn_interval = conn_interval;
            conn_pair_info.conn_latency =  conn_latency;
            conn_pair_info.conn_timeout =  conn_timeout;
            if (conn_pair_info.pair_flag) {
                conn_pair_vm_do(&conn_pair_info, 1);
            }
        } else {
            struct pair_info_t  tmp_conn_pair_info;
            int i;
            for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
                if (conn_pair_info_table[i].pair_flag && conn_pair_info_table[i].head_tag == BLE_VM_HEAD_TAG
                    && conn_pair_info_table[i].tail_tag == BLE_VM_TAIL_TAG) {
                    //vail pair
                    log_info("%d:handle=%04x\n", i, conn_pair_info_table[i]);
                    if (conn_pair_info_table[i].conn_handle == conn_handle) {
                        memcpy(&tmp_conn_pair_info, &conn_pair_info_table[i], sizeof(struct pair_info_t));
                        tmp_conn_pair_info.conn_interval = conn_interval;
                        tmp_conn_pair_info.conn_latency =  conn_latency;
                        tmp_conn_pair_info.conn_timeout =  conn_timeout;
                        log_info("table_conn_pair_info:%d\n", i);
                        conn_pair_vm_do(&tmp_conn_pair_info, 1);
                        break;
                    }
                }
            }
        }
    }
}

static void can_send_now_wakeup(void)
{
    /* putchar('W'); */

    if (!mul_dev_have_connected(MULTI_ROLE_CLIENT)) {
        return;
    }
}

static const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

static void set_connection_data_length(u16 tx_octets, u16 tx_time)
{
    if (client_con_handle[cur_dev_cid]) {
        ble_op_set_data_length(client_con_handle[cur_dev_cid], tx_octets, tx_time);
    }
}

static void set_connection_data_phy(u8 tx_phy, u8 rx_phy)
{
    if (0 == client_con_handle[cur_dev_cid]) {
        return;
    }

    u8 all_phys = 0;
    u16 phy_options = 0;

    ble_op_set_ext_phy(client_con_handle[cur_dev_cid], all_phys, tx_phy, rx_phy, phy_options);
}

static void client_profile_start(u16 conn_handle)
{
    set_ble_work_state(cur_dev_cid, BLE_ST_CONNECT);

    if (0 == client_config->security_en) {
        client_search_profile_start(conn_handle);
    }
}

//尝试开新设备scan
int ble_enable_new_dev_scan(void)
{
    //log_info("%s\n", __FUNCTION__);

    /*先检查当前操作的状态机*/
    if (client_con_handle[cur_dev_cid]) {
        /*已连接*/
        if (get_ble_work_state(cur_dev_cid) != BLE_ST_SEARCH_COMPLETE) {
            /*要等当前链路走完才能 配设备开scan*/
            log_info("%d,dev_connecting\n", cur_dev_cid);
            return -1;
        }
    } else {
        /*未连接*/
        if (BLE_ST_SCAN == get_ble_work_state(cur_dev_cid)
            || BLE_ST_CREATE_CONN == get_ble_work_state(cur_dev_cid)) {
            log_info("dev_doing\n");
            return -2;
        }
    }

    s8 tmp_cid = mul_get_idle_dev_index(MULTI_ROLE_CLIENT);
    if (tmp_cid == MULTI_INVAIL_INDEX) {
        log_info("no idle dev to do!!!\n");
        return -3;
    }

    log_info("new_dev_scan\n");
    cur_dev_cid = tmp_cid;
    set_ble_work_state(cur_dev_cid, BLE_ST_IDLE);
    bt_ble_scan_enable(0, 1);
    return 0;
}

void client_deal_passkey_input(u32 *key, u16 conn_handle)
{
    //0~999999
    *key = 123456;
    log_info("con_handle= %04x,new_key: %06u.\n", conn_handle, *key);
}

void client_cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;
    //log_info("%s\n", __FUNCTION__);

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("Just Works Confirmed.\n");
            encryption_process = 1;
            break;

        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u.\n", tmp32);
            encryption_process = 1;
            break;

        case SM_EVENT_PASSKEY_INPUT_NUMBER:
            /*IO_CAPABILITY_KEYBOARD_ONLY 方式*/
            client_deal_passkey_input(&tmp32, event->con_handle);
            log_info("%04x->Passkey input: %06u.\n", event->con_handle, tmp32);
            sm_passkey_input(event->con_handle, tmp32); /*update passkey*/
            break;

        case SM_EVENT_PAIR_PROCESS:
            log_info("======PAIR_PROCESS: %02x\n", event->data[0]);
            put_buf(event->data, 4);
            break;
        }
        break;
    }
}

/* LISTING_START(packetHandler): Packet Handler */
void client_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;
    u8 status;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        /* case DAEMON_EVENT_HCI_PACKET_SENT: */
        /* break; */
        case ATT_EVENT_CAN_SEND_NOW:
            can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {

            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE: {
                if (1 == hci_subevent_le_connection_complete_get_role(packet)) {
                    //connect is slave
                    break;
                }

                status = hci_subevent_le_enhanced_connection_complete_get_status(packet);
                if (status) {
                    log_info("LE_MASTER CREATE CONNECTION FAIL!!! %0x\n", status);
                    set_ble_work_state(cur_dev_cid, BLE_ST_DISCONN);
                    break;
                }
                u16 tmp_handle1 = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE : 0x%0x\n", tmp_handle1);
                s8 tmp_dev_cid1 = mul_get_idle_dev_index(MULTI_ROLE_CLIENT);

                if (tmp_dev_cid1 == MULTI_INVAIL_INDEX) {
                    log_info("dev_cid full!!!\n");
                    ble_op_disconnect(tmp_handle1);
                    break;
                }

                encryption_process = 0;
                cur_dev_cid = tmp_dev_cid1;
                client_con_handle[cur_dev_cid] = tmp_handle1;
                ble_op_multi_att_send_conn_handle(client_con_handle[cur_dev_cid], cur_dev_cid, MULTI_ROLE_CLIENT);

                log_info("conn_interval = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_interval(packet));
                log_info("conn_latency = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_latency(packet));
                log_info("conn_timeout = %d\n", hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(packet));
                client_profile_start(client_con_handle[cur_dev_cid]);
            }
            break;

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                if (1 == hci_subevent_le_connection_complete_get_role(packet)) {
                    //connect is slave
                    break;
                }

                if (packet[3]) {
                    log_info("LE_MASTER CREATE CONNECTION FAIL!!! %0x\n", packet[3]);
                    set_ble_work_state(cur_dev_cid, BLE_ST_DISCONN);
                    //auto to do
                    ble_enable_new_dev_scan();
                    //bt_ble_scan_enable(0, 1);
                    break;
                }

                u16 tmp_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", tmp_handle);
                s8 tmp_dev_cid = mul_get_idle_dev_index(MULTI_ROLE_CLIENT);

                if (tmp_dev_cid == MULTI_INVAIL_INDEX) {
                    log_info("dev_cid full!!!\n");
                    ble_op_disconnect(tmp_handle);
                    break;
                }

                encryption_process = 0;
                cur_dev_cid = tmp_dev_cid;
                client_con_handle[cur_dev_cid] = tmp_handle;
                ble_op_multi_att_send_conn_handle(client_con_handle[cur_dev_cid], cur_dev_cid, MULTI_ROLE_CLIENT);

                connection_update_complete_success(tmp_handle, packet + 8, 1);
                client_profile_start(client_con_handle[cur_dev_cid]);
                client_event_report(CLI_EVENT_CONNECTED, packet, size);

                conn_pair_info.match_dev_id = match_dev_id;
                memcpy(&conn_pair_info.peer_address_info, &packet[7], 7);

                if (pair_bond_enable && 0 == client_config->security_en) {
                    conn_pair_info.pair_flag = 1;
                    log_info("no_pair,bond remoter\n");
                    conn_pair_vm_do(&conn_pair_info, 1);
                    pair_bond_enable = 0;
                } else {
                    conn_pair_info.pair_flag = 0;
                }
            }
            break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE: {
                u16 tmp_handle = little_endian_read_16(packet, 4);
                if (mul_get_dev_index(tmp_handle, MULTI_ROLE_CLIENT) >= 0) {
                    connection_update_complete_success(tmp_handle, packet, 0);
                }
            }
            break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE");
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                log_info("Tx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                log_info("Rx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                break;

#if EXT_ADV_MODE_EN
            case HCI_SUBEVENT_LE_EXTENDED_ADVERTISING_REPORT:
                /* log_info("APP HCI_SUBEVENT_LE_EXTENDED_ADVERTISING_REPORT"); */
                /* log_info_hexdump(packet, size); */
                if (BLE_ST_SCAN == get_ble_work_state(cur_dev_cid)) {
                    client_report_ext_adv_data(&packet[2], packet[1]);
                } else {
                    log_info("drop ext_adv_report!!!\n");
                }
                break;
#endif /* EXT_ADV_MODE_EN */
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);

            u16 tmp_handle = little_endian_read_16(packet, 3);
            s8 tmp_index = mul_get_dev_index(tmp_handle, MULTI_ROLE_CLIENT);
            if (tmp_index == MULTI_INVAIL_INDEX) {
                log_info("unknown_handle:%04x\n", tmp_handle);
                break;
            }

            client_con_handle[tmp_index] = 0;
            set_ble_work_state(tmp_index, BLE_ST_DISCONN);
            client_event_report(CLI_EVENT_DISCONNECT, packet, size);
            ble_op_multi_att_send_conn_handle(0, tmp_index, MULTI_ROLE_CLIENT);

            if (0 == client_con_handle[cur_dev_cid]) {
                //判断当前设备是否正在scan 或者 creat
                if (BLE_ST_SCAN == get_ble_work_state(cur_dev_cid) || BLE_ST_CREATE_CONN == get_ble_work_state(cur_dev_cid)) {
                    //由于连上一台和未连接的时候scan参数可能不一致，所以有链路断开时，则先关闭已有scan再打开;
                    log_info("%d,doing\n", cur_dev_cid);
                    bt_ble_scan_enable(0, 0);
                }
            }

            //配设备开scan
            cur_dev_cid = tmp_index;

            //auto to do
            if (0) { //(conn_pair_info.pair_flag && SUPPORT_MAX_CLIENT < 2) {
                client_create_connect_api(0, 0, 1);
            } else {
                ble_enable_new_dev_scan();
                //bt_ble_scan_enable(0, 1);
            }
        }
        break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE: {
            u16 tmp_handle = little_endian_read_16(packet, 2);
            s8 tmp_cid = mul_get_dev_index(tmp_handle, MULTI_ROLE_CLIENT);
            if (tmp_cid != MULTI_INVAIL_INDEX) {
                mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
                log_info("ATT MTU = %u\n", mtu);
                ble_op_multi_att_set_send_mtu(tmp_handle, mtu);
                /* set_connection_data_length(251, 2120); */
            }
        }
        break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE: {
            u16 tmp_handle = little_endian_read_16(packet, 2);
            s8  tmp_cid = mul_get_dev_index(tmp_handle, MULTI_ROLE_CLIENT);
            if (tmp_cid != MULTI_INVAIL_INDEX) {
                tmp = little_endian_read_16(packet, 4);
                log_info("-update_rsp:%04x, %02x\n", tmp_handle, tmp);
            }
        }
        break;

        case GAP_EVENT_ADVERTISING_REPORT:
            /* putchar('@'); */
            if (BLE_ST_SCAN == get_ble_work_state(cur_dev_cid)) {
                client_report_adv_data((void *)&packet[2], packet[1]);
            } else {
                log_info("drop adv_report!!!%02x\n", get_ble_work_state(cur_dev_cid));
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE: {
            u16 tmp_handle = little_endian_read_16(packet, 3);
            s8 tmp_cid = mul_get_dev_index(tmp_handle, MULTI_ROLE_CLIENT);
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d,handle=%04x\n", packet[2], tmp_handle);

            if (packet[2]) {
                log_info("fail,break\n");
                break;
            }

            if (0 == encryption_process) {
                encryption_process = 2;
                log_info("reconnect remoter\n");
            } else {
                //first pair
                log_info("first pair\n");
            }

            if (pair_bond_enable) {
                if (conn_pair_info.conn_handle == tmp_handle) {
                    log_info("pair,bond remoter\n");
                    conn_pair_info.pair_flag = 1;
                    conn_pair_vm_do(&conn_pair_info, 1);
                    pair_bond_enable = 0;
                } else {
                    log_info("not match???\n");
                }
            }

            if (tmp_cid != MULTI_INVAIL_INDEX) {
                if (client_config->security_en) {
                    client_search_profile_start(tmp_handle);
                }
            }
        }
        break;
        }
        break;
    }
}

static int get_buffer_vaild_len(void *priv)
{
    u32 vaild_len = 0;
    ble_op_multi_att_get_remain(client_con_handle[cur_dev_cid], &vaild_len);
    return vaild_len;
}

static int client_operation_send(u16 handle, u8 *data, u16 len, u8 att_op_type)
{
    int ret = APP_BLE_NO_ERROR;
    if (!client_con_handle[cur_dev_cid]) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (!handle) {
        log_info("handle is null\n");
        return APP_BLE_OPERATION_ERROR;
    }

    if (get_buffer_vaild_len(0) < len) {
        log_info("opt_buff_full!!!\n");
        return APP_BLE_BUFF_FULL;
    }

    ret = ble_op_multi_att_send_data(client_con_handle[cur_dev_cid], handle, data, len, att_op_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }
    return ret;
}

static int client_operation_send_ext(u16 conn_handle, u16 handle, u8 *data, u16 len, u8 att_op_type)
{
    int ret = APP_BLE_NO_ERROR;
    if (!conn_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    ret = ble_op_multi_att_send_data(conn_handle, handle, data, len, att_op_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }
    return ret;
}
//-----------------------------------------------
static int client_write_send(void *priv, u8 *data, u16 len)
{
    return client_operation_send(target_handle[cur_dev_cid].write_handle, data, len, ATT_OP_WRITE);
}

static int client_write_without_respond_send(void *priv, u8 *data, u16 len)
{
    return client_operation_send(target_handle[cur_dev_cid].write_no_respond, data, len, ATT_OP_WRITE_WITHOUT_RESPOND);
}

static int client_read_value_send(void *priv)
{
    u16 tmp_flag = 0x55A1;
    return client_operation_send(target_handle[cur_dev_cid].read_handle, (u8 *)&tmp_flag, 2, ATT_OP_READ);
}

static int client_read_long_value_send(void *priv)
{
    u16 tmp_flag = 0x55A2;
    return client_operation_send(target_handle[cur_dev_cid].read_handle, (u8 *)&tmp_flag, 2, ATT_OP_READ_LONG);
}

#if EXT_ADV_MODE_EN

struct __ext_scan_param {
    u8 Own_Address_Type;
    u8 Scanning_Filter_Policy;
    u8 Scanning_PHYs;
    u8  Scan_Type;
    u16 Scan_Interval;
    u16 Scan_Window;
} _GNU_PACKED_;

struct __ext_scan_enable {
    u8  Enable;
    u8  Filter_Duplicates;
    u16 Duration;
    u16 Period;
} _GNU_PACKED_;

static const struct __ext_scan_param ext_scan_param = {
    .Own_Address_Type = 0,
    .Scanning_Filter_Policy = 0,
    .Scanning_PHYs = SCAN_SET_1M_PHY,
    .Scan_Type = SET_SCAN_TYPE,
    .Scan_Interval = SET_SCAN_INTERVAL,
    .Scan_Window = SET_SCAN_WINDOW,
};

static const struct __ext_scan_enable ext_scan_enable = {
    .Enable = 1,
    .Filter_Duplicates = 0,
    .Duration = 0,
    .Period = 0,
};

static const struct __ext_scan_enable ext_scan_disable = {
    .Enable = 0,
    .Filter_Duplicates = 0,
    .Duration = 0,
    .Period = 0,
};

#endif /* EXT_ADV_MODE_EN */

//扫描数设置
static void scanning_setup_init(void)
{
    ble_op_set_scan_param(SET_SCAN_TYPE, SET_SCAN_INTERVAL, SET_SCAN_WINDOW);
}

static int bt_ble_scan_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!scan_ctrl_en && en) {
        return 	APP_BLE_OPERATION_ERROR;
    }

    if (en) {
        next_state = BLE_ST_SCAN;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  get_ble_work_state(cur_dev_cid);
    switch (cur_state) {
    case BLE_ST_SCAN:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
    case BLE_ST_CONNECT_FAIL:
    case BLE_ST_SEND_CREATE_CONN_CANNEL:
        break;
    default:
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }

    log_info("scan_en:%d\n", en);
    set_ble_work_state(cur_dev_cid, next_state);

#if EXT_ADV_MODE_EN
    if (en) {
        ble_op_set_ext_scan_param(&ext_scan_param, sizeof(ext_scan_param));
        ble_op_ext_scan_enable(&ext_scan_enable, sizeof(ext_scan_enable));
    } else {
        ble_op_ext_scan_enable(&ext_scan_disable, sizeof(ext_scan_disable));
    }
#else
    if (en) {
#if 0
        if (mul_dev_have_connected(SUPPORT_MAX_CLIENT)) {
            //已连上一台机，则scan参数调整
            ble_op_set_scan_param(SET_SCAN_TYPE, ADV_SCAN_MS(1), ADV_SCAN_MS(1));
        } else
#endif
        {
            scanning_setup_init();
        }
    }
    ble_op_scan_enable2(en, 0);
#endif /* EXT_ADV_MODE_EN */

    return APP_BLE_NO_ERROR;
}

static int client_regiest_wakeup_send(void *priv, void *cbk)
{
    /* att_regist_wakeup_send(cbk); */
    return APP_BLE_NO_ERROR;
}

static int client_regiest_recieve_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    return APP_BLE_NO_ERROR;
}

static int client_regiest_state_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_ble_state_callback = cbk;
    return APP_BLE_NO_ERROR;
}

//该接口重新配置搜索的配置项
static int client_init_config(void *priv, void *cfg)
{
    log_info("client_init_config\n");
    client_config = cfg;//reset config
    return APP_BLE_NO_ERROR;
}

//可配置进入强制搜索方式连接，更加信号强度过滤设备
static int client_force_search(u8 onoff, s8 rssi)
{
    force_seach_rssi = rssi;

    if (force_seach_onoff != onoff) {
        force_seach_onoff = onoff;
        //强制搜索前后，关创建监听
        if (get_ble_work_state(cur_dev_cid) == BLE_ST_CREATE_CONN) {
            client_create_connection_cannel();
        }
    }
    return 0;
}

static int client_create_connect_api(u8 *addr, u8 addr_type, u8 mode)
{
    u8 cur_state =  get_ble_work_state(cur_dev_cid);

    switch (cur_state) {
    case BLE_ST_SCAN:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
    case BLE_ST_CONNECT_FAIL:
    case BLE_ST_SEND_CREATE_CONN_CANNEL:
        break;
    default:
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == BLE_ST_SCAN) {
        log_info("stop scan\n");
        bt_ble_scan_enable(0, 0);
    }

    //pair mode
    if (mode == 1) {
        if (conn_pair_info.pair_flag) {
            if (conn_pair_info.pair_flag) {
                //有配对,跳过搜索,直接创建init_creat
                log_info("pair to creat!\n");
                log_info_hexdump(conn_pair_info.peer_address_info, 7);
                bt_ble_create_connection(&conn_pair_info.peer_address_info[1], conn_pair_info.peer_address_info[0]);
                return 0;
            }

        } else {
            log_info("no pair to creat!\n");
            return APP_BLE_OPERATION_ERROR;
        }
    } else {
        log_info("addr to creat!\n");
        log_info_hexdump(addr, 7);
        bt_ble_create_connection(addr, addr_type);
    }
    return 0;
}

static int client_create_cannel_api(void)
{
    if (get_ble_work_state(cur_dev_cid) == BLE_ST_CREATE_CONN) {
        client_create_connection_cannel();
        return 0;
    }
    return 1;
}

static int get_ble_work_state_api(void)
{
    return get_ble_work_state(cur_dev_cid);
}

static const struct ble_client_operation_t client_operation = {
    .scan_enable = bt_ble_scan_enable,
    .disconnect = client_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .write_data = (void *)client_write_without_respond_send,
    .read_do = (void *)client_read_value_send,
    .regist_wakeup_send = client_regiest_wakeup_send,
    .regist_recieve_cbk = client_regiest_recieve_cbk,
    .regist_state_cbk = client_regiest_state_cbk,
    .init_config = client_init_config,
    .opt_comm_send = client_operation_send,
    .set_force_search = client_force_search,
    .create_connect = client_create_connect_api,
    .create_connect_cannel = client_create_cannel_api,
    .get_work_state = get_ble_work_state_api,
    .opt_comm_send_ext = client_operation_send_ext,
};

struct ble_client_operation_t *ble_get_client_operation_table(void)
{
    return (struct ble_client_operation_t *)&client_operation;
}

static void device_bonding_init(void)
{
    int i;
    int cfg_bonding = 0;
    const client_match_cfg_t *cfg;
    for (i = 0; i < CLIENT_MATCH_CONN_MAX; i++) {
        cfg = client_config->match_dev_cfg[i];
        if (cfg == NULL) {
            continue;
        }
        if (cfg->bonding_flag) {
            cfg_bonding = 1;
        }
    }
    if (!cfg_bonding) {
        client_clear_bonding_info();
    }
}

//模块开关
void ble_client_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);

    if (en) {
        scan_ctrl_en = 1;
        bt_ble_scan_enable(0, 1);
    } else {
        scan_ctrl_en = 0;
        bt_ble_scan_enable(0, 0);
        ble_multi_client_disconnect();
    }
}

void client_profile_init(void)
{
    //setup GATT client
    gatt_client_init();
    gatt_client_register_packet_handler(client_cbk_packet_handler);
    memset(&conn_pair_info, 0, sizeof(struct pair_info_t));
    memset(conn_pair_info_table, 0, sizeof(conn_pair_info_table));
    conn_pair_info.head_tag = BLE_VM_HEAD_TAG;
    conn_pair_info.tail_tag = BLE_VM_TAIL_TAG;

    sm_set_master_pair_redo(sm_master_pair_redo);
}

void bt_multi_client_init(void)
{
    ASSERT(client_config != NULL, "%s\n", "client's config not init\n");

    cur_dev_cid = 0;
    memset(&ble_work_state, BLE_ST_INIT_OK, SUPPORT_MAX_CLIENT);
    memset(&client_con_handle, 0, SUPPORT_MAX_CLIENT << 1);

    set_ble_work_state(cur_dev_cid, BLE_ST_INIT_OK);
    conn_pair_vm_do(NULL, 0);
    /* conn_pair_vm_do(&conn_pair_info, 0); */
    device_bonding_init();
    ble_client_module_enable(1);

#if SHOW_RX_DATA_RATE
    client_timer_start();
#endif /* SHOW_RX_DATA_RATE */
}

void bt_multi_client_exit(void)
{
    ble_client_module_enable(0);
}

//主机请求更新连接参数
void client_send_conn_param_update(void)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[send_param_index];//for test
    log_info("client update param:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (client_con_handle[cur_dev_cid]) {
        ble_op_conn_param_update(client_con_handle[cur_dev_cid], param);
    }
}

/* static void test_send_conn_update(void) */
/* { */
/* client_send_conn_param_update(); */
/* send_param_index++; */
/* send_param_index &= 3; */
/* sys_timeout_add(0,test_send_conn_update,10000); */
/* } */

//切换已连接多机
int le_multi_client_switch_connect_cid(u8 cid)
{
    if (cid < SUPPORT_MAX_CLIENT && client_con_handle[cid]) {
        cur_dev_cid = cid;
        return 0;
    } else {
        log_info("cid not exist!!!\n");
        return -1;
    }
}

void le_multi_client_pair_reconnect_search_enable(u8 enable)
{
    pair_reconnect_search_profile = enable;
}

void le_multi_client_pair_redo(u8 enable)
{
    sm_master_pair_redo = enable;
}

//----------------------------------------------------------------------------------
#endif

