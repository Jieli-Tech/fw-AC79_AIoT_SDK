#include "sock_api/sock_api.h"
#include "mqtt/MQTTClient.h"
#include "echo_random.h"
#include "echo_cloud.h"
#include <time.h>

#define MQTT_DEBUG_EN          1
#if MQTT_DEBUG_EN
#define MQTT_DEBUG              log_d
#define MQTT_LOGV               log_v
#define MQTT_LOGD               log_d
#define MQTT_LOGI               log_i
#define MQTT_LOGW               log_w
#define MQTT_LOGE               log_e
#define MQTT_LOGWTF             log_i
#else
#define MQTT_DEBUG(_l,...)
#define MQTT_LOGV(...)
#define MQTT_LOGD(...)
#define MQTT_LOGI(...)
#define MQTT_LOGW(...)
#define MQTT_LOGE(...)
#define MQTT_LOGWTF(...)
#endif


#undef	MAX_MESSAGE_HANDLERS
#define MAX_MESSAGE_HANDLERS			1

#define DEV_MQTT_QOS					QOS2

#define MQTT_SEND_BUF_LEN               1024
#define MQTT_READ_BUF_LEN               (1024 * 2)

#define DEV_ENT_RECV_TIMEOUT          	1000
#define DEV_ENT_SEND_TIMEOUT          	1000

#define DEV_MQTT_KEEPALIVE_TIME       	(30 * 1000)
#define MQTT_PUT_TO_MS      			(2*100)

#define MQTT_SERVER_HOST 				"mqtt.echocloud.com"
#define MQTT_SERVER_PORT 				1883


enum {
    ECHO_CLOUD_MQTT_ERR_MALLOC,
    ECHO_CLOUD_MQTT_ERR_PTR,
    ECHO_CLOUD_MQTT_ERR_STU,
    ECHO_CLOUD_MQTT_ERR_CLOSE,
    ECHO_CLOUD_MQTT_ERR_SEND,
    ECHO_CLOUD_MQTT_ERR_USER,
    ECHO_CLOUD_MQTT_ERR_JSON,
};

struct dev_mqtt_msg_hdl {
    char topicFilter[UUID_LEN + DEVICE_ID_LEN + 5 + 1];
    void (*fp)(MessageData *);
};

struct dev_mqtt_hdl {
    Client c;
    Network n;
    MQTTPacket_connectData data;

    struct echo_cloud_auth_parm *auth;

    unsigned char sendbuf[MQTT_SEND_BUF_LEN];
    unsigned char recvbuf[MQTT_READ_BUF_LEN];

    u8 ready;
    u8 stu;

    u16  port;
    char host[19];
    char password[PASSWORD_LEN + 1];
    char username[USERNAME_LEN + 1];

    struct dev_mqtt_msg_hdl msg[MAX_MESSAGE_HANDLERS];
};

static const MQTTPacket_connectData MQTTPacket_default = MQTTPacket_connectData_initializer;

static int echo_mqtt_task_pid;
static u8 volatile echo_mqtt_exit_flag = 0;


static void dev_mqtt_user_msg(MessageData *md)
{
    MQTTMessage *message = md->message;
    u8 *msg = (u8 *)malloc(message->payloadlen + 1);
    if (!msg) {
        return;
    }

    memcpy(msg, message->payload, message->payloadlen);
    msg[message->payloadlen] = 0;

    //消息全放在同一线程里处理
    if (OS_NO_ERR != os_taskq_post("echo_cloud_app_task", 2, ECHO_CLOUD_MQTT_MSG_DEAL, msg)) {
        free(msg);
    }
}

static int sock_cb_func(enum sock_api_msg_type type, void *priv)
{
    if (echo_mqtt_exit_flag) {
        return -1;
    }
    return 0;
}

static int dev_net_read(Network *n, unsigned char *buffer, int len, unsigned long timeout_ms)
{
    int bytes;

    bytes = sock_recv(n->my_socket, buffer, len, 0);
    if (bytes <= 0) {
        if (bytes == 0 || !sock_would_block(n->my_socket)) {
            n->state = -1;
            MQTT_LOGE("Socket %d recv ret %d, error %d \n", *(int *)n->my_socket, bytes, sock_get_error(n->my_socket));
        }
        bytes = -1;
    }

    return bytes;
}

