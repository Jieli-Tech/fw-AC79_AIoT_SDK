#include "app_config.h"
#include "wifi/wifi_connect.h"
#include "system/includes.h"
#include "kcp/ikcp.h"

#ifdef USE_WIFI_RAW_KCP_TEST

#define KCP_SEM_TIMEOUT 150
static ikcpcb *kcp_hdl;

static int ikcp_sem_del(void *psem)
{
    return os_sem_del(psem, OS_DEL_ALWAYS);
}

static int ikcp_sem_post(void *psem)
{
    /* if (os_sem_query(psem) == 0) { */
//        os_sem_set(psem,0);
    os_sem_post(psem);
    /* } */

    return 0;
}
static int ikcp_sem_pend(void *psem, int timeout)
{
    return os_sem_pend(psem, timeout);
}

static int ikcp_mutex_lock(void *mutex)
{
    return os_mutex_pend(mutex, 0);
}
static void ikcp_mutex_unlock(void *mutex)
{
    os_mutex_post(mutex);
}

static void ikcp_mutex_del(void *mutex)
{
    os_mutex_del(mutex, OS_DEL_ALWAYS);
}


static void ikcp_writelog(struct IKCPCB *kcp, char *fmt, va_list va, void *user)
{
    puts("IKCP_DBG: ");
    vprintf(fmt, va);
    puts("\n");
}

static unsigned int iclock(void)
{
    return timer_get_ms();
}


static int *udp_output(const char *buf, int len, struct IKCPCB *kcp, void *user)
{
    memcpy(wifi_get_payload_ptr(), buf, len);
    wifi_send_data(len, WIFI_TXRATE_1M);

    return 0;
}

static void kcp_udp_create(u32 conv)
{
    kcp_hdl = ikcp_create(conv, (void *)NULL);

    kcp_hdl->output = (int (*)(const char *, int, struct IKCPCB *, void *))udp_output;
    ikcp_wndsize(kcp_hdl, 256, 512);    //set snd recv windows size
    ikcp_nodelay(kcp_hdl, 0, 1, 2, 0);    //set kcp mode

    ikcp_setmtu(kcp_hdl, 2510, 2510);

//    ikcp_set_writelog(kcp_hdl, ikcp_writelog, IKCP_LOG_OUTPUT|IKCP_LOG_INPUT|IKCP_LOG_SEND|IKCP_LOG_RECV|IKCP_LOG_IN_DATA|\
//                      IKCP_LOG_IN_ACK|IKCP_LOG_IN_PROBE|IKCP_LOG_IN_WINS|IKCP_LOG_OUT_DATA|IKCP_LOG_OUT_ACK	|IKCP_LOG_OUT_PROBE|IKCP_LOG_OUT_WINS);


    OS_MUTEX *pmutex = (OS_MUTEX *)malloc(sizeof(OS_MUTEX));
    os_mutex_create(pmutex);
    ikcp_set_mutex_lock_func(kcp_hdl, ikcp_mutex_lock, ikcp_mutex_unlock, ikcp_mutex_del, pmutex);

    OS_SEM *psendsem = (OS_SEM *)malloc(sizeof(OS_SEM));
    os_sem_create(psendsem, 0);
    ikcp_set_send_block(kcp_hdl, 0, ikcp_sem_post, ikcp_sem_pend, ikcp_sem_del, KCP_SEM_TIMEOUT, psendsem);

    OS_SEM *precvsem = (OS_SEM *)malloc(sizeof(OS_SEM));
    os_sem_create(precvsem, 0);
    ikcp_set_recv_block(kcp_hdl, 0, ikcp_sem_post, ikcp_sem_pend, ikcp_sem_del,  KCP_SEM_TIMEOUT, precvsem);

    ikcp_update(kcp_hdl, iclock);
}


static void kcp_send_test(void *p)
{
    puts("kcp_send test \r\n\r\n ");


#define KCP_TEST_SEND_BUF_SIZE 123*1024+4567

    int time_hdl = 0;
    unsigned int total_send = 0;
    int ret;
    char *send_buf;
    unsigned int send_len;


    kcp_udp_create(0x56444c4a);


    send_buf = (char *)malloc(KCP_TEST_SEND_BUF_SIZE);
    for (ret = 0; ret < KCP_TEST_SEND_BUF_SIZE; ret++) {
        send_buf[ret] = ret;
    }

    while (1) {
        /*os_time_dly(1);*/

        if (time_lapse((u32 *)(&time_hdl), 1 * 1000)) {
            printf(" KCP_SEND RATE = %d KB/S \r\n", total_send / 1024 / 1);
            total_send = 0;
        }
        send_len = 48;
        ret = ikcp_send(kcp_hdl, (const char *)send_buf, send_len);
        if (!ret) {
            /*puts("SEND_OK ");*/
            total_send += send_len;
//            ikcp_flush(kcp_hdl,iclock);
            ikcp_update(kcp_hdl, iclock);
        } else {
            puts("*");
        }
    }
}

static void kcp_recv_test(void *p)
{
    puts("kcp_recv test \r\n");

#define KCP_TEST_RECV_BUF_SIZE 100*1024
    int time_hdl = 0;
    unsigned int total_recv = 0;
    char *recv_buf;
    int nread;

    kcp_udp_create(0x56444c4a);

    recv_buf = malloc(KCP_TEST_RECV_BUF_SIZE);
    while (1) {
        if (time_lapse((u32 *)(&time_hdl), 1 * 1000)) {
            printf(" KCP_RECV RATE = %d KB/S \r\n", total_recv / 1024 / 1);
            total_recv = 0;
        }

        nread = ikcp_recv(kcp_hdl, recv_buf, KCP_TEST_RECV_BUF_SIZE);
        if (nread > 0) {
            total_recv += nread;
            printf("^^ ikcp_recv size = %d\r\n", nread);
//                hexdump(recv_buf, nread);
        } else {
            puts("*");
        }
    }
    free(recv_buf);
}




static void wifi_rx_cb(void *rxwi, struct ieee80211_frame *wh, void *data, u32 len, void *priv)
{
    static const u8 pkg_head_fill_magic[] = {
        /*dst*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,/*src*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,/*BSSID*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, /*Seq,Frag num*/0x88, 0x88,
    };

    if (len < 25 || memcmp(&((u8 *)data)[28], pkg_head_fill_magic, sizeof(pkg_head_fill_magic))) {
        return;
    }

    u8 *payload = &((u8 *)data)[48];
    u32 payload_len = len - 24;

    /*printf("WIFI_RX LEN = %d\r\n", payload_len);*/
    /*put_buf(payload, payload_len);*/

    ikcp_input(kcp_hdl, payload, payload_len);
    ikcp_update(kcp_hdl, iclock);
}


static void c_main(void *priv)
{
    while (!wifi_is_on()) {
        os_time_dly(10);
    }
    os_time_dly(10);

    wifi_set_smp_cfg_just_monitor_mode(1);
    wifi_enter_smp_cfg_mode();      //进入混杂模式
    wifi_set_frame_cb(wifi_rx_cb, NULL); //注册接收802.11数据帧回调

    //配置WIFI RF 通信信道
    wifi_set_channel(1);

    //配置底层重传次数
    wifi_set_long_retry(0);
    wifi_set_short_retry(0);


    thread_fork("kcp_send_tst", 15, 0x2000, 0, NULL, kcp_send_test, (void *)NULL);
    /*thread_fork("kcp_recv_tst", 15, 0x2000, 0, NULL, kcp_recv_test, (void *)NULL);*/
}
late_initcall(c_main);


#endif //USE_WIFI_RAW_KCP_TEST
