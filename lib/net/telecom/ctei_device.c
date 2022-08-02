#include "ctei_aes.h"
#include "common/base64.h"
#include "system/includes.h"
#include "sock_api/sock_api.h"
#include "http/http_cli.h"
#include "server/ai_server.h"
#include "json_c/json_object.h"
#include "wifi/wifi_connect.h"
#include <stdlib.h>
#include "ntp/ntp.h"
#include "time.h"
#include "mbedtls/aes.h"
#include "lwip.h"
#include <stdlib.h>
#include "cJSON_common/cJSON.h"

#ifdef TELECOM_CTEI_SDK_VERSION
MODULE_VERSION_EXPORT(telecom_ctei_sdk, TELECOM_CTEI_SDK_VERSION);
#endif

#if 1  //1:使能，0：禁止
#define log_info(x, ...)    printf("\n\n>>>>>>[CTEI_test]" x " ", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

const struct ai_sdk_api psmarthome_ctei_device_api;
extern struct json_object *json_tokener_parse(const char *str);

#define BUFFER_SIZE 2096
static char base64[BUFFER_SIZE];

typedef enum {
    REPORT_NULL = 1,
    REPORT_SUCCESS,
    REPORT_FAIL,
} CTEI_STATE;

//设备、厂商等信息，根据实际情况进行修改
const char *address = "http://smarthome.ctdevice.ott4china.com:9010/boot";    //接入地址
#define MANUFACTURER_ID "A000"              								  //厂商标识ID，必须要与电信门户信息对应，否则无法上报
#define KEY_VERSION "1.0"												      //密钥版本号
#define DEVICE_SN "000000000000001"		                                      //设备标识码
#define FMW_VERSION "11.2.5.21"				                                  //终端固件版本号
#define LINK_TYPE "1"						                                  //设备连接类型: 1.elink方式接入网络。2.非elink方式接入网络
#define PARENT_DEV_MAC ""                                                     //上级设备的MAC地址
#define POWER_ON "1"
#define TIME_UP  "2"

typedef struct SMARTHOME_CTEI_DEVICE_INFOMATION {
    char device_mac[16];
    char device_ip[16];
    CTEI_STATE report_state;
    int task_pid;
} ctei_dev_info;

static u32 ctei_timer_handle = 0;
static ctei_dev_info dev_info;
static const char SMARTHOME_CTEI_VER[] = "V20200320";                          //版本号
#define TIME_INTERVAL 30000                                                    //上报ctei时间间隔，请根据需要修改
#define __this (&dev_info)

extern char *get_device_ctei(void);
extern char *get_device_key(void);

//请自行保证 keys/values，元素数目相同
char *genJson(char **keys, char **values, int nr)
{
    cJSON *root;
    char *out;

    root = cJSON_CreateObject();
    for (int index = 0; index < nr; index++) {
        cJSON_AddItemToObject(root, keys[index], cJSON_CreateString(values[index]));
    }

    out = cJSON_PrintUnformatted(root);

    cJSON_Delete(root);
    return out;
}

void RemoveChars(char *s, char c)
{
    int writer = 0, reader = 0;

    while (s[reader]) {
        if (s[reader] != c) {
            s[writer++] = s[reader];
        }

        reader++;
    }

    s[writer] = 0;
}

const char *smarthome_ctei_version(void)
{
    return SMARTHOME_CTEI_VER;
}

static void ctei_dev_info_init(ctei_dev_info *info)
{
    memset(info, 0, sizeof(ctei_dev_info));
}

//获取时间戳，单位:秒
static const unsigned long getCurrentTimeMillis(char *time_t)
{
    char time_str[32] = {0};
    struct tm timeinfo = {0};
    long timestamp = time(NULL) + 28800;
    localtime_r(&timestamp, &timeinfo);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &timeinfo);

    strcpy(time_t, time_str);

    return (timestamp - 28800);
}

static int ctei_state_check(const char *rec)
{
    json_object *state_rec = NULL;
    unsigned int code;

    if (NULL == rec) {
        return -1;
    }

    state_rec = json_tokener_parse(rec);
    if (NULL == state_rec) {
        return -1;
    }

    __this->report_state = REPORT_NULL;

    code = atoi(json_object_get_string(json_object_object_get(state_rec, "code")));

    if (code == 0) {
        __this->report_state = REPORT_SUCCESS;
    } else {
        __this->report_state = REPORT_FAIL;
    }

    json_object_put(state_rec);

    return 0;
}

static int ctei_device_report(const char *address, const char *json)
{
    char *url = NULL;
    http_body_obj http_recv = {0};
    int ret = 0;
    httpcli_ctx ctx = {0};

    //init
    http_recv.recv_len = 0;
    http_recv.buf_len = 256;
    http_recv.buf_count = 1;
    http_recv.p = (char *)malloc(http_recv.buf_len * http_recv.buf_count);
    if (http_recv.p == NULL) {
        log_info("malloc err!!!");
        ret = -1;
        goto exit;
    }

    ctx.data_format = "application/json";
    ctx.data_len = strlen(json);
    ctx.post_data = json;

    ctx.timeout_millsec = 5000;
    ctx.priv = &http_recv;
    ctx.url = address;
    ctx.connection = "close";
    ret = httpcli_post(&ctx);
    if (ret) {
        ret = -1;
        goto exit;
    }

exit:
    if (http_recv.p) {

        log_info("Received data : %s\n\r", http_recv.p);

        if (ret == 0) {
            ctei_state_check(http_recv.p); //上报状态监测
        } else {
            log_info("Check whether the network is connected ");
            __this->report_state == REPORT_FAIL;
        }

        free(http_recv.p);
        http_recv.p = NULL;
    }

    return ret;
}

