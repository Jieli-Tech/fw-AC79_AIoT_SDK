#include "asm/gpio.h"
#include "device/uart.h"
#include "list.h"
#include "datatype.h"
#include "rf_fcc_main.h"
#include "wifi/wifi_connect.h"
#include "btcontroller_modules.h"
#include "utils/syscfg/syscfg_id.h"
#include "app_config.h"
#include "sock_api/sock_api.h"
#include "gpcnt.h"
#include "gpio.h"


#ifdef RF_FCC_TEST_ENABLE

#if 1
#define     log_info(x, ...)     printf("[RF_FCC_TEST][INFO] " x " ", ## __VA_ARGS__)
#define     log_err(x, ...)      printf("[RF_FCC_TEST][ERR] " x " ", ## __VA_ARGS__)
#else
#define     log_info(...)
#define     log_err(...)
#endif

#define MAX_DGAIN   (128)
#define MIN_DGAIN   (0)
#define MAX_XOSC   (15)
#define MIN_XOSC   (0)
#define TXINFO_SIZE			  4
#define USB_DIR_OUT	          0
#define USB_DIR_IN	          0x80
#define WIFI_TXWI_HEAD_SIZE   20
#define WIFI_80211_FILL_SIZE  27 //保留一些字节不填充, 为了避开duration段和上层发送1个字节也支持
#define MAC_SYS_CTRL          0x1004		/*MAC_CSR1 */

#define TX_INR_IN_ADJ   (500)
#define TX_LEN_IN_ADJ   (512)

static u8 CUR_MODE = 0;
static u32 TOTAL_RX_CNT;
static struct FCC_TX tx_params;
static struct FCC_RX rx_params;

static u32 RAND_NUM;
static int tx_pid = 0;
static void *uart_hdl = NULL;
static u8 uart_circlebuf[1024];
static struct list_head head;
static u8 sign_ssid[33], sign_pwd[65];

static  const  struct  {
    const char *string;
    u8 phy;
    u8 mcs;
} tx_rate_tab[] = {
    {"1M",		0,		0},
    {"2M",		0,		1},
    {"5.5M",	0,		2},
    {"11M",		0,		3},

    {"6M",		1,		0},
    {"9M",		1,		1},
    {"12M",		1,		2},
    {"18M",		1,		3},
    {"24M",		1,		4},
    {"36M",		1,		5},
    {"48M",		1,		6},
    {"54M",		1,		7},

    {"HTMCS0",	2,		0},
    {"HTMCS1",	2,		1},
    {"HTMCS2",	2,		2},
    {"HTMCS3",	2,		3},
    {"HTMCS4",	2,		4},
    {"HTMCS5",	2,		5},
    {"HTMCS6",	2,		6},
    {"HTMCS7",	2,		7},
};


static u8 fcc_str[3][8] = {
    {"SUCC"},
    {"FAIL"},
    {"NULL"},
};

struct fcc_info {
    u8 res[8];
    u16 crc;
};


static u8 FCC_RSP_ARRAY[]       = {0x04, 0x0E, 0x01, 0x01, 0xA1, 0xA2, 0x00};
static u8 FCC_RSP_FAIL_ARRAY[]  = {0x04, 0x0E, 0x01, 0x01, 0xA1, 0xA2, 0x01};
static u8 FCC_READY_RSP_ARRAY[] = {0x04, 0x0E, 0x01, 0x01, 0xA1, 0xA2, 0x02};
static u8 FCC_HEART_RSP_ARRAY[] = {0x04, 0x0E, 0x04, 0x01, 0xA1, 0xA3, 0x00, 0x00, 0x00, 0x00};
static u8 FCC_WIFI_ARRAY[]      = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x01};
static u8 FCC_BT_ARRAY[]        = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x02};
static u8 FCC_RESET_ARRAY[]     = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x03};
static u8 FCC_RES_SUCC_ARRAY[]  = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x04};
static u8 FCC_RES_FAIL_ARRAY[]  = {0x01, 0xA1, 0xA2, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x05};
static u8 FCC_HEART_ARRAY[]     = {0x01, 0xA1, 0xA3, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x00};
static u8 FCC_READY_ARRAY[]     = {0x01, 0xA1, 0xA3, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x01};

__attribute__((aligned(4))) static u8 wifi_send_pkg[1564] = {
    0xc6, 0x00, 0x00, 0x04, 0xB0, 0x00, 0x04, 0x80, 0x35, 0x01, 0xB6, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*保留一些字节*/
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};


int wifi_get_mac(u8 *mac);
u32 sdio_mac_rreg(u32 reg);
void wf_tx_sine_test(void);
void wf_read_xosc(u8 *xosc);
void wf_write_xosc(u8 *xosc);
void wl_anl_actl_init(char init_sel);
void rf_fcc_save_change_mode(u8 mode);
void sdio_mac_wreg(u32 wr_addr, u32 dat);
void set_pa_config_data(u8 user_data[7]);
void wifi_set_channel(unsigned char set_freq);
u8 *fcc_common_data_deal(u8 packet_type, u8 *data, u32 len, u32 *rsp_len, u8 *reset);
void mp_test_pa_mcs_dgain_set(char mode, char mcs, unsigned char gain);
static void read_res(void);
static int udp_server_init(int port);


static u8 array_cmp(void *a_array, void *b_array, u32 size)
{
    int i;
    u8 *a = (u8 *)a_array;
    u8 *b = (u8 *)b_array;

    for (i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            break;
        }
    }
    return (i < size ? 1 : 0);
}


__attribute__((weak)) const char *rf_fcc_get_uart(void)
{
    return "uart1";
}


static u32 comm_dev_tx_data(u8 *buf, u32 len)
{
    return dev_write(uart_hdl, buf, len);
}


static void comm_dev_recv_task(void *priv)
{
    u8 *buf, *rsp = NULL, reset = 0;
    u32 rsp_len;
    u8 uart_data[256];
    int len, move_len, crc_check_len;
    struct fcc_data *fcc = NULL;
    struct host_data *host = NULL;

    for (;;) {
        move_len = 0;
        memset(uart_data, 0, sizeof(uart_data));
        len = dev_read(uart_hdl, uart_data, sizeof(uart_data));
        if (len < 0) {
            continue;
        }

        /* put_buf(uart_data, len); */
        rsp  = fcc_common_data_deal(uart_data[0], &uart_data[1], len - 1, &rsp_len, &reset);
        if (rsp) {
            comm_dev_tx_data(rsp, rsp_len);
        }

        if (reset) {
            cpu_reset();
        }

        if (!CUR_MODE) {
            log_err("no in fcc mode");
            continue;
        }

        while (move_len < len) {
            buf = (u8 *)uart_data + move_len;
            if (buf[0] == 'J' && buf[1] == 'L') {
                host = zalloc(sizeof(struct host_data));
                if (!host) {
                    goto _exit_deal_;
                }

                host->self = host;
                host->flag = 0;
                fcc = &host->fcc_data;

                memcpy((u8 *) & (fcc->mark),       buf,  sizeof(fcc->mark));
                memcpy((u8 *) & (fcc->opcode),     buf + sizeof(fcc->mark), sizeof(fcc->opcode));
                memcpy((u8 *) & (fcc->params_len), buf + sizeof(fcc->mark) + sizeof(fcc->opcode), sizeof(fcc->params_len));
                memcpy((u8 *) & (fcc->crc),        buf + sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len) + fcc->params_len, sizeof(fcc->crc));

                if (fcc->params_len) {
                    fcc->params = (u8 *)zalloc(fcc->params_len);
                    if (!fcc->params) {
                        goto _exit_deal_;
                    }
                    memcpy(fcc->params, buf + sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len), fcc->params_len);
                }


                crc_check_len = sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len) + fcc->params_len;
                if (fcc->crc != CRC16(buf, crc_check_len)) {
                    log_err("%s, crc16 error\n", __FUNCTION__);
                    goto _exit_deal_;
                }

                list_add_tail(&host->entry, &head);
                host = NULL;
                fcc = NULL;
                move_len += crc_check_len + sizeof(fcc->crc);
            } else {
                break;
            }
        }
_exit_deal_:
        if (host) {
            free(host);
            host = NULL;
        }
        if (fcc) {
            free(fcc);
            fcc = NULL;
        }
    }
}


static u8 comm_dev_init(void)
{
    uart_hdl = dev_open(rf_fcc_get_uart(), 0);
    if (!uart_hdl) {
        return -1;
    }

    dev_ioctl(uart_hdl, UART_SET_CIRCULAR_BUFF_ADDR, (u32)uart_circlebuf);
    dev_ioctl(uart_hdl, UART_SET_CIRCULAR_BUFF_LENTH, sizeof(uart_circlebuf));
    dev_ioctl(uart_hdl, UART_SET_RECV_BLOCK, 1);
    dev_ioctl(uart_hdl, UART_SET_RECV_TIMEOUT, 1000);
    dev_ioctl(uart_hdl, UART_START, 0);

    return 0;
}


