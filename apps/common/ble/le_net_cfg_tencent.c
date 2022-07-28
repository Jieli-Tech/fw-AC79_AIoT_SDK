/*********************************************************************************************
    *   Filename        : le_server_module.c

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
#include "event/net_event.h"
#include "app_config.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "btcontroller_modules.h"
#include "bt_common.h"

#include "third_party/common/ble_user.h"
#include "le_net_cfg_tencent.h"
#include "le_common.h"

#include "os/os_api.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_NET_CFG_TENCENT)

#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

/* #define LOG_TAG_CONST       BT_BLE */
/* #define LOG_TAG             "[LE_S_DEMO]" */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
/* #define LOG_CLI_ENABLE */
/* #include "debug.h" */

//------
#define ATT_LOCAL_PAYLOAD_SIZE    (128)                   //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));
//---------------

//---------------
#define ADV_INTERVAL_MIN          (160)

static volatile hci_con_handle_t con_handle;
static void ble_module_enable(u8 en);

//连接参数设置
static const uint8_t connection_update_enable = 1; ///0--disable, 1--enable
static uint8_t connection_update_cnt = 0; //
static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 10, 600},//11
    {12, 28, 10, 600},//3.7
    {8,  20, 10, 600},
    /* {12, 28, 4, 600},//3.7 */
    /* {12, 24, 30, 600},//3.05 */
};
#define CONN_PARAM_TABLE_CNT      (sizeof(connection_param_table)/sizeof(struct conn_update_param_t))

// 广播包内容
/* static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31 */
// scan_rsp 内容
/* static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31 */

#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif

#define adv_data       &att_ram_buffer[0]
#define scan_rsp_data  &att_ram_buffer[32]

static char gap_device_name[BT_NAME_LEN_MAX] = "wl80_ble_test";
static u8 complete = 0;
static u8 gap_device_name_len = 0;
static u8 ble_work_state = 0;
static u8 adv_ctrl_en;
static OS_MUTEX mutex;
static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

static int app_send_user_data_check(u16 len);
static int app_send_user_data_do(void *priv, u8 *data, u16 len);
static int app_send_user_data(u16 handle, const u8 *data, u16 len, u8 handle_type);
extern u8 is_in_config_network_state(void);

// Complete Local Name  默认的蓝牙名字
extern const char *bt_get_local_name();

/********************************************/
#define WIFI_CONN_CNT   100   //延迟 ( x )*(20/100) s  等待wifi连接
#define DEFAULT_SEND_MAX_LEN	18		//数据包默认最大包长度 >= 6

#define DEFAULT_SSID_MAX_LEN		32		//默认ssid最大长度
#define DEFAULT_PWD_MAX_LEN			64		//默认pwd最大长度
#define DEFAULT_TOKEN_LEN			32		//默认token长度

extern int lwip_dhcp_bound(void);
extern void ble_wifi_config(void);
extern void qiot_device_bind_set_token(const char *token);
extern void tencent_net_config(const char *recv_buf, int recv_len, u8 mode);

//1字节长度，1字节\0
static u8 save_ssid_info[DEFAULT_SSID_MAX_LEN + 2] = {0};     //存放ssid信息，用于信息返回
static u8 save_pwd_info[DEFAULT_PWD_MAX_LEN + 2] = {0};     //存放password信息
//1字节\0
static u8 save_token_info[DEFAULT_TOKEN_LEN + 1] = {0};	//存放token信息

static int send_max_length;	//一次发送最长的长度
static int BTCombo_flag = 0;		//配网步骤标志位
extern void write_BTCombo(bool flag);

enum {
    BTCOMBO_RECEIVE = 1,
    SSID_RECEIVE,
    PWD_RECEIVE,
    CONNECT_REQUEST,
    TOKEN_RECEIVE,
    ERROR_OCCURRED,
};
/*******************************************/
//------------------------------------------------------
static void send_request_connect_parameter(u8 table_index)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[table_index];//static ram

    log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (con_handle) {
        ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, con_handle, param);
    }
}

static void check_connetion_updata_deal(void)
{
    if (connection_update_enable) {
        if (connection_update_cnt < CONN_PARAM_TABLE_CNT) {
            send_request_connect_parameter(connection_update_cnt);
        }
    }
}

