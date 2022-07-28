
#define TVS_LOG_DEBUG_MODULE  "IPPROVIDER"
#include "tvs_log.h"

#include "tvs_http_client.h"
#include "tvs_http_manager.h"
#include "tvs_config.h"
#include "tvs_executor_service.h"
#include "tvs_jsons.h"
#include "tvs_preference.h"
#include "tvs_iplist.h"
#include "tvs_echo.h"
#include "tvs_dns.h"
#include "tvs_threads.h"
#include "mongoose.h"
#include "tvs_iplist_preset.h"
#include "tvs_down_channel.h"
#include "tvs_ip_provider.h"


typedef int (*tvs_func_get_next_ip)(int index);
typedef bool (*tvs_func_remove_ip)(int ip_addr);
typedef bool (*tvs_func_on_start)();
typedef bool (*tvs_func_on_stop)();
typedef int (*tvs_func_get_count)();
typedef int (*tvs_func_do_init)();
typedef void (*tvs_func_on_network_changed)(bool connected);

extern u8 net_connected;

typedef struct {
    const char *name;
    tvs_func_do_init do_init;
    tvs_func_on_start on_start;
    tvs_func_on_stop on_stop;
    tvs_func_get_next_ip get_next_ip;
    tvs_func_remove_ip remove_ip;
    tvs_func_get_count get_count;
    tvs_func_on_network_changed on_network_changed;
    bool need_echo;
    bool dns_ip;
} tvs_ip_provider_module;

#define MAX_MODULE_COUNT    5

static int g_valid_ip = 0;

static tvs_ip_provider_module g_ip_provider_modules[MAX_MODULE_COUNT];

static tvs_thread_handle_t *g_ip_provider_thread = NULL;

static bool g_is_dns_ip = false;

static bool g_running = false;

TVS_LOCKER_DEFINE

static void tvs_ip_provider_on_ip_invalid_func(tvs_thread_handle_t *thread);

int tvs_ip_provider_init()
{
    TVS_LOCKER_INIT

    if (!tvs_config_ip_provider_enabled()) {
        return 0;
    }

    if (g_ip_provider_thread == NULL) {
        g_ip_provider_thread = tvs_thread_new(tvs_ip_provider_on_ip_invalid_func, NULL);
    }

    memset(&g_ip_provider_modules, 0, sizeof(g_ip_provider_modules));
    int index = 0;
    g_ip_provider_modules[index].do_init = tvs_dns_init;
    g_ip_provider_modules[index].on_start = tvs_http_dns_check_start;
    g_ip_provider_modules[index].get_count = tvs_http_dns_get_count;
    g_ip_provider_modules[index].get_next_ip = tvs_http_dns_get_next_ip;
    g_ip_provider_modules[index].name = "HTTPDNS";
    g_ip_provider_modules[index].need_echo = true;
    g_ip_provider_modules[index].remove_ip = tvs_http_dns_remove_ip;
    g_ip_provider_modules[index].on_network_changed = tvs_dns_on_network_changed;
    g_ip_provider_modules[index].dns_ip = true;
    index++;
    g_ip_provider_modules[index].do_init = NULL; // 在tvs_dns_init中已经初始化
    g_ip_provider_modules[index].on_start = tvs_system_dns_check_start;
    g_ip_provider_modules[index].get_count = tvs_system_dns_get_count;
    g_ip_provider_modules[index].get_next_ip = tvs_system_dns_get_next_ip;
    g_ip_provider_modules[index].name = "SYSDNS";
    g_ip_provider_modules[index].need_echo = true;
    g_ip_provider_modules[index].remove_ip = tvs_system_dns_remove_ip;
    g_ip_provider_modules[index].on_network_changed = NULL; // 在tvs_dns_on_network_changed中处理
    g_ip_provider_modules[index].dns_ip = true;

    // 暂不提供在线IPLIST功能
#if 0
    index++;
    g_ip_provider_modules[index].do_init = tvs_iplist_init;
    g_ip_provider_modules[index].on_start = tvs_iplist_check_start;
    g_ip_provider_modules[index].get_count = tvs_iplist_get_count;
    g_ip_provider_modules[index].get_next_ip = tvs_iplist_get_next_ip;
    g_ip_provider_modules[index].name = "IPLIST";
    g_ip_provider_modules[index].need_echo = true;
    g_ip_provider_modules[index].remove_ip = tvs_iplist_remove_ip;
    g_ip_provider_modules[index].on_network_changed = tvs_iplist_on_network_changed;
    g_ip_provider_modules[index].dns_ip = false;
#endif

    // 预置IP-LIST
    index++;
    g_ip_provider_modules[index].do_init = tvs_iplist_preset_init;
    g_ip_provider_modules[index].on_start = tvs_iplist_preset_check_start;
    g_ip_provider_modules[index].get_count = tvs_iplist_preset_get_count;
    g_ip_provider_modules[index].get_next_ip = tvs_iplist_preset_get_next_ip;
    g_ip_provider_modules[index].name = "PRESET";
    g_ip_provider_modules[index].need_echo = true;
    g_ip_provider_modules[index].remove_ip = tvs_iplist_preset_remove_ip;
    g_ip_provider_modules[index].on_network_changed = tvs_iplist_preset_on_network_changed;
    g_ip_provider_modules[index].dns_ip = false;

    index++;

    for (int i = 0; i < index; i++) {
        if (g_ip_provider_modules[i].do_init != NULL) {
            g_ip_provider_modules[i].do_init();
        }
    }

    return 0;
}

