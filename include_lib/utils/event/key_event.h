#ifndef KEY_EVENT_H
#define KEY_EVENT_H


#include "event/event.h"


#define     KEY_POWER           0
#define     KEY_PREV            1
#define     KEY_NEXT            2
#define     KEY_PLAY            3
#define     KEY_VOLUME_DEC      4
#define     KEY_VOLUME_INC      5
#define     KEY_MODE            6
#define     KEY_MENU            7
#define     KEY_ENC             8
#define     KEY_PHONE           9
#define     KEY_PHOTO           10

#define 	KEY_F1 				11
#define     KEY_OK              12
#define     KEY_CANCLE          13
#define     KEY_LEFT            14
#define     KEY_UP              15
#define     KEY_RIGHT           16
#define     KEY_DOWN            17
#define     KEY_MUTE            18
#define     KEY_COLLECT         19
#define     KEY_LOCAL           20

#define 	KEY_0 				30
#define 	KEY_1 				31
#define 	KEY_2 				32
#define 	KEY_3 				33
#define 	KEY_4 				34
#define 	KEY_5 				35
#define 	KEY_6 				36
#define 	KEY_7 				37
#define 	KEY_8 				38
#define 	KEY_9 				39

#define     NO_KEY              255

#define 	KEY_K1 			    13
#define 	KEY_K2 			    6
#define 	KEY_K3 			    4
#define 	KEY_K4 			    5
#define 	KEY_K5 		        12
#define 	KEY_K6 			    10
#define 	KEY_K7 			    8
#define 	KEY_K8 			    0

enum key_action {
    KEY_EVENT_CLICK = 0,
    KEY_EVENT_LONG,
    KEY_EVENT_HOLD,
    KEY_EVENT_UP,
    KEY_EVENT_DOUBLE_CLICK,
    KEY_EVENT_TRIPLE_CLICK,
    KEY_EVENT_FOURTH_CLICK,
    KEY_EVENT_FIRTH_CLICK,
    KEY_EVENT_USER,
    KEY_EVENT_MAX,
};

enum key_from {
    KEY_EVENT_FROM_KEY,
    KEY_EVENT_FROM_TWS,
    KEY_EVENT_FROM_USER,
};

struct key_event {
    u8 type;
    u8 action;
    u8 value;
    u16 msg;
};

struct key_event_msg_table {
    u8 key_value;
    u16 msg_list[8];
};

void key_event_disable();

void key_event_enable();

void key_event_filter_hold_disable();

void key_event_filter_hold_enable();

void key_event_set_msg_table(const struct key_event_msg_table *table);

int key_event_notify(enum key_from from, struct key_event *event);

int key_event_send_msg(u16 msg);


/*
struct key_event_msg_table music_key_table[] = {
    {
       .key_value   = KEY_POWER,
       .msg_list    = {MSG_MUSIC_PP, MSG_POWER_OFF },
    },
    {
       .key_value   = KEY_NEXT,
       .msg_list    = {MSG_MUSIC_PP, MSG_POWER_OFF },
    },
    {
        .key_value = NO_KEY;
    },
};
*/
#endif

