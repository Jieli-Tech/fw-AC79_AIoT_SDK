#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "app_config.h"
#include "device/uart.h"

#ifdef CONFIG_WIFIBOX_ENABLE
#include "client.h"

#ifdef WBCP_DEBUG_ENABLE
#define     log_info(x, ...)     printf("[WBCP][INFO]" x " ", ## __VA_ARGS__)
#define     log_err(x, ...)      printf("[WBCP][ERR]" x " ", ## __VA_ARGS__)
#else
#define     log_info(...)
#define     log_err(...)
#endif

#define     LOCK_TX_PACKET()    (os_mutex_pend(&wbcp.tx_mutex, 0))
#define     UNLOCK_TX_PACKET()  (os_mutex_post(&wbcp.tx_mutex))

static u8 user_handler_task[64] = {0};
static void (*wbcp_audio_data_user_handler)(void *priv) = NULL;

typedef	struct	GNU_PACKED {
    /* Word	0 */
    u32		WirelessCliID: 8;
    u32		KeyIndex: 2;
    u32		BSSID: 3;
    u32		UDF: 3;
    u32		MPDUtotalByteCount: 12;
    u32		TID: 4;
    /* Word	1 */
    u32		FRAG: 4;
    u32		SEQUENCE: 12;
    u32		MCS: 7;
    u32		BW: 1;
    u32		ShortGI: 1;
    u32		STBC: 2;
    u32		rsv: 3;
    u32		PHYMODE: 2;             /* 1: this RX frame is unicast to me */
    /*Word2 */
    u32		RSSI0: 8;
    u32		RSSI1: 8;
    u32		RSSI2: 8;
    u32		rsv1: 8;
    /*Word3 */
    u32		SNR0: 8;
    u32		SNR1: 8;
    u32		FOFFSET: 8;
    u32		rsv2: 8;
    /*UINT32		rsv2:16;*/
} RXWI_STRUC, *PRXWI_STRUC;


enum {
    START_SEND_MSG,
    SEND_PACKET_MSG,
};


static struct {
    u8 start;
    s32 count;
    s32 num;
    s32 pwr_sum;
    s32 evm_sum;
    s32 freq_sum;
    u8 send_test_flag;
    OS_SEM stat_sem;
    OS_SEM test_sem;
    stat_res_type stat_res;
} stat_prop;

static struct {
    u8 mode;
    u8 assist_flag;
    u8 build_conn;
    u8 enter;
    u8 exit;
    u8 shield;
    u8 respond_flag;
    u8 send_withack_flag;
    OS_MUTEX tx_mutex;
    OS_MUTEX ack_mutex;
    OS_SEM tx_sem;
    OS_SEM respond_sem;
    wbcp_tx_packet_type tx_packet;
    wbcp_tx_packet_type ack_packet;
} wbcp = {0};

static wbcp_conn_params_type conn_params;

static u8 BROADCAST_MAC[MAC_ADDR_SIZE] = {0x8A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F};
static u8 LOCAL_MAC[MAC_ADDR_SIZE] = {0};
static u8 REMOTE_MAC[MAC_ADDR_SIZE] = {0};

extern unsigned char bbp_rd(unsigned char adr);
static void wbcp_send_ack(wbcp_payload_type *payload, u8 *dest_mac, u8 mcs, u8 phymode);
static s8 wbcp_respond_conn_req(void);
static s8 wbcp_send_event_data(event_data_type *event_data, u8 *dest_mac, u8 ack);
static s32 wbcp_update_seach_status(u8 type, u8 ch);
static void conn_params_show(void *priv);
static s32 wbcp_update_event_to_user_handler(u8 type, s32 msg_size, s32 *msg);
static s8 wifi_init(void);


typedef struct {
    u16 crc;
    wbcp_payload_type payload;
} assist_data_type;

static u8 assist_stop_flag = 0;
static u32 assist_idx = 0;
static OS_SEM assist_sem;

static void *uart_hdl = NULL;
static u8 uart_buf[5 * 1024] __attribute__((aligned(32)));


static void assist_tx_task(void *priv)
{
    u8 retry_cnt = 0;
    assist_data_type tx_data;

    tx_data.payload.version = WBCP_VERSION_ID;
    tx_data.payload.type = EVENT_DATA_TYPE;
    tx_data.payload.ack = 0;
    tx_data.payload.seq = 0;
    tx_data.payload.idx = ++assist_idx;
    memcpy(&tx_data.payload.wbcp_data, (u8 *)priv, sizeof(wbcp_data_type));
    tx_data.crc = CRC16(&tx_data.payload, sizeof(wbcp_payload_type));

    for (;;) {
        s32 send_ret = dev_write(uart_hdl, &tx_data, sizeof(assist_data_type));
        log_info("assist retry_cnt = %d, send_ret = %d", retry_cnt, send_ret);
        retry_cnt++;
        os_time_dly(5);
        if (assist_stop_flag) {
            break;
        }
    }
}


static u32 assist_tx_data(void *event_data)
{
    u32 ret = 0;

    assist_stop_flag = 0;
    /* task_create(assist_tx_task, event_data, "assist_tx"); */
    thread_fork("assist_tx_task", 12, 512, 0, NULL, assist_tx_task, event_data);

    os_sem_set(&assist_sem, 0);
    if (os_sem_pend(&assist_sem, 50) != OS_NO_ERR) {
        log_err("assist tx timeout");
        ret = 0;
    } else {
        ret = sizeof(event_data_type);
    }

    assist_stop_flag = 1;
    task_kill("assist_tx");

    return ret;
}


static void assist_data_handler(event_data_type *data)
{
    s32 msg;

    log_info("assist event = %d\n", data->event);
    if (data->event == ENTER_TEST_EVENT) {
        switch (wbcp.mode) {
        case WB_SERVER_MODE:
            wbcp.enter = 1;
            memcpy(REMOTE_MAC, data->params.conn_params.mac, MAC_ADDR_SIZE);
            msg = (s32)&REMOTE_MAC[0];
            wbcp_update_event_to_user_handler(ENTER_TEST_SUCC_MSG, 1, &msg);
            break;

        case WB_CLIENT_MODE:
            memcpy(REMOTE_MAC, data->params.conn_params.mac, MAC_ADDR_SIZE);
            memcpy(&conn_params, &data->params.conn_params, WBCP_CONN_PARAMS_SIZE);
            conn_params_show(&conn_params);
            os_sem_post(&wbcp.respond_sem);
            break;

        default:
            break;
        }
    } else {
        wbcp_update_event_to_user_handler(NEW_EVENT_MSG, WBCP_EVENT_DATA_SIZE / sizeof(s32), (s32 *)data);
    }
}


