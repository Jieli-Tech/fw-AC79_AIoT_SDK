// Copyright (2017) Baidu Inc. All rights reserveed.
/**
 * File: duerapp_alarm.h
 * Auth: Gang Chen (chengang12@baidu.com)
 * Desc: Duer init alarm.
 */

#ifndef BAIDU_DUER_DUERAPP_DUERAPP_INIT_ALARM_H
#define BAIDU_DUER_DUERAPP_DUERAPP_INIT_ALARM_H

#include "lightduer_timers.h"
#include "list.h"

typedef struct duerapp_alarm_node_ {
    int id;
    u8 alarm_type;
    u8 del_flag;
    u8 is_active;
    char *time;
    char *token;
    char *url;
    duer_timer_handler handle;
    struct list_head entry;
} duerapp_alarm_node;

void duer_init_alarm();
bool duer_alert_bell();
void duer_alert_stop();

#endif/*BAIDU_DUER_DUERAPP_DUERAPP_INIT_ALARM_H*/
