#include "system/includes.h"
#include "json_c/json_object.h"
#include "common/base64.h"
#include "common/aes.h"
#include "sock_api/sock_api.h"
#include "lwip.h"
#include "wifi/wifi_connect.h"
#include "mbedtls/aes.h"
#include "mbedtls/dhm.h"
#include "mbedtls/rsa.h"
#include "mbedtls/sha1.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "time.h"

//厂商及设备信息，根据实际情况进行修改
#define VENDOR "A000" 								//设备厂商
#define MODEL "model"								//设备型号
#define SW_VERSION "1.0"							//软件版本
#define HD_VERSION "1.0"							//硬件版本
#define SN "00A1140101080210037734215333129368"     //设备序列号
#define IPADDR "192.168.1.2"					    //设备出口IP
#define URL "" 										//厂商链接地址
#define DEFAULT_DEVICE_CTEI "000000000000001"		//CTEI码
#define DEV_TYPE "IOT"								//设备类型，IOT:表示智能终端设备。AP：表示组网设备
#define DESCRIPTION "智能机器人"			        //设备描述
#define WIRELESS "yes"								//设备上行链路接入方式，yes：表示无线，no：表示有线
#define PROTOCOL_VERSION "V2019.1.0"				//协议版本号

#if 1   //1:使能，0：禁止
#define log_info(x, ...)    printf("\n\n>>>>>>[elink_test]" x " ", ## __VA_ARGS__)
#define ELINK_DEBUG 1
#else
#define log_info(...)
#endif

extern struct json_object *json_tokener_parse(const char *str);

//tcp connect
#define ELINK_SERVER_IP "192.168.1.1"
#define ELINK_SERVER_TCP_PORT 32769     //隐藏SSID交互端口
#define ELINK_LOCAL_PORT 0
static struct sockaddr_in local;
static struct sockaddr_in dest;
static void *sock;

//udp connect
#define ELINK_SERVER_UDP_PORT 32767
static struct sockaddr_in udp_dest;
static void *udp_sock;

static int elink_keyng_req(char *buf, int buf_len);
static int elink_dh_mode(char *buf, int buf_len);
static int elink_keepalive_req(char *buf, int buf_len);
static int elink_dev_reg_req(char *buf, int buf_len);
static int elink_keepalive_send_ack(char *buf, int buf_len);
static int elink_qlink_success(char *buf, int buf_len);
static int elink_dh_key_base64_encode(char *output);
static int elink_dh_p_base64_encode(unsigned char *output);
static int elink_dh_g_base64_encode(char *output);
static int elink_aes_cbc_encode(const unsigned char *input_buf, const unsigned int input_len, unsigned char *encode_buf);
static int elink_aes_cbc_decode(const unsigned char *input_buf, const unsigned int input_len, unsigned char *decode_buf);
extern void wifi_sta_connect(char *ssid, char *pwd, char save, void *priv);
extern int elink_qlink_set_ssid_pwd(const char *ssid, const char *pwd);
static int elink_get_key(const unsigned char *encode_str, unsigned char *output);
extern char *get_device_ctei(void);

typedef enum {
    elink_idel_state = 1,
    elink_wait_key_ack,
    elink_wait_keymode_ack,
    elink_wait_dev_reg_ack,
    elink_keepAlive_req,
    elink_wait_keepAlive_ack,
    elink_wait_qlink_info,
    elink_qlink_finish,
} elink_state;

typedef enum {
    elink_start = 1,
    elink_receive_key_ack,
    elink_receive_keymode_ack,
    elink_receive_dev_reg_success,
    elink_receive_keepAlive_allow,
    elink_receive_qlink_info,
} elink_event;

typedef struct {
    elink_state curState;
    elink_event event;
    int (*elink_action)(char *, int);
    elink_state nextState;
} elink_state_change;

typedef struct {
    elink_state curState;
    int stuNum;
    elink_state_change *form;
} elink_qlink;

typedef enum {
    dh_mode = 1,
    ecdh_mode,
    nanoECC_mode,
} elink_mode;

