// Copyright (2017) Baidu Inc. All rights reserved.
//
// File: lightduer_priority_conf.h
// Auth: Su Hao (suhao@baidu.com)
// Desc: Configuation the task priority.


#ifndef BAIDU_DUER_LIGHTDUER_PORT_SOURCE_FREERTOS_LIGHTDUER_PRIORITY_CONF_H
#define BAIDU_DUER_LIGHTDUER_PORT_SOURCE_FREERTOS_LIGHTDUER_PRIORITY_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

enum _task_identify_e {
    DUER_TASK_CA,
    DUER_TASK_VOICE,
    DUER_TASK_SOCKET,
    DUER_TASK_OTA,
    DUER_TASK_BIND,
    DUER_TASK_HANDLER,
    DUER_TASK_TOTAL
};

int duer_priority_get_task_id(const char *name);

void duer_priority_set(int task, int priority);

int duer_priority_get(int task);

#ifdef __cplusplus
}
#endif

#endif/*BAIDU_DUER_LIGHTDUER_PORT_SOURCE_FREERTOS_LIGHTDUER_PRIORITY_CONF_H*/