static void wifi_tx_data(u8 *pkg, int len, u32 rate, u8 bw, u8 short_gi)// 最大包1513
{
    u16 *PktLen = &wifi_send_pkg[0];
    u16 *MPDUtotalByteCount = &wifi_send_pkg[10];
    *PktLen = WIFI_80211_FILL_SIZE + len + WIFI_TXWI_HEAD_SIZE + 4 - 8;
    *MPDUtotalByteCount = WIFI_80211_FILL_SIZE + len;

    if (pkg) {
        put_buf(pkg, len);
        memcpy(&wifi_send_pkg[WIFI_TXWI_HEAD_SIZE + WIFI_80211_FILL_SIZE], pkg, len);
    } else {
        memset(&wifi_send_pkg[WIFI_TXWI_HEAD_SIZE + WIFI_80211_FILL_SIZE], 0xaa, len);
    }

    struct urb urb;
    urb.pipe = USB_DIR_OUT;
    urb.complete = NULL;
    urb.transfer_buffer = wifi_send_pkg;
    urb.transfer_buffer_length = len + WIFI_TXWI_HEAD_SIZE + WIFI_80211_FILL_SIZE + 4;

    PTXWI_STRUC pTxWI = TXINFO_SIZE + (u8 *)urb.transfer_buffer;
    pTxWI->PHYMODE    = tx_rate_tab[rate].phy;
    pTxWI->MCS        = tx_rate_tab[rate].mcs;
    pTxWI->BW         = bw;
    pTxWI->ShortGI    = !short_gi;
    usb_submit_urb(&urb, 0);
}


static void wifi_rx_frame_cb(void *rxwi, void *header, void *data, u32 len, void *reserve)
{
    PRXWI_STRUC pRxWI = (PRXWI_STRUC *)rxwi;
    u8 *src_mac = (u8 *)data + 34;
    /* u8 *src_mac = (u8 *)data + 28; */

    /* putchar('A'); */
    /* if (pRxWI->BW == rx_params.bandwidth && \ */
    /*     pRxWI->ShortGI == rx_params.short_gi) { */
    /* putchar('B'); */
    /* put_buf((u8 *)data, len); */
    if (rx_params.filter_enable) {
        /* log_info("recv_mac:%x:%x:%x:%x:%x:%x\n", src_mac[0], src_mac[1], src_mac[2], src_mac[3], src_mac[4], src_mac[5]); */
        if (!array_cmp(src_mac, rx_params.filter_mac, sizeof(rx_params.filter_mac))) {
            /* putchar('C'); */
            TOTAL_RX_CNT++;
        }
    } else {
        TOTAL_RX_CNT++;
    }
    /* } */
}


static int wifi_event_callback(void *network_ctx, enum WIFI_EVENT event)
{
    static u8 init = 0;
    struct wifi_store_info wifi_default_mode_parm = {0};

    switch (event) {
    case WIFI_EVENT_MODULE_INIT:
        puts("|network_user_callback->WIFI_EVENT_MODULE_INIT\n");

        wifi_default_mode_parm.mode = init ? STA_MODE : MP_TEST_MODE;
        init = 1;

        if (wifi_default_mode_parm.mode <= AP_MODE) {
            strncpy((char *)wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE], sign_ssid, sizeof(wifi_default_mode_parm.ssid[wifi_default_mode_parm.mode - STA_MODE]) - 1);
            strncpy((char *)wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE], sign_pwd, sizeof(wifi_default_mode_parm.pwd[wifi_default_mode_parm.mode - STA_MODE]) - 1);
        }
        wifi_set_default_mode(&wifi_default_mode_parm, 1, wifi_default_mode_parm.mode == STA_MODE);
        break;

    case WIFI_EVENT_MODULE_START:
        puts("|network_user_callback->WIFI_EVENT_MODULE_START\n");

#if 0

        u32  tx_rate_control_tab = // 不需要哪个速率就删除掉,可以动态设定
            0
            | BIT(0) //0:CCK 1M
            | BIT(1) //1:CCK 2M
            | BIT(2) //2:CCK 5.5M
            | BIT(3) //3:OFDM 6M
            | BIT(4) //4:MCS0/7.2M
            | BIT(5) //5:OFDM 9M
            | BIT(6) //6:CCK 11M
            | BIT(7) //7:OFDM 12M
            | BIT(8) //8:MCS1/14.4M
            | BIT(9) //9:OFDM 18M
            | BIT(10) //10:MCS2/21.7M
            | BIT(11) //11:OFDM 24M
            | BIT(12) //12:MCS3/28.9M
            | BIT(13) //13:OFDM 36M
            | BIT(14) //14:MCS4/43.3M
            | BIT(15) //15:OFDM 48M
            | BIT(16) //16:OFDM 54M
            | BIT(17) //17:MCS5/57.8M
            | BIT(18) //18:MCS6/65.0M
            | BIT(19) //19:MCS7/72.2M
            ;
        wifi_set_tx_rate_control_tab(tx_rate_control_tab);
#endif
        break;

    case WIFI_EVENT_MODULE_STOP:
        puts("|network_user_callback->WIFI_EVENT_MODULE_STOP\n");
        break;

    case WIFI_EVENT_AP_START:
        printf("|network_user_callback->WIFI_EVENT_AP_START,CH=%d\n", wifi_get_channel());
        break;
    case WIFI_EVENT_AP_STOP:
        puts("|network_user_callback->WIFI_EVENT_AP_STOP\n");
        break;

    case WIFI_EVENT_STA_START:
        puts("|network_user_callback->WIFI_EVENT_STA_START\n");
        break;
    case WIFI_EVENT_MODULE_START_ERR:
        puts("|network_user_callback->WIFI_EVENT_MODULE_START_ERR\n");
        break;
    case WIFI_EVENT_STA_STOP:
        puts("|network_user_callback->WIFI_EVENT_STA_STOP\n");
        break;
    case WIFI_EVENT_STA_DISCONNECT:
        puts("|network_user_callback->WIFI_STA_DISCONNECT\n");
        break;
    case WIFI_EVENT_STA_SCAN_COMPLETED:
        puts("|network_user_callback->WIFI_STA_SCAN_COMPLETED\n");
        break;
    case WIFI_EVENT_STA_CONNECT_SUCC:
        printf("|network_user_callback->WIFI_STA_CONNECT_SUCC,CH=%d\r\n", wifi_get_channel());
        break;

    case WIFI_EVENT_MP_TEST_START:
        puts("|network_user_callback->WIFI_EVENT_MP_TEST_START\n");
        break;
    case WIFI_EVENT_MP_TEST_STOP:
        puts("|network_user_callback->WIFI_EVENT_MP_TEST_STOP\n");
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID:
        puts("|network_user_callback->WIFI_STA_CONNECT_TIMEOUT_NOT_FOUND_SSID\n");
        cpu_reset();
        break;

    case WIFI_EVENT_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL:
        puts("|network_user_callback->WIFI_STA_CONNECT_TIMEOUT_ASSOCIAT_FAIL .....\n");
        cpu_reset();
        break;

    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_SUCC:
        puts("|network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_SUCC\n");
        break;
    case WIFI_EVENT_STA_NETWORK_STACK_DHCP_TIMEOUT:
        puts("|network_user_callback->WIFI_EVENT_STA_NETWPRK_STACK_DHCP_TIMEOUT\n");
        break;

    case WIFI_EVENT_P2P_START:
        puts("|network_user_callback->WIFI_EVENT_P2P_START\n");
        break;
    case WIFI_EVENT_P2P_STOP:
        puts("|network_user_callback->WIFI_EVENT_P2P_STOP\n");
        break;
    case WIFI_EVENT_P2P_GC_DISCONNECTED:
        puts("|network_user_callback->WIFI_EVENT_P2P_GC_DISCONNECTED\n");
        break;
    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC:
        puts("|network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_SUCC\n");
        break;
    case WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT:
        puts("|network_user_callback->WIFI_EVENT_P2P_GC_NETWORK_STACK_DHCP_TIMEOUT\n");
        break;

    case WIFI_EVENT_SMP_CFG_START:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_START\n");
        break;
    case WIFI_EVENT_SMP_CFG_STOP:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_STOP\n");
        break;
    case WIFI_EVENT_SMP_CFG_TIMEOUT:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_TIMEOUT\n");
        break;
    case WIFI_EVENT_SMP_CFG_COMPLETED:
        puts("|network_user_callback->WIFI_EVENT_SMP_CFG_COMPLETED\n");
        break;

    case WIFI_EVENT_PM_SUSPEND:
        puts("|network_user_callback->WIFI_EVENT_PM_SUSPEND\n");
        break;
    case WIFI_EVENT_PM_RESUME:
        puts("|network_user_callback->WIFI_EVENT_PM_RESUME\n");
        break;
    case WIFI_EVENT_AP_ON_ASSOC:
        puts("WIFI_EVENT_AP_ON_ASSOC\n");
        break;
    case WIFI_EVENT_AP_ON_DISCONNECTED:
        puts("WIFI_EVENT_AP_ON_DISCONNECTED\n");
        break;
    default:
        break;
    }

    return 0;
}


