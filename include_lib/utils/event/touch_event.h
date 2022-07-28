#ifndef TOUCH_EVENT_H
#define TOUCH_EVENT_H


#include "event/event.h"

#define     NO_TOUCH             255

enum touch_action {
    TOUCH_EVENT_CLICK = 0,
    TOUCH_EVENT_LONG,
    TOUCH_EVENT_HOLD,
    TOUCH_EVENT_UP,
    TOUCH_EVENT_DOUBLE_CLICK,
    TOUCH_EVENT_TRIPLE_CLICK,
    TOUCH_EVENT_FOURTH_CLICK,
    TOUCH_EVENT_FIRTH_CLICK,
    TOUCH_EVENT_USER,
    TOUCH_EVENT_MAX,
};

enum touch_from {
    TOUCH_EVENT_FROM_TOUCH,
    TOUCH_EVENT_FROM_TWS,
    TOUCH_EVENT_FROM_USER,
};

struct touch_event {
    int action;
    int x;
    int y;
};

struct touch_event_msg_table {
    u8 touch_value;
    u16 msg_list[8];
};

void touch_event_disable();

void touch_event_enable();

void touch_event_filter_hold_disable();

void touch_event_filter_hold_enable();

void touch_event_set_msg_table(const struct touch_event_msg_table *table);

int touch_event_notify(enum touch_from from, struct touch_event *event);

int touch_event_send_msg(u16 msg);


/*
struct touch_event_msg_table music_key_table[] = {
    {
       .touch_value   = KEY_POWER,
       .msg_list    = {MSG_MUSIC_PP, MSG_POWER_OFF },
    },
    {
       .touch_value   = KEY_NEXT,
       .msg_list    = {MSG_MUSIC_PP, MSG_POWER_OFF },
    },
    {
        .touch_value = NO_KEY;
    },
};
*/
#endif