static void assist_rx_task(void *priv)
{
    u16 crc;
    u32 len = 0;
    assist_data_type rx_data = {0};
    u32 last_event_idx = 0, last_ack_idx = 0;

    for (;;) {
        memset(&rx_data, 0, sizeof(assist_data_type));
        len = dev_read(uart_hdl, &rx_data, sizeof(assist_data_type));
        if (len == UART_CIRCULAR_BUFFER_WRITE_OVERLAY) {
            log_err("UART_CIRCULAR_BUFFER_WRITE_OVERLAY");
            dev_ioctl(uart_hdl, UART_FLUSH, 0);
            continue;
        }

        if (len == sizeof(assist_data_type)) {
            crc = CRC16(&rx_data.payload, sizeof(wbcp_payload_type));
            if (rx_data.crc != crc || !crc) {
                /* log_err("assist rx_data crc err, rx_data.crc = %d, crc = %d\n", rx_data.crc, crc); */
                continue;
            }

            switch (rx_data.payload.type) {
            case RESP_DATA_TYPE:
                if (rx_data.payload.idx != last_ack_idx) {
                    os_sem_post(&assist_sem);
                    last_ack_idx = rx_data.payload.idx;
                } else {
                    log_err("the same assist ack\n");
                }
                break;

            case EVENT_DATA_TYPE:
                rx_data.payload.type = RESP_DATA_TYPE;
                rx_data.crc = CRC16(&rx_data.payload, sizeof(wbcp_payload_type));
                dev_write(uart_hdl, &rx_data, sizeof(assist_data_type));
                if (rx_data.payload.idx != last_event_idx || rx_data.payload.wbcp_data.event_data.event == ENTER_TEST_EVENT) {
                    /* put_buf(&rx_data.payload.wbcp_data, sizeof(wbcp_data_type)); */
                    last_event_idx = rx_data.payload.idx;
                    assist_data_handler(&rx_data.payload.wbcp_data.event_data);
                } else {
                    log_err("the same assist event\n");
                }
                break;

            default:
                break;
            }
        }
    }
}


static u8 assist_dev_init(void)
{
    uart_hdl = dev_open(WB_ASSIST_UART, NULL);
    if (!uart_hdl) {
        log_err("special uart init fail");
        return -1;
    } else {
        log_err("special uart init succ");
    }
    dev_ioctl(uart_hdl, UART_SET_CIRCULAR_BUFF_ADDR, (int)uart_buf);
    dev_ioctl(uart_hdl, UART_SET_CIRCULAR_BUFF_LENTH, sizeof(uart_buf));
    dev_ioctl(uart_hdl, UART_SET_RECV_BLOCK, 1);
    dev_ioctl(uart_hdl, UART_START, 0);

    os_sem_create(&assist_sem, 0);
    thread_fork("assist_rx_task", 13, 512, 0, NULL, assist_rx_task, NULL);
    return 0;
}


u8 is_cal_client_mac(void)
{
    return ((REMOTE_MAC[0] == 0xab && REMOTE_MAC[1] == 0xab) ? 1 : 0);
}
static void wifi_enter_smp_mode(void)
{
    wifi_set_smp_cfg_scan_all_channel(1);
    wifi_set_smp_cfg_just_monitor_mode(0);
    wifi_set_smp_cfg_timeout(100);
    wifi_set_frame_cb(wbcp_rx_frame_cb);
    wifi_enter_smp_cfg_mode();
}


static s8 wifi_init(void)
{
    static u8 init = 0;

    if (wbcp.mode == WB_SERVER_MODE) {
        if (init) {
            log_info("wifi already set");
            return 0;
        }
        extern int wifi_is_on(void);
        while (wifi_is_on() == 0) {
            os_time_dly(1);
            putchar('.');
        }
        wifi_set_short_retry(0);
        wifi_set_long_retry(0);
    }

    if (wbcp.mode == WB_SERVER_MODE) {
        wifi_set_frame_cb(wbcp_rx_frame_cb);
    }
    wifi_set_smp_cfg_scan_all_channel(0);
    wifi_set_smp_cfg_just_monitor_mode(1);
    wifi_set_smp_cfg_timeout(-1);
    wifi_enter_smp_cfg_mode();

    init = 1;
    return 0;
}


static u32 wifi_send_packet(char *pkg, int len, char mcs, char phymode)
{
    wifi_set_short_retry(0);
    wifi_set_long_retry(0);

    char i = 0;
    static  const  struct  {
        char mode;
        char mcs;
    } mcs_mode_tab[] = {
        {0, 0}, //0:CCK 1M
        {0, 1}, //1:CCK 2M
        {0, 2}, //2:CCK 5.5M
        {1, 0}, //3:OFDM 6M
        {2, 0}, //4:MCS0/7.2M
        {1, 1}, //5:OFDM 9M
        {0, 3}, //6:CCK 11M
        {1, 2}, //7:OFDM 12M
        {2, 1}, //8:MCS1/14.4M
        {1, 3}, //9:OFDM 18M
        {2, 2}, //10:MCS2/21.7M
        {1, 4}, //11:OFDM 24M
        {2, 3}, //12:MCS3/28.9M
        {1, 5}, //13:OFDM 36M
        {2, 4}, //14:MCS4/43.3M
        {1, 6}, //15:OFDM 48M
        {1, 7}, //16:OFDM 54M
        {2, 5}, //17:MCS5/57.8M
        {2, 6}, //18:MCS6/65.0M
        {2, 7}, //19:MCS7/72.2M
    };
    for (i = 0; i < sizeof(mcs_mode_tab) / sizeof(mcs_mode_tab[0]); i++) {
        if (mcs_mode_tab[i].mode == phymode && mcs_mode_tab[i].mcs == mcs) {
            break;
        }
    }

    wifi_send_data(pkg, len + 4, i);

    return len;
}