static void connection_update_complete_success(u8 *packet)
{
    int con_handle, conn_interval, conn_latency, conn_timeout;

    con_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
}

static void set_ble_work_state(ble_state_e state)
{
    if (state != ble_work_state) {
        log_info("ble_work_st:%x->%x\n", ble_work_state, state);
        ble_work_state = state;
        if (app_ble_state_callback) {
            app_ble_state_callback((void *)channel_priv, state);
        }
    }
}

static ble_state_e get_ble_work_state(void)
{
    return ble_work_state;
}

static void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("Just Works Confirmed.\n");
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u.\n", tmp32);
            break;
        }
        break;
    }
}

static void can_send_now_wakeup(void)
{
    putchar('E');
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }
}

static const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

static void server_profile_start(u16 con_handle)
{
    ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
    set_ble_work_state(BLE_ST_CONNECT);
    /* set_connection_data_phy(CONN_SET_CODED_PHY, CONN_SET_CODED_PHY); */
}

/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */

/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;
    u8 status;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        /* case DAEMON_EVENT_HCI_PACKET_SENT: */
        /* break; */
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");
        case ATT_EVENT_CAN_SEND_NOW:
            can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
                status = hci_subevent_le_enhanced_connection_complete_get_status(packet);
                if (status) {
                    log_info("LE_SLAVE CONNECTION FAIL!!! %0x\n", status);
                    set_ble_work_state(BLE_ST_DISCONN);
                    break;
                }
                con_handle = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE : %0x\n", con_handle);
                log_info("conn_interval = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_interval(packet));
                log_info("conn_latency = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_latency(packet));
                log_info("conn_timeout = %d\n", hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(packet));
                server_profile_start(con_handle);
                break;

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", con_handle);
                connection_update_complete_success(packet + 8);
                server_profile_start(con_handle);
                break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                connection_update_complete_success(packet);
                break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE\n");
                /* set_connection_data_phy(CONN_SET_CODED_PHY, CONN_SET_CODED_PHY); */
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                log_info("Tx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                log_info("Rx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
            con_handle = 0;
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, 0, 0, 0);
            set_ble_work_state(BLE_ST_DISCONN);
#if !CONFIG_POWER_ON_ENABLE_BLE
            if (is_in_config_network_state()) {
#else
            {
#endif
                os_mutex_pend(&mutex, 0);
                bt_ble_adv_enable(1);
                complete = 0;
                os_mutex_post(&mutex);
            }
            connection_update_cnt = 0;
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %u\n", mtu);
            send_max_length = mtu;	//yii:如果手机端有做mtu交换，则更新下最大包长度
            ble_user_cmd_prepare(BLE_CMD_ATT_MTU_SIZE, 1, mtu);
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!\n");
                check_connetion_updata_deal();
            } else {
                connection_update_cnt = CONN_PARAM_TABLE_CNT;
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            break;
        }
        break;
    }
}

/* LISTING_END */

/*
 * @section ATT Read
 *
 * @text The ATT Server handles all reads to constant data. For dynamic data like the custom characteristic, the registered
 * att_read_callback is called. To handle long characteristics and long reads, the att_read_callback is first called
 * with buffer == NULL, to request the total value length. Then it will be called again requesting a chunk of the value.
 * See Listing attRead.
 */

/* LISTING_START(attRead): ATT Read */

// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback, handle= 0x%04x,buffer= %08x\n", handle, (u32)buffer);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        att_value_len = gap_device_name_len;

        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_device_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s \n", gap_device_name);
        }
        break;

    default:
        break;
    }

    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;
}

void set_default_length()
{
    send_max_length = DEFAULT_SEND_MAX_LEN;
}

