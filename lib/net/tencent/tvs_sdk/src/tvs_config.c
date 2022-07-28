/**
* @file  tvs_config.c
* @brief TVS SDK配置接口
* @date  2019-5-10
*/

#include "tvs_config.h"
#include "tvs_log.h"
#include "tvs_core.h"
#include "tvs_preference.h"
#include "tvs_version.h"
#include "tvs_threads.h"


#define TVS_HTTP  "http://"
#define TVS_HTTPS  "https://"

#define TVS_HOST_TEST   "tvstest.html5.qq.com"

#define TVS_HOST_NORMAL   "tvs.html5.qq.com"

#define TVS_HOST_EXP   "tvsexp.html5.qq.com"

#define TVS_HOST_DEV   "tvs.sparta.html5.qq.com"

#define TVS_PATH_DOWN_CHANNEL   "/v20160207/directives"
#define TVS_PATH_PING   "/ping"
#define TVS_PATH_EVENTS   "/v20160207/events"

//#define TVS_PATH_AUTH   "/auth/o2/token"
#define TVS_PATH_AUTH   "/v20200508/auth/o2/token" //新版本支持错误返回

#define TVS_PATH_ECHO   "/echo?bn=freeRTOS1.0&id=FREERTOS_"

#define TVS_URL(s, type, env)   TVS_##s TVS_HOST_##env TVS_PATH_##type

#define TENCENT_QUA   "QV=3&PL=ADR&PR=TVS&VE=DD&VN=1.0.0.1001&PP=com.tencent.freertos.test1&NB=WEB&DE=SPEAKER"

static bool g_network_valid = false;

// 是否允许容灾机制
static bool g_enable_ip_provider = false;

#ifndef PLATFORM_RT_THREAD
// 下行通道心跳包发送间隔，默认为45秒，如果间隔太长，下行通道会频繁断开
static int g_down_channel_interval = 45;
#else
// rt-thread心跳包发送间隔必须少于30秒
static int g_down_channel_interval = 27;
#endif

TVS_LOCKER_DEFINE

// 是否允许下行通道建立
static bool g_enable_down_channel = true;

extern tvs_default_config g_tvs_system_config;
static char *g_tvs_produce_qua = NULL;

static bool g_tvs_sdk_running = true;

//static bool g_use_https = false;
static bool g_use_https = true;
static bool g_print_asr = false;

static int g_speex_compress = 5;

#define  TVS_GET_URL(tp)       \
		switch (g_tvs_system_config.def_env) { \
		case TVS_API_ENV_TEST: \
			if (g_use_https) { \
				return TVS_URL(HTTPS, tp, TEST); \
			} \
			else { \
				return TVS_URL(HTTP, tp, TEST); \
			} \
		case TVS_API_ENV_EXP: \
			if (g_use_https) { \
				return TVS_URL(HTTPS, tp, EXP); \
			} \
			else { \
				return TVS_URL(HTTP, tp, EXP); \
			} \
		case TVS_API_ENV_DEV: \
			if (g_use_https) { \
				return TVS_URL(HTTPS, tp, DEV); \
			} \
			else { \
				return TVS_URL(HTTP, tp, DEV); \
			} \
		default: \
			if (g_use_https) { \
				return TVS_URL(HTTPS, tp, NORMAL); \
			} \
			else { \
				return TVS_URL(HTTP, tp, NORMAL); \
			} \
		}

void tvs_config_set_qua(tvs_product_qua *qua)
{
    if (qua != NULL) {
        if (g_tvs_produce_qua == NULL) {
            g_tvs_produce_qua = TVS_MALLOC(255);
        }
        if (g_tvs_produce_qua != NULL) {
            memset(g_tvs_produce_qua, 0, 255);
            sprintf(g_tvs_produce_qua, "QV=3&VN=%s&PP=%s&SDK=1.0.%s.%s%s&SDKName=TVS_RTOS",
                    qua->version,
                    qua->package_name == NULL ? "com.unknown.tvs.ai" : qua->package_name,
                    TVS_BUILD_DATE,
                    TVS_BUILD_NUMBER,
                    qua->reserve == NULL ? "" : qua->reserve);
            TVS_LOG_PRINTF("init qua: %s\n", g_tvs_produce_qua);
        }
    }
}