static void wbcp_event_data_handler(void *priv)
{
    u32 msg[16] = {0};
    static u32 idx_last = 100, recv_count = 0;
    u8 case_1 = 0, case_2 = 0, case_3 = 0, the_same_flag = 0;
    wbcp_rx_packet_type *rx_packet = (wbcp_rx_packet_type *)priv;

    if (WBCP_IDX(rx_packet) != idx_last) {
        idx_last = WBCP_IDX(rx_packet);
        the_same_flag = 0;
    } else {
        the_same_flag = 1;
        /* log_info("the same idx.\n");  */
    }

    /* log_info("event = %d, idx = %d, idx_last = %d, the_same_flag = %d.\n", WBCP_EVENT(rx_packet), WBCP_IDX(rx_packet), idx_last, the_same_flag); */

    /* if (the_same_flag == 0) { */
    /*     recv_count++; */
    /*     log_info("recv_count = %d.\n", recv_count); */
    /* } */

    //1.进入测试模式之前，只处理 ENTER_TEST_EVENT
    //2.进入测试模式之后，处理需要ack的重复数据、ENTER_TEST_EVENT
    //3.进入测试模式之后，处理非重复数据

    case_1 = !wbcp.enter && WBCP_EVENT(rx_packet) == ENTER_TEST_EVENT;
    case_2 =  wbcp.enter && the_same_flag && (WBCP_ACK(rx_packet) || WBCP_EVENT(rx_packet) == ENTER_TEST_EVENT);
    case_3 =  wbcp.enter && !the_same_flag;

    if (case_1 || case_2 || case_3) {
        log_info("case_1 = %d, case_2 = %d, case_3 = %d.", case_1, case_2, case_3);
        msg[0] = WBCP_IDX(rx_packet);
        msg[1] = WBCP_ACK(rx_packet);
        msg[2] = the_same_flag;
        memcpy(&msg[3], &WBCP_SRC_MAC(rx_packet), MAC_ADDR_SIZE);
        memcpy(&msg[5], &WBCP_EVENT_DATA(rx_packet), WBCP_EVENT_DATA_SIZE);

        os_taskq_post_type("wbcp_event_data_handler_task", NEW_EVENT_MSG, ARRAY_SIZE(msg), msg);
    }
}


static void wbcp_audio_data_handler(void *priv)
{
    static u32 seq_last = 100;
    u8 the_same_seq = 0;
    s32 msg[16] = {0};

    wbcp_rx_packet_type *rx_packet = (wbcp_rx_packet_type *)priv;

    if (WBCP_SEQ(rx_packet) != seq_last) {
        seq_last = WBCP_SEQ(rx_packet);
        the_same_seq = 0;
    } else {
        the_same_seq = 1;
    }

    msg[0] = WBCP_IDX(rx_packet);
    msg[1] = WBCP_ACK(rx_packet);
    msg[2] = WBCP_SEQ(rx_packet);
    memcpy(&msg[3], &WBCP_SRC_MAC(rx_packet), MAC_ADDR_SIZE);
    os_taskq_post_type("wbcp_event_data_handler_task", RECV_AUDIO_MSG, ARRAY_SIZE(msg), msg);
    if (!the_same_seq) {
        wbcp_audio_data_user_handler(&WBCP_AUDIO_DATA(rx_packet));
    }
}


static void wbcp_null_data_handler(void *rxwi)
{
    s8 pwr = 0, evm = 0;
    s32 freq = 0, freq_temp = 0;
    PRXWI_STRUC pRxWI = (PRXWI_STRUC *)rxwi;

    if (wbcp.enter && stat_prop.start) {
        if ((pRxWI->PHYMODE == conn_params.test_phymode) && (pRxWI->MCS == conn_params.test_mcs)) {
            putchar('R');
            pwr = (pRxWI->RSSI0);
            evm = (pRxWI->SNR1);
            freq_temp = (bbp_rd(62) << 8) | bbp_rd(61);
            freq = (freq_temp * 20 * 1000000) / 16 / 2 / 512 / 1000;

            stat_prop.num++;
            stat_prop.pwr_sum  += pwr;
            stat_prop.evm_sum  += evm;
            stat_prop.freq_sum += freq;
            stat_prop.stat_res.pcnt = stat_prop.num;
            stat_prop.stat_res.pwr  = stat_prop.pwr_sum  / stat_prop.num;
            stat_prop.stat_res.evm  = stat_prop.evm_sum  / stat_prop.num;
            stat_prop.stat_res.freq = stat_prop.freq_sum / stat_prop.num;

            /* log_info("num = %d, pwr = %d, evm = %d, freq = %d.\n", stat_prop.num, pwr, evm, freq); */
            if (stat_prop.num > stat_prop.count && stat_prop.count) {
                log_info("result : pcnt = %d, pwr = %d, evm = %d, freq = %d.\n", stat_prop.stat_res.pcnt, stat_prop.stat_res.pwr, stat_prop.stat_res.evm, stat_prop.stat_res.freq);
                os_sem_post(&stat_prop.stat_sem);
            }
        }
    }
}


static void wbcp_rx_packet_handler(void *rxwi, void *priv)
{
    static u32 idx_last = 100;
    u8 the_same_flag = 0;
    wbcp_rx_packet_type *rx_packet = (wbcp_rx_packet_type *)priv;
    wbcp_tx_packet_type *tx_packet = &wbcp.tx_packet;

    if (wbcp.assist_flag && WBCP_TYPE(rx_packet) != NULL_DATA_TYPE) {
        return;
    }

    switch (WBCP_TYPE(rx_packet)) {
    case EVENT_DATA_TYPE :
        /* log_info("EVENT_DATA_TYPE.\n"); */
        wbcp_event_data_handler(rx_packet);
        break;

    case RESP_DATA_TYPE:
        if (WBCP_IDX(rx_packet) != idx_last) {
            idx_last = WBCP_IDX(rx_packet);
            the_same_flag = 0;
        } else {
            the_same_flag = 1;
        }
        if (WBCP_IDX(rx_packet) == WBCP_IDX(tx_packet) && !the_same_flag) {
            log_info("RESP_DATA_TYPE : idx = %d, tx_idx = %d.\n", WBCP_IDX(rx_packet), WBCP_IDX(tx_packet));
            os_sem_post(&wbcp.tx_sem);
        }
        break;

    case AUDIO_DATA_TYPE:
        log_info("AUDIO_DATA_TYPE.\n");
        if (!wbcp_audio_data_user_handler) {
            log_info("wbcp_audio_data_user_handler no regist.");
            break;
        }
        wbcp_audio_data_handler(rx_packet);
        break;

    case NULL_DATA_TYPE:
        wbcp_null_data_handler(rxwi);
        break;

    default :
        break;
    }
}


