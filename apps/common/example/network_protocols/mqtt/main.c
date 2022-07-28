/***********************************MQTT 测试说明*******************************************
 *说明：
 * 	 通过MQTT协议连接阿里云, 向阿里云订阅主题和发布温度，湿度消息
 *********************************************************************************************/
#include "mqtt/MQTTClient.h"
#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "app_config.h"

#ifdef USE_MQTT_TEST

#define COMMAND_TIMEOUT_MS 30000        //命令超时时间
#define MQTT_TIMEOUT_MS 10000           //接收阻塞时间
#define MQTT_KEEPALIVE_TIME 30000       //心跳时间
#define SEND_BUF_SIZE 1024              //发送buf大小
#define READ_BUF_SIZE 1024              //接收buf大小
static char send_buf[SEND_BUF_SIZE];    //发送buf
static char read_buf[READ_BUF_SIZE];    //接收buf

//接收回调，当订阅的主题有信息下发时，在这里接收
static void messageArrived(MessageData *data)
{
    char temp[128] = {0};

    strncpy(temp, data->topicName->lenstring.data, data->topicName->lenstring.len);
    temp[data->topicName->lenstring.len] = '\0';
    printf("Message arrived on topic (len : %d, topic : %s)\n", data->topicName->lenstring.len, temp);

    memset(temp, 0, sizeof(temp));
    strncpy(temp, data->message->payload, data->message->payloadlen);
    temp[data->message->payloadlen] = '\0';
    printf("message (len : %d, payload : %s)\n", data->message->payloadlen, temp);
}

static int mqtt_start(void)
{
    Client client;
    Network network;
    MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
    MQTTMessage message;
    int err;
    int loop_cnt = 0;
    int temperature = 0;
    int humidity = 0;

    char sendbuf[256];
    char payload[256] = "{\"id\":\"161848123\",\"version\":\"1.0\",\"params\":{\"temperature\":%d,\"Humidity\":%d},\"method\":\"thing.event.property.post\"}";
    char *address = "a1mhUx5m6Uq.iot-as-mqtt.cn-shanghai.aliyuncs.com";
    char *username = "iot-001&a1mhUx5m6Uq";
    char *password = "083dc23c92b1aad822c7c1f1a3c687dda022089b";
    char *subscribeTopic = "/a1mhUx5m6Uq/iot-001/user/get";                      //订阅的主题
    char *publishTopic = "/sys/a1mhUx5m6Uq/iot-001/thing/event/property/post";   //发布消息的主题
    char *clientID = "12345|securemode=3,signmethod=hmacsha1|";

_reconnect:
    //初始化网络接口
    NewNetwork(&network);

    SetNetworkRecvTimeout(&network, 1000);

    //初始化客户端
    MQTTClient(&client, &network, COMMAND_TIMEOUT_MS, send_buf, sizeof(send_buf), read_buf, sizeof(read_buf));

    //tcp层连接服务器
    err = ConnectNetwork(&network, address, 1883);
    if (err != 0) {
        printf("ConnectNetwork fail\n");
        return -1;
    }

    connectData.willFlag = 0;
    connectData.MQTTVersion = 3;                                   //mqtt版本号
    connectData.clientID.cstring = clientID;                       //客户端id
    connectData.username.cstring = username;                       //连接时的用户名
    connectData.password.cstring = password;                       //连接时的密码
    connectData.keepAliveInterval = MQTT_KEEPALIVE_TIME / 1000;    //心跳时间
    connectData.cleansession = 1;                                  //是否使能服务器的cleansession，0:禁止, 1:使能

    //mqtt层连接,向服务器发送连接请求
    err = MQTTConnect(&client, &connectData);
    if (err != 0) {
        network.disconnect(&network);
        printf("MQTTConnect fail, err : 0x%x\n", err);
        return -1;
    }

    //订阅主题
    err = MQTTSubscribe(&client, subscribeTopic, QOS1, messageArrived);
    if (err != 0) {
        MQTTDisconnect(&client);
        network.disconnect(&network);
        printf("MQTTSubscribe fail, err : 0x%x\n", err);
        return -1;
    }

    //取消主题订阅
    //MQTTUnsubscribe(&client, subscribeTopic);

    message.qos = QOS1;
    message.retained = 0;

    while (1) {
        if (0 == loop_cnt % 2) {
            sprintf(sendbuf, payload, temperature, humidity);
            message.payload = sendbuf;
            message.payloadlen = strlen(sendbuf) + 1;

            temperature += 1;
            humidity += 2;

            if (temperature > 100) {
                temperature = 0;
            }

            if (humidity > 100) {
                humidity = 0;
            }

            //发布消息
            err = MQTTPublish(&client, publishTopic, &message);
            if (err != 0) {
                printf("MQTTPublish fail, err : 0x%x\n", err);
            }

            printf("MQTTPublish payload:(%s)\n", sendbuf);
        }

        loop_cnt += 1;

        err = MQTTYield(&client, MQTT_TIMEOUT_MS);
        if (err != 0) {
            if (client.isconnected) {
                //断开mqtt层连接
                err = MQTTDisconnect(&client);
                if (err != 0) {
                    printf("MQTTDisconnect fail\n");
                }

                //断开tcp层连接
                network.disconnect(&network);
            }

            printf("MQTT : Reconnecting\n");

            //重新连接
            goto _reconnect;
        }

    }

    return 0;
}

static void aliyun_mqtt_example(void)
{
    if (thread_fork("mqtt_start", 10, 2 * 1024, 0, NULL, mqtt_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

static void mqtt_test(void *priv)
{
    int err;
    enum wifi_sta_connect_state state;
    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }

        os_time_dly(1000);
    }

    aliyun_mqtt_example();
}

//应用程序入口,需要运行在STA模式下
int c_main(void)
{
    if (thread_fork("mqtt_test", 10, 512, 0, NULL, mqtt_test, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }

    return 0;
}

late_initcall(c_main);

#endif//USE_MQTT_TEST
