#include "wifi/wifi_connect.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "os/os_api.h"
#include "system/init.h"
#include "lwip.h"
#include "device/device.h"
#include "system/app_core.h"
#include "server/server_core.h"
#include "system/timer.h"
#include "asm/debug.h"
#include "asm/power_interface.h"
#include "app_config.h"
#include "http/http_cli.h"
#include "system/timer.h"
#include "database.h"
#include "json_c/json_tokener.h"
#include "voiceprint_cfg.h"
#include "server/net_server.h"
#include <time.h>
#include "event/net_event.h"

#if BT_NET_CFG_DUI_EN || BT_NET_CFG_TURING_EN || BT_NET_CFG_TENCENT_EN
#undef  BT_NET_CFG_EN
#define BT_NET_CFG_EN	1
#endif

static u8 config_network_flag;

#ifdef CONFIG_AIRKISS_NET_CFG
static struct airkiss_result {
    struct smp_cfg_result result;
    char scan_ssid_found;
} airkiss_result;
#endif

#if TCFG_USER_BLE_ENABLE && BT_NET_CFG_EN
static struct bt_net_result {
    char ssid[33];
    char pwd[65];
} bt_net_result;
#endif

#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
static struct voiceprint_result {
    char ssid[33];
    char pwd[65];
    char rand_str[8];
} voiceprint_result;
#endif

#ifdef CONFIG_QR_CODE_NET_CFG
static struct qr_code_net_result {
    char ssid[33];
    char pwd[65];
} qr_code_net_result;
#endif

extern void config_network_get_yuv_init(void);
extern void config_network_get_yuv_uninit(void);
extern void qr_code_net_cfg_stop(void);
extern void wifi_sta_connect(char *ssid, char *pwd, char save);
extern int url_decode(const char *input, const u32 inlen, char *output, const u32 outbuf_len);

u8 is_in_config_network_state(void)
{
    return config_network_flag;
}

void config_network_start(void)
{
    config_network_flag = 1;

#ifdef CONFIG_LOW_POWER_ENABLE
    low_power_hw_unsleep_lock();
#endif

#ifdef CONFIG_AIRKISS_NET_CFG
    memset(&airkiss_result, 0, sizeof(airkiss_result));
    wifi_set_smp_cfg_timeout(100);
    wifi_enter_smp_cfg_mode();
#endif

#ifdef CONFIG_QR_CODE_NET_CFG
    // 为了节省内存,关闭WIFI
    /* void wifi_off(void); */
    /* wifi_off(); */
    memset(&qr_code_net_result, 0, sizeof(qr_code_net_result));
    extern void qr_code_net_cfg_start(void);
    qr_code_net_cfg_start();
    config_network_get_yuv_init();
#endif

#if TCFG_USER_BLE_ENABLE && BT_NET_CFG_EN
    memset(&bt_net_result, 0, sizeof(bt_net_result));
#if !CONFIG_POWER_ON_ENABLE_BLE
#if BT_NET_CENTRAL_EN
    void bt_master_ble_exit(void);
    bt_master_ble_exit();
    os_time_dly(200);
#endif
    extern void bt_ble_init(void);
    bt_ble_init();
#endif
#endif

#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
    memset(&voiceprint_result, 0, sizeof(voiceprint_result));
    voiceprint_cfg_start();
#endif

    /* config_network_flag = 1; */
}

#if BT_NET_CFG_TENCENT_EN
static u8 enter_ble_config_flag;
#endif

void config_network_stop(void)
{
    config_network_flag = 0;
#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
    voiceprint_cfg_stop();
#endif
#ifdef CONFIG_QR_CODE_NET_CFG
    config_network_get_yuv_uninit();
    qr_code_net_cfg_stop();
    // 为了节省内存,重新打开WIFI
    /* void wifi_on(void); */
    /* wifi_on(); */
#endif
#if TCFG_USER_BLE_ENABLE && BT_NET_CFG_EN && !CONFIG_POWER_ON_ENABLE_BLE
    void bt_ble_exit(void);
#if BT_NET_CFG_TENCENT_EN
    if (!enter_ble_config_flag)
#endif
        bt_ble_exit();
#endif

#ifdef CONFIG_LOW_POWER_ENABLE
    low_power_hw_unsleep_unlock();
#endif
}