void wbcp_rx_frame_cb(void *rxwi, void *header, void *data, u32 len, void *reserve)
{
    wbcp_rx_packet_type *rx_packet = (wbcp_rx_packet_type *)data;
    u8 *str = NULL;
    PRXWI_STRUC pRxWI = (PRXWI_STRUC *)rxwi;
    s8 rssi0 = pRxWI->RSSI0;
    struct wifi_mode_info mode_info = {0};

    if (wbcp.shield) {
        return;
    }

    switch (wbcp.mode) {
    case WB_SERVER_MODE:
        if (wbcp.build_conn && memcmp(WBCP_DEST_MAC(rx_packet), LOCAL_MAC, MAC_ADDR_SIZE) == 0) {
            if (!wbcp.enter || (wbcp.enter && (memcmp(WBCP_SRC_MAC(rx_packet), REMOTE_MAC, MAC_ADDR_SIZE) == 0))) {
                if ((WBCP_CRC(rx_packet) == WBCP_PACKET_CRC_CAL(rx_packet)) && (WBCP_VERSION(rx_packet) == WBCP_VERSION_ID)) {
                    wbcp_rx_packet_handler(rxwi, rx_packet);
                }
            }
        }
        break;

    case WB_CLIENT_MODE:
        wifi_get_mode_cur_info(&mode_info);
        if (mode_info.mode != SMP_CFG_MODE) {
            return;
        }

        if (!wbcp.respond_flag) {
            if (strcmp(&WBCP_CONN_SER(rx_packet), WBCP_CONN_SERIAL) == 0) {
                if (WBCP_CRC(rx_packet) == WBCP_CONN_PARAMS_CRC_CAL(rx_packet)) {
                    memcpy(REMOTE_MAC, &WBCP_SRC_MAC(rx_packet), MAC_ADDR_SIZE);
                    memcpy(&conn_params, &WBCP_CONN_PARAMS(rx_packet), WBCP_CONN_PARAMS_SIZE);
                    conn_params_show(&conn_params);
                    if (rssi0 < conn_params.rssi_thr) {
                        log_err("rssi too low, rssi(%d) < rssi_thr(%d)", rssi0, conn_params.rssi_thr);
                        return;
                    }

                    log_info("REMOTE_MAC : %02X-%02X-%02X-%02X-%02X-%02X.\n", REMOTE_MAC[0], REMOTE_MAC[1], REMOTE_MAC[2], \
                             REMOTE_MAC[3], REMOTE_MAC[4], REMOTE_MAC[5]);
                    int wifibox_client_init(void);
                    wifibox_client_init();
                    wbcp.respond_flag = 1;
                    os_sem_post(&wbcp.respond_sem);
                }
            }
        } else {
            if (memcmp(WBCP_DEST_MAC(rx_packet), LOCAL_MAC, MAC_ADDR_SIZE) == 0) {
                if (memcmp(WBCP_SRC_MAC(rx_packet), REMOTE_MAC, MAC_ADDR_SIZE) == 0) {
                    if ((WBCP_CRC(rx_packet) == WBCP_PACKET_CRC_CAL(rx_packet)) && \
                        (WBCP_VERSION(rx_packet) == WBCP_VERSION_ID)) {
                        wbcp_rx_packet_handler(rxwi, rx_packet);
                    }
                }
            }
        }
        break;

    default:
        break;
    }
}


static s32 wbcp_update_event_to_user_handler(u8 type, s32 msg_size, s32 *msg)
{
    s32 ret = 0;
    u8 retry_cnt = 0;

    do {
        ret = os_taskq_post_type(user_handler_task, type, msg_size, msg);
        if (ret != OS_NO_ERR) {
            retry_cnt++;
            os_time_dly(1);
        }
    } while (ret != OS_NO_ERR && retry_cnt < 10);

    if (ret != OS_NO_ERR) {
        log_err("wbcp_update_event_to_user_handler err = %d.", ret);
    }

    return ret;
}


static void wbcp_event_data_handler_task(void *priv)
{
    int msg[32] = {0};
    u32 idx = 0;
    u8 the_same_flag = 0, ack_flag = 0, seq = 0;
    u8 *src_mac = NULL;
    event_data_type *event_data = NULL;
    event_data_type new_event = {0};
    static wbcp_payload_type ack_payload = {0};

    for (;;) {
        if (os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }

        if (msg[0] == NEW_EVENT_MSG) {
            idx = msg[1];
            ack_flag = msg[2];
            the_same_flag = msg[3];
            src_mac = (u8 *)&msg[4];
            event_data = (event_data_type *)&msg[6];
            log_info("ack : event %d", event_data->event);
        } else if (msg[0] == RECV_AUDIO_MSG) {
            idx = msg[1];
            ack_flag = msg[2];
            seq = msg[3];
            src_mac = (u8 *)&msg[4];
            log_info("ack : seq %d", seq);
        } else {
            continue;
        }

        if (ack_flag) {
            ack_payload.version = WBCP_VERSION_ID;
            ack_payload.type = RESP_DATA_TYPE;
            ack_payload.ack = 0;
            ack_payload.seq = 0;
            ack_payload.idx = idx;
            wbcp_send_ack(&ack_payload, src_mac, conn_params.conn_mcs, conn_params.conn_phymode);
        }

        if (msg[0] == RECV_AUDIO_MSG) {
            continue;
        }

        /* log_info("handler task : event = %d, idx = %d, the_same_flag = %d.\n", event_data->event, idx, the_same_flag); */
        switch (wbcp.mode) {
        case WB_SERVER_MODE:
            if (!wbcp.enter) {
                if (event_data->event == ENTER_TEST_EVENT && !wbcp.respond_flag) {
                    wbcp_update_seach_status(STOP_SEARCH_MSG, 0);
                    wbcp.respond_flag = 1;
                    new_event.event = ENTER_TEST_EVENT;
                    memcpy(REMOTE_MAC, src_mac, MAC_ADDR_SIZE);
                    log_info("REMOTE_MAC : %02X-%02X-%02X-%02X-%02X-%02X.\n", REMOTE_MAC[0], REMOTE_MAC[1], REMOTE_MAC[2], \
                             REMOTE_MAC[3], REMOTE_MAC[4], REMOTE_MAC[5]);

                    wifi_set_channel(conn_params.channel);
                    log_info("WB_CH[%d]", conn_params.channel);
                    if (wbcp_send_event_data(&new_event, REMOTE_MAC, 1) == 0) {
                        wbcp.enter = 1;
                        msg[0] = (s32)&REMOTE_MAC[0];
                        if (is_cal_client_mac()) {
                            memcpy(&msg[1], &event_data->params.cal_params, sizeof(calibrate_params_type));
                            wbcp_update_event_to_user_handler(ENTER_TEST_SUCC_MSG, (sizeof(calibrate_params_type) / sizeof(s32)) + 1, msg);
                        } else {
                            wbcp_update_event_to_user_handler(ENTER_TEST_SUCC_MSG, 1, msg);
                        }
                    } else {
                        wbcp_update_event_to_user_handler(ENTER_TEST_FAIL_MSG, 1, msg);
                    }
                }
            } else {
                if (!the_same_flag && event_data->event != ENTER_TEST_EVENT) {
                    wbcp_update_event_to_user_handler(NEW_EVENT_MSG, WBCP_EVENT_DATA_SIZE / sizeof(s32), (s32 *)event_data);
                }
            }
            break;

        case WB_CLIENT_MODE:
            if (!wbcp.enter) {
                if (event_data->event == ENTER_TEST_EVENT) {
                    wbcp.enter = 1;
                    msg[0] = (s32)&REMOTE_MAC[0];
                    msg[1] = (s32)&conn_params;
                    wbcp_update_event_to_user_handler(ENTER_TEST_SUCC_MSG, 2, msg);
                }
            } else {
                if (!the_same_flag && event_data->event != ENTER_TEST_EVENT) {
                    wbcp_update_event_to_user_handler(NEW_EVENT_MSG, WBCP_EVENT_DATA_SIZE / sizeof(s32), (s32 *)event_data);
                }
            }
            break;

        default:
            break;
        }
    }
}


