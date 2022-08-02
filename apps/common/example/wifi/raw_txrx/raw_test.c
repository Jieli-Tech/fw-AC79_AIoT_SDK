#include "app_config.h"
#include "wifi/wifi_connect.h"
#include "system/includes.h"

#ifdef USE_WIFI_RAW_TXRX_TEST

static void wifi_rx_cb(void *rxwi, struct ieee80211_frame *wh, void *data, u32 len, void *reserve)
{
    static const u8 pkg_head_fill_magic[] = {
        /*dst*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,/*src*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,/*BSSID*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, /*Seq,Frag num*/0x88, 0x88,
    };

    if (len < 25 || memcmp(&((u8 *)data)[28], pkg_head_fill_magic, sizeof(pkg_head_fill_magic))) {
        return;
    }

    u8 *payload = &((u8 *)data)[48];
    u32 payload_len = len - 24;

    printf("len = %d,%d\r\n", payload_len, payload[payload_len - 1]);
    /*put_buf(payload, payload_len);*/
}

static void wifi_raw_tx_rx_test_task(void *p)
{
    while (!wifi_is_on()) {
        os_time_dly(10);
    }

    wifi_set_smp_cfg_just_monitor_mode(1);
    wifi_enter_smp_cfg_mode();      //进入混杂模式
    wifi_set_frame_cb(wifi_rx_cb, NULL); //注册接收802.11数据帧回调

    //配置WIFI RF 通信信道
    wifi_set_channel(1);

    //配置底层重传次数
    wifi_set_long_retry(0);
    wifi_set_short_retry(0);

    enum wifi_tx_rate rate = WIFI_TXRATE_1M - 1;
    while (1) {
        os_time_dly(100);

        if (++rate > WIFI_TXRATE_72M) {
            rate = WIFI_TXRATE_1M;
        }

        u8 *wifi_test_pkg = wifi_get_payload_ptr();
        ++wifi_test_pkg[2509];
        wifi_send_data(2510, rate);
    }
}

static void c_main(void *priv)
{
    thread_fork("wifi_raw_tx_rx_test_task", 10, 1024, 0, NULL, wifi_raw_tx_rx_test_task, NULL);
}

late_initcall(c_main);

#endif //USE_WIFI_RAW_TXRX_TEST