static void send_wifi_report()
{
    char send_buf[128] = {0};
    int seq = 0;
    int offset = 0;
    char ready_send[128] = {0x01, 0x00, 0x00, 0x02};
    strncpy(&ready_send[4], (const char *)save_ssid_info, save_ssid_info[0] + 1);
    int remain_len = save_ssid_info[0] + 5;
    int send_len = send_max_length - 6;

    if (remain_len + 4 > send_max_length) {
        while (remain_len > send_len) {
            memset(send_buf, 0, sizeof(send_buf));
            send_buf[0] = 0x11;                 //0x11 发送配网状态给手机
            send_buf[1] = 0x10 | 0x04;          //0x14  0x10分片 0x04 设备发数
            send_buf[2] = seq++;                //序列号,每次加1
            send_buf[3] = send_len + 2;         //包含剩余数据长度在内2byte的数据长度
            send_buf[4] = remain_len % 0xff;    //剩余数据长度低位
            send_buf[5] = remain_len / 0xff;    //剩余数据长度高位

            memcpy(&send_buf[6], ready_send + offset, send_len);
            app_send_user_data(ATT_CHARACTERISTIC_FF02_01_VALUE_HANDLE, (u8 *)send_buf, send_len + 6, ATT_OP_AUTO_READ_CCC);

            remain_len  -= send_len;
            offset      += send_len;
            printf("-------remain len  %d     offset = %d", remain_len, offset);
            put_buf((const unsigned char *)send_buf, send_len + 6);
        }
    }
    memset(send_buf, 0, sizeof(send_buf));
    send_buf[0] = 0x11;
    send_buf[1] = 0x00 | 0x04;              //0x04
    send_buf[2] = seq++;
    send_buf[3] = remain_len;

    memcpy(&send_buf[4], ready_send + offset, remain_len);
    app_send_user_data(ATT_CHARACTERISTIC_FF02_01_VALUE_HANDLE, (u8 *)send_buf, remain_len + 4, ATT_OP_AUTO_READ_CCC);
    put_buf((const unsigned char *)send_buf, remain_len + 4);
}


int tc_wait_wifi_conn(void)
{
    int cnt = 0;
    while (!lwip_dhcp_bound()) {     //等待20s,检测wifi连接状态
        os_time_dly(20);
        cnt++;
        if (cnt >= WIFI_CONN_CNT) {
            u8 wifi_conn_fail[] = { 0x11, 0x04, 0x00, 0x03, 0x01, 0x01, 0x00};     //配网失败返回cmd
            app_send_user_data(ATT_CHARACTERISTIC_FF02_01_VALUE_HANDLE, wifi_conn_fail, sizeof(wifi_conn_fail), ATT_OP_AUTO_READ_CCC);
            return -1;
        }
    }
    send_wifi_report();

    return 0;

}

void send_reply_token(char *para_format, int len)
{
    u8 send_buf[64] = {0};
    int seq = 0;
    int offset = 0;
    char ready_send[128] = {0};
    strncpy(ready_send, para_format, len);
    int remain_len = len;
    int send_len = send_max_length - 6;

    printf("buf = %s  ---%lu", ready_send, strlen(ready_send));
    put_buf((const unsigned char *)ready_send, strlen(ready_send));

    while (remain_len > send_len) {
        memset(send_buf, 0, sizeof(send_buf));
        send_buf[0] = 0x15;                 //0x15 用户发送token 或者接收 token绑定状态信息
        send_buf[1] = 0x10 | 0x04;          //0x14  0x10分片 0x04 设备发数
        send_buf[2] = seq++;                //序列号,每次加1
        send_buf[3] = send_len + 2;         //包含剩余数据长度在内2byte的数据长度
        send_buf[4] = remain_len % 0xff;    //剩余数据长度低位
        send_buf[5] = remain_len / 0xff;    //剩余数据长度高位

        memcpy(&send_buf[6], ready_send + offset, send_len);
        app_send_user_data(ATT_CHARACTERISTIC_FF02_01_VALUE_HANDLE, send_buf, send_len + 6, ATT_OP_AUTO_READ_CCC);

        remain_len  -= send_len;
        offset      += send_len;
        printf("-------remain len  %d     offset = %d", remain_len, offset);
        put_buf(send_buf, send_len + 6);
    }
    memset(send_buf, 0, sizeof(send_buf));
    send_buf[0] = 0x15;
    send_buf[1] = 0x00 | 0x04;              //0x04
    send_buf[2] = seq++;
    send_buf[3] = remain_len;

    memcpy(&send_buf[4], ready_send + offset, remain_len); //15 04 01 0E 41 00
    app_send_user_data(ATT_CHARACTERISTIC_FF02_01_VALUE_HANDLE, send_buf, remain_len + 4, ATT_OP_AUTO_READ_CCC);
    put_buf(send_buf, remain_len + 4);

}

