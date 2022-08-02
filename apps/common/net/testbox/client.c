#include "client.h"
#include "system/includes.h"
/* #include "os/os_compat.h" */
#include "btstack/avctp_user.h"
#include "server/audio_server.h"
#include "server/server_core.h"
#include "syscfg/syscfg_id.h"

#ifdef CONFIG_WIFIBOX_ENABLE

#ifdef  WB_CLIENT_DEBUG_ENABLE
#define     log_info(x, ...)     printf("[WB_PRC][INFO] " x " ", ## __VA_ARGS__)
#define     log_err(x, ...)      printf("[WB_PRC][ERR] " x " ", ## __VA_ARGS__)
#else
#define     log_info(...)
#define     log_err(...)
#endif

MODULE_VERSION_EXPORT(wb_client, WB_CLIENT_VERSION);


typedef struct {
    u8 serial[16];
    u32 conn_fail;
    u32 conn_cnt;
    u32 finish_cnt;
} stat_data_type;

static stat_data_type stat_data = {0};

typedef struct {
    struct server *dec_server;  //解码服务
    u8 dec_status;              //解码器运行标志位
    u8 *data_buf;               //音频数据buffer
    cbuffer_t data_cbuf;        //音频数据cycle buffer
    u32 total_size;             //音频文件总长度
    u32 recv_size;              //已接收音频数据长度
    u32 play_size;              //当前播放进度
    u8 dec_ready_flag;          //解码器就绪标志位
    OS_SEM audio_sem;
    int init_data_len;
    int init_tmp_len;
    u8 *init_read_ptr;
} audio_hdl_type;

static  audio_hdl_type audio_hdl = {0};

static struct {
    u8 xosc[2];
    test_option_type test_option;
    u8 dut_reset_flag;
    u8 freq_adj_flag;
    u8 bt_freq_stat_flag;
    u8 test_pass_flag;
    OS_SEM conn_sem;
    OS_SEM freq_sem;
    FSM_T thradj_fsm;
    FSM_T test_fsm;
    u8 senthr_adj_flag;
    u8 *remote_mac;
    wbcp_operations_interface_type *wbcp_ops;
} client = {0};

#define     __this      (&client)

int bt_fre_offset_get();

static void exit_fsm_process(void);
static void bt_freq_test_before(void);
static void bt_freq_test_after(void);
static int wifibox_bt_connect_pend(int timeout);
static s32 wifibox_bt_freq_finish_pend(int timeout);
static s8 open_audio_dec(void);
/* static u32 wifibox_read_stat_data(stat_data_type *data); */
/* static s32 wifibox_write_stat_data(stat_data_type *data); */

static s8 pwrthr_adj_event_handler(void *priv);
static s8 senthr_adj_event_handler(void *priv);
static void thradj_fsm_err(void *priv);

static s8 pwr_test_event_handler(void *priv);
static s8 sen_test_event_handler(void *priv);
static s8 freq_test_event_handler(void *priv);
static s8 finish_test_event_handler(void *priv);
static void test_fsm_err(void *priv);
void __attribute__((weak)) wifibox_result_deal(u8 res);

static FsmTable_T thradj_fsm_tal[] = {
    {PWRTHR_ADJ_EVENT,  PWRTHR_ADJ_STATUS,  pwrthr_adj_event_handler,  SENTHR_ADJ_STATUS,  0},
    {SENTHR_ADJ_EVENT,  SENTHR_ADJ_STATUS,  senthr_adj_event_handler,  PWRTHR_ADJ_STATUS,  0},
};

static FsmTable_T test_fsm_tal[] = {
    {PWR_TEST_EVENT,     PWR_TEST_STATUS,     pwr_test_event_handler,     SEN_TEST_STATUS,     0},
    {SEN_TEST_EVENT,     SEN_TEST_STATUS,     sen_test_event_handler,     FREQ_TEST_STATUS,    0},
    /* {FREQ_TEST_EVENT,    FREQ_TEST_STATUS,    freq_test_event_handler,    PWR_TEST_STATUS,  0}, */
    {FREQ_TEST_EVENT,    FREQ_TEST_STATUS,    freq_test_event_handler,    FINISH_TEST_STATUS,  0},
    {FINISH_TEST_EVENT,  FINISH_TEST_STATUS,  finish_test_event_handler,  PWR_TEST_STATUS,     0},
};


