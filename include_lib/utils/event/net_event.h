#ifndef NET_EVENT_H
#define NET_EVENT_H

#include "event/event.h"


enum net_event_from {
    NET_EVENT_FROM_USER = 1,
};

enum {
    NET_EVENT_CMD,
    NET_EVENT_DATA,
    NET_EVENT_CONNECTED,
    NET_EVENT_DISCONNECTED,
    NET_EVENT_SMP_CFG_TIMEOUT,
    NET_EVENT_SMP_CFG_FINISH,
    NET_EVENT_SMP_CFG_FIRST,
    NET_CONNECT_TIMEOUT_NOT_FOUND_SSID,
    NET_CONNECT_TIMEOUT_ASSOCIAT_FAIL,
    NET_SMP_CFG_COMPLETED,
    NET_EVENT_DISCONNECTED_AND_REQ_CONNECT,
    NET_NTP_GET_TIME_SUCC,
};

struct net_event {
    u8 event;
    void *arg;
};

int net_event_notify(enum net_event_from from, struct net_event *event);


#endif

