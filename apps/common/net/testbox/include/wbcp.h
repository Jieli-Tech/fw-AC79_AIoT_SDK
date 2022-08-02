#ifndef _WIFIBOX_WBCP_H_
#define _WIFIBOX_WBCP_H_

#include "typedef.h"
#include "asm/crc16.h"

// ==================================== WBCP 配置 ====================================
#define     WBCP_CONN_SERIAL    ("WIFIBOX")
#define     WBCP_VERSION_ID                 (1)         //版本号
#define     MAC_ADDR_SIZE                   (6)         //MAC地址大小
#define     WBCP_WAIT_ACK_TIMEOUT           (50)        //等待ack的超时时间(ticks)
#define     WBCP_SEND_WITHACK_RETRY_PERIOD  (5)         //重发间隔(ticks)
#define     MAX_RESPOND_CNT                 (50)        //从机发送连接响应的最大次数
#define     AUDIO_DATA_SIZE                 (32)       //音频数据包大小
//#define     WBCP_CAL_DUT

// ==================================== WBCP 命令 ====================================
enum fsm_status {
    PWRTHR_ADJ_STATUS = 0,   //功率阈值校准状态
    SENTHR_ADJ_STATUS,       //灵敏度阈值校准状态
    FINISH_ADJ_STATUS,       //结束校准状态

    PWR_TEST_STATUS,         //功率测试状态
    SEN_TEST_STATUS,         //灵敏度测试状态
    FREQ_TEST_STATUS,        //频偏测试状态
    FINISH_TEST_STATUS,      //结束测试状态
};


enum wbcp_event {
    ENTER_TEST_EVENT = 0,   //进入测试命令

    PWRTHR_ADJ_EVENT,       //功率阈值校准命令
    SENTHR_ADJ_EVENT,       //灵敏度阈值校准命令
    FINISH_ADJ_EVENT,       //结束校准命令

    PWR_TEST_EVENT,         //功率测试命令
    SEN_TEST_EVENT,         //灵敏度测试命令
    FREQ_TEST_EVENT,        //频偏测试命令
    FINISH_TEST_EVENT,      //结束测试命令
};


typedef struct {
    u8 stat;
    u32 pcnt;
    s32 sen;
} senthr_adj_event_params_type;


typedef struct {
    u8 bt_mac[MAC_ADDR_SIZE];
    s32 sen;
    s32 pcnt;
} freq_test_event_params_type;


typedef struct {
    u8 result;
} finish_test_event_params_type;


typedef struct {
    float pwr;
    float evm;
    s32 freq;
    s32 sen;
    u32 loss_pag;
    float rx_rate;
} calibrate_params_type;

typedef struct {
    u32 thradj_fsm[2];
    u32 test_fsm[4];
} fsm_timeout_type;


typedef struct {
    u32 wifi_pwr_option: 1;
    u32 wifi_sen_option: 1;
    u32 wifi_rx_rate_option: 1;
    u32 wifi_freq_option: 1;
    u32 reserved: 28;
} test_option_type;


typedef struct {
    u8 serial[8];
    u8 mac[6];
    u8 mode;
    u8 channel;
    u8 conn_mcs;
    u8 conn_phymode;
    u8 test_mcs;
    u8 test_phymode;
    s8 rssi_thr;
    u8 dut_reset_flag;
    u8 freq_adj_flag;
    fsm_timeout_type fsm_timeout;
    test_option_type test_option;
} wbcp_conn_params_type;


typedef union {
    senthr_adj_event_params_type  senthr_adj_event_params;
    freq_test_event_params_type   freq_test_event_params;
    finish_test_event_params_type finish_test_event_params;
    calibrate_params_type cal_params;
    wbcp_conn_params_type conn_params;
} event_params_type;



// ================================== WBCP 数据结构 ==================================


typedef struct {
    s32 pcnt;
    s32 pwr;
    s32 evm;
    s32 freq;
} stat_res_type;


enum {
    START_SEARCH_MSG,
    STOP_SEARCH_MSG,
    UPDATE_CH_MSG,
    ENTER_TEST_SUCC_MSG,
    ENTER_TEST_FAIL_MSG,
    NEW_EVENT_MSG,
    RECV_AUDIO_MSG,
};


enum wbcp_data_type {
    EVENT_DATA_TYPE,
    RESP_DATA_TYPE,
    AUDIO_DATA_TYPE,
    NULL_DATA_TYPE,
};


typedef struct {
    u8 event;
    event_params_type params;
} event_data_type;


