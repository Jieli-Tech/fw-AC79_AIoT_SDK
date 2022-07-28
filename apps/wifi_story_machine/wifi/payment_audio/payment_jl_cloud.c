#include "server/ai_server.h"
#include "system/database.h"
#include "os/os_api.h"
#include "sock_api/sock_api.h"
#include "http/http_cli.h"
#include "mqtt/MQTTClient.h"
#include "json_c/json_object.h"
#include "time.h"
#include "avctp_user.h"
#include "asm/port_waked_up.h"
#include "asm/gpio.h"
#include "app_config.h"

#if 1
#define	log_info(x, ...)	printf("\n------------[payment_jl_cloud]###" x "----------------\r\n",##__VA_ARGS__)
#else
#define	log_info(...)
#endif

/* #define PAYMENT_LOG		//need open debug.c -> CONFIG_NETWORK_DEBUG_ENABLE */
#ifdef PAYMENT_LOG
#define log_time	(100* 10)
#endif


extern int get_app_music_volume(void);

static char msgid_first[30], msgid_second[30];

struct	payment_msg {
    char buf[30];
    char msgid[30];
    u8 volume;
    u8 type;
};


enum JL_CLOUD_SDK_EVENT {
    JL_CLOUD_MEDIA_END     = 0x02,
    JL_CLOUD_QUIT    	   = 0xff,
};


typedef struct {
    char robotId[40];
    char deviceID[40];
    char uuid[48];
    char clientid[64];
    char bind_token[64];
    char username[64];
    char password[64];
    char *text;
} MUTUAL_PARAM;

typedef struct {
    httpcli_ctx ctx;
    u8 connect_status;
} MUTUAL_HDL;

typedef struct {
    char access_token[1024];
    char url[1460];
} MUTUAL_REPLY;
struct jl_cloud_para {

    MUTUAL_PARAM param;
    MUTUAL_REPLY reply;
    MUTUAL_HDL hdl;
};

struct jl_cloud_var {
    struct jl_cloud_para para;
    u8 voice_mode;
    u8 exit_flag;
    u8 use_vad;
};


#define JL_CLOUD_MQTT_QOS QOS0
#define MQTT_BUF_SIZE               (1024 * 10)    /*MQTT buf size*/
#define MQTT_PROC_BUF_SIZE          MQTT_BUF_SIZE


static u8 payment_jl_cloud_init_flag = 0;
static struct jl_cloud_var jl_cloud_hdl;
#define __this      (&jl_cloud_hdl)


typedef struct {
    Network     network;
    Client      client;
    uint8_t     send_buffer[MQTT_PROC_BUF_SIZE];
    uint8_t     recv_buffer[MQTT_PROC_BUF_SIZE];
    bool        client_connected;
    uint8_t     exit_flag;
    /* int         task_pid; */
} JL_CLOUD_MQTT;

static JL_CLOUD_MQTT payment_jl_cloud_mqtt;

const struct ai_sdk_api payment_audio_cloud_api;

extern struct json_object *json_tokener_parse(const char *str);

extern void payment_audio_open(void *priv);
extern void payment_audio_kill();
extern void upload_log_trig(void);
extern void dev_profile_init(void);


