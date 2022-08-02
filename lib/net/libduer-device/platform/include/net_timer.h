#ifndef __NET_TIMER_H_
#define __NET_TIMER_H_

#include "typedef.h"

int net_timer_init(void);
void net_timer_uninit(void);
int net_timer_add(void *priv, void (*func)(void *priv), u32 msec, u8 periodic);
void net_timer_del(int __t);

#endif
