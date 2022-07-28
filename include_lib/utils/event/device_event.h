#ifndef DEVICE_EVNET_H
#define DEVICE_EVNET_H

#include "event/event.h"


enum {
    DEVICE_EVENT_IN,
    DEVICE_EVENT_OUT,
    DEVICE_EVENT_ONLINE,
    DEVICE_EVENT_OFFLINE,
    DEVICE_EVENT_CHANGE,
    DEVICE_EVENT_POWER_CHARGER_IN,
    DEVICE_EVENT_POWER_CHARGER_OUT,
    DEVICE_EVENT_POWER_PERCENT,
    DEVICE_EVENT_POWER_SHUTDOWN,
    DEVICE_EVENT_POWER_STARTUP,
};

enum device_event_from {
    DEVICE_EVENT_FROM_SD = 1,

    DEVICE_EVENT_FROM_USB_HOST,
    DEVICE_EVENT_FROM_OTG,
    DEVICE_EVENT_FROM_PC,
    DEVICE_EVENT_FROM_UAC,

    DEVICE_EVENT_FROM_FM,
    DEVICE_EVENT_FROM_ALM,
    DEVICE_EVENT_FROM_LINEIN,

    DEVICE_EVENT_FROM_EARTCH,
    DEVICE_EVENT_FROM_TONE,
    DEVICE_EVENT_FROM_SENSOR,
    DEVICE_EVENT_FROM_VIDEO,
    DEVICE_EVENT_FROM_POWER,
};

struct device_event {
    u8 event;
    int value;
    void *arg;
};

int device_event_notify(enum device_event_from from, struct device_event *event);
void device_event_clear(u8 from);



#endif