static int dev_net_write(Network *n, unsigned char *buffer, int len, unsigned long timeout_ms)
{
    int bytes;

    bytes = sock_send(n->my_socket, buffer, len, 0);
    if (bytes <= 0) {
        if (bytes == 0 || !sock_would_block(n->my_socket)) {
            n->state = -1;
            MQTT_LOGE("Socket %d send ret %d, error %d \n", *(int *)n->my_socket, bytes, sock_get_error(n->my_socket));
        }
        bytes = -1;
    }

    return bytes;
}

static void dev_net_disconnect(Network *n)
{
    sock_unreg(n->my_socket);
    n->my_socket = NULL;
}

static void newNetwork(Network *n)
{
    n->my_socket  = NULL;
    n->mqttread   = dev_net_read;
    n->mqttwrite  = dev_net_write;
    n->disconnect = dev_net_disconnect;
}

static int connectNetwork(Network *n, char *addr, u16 port)
{
    struct sockaddr_in address;
    int rc = -1;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

    if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0) {
        struct addrinfo *res = result;

        /* prefer ip4 addresses */
        while (res) {
            if (res->ai_family == AF_INET) {
                result = res;
                break;
            }
            res = res->ai_next;
        }

        if (result->ai_family == AF_INET) {
            address.sin_port = htons(port);
            address.sin_family = AF_INET;
            address.sin_addr = ((struct sockaddr_in *)(result->ai_addr))->sin_addr;
        } else {
            rc = -1;
        }

        freeaddrinfo(result);
    }

    if (rc == 0) {
        n->state = 0;
        n->my_socket = sock_reg(AF_INET, SOCK_STREAM, 0, sock_cb_func, n);
        if (n->my_socket) {
            sock_set_recv_timeout(n->my_socket, DEV_ENT_RECV_TIMEOUT);
            rc = sock_connect(n->my_socket, (struct sockaddr *)&address, sizeof(address));
            if (rc) {
                MQTT_LOGE("mqtt_sock connect err = %d \r\n", rc);
            }
        } else {
            rc = -1;
        }
    }

    return rc;
}

static int dev_mqtt_start(struct dev_mqtt_hdl *mqtt)
{
    int i, ret;

    if (mqtt->stu) {
        return ECHO_CLOUD_MQTT_ERR_STU;
    }

    mqtt->n.priv = &mqtt->c;
    newNetwork(&mqtt->n);

    ret = connectNetwork(&mqtt->n, mqtt->host, mqtt->port);
    if (ret) {
        MQTT_LOGE("connectNetwork error :%d \n", ret);
        goto __mqtt_start_err1;
    }
    MQTTClient(&mqtt->c, &mqtt->n, 1000, mqtt->sendbuf, MQTT_SEND_BUF_LEN, mqtt->recvbuf, MQTT_READ_BUF_LEN);

    memcpy(&mqtt->data, &MQTTPacket_default, sizeof(MQTTPacket_connectData));
    mqtt->data.willFlag = 0;
    mqtt->data.MQTTVersion = 4;
    mqtt->data.clientID.cstring = mqtt->auth->device_id;
    mqtt->data.username.cstring = mqtt->username;
    mqtt->data.password.cstring = mqtt->password;
    mqtt->data.keepAliveInterval = DEV_MQTT_KEEPALIVE_TIME / 1000;
    mqtt->data.cleansession = 1;

    ret = MQTTConnect(&mqtt->c, &mqtt->data);
    if (ret) {
        MQTT_LOGE("MQTTConnect error :%d \n", ret);
        goto __mqtt_start_err1;
    }

    for (i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
        if (!mqtt->msg[i].topicFilter[0]) {
            break;
        }
        /* printf("sub fp:0x%x , topic: %s \n", g_jl_mqtt->msg[i].fp, g_jl_mqtt->msg[i].topicFilter); */
        ret = MQTTSubscribe(&mqtt->c, mqtt->msg[i].topicFilter, DEV_MQTT_QOS, mqtt->msg[i].fp);
        if (ret) {
            MQTT_LOGE("MQTTSubscribe error :%d \n", ret);
            goto __mqtt_start_err;
        }
    }

    mqtt->stu = 1;

    MQTT_LOGI("mqtt start ok \n");

    return 0;

__mqtt_start_err:
    MQTTDisconnect(&mqtt->c);
__mqtt_start_err1:
    mqtt->n.disconnect(&mqtt->n);
    mqtt->stu = 0;

    return ret;
}

