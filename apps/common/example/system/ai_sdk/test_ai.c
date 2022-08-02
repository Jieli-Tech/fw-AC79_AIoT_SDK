#include "app_config.h"

#ifdef USE_AI_SDK_DEMO

#include "system/includes.h"
#include "server/server_core.h"
#include "ai_test_server.h"

const struct ai_test_sdk_api test_ai1_sdk_api;
const struct ai_test_sdk_api test_ai2_sdk_api;


static int ai1_sdk_open(void)
{
    //不能长时间堵塞
    printf(" ------%s---------%d--\n", __func__, __LINE__);

    return 0;
}

static int ai1_sdk_check(void)
{
    printf(" ------%s---------%d-----%s\n", __func__, __LINE__, __TIME__);
    if (1) {
        return AI_STAT_CONNECTED;
    } else {
        return AI_STAT_DISCONNECTED;
    }
}

static int ai1_sdk_do_event(int event, int arg)
{
    //此处建议消息传递给AI任务，不能长时间堵塞
    printf("%s: %d , arg = %d\n", __func__, event, arg);

    return 0;
}

static int ai1_sdk_disconnect(void)
{
    //不能长时间堵塞
    printf(" ------%s---------%d-----%s\n", __func__, __LINE__, __TIME__);
    return 0;
}

REGISTER_AI_SDK(test_ai1_sdk_api) = {
    .name           = "ai1",
    .connect        = ai1_sdk_open,
    .state_check    = ai1_sdk_check,
    .do_event       = ai1_sdk_do_event,
    .disconnect     = ai1_sdk_disconnect,
};


static int ai2_sdk_open(void)
{
    //不能长时间堵塞
    printf(" ------%s---------%d-----%s\n", __func__, __LINE__, __TIME__);

    return 0;
}

static int ai2_sdk_check(void)
{
    printf(" ------%s---------%d-----%s\n", __func__, __LINE__, __TIME__);
    if (1) {
        return AI_STAT_CONNECTED;
    } else {
        return AI_STAT_DISCONNECTED;
    }
}

static int ai2_sdk_do_event(int event, int arg)
{
    printf("%s: %d , arg = %d\n", __func__, event, arg);

    return 0;
}

static int ai2_sdk_disconnect(void)
{
    //不能长时间堵塞
    printf(" ------%s---------%d-----%s\n", __func__, __LINE__, __TIME__);
    return 0;
}

REGISTER_AI_SDK(test_ai2_sdk_api) = {
    .name           = "ai2",
    .connect        = ai2_sdk_open,
    .state_check    = ai2_sdk_check,
    .do_event       = ai2_sdk_do_event,
    .disconnect     = ai2_sdk_disconnect,
};

#endif //USE_AI_SDK_DEMO
