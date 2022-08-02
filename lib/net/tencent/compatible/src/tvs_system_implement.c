
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_system_interface.h"

#include "tvs_core.h"
#include "tvs_api/tvs_api.h"
#include "tvs_preference.h"
#include "tvs_preference_interface.h"
#include "tvs_config.h"
#include "tvs_log.h"


tvs_platform_adaptor g_tvs_platform_adaptor = {0};

const char *g_mode_str[] = {"NORMAL",
                            "BAOLA",
                            "CHILD"
                           };

tvs_mode tvs_get_current_mode_type(const char *mode_str)
{
    int size = sizeof(g_mode_str) / sizeof(char *);

    for (int i = 0; i < size; i++) {
        if (strcasecmp(g_mode_str[i], mode_str) == 0) {
            return i;
        }
    }

    return TVS_MODE_NORMAL;
}
char *tvs_system_get_current_mode()
{
    int mode = 0;
    tvs_preference_get_number_value(TVS_PREFERENCE_CURRENT_MODE, &mode, TVS_MODE_NORMAL);
    if (mode <= 0 || mode >= TVS_MODE_COUNT) {
        mode = TVS_MODE_NORMAL;
    }
    return (char *)g_mode_str[mode];
}

void tvs_system_set_current_mode(char *mode)
{
    int i_mode = (int)tvs_get_current_mode_type(mode);

    tvs_preference_set_number_value(TVS_PREFERENCE_CURRENT_MODE, i_mode);
}

const char *tvs_preference_load_data(int *preference_size)
{
    if (g_tvs_platform_adaptor.load_preference != NULL) {
        return g_tvs_platform_adaptor.load_preference(preference_size);
    }

    return NULL;
}

int tvs_preference_save_data(const char *preference_buffer, int preference_size)
{
    if (g_tvs_platform_adaptor.save_preference != NULL) {
        return g_tvs_platform_adaptor.save_preference(preference_buffer, preference_size);
    }

    return -1;
}

extern int tvs_tools_get_current_volume();

int tvs_platform_adapter_init(tvs_platform_adaptor *adapter)
{
    if (adapter != NULL) {
        memcpy(&g_tvs_platform_adaptor, adapter, sizeof(tvs_platform_adaptor));
    }

    int value = 0;

    int volume = tvs_tools_get_current_volume();
    if (g_tvs_platform_adaptor.set_volume != NULL) {
        TVS_LOG_PRINTF("tvs_cloud_volume init to %d\n", volume);
        g_tvs_platform_adaptor.set_volume(volume, 100, true);
    }

    tvs_api_env def_env = tvs_config_get_current_env();

    tvs_preference_get_number_value(TVS_PREF_ENV_TYPE, &value, def_env);

    tvs_config_set_current_env(value);

    int def_sanbox = tvs_config_is_sandbox_open() ? 1 : 0;

    tvs_preference_get_number_value(TVS_PREFERENCE_SANDBOX, &value, def_sanbox);

    tvs_config_set_sandbox_open(value);

    int def_bitrate = tvs_config_get_current_asr_bitrate();

    tvs_preference_get_number_value(TVS_PREF_ARS_BITRATE, &value, def_bitrate);

    tvs_config_set_current_asr_bitrate(value);

    TVS_LOG_PRINTF("default env %d, sandbox %d, bitrate %d, actrual env %d, sandbox %d, bitrate %d\n",
                   def_env, def_sanbox, def_bitrate,
                   tvs_config_get_current_env(),
                   tvs_config_is_sandbox_open(),
                   tvs_config_get_current_asr_bitrate());

    return 0;
}

void tvs_platform_adapter_on_volume_changed(int cloud_volume)
{
    TVS_LOG_PRINTF("tvs_cloud_volume changed - %d\n", cloud_volume);

    tvs_core_notify_volume_changed(cloud_volume);
}

void tvs_platform_adapter_on_network_state_changed(bool connect)
{
    tvs_core_notify_net_state_changed(connect);
}
