#ifndef ADAPTER_SYSTIMER_H
#define ADAPTER_SYSTIMER_H

#if 0

#include "generic/sys_time.h"

#else

#include "generic/typedef.h"
#include "generic/list.h"

typedef int sys_timer;

void sys_timer_init(u32 nms);

void sys_timer_schedule();

u32 sys_timer_get_time(void);

sys_timer sys_timer_register(u32 msec, void (*callback)(void *));

sys_timer sys_timer_register_periodic(u32 msec, void (*callback)(void *));

void sys_timer_remove(sys_timer timer);

void sys_timer_reset(sys_timer timer);

void sys_timer_change_period(sys_timer timer, u32 msec);

void sys_timer_set_context(sys_timer timer, void *context);

void *sys_timer_get_context(sys_timer timer);

#endif

#endif  /*ADAPTER_SYSTIMER_H*/