/* LISTING_END */
/*
 * @section ATT Write
 *
 * @text The only valid ATT write in this example is to the Client Characteristic Configuration, which configures notification
 * and indication. If the ATT handle matches the client configuration handle, the new configuration value is stored and used
 * in the heartbeat handler to decide if a new value should be sent. See Listing attWrite.
 */

/* LISTING_START(attWrite): ATT Write */
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;
    u16 handle = att_handle;
    log_info("write_callback, handle= 0x%04x,size = %d\n", handle, buffer_size);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_FF01_01_VALUE_HANDLE:
        printf("\n-FF01_rx(%d):", buffer_size);
        printf_buf(buffer, buffer_size);
        if (buffer[0] == 0x08) {
            BTCombo_flag = BTCOMBO_RECEIVE;

        } else if (buffer[0] == 0x09) {
            BTCombo_flag = SSID_RECEIVE;
            if (buffer[3] <= DEFAULT_SSID_MAX_LEN) {
                strncat((char *)save_ssid_info, (const char *)&buffer[3], buffer_size - 3);       //buffer[3] 是ssid长度 buffer[4] 是ssid
            } else {
                //接收到的ssid长度大于最大长度
                log_info("err: ssid len too long\n");
            }

        } else if (buffer[0] == 0x0D) {
            BTCombo_flag = PWD_RECEIVE;
            if (buffer[3] <= DEFAULT_PWD_MAX_LEN) {
                strncat((char *)save_pwd_info, (const char *)&buffer[3], buffer_size - 3);       //buffer[3] 是pwd长度 buffer[4] 是pwd
            } else {
                log_info("err: pwd len too long\n");
            }

        } else if (buffer[0] == 0x0C) {
            BTCombo_flag = CONNECT_REQUEST;
            tencent_net_config(NULL, 0, 3);                    //接收wifi 连接请求，开始连接wifi
            thread_fork("ble_wifi_config", 20, 512, 0, NULL, (void (*)(void *))ble_wifi_config, NULL); //yii:创建腾讯连连通信线程

        } else if (buffer[0] == 0x15) {
            BTCombo_flag = TOKEN_RECEIVE;
            if (buffer[3] <= DEFAULT_TOKEN_LEN) {
                strncat((char *)save_token_info, (const char *)&buffer[4], buffer_size - 4);
            } else {
                log_info("err: token len too long\n");
            }
        }

        switch (BTCombo_flag) {
        case BTCOMBO_RECEIVE:
            //接收到该消息,意味着配网开始,先将配网标志位false
            write_BTCombo(false);
            memset(save_ssid_info, 0, sizeof(save_ssid_info));
            memset(save_pwd_info, 0, sizeof(save_pwd_info));
            memset(save_token_info, 0, sizeof(save_token_info));
            break;
        case SSID_RECEIVE:
            if (buffer[0] != 0x09) {
                if (strlen((const char *)save_ssid_info) + buffer_size <= DEFAULT_SSID_MAX_LEN) {
                    strncat((char *)save_ssid_info, (const char *)buffer, buffer_size);
                } else {
                    log_info("err: ssid len too long\n");
                }
            }
            if (strlen((const char *)save_ssid_info) == save_ssid_info[0] + 1) {
                tencent_net_config((const char *)&save_ssid_info[1], save_ssid_info[0], 1); //接收wifi ssid
            }
            break;

        case PWD_RECEIVE:
            if (buffer[0] != 0x0D) {
                if (strlen((const char *)save_pwd_info) + buffer_size <= DEFAULT_PWD_MAX_LEN) {
                    strncat((char *)save_pwd_info, (const char *)buffer, buffer_size);
                } else {
                    log_info("err: pwd len too long\n");
                }
            }
            /* put_buf(save_pwd_info, strlen((const char *)save_pwd_info) + 1); */
            if (strlen((const char *)save_pwd_info) == save_pwd_info[0] + 1) {
                tencent_net_config((const char *)&save_pwd_info[1], save_pwd_info[0], 2); //接收wifi pwd
            }
            break;

        case TOKEN_RECEIVE:
            if (buffer[0] != 0x15) {
                if (strlen((const char *)save_token_info) + buffer_size <= DEFAULT_TOKEN_LEN) {
                    strncat((char *)save_token_info, (const char *)buffer, buffer_size);
                } else {
                    log_info("err: token len too long\n");
                }
            }
            if (strlen((const char *)save_token_info) == DEFAULT_TOKEN_LEN) {
                qiot_device_bind_set_token((const char *)save_token_info);	//yii:获得小程序下发回来的token
            }
            break;

        case ERROR_OCCURRED:
            //TODO这里发送错误日志
            printf("-------%s-------%d\n\r", __func__, __LINE__);
            break;

        default:
            break;
        }
        break;

    case ATT_CHARACTERISTIC_FF02_01_CLIENT_CONFIGURATION_HANDLE:
        printf_buf(buffer, buffer_size);
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        check_connetion_updata_deal();
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        att_set_ccc_config(handle, buffer[0]);
        break;

    default:
        break;
    }

    return 0;
}

