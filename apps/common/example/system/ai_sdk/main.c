#include "app_config.h"

#ifdef USE_AI_SDK_DEMO

#include "ai_test_server.h"
#include "system/includes.h"
#include "server/server_core.h"

extern const struct ai_test_sdk_api test_ai1_sdk_api;
extern const struct ai_test_sdk_api test_ai2_sdk_api;

struct server *test_server_hdl = NULL;

static int test_arg[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a};

//AI_SDK下发消息处理函数
static void ai_test_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {

    case AI_SERVER_EVENT_URL:
    case AI_SERVER_EVENT_URL_TTS:
    case AI_SERVER_EVENT_URL_MEDIA:
        printf(" run : priv=%s, argc=0x%x, argv[0] = %d , argv[1]=%s\n", priv, argc, argv[0], argv[1]);
        free((char *)argv[1]);
        break;
    case AI_SERVER_EVENT_DISCONNECTED:
    case AI_SERVER_EVENT_CONTINUE:
    case AI_SERVER_EVENT_PAUSE:
    case AI_SERVER_EVENT_STOP:
    case AI_SERVER_EVENT_UPGRADE:
    case AI_SERVER_EVENT_UPGRADE_SUCC:
    case AI_SERVER_EVENT_UPGRADE_FAIL:
    case AI_SERVER_EVENT_MIC_OPEN:
    case AI_SERVER_EVENT_MIC_CLOSE:
    case AI_SERVER_EVENT_VOLUME_CHANGE:
    case AI_SERVER_EVENT_SET_PLAY_TIME:
    case AI_SERVER_EVENT_SEEK:
    case AI_SERVER_EVENT_PLAY_BEEP:
    case AI_SERVER_EVENT_RESUME_PLAY:
    case AI_SERVER_EVENT_PREV_PLAY:
    case AI_SERVER_EVENT_NEXT_PLAY:
//            int *test_arg = (int *)argv[1];
        printf(" run : priv=%s, argc=0x%x, argv[0] = %d , argv[1]=%d  argv[2] = %s\n", priv, argc, argv[0], *(int *)(argv[1]), argv[2]);
        break;
    case AI_SERVER_EVENT_CONNECTED:
        printf(" run : priv=%s, argc=0x%x, argv[0] = %d , argv[1]=%s\n", priv, argc, argv[0], argv[1]);
        if (argc == 3) {
            free((char *)argv[1]);
        }
        break;
    default:
        break;
    }
}

static void ai_sdk_upload_task(void)
{
    union ai_test_req req = {0};

    //通过ai_server上报消息
    req.evt.event   = AI_EVENT_SPEAK_END;
    req.evt.arg     = 0x20;
    req.evt.ai_name = "ai1";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_MEDIA_END;
    req.evt.arg     = 0x21;
    req.evt.ai_name = "ai2";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_PLAY_PAUSE;
    req.evt.arg     = 0x22;
    req.evt.ai_name = "ai1";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_PREVIOUS_SONG;
    req.evt.arg     = 0x23;
    req.evt.ai_name = "ai1";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_NEXT_SONG;
    req.evt.arg     = 0x24;
    req.evt.ai_name = "ai1";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_VOLUME_CHANGE;
    req.evt.arg     = 0x25;
    req.evt.ai_name = "ai2";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_RECORD_START;
    req.evt.arg     = 0x26;
    req.evt.ai_name = "ai2";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_RECORD_BREAK;
    req.evt.arg     = 0x27;
    req.evt.ai_name = "ai2";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_RECORD_STOP;
    req.evt.arg     = 0x28;
    req.evt.ai_name = "ai2";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_PLAY_TIME;
    req.evt.arg     = 0x29;
    req.evt.ai_name = "ai2";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_MEDIA_STOP;
    req.evt.arg     = 0x2a;
    req.evt.ai_name = "ai2";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);

    req.evt.event   = AI_EVENT_MEDIA_START;
    req.evt.arg     = 0x2b;
    req.evt.ai_name = "ai2";
    ai_server_request(test_server_hdl, AI_REQ_EVENT, &req);
}