static u32 wbcp_send_packet_with_noack(wbcp_payload_type *payload, u8 *dest_mac, u8 mcs, u8 phymode)
{
    memcpy(wbcp.tx_packet.dest_mac, dest_mac, MAC_ADDR_SIZE);
    memcpy(&wbcp.tx_packet.payload, payload, WBCP_PAYLOAD_SIZE);
    wbcp.tx_packet.crc = CRC16(payload, WBCP_PAYLOAD_SIZE);

    return wifi_send_packet(&wbcp.tx_packet, WBCP_TX_PACKET_SIZE, mcs, phymode);
}


static void wbcp_send_packet_with_ack_task(void *priv)
{
    s32 msg[8] = {0};
    wbcp_payload_type *payload = NULL;
    u8 *dest_mac = NULL;
    u8 mcs = 0, phymode = 0;
    u8 retry_cnt = 0;

    for (;;) {
        if (os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }

        if (msg[0] != SEND_PACKET_MSG) {
            continue;
        }

        mcs = msg[1];
        phymode = msg[2];
        retry_cnt = 0;

        while (wbcp.send_withack_flag) {
            wifi_send_packet(&wbcp.tx_packet, WBCP_TX_PACKET_SIZE, mcs, phymode);
            log_info("retry_cnt = %d.", ++retry_cnt);
            os_time_dly(WBCP_SEND_WITHACK_RETRY_PERIOD);
        }
    }
}


static u32 wbcp_send_packet_with_ack(wbcp_payload_type *payload, u8 *dest_mac, u8 mcs, u8 phymode)
{
    u32 ret = 0;
    s32 msg[4] = {0};

    msg[0] = mcs;
    msg[1] = phymode;

    memcpy(wbcp.tx_packet.dest_mac, dest_mac, MAC_ADDR_SIZE);
    memcpy(&wbcp.tx_packet.payload, payload, WBCP_PAYLOAD_SIZE);
    wbcp.tx_packet.crc = CRC16(payload, WBCP_PAYLOAD_SIZE);

    if (os_taskq_post_type("wbcp_send_packet_with_ack_task", SEND_PACKET_MSG, ARRAY_SIZE(msg), msg) != OS_NO_ERR) {
        return 0;
    }

    wbcp.send_withack_flag = 1;
    os_sem_set(&wbcp.tx_sem, 0);
    ret = os_sem_pend(&wbcp.tx_sem, WBCP_WAIT_ACK_TIMEOUT);
    wbcp.send_withack_flag = 0;

    ret = (ret == OS_NO_ERR) ? WBCP_TX_PACKET_SIZE : 0;

    return ret;
}


static u32 wbcp_send_packet(wbcp_payload_type *payload, u8 *dest_mac, u8 mcs, u8 phymode, u8 ack)
{
    static u32 global_idx = 0;
    u32 ret = 0;

    global_idx++;
    payload->idx = global_idx;
    /* log_info("global_idx = %d.", global_idx); */

    if (ack) {
        ret = wbcp_send_packet_with_ack(payload, dest_mac, mcs, phymode);
    } else {
        ret = wbcp_send_packet_with_noack(payload, dest_mac, mcs, phymode);
    }

    return ret;
}


static void wbcp_send_ack(wbcp_payload_type *payload, u8 *dest_mac, u8 mcs, u8 phymode)
{
    /* os_mutex_pend(&wbcp.ack_mutex, 0); */

    memcpy(wbcp.ack_packet.dest_mac, dest_mac, MAC_ADDR_SIZE);
    memcpy(&wbcp.ack_packet.payload, payload, WBCP_PAYLOAD_SIZE);
    wbcp.ack_packet.crc = CRC16(payload, WBCP_PAYLOAD_SIZE);
    wifi_send_packet(&wbcp.ack_packet, WBCP_TX_PACKET_SIZE, mcs, phymode);

    /* os_mutex_post(&wbcp.ack_mutex); */
}


static s8 wbcp_send_event_data(event_data_type *event_data, u8 *dest_mac, u8 ack)
{
    u32 len = 0;
    wbcp_payload_type payload = {0};

    if (wbcp.assist_flag) {
        len = assist_tx_data(event_data);
        return ((len == sizeof(event_data_type)) ? 0 : -1);
    }

    LOCK_TX_PACKET();

    payload.version = WBCP_VERSION_ID;
    payload.type = EVENT_DATA_TYPE;
    payload.ack = ack;
    payload.seq = 0;
    payload.idx = 0;
    memcpy(&payload.wbcp_data.event_data, event_data, WBCP_EVENT_DATA_SIZE);
    len = wbcp_send_packet(&payload, dest_mac, conn_params.conn_mcs, conn_params.conn_phymode, ack);

    UNLOCK_TX_PACKET();

    return ((len == WBCP_TX_PACKET_SIZE) ? 0 : -1);
}


