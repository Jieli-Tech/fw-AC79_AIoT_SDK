// Copyright (2017) Baidu Inc. All rights reserved.
//
// File: lightduer_priority_conf.c
// Auth: Su Hao (suhao@baidu.com)
// Desc: Configuation the task priority.


#include "lightduer_priority_conf.h"

#include <string.h>

static int g_task_priorities[DUER_TASK_TOTAL] = {
    24, 25, 24, 23, 23, 22
};

static const char *const g_priority_tags[] = {
    "lightduer_ca",
    "lightduer_voice",
    "lightduer_socket",
    "lightduer_OTA_Updater",
    "lightduer_bind_device",
    "lightduer_handler",
};

int duer_priority_get_task_id(const char *name)
{
    int i = 0;
    for (i = 0; i < DUER_TASK_TOTAL; i ++) {
        if (strcmp(g_priority_tags[i], name) == 0) {
            return i;
        }
    }
    return -1;
}

void duer_priority_set(int task, int priority)
{
    if (task >= 0 && task < DUER_TASK_TOTAL) {
        g_task_priorities[task] = priority;
    }
}

int duer_priority_get(int task)
{
    return (task >= 0 && task < DUER_TASK_TOTAL) ? g_task_priorities[task] : -1;
}