typedef struct DEVICE_INFORMATION {
    char ipaddr[16];
    unsigned char common_key[16];
    char elink_ssid[32];
    char elink_pwd[32];
    char dev_mac[16];
    unsigned int sequence_cnt;
    mbedtls_dhm_context dhm_cli;
    elink_qlink e_qlink;
    elink_event e_event;
    elink_mode  key_mode;
} dev_info;

static dev_info elink_dev_info;
#define __this (&elink_dev_info)

static unsigned int is_get_ssid_pwd_ok = 0;

static const char elink_qlink_ver[] = "V3.0";

//密钥采用128位dh
#define DH_P "FF3163333631636339653433666238FF" //128bit
#define DH_G "02"
unsigned char key_p[16] = {0xFF, 0x31, 0x63, 0x33, 0x36, 0x31, 0x63, 0x63, 0x39, 0x65, 0x34, 0x33, 0x66, 0x62, 0x38, 0xFF};
unsigned char key_g[1]  = {0x02};

//State Table : curState   event   action  nextState
static elink_state_change elink_table[] = {
    {elink_idel_state, elink_start, elink_keyng_req, elink_wait_key_ack},
    {elink_wait_key_ack, elink_receive_key_ack, elink_dh_mode, elink_wait_keymode_ack},
    {elink_wait_keymode_ack, elink_receive_keymode_ack, elink_dev_reg_req, elink_wait_dev_reg_ack},

    {elink_wait_dev_reg_ack, elink_receive_dev_reg_success, elink_keepalive_req, elink_wait_keepAlive_ack},
    {elink_wait_keepAlive_ack, elink_receive_keepAlive_allow, NULL, elink_wait_qlink_info},
    {elink_wait_qlink_info, elink_receive_qlink_info, elink_qlink_success, elink_qlink_finish},
};

unsigned int elink_connect_ok(void)
{
    if (is_get_ssid_pwd_ok) {
        is_get_ssid_pwd_ok = 0;
        return 1;
    }

    return is_get_ssid_pwd_ok;
}

const char *elink_qlink_version(void)
{
    return elink_qlink_ver;
}

static void elink_state_transFer(elink_qlink *qlink, elink_state state)
{
    qlink->curState = state;
}

static int elink_tcp_init(const int port)
{
    char ipaddr[20];

    sock = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);
    if (sock == NULL) {
        log_info("%s tcp: sock_reg fail\n",  __FILE__);
        return -1;
    }

    Get_IPAddress(1, ipaddr);

    local.sin_addr.s_addr = inet_addr(ipaddr);
    local.sin_port = htons(port);
    local.sin_family = AF_INET;
    if (0 != sock_bind(sock, (struct sockaddr *)&local, sizeof(struct sockaddr_in))) {
        sock_unreg(sock);
        log_info("%s sock_bind fail\n",  __FILE__);
        return -1;
    }

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(ELINK_SERVER_IP);
    dest.sin_port = htons(ELINK_SERVER_TCP_PORT);
    if (0 != sock_connect(sock, (struct sockaddr *)&dest, sizeof(struct sockaddr_in))) {
        log_info("sock_connect fail.\n");
        sock_unreg(sock);
        return -1;
    }

    return 0;
}

static int elink_eventHandle(elink_qlink *qlink, const elink_event event, void *parm)
{
    elink_state_change *changeTable = qlink->form;
    int (*elink_action)(char *, int) = NULL;
    elink_state nextState;
    elink_state curState = qlink->curState;
    unsigned something_happen = 0;
    unsigned int i;
    unsigned int ret;
    char send_buf[512];
    unsigned int len;
    int err;

    for (i = 0; i < qlink->stuNum; i++) {
        if (event == changeTable[i].event && curState == changeTable[i].curState) {
            elink_action = changeTable[i].elink_action;
            nextState = changeTable[i].nextState;
            something_happen = 1;
            break;
        }
    }

    if (something_happen) {
        if (elink_action != NULL) {
            len = elink_action(send_buf, sizeof(send_buf));
            if (len == -1) {
                return -1;
            }

            /* put_buf(send_buf, len); */

            os_time_dly(50);

            ret = sock_sendto(sock, (void *)&send_buf, len, 0, (struct sockaddr *)&dest, sizeof(struct sockaddr_in));
            if (ret == -1) {
                log_info("sock_sendto err!!!");
                return -1;
            }
        }

        elink_state_transFer(qlink, nextState);
        something_happen = 0;
    }

    return 0;
}