static s8 wbcp_send_audio_data(audio_data_type *audio_data, u8 *dest_mac, u8 seq)
{
    u32 len = 0;
    wbcp_payload_type payload = {0};

    LOCK_TX_PACKET();

    payload.version = WBCP_VERSION_ID;
    payload.type = AUDIO_DATA_TYPE;
    payload.ack = 1;
    payload.seq = seq;
    payload.idx = 0;
    memcpy(&payload.wbcp_data.audio_data, audio_data, WBCP_AUDIO_DATA_SIZE);
    len = wbcp_send_packet(&payload, dest_mac, conn_params.conn_mcs, conn_params.conn_phymode, 1);

    UNLOCK_TX_PACKET();

    return ((len == WBCP_TX_PACKET_SIZE) ? 0 : -1);
}


static s8 wbcp_send_null_data(u8 *dest_mac)
{
    u32 len = 0;
    wbcp_payload_type payload = {0};

    LOCK_TX_PACKET();

    payload.version = WBCP_VERSION_ID;
    payload.type = NULL_DATA_TYPE;
    payload.ack = 0;
    payload.seq = 0;
    payload.idx = 0;

    len = wbcp_send_packet(&payload, dest_mac, conn_params.test_mcs, conn_params.test_phymode, 0);

    UNLOCK_TX_PACKET();

    return ((len == WBCP_TX_PACKET_SIZE) ? 0 : -1);
}


static void wbcp_start_send_test_data_task(void *priv)
{
    s32 msg[16];
    u32 num = 0;
    u8 *dest_mac = NULL;
    u32 count = 0, periodic = 0;

    for (;;) {
        if (os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }

        if (msg[0] != START_SEND_MSG) {
            continue;
        }

        dest_mac = (u8 *)msg[1];
        periodic = msg[2];
        count = msg[3];
        num = 0;

        while (stat_prop.send_test_flag) {
            wbcp_send_null_data(dest_mac);
            putchar('T');
            if (++num > count && count) {
                os_sem_post(&stat_prop.test_sem);
                stat_prop.send_test_flag = 0;
            }
            os_time_dly(periodic);
        }
    }
}


static s32 wbcp_start_send_test_data(u8 *dest_mac, u32 periodic, u32 count)
{
    s32 msg[8] = {0};
    s32 ret = 0;

    msg[0] = (s32)dest_mac;
    msg[1] = periodic;
    msg[2] = count;

    stat_prop.send_test_flag = 1;
    ret = os_taskq_post_type("wbcp_start_send_test_data_task", START_SEND_MSG, ARRAY_SIZE(msg), msg);

    if (ret == OS_NO_ERR) {
        if (count) {
            os_sem_set(&stat_prop.test_sem, 0);
            os_sem_pend(&stat_prop.test_sem, 0);
            return 0;
        }
        return 0;
    }

    return -1;
}


static void wbcp_stop_send_test_data(void)
{
    stat_prop.send_test_flag = 0;
}


static void wbcp_regist_user_handler(void *priv)
{
    strcpy(user_handler_task, priv);
}


static void wbcp_regist_audio_data_user_handler(void *priv)
{
    wbcp_audio_data_user_handler = priv;
}


static void wbcp_launch_conn_req_task(void *priv)
{
    s32 msg[4] = {0};
    u8 ch = 0;
    u8 flag = 0;

    /* LOCK_TX_PACKET(); */
    /* memcpy(&wbcp.tx_packet.conn_params, &conn_params, WBCP_CONN_PARAMS_SIZE); */
    /* wbcp.tx_packet.crc = CRC16(&wbcp.tx_packet.conn_params, WBCP_CONN_PARAMS_SIZE); */

    /* printf("TEST_TEST, START_SEARCH_MSG\n"); */
    /* msg[0] = conn_params.channel; */
    /* wbcp_update_event_to_user_handler(START_SEARCH_MSG, 1, &msg[0]); */

    for (;;) {
        if (os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }

        if (msg[0] != UPDATE_CH_MSG && msg[0] != START_SEARCH_MSG && msg[0] != STOP_SEARCH_MSG) {
            continue;
        }

        if (msg[0] == STOP_SEARCH_MSG) {
            log_info("TEST_TEST, STOP_SEARCH_MSG");
            flag = 0;
            /* goto _exit_serch_;  */
        } else if (msg[0] == START_SEARCH_MSG) {
            flag = 1;
        }

        if (flag) {
            ch = msg[1];
            wifi_set_channel(ch);
            log_info("WB_CH[%d]", ch);

            for (u8 j = 0; j < 5; j++) {
                wifi_send_packet(&wbcp.tx_packet, WBCP_TX_PACKET_SIZE, conn_params.conn_mcs, conn_params.conn_phymode);
                os_time_dly(1);
            }

            msg[0] = ch;
            wbcp_update_event_to_user_handler(UPDATE_CH_MSG, 1, &msg[0]);
        }
    }

    /* _exit_serch_: */
    /* UNLOCK_TX_PACKET(); */
}


static s32 wbcp_update_seach_status(u8 type, u8 ch)
{
    s32 ret = 0, msg = 0;
    u8 retry_cnt = 0;

    msg = ch;

    do {
        ret = os_taskq_post_type("wbcp_launch_conn_req_task", type, 1, &msg);
        if (ret != OS_NO_ERR) {
            retry_cnt++;
            os_time_dly(1);
        }
    } while (ret != OS_NO_ERR && retry_cnt < 10);

    if (ret != OS_NO_ERR) {
        log_err("wbcp_update_seach_status err = %d.", ret);
    }

    return ret;
}