static u8 tx_stop_flag = 0;
static void tx_test_task(void *priv)
{
    u32 cnt = 0;

    for (;;) {
        wifi_tx_data((u8 *)priv, tx_params.packet_len, tx_params.rate, tx_params.bandwidth, tx_params.short_gi);
        /* putchar('T'); */
        if (tx_params.send_interval) {
            delay_us(tx_params.send_interval);
        }

        if (tx_params.npackets) {
            cnt++;
            if (cnt >= tx_params.npackets) {
                tx_pid = 0;
                return;
            }
        }

        if (tx_stop_flag) {
            tx_pid = 0;
            return;
        }
    }
}


static void start_tx_test_data(void *priv)
{
    wifi_set_long_retry(0);
    wifi_set_short_retry(0);
    tx_stop_flag = 0;
    thread_fork("tx_test_task", 10, 256, 0, &tx_pid, tx_test_task, priv);
}


static void stop_tx_test_data(void)
{
    if (tx_pid) {
        tx_stop_flag = 1;
        while (tx_pid) {
            os_time_dly(1);
        }
    }

    /* if (tx_pid) { */
    /*     thread_kill(&tx_pid, KILL_FORCE); */
    /*     tx_pid = 0; */
    /* } */
}


static void report_data(u8 opcode, void *data, u32 len)
{
    u16 crc;
    u32 offset;
    u8 buf[64] = {0};
    struct fcc_data *fcc;

    /* log_info("opcode = %d, len = %d\n", opcode, len); */
    ((struct fcc_data *)buf)->mark[0] = 'J';
    ((struct fcc_data *)buf)->mark[1] = 'L';
    ((struct fcc_data *)buf)->opcode = opcode;
    ((struct fcc_data *)buf)->params_len = len;

    offset = sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len);
    memcpy(buf + offset, (u8 *)data, len);

    offset += len;
    crc = CRC16(buf, offset);
    memcpy(buf + offset, &crc, sizeof(fcc->crc));

    offset += sizeof(fcc->crc);
    comm_dev_tx_data(buf, offset);
    /* put_buf(buf, offset); */
}


static void fcc_data_show(u8 opcode, void *priv)
{
    u8 *mac, mode, *pa;
    struct FCC_TX *tx;
    struct FCC_RX *rx;
    struct FREQ_ADJ *freq_adj;
    struct PWR_ADJ *pwr_adj;
    struct WIFI_SIGN *sign;
    struct FCC_RX_STAT *rxs;

    switch (opcode) {
    case OP_FCC_INQ_RAND_NUM:
        /* log_info("OP_FCC_INQ_RAND_NUM:\n"); */
        putchar('H');
        break;

    case OP_FCC_START_TX:
        tx = (struct FCC_TX *)priv;
        log_info("\tOP_FCC_START_TX:\n");
        log_info("\tpa            = %d%d%d%d%d%d%d\n", tx->pa[0], tx->pa[1], \
                 tx->pa[2], tx->pa[3], tx->pa[4], tx->pa[5], tx->pa[6]);
        log_info("\txosc          = %d, %d\n", tx->xosc[0], tx->xosc[1]);
        log_info("\tchannel       = %d\n", tx->channel);
        log_info("\tbandwidth     = %d\n", tx->bandwidth);
        log_info("\tshort_gi      = %d\n", tx->short_gi);
        log_info("\tantenna_x     = %d\n", tx->antenna_x);
        log_info("\tpathx_txpower = %d\n", tx->pathx_txpower);
        log_info("\trate          = %s\n", tx_rate_tab[tx->rate].string);
        log_info("\tnpackets      = %d\n", tx->npackets);
        log_info("\tpacket_len    = %d\n", tx->packet_len);
        log_info("\tsend_interval = %d\n", tx->send_interval);
        log_info("\tcw_flag       = %d\n", tx->cw_flag);
        break;

    case OP_FCC_STOP_TX:
        log_info("\tOP_FCC_STOP_TX:\n");
        break;

    case OP_FCC_START_RX:
        rx = (struct FCC_RX *)priv;
        log_info("OP_FCC_START_RX:\n");
        log_info("\tpa            = %d%d%d%d%d%d%d\n", rx->pa[0], rx->pa[1], \
                 rx->pa[2], rx->pa[3], rx->pa[4], rx->pa[5], rx->pa[6]);
        log_info("\txosc          = %d, %d\n", rx->xosc[0], rx->xosc[1]);
        log_info("\tchannel       = %d\n", rx->channel);
        log_info("\tbandwidth     = %d\n", rx->bandwidth);
        log_info("\tshort_gi      = %d\n", rx->short_gi);
        log_info("\tantenna_x     = %d\n", rx->antenna_x);
        log_info("\tfilter_enable = %d\n", rx->filter_enable);
        log_info("\tfilter_mac    = %x:%x:%x:%x:%x:%x\n", rx->filter_mac[0], rx->filter_mac[1], \
                 rx->filter_mac[2], rx->filter_mac[3], rx->filter_mac[4], rx->filter_mac[5]);
        break;

    case OP_FCC_STOP_RX:
        log_info("OP_FCC_STOP_RX\n");
        break;

    case OP_FCC_SET_MAC:
        log_info("OP_FCC_SET_MAC:\n");
        mac = (u8 *)priv;
        log_info("MAC = %X:%X:%X:%X:%X:%X\n", mac[0], mac[1],
                 mac[2], mac[3], mac[4], mac[5]);
        break;

    case OP_FCC_INQ_MAC:
        log_info("OP_FCC_INQ_MAC:\n");
        break;

    case OP_STA_FREQ_ADJ:
        log_info("OP_STA_FREQ_ADJ:\n");
        freq_adj = (struct FREQ_ADJ *)priv;
        log_info("\tchannel       = %d\n", freq_adj->channel);
        log_info("\trate          = %d\n", freq_adj->rate);
        log_info("\tpathx_txpower = %d\n", freq_adj->pathx_txpower);
        log_info("\tthr_max       = %.2f\n", freq_adj->thr_max);
        log_info("\tthr_min       = %.2f\n", freq_adj->thr_min);
        log_info("\tmax_cnt       = %d\n", freq_adj->max_cnt);
        log_info("\tstep          = %d\n", freq_adj->step);
        break;

    case OP_IN_FREQ_ADJ:
        log_info("OP_IN_FREQ_ADJ:\n");
        log_info("\tcur_freq = %.2f\n", *((float *)priv));
        break;

    case OP_STA_PWR_ADJ:
        log_info("OP_STA_PWR_ADJ:\n");
        pwr_adj = (struct PWR_ADJ *)priv;
        log_info("\tchannel = %d\n", pwr_adj->channel);
        log_info("\trate    = %d\n", pwr_adj->rate);
        log_info("\tthr_max = %.2f\n", pwr_adj->thr_max);
        log_info("\tthr_min = %.2f\n", pwr_adj->thr_min);
        log_info("\tmax_cnt = %d\n", pwr_adj->max_cnt);
        log_info("\tstep    = %d\n", pwr_adj->step);
        break;

    case OP_IN_PWR_ADJ:
        log_info("OP_IN_PWR_ADJ:\n");
        log_info("\tcur_pwr  = %.2f\n", *((float *)priv));
        log_info("\tcur_evm  = %.2f\n", *((float *)priv + 1));
        log_info("\tcur_mask = %.2f\n", *((float *)priv + 2));
        break;

    case OP_FCC_INQ_DEF_DATA:
        log_info("OP_FCC_INQ_DEF_DATA:\n");
        break;


    case OP_FCC_SET_PA:
        log_info("OP_FCC_SET_PA:\n");
        pa = (u8 *)priv;
        log_info("\tpa = %d%d%d%d%d%d%d\n", pa[0], pa[1], pa[2], pa[3], pa[4], pa[5], pa[6]);
        break;

    case OP_FCC_SET_CH:
        log_info("OP_FCC_SET_CH:\n");
        log_info("\tCH = %d\n", *(u8 *)priv);
        break;

    case OP_FCC_SET_TX_RATE:
        log_info("OP_FCC_SET_TX_RATE:\n");
        log_info("\trate = %s\n", tx_rate_tab[*(u8 *)priv].string);
        break;

    case OP_FCC_SET_TX_GAIN:
        log_info("OP_FCC_SET_TX_GAIN:\n");
        log_info("\tgain = %d, rate = %s\n", *(u8 *)priv, tx_rate_tab[tx_params.rate].string);
        break;

    case OP_FCC_ENTER_WIFI_SIGN:
        log_info("OP_FCC_ENTER_WIFI_SIGN:\n");
        sign = (struct WIFI_SIGN *)priv;
        log_info("\tssid = %s\n", sign->ssid);
        log_info("\tpwd  = %s\n", sign->pwd);
        log_info("\txosc = %d, %d\n", sign->xosc[0], sign->xosc[1]);
        log_info("\tpa   = %d%d%d%d%d%d%d\n", sign->pa[0], sign->pa[1], \
                 sign->pa[2], sign->pa[3], sign->pa[4], sign->pa[5], sign->pa[6]);
        log_info("\tB_GAIN = %d, %d, %d, %d\n", sign->gain[0], sign->gain[1], sign->gain[2], sign->gain[3]);
        log_info("\tG_GAIN = %d, %d, %d, %d, %d, %d, %d, %d\n", sign->gain[4], sign->gain[5], sign->gain[6], sign->gain[7], \
                 sign->gain[8], sign->gain[9], sign->gain[10], sign->gain[11]);
        log_info("\tN_GAIN = %d, %d, %d, %d, %d, %d, %d, %d\n", sign->gain[12], sign->gain[13], sign->gain[14], sign->gain[15], \
                 sign->gain[16], sign->gain[17], sign->gain[18], sign->gain[19]);
        break;

    case OP_FCC_START_RX_STAT:
        rxs = (struct FCC_RX_STAT *)priv;
        log_info("OP_FCC_START_RX_STAT:\n");
        log_info("\tpa            = %d%d%d%d%d%d%d\n", rxs->pa[0], rxs->pa[1], \
                 rxs->pa[2], rxs->pa[3], rxs->pa[4], rxs->pa[5], rxs->pa[6]);
        log_info("\txosc          = %d, %d\n", rxs->xosc[0], rxs->xosc[1]);
        log_info("\tchannel       = %d\n", rxs->channel);
        log_info("\tbandwidth     = %d\n", rxs->bandwidth);
        log_info("\tshort_gi      = %d\n", rxs->short_gi);
        log_info("\tantenna_x     = %d\n", rxs->antenna_x);
        log_info("\tstat_time     = %d\n", rxs->stat_time);
        log_info("\tfilter_enable = %d\n", rxs->filter_enable);
        log_info("\tfilter_mac    = %x:%x:%x:%x:%x:%x\n", rxs->filter_mac[0], rxs->filter_mac[1], \
                 rxs->filter_mac[2], rxs->filter_mac[3], rxs->filter_mac[4], rxs->filter_mac[5]);
        break;

    case OP_FCC_STOP_RX_STAT:
        log_info("OP_FCC_STOP_RX_STAT\n");
        break;

    case OP_FCC_INQ_RX_STAT:
        log_info("OP_FCC_INQ_RX_STAT\n");
        break;

    default:
        break;
    }
}