bool tvs_ip_provider_is_valid(int ipaddr)
{
    if (ipaddr <= 0) {
        return false;
    }
    // do echo
    for (int i = 0; i < 3; i++) {
        if (tvs_echo_start(ipaddr)) {
            return true;
        }
    }
    return false;;
}

void tvs_ip_provider_set_valid_ip_ex(const char *ip_str, bool dnsIp)
{
    int ip = tvs_ip_provider_convert_ip_str(ip_str);
    tvs_ip_provider_set_valid_ip(ip, dnsIp);
}

void tvs_ip_provider_set_valid_ip(int ip, bool dnsIp)
{
    if (ip == 0) {
        return;
    }
    TVS_LOG_PRINTF("set valid ip %d - %s\n", ip, inet_ntoa(*(struct in_addr *)&ip));
    bool changed = false;
    do_lock();
    changed = (g_valid_ip != 0 && g_valid_ip != ip);
    g_valid_ip = ip;
    g_is_dns_ip = dnsIp;
    do_unlock();

    if (changed) {
        // IP改变，需要重连下行通道

        tvs_down_channel_break();

        // TO-DO 如果切到了非dns ip, 需要设置定时器，一段时间后校验dns ip是否有效并尝试切回dns ip

    }
}

void tvs_ip_provider_get_first_ip()
{
    if (!tvs_config_ip_provider_enabled()) {
        return;
    }

    tvs_dns_get_first_ip();
}