static void wbcp_respond_conn_req_task(void)
{
    event_data_type event_data = {0};
    event_data.event = ENTER_TEST_EVENT;
    s8 respond_cnt = 0;
    s32 msg[2];

#ifdef WBCP_CAL_DUT
    memcpy(&event_data.params.cal_params, get_wifibox_cal_params(), sizeof(calibrate_params_type));
#endif

_next_:
    respond_cnt = 0;
    for (;;) {
        os_sem_set(&wbcp.respond_sem, 0);
        os_sem_pend(&wbcp.respond_sem, 0);

        if (wbcp.assist_flag) {
            wbcp.enter = 1;
            wbcp.respond_flag = 1;
            msg[0] = (s32)&REMOTE_MAC[0];
            msg[1] = (s32)&conn_params;
            wbcp_update_event_to_user_handler(ENTER_TEST_SUCC_MSG, 2, msg);

            wifi_init();
            wifi_set_channel(conn_params.channel);
            log_info("WB_CH[%d]", conn_params.channel);

            event_data.event = ENTER_TEST_EVENT;
            memcpy(event_data.params.conn_params.mac, LOCAL_MAC, MAC_ADDR_SIZE);
            if (!assist_tx_data(&event_data)) {
                msg[0] = conn_params.dut_reset_flag;
                wbcp_update_event_to_user_handler(ENTER_TEST_FAIL_MSG, 1, &msg[0]);
            }
            goto _next_;
        }

        wifi_init();
        wifi_set_channel(conn_params.channel);
        log_info("WB_CH[%d]", conn_params.channel);

        for (;;) {
            if (++respond_cnt < MAX_RESPOND_CNT) {
                wbcp_send_event_data(&event_data, REMOTE_MAC, 0);
                log_info("respond_cnt = %d.", respond_cnt);
                putchar('>');
            } else {
                msg[0] = conn_params.dut_reset_flag;
                wbcp_update_event_to_user_handler(ENTER_TEST_FAIL_MSG, 1, &msg[0]);
                goto _next_;
            }

            if (wbcp.enter) {
                goto _next_;
                /* return; */
            }
            os_time_dly(5);
        }
        os_time_dly(1);
    }
}


static s8 wbcp_build_conn(void *params)
{
    s32 msg = 0;
    event_data_type event_data = {0};

    if (wbcp.enter) {
        return -1;
    }
    wbcp.exit = 0;
    wbcp.build_conn = 1;

    memcpy(&conn_params, params, WBCP_CONN_PARAMS_SIZE);
    conn_params_show(&conn_params);

    wifi_init();

    if (wbcp.assist_flag) {
        event_data.event = ENTER_TEST_EVENT;
        memcpy(&event_data.params.conn_params, params, WBCP_CONN_PARAMS_SIZE);
        memcpy(event_data.params.conn_params.mac, LOCAL_MAC, MAC_ADDR_SIZE);
        if (!assist_tx_data(&event_data)) {
            wbcp_update_event_to_user_handler(ENTER_TEST_FAIL_MSG, 1, &msg);
        }
        return 0;
    }

    LOCK_TX_PACKET();
    memcpy(&wbcp.tx_packet.conn_params, &conn_params, WBCP_CONN_PARAMS_SIZE);
    wbcp.tx_packet.crc = CRC16(&wbcp.tx_packet.conn_params, WBCP_CONN_PARAMS_SIZE);
    UNLOCK_TX_PACKET();

    msg = conn_params.channel;
    wbcp_update_event_to_user_handler(START_SEARCH_MSG, 1, &msg);
    wbcp_update_seach_status(START_SEARCH_MSG, msg);

    /* if (thread_fork("wbcp_launch_conn_req_task", 12, 512, 512, NULL, wbcp_launch_conn_req_task, NULL) != OS_NO_ERR) { */
    /*     log_err("thread fork fail : %s, %d.\n", __FILE__, __LINE__); */
    /*     return -1; */
    /* } */

    return 0;
}


static void wbcp_break_conn(void)
{
    wbcp.build_conn = 0;
    wbcp.enter = 0;
    wbcp.exit = 1;
    wbcp.respond_flag = 0;
    memset(REMOTE_MAC, 0, MAC_ADDR_SIZE);

    if (wbcp.mode == WB_SERVER_MODE && !wbcp.assist_flag) {
        wbcp_update_seach_status(STOP_SEARCH_MSG, 0);
    } else if (wbcp.mode == WB_CLIENT_MODE) {
        wifi_enter_smp_mode();
    }
}


static void wbcp_rx_shield(void)
{
    wbcp.shield = 1;
}


static void *wbcp_start_stat_prop(u32 count, u32 timeout)
{
    s32 ret = 0;

    stat_prop.count = count;
    stat_prop.num = 0;
    stat_prop.pwr_sum = 0;
    stat_prop.evm_sum = 0;
    stat_prop.freq_sum = 0;
    memset(&stat_prop.stat_res, 0, sizeof(stat_res_type));
    stat_prop.start = 1;

    if (stat_prop.count) {
        os_sem_set(&stat_prop.stat_sem, 0);
        if (os_sem_pend(&stat_prop.stat_sem, timeout) == OS_NO_ERR) {
            stat_prop.start = 0;
            return &stat_prop.stat_res;
        }
    }

    return NULL;
}


static void *wbcp_stop_stat_prop(void)
{
    stat_prop.start = 0;
    return &stat_prop.stat_res;
}