static int app_send_user_data(u16 handle, const u8 *data, u16 len, u8 handle_type)
{
    u32 ret = APP_BLE_NO_ERROR;

    if (!con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    os_mutex_pend(&mutex, 0);

    if (!att_get_ccc_config(handle + 1)) {
        log_info("fail,no write ccc!!!,%04x\n", handle + 1);
        os_mutex_post(&mutex);
        return APP_BLE_NO_WRITE_CCC;
    }

    ret = ble_user_cmd_prepare(BLE_CMD_ATT_SEND_DATA, 4, handle, data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    os_mutex_post(&mutex);

    if (ret) {
        log_info("app_send_fail:%d !!!!!!\n", ret);
    }
    return ret;
}

//------------------------------------------------------
static int make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, 0xFFFF, 2);


    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, offset, buf);
    return 0;
}

static int make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;

    u8 name_len = gap_device_name_len;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len > vaild_len) {
        name_len = vaild_len;
    }
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, offset, buf);
    return 0;
}

//广播参数设置
static void advertisements_setup_init()
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    int   ret = 0;

    ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, ADV_INTERVAL_MIN, adv_type, adv_channel);

    ret |= make_set_adv_data();
    ret |= make_set_rsp_data();

    if (ret) {
        puts("advertisements_setup_init fail !!!!!!\n");
        return;
    }
}

#define PASSKEY_ENTER_ENABLE      0 //输入passkey使能，可修改passkey
//重设passkey回调函数，在这里可以重新设置passkey
//passkey为6个数字组成，十万位、万位。。。。个位 各表示一个数字 高位不够为0
static void reset_passkey_cb(u32 *key)
{
#if 1
    u32 newkey = rand32();//获取随机数

    newkey &= 0xfffff;
    if (newkey > 999999) {
        newkey = newkey - 999999; //不能大于999999
    }
    *key = newkey; //小于或等于六位数
    printf("set new_key= %06u\n", *key);
#else
    *key = 123456; //for debug
#endif
}

extern void reset_PK_cb_register(void (*reset_pk)(u32 *));
void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    sm_init();
    sm_set_io_capabilities(io_type);
    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);
    sm_set_request_security(security_en);
    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}

extern void le_device_db_init(void);
void ble_profile_init(void)
{
    printf("ble profile init\n");
    le_device_db_init();

#if PASSKEY_ENTER_ENABLE
    ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_BONDING | SM_AUTHREQ_MITM_PROTECTION, 7, TCFG_BLE_SECURITY_EN);
#else
    ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING | SM_AUTHREQ_MITM_PROTECTION, 7, TCFG_BLE_SECURITY_EN);
#endif

    /* setup ATT server */
    att_server_init(profile_data, att_read_callback, att_write_callback);
    att_server_register_packet_handler(cbk_packet_handler);
    /* gatt_client_register_packet_handler(packet_cbk); */

    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    /* ble_l2cap_register_packet_handler(packet_cbk); */
    /* sm_event_packet_handler_register(packet_cbk); */
    le_l2cap_register_packet_handler(&cbk_packet_handler);
}