static void fsm_tbl_init(void *priv)
{
    wbcp_conn_params_type *conn_params = (wbcp_conn_params_type *)priv;
    fsm_timeout_type *fsm_timeout = &conn_params->fsm_timeout;
    test_option_type *test_option = &conn_params->test_option;
    FsmTable_T fsm_tal[4] = {0};
    static FsmTable_T init_fsm_tal[4] = {0};
    u8 idx = 0;
    static u8 init = 0;

    if (!init) {
        init = 1;
        memcpy(&init_fsm_tal, &test_fsm_tal, sizeof(test_fsm_tal));
    } else {
        memcpy(&test_fsm_tal, &init_fsm_tal, sizeof(test_fsm_tal));
    }

    __this->dut_reset_flag = conn_params->dut_reset_flag;
    __this->freq_adj_flag = conn_params->freq_adj_flag;
    memcpy(&__this->test_option, test_option, sizeof(test_option_type));

    thradj_fsm_tal[0].timeout = fsm_timeout->thradj_fsm[0];
    thradj_fsm_tal[1].timeout = fsm_timeout->thradj_fsm[1];

    for (u8 i = 0; i < ARRAY_SIZE(fsm_timeout->test_fsm); i++) {
        test_fsm_tal[i].timeout = fsm_timeout->test_fsm[i];
    }

    if (__this->test_option.wifi_pwr_option) {
        memcpy(&fsm_tal[idx], &test_fsm_tal[0], sizeof(FsmTable_T));
        idx++;
    }

    if (__this->test_option.wifi_sen_option || __this->test_option.wifi_rx_rate_option) {
        memcpy(&fsm_tal[idx], &test_fsm_tal[1], sizeof(FsmTable_T));
        if (!__this->test_option.wifi_pwr_option) {
            test_fsm_tal[ARRAY_SIZE(test_fsm_tal) - 1].NextState = test_fsm_tal[1].CurState;
        }
        idx++;
    } else {
        if (__this->test_option.wifi_pwr_option) {
            fsm_tal[0].NextState = test_fsm_tal[1].NextState;
        } else {
            test_fsm_tal[ARRAY_SIZE(test_fsm_tal) - 1].NextState = test_fsm_tal[ARRAY_SIZE(test_fsm_tal) - 2].CurState;
        }
    }
    memcpy(&fsm_tal[idx], &test_fsm_tal[2], sizeof(FsmTable_T) * 2);
    memcpy(test_fsm_tal, fsm_tal, sizeof(test_fsm_tal));

    FSM_Init(&__this->test_fsm, test_fsm_tal, idx + 2, test_fsm_tal[0].CurState, test_fsm_err);
}


static s8 pwrthr_adj_event_handler(void *priv)
{
    log_info("==================== STATUS : PWRTHR_ADJ_STATUS ====================\n\n");
    if (__this->wbcp_ops->start_send_test_data(__this->remote_mac, PWRTHR_SEND_TEST_DATA_PERIOD, 0) == 0) {
        return FSM_NEXT_STATE;
    }
    return FSM_ERR;
}


static s8 senthr_adj_event_handler(void *priv)
{
    log_info("==================== STATUS : SENTHR_ADJ_STATUS ====================\n\n");
    stat_res_type *res = NULL;
    event_data_type new_event = {0};
    event_data_type *event_data = (event_data_type *)priv;
    senthr_adj_event_params_type *params = &event_data->params.senthr_adj_event_params;

    if (!__this->senthr_adj_flag) {
        __this->senthr_adj_flag = 1;
        __this->wbcp_ops->stop_send_test_data();

        new_event.event = SENTHR_ADJ_EVENT;
        if (__this->wbcp_ops->send_event_data(&new_event, __this->remote_mac, 1) != 0) {
            log_err("SEND_FAIL : SENTHR_ADJ_EVENT");
            goto _exit_;
        }
        return FSM_KEEP_STATE;
    }

    switch (params->stat) {
    case 1 :
        log_info("SENTHR_STEP : OPEN STAT");
        __this->wbcp_ops->start_stat_prop(0, 0);
        return FSM_KEEP_STATE;

    case 2 :
        res = __this->wbcp_ops->stop_stat_prop();
        log_info("SENTHR_STEP : SEND RES, pcnt = %d, sen = %d.", res->pcnt, res->pwr);
        new_event.event = SENTHR_ADJ_EVENT;
        new_event.params.senthr_adj_event_params.pcnt = res->pcnt;
        new_event.params.senthr_adj_event_params.sen = res->pwr;
        if (__this->wbcp_ops->send_event_data(&new_event, __this->remote_mac, 1) != 0) {
            log_err("SEND_FAIL : SENTHR_ADJ_EVENT");
            goto _exit_;
        }
        return FSM_KEEP_STATE;

    case 3 :
        log_info("SENTHR_STEP : FINISH ADJUST");
        cpu_reset();
    /* return FSM_NEXT_STATE;  */

    default :
        break;
    }

_exit_:
    __this->senthr_adj_flag = 0;
    return FSM_ERR;
}


