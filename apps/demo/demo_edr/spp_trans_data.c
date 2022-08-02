#include "app_config.h"
#include "spp_user.h"
#include "bt_common.h"
#include "btstack/avctp_user.h"
#include "system/timer.h"
#include "system/sys_time.h"

/// \cond DO_NOT_DOCUMENT
#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define TEST_SPP_DATA_RATE        0

#if TEST_SPP_DATA_RATE
#define SPP_TIMER_MS            (2)
#define TEST_SPP_SEND_SIZE      660
static u32 test_data_count;
static u8 spp_test_start;
#endif
/// \endcond

/**
 * \name 流控配置
 * \{
 */
#define SPP_DATA_RECIEVT_FLOW              0 /*!< 确定配置接口transport_spp_flow_cfg被调用后，在连接过程中通过接口transport_spp_flow_enable来控制流控开关 */
#define FLOW_SEND_CREDITS_NUM              1 /*!< 流控速度控制，控制命令中控制可接收的数据包个数，发送给对方的range(1~32) */
#define FLOW_SEND_CREDITS_TRIGGER_NUM      1 /*!< 触发更新控制命令的阈值，range(1 to <= FLOW_SEND_CREDITS_NUM) */
/* \} name */

/// \cond DO_NOT_DOCUMENT
int transport_spp_flow_enable(u8 en);
void rfcomm_change_credits_setting(u16 init_credits, u8 base);
int rfcomm_send_cretits_by_profile(u16 rfcomm_cid, u16 credit, u8 auto_flag);

static struct spp_operation_t *spp_api = NULL;
static u8 spp_state;
static u16 spp_channel;
/// \endcond

/* ----------------------------------------------------------------------------*/
/**
 * @brief spp传输发送数据
 * @param[in]  data: 准备发送的数据指针
 * @param[in]  len: 准备发送的数据长度
 * @return 0: 发送成功
 * @return other: 发送失败
 */
/* ----------------------------------------------------------------------------*/
int transport_spp_send_data(u8 *data, u16 len)
{
    if (spp_api) {
        /* log_info("spp_api_tx(%d) \n", len); */
        /* log_info_hexdump(data, len); */
        clear_sniff_cnt();
        return spp_api->send_data(NULL, data, len);
    }
    return SPP_USER_ERR_SEND_FAIL;
}

/* ----------------------------------------------------------------------------*/
/**
 * @brief 检查spp是否允许发送数据
 * @param[in]  len: 准备发送的数据长度
 * @return 1: 允许发送
 * @return 0: 发送忙碌
 */
/* ----------------------------------------------------------------------------*/
int transport_spp_send_data_check(u16 len)
{
    if (spp_api) {
        if (spp_api->busy_state()) {
            return 0;
        }
    }
    return 1;
}

/* ----------------------------------------------------------------------------*/
/**
 * @brief spp传输状态回调
 * @param[in]  state: 连接状态
 */
/* ----------------------------------------------------------------------------*/
static void transport_spp_state_cbk(u8 state)
{
    spp_state = state;

    switch (state) {
    case SPP_USER_ST_CONNECT:
        log_info("SPP_USER_ST_CONNECT ~~~\n");
        break;
    case SPP_USER_ST_DISCONN:
        log_info("SPP_USER_ST_DISCONN ~~~\n");
        spp_channel = 0;
        break;
    default:
        break;
    }
}

/* ----------------------------------------------------------------------------*/
/**
 * @brief spp传输唤醒回调
 */
/* ----------------------------------------------------------------------------*/
static void transport_spp_send_wakeup(void)
{
    /* putchar('W'); */
}

/* ----------------------------------------------------------------------------*/
/**
 * @brief spp数据接收回调
 * @param[in]  priv: spp_channel
 * @param[in]  buf: 接收到的数据指针
 * @param[in]  len: 接收到的数据长度
 */