static void cal_xosc(float cur_freq, u8 *cur_xosc, u8 step)
{
    for (int i = 0; i < step; i++) {
        if (cur_freq > 0) {
            if (cur_xosc[0] > cur_xosc[1]) {
                cur_xosc[1]++;
                cur_xosc[1] = (cur_xosc[1] > MAX_XOSC) ? MAX_XOSC : cur_xosc[1];
            } else {
                cur_xosc[0]++;
                cur_xosc[0] = (cur_xosc[0] > MAX_XOSC) ? MAX_XOSC : cur_xosc[0];
            }
        } else {
            if (cur_xosc[0] > cur_xosc[1]) {
                cur_xosc[0]--;
                cur_xosc[0] = (cur_xosc[0] < MIN_XOSC) ? MIN_XOSC : cur_xosc[0];
            } else {
                cur_xosc[1]--;
                cur_xosc[1] = (cur_xosc[1] < MIN_XOSC) ? MIN_XOSC : cur_xosc[1];
            }
        }
    }
}


void cal_gain(u8 rate, u8 gain, u8 *gain_array)
{
    s8 offset;
    u8 i, range_l, range_r;

    if (!strcmp(tx_rate_tab[rate].string, "11M")) {
        range_l = 0;
        range_r = 3;
    } else if (!strcmp(tx_rate_tab[rate].string, "54M")) {
        range_l = 4;
        range_r = 11;
    } else if (!strcmp(tx_rate_tab[rate].string, "HTMCS7")) {
        range_l = 12;
        range_r = 19;
    } else {
        gain_array[rate] = gain;
        return;
    }

    offset = gain - gain_array[rate];
    for (i = range_l; i <= range_r; i++) {
        gain_array[i] += offset;
    }
}


u8 fcc_freq_adj_read(s16 *data)
{
    if (rf_fcc_adj_res_read("offset", data)) {
        printf("\n ---  [RF_FCC_ADJ]fre_offset %d ---- \n", *data);
        return 1;
    } else {
        printf("\n ---  [RF_FCC_ADJ]fre_offset NULL ---- \n");
    }
    return 0;
}


u8 *fcc_common_data_deal(u8 packet_type, u8 *data, u32 len, u32 *rsp_len, u8 *reset)
{
    s32 ret;
    u8 buf[16] = {0};
    static u8 ready = 0;

    if (len < sizeof(FCC_BT_ARRAY) - 1) {
        return 0;
    }

    buf[0] = packet_type;
    memcpy(&buf[1], data, sizeof(buf) - 1);

    if (!array_cmp(buf, FCC_WIFI_ARRAY, sizeof(FCC_WIFI_ARRAY))) {
        log_info("COMMON OPCODE : change to FCC_WIFI_MODE");
        rf_fcc_save_change_mode(FCC_WIFI_MODE);
        *reset = 1;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
    } else if (!array_cmp(buf, FCC_BT_ARRAY, sizeof(FCC_BT_ARRAY))) {
        log_info("COMMON OPCODE : change to FCC_BT_MODE");
#ifdef CONFIG_BT_ENABLE
        rf_fcc_save_change_mode(FCC_BT_MODE);
        *reset = 1;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
#else
        log_err("no support BT Moudle\n");
        *reset = 0;
        return NULL;
#endif

    } else if (!array_cmp(buf, FCC_RESET_ARRAY, sizeof(FCC_RESET_ARRAY))) {
        log_info("COMMON OPCODE : RESET");
        rf_fcc_save_change_mode(CUR_MODE);
        *reset = 1;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        return FCC_RSP_ARRAY;
    } else if (!array_cmp(buf, FCC_HEART_ARRAY, sizeof(FCC_HEART_ARRAY))) {
        /* log_info("COMMON OPCODE : HEART"); */
        putchar('H');
        /* log_info("RAND_NUM = %d\n", RAND_NUM); */
        memcpy(&FCC_HEART_RSP_ARRAY[6], &RAND_NUM, sizeof(RAND_NUM));
        *reset = 0;
        *rsp_len = sizeof(FCC_HEART_RSP_ARRAY);
        return FCC_HEART_RSP_ARRAY;
    } else if (!array_cmp(buf, FCC_RES_SUCC_ARRAY, sizeof(FCC_RES_SUCC_ARRAY))) {
        log_info("COMMON OPCODE : RES_SUCC");
        read_res();
        fcc_res_handler(true);
        /* void clear_efuse(void); */
        /* clear_efuse(); */
        struct fcc_info info;
        strcpy(info.res, fcc_str[0]);
        info.crc = CRC16(info.res, sizeof(info.res));
        ret = syscfg_write(WIFI_BT_FCC_RES_FLAG, &info, sizeof(struct fcc_info));
        *reset = 0;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        ready = 1;
        return ((ret == sizeof(struct fcc_info)) ? FCC_RSP_ARRAY : FCC_RSP_FAIL_ARRAY);
    } else if (!array_cmp(buf, FCC_RES_FAIL_ARRAY, sizeof(FCC_RES_FAIL_ARRAY))) {
        log_info("COMMON OPCODE : RES_FAIL");
        read_res();
        fcc_res_handler(false);
        /* void clear_efuse(void); */
        /* clear_efuse(); */
        struct fcc_info info;
        strcpy(info.res, fcc_str[1]);
        info.crc = CRC16(info.res, sizeof(info.res));
        ret = syscfg_write(WIFI_BT_FCC_RES_FLAG, &info, sizeof(struct fcc_info));
        *reset = 0;
        *rsp_len = sizeof(FCC_RSP_ARRAY);
        ready = 1;
        return ((ret == sizeof(struct fcc_info)) ? FCC_RSP_ARRAY : FCC_RSP_FAIL_ARRAY);
    } else if (!array_cmp(buf, FCC_READY_ARRAY, sizeof(FCC_READY_ARRAY))) {
        log_info("COMMON OPCODE : READY");
        *reset = 0;
        *rsp_len = sizeof(FCC_READY_RSP_ARRAY);
        return (ready ? NULL : FCC_READY_RSP_ARRAY);
    } else {
        *reset = 0;
        return NULL;
    }
    return NULL;
}