int payment_jl_cloud_device_unbind(void)
{
    extern int lwip_dhcp_bound(void);
    if (!lwip_dhcp_bound()) {
        return -2;
    }
    int ret;
    char *url = NULL;
    json_object *root_node = NULL;
    http_body_obj *http_recv_body = NULL;
    httpcli_ctx *ctx = NULL;
    http_recv_body = (http_body_obj *)calloc(1, sizeof(http_body_obj));
    if (http_recv_body == NULL) {
        log_info("%s %d unbind calloc err", __func__, __LINE__);
        return -1;
    }
    ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (ctx == NULL) {
        log_info("%s %d unbind calloc err", __func__, __LINE__);
        ret = -1;
        goto unbind_exit;
    }
    http_recv_body->recv_len = 0;
    http_recv_body->buf_len = 1024;
    http_recv_body->buf_count = 1;
    url = (char *)calloc(1, 256);
    if (url == NULL) {
        log_info("%s %d unbind calloc err", __func__, __LINE__);
        ret = -1;
        goto unbind_exit;
    }
    strcpy(url, "http://log.jieliapp.com/payassistant/v1/device/unbind");
    http_recv_body->p = (char *)calloc(http_recv_body->buf_count, http_recv_body->buf_len);
    if (http_recv_body->p == NULL) {
        log_info("%s %d unbind calloc err", __func__, __LINE__);
        ret = -1;
        goto unbind_exit;
    }

    ctx->data_format = "application/json";
    ctx->data_len = strlen("{}");
    ctx->post_data = "{}";
    ctx->timeout_millsec = 5000;
    ctx->priv = http_recv_body;
    ctx->url = url;
    ctx->connection = "close";
    ctx->cookie = &(&__this->para)->reply.access_token;
    ret = httpcli_post(ctx);
    if (ret) {
        printf("%s %d \r\n", __func__, __LINE__);
        goto unbind_exit;
    }
    if (http_recv_body->p == NULL) {
        ret = -1;
    } else {
        log_info("unbind = %s\r\n", (unsigned char *)http_recv_body->p);
        root_node = json_tokener_parse((unsigned char *)http_recv_body->p);
        if (json_object_get_int(json_object_object_get(root_node, "code")) == 0) {
            if (strcmp(json_object_get_string(json_object_object_get(root_node, "data")), "true") == 0) {
                ret = 0;    //success
            } else {
                ret = 1;    //unbind already
            }
        } else {
            ret = -1;    //fail
        }
    }


unbind_exit:
    json_object_put(root_node);
    if (http_recv_body->p) {
        free(http_recv_body->p);
    }
    if (http_recv_body) {
        free(http_recv_body);
    }
    if (ctx) {
        free(ctx);
    }
    if (url) {
        free(url);
    }


    return ret;
}

static int payment_jl_cloud_mqtt_ctl_state(char *msgid)
{
    struct jl_cloud_var *var = __this;
    struct jl_cloud_para *para = (struct jl_cloud_para *)&var->para;

    char buf[256];
    if (payment_jl_cloud_mqtt.client_connected == FALSE) {
        return -1;
    }


    sprintf(buf, "{\n\t\"msgid\": \"%s\",\n\t\"status\": \"true\",\n\t\"ts\":%ld\n}", msgid,
            time(NULL));


    MQTTMessage message;
    message.id         = 0;
    message.qos        = JL_CLOUD_MQTT_QOS;
    message.retained   = false;
    message.dup        = false;
    message.payload    = buf;
    message.payloadlen = strlen(buf);

    char topic[128];
    sprintf(topic, "iot/payassistant/%s/device/pub/payment", para->param.clientid);
    MQTTPublish(&payment_jl_cloud_mqtt.client, topic, &message);
    return 0;
}




static int payment_jl_cloud_device_broadcast(struct jl_cloud_para *para)
{
    int ret = 0;
    http_body_obj http_recv_body = {0};
    httpcli_ctx ctx = {0};

    http_recv_body.recv_len = 0;
    http_recv_body.buf_len = 2048;
    http_recv_body.buf_count = 1;
    http_recv_body.p = (char *)malloc(http_recv_body.buf_len * http_recv_body.buf_count);
    if (http_recv_body.p == NULL) {
        log_e("\n %s %d mallloc err\n", __func__, __LINE__);
        ret = -1;
        goto boardcast_exit;
    }
    char url[1024];
    u8 mac_addr[6];
    wifi_get_mac(mac_addr);

    snprintf(url, sizeof(url), "http://log.jieliapp.com/payassistant/v1/device/broadcast?clientid=%s&deviceMac=%02X%02X%02X%02X%02X%02X",
             para->param.clientid,
             mac_addr[0],
             mac_addr[1],
             mac_addr[2],
             mac_addr[3],
             mac_addr[4],
             mac_addr[5]);

    ctx.data_format = "application/json";
    ctx.data_len = strlen("{}");
    ctx.post_data = "{}";
    ctx.timeout_millsec = 5000;
    ctx.priv = &http_recv_body;
    ctx.url = url;
    ctx.connection = "close";
    ctx.cookie = para->reply.access_token;
    ret = httpcli_post(&ctx);
    if (ret) {
        goto boardcast_exit;
    }
    log_info("\n broadcast buf = %s\n", (unsigned char *)http_recv_body.p);
boardcast_exit:
    if (http_recv_body.p) {
        free(http_recv_body.p);
    }
    return ret;
}

static void payment_init_jl_cloud_para(struct jl_cloud_para *para)
{

    extern int get_page_turning_profile(char *username, char *password);
    extern int get_payment_bind_msg(char *uuid, char *clientid, char *bind_token);
    while (get_page_turning_profile(para->param.username, para->param.password)) {
        os_time_dly(1000);
    }
    while (get_payment_bind_msg(para->param.uuid, para->param.clientid, para->param.bind_token)) {
        os_time_dly(1000);
    }

    log_info("\nuuid = %s clientid = %s username = %s password = %s bind_token = %s", para->param.uuid, para->param.clientid, para->param.username, para->param.password, para->param.bind_token);

}