int tvs_ip_provider_convert_ip_str(const char *ip_str)
{
    if (NULL == ip_str) {
        return 0;
    }

    unsigned int a, b, c, d = 0;
    if (sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {

        int ip = tvs_htonl(((uint32_t) a << 24) | ((uint32_t) b << 16) | c << 8 | d);

        return ip;
    }

    return 0;
}


int tvs_ip_provider_get_valid_ip()
{
    if (!tvs_config_ip_provider_enabled()) {
        return 0;
    }

    do_lock();
    int ip = g_valid_ip;
    do_unlock();

    return ip;
}

void tvs_ip_provider_remove_ip(int ip)
{
    if (ip == 0) {
        return;
    }
    TVS_LOG_PRINTF("remove invalid ip %d - %s\n", ip, inet_ntoa(*(struct in_addr *)&ip));
    tvs_ip_provider_module *child = NULL;

    for (int i = 0; i < MAX_MODULE_COUNT; i++) {
        child = &g_ip_provider_modules[i];
        if (child == NULL
            || child->remove_ip == NULL) {
            continue;
        }

        child->remove_ip(ip);
    }
}

void tvs_ip_provider_start_all(int ip)
{

    tvs_ip_provider_module *child = NULL;

    for (int i = 0; i < MAX_MODULE_COUNT; i++) {
        child = &g_ip_provider_modules[i];
        if (child == NULL
            || child->on_start == NULL) {
            continue;
        }

        child->on_start();
    }
}

void tvs_ip_provider_stop_all(int ip)
{

    tvs_ip_provider_module *child = NULL;

    for (int i = 0; i < MAX_MODULE_COUNT; i++) {
        child = &g_ip_provider_modules[i];
        if (child == NULL
            || child->on_stop == NULL) {
            continue;
        }

        child->on_stop();
    }
}

// return found valid ip or not
static bool look_for_ip(tvs_ip_provider_module *child)
{
    TVS_LOG_PRINTF("loop for module %s\n", child->name);
    int size = child->get_count();
    int ip = 0;

    for (int i = 0; i < size; i++) {
        ip = child->get_next_ip(i);
        if (tvs_ip_provider_is_valid(ip)) {
            // get valid ip;
            tvs_ip_provider_set_valid_ip(ip, child->dns_ip);
            return true;
        } else {
            tvs_ip_provider_remove_ip(ip);
        }
    }

    return false;

}

bool tvs_ip_provider_is_running()
{
    do_lock();

    bool ret = g_running;

    do_unlock();

    return ret;
}

void tvs_ip_provider_set_running(bool running)
{
    do_lock();
    g_running = running;

    do_unlock();
}

int tvs_ping_host(const char *host, int times);

bool tvs_ping_all()
{
    int ping_count = tvs_ping_host("www.sina.com.cn", 0) + tvs_ping_host("www.qq.com", 0) + tvs_ping_host("www.baidu.com", 0);

    return ping_count > 0;
}

bool tvs_ping_one()
{
    int ping_count = 0;

    ping_count = tvs_ping_host("www.sina.com.cn", 0);
    if (ping_count > 0) {
        return true;
    }

    ping_count = tvs_ping_host("www.qq.com", 0);
    if (ping_count > 0) {
        return true;
    }

    ping_count = tvs_ping_host("www.baidu.com", 0);
    if (ping_count > 0) {
        return true;
    }

    return false;
}


static int tvs_ip_provider_on_ip_invalid_inner(int ipaddr)
{
    TVS_LOG_PRINTF("check ip %d - %s\n", ipaddr, inet_ntoa(*(struct in_addr *)&ipaddr));
    if (ipaddr == 0) {
        return -1;
    }

    // 首先检查网络通不通
    bool network_valid = tvs_ping_all();

    if (!network_valid) {
        // 网络不通，不执行查找备用IP的操作
        TVS_LOG_PRINTF("network is unreachable\n");
        net_connected = 0;
        return -1;
    }
    if (tvs_ip_provider_is_valid(ipaddr)) {
        TVS_LOG_PRINTF("ipaddr %d is valid\n", ipaddr);
        return 0;
    }
    tvs_ip_provider_start_all(ipaddr);
    tvs_ip_provider_remove_ip(ipaddr);
    tvs_ip_provider_module *child = NULL;

    for (int i = 0; i < MAX_MODULE_COUNT; i++) {
        child = &g_ip_provider_modules[i];
        if (child == NULL
            || child->get_count == NULL
            || child->get_next_ip == NULL
            || child->remove_ip == NULL) {
            TVS_LOG_PRINTF("invalid ip provider module\n");
            continue;
        }

        if (!tvs_config_is_network_valid()) {
            break;
        }

        if (look_for_ip(child)) {
            break;
        }
    }

    tvs_ip_provider_stop_all(ipaddr);
    return 0;
}


static void tvs_ip_provider_on_ip_invalid_inner1(int ipaddr)
{
    if (tvs_ip_provider_is_running()) {
        TVS_LOG_PRINTF("ip provider is running\n");
        return;
    }

    tvs_ip_provider_set_running(true);

    tvs_ip_provider_on_ip_invalid_inner(ipaddr);

    tvs_ip_provider_set_running(false);
}

static void tvs_ip_provider_on_ip_invalid_func(tvs_thread_handle_t *thread)
{
    int *ip = (int *)tvs_thread_get_param(thread);
    if (ip != NULL) {
        tvs_ip_provider_on_ip_invalid_inner1(*ip);
    }
}

int tvs_ip_provider_on_ip_invalid(int ipaddr)
{
    if (!tvs_config_ip_provider_enabled()) {
        return 0;
    }
    if (!tvs_thread_is_stop(g_ip_provider_thread)) {
        return 0;
    }
    tvs_thread_start_prepare(g_ip_provider_thread, &ipaddr, sizeof(int));

    tvs_thread_start_now(g_ip_provider_thread, "ipprovider", 2, 1024);

    return 0;
}

void tvs_ip_provider_on_network_changed(bool connected)
{
    if (!tvs_config_ip_provider_enabled()) {
        return;
    }

    for (int i = 0; i < MAX_MODULE_COUNT; i++) {
        if (g_ip_provider_modules[i].on_network_changed != NULL) {
            g_ip_provider_modules[i].on_network_changed(connected);
        }
    }
}

