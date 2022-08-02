#ifndef _WIFIBOX_CLIENT_B_H_
#define _WIFIBOX_CLIENT_B_H_

#include "app_config.h"
#include "wbcp.h"
#include "fsm.h"
#include "versions.h"

#define     WB_CLIENT_DEBUG_ENABLE
#define     FSM_DEBUG_ENABLE
#define     WBCP_DEBUG_ENABLE

#define     WB_ASSIST_UART                   ("uart1")
// #define     WB_ASSIST_ENABLE                        //是否开启辅助通信模式
#define     WB_DUT_RESET_ENABLE

#define     RECV_EVENT_TIMEOUT               (2000) //连接成功后，接收命令的超时时间(ms)

#define     PWRTHR_SEND_TEST_DATA_PERIOD     (2)    //功率阈值校准，测试数据的发送周期(tics)

#define     PWRTEST_SEND_TEST_DATA_PERIOD    (2)    //功率测试，测试数据的发送周期(tics)
#define     BT_FREQ_CONN_TIMEOUT             (1500) //(tick)
#define     BT_FREQ_STAT_TIMEOUT             (1800/* 350 */)  //蓝牙频偏统计超时时间(tick)

#define     FREQ_ADJUST_LIMIT                (30)   //kHz

// #define    AUDIO_TRANSFER_ENABLE
#define    AUDIO_BUFFER_SIZE                 (10 * 1024) //音频缓存大小
#define    AUDIO_RECV_TIMEOUT                (200)       //音频接收超时时间(ticks)

enum {
    WB_CLIENT_MODE = 0,
    WB_SERVER_MODE,
};


typedef struct {
    u16 crc;
    s16 offset;
    u8 xosc[2];
} wifi_freq_offset_type;

void wifi_get_xosc(u8 *xosc);
u8 wifi_freq_adjust(void);

#endif


