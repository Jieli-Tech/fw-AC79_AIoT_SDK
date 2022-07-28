/*********************************************************************************************
    *   Filename        : le_multi_common.c.c

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
#include "le_common.h"
#include "btstack/btstack_event.h"
#include "le_multi_common.h"
#include "btcontroller_config.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_MULTI)

#if LE_DEBUG_PRINT_EN
/* extern void printf_buf(u8 *buf, u32 len); */
//#define log_info            r_printf
#define log_info(x, ...)  r_printf("[LE_MUL_COMM]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define PASSKEY_ENTER_ENABLE      0 //输入passkey使能，可修改passkey
//----------------------------------------------------------------------------------------
#define MULTI_ATT_MTU_SIZE              (517) //ATT MTU的值

//ATT发送的包长,    note: 20 <= need >= MTU
#define MULTI_ATT_LOCAL_PAYLOAD_SIZE    (MULTI_ATT_MTU_SIZE)                   //
//ATT缓存的buffer大小,  note: need >= 20,可修改
#define MULTI_ATT_SEND_CBUF_SIZE        (512*3)                 //

//共配置的RAM
#define MULTI_ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + MULTI_ATT_LOCAL_PAYLOAD_SIZE + MULTI_ATT_SEND_CBUF_SIZE)                   //note:
static u8 multi_att_ram_buffer[MULTI_ATT_RAM_BUFSIZE] __attribute__((aligned(4)));

u16 server_con_handle[SUPPORT_MAX_SERVER];
u16 client_con_handle[SUPPORT_MAX_CLIENT];

extern const int config_le_sm_support_enable;
extern const int config_btctler_le_master_multilink;
extern const int config_le_gatt_server_num;
extern const int config_le_gatt_client_num;
extern const int config_le_hci_connection_num;

extern void reset_PK_cb_register_ext(void (*reset_pk)(u32 *, u16));
extern void sm_set_master_request_pair(int enable);
//----------------------------------------------------------------------------------------
s8 mul_get_dev_index(u16 handle, u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (MULTI_ROLE_SERVER == role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (handle == group_handle[i]) {
            return i;
        }
    }
    return MULTI_INVAIL_INDEX;
}

s8 mul_get_idle_dev_index(u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (MULTI_ROLE_SERVER == role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (0 == group_handle[i]) {
            return i;
        }
    }
    return MULTI_INVAIL_INDEX;
}

s8 mul_del_dev_index(u16 handle, u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (MULTI_ROLE_SERVER == role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (handle == group_handle[i]) {
            group_handle[i] = 0;
            return i;
        }
    }
    return MULTI_INVAIL_INDEX;
}

bool mul_dev_have_connected(u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (MULTI_ROLE_SERVER == role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (group_handle[i]) {
            return true;
        }
    }
    return false;
}

u16 mul_dev_get_conn_handle(u8 index, u8 role)
{
    u16 *group_handle;
    u8 count;

    if (MULTI_ROLE_SERVER == role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }

    if (index < count) {
        return group_handle[index];
    } else {
        return 0;
    }
}

u8 mul_dev_get_handle_role(u16 handle)
{
    u8 i;
    for (i = 0; i < SUPPORT_MAX_CLIENT; i++) {
        if (handle == client_con_handle[i]) {
            return MULTI_ROLE_CLIENT;
        }
    }
    return MULTI_ROLE_SERVER;
}

/* #define SM_EVENT_PAIR_PROCESS        0xDF */
void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
        case SM_EVENT_PASSKEY_INPUT_NUMBER:
        case SM_EVENT_PAIR_PROCESS:

#if SUPPORT_MAX_SERVER
            if (mul_dev_get_handle_role(event->con_handle) == MULTI_ROLE_SERVER) {
                trans_cbk_sm_packet_handler(packet_type, channel, packet, size);
            }
#endif

#if SUPPORT_MAX_CLIENT
            if (mul_dev_get_handle_role(event->con_handle) == MULTI_ROLE_CLIENT) {
                client_cbk_sm_packet_handler(packet_type, channel, packet, size);
            }

#endif

            break;
        default:
            break;
        }
    }
}

/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    u8 role = 0xff;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        case ATT_EVENT_CAN_SEND_NOW:
            if (0 == packet[1]) {
                role = MULTI_ROLE_SERVER;
            } else {
                role = MULTI_ROLE_CLIENT;
            }
            break;

        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            role = MULTI_ROLE_SERVER;
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            role = mul_dev_get_handle_role(little_endian_read_16(packet, 2));
            break;

        case GAP_EVENT_ADVERTISING_REPORT:
            role = MULTI_ROLE_CLIENT;
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
                if (hci_subevent_le_enhanced_connection_complete_get_role(packet)) {
                    role = MULTI_ROLE_SERVER;
                } else {
                    role = MULTI_ROLE_CLIENT;
                }
                break;

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                if (hci_subevent_le_connection_complete_get_role(packet)) {
                    role = MULTI_ROLE_SERVER;
                } else {
                    role = MULTI_ROLE_CLIENT;
                }
                break;

            //参数更新完成
            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                role = mul_dev_get_handle_role(little_endian_read_16(packet, 4));
                break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                role = mul_dev_get_handle_role(little_endian_read_16(packet, 3));
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                role = mul_dev_get_handle_role(little_endian_read_16(packet, 4));
                break;

            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            role = mul_dev_get_handle_role(little_endian_read_16(packet, 3));
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("HCI_EVENT_VENDOR_REMOTE_TEST: %d,%d\n", packet[1], packet[2]);
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            role = mul_dev_get_handle_role(little_endian_read_16(packet, 2));
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            role = mul_dev_get_handle_role(little_endian_read_16(packet, 3));
            break;
        }
        break;

    default:
        break;

    }