static int get_payment_jl_cloud_token_cb(u8 *buf, void *priv)
{
    log_info("\ntoken cb buf = %s\n", buf);

    int ret = 0;
    struct jl_cloud_para *para = (struct jl_cloud_para *)priv;
    json_object *root_node = NULL, *first_node = NULL, *second_node = NULL;

    root_node = json_tokener_parse((const char *)buf);
    if (!root_node) {
        return -1;
    }
    if (!json_object_object_get_ex(root_node, "data", &first_node)) {
        ret = -1;
        goto token_cb_exit;
    }
    if (!json_object_object_get_ex(first_node, "access_token", &second_node)) {
        ret = -1;
        goto token_cb_exit;
    }
    log_info("\npara->reply.access_token len  = %lu \n", strlen(json_object_get_string(second_node)));
    log_info("\npara->reply.access_token = %s \n", json_object_get_string(second_node));

    sprintf(para->reply.access_token, "%s", "jwt-token=");
    sprintf(para->reply.access_token + strlen("jwt-token="), "%s", json_object_get_string(second_node));

    para->hdl.connect_status = 1;
token_cb_exit:
    json_object_put(root_node);
    return ret;
}

static int get_payment_jl_cloud_token(struct jl_cloud_para *para)
{
    char *url = NULL;
    http_body_obj http_recv_body = {0};
    int ret = 0;
    httpcli_ctx ctx = {0};

    url = calloc(1, 1024);
    if (!url) {
        goto token_exit;
    }
    sprintf(url, "http://log.jieliapp.com/payassistant/v1/auth/login?username=%s&password=%s&type=device", \
            para->param.username,
            para->param.password);
    http_recv_body.recv_len = 0;
    http_recv_body.buf_len = 2048;
    http_recv_body.buf_count = 1;
    http_recv_body.p = (char *)malloc(http_recv_body.buf_len * http_recv_body.buf_count);
    if (http_recv_body.p == NULL) {
        log_e("\n %s %d mallloc err\n", __func__, __LINE__);
        ret = -1;
        goto token_exit;
    }
    ctx.timeout_millsec = 5000;
    ctx.priv = &http_recv_body;
    ctx.url = url;
    ctx.connection = "close";
    ret = httpcli_post(&ctx);
    if (ret) {
        goto token_exit;
    }
    ret = get_payment_jl_cloud_token_cb((unsigned char *)http_recv_body.p, para);
token_exit:
    if (http_recv_body.p) {
        free(http_recv_body.p);
    }
    if (url) {
        free(url);
    }
    return ret;
}



static void payment_mqtt_msg_callback(MessageData *md)
{
    json_object *root_node = NULL, *first_node = NULL, *second_node = NULL, *third_node = NULL;

    *((char *)md->message->payload +  md->message->payloadlen)  = 0;
    log_info("\n callback  buf = %s\n", md->message->payload);
    root_node = json_tokener_parse((char *)md->message->payload);
    struct jl_cloud_var *var = __this;
    struct jl_cloud_para *para = (struct jl_cloud_para *)&var->para;

    u8 get_bt_connect_status(void);
    u8 ret = get_bt_connect_status();

    if (!root_node) {
        log_info("root NULL,%d", __LINE__);
        return;
    }
    u8 pay_type = 0;
    first_node = json_object_object_get(root_node, "type");
    if (!strcmp(json_object_get_string(first_node), "payment")) {
        if (json_object_object_get_ex(root_node, "application", &first_node) && json_object_object_get_ex(root_node, "amount", &second_node)
            && json_object_object_get_ex(root_node, "msgid", &third_node)) {
            if (!strcmp(json_object_get_string(first_node), "wxpay")) {
                pay_type = 1;
            } else if (!strcmp(json_object_get_string(first_node), "alipay")) {
                pay_type = 2;
            }
            memset(msgid_first, 0, sizeof(msgid_first));
            strcpy(msgid_first, json_object_get_string(third_node));
            if ((ret != BT_STATUS_INITING && ret != BT_STATUS_WAITINT_CONN && ret != BT_STATUS_AUTO_CONNECTINT) || strcmp(msgid_second, msgid_first) == 0) {
                os_taskq_post("payment_jl_cloud_task", 2, 2, msgid_first);
                return;
            }
            memset(msgid_second, 0, sizeof(msgid_second));
            strcpy(msgid_second, msgid_first);

            struct payment_msg *post_msg = (struct payment_msg *)calloc(1, sizeof(struct payment_msg));
            strcpy(post_msg->buf, json_object_get_string(second_node));
            strcpy(post_msg->msgid, msgid_second);
            post_msg->volume = get_app_music_volume();
            post_msg->type = pay_type;

            os_taskq_post("payment_audio_play_task", 1, 1001);
            os_taskq_post("payment_audio_open", 1, post_msg);

        }
    }


    json_object_put(root_node);
}