static void thradj_fsm_err(void *priv)
{
    log_info("============================== THR_ADJ_FSM ERR =============================\n\n");
    cpu_reset();
}


static s8 pwr_test_event_handler(void *priv)
{
    log_info("==================== STATUS : PWR_TEST_STATUS =====================\n\n");
    if (__this->wbcp_ops->start_send_test_data(__this->remote_mac, PWRTEST_SEND_TEST_DATA_PERIOD, 0) == 0) {
        return FSM_NEXT_STATE;
    }
    return FSM_ERR;
}


static s8 sen_test_event_handler(void *priv)
{
    log_info("==================== STATUS : SEN_TEST_STATUS =====================\n\n");

    __this->wbcp_ops->stop_send_test_data();
    __this->wbcp_ops->start_stat_prop(0, 0);

    return FSM_NEXT_STATE;
}


static s8 freq_test_event_handler(void *priv)
{
    log_info("==================== STATUS : FREQ_TEST_STATUS ====================\n\n");

    event_data_type *event = (event_data_type *)priv;
    freq_test_event_params_type *params = &event->params.freq_test_event_params;
    event_data_type new_event = {0};
    freq_test_event_params_type *new_params = &new_event.params.freq_test_event_params;
    stat_res_type *res = NULL;
    const u8 *bt_mac = NULL;

    if (__this->test_option.wifi_pwr_option) {
        __this->wbcp_ops->stop_send_test_data();
    }

    if (__this->test_option.wifi_sen_option || __this->test_option.wifi_rx_rate_option) {
        res = __this->wbcp_ops->stop_stat_prop();
        new_params->pcnt = res->pcnt;
        new_params->sen = res->pwr;
        log_info("SEN_RES : pcnt = %d, sen = %d.", res->pcnt, res->pwr);
    }

    if (__this->test_option.wifi_freq_option) {
        const u8 *bt_get_mac_addr();
        bt_mac = bt_get_mac_addr();
        if (bt_mac == NULL) {
            return FSM_ERR;
        }
        put_buf(bt_mac, MAC_ADDR_SIZE);
        memcpy(&new_params->bt_mac, bt_mac, MAC_ADDR_SIZE);
    }

    new_event.event = FREQ_TEST_EVENT;
    if (__this->wbcp_ops->send_event_data(&new_event, __this->remote_mac, 1) != 0) {
        log_err("SEND_FAIL : FREQ_TEST_EVENT.");
        return FSM_ERR;
    }

    if (__this->test_option.wifi_freq_option) {
        /* wifi_off(); */
        bt_freq_test_before();
        /* if (wifibox_bt_connect_pend(BT_FREQ_CONN_TIMEOUT) != OS_NO_ERR) { */
        /*     log_err("BT CONN TIMEOUT.\n"); */
        /*     return FSM_ERR; */
        /* } */

        if (wifibox_bt_freq_finish_pend(BT_FREQ_STAT_TIMEOUT) != OS_NO_ERR) {
            log_err("BT FREQ TEST TIMEOUT.\n");
            return FSM_ERR;
        }
        bt_freq_test_after();
    }

    /* exit_fsm_process(); */
    return FSM_NEXT_STATE;
}


