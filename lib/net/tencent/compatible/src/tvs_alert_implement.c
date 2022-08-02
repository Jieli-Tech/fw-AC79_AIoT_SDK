#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_alert_interface.h"

#include "tvs_core.h"
#include "tvs_api/tvs_api.h"
#include "os_wrapper.h"

tvs_alert_adapter g_tvs_alert_adapter;

int tvs_alert_new(cJSON *alert_detail)
{
    int ret = -1;
    if (g_tvs_alert_adapter.do_new_alert != NULL) {
        if (alert_detail != NULL) {
            char *alert = cJSON_PrintUnformatted(alert_detail);

            if (alert != NULL) {
                tvs_alert_infos infos = {0};
                infos.alert_detail = alert;
                ret = g_tvs_alert_adapter.do_new_alert(&infos, 1);

                TVS_FREE(alert);
            }
        }

    }
    return ret;
}

int tvs_alert_delete(char *alert_token)
{
    if (alert_token == NULL) {
        return -1;
    }

    int ret = -1;
    if (g_tvs_alert_adapter.do_delete != NULL) {
        tvs_alert_summary sum;
        memset(&sum, 0, sizeof(tvs_alert_summary));
        strcpy(sum.alert_token, alert_token);
        ret = g_tvs_alert_adapter.do_delete(&sum, 1);
    }

    return ret;
}

cJSON *tvs_alert_read_all()
{
    if (g_tvs_alert_adapter.get_alerts_ex != NULL) {
        return g_tvs_alert_adapter.get_alerts_ex();
    }

    return NULL;
}

int tvs_alert_adapter_init(tvs_alert_adapter *adapter)
{
    if (adapter != NULL) {
        memcpy(&g_tvs_alert_adapter, adapter, sizeof(tvs_alert_adapter));
    }

    return 0;
}

int tvs_alert_adapter_on_trigger(const char *alert_token)
{
    tvs_core_notify_alert_start_ring(alert_token);
    return 0;
}

int tvs_alert_adapter_on_trigger_stop(const char *alert_token, tvs_alert_stop_reason reason)
{
    tvs_core_notify_alert_stop_ring(alert_token);
    return 0;
}