#if TCFG_USER_BLE_ENABLE && BT_NET_CFG_EN


#if BT_NET_CFG_TENCENT_EN
//腾讯连连
void tencent_net_config(const char *recv_buf, int recv_len, u8 mode)
{
    struct net_event net;
    switch (mode) {
    case 1:
        strncpy(bt_net_result.ssid, recv_buf, recv_len);
        printf("bt net config ssid : %s\n", bt_net_result.ssid);
        break;
    case 2:
        strncpy(bt_net_result.pwd, recv_buf, recv_len);
        printf("bt net config pwd : %s\n", bt_net_result.pwd);
        break;
    case 3:
        net.arg = "net";
        net.event = NET_EVENT_SMP_CFG_FINISH;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        enter_ble_config_flag = 1;
        break;
    default:
        break;
    }
}
#endif


int bt_net_config_set_ssid_pwd(const char *data)
{
    json_object *json = json_tokener_parse(data);
    const char *str_ssid, *str_pwd;

    if (json) {
#if BT_NET_CFG_DUI_EN
        str_ssid = json_object_get_string(json_object_object_get(json, "SSID"));
#else
        str_ssid = json_object_get_string(json_object_object_get(json, "ssid"));
#endif

        if (str_ssid) {
#if BT_NET_CFG_TURING_EN
            url_decode(str_ssid, strlen(str_ssid), bt_net_result.ssid, sizeof(bt_net_result.ssid));
#else
            strcpy(bt_net_result.ssid, str_ssid);
#endif
            printf("bt net config ssid : %s\n", str_ssid);
        } else {
            json_object_put(json);
            return -1;
        }

#if BT_NET_CFG_DUI_EN
        str_pwd = json_object_get_string(json_object_object_get(json, "PSK"));
#else
        str_pwd = json_object_get_string(json_object_object_get(json, "pass"));
#endif

        if (str_pwd) {
#if BT_NET_CFG_TURING_EN
            url_decode(str_pwd, strlen(str_pwd), bt_net_result.pwd, sizeof(bt_net_result.pwd));
#else
            strcpy(bt_net_result.pwd, str_pwd);
#endif
            printf("bt net config pwd : %s\n", str_pwd);
        } else {
            memset(bt_net_result.pwd, 0, sizeof(bt_net_result.pwd));
        }

        json_object_put(json);
        struct net_event net;
        net.arg = "net";
        net.event = NET_EVENT_SMP_CFG_FINISH;
        net_event_notify(NET_EVENT_FROM_USER, &net);
        return 0;
    }

    return -1;
}
#endif

#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
int voiceprint_cfg_net_set_ssid_pwd(const char *ssid, const char *pwd, const char *rand_str)
{
    strcpy(voiceprint_result.ssid, ssid);
    strcpy(voiceprint_result.pwd, pwd);
    strcpy(voiceprint_result.rand_str, rand_str);

    struct net_event net;
    net.arg = "net";
    net.event = NET_EVENT_SMP_CFG_FINISH;
    net_event_notify(NET_EVENT_FROM_USER, &net);

    return 0;
}
#endif

#ifdef CONFIG_QR_CODE_NET_CFG
int qr_code_net_set_ssid_pwd(const char *ssid, const char *pwd)
{
    strcpy(qr_code_net_result.ssid, ssid);
    strcpy(qr_code_net_result.pwd, pwd);

    struct net_event net;
    net.arg = "net";
    net.event = NET_EVENT_SMP_CFG_FINISH;
    net_event_notify(NET_EVENT_FROM_USER, &net);

    return 0;
}
#endif