static void elink_init(elink_qlink *qlink, elink_state curState, const int num, const elink_state_change *form)
{
    qlink->form = (elink_state_change *)form;
    qlink->stuNum = num;
    qlink->curState = curState;
    return;
}

static int elink_dev_mac(void)
{
    u8 mac[6];
    wifi_get_mac(mac);
    sprintf(__this->dev_mac, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return 0;
}

static int elink_package_assembly(char *data, int data_len, char *package, int package_buf_len)
{
    if (data == NULL || data_len <= 0 || package == NULL || (data_len + 8) > package_buf_len) {
        return -1;
    }

    package[0] = 0x3f;
    package[1] = 0x72;
    package[2] = 0x1f;
    package[3] = 0xb5;

    package[4] = (u8)(((u32)data_len >> 24) & 0x00ff);
    package[5] = (u8)(((u32)data_len >> 16) & 0x00ff);
    package[6] = (u8)(((u32)data_len >> 8) & 0x00ff);
    package[7] = (u8)(((u32)data_len >> 0) & 0x00ff);

    memcpy(package + 8, data, data_len);

    return (data_len + 8);
}

static int elink_keyng_req(char *buf, int buf_len)
{
    char temp_buf[512];
    unsigned int len;
    char *mac;

    snprintf(temp_buf, sizeof(temp_buf), "{\"type\":\"keyngreq\",\"sequence\":1,\"mac\":\"%s\",\"version\":\"%s\", \
			\"keymodelist\":[{\"keymode\":\"dh\"}]}", \
             __this->dev_mac,
             PROTOCOL_VERSION);

    len = elink_package_assembly(temp_buf, strlen(temp_buf), buf, buf_len);

    return len;
}

static int elink_dh_mode(char *buf, int buf_len)
{
    char temp_buf[256];
    char key[64];
    unsigned char dh_p[64];
    char dh_g[64];
    unsigned int len;

    elink_dh_key_base64_encode(key);
    elink_dh_p_base64_encode(dh_p);
    elink_dh_g_base64_encode(dh_g);

    snprintf(temp_buf, sizeof(temp_buf), "{\"type\":\"dh\",\"sequence\":2,\"mac\":\"%s\",\"data\":{\"dh_key\":\"%s\",\"dh_p\":\"%s\",\"dh_g\":\"%s\"}}",
             __this->dev_mac,
             key,
             dh_p,
             dh_g);

    len = elink_package_assembly(temp_buf, strlen(temp_buf), buf, buf_len);

    return len;
}

static int elink_dev_reg(void)
{
    char ipaddr[20];

    Get_IPAddress(1, ipaddr);

    strcpy(__this->ipaddr, ipaddr);

    return 0;
}

static int elink_dev_reg_req(char *buf, int buf_len)
{
    char encode_buf[512];
    unsigned int encode_len;
    unsigned int len;
    char temp_buf[512];
    char *ctei_code;

    elink_dev_reg();

    ctei_code = get_device_ctei();
    if (!ctei_code) {
        log_info("get_device_ctei err!!!");
        ctei_code = DEFAULT_DEVICE_CTEI;
    }

    snprintf(temp_buf, sizeof(temp_buf), "{\"type\":\"dev_reg\",\"sequence\":3,\"mac\":\"%s\",\"data\":{\"vendor\":\"%s\",\"model\":\"%s\", \
			\"swversion\":\"%s\",\"hdversion\":\"%s\",\"sn\":\"%s\",\"ipaddr\":\"%s\",\"url\":\"%s\",\"wireless\":\"%s\",\"CTEI\":\"%s\",    \
			\"devtype\":\"%s\",\"description\":\"%s\"}}", \
             __this->dev_mac,  \
             VENDOR,  \
             MODEL,   \
             SW_VERSION,  \
             HD_VERSION,   \
             SN,     \
             IPADDR,   \
             URL,   \
             WIRELESS,   \
             ctei_code,   \
             DEV_TYPE,  \
             DESCRIPTION);

    len = strlen(temp_buf);

    encode_len = elink_aes_cbc_encode((unsigned char *)temp_buf, len, (unsigned char *)encode_buf);

    len = elink_package_assembly(encode_buf, encode_len, buf, buf_len);

#if ELINK_DEBUG
    unsigned int decode_len;
    char decode_buf[512];
    decode_len = elink_aes_cbc_decode((unsigned char *)encode_buf, encode_len, (unsigned char *)decode_buf);
    *(decode_buf + decode_len) = '\0';
    log_info("Decoded Data , length %d: \r\n%s\r\n", decode_len, decode_buf);
#endif

    return len;
}

static int elink_keepalive_req(char *buf, int buf_len)
{
    unsigned int len;
    char encode_buf[216];
    unsigned int encode_len;
    char temp_buf[128];

    sprintf(temp_buf, "{\"type\":\"keepalive\",\"sequence\":4,\"mac\":\"%s\"}", __this->dev_mac);

    len = strlen(temp_buf);

    encode_len = elink_aes_cbc_encode((unsigned char *)temp_buf, len, (unsigned char *)encode_buf);

    len = elink_package_assembly(encode_buf, encode_len, buf, buf_len);

#if ELINK_DEBUG
    unsigned int decode_len;
    char decode_buf[216];
    decode_len = elink_aes_cbc_decode((unsigned char *)encode_buf, encode_len, (unsigned char *)decode_buf);
    *(decode_buf + decode_len) = '\0';
    log_info("Decoded Data , length %d: \r\n%s\r\n", decode_len, decode_buf);
#endif

    return len;
}

static int elink_keepalive_send_ack(char *buf, int buf_len)
{
    unsigned int len;
    char encode_buf[216];
    unsigned int encode_len;
    char temp_buf[128];

    sprintf(temp_buf, "{\"type\":\"ack\",\"sequence\":4,\"mac\":\"%s\"}", __this->dev_mac);

    len = strlen(temp_buf);

    encode_len = elink_aes_cbc_encode((unsigned char *)temp_buf, len, (unsigned char *)encode_buf);

    len = elink_package_assembly(encode_buf, encode_len, buf, buf_len);

#if ELINK_DEBUG
    unsigned int decode_len;
    char decode_buf[216];
    decode_len = elink_aes_cbc_decode((unsigned char *)encode_buf, encode_len, (unsigned char *)decode_buf);
    *(decode_buf + decode_len) = '\0';
    log_info("Decoded Data , length %d: \r\n%s\r\n", decode_len, decode_buf);
#endif

    return len;
}

static int elink_qlink_success(char *buf, int buf_len)
{
    unsigned int len;
    char encode_buf[216];
    unsigned int encode_len;
    char temp_buf[128];

    sprintf(temp_buf, "{\"type\":\"ack\",\"sequence\":5,\"mac\":\"%s\"}", __this->dev_mac);

    len = strlen(temp_buf);

    encode_len = elink_aes_cbc_encode((unsigned char *)temp_buf, len, (unsigned char *)encode_buf);

    len = elink_package_assembly(encode_buf, encode_len, buf, buf_len);

#if ELINK_DEBUG
    unsigned int decode_len;
    char decode_buf[216];
    decode_len = elink_aes_cbc_decode((unsigned char *)encode_buf, encode_len, (unsigned char *)decode_buf);
    *(decode_buf + decode_len) = '\0';
    log_info("Decoded Data , length %d: \r\n%s\r\n", decode_len, decode_buf);
#endif

    return len;
}

static int elink_qlinkFinish_send_ack(char *buf, int buf_len)
{
    char send_buf[128];
    unsigned int len;
    char temp_buf[128];

    sprintf(temp_buf, "{\"type\":\"qlinkFinish\",\"mac\":\"%s\"}", __this->dev_mac);

    len = elink_package_assembly(temp_buf, strlen(temp_buf), buf, buf_len);

    return len;
}

static int elink_qlink_info(const char *object)
{
    json_object *json = NULL, *wifi = NULL, *mode = NULL, *ap = NULL, *ap_data = NULL;
    json_object *ssid = NULL, *key = NULL, *result = NULL;

    if (NULL == object) {
        return -1;
    }

    json = json_tokener_parse(object);
    if (NULL == json) {
        return -1;
    }

    json_object_object_get_ex(json, "result", &result);
    log_info("the keymode: %s.\n", json_object_get_string(result));

    if (0 == strcmp("allow", json_object_get_string(result))) {
        json_object_object_get_ex(json, "wifi", &wifi);
        log_info("the keymode: %s.\n", json_object_get_string(wifi));

        mode = json_object_array_get_idx(wifi, 0); //2.4G
        log_info("the keymode: %s.\n", json_object_get_string(mode));

        json_object_object_get_ex(mode, "ap", &ap);//ap

        ap_data = json_object_array_get_idx(ap, 0); //ssid key auth encrypt

        log_info("the keymode: %s.\n", json_object_get_string(ap_data));


        json_object_object_get_ex(ap_data, "ssid", &ssid); //ssid
        log_info("the ssid: %s.\n", json_object_get_string(ssid));
        strcpy(__this->elink_ssid, json_object_get_string(ssid));

        json_object_object_get_ex(ap_data, "key", &key); //key
        log_info("the ssid: %s.\n", json_object_get_string(key));
        strcpy(__this->elink_pwd, json_object_get_string(key));

    } else {
        log_info("KEY REFUSE");
        return -1;
    };

    json_object_put(json);

    return 0;
}

static int elink_rec_handle(const char *object)
{
    json_object *type = NULL, *keymode = NULL, *json = NULL, *data = NULL;
    json_object *dh_key = NULL;
    int err;

    if (NULL == object) {
        return -1;
    }

    json = json_tokener_parse(object);
    if (NULL == json) {
        return -1;
    }

    json_object_object_get_ex(json, "type", &type);

    //keymode
    if (0 == strcmp(json_object_get_string(type), "keyngack")) {
        __this->sequence_cnt++;
        __this->e_event = elink_receive_key_ack;

        json_object_object_get_ex(json, "keymode", &keymode);

        if (0 == strcmp(json_object_get_string(keymode), "dh")) {
            __this->key_mode = dh_mode;
            goto exit;
        }

        if (0 == strcmp(json_object_get_string(keymode), "ecdh")) {
            __this->key_mode = ecdh_mode;
            goto exit;
        }

        if (0 == strcmp(json_object_get_string(keymode), "nanoECC")) {
            __this->key_mode = nanoECC_mode;
            goto exit;
        }

exit:
        json_object_put(json);

        return 0;
    }

    //dh
    if (0 == strcmp(json_object_get_string(type), "dh")) {
        __this->sequence_cnt++;

        __this->e_event = elink_receive_keymode_ack;

        json_object_object_get_ex(json, "data", &data);
        json_object_object_get_ex(data, "dh_key", &dh_key);

        elink_get_key((unsigned char *)json_object_get_string(dh_key), __this->common_key);

        log_info("get dh key: %s/n/r", json_object_get_string(dh_key));
        /* put_buf(__this->common_key, 16); */

        json_object_put(json);
        return 0;
    }

    if (0 == strcmp(json_object_get_string(type), "ack")) {
        if (__this->e_qlink.curState == elink_wait_dev_reg_ack) {
            __this->e_event = elink_receive_dev_reg_success;
            json_object_put(json);
            return 0;
        } else if (__this->e_qlink.curState == elink_wait_keepAlive_ack) {
            __this->e_event = elink_receive_keepAlive_allow;
            json_object_put(json);
            os_time_dly(500);  //适当延时，避免配网超时，可根据调试调整
            return 0;
        }
    }

    if (0 == strcmp(json_object_get_string(type), "qlink")) {
        __this->sequence_cnt = 0;
        __this->e_event = elink_receive_qlink_info;
        elink_qlink_info(object);
        json_object_put(json);
        return 0;
    }

    return 0;
}

static int elink_dh_make_public(char *public)
{
    int ret = 0;
    const char *pers = "elink_dh";
    char cli_pub[16];
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;


    mbedtls_dhm_init(&(__this->dhm_cli));
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0) {

        mbedtls_printf("error!!! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto err;
    }

    mbedtls_mpi_read_string(&(__this->dhm_cli).P, 16, DH_P);
    mbedtls_mpi_read_string(&(__this->dhm_cli).G, 10, DH_G);
    __this->dhm_cli.len = mbedtls_mpi_size(&(__this->dhm_cli).P);


    ret = mbedtls_dhm_make_public(&(__this->dhm_cli), 16, (unsigned char *)cli_pub, sizeof(cli_pub),
                                  mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        mbedtls_printf(" error!!! mbedtls_dhm_make_public %d\n", ret);
        goto err;
    }

    /* put_buf(cli_pub, sizeof(cli_pub)); */

    memcpy(public, cli_pub, sizeof(cli_pub));

    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return 0;
err:
    mbedtls_dhm_free(&(__this->dhm_cli));
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return -1;
}

static int elink_dh_key_base64_encode(char *output)
{
    unsigned char key[16] = {0};
    int out_len;

    elink_dh_make_public((char *)key);

    memcpy(output, base64_encode(key, sizeof(key), &out_len), out_len);
    *(output + out_len) = '\0';

    return out_len;
}

static int elink_dh_p_base64_encode(unsigned char *output)
{
    int out_len;

    memcpy(output, base64_encode(key_p, sizeof(key_p), &out_len), out_len);
    *(output + out_len) = '\0';

    return out_len;
}

static int elink_dh_g_base64_encode(char *output)
{
    int out_len;

    memcpy(output, base64_encode(key_g, sizeof(key_g), &out_len), out_len);
    *(output + out_len) = '\0';

    return out_len;
}

static int elink_dh_key_base64_decode(unsigned char *input, unsigned char *output)
{

    int out_len;

    memcpy(output, base64_decode(input, strlen((char *)input), &out_len), out_len);
    /* put_buf(output, out_len); */

    return out_len;
}

static int elink_dh_calc_secret(unsigned char *srv_pub, char *out_put)
{
    unsigned char secret[16];
    int ret;
    size_t n = 0;

    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_ctr_drbg_init(&ctr_drbg);

    ret = mbedtls_dhm_read_public(&(__this->dhm_cli), srv_pub, 16);

    ret = mbedtls_dhm_calc_secret(&(__this->dhm_cli), secret, sizeof(secret),
                                  &n, mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        mbedtls_printf(" failed\n! mbedtls_dhm_calc_secret %d\n", ret);
        return -1;
    }

    memcpy(out_put, secret, 16);

    mbedtls_dhm_free(&(__this->dhm_cli));

    return 0;
}

static int elink_get_key(const unsigned char *encode_str, unsigned char *output)
{
    unsigned char srv_pub[16];
    char key[16];

    elink_dh_key_base64_decode((unsigned char *)encode_str, srv_pub);

    elink_dh_calc_secret(srv_pub, key);

    memcpy(output, key, 16);

    return 0;
}

static int elink_aes_cbc_encode(const unsigned char *input_buf, const unsigned int input_len, unsigned char *encode_buf)
{

    unsigned char iv[16] = {0};
    unsigned int encode_len;

    CT_AES_CBC_Encrypt((unsigned char *)input_buf, input_len, __this->common_key, 16, iv, 16, encode_buf, &encode_len);

    return encode_len;
}

static int elink_aes_cbc_decode(const unsigned char *input_buf, const unsigned int input_len, unsigned char *decode_buf)
{
    unsigned char iv[16] = {0};
    unsigned int decode_len;

    CT_AES_CBC_Decrypt((unsigned char *)input_buf, input_len, __this->common_key, 16, iv, 16, decode_buf, &decode_len);

    return decode_len;
}

static int elink_udp_init(int port)  //init udp
{
    udp_sock = sock_reg(AF_INET, SOCK_DGRAM, 0, NULL, NULL);
    if (udp_sock == NULL) {
        log_info("%s udp: sock_reg fail\r\n",  __FILE__);
        return -1;
    }

    udp_dest.sin_family = AF_INET;
    udp_dest.sin_addr.s_addr = inet_addr(ELINK_SERVER_IP);
    udp_dest.sin_port = htons(ELINK_SERVER_UDP_PORT);

    return 0;
}

int elink_udp_send(void)
{
    char send_buf[128];
    int ret;
    int i;

    elink_qlinkFinish_send_ack(send_buf, sizeof(send_buf));

    for (i = 0; i < 4; i++) {
        ret = sock_sendto(udp_sock, send_buf, sizeof(send_buf), 0, (struct sockaddr *)&udp_dest, sizeof(struct sockaddr_in));
        if (ret == -1) {
            log_info("udp sock_sendto err!");
            return -1;
        } else {
            log_info("udp sock_sendto success!");
        }

        os_time_dly(50);
    }

    sock_unreg(udp_sock);

    return 0;
}

int elink_dev_connet_wifi_success(void)
{
    int err;

    err = elink_udp_init(ELINK_LOCAL_PORT);
    if (err != 0) {
        sock_unreg(udp_sock);
        log_info("udp sock err\r\n");
        goto exit;
    }
    elink_udp_send();

exit:
    return 0;
}

static void elink_qlink_start(void *priv)
{
    char ipaddr[20];
    int ret;
    elink_event event;
    int recv_len = 0;
    char recv_buf[1024] = {0};
    char decode_buf[512];
    unsigned int decode_len;
    int error;

    elink_dev_mac();
    __this->sequence_cnt = 0;

    ret = elink_tcp_init(ELINK_LOCAL_PORT);
    if (ret == -1) {
        log_info("elink tcp init err\r\n");
        goto exit;
    }

    __this->e_event = elink_start;

    elink_init(&(__this->e_qlink), elink_idel_state, sizeof(elink_table) / sizeof(elink_state_change), elink_table);

    for (;;) {
        error = elink_eventHandle(&(__this->e_qlink), __this->e_event, NULL);
        if (error == -1) {
            log_info("send data err\r\n");
            goto exit;
        }

        if (__this->e_qlink.curState == elink_qlink_finish) {
            log_info("Successfully obtained distribution network information\r\n");
            break;
        }

        recv_len = sock_recvfrom(sock, recv_buf, sizeof(recv_buf), 0, NULL, NULL);
        if ((recv_len != -1) && (recv_len != 0)) {
            if (__this->sequence_cnt > 1) {
                /* put_buf(recv_buf, recv_len); */
                decode_len = elink_aes_cbc_decode((unsigned char *)(recv_buf + 8), recv_len - 8, (unsigned char *)decode_buf);

#if ELINK_DEBUG
                /* put_buf(decode_buf, decode_len); */
                *(decode_buf + decode_len) = '\0';
                log_info("received data, length %d : \r\n%s\r\n", recv_len, decode_buf);
#endif

                elink_rec_handle(decode_buf);

            } else {
#if ELINK_DEBUG
                /* put_buf(recv_buf, recv_len); */
                *(recv_buf + recv_len) = '\0';
                log_info("received data, length %d : \r\n%s\r\n", recv_len, recv_buf + 8);
#endif
                elink_rec_handle(recv_buf + 8);
            }

            memset(recv_buf, 0, sizeof(recv_buf));

            os_time_dly(30); //适当延时，可根据调试调整

        } else {
            log_info("sock_recvfrom error");
            goto exit;
        }

    }

    log_info("elink qlink get ssid :%s, pwd : %s，start conneting.....", __this->elink_ssid, __this->elink_pwd);

    is_get_ssid_pwd_ok = 1;//成功获取到配网信息

    elink_qlink_set_ssid_pwd(__this->elink_ssid, __this->elink_pwd);//开始连接网络

exit:

    sock_unreg(sock);//关闭sock

    if (is_get_ssid_pwd_ok == 0) { //获取配网信息失败
        log_info("Failed to connect to the Internet, please help me connect to the Internet again");
        extern void elink_qlink_timeout(void);
        elink_qlink_timeout();//该接口为配网失败处理，这里为播放配网失败语音，根据情况修改
        //extern u8 elink_qlink_is_start_reset(void);//防止配网失败后，重新连接隐藏wifi，需要重新按配网键进行配网
        //elink_qlink_is_start_reset();
    }
}

int elink_qlink_task(void)
{
    if (thread_fork("elink_qlink_start", 10, 2 * 1024, 32, NULL, elink_qlink_start, NULL) != OS_NO_ERR) {
        log_info("%s thread fork fail\n", __FILE__);
        return -1;
    }
    return 0;
}
