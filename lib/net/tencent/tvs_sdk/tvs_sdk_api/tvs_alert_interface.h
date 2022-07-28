#ifndef __TVS_ALERT_INTERFACE_H__
#define __TVS_ALERT_INTERFACE_H__

#include "cJSON_common/cJSON.h"

int tvs_alert_new(cJSON *alert_detail);

int tvs_alert_delete(char *alert_token);

cJSON *tvs_alert_read_all();

#endif