static unsigned int get_device_mac_ip(char *device_mac, char *device_ip)
{
    char ipaddr[32];
    u8 mac[6];
    char mac_tmp[32];

    wifi_get_mac(mac);

    snprintf(mac_tmp, sizeof(mac_tmp), "%02X%02X%02X%02X%02X%02X",
             mac[0],
             mac[1],
             mac[2],
             mac[3],
             mac[4],
             mac[5]);

    Get_IPAddress(1, ipaddr);

    strcpy(device_mac, mac_tmp);
    strcpy(device_ip, ipaddr);

    return 0;
}

static int ctei_report_start(void *type)
{
    aes_context ctx;

    char time_tmp[32];
    unsigned long sec;
    char ticks[32];
    int len;

    int out_len;
    unsigned char *bindata = NULL;
    char *encrypt = NULL;
    char *input = NULL;

    int err = 0;
    char *key;
    char *ctei_code;

    key = get_device_key();
    if (!key) {
        log_info("get_device_key err!!!");
        goto exit;
    }

    ctei_code = get_device_ctei();
    if (!ctei_code) {
        log_info("get_device_ctei err!!!");
        goto exit;
    }

    sec = getCurrentTimeMillis(time_tmp); //毫秒
    /* log_info("date : %s, timestamp : %lu\n\r", time_tmp, sec); */
    sprintf(ticks, "%lu000", sec);

    char *k_encrypt[] = { "date", "timeStamp", "ip", "fmwVersion",
                          "linkType", "ctei", "sn", "mac", "rptType"
                        };

    char *v_encrypt[] = {time_tmp, ticks, __this->device_ip, FMW_VERSION,
                         LINK_TYPE, ctei_code, DEVICE_SN, __this->device_mac, (char *)type
                        };

    encrypt = genJson(k_encrypt, v_encrypt, sizeof(k_encrypt) / sizeof(char *));
    if (encrypt == NULL) {
        goto exit;
    }

    len = (int)strlen(encrypt);
    /* log_info("Encoded: %s\n", encrypt); */

    bindata = encode(&ctx, AES_ENCRYPT, (unsigned char *)key, (unsigned char *)encrypt, len);
    if (bindata == NULL) {
        goto exit;
    }
    os_time_dly(30);

    memset(base64, 0, BUFFER_SIZE);
    memcpy(base64, base64_encode(bindata, encode_length(len), &out_len), out_len);
    os_time_dly(20);

    char *keys[] = {"manufacturerId", "version", "decryptData", "timeStamp"};
    char *values[] = {MANUFACTURER_ID, KEY_VERSION, base64, ticks};

    input = genJson(keys, values, sizeof(keys) / sizeof(char *));
    if (input == NULL) {
        goto exit;
    }

    /* log_info("json:%s, length:%d\n", input, len); */

    err = ctei_device_report(address, input);

exit:
    if (encrypt) {
        free(encrypt);
    }

    if (input) {
        free(input);
    }

    if (bindata) {
        free(bindata);
    }

    return err;
}

static void ctei_report(void *type)
{
    ctei_report_start(type);
}

static void smarthome_ctei_device_task(void *priv)
{
    unsigned long sec;
    char device_mac[32];
    char device_ip[20];

    int err;

    os_time_dly(400); //适当延时，等待成功获取网络时间，可根据调试调整，避免时间获取错误时无法上报

    ctei_dev_info_init(__this);

    get_device_mac_ip(device_mac, device_ip);

    strcpy(__this->device_mac, device_mac);
    strcpy(__this->device_ip, device_ip);

    err = ctei_report_start(POWER_ON); //1:开机上报
    if (err == 0) {
        ctei_timer_handle = sys_timer_add(TIME_UP, ctei_report, TIME_INTERVAL); //2：隔TIME_INTERVAL上报，根据需要修改上报时间
    } else {
        __this->report_state == REPORT_FAIL;
    }
}

static int ctei_device_connect()
{
    if (thread_fork("smarthome_ctei_device_task", 5, 2 * 1024, 0, NULL, smarthome_ctei_device_task, NULL) != OS_NO_ERR) {
        printf("%s thread fork fail\n", __FILE__);
        return -1;
    }

    return 0;
}

static int ctei_device_state()
{
    if (__this->report_state == REPORT_SUCCESS) {
        log_info("State: ctei report success!\n");
        return AI_STAT_CONNECTED;
    } else if (__this->report_state == REPORT_FAIL) {
        log_info("State: ctei report error!\n");
        return AI_STAT_DISCONNECTED;
    }

    return AI_STAT_DISCONNECTED;
}

static int ctei_device_event(int event, int arg)
{
    return 0;
}

static int ctei_device_disconnect()
{
    if (ctei_timer_handle) {
        sys_timer_del(ctei_timer_handle);
        __this->report_state = REPORT_NULL;
    }

    return 0;
}

REGISTER_AI_SDK(psmarthome_ctei_device_api) = {
    .name           = "ceti_device",
    .connect        = ctei_device_connect,
    .state_check    = ctei_device_state,
    .do_event       = ctei_device_event,
    .disconnect     = ctei_device_disconnect,
};