static void fcc_data_deal_task(void *priv)
{
    s32 ret;
    u8 *xosc;
    u8 mac[6], gain_array[20];
    u8 save_flag, status, cur_dgain, step;
    float cur_freq, cur_pwr;
    static u8 adj_cnt, max_cnt;
    static float thr_max, thr_min;
    u32 offset, val, last_cnt, res;
    static u8 cur_xosc[2];
    struct wifi_mode_info cur_info = {0};
    struct FCC_HIS his = {0};
    struct host_data *host;
    struct fcc_data *fcc;
    struct FCC_TX *tx;
    struct FCC_RX *rx;
    struct FCC_RX_STAT *rxs;
    struct FREQ_ADJ *freq_adj;
    struct PWR_ADJ *pwr_adj;
    struct WIFI_SIGN *sign;
    u8 none_pa[7] = {0};
    static u8 last_pa[7] = {0}, last_ch = 0;

    for (;;) {
        if (wifi_is_on()) {
            log_info("wifi_is_on");
            break;
        }
        os_time_dly(10);
    }

    for (;;) {
        list_for_each_entry(host, &head, entry) {
            fcc = &host->fcc_data;
            fcc_data_show(fcc->opcode, fcc->params);

            wifi_get_mode_cur_info(&cur_info);
            if (fcc->opcode > OP_IN_WIFI && fcc->opcode < OP_OUT_WIFI && cur_info.mode != MP_TEST_MODE) {
                wifi_enter_mp_test_mode();
                os_time_dly(2);
            }

#if 0 //for cmd need reset
            if (fcc->opcode == OP_FCC_ENTER_TEST) {
                save_flag = (*((u8 *)fcc->params) == FCC_BT_MODE) ? 1 : 0;
                his.mode = FCC_BT_MODE;
            } else {
                save_flag = 0;
            }

            if (save_flag) {
                offset = sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len);
                memcpy((u8 *)&his.data, (u8 *)fcc, offset);
                memcpy((u8 *)&his.data + offset, fcc->params, fcc->params_len);
                offset += fcc->params_len;
                memcpy((u8 *)&his.data + offset, &fcc->crc, sizeof(fcc->crc));
                put_buf(&his, sizeof(struct FCC_HIS));

                syscfg_write(CFG_USER_DEFINE_TEST, (u8 *)&his, sizeof(struct FCC_HIS));
                cpu_reset();
            }
#endif

            switch (fcc->opcode) {
            case OP_FCC_INQ_RAND_NUM:
                /* log_info("RAND_NUM = %d\n", RAND_NUM); */
                report_data(OP_FCC_RPT_RAND_NUM, &RAND_NUM, sizeof(RAND_NUM));
                break;

            case OP_FCC_START_TX:
                tx = (struct FCC_TX *)fcc->params;
                xosc = ((u8 *)fcc->params) + sizeof(tx->pa);

                wf_write_xosc(xosc);

                //认证模式下使用默认PA
                if (array_cmp(none_pa, tx->pa, sizeof(none_pa))) {
                    set_pa_config_data(tx->pa);
                    wl_anl_actl_init(0);
                }

                wifi_set_channel(tx->channel);
                mp_test_pa_mcs_dgain_set(tx_rate_tab[tx->rate].phy, tx_rate_tab[tx->rate].mcs, tx->pathx_txpower);

                if (tx->cw_flag) {
                    val = sdio_mac_rreg(MAC_SYS_CTRL);
                    val = val | 0x00000002;
                    sdio_mac_wreg(MAC_SYS_CTRL, val);

                    val = sdio_mac_rreg(MAC_SYS_CTRL);
                    val = val & ~(0x00000002);
                    sdio_mac_wreg(MAC_SYS_CTRL, val);

                    val = sdio_mac_rreg(MAC_SYS_CTRL);
                    val &= ~(1 << 3);
                    sdio_mac_wreg(MAC_SYS_CTRL, val);

                    val = sdio_mac_rreg(MAC_SYS_CTRL);
                    val = val | 0x00000010;
                    sdio_mac_wreg(MAC_SYS_CTRL, val);

                    wf_tx_sine_test();
                }

                stop_tx_test_data();
                memcpy(&tx_params, tx, sizeof(struct FCC_TX));
                start_tx_test_data(NULL);
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_STOP_TX:
                stop_tx_test_data();
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_START_RX:
                stop_tx_test_data();
                rx = (struct FCC_RX *)fcc->params;
                xosc = ((u8 *)fcc->params) + sizeof(tx->pa);

                wf_write_xosc(xosc);
                wifi_set_channel(rx->channel);
                wifi_set_frame_cb(wifi_rx_frame_cb, NULL);

                memcpy(&rx_params, rx, sizeof(struct FCC_RX));
                TOTAL_RX_CNT = 0;
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_STOP_RX:
                wifi_set_frame_cb(NULL, NULL);
                report_data(OP_FCC_RPT_RX_RES, &TOTAL_RX_CNT, sizeof(TOTAL_RX_CNT));
                log_info("TOTAL_RX_CNT = %d\n", TOTAL_RX_CNT);
                TOTAL_RX_CNT = 0;
                break;

            case OP_FCC_SET_MAC:
                wifi_set_mac(fcc->params);
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_INQ_MAC:
                memset(mac, 0, sizeof(mac));
                wifi_get_mac(mac);
                log_info("MAC= %X:%X:%X:%X:%X:%X\n", mac[0], mac[1], \
                         mac[2], mac[3], mac[4], mac[5]);
                report_data(OP_FCC_RPT_MAC, mac, sizeof(mac));
                break;

            case OP_STA_FREQ_ADJ:
                freq_adj = (struct FREQ_ADJ *)fcc->params;

                wf_read_xosc(cur_xosc);
                wifi_set_channel(freq_adj->channel);

                adj_cnt = 0;
                step    = freq_adj->step;
                max_cnt = freq_adj->max_cnt;
                thr_max = freq_adj->thr_max;
                thr_min = freq_adj->thr_min;

                stop_tx_test_data();
                memset(&tx_params, 0, sizeof(struct FCC_TX));
                tx_params.pathx_txpower = freq_adj->pathx_txpower;
                tx_params.rate          = freq_adj->rate;
                tx_params.packet_len    = TX_LEN_IN_ADJ;
                tx_params.send_interval = TX_INR_IN_ADJ;
                log_info("START_FREQ_ADJ: cur_xosc = %d, %d\n", cur_xosc[0], cur_xosc[1]);
                start_tx_test_data(NULL);
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_IN_FREQ_ADJ:
                cur_freq = *((float *)fcc->params);
                adj_cnt++;
                if (adj_cnt > max_cnt || (cur_freq >= thr_min && cur_freq <= thr_max)) {
                    stop_tx_test_data();
                    log_info("STOP_FREQ_ADJ: %d, %d, adj_cnt = %d / %d\n", cur_xosc[0], cur_xosc[1], adj_cnt, max_cnt);
                    status = (cur_freq >= thr_min && cur_freq <= thr_max) ?  ST_SUCC : ST_FAIL;
                    if (status == ST_SUCC) {
                        ret = syscfg_write(VM_XOSC_INDEX, cur_xosc, sizeof(cur_xosc));
                        status = (ret == sizeof(cur_xosc)) ? ST_SUCC : ST_FAIL;
                        log_info("FREQ_ADJ_SAVE: ret = %d, status = %d\n", ret, status);
                    }
                    report_data(OP_FIN_FREQ_ADJ, &status, sizeof(status));
                } else {
                    cal_xosc(cur_freq, cur_xosc, step);
                    wf_write_xosc(cur_xosc);
                    log_info("IN_FREQ_ADJ: %d, %d, adj_cnt = %d / %d\n", cur_xosc[0], cur_xosc[1], adj_cnt, max_cnt);

                    status = ST_SUCC;
                    report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                }
                break;

            case OP_STA_PWR_ADJ:
                pwr_adj = (struct PWR_ADJ *)fcc->params;

                stop_tx_test_data();
                wifi_set_channel(pwr_adj->channel);
                wifi_get_mcs_dgain(gain_array);
                cur_dgain = gain_array[pwr_adj->rate];
                mp_test_pa_mcs_dgain_set(tx_rate_tab[pwr_adj->rate].phy, tx_rate_tab[pwr_adj->rate].mcs, cur_dgain);

                adj_cnt = 0;
                step    = pwr_adj->step;
                max_cnt = pwr_adj->max_cnt;
                thr_max = pwr_adj->thr_max;
                thr_min = pwr_adj->thr_min;

                memset(&tx_params, 0, sizeof(struct FCC_TX));
                tx_params.pathx_txpower = cur_dgain;
                tx_params.rate = pwr_adj->rate;
                tx_params.packet_len    = TX_LEN_IN_ADJ;
                tx_params.send_interval = TX_INR_IN_ADJ;
                log_info("START_PWR_ADJ: rate = %s, cur_dgain = %d\n", tx_rate_tab[tx_params.rate].string, cur_dgain);
                start_tx_test_data(NULL);
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_IN_PWR_ADJ:
                cur_pwr = *((float *)fcc->params);
                adj_cnt++;
                if (adj_cnt > max_cnt || (cur_pwr >= thr_min && cur_pwr <= thr_max)) {
                    stop_tx_test_data();
                    log_info("STOP_PWR_ADJ: rate = %s, dgain = %d, adj_cnt = %d / %d\n", tx_rate_tab[tx_params.rate].string, cur_dgain, adj_cnt, max_cnt);
                    status = (cur_pwr >= thr_min && cur_pwr <= thr_max) ?  ST_SUCC : ST_FAIL;
                    if (status == ST_SUCC) {
                        gain_array[tx_params.rate] = cur_dgain;
                        /* cal_gain(tx_params.rate, cur_dgain, gain_array); */
                        ret = syscfg_write(VM_WIFI_PA_MCS_DGAIN, gain_array, sizeof(gain_array));
                        status = (ret == sizeof(gain_array)) ? ST_SUCC : ST_FAIL;
                        log_info("PWR_ADJ_SAVE: ret = %d, status = %d\n", ret, status);
                    }
                    report_data(OP_FIN_PWR_ADJ, &status, sizeof(status));
                } else {
                    if (cur_pwr < thr_min) {
                        cur_dgain = (cur_dgain + step) > MAX_DGAIN ? MAX_DGAIN : (cur_dgain + step);
                    } else if (cur_pwr > thr_max) {
                        cur_dgain = (cur_dgain - step) < MIN_DGAIN ? MIN_DGAIN : (cur_dgain - step);
                    }

                    log_info("IN_PWR_ADJ: rate = %s, dgain = %d, adj_cnt = %d / %d\n", tx_rate_tab[tx_params.rate].string, cur_dgain, adj_cnt, max_cnt);

                    stop_tx_test_data();
                    mp_test_pa_mcs_dgain_set(tx_rate_tab[tx_params.rate].phy, tx_rate_tab[tx_params.rate].mcs, cur_dgain);
                    start_tx_test_data(NULL);
                    status = ST_SUCC;
                    report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                }
                break;

            case OP_FCC_INQ_DEF_DATA:
                struct WIFI_DEF_DATA def_data = {0};
                syscfg_read(VM_WIFI_PA_DATA, (u8 *)def_data.pa, sizeof(def_data.pa));
                syscfg_read(VM_XOSC_INDEX, (u8 *)def_data.xosc, sizeof(def_data.xosc));
                syscfg_read(VM_WIFI_PA_MCS_DGAIN, (u8 *)def_data.mcs_dgain, sizeof(def_data.mcs_dgain));
                report_data(OP_FCC_RPT_DEF_DATA, &def_data, sizeof(struct WIFI_DEF_DATA));
                break;

            case OP_FCC_SET_PA:
                if (array_cmp(last_pa, fcc->params, sizeof(last_pa))) {
                    set_pa_config_data(fcc->params);
                    wl_anl_actl_init(0);
                    memcpy(last_pa, fcc->params, sizeof(last_pa));
                } else {
                    log_info("the same PA\n");
                }
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_SET_CH:
                if (last_ch != *(fcc->params)) {
                    last_ch = *(fcc->params);
                    wifi_set_channel(*(fcc->params));
                } else {
                    log_info("the same CH\n");
                }
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_SET_TX_RATE:
                if (tx_pid) {
                    stop_tx_test_data();
                    tx_params.rate = *(fcc->params);
                    start_tx_test_data(NULL);
                    status = ST_SUCC;
                } else {
                    status = ST_FAIL;
                }
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_SET_TX_GAIN:
                if (tx_pid) {
                    mp_test_pa_mcs_dgain_set(tx_rate_tab[tx_params.rate].phy, tx_rate_tab[tx_params.rate].mcs, *(fcc->params));
                    status = ST_SUCC;
                } else {
                    status = ST_FAIL;
                }
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_ENTER_WIFI_SIGN:
                sign = (struct WIFI_SIGN *)fcc->params;
                strcpy(sign_ssid, sign->ssid);
                strcpy(sign_pwd, sign->pwd);
                stop_tx_test_data();
                wifi_set_frame_cb(NULL, NULL);
                /* extern u8 ntp_get_time_init; */
                /* ntp_get_time_init = 0; */
                wifi_off();
                wifi_on();

                wf_write_xosc(sign->xosc);
                set_pa_config_data(sign->pa);
                wl_anl_actl_init(0);

                for (u8 i = 0; i < sizeof(sign->gain); i++) {
                    mp_test_pa_mcs_dgain_set(tx_rate_tab[i].phy, tx_rate_tab[i].mcs, sign->gain[i]);
                }
                udp_server_init(30136);
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_START_RX_STAT:
                stop_tx_test_data();
                rxs = (struct FCC_RX_STAT *)fcc->params;
                xosc = ((u8 *)fcc->params) + sizeof(tx->pa);

                wf_write_xosc(xosc);
                wifi_set_channel(rxs->channel);
                wifi_set_frame_cb(wifi_rx_frame_cb, NULL);

                memcpy(&rx_params, rxs, sizeof(struct FCC_RX));
                last_cnt = 0;
                TOTAL_RX_CNT = 0;
                status = ST_SUCC;
                report_data(OP_FCC_RPT_ST, &status, sizeof(status));
                break;

            case OP_FCC_STOP_RX_STAT:
                wifi_set_frame_cb(NULL, NULL);
                report_data(OP_FCC_RPT_RX_RES, &TOTAL_RX_CNT, sizeof(TOTAL_RX_CNT));
                log_info("TOTAL_RX_CNT = %d\n", TOTAL_RX_CNT);
                TOTAL_RX_CNT = 0;
                break;

            case OP_FCC_INQ_RX_STAT:
                res = TOTAL_RX_CNT - last_cnt;
                log_info("RX_STAT: res = %d, now = %d, last = %d\n", res, TOTAL_RX_CNT, last_cnt);
                last_cnt = TOTAL_RX_CNT;
                report_data(OP_FCC_RPT_RX_STAT, &res, sizeof(res));
                break;

            default:
                break;
            }
            list_del(&host->entry);
            free(fcc->params);
            free(host->self);
            break;
        }
        os_time_dly(1);
    }
}


