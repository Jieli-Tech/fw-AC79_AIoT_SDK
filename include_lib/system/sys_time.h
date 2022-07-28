#ifndef SYS_TIME_H
#define SYS_TIME_H

#include "generic/typedef.h"

struct sys_time {
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 min;
    u8 sec;
};

extern u32 timer_get_sec(void);
extern u32 timer_get_ms(void);
extern unsigned int time_lapse(unsigned int *handle, unsigned int time_out);//2^32/1000/60/60/24 后超时


#endif