static s8 finish_test_event_handler(void *priv)
{
    log_info("=================== STATUS : FINISH_TEST_STATUS ===================\n\n");

    s32 err = 0;
    wifi_freq_offset_type wifi_offset = {0};
    event_data_type *event = (event_data_type *)priv;
    finish_test_event_params_type *params = &event->params.finish_test_event_params;
    event_data_type new_event = {0};

    wifibox_result_deal(params->result);

    if (__this->freq_adj_flag) {
        wifi_offset.offset = bt_fre_offset_get();
        if (wifi_offset.offset < FREQ_ADJUST_LIMIT && \
            wifi_offset.offset > -FREQ_ADJUST_LIMIT) {
            wifi_get_xosc(wifi_offset.xosc);
            wifi_offset.crc = CRC16(&wifi_offset.offset, sizeof(wifi_freq_offset_type) - sizeof(s16));
            err = syscfg_write(CFG_WIFI_FRE_OFFSET, &wifi_offset, sizeof(wifi_freq_offset_type));
            if (err == sizeof(wifi_freq_offset_type)) {
                log_info("syscfg_write : fre = %d, osc = %d/%d\n", wifi_offset.offset, wifi_offset.xosc[0], wifi_offset.xosc[1]);
            } else {
                log_err("syscfg_write : err = %d\n", err);
            }
        } else {
            log_info("freq = %d, out of FREQ_ADJUST_LIMIT\n", wifi_offset.offset);
        }
    }

#ifdef AUDIO_TRANSFER_ENABLE
    if (!params->result) {
        os_sem_set(&audio_hdl.audio_sem, 0);
        if (os_sem_pend(&audio_hdl.audio_sem, AUDIO_RECV_TIMEOUT) != OS_NO_ERR) {
            log_err("AUDIO RECV ERR");
            return FSM_ERR;
        }
        log_info("AUDIO RECV COMPLETE");
        __this->test_pass_flag = 1;
        open_audio_dec();
    }
#endif
    stat_data.finish_cnt++;
    os_time_dly(WBCP_WAIT_ACK_TIMEOUT);
    exit_fsm_process();
    return FSM_NEXT_STATE;
}


static void test_fsm_err(void *priv)
{
    log_info("============================== FSM ERR =============================\n\n");

    FSM_StateTransfer(&__this->test_fsm, __this->test_fsm.FsmTable[0].CurState);
    exit_fsm_process();
}


static void exit_fsm_process(void)
{
    log_info("======================== EXIT FSM PROCESS =========================\n\n");
    /* wifibox_write_stat_data(&stat_data); */
    if (__this->test_pass_flag) {
        __this->wbcp_ops->rx_shield();
    } else {
#ifdef WB_DUT_RESET_ENABLE
        /* if (__this->dut_reset_flag) { */
        cpu_reset();
        /* } */
#endif
    }
    __this->senthr_adj_flag = 0;
    __this->wbcp_ops->stop_send_test_data();
    __this->wbcp_ops->stop_stat_prop();
    bt_freq_test_after();

    /* if (!wifi_is_on()) { */
    /*     wifi_on(); */
    /*     while (!wifi_is_on()); */
    /* } */
    __this->wbcp_ops->break_conn();
}


static void wbcp_event_handler(event_data_type *event_data)
{
    switch (event_data->event) {
    case PWRTHR_ADJ_EVENT :
    case SENTHR_ADJ_EVENT :
    case FINISH_ADJ_EVENT :
        FSM_EventHandle(&__this->thradj_fsm, event_data->event, event_data);
        break;

    case PWR_TEST_EVENT :
    case SEN_TEST_EVENT :
    case FREQ_TEST_EVENT :
    case FINISH_TEST_EVENT :
        FSM_EventHandle(&__this->test_fsm, event_data->event, event_data);
        break;

    default :
        break;
    }
}