void config_network_connect(void)
{
#if TCFG_USER_BLE_ENABLE && BT_NET_CFG_EN

    if (bt_net_result.ssid[0]) {
        wifi_sta_connect(bt_net_result.ssid, bt_net_result.pwd, 1);
        return;
    }

#endif
#ifdef CONFIG_QR_CODE_NET_CFG

    if (qr_code_net_result.ssid[0]) {
        wifi_sta_connect(qr_code_net_result.ssid, qr_code_net_result.pwd, 1);
        return;
    }

#endif
#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE

    if (voiceprint_result.ssid[0]) {
        wifi_sta_connect(voiceprint_result.ssid, voiceprint_result.pwd, 1);
        return;
    }

#endif

#ifdef CONFIG_AIRKISS_NET_CFG

    if (airkiss_result.result.ssid[0]) {
        wifi_sta_connect(airkiss_result.result.ssid, airkiss_result.result.passphrase, 1);
        return;
    }

#endif
}

#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
const char *get_voiceprint_result_random(void)
{
    return voiceprint_result.rand_str[0] ? voiceprint_result.rand_str : NULL;
}

static void voiceprint_broadcast(void)
{
    if (voiceprint_result.rand_str[0]) {

#define SEND_VOICE_PRINT_MSG		"https://robot.jieliapp.com/wx/wechat/network/config/voice/success?wifirand=%s"

        int ret = 0;
        char url[128];

        http_body_obj http_body_buf;
        httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));

        if (!ctx) {
            return;
        }

        sprintf(url, SEND_VOICE_PRINT_MSG, voiceprint_result.rand_str);

        memset(&http_body_buf, 0x0, sizeof(http_body_obj));

        http_body_buf.recv_len = 0;
        http_body_buf.buf_len = 4 * 1024;
        http_body_buf.buf_count = 1;
        http_body_buf.p = (char *) malloc(http_body_buf.buf_len * sizeof(char));

        if (!http_body_buf.p) {
            free(ctx);
            return;
        }

        ctx->url = url;
        ctx->priv = &http_body_buf;
        ctx->connection = "close";

        ret = httpcli_get(ctx);

        if (ret != HERROR_OK) {
            puts("voiceprint_broadcast fail!\n");
        }

        if (http_body_buf.p) {
            free(http_body_buf.p);
        }

        free(ctx);

        voiceprint_result.rand_str[0] = 0;
    }
}
#endif

#ifdef CONFIG_AIRKISS_NET_CFG

void wifi_smp_set_ssid_pwd(void)
{
    struct smp_cfg_result smp_cfg;

    wifi_get_cfg_net_result(&smp_cfg);

    if (smp_cfg.type == AIRKISS_SMP_CFG) {
        printf("\r\n AIRKISS INFO, SSID = %s, PWD = %s, ssid_crc = 0x%x, ran_val = 0x%x \r\n", smp_cfg.ssid, smp_cfg.passphrase, smp_cfg.ssid_crc, smp_cfg.random_val);
        airkiss_result.result.type = AIRKISS_SMP_CFG ;
        airkiss_result.result.ssid_crc = smp_cfg.ssid_crc;
        airkiss_result.result.random_val = smp_cfg.random_val;
        strcpy(airkiss_result.result.ssid, smp_cfg.ssid);
        strcpy(airkiss_result.result.passphrase, smp_cfg.passphrase);

        if (airkiss_result.result.ssid_crc == wifi_airkiss_calcrc_bytes((u8 *)airkiss_result.result.ssid, strlen(airkiss_result.result.ssid))) {
            airkiss_result.scan_ssid_found = 1;

            struct net_event net;
            net.arg = "net";
            net.event = NET_EVENT_SMP_CFG_FINISH;
            net_event_notify(NET_EVENT_FROM_USER, &net);


        } else {
            wifi_scan_req();
        }

        return ;
    }

    strcpy(airkiss_result.result.ssid, smp_cfg.ssid);
    strcpy(airkiss_result.result.passphrase, smp_cfg.passphrase);
    struct net_event net;
    net.arg = "net";
    net.event = NET_EVENT_SMP_CFG_FINISH;
    net_event_notify(NET_EVENT_FROM_USER, &net);

}