static int set_adv_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!adv_ctrl_en && en) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  get_ble_work_state();
    switch (cur_state) {
    case BLE_ST_ADV:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
        break;
    default:
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }
    log_info("adv_en:%d\n", en);
    set_ble_work_state(next_state);
    if (en) {
        advertisements_setup_init();
    }
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, en);
    return APP_BLE_NO_ERROR;
}

static int ble_disconnect(void *priv)
{
    if (con_handle) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info(">>>ble send disconnect\n");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, con_handle);
        } else {
            log_info(">>>ble wait disconnect...\n");
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

static int get_buffer_vaild_len(void *priv)
{
    u32 vaild_len = 0;
    ble_user_cmd_prepare(BLE_CMD_ATT_VAILD_LEN, 1, &vaild_len);
    return vaild_len;
}

static int app_send_user_data_do(void *priv, u8 *data, u16 len)
{
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-le_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return app_send_user_data(ATT_CHARACTERISTIC_ae82_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}

static int app_send_user_data_check(u16 len)
{
    u32 buf_space = get_buffer_vaild_len(0);
    if (len <= buf_space) {
        return 1;
    }
    return 0;
}

static int regiest_wakeup_send(void *priv, void *cbk)
{
    ble_resume_send_wakeup = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_recieve_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_recieve_callback = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_state_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_ble_state_callback = cbk;
    return APP_BLE_NO_ERROR;
}

void bt_ble_adv_enable(u8 enable)
{
    set_adv_enable(0, enable);
}

static void ble_module_enable(u8 en)
{
    os_mutex_pend(&mutex, 0);
    log_info("mode_en:%d\n", en);
    if (en) {
        extern u8 get_ble_gatt_role(void);
        if (1 == get_ble_gatt_role()) {
            ble_stack_gatt_role(0);
            // register for HCI events
            hci_event_callback_set(&cbk_packet_handler);
            le_l2cap_register_packet_handler(&cbk_packet_handler);
        }
        complete = 0;
        adv_ctrl_en = 1;
        bt_ble_adv_enable(1);
    } else {
        if (con_handle) {
            if (complete) {
                os_mutex_post(&mutex);
                return;
            }
            adv_ctrl_en = 0;
            ble_disconnect(NULL);
        } else {
            bt_ble_adv_enable(0);
            adv_ctrl_en = 0;
        }
        complete = 0;
    }
    os_mutex_post(&mutex);
}

static const char ble_ext_name[] = "(BLE)";

void bt_ble_init(void)
{
    log_info("***** ble_init******\n");
    const char *name_p = bt_get_local_name();
    u8 ext_name_len = sizeof(ble_ext_name) - 1;

    if (!os_mutex_valid(&mutex)) {
        os_mutex_create(&mutex);
    }

    gap_device_name_len = strlen(name_p);
    if (gap_device_name_len > BT_NAME_LEN_MAX - ext_name_len) {
        gap_device_name_len = BT_NAME_LEN_MAX - ext_name_len;
    }

    //增加后缀，区分名字
    memcpy(gap_device_name, name_p, gap_device_name_len);
    memcpy(&gap_device_name[gap_device_name_len], "(BLE)", ext_name_len);
    gap_device_name_len += ext_name_len;

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

    set_ble_work_state(BLE_ST_INIT_OK);
    ble_module_enable(1);
    BTCombo_flag = 0;	//初始化清空标志位
}

static void stack_exit(void)
{
    ble_module_enable(0);
    /* set_ble_work_state(BLE_ST_SEND_STACK_EXIT); */
    /* ble_user_cmd_prepare(BLE_CMD_STACK_EXIT, 1, 0); */
    /* set_ble_work_state(BLE_ST_STACK_EXIT_COMPLETE); */
}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");

    stack_exit();
}

void ble_app_disconnect(void)
{
    ble_disconnect(NULL);
}

static const struct ble_server_operation_t mi_ble_operation = {
    .adv_enable = set_adv_enable,
    .disconnect = ble_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .send_data = (void *)app_send_user_data_do,
    .regist_wakeup_send = regiest_wakeup_send,
    .regist_recieve_cbk = regiest_recieve_cbk,
    .regist_state_cbk = regiest_state_cbk,
};

void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&mi_ble_operation;
}

#endif


