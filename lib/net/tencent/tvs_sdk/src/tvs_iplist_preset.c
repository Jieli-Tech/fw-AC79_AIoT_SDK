#define TVS_LOG_DEBUG_MODULE  "IPPRESET"
#include "tvs_log.h"

#include "tvs_config.h"
#include "tvs_http_client.h"

#include "tvs_threads.h"

#include "tvs_ip_provider.h"


static const char *const g_preset_tvs[] = {"183.3.225.67",
                                           "58.250.136.67",
                                           "183.3.235.88",
                                           "58.251.81.44",
                                           "123.151.190.159",
                                           "111.30.159.168",
                                           "220.194.95.159",
                                           "101.226.233.190",
                                           "183.192.202.190",
                                           "58.247.206.185"
                                          };

static const char *const g_preset_tvs_test[] = {"183.3.225.67",
                                                "58.250.136.67",
                                                "58.247.214.111"
                                               };


static int *g_preset_ip_list = NULL;
static int g_preset_ip_count = 0;

TVS_LOCKER_DEFINE

static void copy_all_ip()
{
    int ip = 0;
    int acture_count = 0;

    tvs_api_env env = tvs_config_get_current_env();
    const char *const *preset_iplist = NULL;
    switch (env) {
    case TVS_API_ENV_TEST:
        preset_iplist = g_preset_tvs_test;
        acture_count = sizeof(g_preset_tvs_test) / sizeof(char *);
        break;
    case TVS_API_ENV_NORMAL:
        preset_iplist = g_preset_tvs;
        acture_count = sizeof(g_preset_tvs) / sizeof(char *);
        break;
    default:
        break;
    }

    if (g_preset_ip_list != NULL && acture_count != g_preset_ip_count) {
        TVS_FREE(g_preset_ip_list);
        g_preset_ip_list = NULL;
    }

    if (g_preset_ip_list == NULL && acture_count > 0) {
        g_preset_ip_list = TVS_MALLOC(sizeof(int) * acture_count);

        g_preset_ip_count = acture_count;
    }

    if (g_preset_ip_list != NULL && g_preset_ip_count > 0) {
        memset(g_preset_ip_list, 0, sizeof(int) * acture_count);
    }

    for (int i = 0; i < g_preset_ip_count; i++) {
        ip = tvs_ip_provider_convert_ip_str(preset_iplist[i]);
        g_preset_ip_list[i] = ip;
        TVS_LOG_PRINTF("get preset ip %d - %s\n", ip, preset_iplist[i]);
    }
}

void refresh_ip_list()
{
    bool has_ip = false;
    for (int i = 0; i < g_preset_ip_count; i++) {
        if (g_preset_ip_list[i] != 0) {
            has_ip = true;
            break;
        }
    }

    // 如果IPLIST中的IP全部无效了，需要全部重新刷回来
    if (!has_ip) {
        copy_all_ip();
    }
}


int tvs_iplist_preset_get_next_ip(int index)
{
    if (g_preset_ip_list == NULL) {
        return 0;
    }

    if (index >= g_preset_ip_count) {
        return 0;
    }

    return g_preset_ip_list[index];
}

bool tvs_iplist_preset_remove_ip(int ip_addr)
{
    for (int i = 0; i < g_preset_ip_count; i++) {
        if (g_preset_ip_list[i] == ip_addr) {
            g_preset_ip_list[i] = 0;
        }
    }
    return true;
}

bool tvs_iplist_preset_check_start()
{
    refresh_ip_list();

    return true;
}

int tvs_iplist_preset_get_count()
{
    return g_preset_ip_count;
}

int tvs_iplist_preset_init()
{
    TVS_LOCKER_INIT

    copy_all_ip();
    return 0;
}

/*static void* g_tvs_iplist_preset_timer = NULL;
#define DO_REFRESH_TIMER     40  // 更新IP-LIST的时间间隔

void tvs_iplist_preset_timer_func(void* param) {
	refresh_ip_list();
}

void tvs_iplist_preset_refresh_start() {
	os_wrapper_start_timer(&g_tvs_iplist_preset_timer, tvs_iplist_preset_timer_func, DO_REFRESH_TIMER * 60 * 1000, true);
}*/

void tvs_iplist_preset_on_network_changed(bool connected)
{
    if (connected) {
        //tvs_iplist_preset_refresh_start();
    }
}