u8 get_fcc_info(void)
{
    u8 ret = FCC_NULL;
    struct fcc_info info;

    if (syscfg_read(WIFI_BT_FCC_RES_FLAG, &info, sizeof(struct fcc_info)) == sizeof(struct fcc_info)) {
        if (info.crc == CRC16(info.res, sizeof(info.res))) {
            if (!strcmp(fcc_str[0], info.res)) {
                ret = FCC_SUCC;
            } else if (!strcmp(fcc_str[1], info.res)) {
                ret = FCC_FAIL;
            }
        }
    }

    log_info("==================FCC adjust %s==================\n", fcc_str[ret]);
    return ret;
}


void clear_efuse(void)
{
    u8 xosc[2];
    xosc[0] = wifi_calibration_param.xosc_l;
    xosc[1] = wifi_calibration_param.xosc_r;
    syscfg_write(VM_XOSC_INDEX, xosc, 2);
    syscfg_write(VM_WIFI_PA_MCS_DGAIN, wifi_calibration_param.mcs_dgain, sizeof(wifi_calibration_param.mcs_dgain));

    s16 adj_freq = -30;
    u8 ble_power = 6, bt_power = 8;
    rf_fcc_adj_res_write("ble", &ble_power);
    rf_fcc_adj_res_write("edr", &bt_power);
    rf_fcc_adj_res_write("offset", &adj_freq);

}


static void create_cmd_data(u8 opcode, void *data, u32 len)
{
    u16 crc;
    u32 offset;
    u8 buf[160] = {0};
    struct fcc_data *fcc;

    /* log_info("opcode = %d, len = %d\n", opcode, len); */
    ((struct fcc_data *)buf)->mark[0] = 'J';
    ((struct fcc_data *)buf)->mark[1] = 'L';
    ((struct fcc_data *)buf)->opcode = opcode;
    ((struct fcc_data *)buf)->params_len = len;

    offset = sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len);
    if (data) {
        memcpy(buf + offset, (u8 *)data, len);
    }

    offset += len;
    crc = CRC16(buf, offset);
    memcpy(buf + offset, &crc, sizeof(fcc->crc));

    offset += sizeof(fcc->crc);
    put_buf(buf, offset);
}