static void wbcp_event_handle_task(void *priv)
{
    s32 msg[32] = {0}, timer_id = 0;
    event_data_type *event_data = NULL;

    for (;;) {
        if (os_taskq_pend(NULL, msg, ARRAY_SIZE(msg)) != OS_TASKQ) {
            continue;
        }

        if (__this->test_pass_flag) {
            continue;
        }

        switch (msg[0]) {
        case ENTER_TEST_SUCC_MSG:
            log_info("ENTER_TEST_SUCC_MSG");
            __this->remote_mac = (u8 *)msg[1];
            log_info("======================= ENTER TEST_MODE SUCC =======================\n\n");
            put_buf(__this->remote_mac, MAC_ADDR_SIZE);
            stat_data.conn_cnt++;
            timer_id = sys_timeout_add(NULL, exit_fsm_process, RECV_EVENT_TIMEOUT);
            fsm_tbl_init((u8 *)msg[2]);
            break;

        case ENTER_TEST_FAIL_MSG :
            log_info("ENTER_TEST_FAIL_MSG");
            log_info("======================= ENTER TEST_MODE FAIL=======================\n\n");
            stat_data.conn_fail++;
            /* wifibox_write_stat_data(&stat_data); */
#ifdef WB_DUT_RESET_ENABLE
            /* if (msg[1]) { */
            cpu_reset();
            /* } */
#endif
            __this->wbcp_ops->break_conn();
            break;

        case NEW_EVENT_MSG:
            event_data = (event_data_type *)&msg[1];
            log_info("NEW_EVENT_MSG : %d.\n", event_data->event);
            if (timer_id && (event_data->event == test_fsm_tal[0].event || event_data->event == thradj_fsm_tal[0].event)) {
                sys_timeout_del(timer_id);
                timer_id = 0;
            }
            wbcp_event_handler(event_data);
            break;

        default :
            break;
        }
    }
}


static void wbcp_audio_data_handler(void *priv)
{
    u32 wlen = 0, rlen = 0, temp_len = 0;
    audio_data_type *audio_data = (audio_data_type *)priv;

    audio_hdl.total_size = audio_data->total_size;
    temp_len = audio_hdl.total_size - audio_hdl.recv_size;
    rlen = (temp_len < AUDIO_DATA_SIZE) ? temp_len : AUDIO_DATA_SIZE;

    wlen = cbuf_write(&audio_hdl.data_cbuf, audio_data->data, rlen);
    if (wlen) {
        putchar('(');
        audio_hdl.recv_size += wlen;
        log_info("AUDIO_RECV :  %d / %d\n", audio_hdl.recv_size, audio_hdl.total_size);

        //音频下载成功，打开解码器，开始播放
        if (audio_hdl.recv_size >= audio_hdl.total_size && audio_hdl.dec_status == 0) {
            os_sem_post(&audio_hdl.audio_sem);
            //保存cycle buffer初始参数，用于循环播放音频
            audio_hdl.init_data_len = audio_hdl.data_cbuf.data_len;
            audio_hdl.init_tmp_len  = audio_hdl.data_cbuf.tmp_len;
            audio_hdl.init_read_ptr = audio_hdl.data_cbuf.read_ptr;
            audio_hdl.dec_status = 1;
        }
    }
}


static int audio_vfs_fread(void *priv, void *data, u32 len)
{
    audio_hdl_type *hdl = (audio_hdl_type *)priv;
    u32 rlen = 0;

    if (hdl->dec_ready_flag) {
        if (hdl->total_size - hdl->play_size < 512) {
            len = hdl->total_size - hdl->play_size;
        }

        rlen = cbuf_read(&hdl->data_cbuf, data, len);
        if (rlen) {
            putchar(')');
            hdl->play_size += rlen;
            log_info("AUDIO_PLAY :  %d / %d\n", hdl->play_size, hdl->total_size);

            if (hdl->play_size >= hdl->total_size) {
                hdl->dec_ready_flag = 0;
                log_info("AUDIO_PLAY_COMPLETE\n");
            }
        }
    }

    return ((hdl->play_size >= hdl->total_size) ? 0 : len);
}


static int audio_vfs_flen(void *priv)
{
    audio_hdl_type *hdl = (audio_hdl_type *)priv;
    return hdl->total_size;
}


static int audio_vfs_seek(void *priv, u32 offset, int orig)
{
    audio_hdl_type *hdl = (audio_hdl_type *)priv;

    if (offset > 0) {
        hdl->dec_ready_flag = 1; //解码器开始偏移数据后，再开始播放，避免音频播放开端吞字
    }
    return 0;
}


static const struct audio_vfs_ops audio_vfs_ops = {
    .fread  = audio_vfs_fread,
    .flen   = audio_vfs_flen,
    .fseek  = audio_vfs_seek,
};