#if SUPPORT_MAX_SERVER
    if (role == 0xff || role == MULTI_ROLE_SERVER) {
        trans_cbk_packet_handler(packet_type, channel, packet, size);
    }
#endif

#if SUPPORT_MAX_CLIENT
    if (role == 0xff || role == MULTI_ROLE_CLIENT) {
        client_cbk_packet_handler(packet_type, channel, packet, size);
    }
#endif
}

//重设passkey回调函数，在这里可以重新设置passkey
//passkey为6个数字组成，十万位、万位。。。。个位 各表示一个数字 高位不够为0
/* static void reset_passkey_cb(u32 *key) */
static void ble_cbk_passkey_input(u32 *key, u16 conn_handle)
{
#if SUPPORT_MAX_SERVER
    if (mul_dev_get_handle_role(conn_handle) == MULTI_ROLE_SERVER) {
        trans_deal_passkey_input(key, conn_handle);
    }
#endif

#if SUPPORT_MAX_CLIENT
    if (mul_dev_get_handle_role(conn_handle) == MULTI_ROLE_CLIENT) {
        client_deal_passkey_input(key, conn_handle);
    }
#endif
}

void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    sm_init();
    sm_set_io_capabilities(io_type);

#if SUPPORT_MAX_CLIENT
    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        sm_set_master_io_capabilities(IO_CAPABILITY_KEYBOARD_ONLY);
    }
#endif

    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);

#if SUPPORT_MAX_SERVER
    sm_set_request_security(security_en);
#endif

#if SUPPORT_MAX_CLIENT
    sm_set_master_request_pair(security_en);
#endif

    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        /* reset_PK_cb_register(reset_passkey_cb); */
        reset_PK_cb_register_ext(ble_cbk_passkey_input);
    }
}

void ble_profile_init(void)
{
#if SUPPORT_MAX_SERVER && SUPPORT_MAX_CLIENT
    ble_stack_gatt_role(2);
#elif SUPPORT_MAX_CLIENT
    ble_stack_gatt_role(1);
#else
    ble_stack_gatt_role(0);
#endif

    ble_vendor_set_default_att_mtu(MULTI_ATT_MTU_SIZE);

    if (config_le_sm_support_enable) {
        le_device_db_init();
#if PASSKEY_ENTER_ENABLE
        ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING, 7, TCFG_BLE_SECURITY_EN);
#else
        ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING, 7, TCFG_BLE_SECURITY_EN);
#endif
    }

#if SUPPORT_MAX_SERVER
    server_profile_init();
#endif

#if SUPPORT_MAX_CLIENT
    client_profile_init();
#endif

    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    /* ble_l2cap_register_packet_handler(packet_cbk); */
    /* sm_event_packet_handler_register(packet_cbk); */
    le_l2cap_register_packet_handler(&cbk_packet_handler);

    ble_op_multi_att_send_init(multi_att_ram_buffer, MULTI_ATT_RAM_BUFSIZE, MULTI_ATT_LOCAL_PAYLOAD_SIZE);
}

//统一接口，关闭模块
void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);

#if SUPPORT_MAX_SERVER
    ble_trans_module_enable(en);
#endif

#if SUPPORT_MAX_CLIENT
    ble_client_module_enable(en);
#endif

}

void ble_app_disconnect(void)
{
#if SUPPORT_MAX_SERVER
    ble_multi_trans_disconnect();
#endif

#if SUPPORT_MAX_CLIENT
    ble_multi_client_disconnect();
#endif
}

void bt_ble_init(void)
{
    int error_reset = 0;
    //log_info("%s,%d\n", __FUNCTION__, __LINE__);
    //log_info("ble_file: %s", __FILE__);

    /*确认配置是否ok*/

    if (config_le_hci_connection_num && att_send_check_multi_dev(config_le_gatt_server_num, config_le_gatt_client_num)) {
        log_info("no support more device!!!\n");
        error_reset = 1;
    }

    if (error_reset) {
        printf("======error config!!!\n");
        ASSERT(0);
        while (1);
    }

#if SUPPORT_MAX_SERVER
    bt_multi_trans_init();
#endif

#if SUPPORT_MAX_CLIENT
    bt_multi_client_init();
#endif
}

void bt_ble_exit(void)
{
#if SUPPORT_MAX_SERVER
    bt_multi_trans_exit();
#endif

#if SUPPORT_MAX_CLIENT
    bt_multi_client_exit();
#endif
}

#endif