static void create_fcc_cmd_demo(void)
{
    log_info("================OP_FCC_ENTER_WIFI_SIGN================\n");
    struct WIFI_SIGN sign = {0};
    strcpy(sign.ssid, "GJ");
    strcpy(sign.pwd, "8888888899");
    memset(sign.xosc, 0x0b, sizeof(sign.xosc));
    memset(sign.pa, 0x07, sizeof(sign.pa));
    memset(sign.gain, 32, sizeof(sign.gain));
    create_cmd_data(OP_FCC_ENTER_WIFI_SIGN, &sign, sizeof(struct WIFI_SIGN));

    log_info("====================OP_FCC_STOP_TX====================\n");
    create_cmd_data(OP_FCC_STOP_TX, NULL, 0);

    log_info("====================OP_FCC_STOP_RX====================\n");
    create_cmd_data(OP_FCC_STOP_RX, NULL, 0);

    log_info("==================OP_FCC_INQ_RAND_NUM=================\n");
    create_cmd_data(OP_FCC_INQ_RAND_NUM, NULL, 0);

    log_info("====================OP_FCC_SET_PA====================\n");
    u8 pa_set[7] = {7, 7, 7, 7, 7, 7, 7};
    create_cmd_data(OP_FCC_SET_PA, pa_set, sizeof(pa_set));

    log_info("====================OP_FCC_SET_CH====================\n");
    u8 ch_set = 7;
    create_cmd_data(OP_FCC_SET_CH, &ch_set, sizeof(ch_set));

    log_info("====================OP_FCC_SET_TX_RATE====================\n");
    u8 rate_set = 1;
    create_cmd_data(OP_FCC_SET_TX_RATE, &rate_set, sizeof(rate_set));

    log_info("====================OP_FCC_SET_TX_GAIN====================\n");
    u8 gain_set = 72;
    create_cmd_data(OP_FCC_SET_TX_GAIN, &gain_set, sizeof(gain_set));

    log_info("====================OP_FCC_SET_MAC====================\n");
    u8 mac[6];
    memset(mac, 0xaa, 6);
    create_cmd_data(OP_FCC_SET_MAC, mac, 6);

    log_info("====================OP_FCC_INQ_MAC====================\n");
    create_cmd_data(OP_FCC_INQ_MAC, NULL, 0);

    log_info("=================OP_FCC_INQ_DEF_DATA==================\n");
    create_cmd_data(OP_FCC_INQ_DEF_DATA, NULL, 0);

    log_info("====================TX_TEST, XOSC=====================\n");
    struct FCC_TX data = {0};

    u8 pa[7] = {1, 7, 4, 7, 9, 1, 7};
    u8 xosc[2] = {11, 10};

    memcpy(data.pa, pa, 7);
    data.channel       = 7;
    data.bandwidth     = 0;
    data.short_gi      = 0;
    data.antenna_x     = 0;
    data.pathx_txpower = 32;
    data.rate          = 3;
    data.npackets      = 0;
    data.packet_len    = 1000;
    data.send_interval = 1000;
    data.cw_flag       = 0;
    for (int i = 0; i < 16; i++) {
        xosc[0] = i;
        xosc[1] = i;
        memcpy(data.xosc, xosc, 2);
        /* log_info("xosc = %d, %d\n", xosc[0], xosc[1]); */
        /* create_cmd_data(OP_FCC_START_TX, &data, sizeof(struct FCC_TX)); */
    }

    log_info("====================TX_TEST, DGAIN=====================\n");
    xosc[0] = 11;
    xosc[1] = 10;
    memcpy(data.pa, pa, 7);
    memcpy(data.xosc, xosc, 2);
    data.channel       = 7;
    data.bandwidth     = 0;
    data.short_gi      = 0;
    data.antenna_x     = 0;
    data.pathx_txpower = 32;
    data.rate          = 3;
    data.npackets      = 0;
    data.packet_len    = 1000;
    data.send_interval = 1000;
    data.cw_flag       = 0;
    for (int i = 0; i < 5; i++) {
        data.pathx_txpower = 32 * i;
        log_info("dgain = %d\n", data.pathx_txpower);
        create_cmd_data(OP_FCC_START_TX, &data, sizeof(struct FCC_TX));
    }


    log_info("========================RX_TEST========================\n");
    struct FCC_RX rx = {0};

    xosc[0] = 11;
    xosc[1] = 10;
    memcpy(rx.pa, pa, 7);
    memcpy(rx.xosc, xosc, 2);
    rx.channel       = 7;
    rx.bandwidth     = 0;
    rx.short_gi      = 0;
    rx.antenna_x     = 0;
    rx.filter_enable = 0;
    create_cmd_data(OP_FCC_START_RX, &rx, sizeof(struct FCC_RX));

    log_info("========================RX_STAT_TEST========================\n");
    struct FCC_RX_STAT rxs = {0};

    xosc[0] = 11;
    xosc[1] = 10;
    memcpy(rxs.pa, pa, 7);
    memcpy(rxs.xosc, xosc, 2);
    rxs.channel       = 7;
    rxs.bandwidth     = 0;
    rxs.short_gi      = 0;
    rxs.antenna_x     = 0;
    rxs.stat_time     = 1;
    rxs.filter_enable = 0;
    create_cmd_data(OP_FCC_START_RX_STAT, &rxs, sizeof(struct FCC_RX_STAT));

    log_info("====================OP_FCC_STOP_RX_STAT====================\n");
    create_cmd_data(OP_FCC_STOP_RX_STAT, NULL, 0);

    log_info("====================OP_FCC_INQ_RX_STAT====================\n");
    create_cmd_data(OP_FCC_INQ_RX_STAT, NULL, 0);

    log_info("====================freq_adj=====================\n");
    float cur_freq;
    struct FREQ_ADJ adj = {0};

    adj.channel   = 7;
    adj.rate      = 3;
    adj.pathx_txpower = 32;
    adj.thr_max   = 5.5;
    adj.thr_min   = -5.5;
    adj.max_cnt   = 3;
    adj.step      = 1;
    create_cmd_data(OP_STA_FREQ_ADJ, &adj, sizeof(struct FREQ_ADJ));

    adj.step      = 2;
    create_cmd_data(OP_STA_FREQ_ADJ, &adj, sizeof(struct FREQ_ADJ));

    cur_freq  = 30.02;
    create_cmd_data(OP_IN_FREQ_ADJ, &cur_freq, sizeof(float));

    cur_freq  = 20.03;
    create_cmd_data(OP_IN_FREQ_ADJ, &cur_freq, sizeof(float));

    cur_freq  = 10.05;
    create_cmd_data(OP_IN_FREQ_ADJ, &cur_freq, sizeof(float));

    cur_freq  = 4.5;
    create_cmd_data(OP_IN_FREQ_ADJ, &cur_freq, sizeof(float));

    log_info("=====================pwr_adj=====================\n");
    s8 cur_pwr;
    struct PWR_ADJ pwr_adj = {0};
    float res_buf[3];

    pwr_adj.channel   = 7;
    pwr_adj.rate      = 3;
    pwr_adj.thr_max   = 15.15;
    pwr_adj.thr_min   = 10.10;
    pwr_adj.max_cnt   = 3;
    pwr_adj.step      = 16;
    create_cmd_data(OP_STA_PWR_ADJ, &pwr_adj, sizeof(struct PWR_ADJ));

    res_buf[1] = 66.66;
    res_buf[2] = 77.77;

    res_buf[0] = 2.2;
    create_cmd_data(OP_IN_PWR_ADJ, res_buf, sizeof(res_buf));

    res_buf[0] = 5.5;
    create_cmd_data(OP_IN_PWR_ADJ, res_buf, sizeof(res_buf));

    res_buf[0] = 6.6;
    create_cmd_data(OP_IN_PWR_ADJ, res_buf, sizeof(res_buf));

    res_buf[0] = 12.12;
    create_cmd_data(OP_IN_PWR_ADJ, res_buf, sizeof(res_buf));
}


u8 get_rf_fcc_mode(void)
{
    return CUR_MODE;
}


static u8 fcc_enter_io_trigger(void)
{
    u8 cnt = 0;

    gpio_set_pull_up(CONFIG_RF_FCC_TRIGGER_IO_PORT, !CONFIG_RF_FCC_TRIGGER_IO_STATE);
    gpio_set_pull_down(CONFIG_RF_FCC_TRIGGER_IO_PORT, CONFIG_RF_FCC_TRIGGER_IO_STATE);
    gpio_direction_input(CONFIG_RF_FCC_TRIGGER_IO_PORT);

    for (cnt = 0; cnt < CONFIG_RF_FCC_TRIGGER_IO_CNT; cnt++) {
        if (gpio_read(CONFIG_RF_FCC_TRIGGER_IO_PORT) != CONFIG_RF_FCC_TRIGGER_IO_STATE) {
            break;
        }
        os_time_dly(1);
    }
    return (cnt >= CONFIG_RF_FCC_TRIGGER_IO_CNT) ? 1 : 0;
}


static u8 fcc_enter_gpcnt_trigger(void)
{
    u8 cnt = 0;
    u32 gpcnt = 1;
    void *gpcnt_hd = NULL;

    gpcnt_hd = dev_open("gpcnt", NULL);
    if (!gpcnt_hd) {
        log_info("gpcnt open err\n");
        return 0;
    }

    for (cnt = 0; cnt < CONFIG_RF_FCC_GPCNT_TRIGGER_CNT; cnt++) {
        dev_ioctl(gpcnt_hd, IOCTL_GET_GPCNT, (u32)&gpcnt);
        if (gpcnt < CONFIG_RF_FCC_GPCNT_TRIGGER_FREQ_L || \
            gpcnt > CONFIG_RF_FCC_GPCNT_TRIGGER_FREQ_H) {
            break;
        }
        os_time_dly(1);
    }

    dev_close(gpcnt_hd);
    return (cnt >= CONFIG_RF_FCC_GPCNT_TRIGGER_CNT) ? 1 : 0;
}


__attribute__((weak))u8 fcc_enter_user_def(void)
{
    return 0;
}


void rf_fcc_save_change_mode(u8 mode)
{
    u8 check_len;
    struct fcc_mode info = {0};

    strcpy(info.str, "change");
    info.mode = mode;
    check_len = sizeof(struct fcc_mode) - sizeof(&info.crc);
    info.crc  = CRC16(&info, check_len);
    syscfg_write(WIFI_BT_FCC_CHANGE, &info, sizeof(struct fcc_mode));
}