static s8 wbcp_init(u8 mode, u8 is_assist)
{
    u8 none_data_1[30] = {
        0x14, 0x06, 0x00, 0x04, 0xB0, 0x00, 0x04, 0x80, 0x35, 0x01, 0x02, 0x46, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x88, 0x81, 0x00, 0x00, 0x44, 0x66, 0x66, 0x66, 0x66, 0x66
    };

    u8 none_data_2[46] = {
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x80, 0x00, 0x00, 0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x45, 0x00, 0x05, 0xDC,
        0x00, 0x0B, 0x20, 0x00, 0xFF, 0x11, 0xD5, 0xC8, 0xC0, 0xA8, 0x1F, 0xD3, 0xff, 0xff, 0xff, 0xff,
        0x13, 0x89, 0xA4, 0xFF, 0x20, 0x08, 0x67, 0x37
    };

    u8 none_data_3[] = {
        0xc6, 0x00, 0x00, 0x04, 0xB0, 0x00, 0x04, 0x80, 0x35, 0x01, 0xB6, 0x40, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x88, 0x02, 0x00, 0x00, 0x0C, 0x62, 0x0E, 0xD4, 0x46, 0x3E, 0x24, 0x8B,
        0xA7, 0x24, 0x63, 0x17, 0x24, 0x8B, 0xA7, 0x24, 0x63, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08, 0x00, 0x45, 0x00, 0x05, 0xDB, 0x00, 0x02, 0x00, 0x00,
        0xFF, 0x01, 0x32, 0xCC, 0xC0, 0xA8, 0x01, 0x01, 0xC0, 0xA8, 0x01, 0x02, 0x08, 0x00, 0x53, 0x75,
        0xAF, 0xAF, 0x70, 0x37
    };

    wbcp.mode = mode;
    wifi_get_mac(LOCAL_MAC);
    log_info("LOCAL_MAC : %02X-%02X-%02X-%02X-%02X-%02X.\n", LOCAL_MAC[0], LOCAL_MAC[1], LOCAL_MAC[2], LOCAL_MAC[3], LOCAL_MAC[4], LOCAL_MAC[5]);

#ifdef WBCP_CAL_DUT
    if (wbcp.mode == WB_CLIENT_MODE) {
        LOCAL_MAC[0] = 0xab;
        LOCAL_MAC[1] = 0xab;
    }
#endif

    /* memcpy(wbcp.tx_packet.none_data_1, none_data_1, sizeof(none_data_1)); */
    /* memcpy(wbcp.tx_packet.none_data_2, none_data_2, sizeof(none_data_2)); */
    u16 *pkg_len = &none_data_3[0];
    pkg_len = sizeof(wbcp_tx_packet_type) + 4 - 8;
    memcpy(&wbcp.tx_packet, none_data_3, sizeof(none_data_3));
    memcpy(wbcp.tx_packet.src_mac, LOCAL_MAC, MAC_ADDR_SIZE);

    memcpy(&wbcp.ack_packet, &wbcp.tx_packet, WBCP_TX_PACKET_SIZE);

    os_mutex_create(&wbcp.tx_mutex);
    os_mutex_create(&wbcp.ack_mutex);
    os_sem_create(&wbcp.tx_sem, 0);
    os_sem_create(&wbcp.respond_sem, 0);
    os_sem_create(&stat_prop.stat_sem, 0);
    os_sem_create(&stat_prop.test_sem, 0);

    if (thread_fork("wbcp_event_data_handler_task", 15, 1024, 2 * 1024, NULL, wbcp_event_data_handler_task, NULL) != OS_NO_ERR) {
        log_err("thread fork fail : %s, %d.\n", __FILE__, __LINE__);
        return -1;
    }

    if (thread_fork("wbcp_send_packet_with_ack_task", 12, 1024, 512, NULL, wbcp_send_packet_with_ack_task, NULL) != OS_NO_ERR) {
        log_err("thread fork fail : %s, %d.\n", __FILE__, __LINE__);
        return -1;
    }

    if (thread_fork("wbcp_start_send_test_data_task", 12, 1024, 512, NULL, wbcp_start_send_test_data_task, NULL) != OS_NO_ERR) {
        log_err("thread fork fail : %s, %d.\n", __FILE__, __LINE__);
        return -1;
    }

    if (wbcp.mode == WB_SERVER_MODE) {
        if (thread_fork("wbcp_launch_conn_req_task", 12, 512, 512, NULL, wbcp_launch_conn_req_task, NULL) != OS_NO_ERR) {
            log_err("thread fork fail : %s, %d.\n", __FILE__, __LINE__);
            return -1;
        }
    } else if (wbcp.mode == WB_CLIENT_MODE) {
        if (thread_fork("wbcp_respond_conn_req_task", 12, 2 * 1024, 0, NULL, wbcp_respond_conn_req_task, NULL) != OS_NO_ERR) {
            log_err("thread fork fail : %s, %d.\n", __FILE__, __LINE__);
            return -1;
        }
    }

    wbcp.assist_flag = is_assist;
    if (wbcp.assist_flag) {
        assist_dev_init();
    }
    return 0;
}


static const wbcp_operations_interface_type wbcp_ops = {
    .init = wbcp_init,
    .update_seach_status        = wbcp_update_seach_status,
    .build_conn                 = wbcp_build_conn,
    .break_conn                 = wbcp_break_conn,
    .rx_shield                  = wbcp_rx_shield,
    .send_event_data            = wbcp_send_event_data,
    .send_audio_data            = wbcp_send_audio_data,
    .start_send_test_data       = wbcp_start_send_test_data,
    .stop_send_test_data        = wbcp_stop_send_test_data,
    .regist_usr_handler         = wbcp_regist_user_handler,
    .regist_audio_data_user_handler  = wbcp_regist_audio_data_user_handler,
    .start_stat_prop            = wbcp_start_stat_prop,
    .stop_stat_prop             = wbcp_stop_stat_prop,
};


void get_wbcp_operations_interface(wbcp_operations_interface_type **ops)
{
    *ops = &wbcp_ops;
}


u8 get_wbcp_connect_status(void)
{
    return wbcp.enter;
}


static void conn_params_show(void *priv)
{
    wbcp_conn_params_type *conn_params = (wbcp_conn_params_type *)priv;
    log_info("mode           = %d", conn_params->mode);
    log_info("channel        = %d", conn_params->channel);
    log_info("conn_mcs       = %d", conn_params->conn_mcs);
    log_info("conn_phymode   = %d", conn_params->conn_phymode);
    log_info("test_mcs       = %d", conn_params->test_mcs);
    log_info("test_phymode   = %d", conn_params->test_phymode);
    log_info("rssi_thr       = %d", conn_params->rssi_thr);
    log_info("dut_reset_flag = %d", conn_params->dut_reset_flag);
    log_info("thradj_fsm     = %d, %d",
             conn_params->fsm_timeout.thradj_fsm[0], \
             conn_params->fsm_timeout.thradj_fsm[1]);
    log_info("test_fsm       = %d, %d, %d, %d", \
             conn_params->fsm_timeout.test_fsm[0], \
             conn_params->fsm_timeout.test_fsm[1], \
             conn_params->fsm_timeout.test_fsm[2], \
             conn_params->fsm_timeout.test_fsm[3]);
    log_info("test_option: pwr= %d, sen = %d rate = %d, freq = %d ", \
             conn_params->test_option.wifi_pwr_option, \
             conn_params->test_option.wifi_sen_option, \
             conn_params->test_option.wifi_rx_rate_option, \
             conn_params->test_option.wifi_freq_option);
}


void set_wbcp_mode(u8 mode)
{
    wbcp.mode = mode;
}


#endif



