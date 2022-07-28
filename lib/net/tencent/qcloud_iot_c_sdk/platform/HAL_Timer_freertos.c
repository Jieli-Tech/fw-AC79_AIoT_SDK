/*
 * Tencent is pleased to support the open source community by making IoT Hub
 available.
 * Copyright (C) 2018-2020 THL A29 Limited, a Tencent company. All rights
 reserved.

 * Licensed under the MIT License (the "License"); you may not use this file
 except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software
 distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND,
 * either express or implied. See the License for the specific language
 governing permissions and
 * limitations under the License.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "qcloud_iot_import.h"

uint32_t HAL_GetTimeMs(void)
{
    extern u32 timer_get_ms(void);
    return timer_get_ms();
}

/*Get timestamp*/
long HAL_Timer_current_sec(void)
{
    return time(NULL);
}

int gettimeofday(struct timeval *tv, void *tz);

char *HAL_Timer_current(char *time_str)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t    now_time = tv.tv_sec;
//    struct tm tm_tmp   = *localtime(&now_time);
//    strftime(time_str, TIME_FORMAT_STR_LEN, "%F %T", &tm_tmp);
    HAL_Snprintf(time_str, 12, "%d", now_time);

    return time_str;
}


bool HAL_Timer_expired(Timer *timer)
{
    uint32_t now_ts;

    now_ts = HAL_GetTimeMs();

    return (now_ts > timer->end_time) ? true : false;
}

void HAL_Timer_countdown_ms(Timer *timer, unsigned int timeout_ms)
{
    timer->end_time = HAL_GetTimeMs();
    timer->end_time += timeout_ms;
}

void HAL_Timer_countdown(Timer *timer, unsigned int timeout)
{
    timer->end_time = HAL_GetTimeMs();
    timer->end_time += timeout * 1000;
}

int HAL_Timer_remain(Timer *timer)
{
    return (int)(timer->end_time - HAL_GetTimeMs());
}

void HAL_Timer_init(Timer *timer)
{
    timer->end_time = 0;
}

#ifdef __cplusplus
}
#endif