u8 rf_fcc_get_change_mode(void)
{
    u8 check_len;
    struct fcc_mode info = {0};

    if (syscfg_read(WIFI_BT_FCC_CHANGE, &info, sizeof(struct fcc_mode)) == sizeof(struct fcc_mode)) {
        check_len = sizeof(struct fcc_mode) - sizeof(&info.crc);
        if (CRC16(&info, check_len) == info.crc) {
            CUR_MODE = info.mode;
            log_info("%s, CUR_MODE = %d\n", __FUNCTION__, CUR_MODE);
            memset(&info, 0, sizeof(struct fcc_mode));
            syscfg_write(WIFI_BT_FCC_CHANGE, &info, sizeof(struct fcc_mode));
            return CUR_MODE;
        }
    }
    return 0;
}


u8 rf_fcc_test_init(void)
{
    u8 mode;
    u8 mac[6] = {0};
    u32 gpio = TRIGGER_IO;
    struct FCC_HIS his = {0};
    struct host_data *host;
    struct fcc_data *fcc;

    if (!(mode = rf_fcc_get_change_mode())) {
#if (CONFIG_RF_FCC_TRIGGER_MODE == IO_TRIGGER_MODE)
        if (!fcc_enter_io_trigger()) {
            return 0;
        }
#elif (CONFIG_RF_FCC_TRIGGER_MODE == GPCNT_TRIGGER_MODE)
        if (!fcc_enter_gpcnt_trigger()) {
            return 0;
        }
#elif (CONFIG_RF_FCC_TRIGGER_MODE == USER_DEF_MODE)
        if (!fcc_enter_user_def()) {
            return 0;
        }
#endif
    }

    get_fcc_info();
    /* create_fcc_cmd_demo(); */
    RAND_NUM = random32(0) & 2147483647;
    log_info("RAND_NUM = %ld\n", RAND_NUM);

    INIT_LIST_HEAD(&head);

#if 0 //for cmd need reset
    int read_ret = syscfg_read(CFG_USER_DEFINE_TEST, (u8 *)&his, sizeof(struct FCC_HIS));
    if (read_ret == sizeof(struct FCC_HIS)) {
        put_buf(&his, sizeof(struct FCC_HIS));
        if (his.mode == FCC_WIFI_MODE) {
            host = zalloc(sizeof(struct host_data));
            host->flag = 1;
            host->self = host;
            fcc = &host->fcc_data;

            memcpy((u8 *) & (fcc->mark),       his.data,  sizeof(fcc->mark));
            memcpy((u8 *) & (fcc->opcode),     his.data + sizeof(fcc->mark), sizeof(fcc->opcode));
            memcpy((u8 *) & (fcc->params_len), his.data + sizeof(fcc->mark) + sizeof(fcc->opcode), sizeof(fcc->params_len));
            memcpy((u8 *) & (fcc->crc),        his.data + sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len) + fcc->params_len, sizeof(fcc->crc));

            fcc->params = (u8 *)zalloc(fcc->params_len);
            memcpy(fcc->params, his.data + sizeof(fcc->mark) + sizeof(fcc->opcode) + sizeof(fcc->params_len), fcc->params_len);

            list_add_tail(&host->entry, &head);
            CUR_MODE = FCC_WIFI_MODE;
        } else if (his.mode == FCC_BT_MODE) {
            CUR_MODE = FCC_BT_MODE;
        }

        memset(&his, 0xaa, sizeof(struct FCC_HIS));
        int ret = syscfg_write(CFG_USER_DEFINE_TEST, &his, sizeof(struct FCC_HIS));
        log_info("syscfg_write, ret = %d\n", ret);
        if (CUR_MODE == FCC_BT_MODE) {
            return 0;
        }
    }
#endif

    if (mode == FCC_BT_MODE) {
#ifdef CONFIG_BT_ENABLE
        wifi_set_mac(mac);

        config_btctler_mode = BT_FCC;
        config_btctler_hci_standard = 1;
        extern void bt_ble_module_init(void);
        bt_ble_module_init();
        return FCC_BT_MODE;
#else
        return 0;
#endif
    } else if (mode == FCC_WIFI_MODE) {
        wifi_set_sta_connect_timeout(10000);
        wifi_set_event_callback(wifi_event_callback);
        wifi_on();

        comm_dev_init();
        thread_fork("comm_dev_recv_task", 28, 256, 0, NULL, comm_dev_recv_task, NULL);
        thread_fork("fcc_data_deal_task", 26, 1024, 0, NULL, fcc_data_deal_task, NULL);
        return FCC_WIFI_MODE;
    }

    comm_dev_init();
    thread_fork("comm_dev_recv_task", 28, 256, 0, NULL, comm_dev_recv_task, NULL);

#if ((CONFIG_RF_FCC_TRIGGER_MODE == IO_TRIGGER_MODE) || \
     (CONFIG_RF_FCC_TRIGGER_MODE == GPCNT_TRIGGER_MODE) || \
     (CONFIG_RF_FCC_TRIGGER_MODE == USER_DEF_MODE))
    return FCC_WAIT_MODE;
#endif
    return 0;
}


void wifi_tx_data_test(u8 channel, u8 power, u8 rate, u8 *packet, u32 packet_len, u32 npackets, u32 tx_interval)
{
    if (packet_len > 1513 || packet_len < 1) {
        log_err("%s, params err, packet_len = %d\n", __FUNCTION__, packet_len);
    }

    stop_tx_test_data();

    memset(&tx_params, 0, sizeof(struct FCC_TX));
    tx_params.rate          = rate;
    tx_params.packet_len    = packet_len;
    tx_params.npackets      = npackets;
    tx_params.send_interval = tx_interval;

    wifi_set_channel(channel);
    mp_test_pa_mcs_dgain_set(tx_rate_tab[rate].phy, tx_rate_tab[rate].mcs, power);
    start_tx_test_data(packet);
}


static void read_res(void)
{
    u8 bt_power = 0, ble_power = 0;
    struct wifi_calibration_param cal = {0};

    syscfg_read(CFG_BT_RF_POWER_ID, &bt_power, 1);
    syscfg_read(CFG_BLE_RF_POWER_ID, &ble_power, 1);

    syscfg_read(VM_XOSC_INDEX, &cal.xosc_l, 2);
    syscfg_read(VM_WIFI_PA_MCS_DGAIN, &cal.mcs_dgain, sizeof(cal.mcs_dgain));
    syscfg_read(VM_WIFI_PA_DATA, &cal.pa_trim_data, sizeof(cal.pa_trim_data));

    log_info("=========================CAL_RES=========================\n");
    log_info("bt_power = %d, ble_power = %d\n", bt_power, ble_power);
    log_info("xosc_l = %d, xosc_r = %d\n", cal.xosc_l, cal.xosc_r);
    log_info("PA : \n");
    put_buf(&cal.pa_trim_data, sizeof(cal.pa_trim_data));
    log_info("MCS_GAIN : \n");
    put_buf(&cal.mcs_dgain, sizeof(cal.mcs_dgain));
    log_info("=========================CAL_RES=========================\n");
}


static void udp_recv_handler(void *socket_fd)
{
    static u32 sock_rcv_cnt;
    struct sockaddr_in remote_addr;
    socklen_t len = sizeof(remote_addr);
    int recv_len;
    u8 recv_buf[4];
    u32 start_time = 0;
    u32 debug_flag = 0;

    for (;;) {
        recv_len = sock_recvfrom(socket_fd, recv_buf, sizeof(recv_buf), 0, (struct sockaddr *)&remote_addr, &len);

        if (recv_len > 0) {

            if (start_time == 0) {
                start_time = timer_get_ms();
            }

            ++sock_rcv_cnt;
        }

        if (timer_get_ms() - start_time >= 30 * 1000) {
            if (debug_flag == 0) {
                debug_flag = 1;
                printf("\r\n\r\n STA MODE UDP RECV PKG CNT = %d\r\n\r\n", sock_rcv_cnt);
            }
        }
    }
}


static int udp_server_init(int port)
{
    static void *socket_fd;
    struct sockaddr_in local_addr = {0};

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(port);

    socket_fd = sock_reg(AF_INET, SOCK_DGRAM, 0, NULL, NULL);
    if (socket_fd == NULL) {
        printf("%s build socket fail\n",  __FILE__);
        return -1;
    }

    if (sock_bind(socket_fd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr))) {
        printf("%s sock_bind fail\n", __FILE__);
        return -1;
    }

    sock_set_recv_timeout(socket_fd, 3000);

    //创建线程，用于接收tcp_client的数据
    if (thread_fork("udp_recv_handler", 25, 512, 0, NULL, udp_recv_handler, socket_fd) != OS_NO_ERR) {
        printf("%s thread fork fail\n", __FILE__);
        return -1;
    }

    return 0;
}


__attribute__((weak))void fcc_res_handler(u8 res)
{
    log_info("%s, res = %d\n", __func__, res);
    if (res == true) {
        gpio_direction_output(CONFIG_RF_FCC_TRIGGER_IO_PORT, 1);
    } else {
        gpio_direction_output(CONFIG_RF_FCC_TRIGGER_IO_PORT, 0);
    }
}

#endif