typedef struct {
    u16 total_size;
    u8 total_seq;
    u8 data[AUDIO_DATA_SIZE];
} audio_data_type;


typedef union {
    event_data_type event_data;
    audio_data_type audio_data;
} wbcp_data_type;


typedef struct {
    u8 version;  //协议版本号
    u8 type;     //数据类型
    u8 ack;
    u8 seq;      //连续包序号
    u32 idx;     //数据包编号，全局唯一
    wbcp_data_type wbcp_data;
} wbcp_payload_type;


typedef struct {
    u8 none_data_1[34];
    u8 src_mac[6];
    u8 dest_mac[6];
    u8 none_data_2[46];
    u16 crc;
    wbcp_conn_params_type conn_params;
    wbcp_payload_type payload;
} wbcp_rx_packet_type;


typedef struct {
    u8 none_data_1[30];
    u8 src_mac[6];
    u8 dest_mac[6];
    u8 none_data_2[42];
    u16 crc;
    wbcp_conn_params_type conn_params;
    wbcp_payload_type payload;
    // u8 none[32];
} wbcp_tx_packet_type;


// ================================== WBCP 接口 ==================================

typedef struct {
    s8(*init)(u8 mode, u8 is_assist);
    s32(*update_seach_status)(u8 type, u8 ch);
    s8(*build_conn)(void *params);
    void (*break_conn)(void);
    void (*rx_shield)(void);
    s8(*send_event_data)(event_data_type *event_data, u8 *dest_mac, u8 ack);
    s8(*send_audio_data)(audio_data_type *audio_data, u8 *dest_mac, u8 seq);
    s8(*send_null_data)(u8 *dest_mac);
    s32(*start_send_test_data)(u8 *dest_mac, u32 periodic, u32 count);
    void (*stop_send_test_data)(void);
    void (*regist_usr_handler)(void *priv);
    void (*regist_audio_data_user_handler)(void *priv);
    void *(*start_stat_prop)(u32 count, u32 timeout);
    void *(*stop_stat_prop)(void);
} wbcp_operations_interface_type;

u8 is_cal_client_mac(void);
void wbcp_rx_frame_cb(void *rxwi, void *header, void *data, u32 len, void *reserve);
void get_wbcp_operations_interface(wbcp_operations_interface_type **ops);
u8 get_wbcp_connect_status(void);
void set_wbcp_mode(u8 mode);

#define     WBCP_RX_PACKET_SIZE      (sizeof(wbcp_rx_packet_type))
#define     WBCP_TX_PACKET_SIZE      (sizeof(wbcp_tx_packet_type))
#define     WBCP_CONN_PARAMS_SIZE    (sizeof(wbcp_conn_params_type))
#define     WBCP_PAYLOAD_SIZE        (sizeof(wbcp_payload_type))
#define     WBCP_DATA_SIZE           (sizeof(wbcp_data_type))
#define     WBCP_EVENT_DATA_SIZE     (sizeof(event_data_type))
#define     WBCP_AUDIO_DATA_SIZE     (sizeof(audio_data_type))

#define     WBCP_CONN_PARAMS(packet)    (packet->conn_params)
#define     WBCP_CONN_SER(packet)       (packet->conn_params.serial)
#define     WBCP_CRC(packet)            (packet->crc)
#define     WBCP_SRC_MAC(packet)        (packet->src_mac)
#define     WBCP_DEST_MAC(packet)       (packet->dest_mac)
#define     WBCP_PAYLOAD(packet)        (packet->payload)
#define     WBCP_VERSION(packet)        (packet->payload.version)
#define     WBCP_TYPE(packet)           (packet->payload.type)
#define     WBCP_ACK(packet)            (packet->payload.ack)
#define     WBCP_SEQ(packet)            (packet->payload.seq)
#define     WBCP_IDX(packet)            (packet->payload.idx)
#define     WBCP_DATA(packet)           (packet->payload.wbcp_data)
#define     WBCP_EVENT_DATA(packet)     (packet->payload.wbcp_data.event_data)
#define     WBCP_AUDIO_DATA(packet)     (packet->payload.wbcp_data.audio_data)
#define     WBCP_EVENT(packet)          (packet->payload.wbcp_data.event_data.event)

#define     WBCP_PACKET_CRC_CAL(packet)        (CRC16(&packet->payload, WBCP_PAYLOAD_SIZE))
#define     WBCP_CONN_PARAMS_CRC_CAL(packet)   (CRC16(&packet->conn_params, WBCP_CONN_PARAMS_SIZE))


#endif