static int sock_cb_func(enum sock_api_msg_type type, void *priv)
{
    return 0;
}


static int payment_jl_cloud_mqtt_connect(struct jl_cloud_para *para)
{
    char topic[128] = {0};
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;

    NewNetwork(&payment_jl_cloud_mqtt.network);
    SetNetworkCb(&payment_jl_cloud_mqtt.network, sock_cb_func, NULL);

    if (0 != ConnectNetwork(&payment_jl_cloud_mqtt.network, "log.jieliapp.com", 1883)) {
        printf("\n %s %d\n", __func__, __LINE__);
        payment_jl_cloud_mqtt.network.disconnect(&payment_jl_cloud_mqtt.network);
        return -1;
    }

    SetNetworkRecvTimeout(&payment_jl_cloud_mqtt.network, 1000);


    MQTTClient(
        &payment_jl_cloud_mqtt.client,
        &payment_jl_cloud_mqtt.network,
        2000,
        payment_jl_cloud_mqtt.send_buffer,
        MQTT_PROC_BUF_SIZE,
        payment_jl_cloud_mqtt.recv_buffer,
        MQTT_PROC_BUF_SIZE);


    data.MQTTVersion       = 4;
    data.clientID.cstring  = para->param.clientid;
    data.username.cstring  = para->param.username;
    data.password.cstring  = para->param.password;
    data.keepAliveInterval = 30;
    data.cleansession      = 1;
    data.willFlag          = 0;

    if (0 != MQTTConnect(&payment_jl_cloud_mqtt.client, &data)) {
        printf("\n %s %d\n", __func__, __LINE__);
        payment_jl_cloud_mqtt.network.disconnect(&payment_jl_cloud_mqtt.network);
        return -1;
    }

    sprintf(topic, "iot/payassistant/%s/device/sub/#", para->param.clientid);
    if (0 != MQTTSubscribe(&payment_jl_cloud_mqtt.client, topic, JL_CLOUD_MQTT_QOS, payment_mqtt_msg_callback)) {
        printf("\n %s %d\n", __func__, __LINE__);
        MQTTDisconnect(&payment_jl_cloud_mqtt.client);
        payment_jl_cloud_mqtt.network.disconnect(&payment_jl_cloud_mqtt.network);
        return -1;
    }

    payment_jl_cloud_mqtt.client_connected = TRUE;

    return 0;
}

static int payment_jl_cloud_mqtt_yield(JL_CLOUD_MQTT *mqtt)
{
    if (payment_jl_cloud_mqtt.client_connected == TRUE) {
        return MQTTYield(&mqtt->client, 1000);
    }
    return -1;

}

static int payment_jl_cloud_mqtt_disconnect(struct jl_cloud_para *para)
{
    if (payment_jl_cloud_mqtt.client_connected ==  TRUE) {
        MQTTDisconnect(&payment_jl_cloud_mqtt.client);
        payment_jl_cloud_mqtt.network.disconnect(&payment_jl_cloud_mqtt.network);
        payment_jl_cloud_mqtt.client_connected = FALSE;
    }
    return 0;
}
static void payment_jl_cloud_mqtt_task(void *priv)
{
    struct jl_cloud_para *para = (struct jl_cloud_para *)priv;

    while (1) {
        if (payment_jl_cloud_mqtt.exit_flag) {
            break;
        }
        if (payment_jl_cloud_mqtt.client_connected ==  FALSE) {
            payment_jl_cloud_mqtt_connect(para);
        }

        if (payment_jl_cloud_mqtt_yield(&payment_jl_cloud_mqtt)) {
            payment_jl_cloud_mqtt_disconnect(para);
        }

    }
    payment_jl_cloud_mqtt_disconnect(para);
}