static s8 open_audio_dec(void)
{
    union audio_req req = {0};

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = 100;
    req.dec.output_buf      = NULL;
    req.dec.output_buf_len  = 12 * 1024;
    req.dec.channel         = 0;
    req.dec.sample_rate     = 0;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = &audio_vfs_ops;
    req.dec.file            = (FILE *)&audio_hdl;
    req.dec.dec_type 		= "mp3";
    req.dec.sample_source   = "dac";

    //打开解码器
    if (server_request(audio_hdl.dec_server, AUDIO_REQ_DEC, &req) != 0) {
        return -1;
    }

    //开始解码
    req.dec.cmd = AUDIO_DEC_START;
    if (server_request(audio_hdl.dec_server, AUDIO_REQ_DEC, &req) != 0) {
        return -1;
    }

    return 0;
}


static int close_audio_dec(void)
{
    union audio_req req = {0};

    req.dec.cmd = AUDIO_DEC_STOP;
    if (audio_hdl.dec_server) {

        //关闭解码器
        log_info("stop dec.\n");
        server_request(audio_hdl.dec_server, AUDIO_REQ_DEC, &req);

        //关闭解码服务
        /* log_info("close audio_server.\n"); */
        /* server_close(audio_hdl.dec_server); */
        /* audio_hdl.dec_server = NULL; */

        audio_hdl.dec_status = 0;
        audio_hdl.recv_size  = 0;
        audio_hdl.play_size  = 0;

        //循环播放音频, 恢复cycle buffer读指针
        audio_hdl.data_cbuf.data_len = audio_hdl.init_data_len;
        audio_hdl.data_cbuf.tmp_len  = audio_hdl.init_tmp_len;
        audio_hdl.data_cbuf.read_ptr = audio_hdl.init_read_ptr;
        open_audio_dec();

        //单次播放音频，清除cycle buffer
        /* cbuf_clear(&audio_hdl.data_cbuf); */
    }

    return 0;
}


static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    audio_hdl_type *hdl = (audio_hdl_type *)priv;

    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        log_info("AUDIO_SERVER_EVENT_ERR\n");
        break;

    case AUDIO_SERVER_EVENT_END:
        log_info("AUDIO_SERVER_EVENT_END\n");
        close_audio_dec();
        break;

    case AUDIO_SERVER_EVENT_CURR_TIME:
        log_info("AUDIO_PLAY_TIME : %d\n", argv[1]);
        break;

    default:
        break;
    }
}

static s8 audio_play_init(void)
{
    audio_hdl.data_buf = (char *)calloc(1, AUDIO_BUFFER_SIZE);
    if (audio_hdl.data_buf == NULL) {
        log_err("audio buffer malloc error.\n");
        return -1;
    }

    cbuf_init(&audio_hdl.data_cbuf, audio_hdl.data_buf, AUDIO_BUFFER_SIZE);

    audio_hdl.dec_server = server_open("audio_server", "dec");
    if (!audio_hdl.dec_server) {
        log_err("audio server open fail.\n");
        return -1;
    } else {
        log_info("open success");
    }

    server_register_event_handler(audio_hdl.dec_server, &audio_hdl, dec_server_event_handler);

    return 0;
}


int wifibox_client_init(void)
{
    static u8 init = 0;

#ifdef WB_ASSIST_ENABLE
    u8 is_assist = 1;
#else
    u8 is_assist = 0;
#endif

    if (init) {
        log_info("wifibox_client already init");
        return 0;
    }

    FSM_Init(&__this->thradj_fsm, thradj_fsm_tal, ARRAY_SIZE(thradj_fsm_tal), PWRTHR_ADJ_STATUS, thradj_fsm_err);
    /* FSM_Init(&__this->test_fsm, test_fsm_tal, ARRAY_SIZE(test_fsm_tal), PWR_TEST_STATUS, test_fsm_err); */

    get_wbcp_operations_interface(&__this->wbcp_ops);
    if (__this->wbcp_ops->init(WB_CLIENT_MODE, is_assist) != 0) {
        log_err("wbcp init fail.\n");
    }

    os_sem_create(&__this->conn_sem, 0);
    os_sem_create(&__this->freq_sem, 0);
    os_sem_create(&audio_hdl.audio_sem, 0);

    if (thread_fork("wbcp_event_handle_task", 14, 3 * 1024, 1024, NULL, wbcp_event_handle_task, NULL) != OS_NO_ERR) {
        log_err("%s thread fork fail\n", __FILE__);
        return -1;
    }

#ifdef AUDIO_TRANSFER_ENABLE
    audio_play_init();
    __this->wbcp_ops->regist_audio_data_user_handler(wbcp_audio_data_handler);
#endif

    __this->wbcp_ops->regist_usr_handler("wbcp_event_handle_task");

    /* if (wifibox_read_stat_data(&stat_data) == 0) { */
    /*     stat_data.finish_cnt = 0; */
    /*     stat_data.conn_cnt = 0; */
    /*     stat_data.conn_fail = 0; */
    /* } */
    log_info("STAT_CNT : %d / %d, %d", stat_data.finish_cnt, stat_data.conn_cnt, stat_data.conn_fail);

    init = 1;

    return 0;
}


