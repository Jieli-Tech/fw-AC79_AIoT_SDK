#include <string.h>
#include <stdlib.h>
#include "printf.h"
#include <time.h>
#include "turing.h"
#include "turing_alarm.h"
#include "os/os_api.h"
#include "system/timer.h"

typedef struct __turing_alarm_node {
    void *priv;
    u16 id;
    u8  set;
    u8  cyc;
} turing_alarm_node;

static turing_alarm_node turing_alarm[MAX_ALARM_NUM];
static OS_MUTEX alarm_mutex;

static void turing_alarm_callback(void *param)
{
    turing_alarm_node *node = (turing_alarm_node *)param;
    void *priv = NULL;

    priv = node->priv;
    if (!node->cyc) {
        os_mutex_pend(&alarm_mutex, 0);
        node->id = 0;
        node->priv = NULL;
        node->set = 0;
        node->cyc = 0;
        os_mutex_post(&alarm_mutex);
    }

    if (!turing_app_get_connect_status()) {
        return;
    }
}

int turing_alarm_set(const char *p_date, const char *p_time, const char *p_cyc, const char *second, void *priv)
{
    int ret = -1;

    if (!p_date || !p_time || !p_cyc) {
        return ret;
    }

    struct tm tmSet;
    time_t tSet;
    time_t tNow = time(NULL) + 28800;
    const char *buf = p_date;
    u32 timeout = 0;
    memset(&tmSet, 0, sizeof(struct tm));

    if (second && strlen(second) > 0) {
        timeout = atoi(second);
    } else {
        tmSet.tm_year = atoi(buf);

        buf = strstr(buf, "-");
        if (!buf) {
            goto __exit;
        }
        tmSet.tm_mon = atoi(++buf);

        buf = strstr(buf, "-");
        if (!buf) {
            goto __exit;
        }
        tmSet.tm_mday = atoi(++buf);

        if ((tmSet.tm_year > 2036) || (tmSet.tm_year < 2019)) {
            goto __exit;
        }
        tmSet.tm_year -= 1900;

        if ((tmSet.tm_mon == 0) || (tmSet.tm_mon > 12)) {
            goto __exit;
        }
        tmSet.tm_mon -= 1;

        if ((tmSet.tm_mday == 0) || (tmSet.tm_mday > 31)) {
            goto __exit;
        }

        // time
        buf = p_time;
        tmSet.tm_hour = atoi(buf);

        buf = strstr(buf, ":");
        if (!buf) {
            goto __exit;
        }
        tmSet.tm_min = atoi(++buf);

        buf = strstr(buf, ":");
        if (!buf) {
            goto __exit;
        }
        tmSet.tm_sec = atoi(++buf);

        if (tmSet.tm_hour > 23) {
            goto __exit;
        }
        if (tmSet.tm_min > 59) {
            goto __exit;
        }
        if (tmSet.tm_sec > 59) {
            goto __exit;
        }

        // set
        tSet = mktime(&tmSet);
        if (tSet <= tNow) {
            goto __exit;
        }

        timeout = tSet - tNow;
    }

    os_mutex_pend(&alarm_mutex, 0);
    for (int i = 0; i < MAX_ALARM_NUM; ++i) {
        if (!turing_alarm[i].set) {
            turing_alarm[i].set = 1;
            turing_alarm[i].priv = priv;
            if (*p_cyc == '1') {
                turing_alarm[i].cyc = 1;
                turing_alarm[i].id = sys_timeout_add(&turing_alarm[i], turing_alarm_callback, timeout * 1000);
            } else {
                turing_alarm[i].cyc = 0;
                turing_alarm[i].id = sys_timer_add(&turing_alarm[i], turing_alarm_callback, timeout * 1000);
            }
            ret = 0;
            break;
        }
    }
    os_mutex_post(&alarm_mutex);

__exit:

    return ret;
}

void turing_alarm_init(void)
{
    if (!os_mutex_valid(&alarm_mutex)) {
        os_mutex_create(&alarm_mutex);
    }
}

void turing_alarm_del_all(void)
{
    os_mutex_pend(&alarm_mutex, 0);
    for (int i = 0; i < MAX_ALARM_NUM; ++i) {
        if (turing_alarm[i].set) {
            sys_timeout_del(turing_alarm[i].id);
            turing_alarm[i].id = 0;
            turing_alarm[i].priv = NULL;
            turing_alarm[i].set = 0;
            turing_alarm[i].cyc = 0;
        }
    }
    os_mutex_post(&alarm_mutex);
}