char *tvs_config_get_qua()
{
    char *qua = g_tvs_produce_qua;
    if (qua == NULL) {
        qua = TENCENT_QUA;
    }
    //TVS_LOG_PRINTF("get qua: %s\n", qua);
    return qua;
}

void tvs_release_qua()
{

}

bool tvs_config_is_down_channel_enable()
{
    return g_enable_down_channel;
}

void tvs_config_enable_down_channel(bool enable)
{
    g_enable_down_channel = enable;
}

bool tvs_config_is_sandbox_open()
{
    return g_tvs_system_config.def_sandbox_open;
}

void tvs_config_set_sandbox_open(bool open)
{
    g_tvs_system_config.def_sandbox_open = open;
}

char *tvs_config_get_down_channel_url()
{
    TVS_GET_URL(DOWN_CHANNEL)
}

char *tvs_config_get_event_url()
{
    TVS_GET_URL(EVENTS)
}

char *tvs_config_get_auth_url()
{
    TVS_GET_URL(AUTH)
}

char *tvs_config_get_ping_url()
{
    TVS_GET_URL(PING)
}

char *tvs_config_get_echo_url()
{
    TVS_GET_URL(ECHO)
}

void tvs_config_enable_https(bool enable)
{
    g_use_https = enable;
    TVS_LOG_PRINTF("use https %d\n", enable);
}

void tvs_config_print_asr_result(bool enable)
{
    g_print_asr = enable;
}

bool tvs_config_will_print_asr_result()
{
    return g_print_asr;
}

void tvs_config_enable_ip_provider(bool enable)
{
    g_enable_ip_provider = enable;
}

bool tvs_config_ip_provider_enabled()
{
    // 返回false代表不执行容灾机制
    return g_enable_ip_provider;
}

int tvs_config_get_current_env()
{
    return g_tvs_system_config.def_env;
}

int tvs_config_get_current_asr_bitrate()
{
    return g_tvs_system_config.recorder_bitrate;
}

void tvs_config_set_current_asr_bitrate(int bitrate)
{
    g_tvs_system_config.recorder_bitrate = bitrate;
}

void tvs_config_set_current_env(int env)
{
    g_tvs_system_config.def_env = env;
}

char *tvs_config_get_current_host()
{
    switch (g_tvs_system_config.def_env) {
    case TVS_API_ENV_TEST:
        return TVS_HOST_TEST;
    case TVS_API_ENV_EXP:
        return TVS_HOST_EXP;
    case TVS_API_ENV_DEV:
        return TVS_HOST_DEV;
    default:
        return TVS_HOST_NORMAL;
    }
}

bool tvs_config_is_network_valid()
{
    do_lock();
    bool valid = g_network_valid;
    do_unlock();

    return valid;
}

void tvs_config_set_network_valid(bool valid)
{
    do_lock();
    g_network_valid = valid;
    do_unlock();
}


void tvs_config_init()
{

    TVS_LOCKER_INIT

}

void tvs_config_set_sdk_running(bool running)
{
    do_lock();

    g_tvs_sdk_running = true;

    do_unlock();
}

bool tvs_config_is_sdk_running()
{
    do_lock();
    bool running = g_tvs_sdk_running;
    do_unlock();

    return running;
}

int tvs_config_get_recorder_bitrate()
{
    return g_tvs_system_config.recorder_bitrate;
}

int tvs_config_get_recorder_channels()
{
    return g_tvs_system_config.recorder_channels;
}

void tvs_config_set_hearttick_interval(int interval_sec)
{
    g_down_channel_interval = interval_sec;
}

int tvs_config_get_hearttick_interval()
{
    return g_down_channel_interval;
}

int tvs_config_get_speex_compress()
{
    return g_speex_compress;
}

void tvs_config_set_speex_compress(int compress)
{
    g_speex_compress = compress;
}