/* ----------------------------------------------------------------------------*/
static void transport_spp_recieve_cbk(void *priv, u8 *buf, u16 len)
{
    spp_channel = (u16)priv;
    log_info("spp_api_rx(%d) \n", len);
    log_info_hexdump(buf, len);

    clear_sniff_cnt();

#if TEST_SPP_DATA_RATE
    if ((buf[0] == 'A') && (buf[1] == 'F')) {
        spp_test_start = 1;//start
    } else if ((buf[0] == 'A') && (buf[1] == 'A')) {
        spp_test_start = 0;//stop
    }

    //loop send data for test
    if (transport_spp_send_data_check(len)) {
        transport_spp_send_data(buf, len);
    }
#endif
}

#if TEST_SPP_DATA_RATE

static void test_spp_send_data(void)
{
    u16 send_len = TEST_SPP_SEND_SIZE;
    if (transport_spp_send_data_check(send_len)) {
        test_data_count += send_len;
        transport_spp_send_data((u8 *)&test_data_count, send_len);
    }
}

static void test_timer_handler(void *p)
{
    static u32 t_sec = 0;

    if (SPP_USER_ST_CONNECT != spp_state) {
        test_data_count = 0;
        spp_test_start = 0;
        return;
    }

    if (spp_test_start) {
        test_spp_send_data();
    }

    if (timer_get_sec() <= t_sec) {
        return;
    }
    t_sec = timer_get_sec();

    if (test_data_count) {
        log_info("\n-spp_data_rate: %d bytes, %d kbytes-\n", test_data_count, test_data_count / 1024);
        test_data_count = 0;
    }
}
#endif

/* ----------------------------------------------------------------------------*/
/**
 * @brief 初始化spp传输接口
 * @note  需要在蓝牙协议栈初始化btstack_init()成功后收到BT_STATUS_INIT_OK消息后调用
 */
/* ----------------------------------------------------------------------------*/
void transport_spp_init(void)
{
#if (USER_SUPPORT_PROFILE_SPP==1)
    log_info("transport_spp_init\n");

    spp_state = 0;
    spp_get_operation_table(&spp_api);
    spp_api->regist_recieve_cbk(0, transport_spp_recieve_cbk);
    spp_api->regist_state_cbk(0, transport_spp_state_cbk);
    spp_api->regist_wakeup_send(NULL, transport_spp_send_wakeup);
#endif

#if TEST_SPP_DATA_RATE
    sys_timer_add(NULL, test_timer_handler, SPP_TIMER_MS);
#endif
}

/* ----------------------------------------------------------------------------*/
/**
 * @brief 断开SPP传输连接
 */
/* ----------------------------------------------------------------------------*/
void transport_spp_disconnect(void)
{
    if (SPP_USER_ST_CONNECT == spp_state) {
        log_info("transport_spp_disconnect\n");
        user_send_cmd_prepare(USER_CTRL_SPP_DISCONNECT, 0, NULL);
    }
}

static void timer_spp_flow_test(void)
{
    static u8 sw = 0;
    if (spp_channel) {
        sw = !sw;
        transport_spp_flow_enable(sw);
    }
}

/* ----------------------------------------------------------------------------*/
/**
 * @brief 配置SPP传输接收流控控制参数
 * @note  需要在蓝牙协议栈初始化btstack_init()前调用
 */
/* ----------------------------------------------------------------------------*/
void transport_spp_flow_cfg(void)
{
#if SPP_DATA_RECIEVT_FLOW
    rfcomm_change_credits_setting(FLOW_SEND_CREDITS_NUM, FLOW_SEND_CREDITS_TRIGGER_NUM);

    //for test
    /* sys_timer_add(0,timer_spp_flow_test,2000); */
#endif
}

/* ----------------------------------------------------------------------------*/
/**
 * @brief SPP传输接收流控开关
 * @param[in]  en: 1-打开  0-关闭
 * @return 0: 成功
 * @return other: 失败
 */
/* ----------------------------------------------------------------------------*/
int transport_spp_flow_enable(u8 en)
{
    int ret = -1;

#if SPP_DATA_RECIEVT_FLOW
    if (spp_channel) {
        ret = rfcomm_send_cretits_by_profile(spp_channel, en ? 0 : FLOW_SEND_CREDITS_NUM, !en);
        log_info("transport_spp_flow_enable:%02x,%d,%d\n", spp_channel, en, ret);
    }
#endif

    return ret;
}