static void ai_sdk_down_task(void)
{
    ai_test_server_event_url(&test_ai1_sdk_api, "123", AI_SERVER_EVENT_URL);

    ai_test_server_event_url(&test_ai2_sdk_api, "456", AI_SERVER_EVENT_URL_TTS);

    ai_test_server_event_url(&test_ai1_sdk_api, "789", AI_SERVER_EVENT_URL_MEDIA);

    const char *char_arg = "123456789";
    ai_test_server_event_url(&test_ai2_sdk_api, char_arg, AI_SERVER_EVENT_CONNECTED);


    ai_test_server_event_notify(&test_ai1_sdk_api, &test_arg[0], AI_SERVER_EVENT_DISCONNECTED);

    ai_test_server_event_notify(&test_ai2_sdk_api, &test_arg[1], AI_SERVER_EVENT_CONTINUE);

    ai_test_server_event_notify(&test_ai2_sdk_api, &test_arg[2], AI_SERVER_EVENT_PAUSE);

    ai_test_server_event_notify(&test_ai1_sdk_api, &test_arg[3], AI_SERVER_EVENT_STOP);

    ai_test_server_event_notify(&test_ai2_sdk_api, &test_arg[4], AI_SERVER_EVENT_UPGRADE);

    ai_test_server_event_notify(&test_ai1_sdk_api, &test_arg[5], AI_SERVER_EVENT_UPGRADE_SUCC);

    ai_test_server_event_notify(&test_ai2_sdk_api, &test_arg[6], AI_SERVER_EVENT_UPGRADE_FAIL);

    ai_test_server_event_notify(&test_ai2_sdk_api, &test_arg[7], AI_SERVER_EVENT_MIC_OPEN);

    ai_test_server_event_notify(&test_ai1_sdk_api, &test_arg[8], AI_SERVER_EVENT_MIC_CLOSE);

    ai_test_server_event_notify(&test_ai2_sdk_api, &test_arg[9], AI_SERVER_EVENT_VOLUME_CHANGE);

    ai_test_server_event_notify(&test_ai2_sdk_api, &test_arg[0], AI_SERVER_EVENT_SET_PLAY_TIME);

    ai_test_server_event_notify(&test_ai1_sdk_api, &test_arg[0], AI_SERVER_EVENT_SEEK);

    ai_test_server_event_notify(&test_ai2_sdk_api, &test_arg[0], AI_SERVER_EVENT_RESUME_PLAY);

    ai_test_server_event_notify(&test_ai1_sdk_api, &test_arg[0], AI_SERVER_EVENT_PREV_PLAY);

    ai_test_server_event_notify(&test_ai1_sdk_api, &test_arg[0], AI_SERVER_EVENT_NEXT_PLAY);
}

static void ai_sdk_example_task(void *p)
{
    union ai_test_req req = {0};
    int test_cnt = 1;
    int ret;
    int msg[32];

    //打开服务
    test_server_hdl = server_open("ai_test_server", NULL);
    if (!test_server_hdl) {
        puts("server_test_task server_open faild!!!\n");
        return;
    }
    //注册服务事件通知回调
    server_register_event_handler(test_server_hdl, "test_priv", ai_test_server_event_handler);

    //AI_SDK连接请求
    ai_server_request(test_server_hdl, AI_REQ_CONNECT, &req);

    thread_fork("ai_sdk_upload_task", 13, 1000, 128, NULL, ai_sdk_upload_task, NULL);

    thread_fork("ai_sdk_down_task", 13, 1500, 128, NULL, ai_sdk_down_task, NULL);
    while (1) {
        //__os_taskq_pend 是为了让 ai_test_server_event_handler 能够在里面运行
        ret = __os_taskq_pend(msg, ARRAY_SIZE(msg), 200);   //200个tick超时返回 OS_Q_EMPTY
        if (!test_cnt--) {
            break;
        }

    }

//     关闭服务,释放资源
    server_close(test_server_hdl);
}

static int c_main(void)
{
    printf("\r\n\r\n\r\n\r\n\r\n -----------ai_sdk_example run %s -------------\r\n\r\n\r\n\r\n\r\n", __TIME__);

    thread_fork("ai_sdk_example_task", 13, 1000, 128, NULL, ai_sdk_example_task, NULL);

    return 0;
}
late_initcall(c_main);

#endif //USE_AI_SDK_DEMO