static void dev_mqtt_stop(struct dev_mqtt_hdl *mqtt)
{
    MQTT_LOGI("dev_mqtt_stop_1\n");

    if (!mqtt->stu) {
        return;
    }

    MQTTDisconnect(&mqtt->c);
    mqtt->n.disconnect(&mqtt->n);
    mqtt->stu = 0;

    MQTT_LOGI("dev_mqtt_stop_2\n");
}

static int dev_mqtt_opr(struct dev_mqtt_hdl *mqtt)
{
    if (!mqtt->stu) {
        return ECHO_CLOUD_MQTT_ERR_STU;
    }

    return MQTTYield(&mqtt->c, MQTT_PUT_TO_MS);
}

static void dev_mqtt_update_user_info(struct dev_mqtt_hdl *mqtt)
{
    char nonce[33];

    avGenRand(nonce, 32);
    nonce[32] = 0;
    echo_cloud_get_hmac_sha256(mqtt->auth->device_id, mqtt->auth->channel_uuid, nonce, mqtt->password, mqtt->auth->hash);
    mqtt->password[PASSWORD_LEN] = 0;

    snprintf(mqtt->username, USERNAME_LEN + 1, "%s.%s.%s", mqtt->auth->channel_uuid, mqtt->auth->device_id, nonce);
}

static void dev_mqtt_set_server(struct dev_mqtt_hdl *mqtt, const char *host, u16 port)
{
    strcpy(mqtt->host, host);
    mqtt->port = port;

    for (u8 i = 0; i < MAX_MESSAGE_HANDLERS; i++) {
        sprintf(mqtt->msg[i].topicFilter, "%s/%s/sub", mqtt->auth->channel_uuid, mqtt->auth->device_id);
        mqtt->msg[i].fp = dev_mqtt_user_msg;
    }
}

static void echo_cloud_mqtt_task(void *priv)
{
    u32 delay_cnt = 200;
    struct dev_mqtt_hdl __mqtt;
    struct dev_mqtt_hdl *mqtt = &__mqtt;

    memset(mqtt, 0, sizeof(struct dev_mqtt_hdl));

    mqtt->auth = (struct echo_cloud_auth_parm *)priv;

    dev_mqtt_set_server(mqtt, MQTT_SERVER_HOST, MQTT_SERVER_PORT);

    while (1) {
        if (mqtt->ready) {
            delay_cnt = 200;
        } else {
            delay_cnt <<= 1;
            if (delay_cnt >  30 * 1000) {
                delay_cnt = 200;
            }
        }

        if (echo_mqtt_exit_flag) {
            mqtt->ready = 0;
            dev_mqtt_stop(mqtt);
            return;
        }

        if (!mqtt->ready) {
            os_time_dly(delay_cnt / 10);

            // mqtt每次连接都要重新生成username和password
            MQTT_LOGI("dev get mqtt user...\n");

            dev_mqtt_update_user_info(mqtt);

            MQTT_LOGI("username:%s  password:%s  device_id:%s  channel_uuid:%s  topic:%s  host:%s  port:%d\n"
                      , mqtt->username
                      , mqtt->password
                      , mqtt->auth->device_id
                      , mqtt->auth->channel_uuid
                      , mqtt->msg[0].topicFilter
                      , mqtt->host
                      , mqtt->port);

            if (0 == dev_mqtt_start(mqtt)) {
                mqtt->ready = 1;
            }
        }

        if (dev_mqtt_opr(mqtt)) {
            // mqtt 有错误，重新连接
            dev_mqtt_stop(mqtt);
            mqtt->ready = 0;
        }
    }
}

int echo_cloud_mqtt_init(struct echo_cloud_auth_parm *auth)
{
    echo_mqtt_exit_flag = 0;

    return thread_fork("echo_cloud_mqtt_task", 15, 1536, 0, &echo_mqtt_task_pid, echo_cloud_mqtt_task, auth);
}

void echo_cloud_mqtt_uninit(void)
{
    echo_mqtt_exit_flag = 1;

    thread_kill(&echo_mqtt_task_pid, KILL_WAIT);

    echo_mqtt_exit_flag = 0;
}