u32 user_send_cmd_prepare(USER_CMD_TYPE cmd, u16 param_len, u8 *param);
static void bt_freq_test_before(void)
{
    if (__this->bt_freq_stat_flag == 0) {
        log_info("bt_freq_test_before.\n");
        __this->bt_freq_stat_flag = 1;

        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_POWER_ON, 0, NULL);
    }
}


static void bt_freq_test_after(void)
{
    if (__this->bt_freq_stat_flag) {
        log_info("bt_freq_test_after.\n");
        __this->bt_freq_stat_flag = 0;

        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    }
}


int wifibox_bt_connect_post(void)
{
    log_info("wifibox_bt_connect_post\n");
    return os_sem_post(&__this->conn_sem);
}


static int wifibox_bt_connect_pend(int timeout)
{
    log_info("wifibox_bt_connect_pend\n");
    os_sem_set(&__this->conn_sem, 0);
    return os_sem_pend(&__this->conn_sem, timeout);
}


int wifibox_bt_freq_finish_post(void)
{
    log_info("wifibox_bt_freq_finish_post\n");
    return os_sem_post(&__this->freq_sem);
}


static s32 wifibox_bt_freq_finish_pend(int timeout)
{
    log_info("wifibox_bt_freq_finish_pend\n");
    int ret = -1;
    os_sem_set(&__this->freq_sem, 0);
    ret = os_sem_pend(&__this->freq_sem, timeout);
    log_info("bt_freq test finish.\n");
    return ret;
}


void __attribute__((weak)) wifibox_result_deal(u8 res)
{
    //测试结果处理回调，可添加非阻塞操作
    if (!res) {
        log_info("wifibox test pass");
    } else {
        log_info("wifibox test nopass");
    }
}


static void wifi_set_xosc(u8 *xosc)
{
    memcpy(__this->xosc, xosc, sizeof(__this->xosc));
}


static void fre_to_xosc(s8 fre, u8 *cur_osc, u8 *cal_osc)
{
    u8 tmp = 0;
    tmp = (((1000000 * (cur_osc[0] + cur_osc[1])) + (195863 * fre) - (466 * fre * fre)) / 1000000);
    cal_osc[0] = tmp / 2;
    cal_osc[1] = (tmp / 2) + ((tmp % 2) ? 1 : 0);
    printf("tmp = %d, %d/%d\n", tmp, cal_osc[0], cal_osc[1]);
}


void __attribute__((weak)) wf_rf_pll_offset_adj(long foffet_khz, char en)
{

}


u8 wifi_freq_adjust(void)
{
    u16 crc = 0;
    s32 ret = 0;
    u8 cal_xosc[2] = {0};
    wifi_freq_offset_type wifi_offset = {0};

    ret = syscfg_read(CFG_WIFI_FRE_OFFSET, &wifi_offset, sizeof(wifi_freq_offset_type));
    crc = CRC16(&wifi_offset.offset, sizeof(wifi_freq_offset_type) - sizeof(s16));
    if (ret == sizeof(wifi_freq_offset_type) && crc == wifi_offset.crc) {
        log_info("^^^syscfg_read : fre = %d, osc = %d/%d\n", wifi_offset.offset, wifi_offset.xosc[0], wifi_offset.xosc[1]);
        if (wifi_offset.offset) {
            wf_rf_pll_offset_adj(-wifi_offset.offset, 0);
            /* fre_to_xosc(wifi_offset.offset, wifi_offset.xosc, cal_xosc); */
            /* wifi_set_xosc(cal_xosc); */
        }
        return 0;
    }
    return -1;
}


