#ifndef SYS_EVENT_H
#define SYS_EVENT_H

#include "generic/typedef.h"


#define SYS_ALL_EVENT           0xffff

#define SYS_KEY_EVENT 			0x0001
#define SYS_TOUCH_EVENT 		0x0002
#define SYS_DEVICE_EVENT 		0x0004
#define SYS_NET_EVENT 		    0x0008
#define SYS_BT_EVENT 		    0x0010


struct sys_event {
    u16 type;
    u8 from;
    u8 len;
    u8 payload[0];
};


struct static_event_handler {
    u16 event_type;
    int (*prob_handler)(struct sys_event *);
    void (*post_handler)(struct sys_event *);
};


#define SYS_EVENT_STATIC_HANDLER_REGISTER(handler) \
	const struct static_event_handler __static_event_##handler \
        SEC_USED(.sys_event.handler)

extern struct static_event_handler sys_event_handler_begin[];
extern struct static_event_handler sys_event_handler_end[];

#define list_for_each_static_event_handler(p) \
	for (p = sys_event_handler_begin; p < sys_event_handler_end; p++)



int register_sys_event_handler(int event_type, u8 from, u8 priority,
                               void (*handler)(struct sys_event *));


void unregister_sys_event_handler(void (*handler)(struct sys_event *));


/*
 * 事件通知函数,系统有事件发生时调用此函数
 */
int sys_event_notify(u16 type, u8 from, void *event, u8 len);


void sys_event_clear(u16 type, u8 from);





#endif