void airkiss_ssid_check(void)
{
    u32 i;

    if (airkiss_result.result.type != AIRKISS_SMP_CFG ||  airkiss_result.scan_ssid_found) {
        return;
    }
    struct wifi_scan_ssid_info *sta_ssid_info;
    u32 sta_ssid_num = 0;
    sta_ssid_info = wifi_get_scan_result(&sta_ssid_num);
    wifi_clear_scan_result();

    for (i = 0; i < sta_ssid_num; i++) {
        if (!strncmp(airkiss_result.result.ssid, sta_ssid_info[i].ssid, strlen(airkiss_result.result.ssid))) {
CHECK_AIRKISS_SSID_CRC:

            if (airkiss_result.result.ssid_crc == wifi_airkiss_calcrc_bytes((u8 *)sta_ssid_info[i].ssid, strlen(sta_ssid_info[i].ssid))) {
                printf("find airkiss ssid = [%s]\r\n", sta_ssid_info[i].ssid);
                strcpy(airkiss_result.result.ssid, sta_ssid_info[i].ssid);
                airkiss_result.scan_ssid_found = 1;

                struct net_event net;
                net.arg = "net";
                net.event = NET_EVENT_SMP_CFG_FINISH;
                net_event_notify(NET_EVENT_FROM_USER, &net);

                free(sta_ssid_info);
                return;
            }
        } else {
            /*goto CHECK_AIRKISS_SSID_CRC;*/
        }
    }

    free(sta_ssid_info);

    printf("cannot found airkiss ssid[%s] !!! \n\n", airkiss_result.result.ssid);
}

static void airkiss_broadcast(void)
{
    int i, ret;
    int onOff = 1;
    int sock;
    struct sockaddr_in dest_addr;

    if (airkiss_result.result.type != AIRKISS_SMP_CFG) {
        return;
    }

    puts("airkiss_broadcast random_val \n");

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock == -1) {
        printf("%s %d->Error in socket()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = 0;
    ret = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    if (ret == -1) {
        printf("%s %d->Error in bind()\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
                     (char *)&onOff, sizeof(onOff));

    if (ret == -1) {
        printf("%s %d->Error in setsockopt() SO_BROADCAST\n", __FUNCTION__, __LINE__);
        goto EXIT;
    }

    inet_pton(AF_INET, "255.255.255.255", &dest_addr.sin_addr.s_addr);
    dest_addr.sin_port = htons(10000);

    for (i = 0; i < 8; i++) {
        ret = sendto(sock, (unsigned char *)&airkiss_result.result.random_val, 1, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

        if (ret == -1) {
            printf("%s %d->Error in sendto\n", __FUNCTION__, __LINE__);
        }

        msleep(20);
    }

    memset(&airkiss_result, 0, sizeof(airkiss_result));

EXIT:

    if (sock != -1) {
        closesocket(sock);
    }
}
#endif

void connect_broadcast(void)
{
    int i, ret;
    int onOff = 1;
    int sock;
    struct sockaddr_in dest_addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        goto EXIT;
    }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = 0;
    ret = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (ret == -1) {
        goto EXIT;
    }

    ret = setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
                     (char *)&onOff, sizeof(onOff));
    if (ret == -1) {
        goto EXIT;
    }

    inet_pton(AF_INET, "255.255.255.255", &dest_addr.sin_addr.s_addr);
    dest_addr.sin_port = htons(10001);

    for (i = 0; i < 10; i++) {
        ret = sendto(sock, "CONNECT OK", 10, 0, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
        if (ret == -1) {
            goto EXIT;
        }
        msleep(10);
    }

EXIT:
    if (sock != -1) {
        closesocket(sock);
    }
}

void config_network_broadcast(void)
{
#ifdef CONFIG_AIRKISS_NET_CFG
    airkiss_broadcast();
#endif
#ifdef CONFIG_ACOUSTIC_COMMUNICATION_ENABLE
    /* voiceprint_broadcast(); */
#endif
    //有些使用了加密的路由器刚获取IP地址后前几个包都会没响应，怀疑路由器没转发成功
    connect_broadcast();
}