/* =============================================调试使用======================================== */
#if 0

#include "json_c/json_tokener.h"
#include "fs/fs.h"
#include "math.h"
#include <stdlib.h>
/* #include "hmc1122_attenuator.h" */

u8 dut_pa[7] = {0};
int dut_xosc_cls = 0;
int dut_xosc_crs = 0;
int dut_tx_attn = 0;
int dut_rx_attn = 0;


void wifi_get_xosc(u8 *xosc)
{
    memcpy(xosc, __this->xosc, sizeof(__this->xosc));
}


static int wifibox_dut_read_config_info(void)
{
    FILE *fd;
    char *buf = NULL;
    json_object *new_obj;
    u32 cfg_pa = 0;
    char *DUT_PA = NULL;
    char *DUT_XOSC_CLS = NULL;
    char *DUT_XOSC_CRS = NULL;
    char *TX_ATTN = NULL;
    char *RX_ATTN = NULL;

    buf = malloc(1024 * 1);
    if (!buf) {
        log_err("malloc error: %d.\n", __LINE__);
    }

    fd = fopen("mnt/sdfile/res/cfg/dut_config.txt", "r");
    if (!fd) {
        log_err("wifibox config : open file error.\n");
        return -1;
    }

    fread(buf, 1024, 1, fd);
    fclose(fd);

    new_obj = json_tokener_parse((const char *)buf);
    if (!new_obj) {
        log_err("wifibox config : json_tokener_parse error.\n");
        return -1;
    }

    DUT_PA       = json_object_get_string(json_object_object_get(new_obj, "PA"));
    DUT_XOSC_CLS = json_object_get_string(json_object_object_get(new_obj, "XOSC_CLS"));
    DUT_XOSC_CRS = json_object_get_string(json_object_object_get(new_obj, "XOSC_CRS"));
    TX_ATTN = json_object_get_string(json_object_object_get(new_obj, "TX_ATTN"));
    RX_ATTN = json_object_get_string(json_object_object_get(new_obj, "RX_ATTN"));

    cfg_pa = atoi(DUT_PA);
    for (int i = 0; i < ARRAY_SIZE(dut_pa); i++) {
        dut_pa[i] = cfg_pa / ((int)pow(10, ARRAY_SIZE(dut_pa) - 1 - i)) % 10;
    }

    dut_xosc_cls = atoi(DUT_XOSC_CLS);
    dut_xosc_crs = atoi(DUT_XOSC_CRS);
    dut_tx_attn = atoi(TX_ATTN);
    dut_rx_attn = atoi(RX_ATTN);

    __this->xosc[0] = dut_xosc_cls;
    __this->xosc[1] = dut_xosc_crs;

    log_info("=====================DUT_CFG_INFO=====================");
    put_buf(dut_pa, sizeof(dut_pa));
    log_info("dut_xosc_cls = %d, dut_xosc_crs = %d.\n", dut_xosc_cls, dut_xosc_crs);
    log_info("dut_tx_attn = %d, dut_rx_attn = %d.\n", dut_tx_attn, dut_rx_attn);
    log_info("=====================DUT_CFG_INFO=====================");

    json_object_put(new_obj);

    if (buf) {
        free(buf);
    }

    return 0;
}


void wifibox_dut_config(void)
{
    if (wifibox_dut_read_config_info() == 0) {
        log_info("DUT SET : PA = %d,%d,%d,%d,%d,%d,%d; XOSC_L = %d, XSOC_R = %d.\n", dut_pa[0], dut_pa[1], dut_pa[2], dut_pa[3], dut_pa[4], dut_pa[5], dut_pa[6], dut_xosc_cls, dut_xosc_crs);
        //衰减器初始化
        /* struct attenuator_operation_t *attn_hdl = NULL; */
        /* get_attn_hdl(&attn_hdl); */
        /* if (attn_hdl) { */
        /*     attn_hdl->init(); */
        /*     attn_hdl->set_tx_state(dut_tx_attn); */
        /*     attn_hdl->set_rx_state(dut_rx_attn); */
        /* }  */
    } else {
        log_err("wifibox_dut_config fail.\n");
    }
}


int wifi_get_pa_trim_data(u8 *pa_data)
{
    memcpy(pa_data, dut_pa, 7);
    return 1;//不使用自动tune
}


#endif

#endif