static int payment_jl_cloud_mqtt_init(struct jl_cloud_para *para)
{
    payment_jl_cloud_mqtt.exit_flag = 0;
    return thread_fork("payment_jl_cloud_mqtt_task", 10, 2048, 32, NULL, payment_jl_cloud_mqtt_task, para);
}
#ifdef PAYMENT_LOG
static void payment_log_report(void *priv)
{

    while (1) {
        upload_log_trig();
        os_time_dly(log_time);
    }
}
#endif

static void payment_jl_cloud_task(void *priv)
{
    int err;
    int msg[32];
    struct jl_cloud_var *p = (struct jl_cloud_var *)priv;
    struct jl_cloud_para *para = &p->para;

    u32 delay_ms = 1000;
    u8 dly_cnt = 0;
    u8 retry = 5;


    payment_init_jl_cloud_para(para);

#ifdef PAYMENT_LOG
    thread_fork("payment_log_report", 20, 512, 20, NULL, payment_log_report, NULL);
#endif

    while (retry > 0 && !__this->exit_flag) {
        if (dly_cnt--) {
            os_time_dly(10);
            continue;
        }

        err = get_payment_jl_cloud_token(para);		//没有网或者没有获得username跟password，一直尝试循环连接,直到连接成功 获取token ---------设备登录
        if (!err) {
            break;
        }
        --retry;
        delay_ms <<= 1;
        dly_cnt = delay_ms / 100;
    };

    if (err || __this->exit_flag) {
        goto cloud_task_exit;
    }
    payment_jl_cloud_mqtt_init(para);		//订阅主题并且循环检测mqtt是否在线

    /* payment_jl_cloud_device_broadcast(para);	//广播以被手机端发现 ---可选 */

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        //统一在这里给网络线程发消息
        switch (msg[1]) {
        case JL_CLOUD_MEDIA_END:
            payment_jl_cloud_mqtt_ctl_state((char *)msg[2]);
            break;

        case JL_CLOUD_QUIT:

            goto cloud_task_exit;
        default:
            break;
        }
    }

cloud_task_exit:
    printf("\n %s  __exit \n", __func__);
}






static int payment_jl_cloud_init(void)
{
    struct jl_cloud_para *para = &__this->para;
    if (!payment_jl_cloud_init_flag) {
        payment_jl_cloud_init_flag = 1;
        __this->exit_flag = 0;
        para->hdl.connect_status = 0;
        return thread_fork("payment_jl_cloud_task", 15, 4096, 128, NULL, payment_jl_cloud_task, __this);
    }
    return -1;
}


static void payment_jl_cloud_mqtt_uninit(void)
{
    payment_jl_cloud_mqtt.exit_flag = 1;

}

static void payment_jl_cloud_uninit(void)
{
    struct jl_cloud_para *para = &__this->para;
    if (payment_jl_cloud_init_flag) {
        payment_jl_cloud_init_flag = 0;
        para->hdl.connect_status = 0;
        __this->exit_flag = 1;
        do {
            if (OS_Q_FULL != os_taskq_post("payment_jl_cloud_task", 1, JL_CLOUD_QUIT)) {
                break;
            }
            log_e("payment_jl_cloud_task send msg QUIT timeout \n");
            os_time_dly(5);
        } while (1);

        payment_jl_cloud_mqtt_uninit();
    }
}

int payment_audio_cloud_disconnect()
{
    payment_jl_cloud_uninit();

    payment_audio_kill();
    os_taskq_post("payment_audio_open", 1, 0);

    return 0;
}

static int payment_audio_cloud_open()
{


    thread_fork("payment_audio_open", 20, 512, 512, NULL, payment_audio_open, NULL);

    /* Port_Wakeup_Reg(EVENT_IO_1, IO_PORTC_05, EDGE_POSITIVE, payment_audio_cloud_disconnect, NULL); */

    return payment_jl_cloud_init();
}



u8 payment_jl_cloud_get_connect_status(void)
{
    struct jl_cloud_para *para = &__this->para;
    return para->hdl.connect_status;
}


static int payment_jl_cloud_check()
{
    if (payment_jl_cloud_get_connect_status()) {
        return AI_STAT_CONNECTED;
    }
    return AI_STAT_DISCONNECTED;
}

static int payment_jl_cloud_do_event(int event, int arg)
{
    return 0;
}


#if (defined PAYMENT_AUDIO_SDK_ENABLE)				//打开语音播报功能

REGISTER_AI_SDK(payment_audio_cloud_api) = {
    .name           = "payment_audio_cloud",
    .connect        = payment_audio_cloud_open,
    .state_check    = payment_jl_cloud_check,
    .do_event       = payment_jl_cloud_do_event,
    .disconnect     = payment_audio_cloud_disconnect,
};
#endif
