#ifndef __UNIX_TIMESTAMP_H__
#define __UNIX_TIMESTAMP_H__

#include "typedef.h"

struct UTC_TIME {
    u16 year : 12,      // max 4095
        month : 4;      // max 15
    u8  day : 5,        // max 31
    weekday : 3;    // max 7
    u8 hour;
    u8 minute;
    u8 second;
} _GNU_PACKED_;


struct UTC_TIME unix32_to_UTC(u32 unix_time);

struct UTC_TIME unix32_to_UTC_beijing(u32 unix_time);

#endif /* __UNIX_TIMESTAMP_H__ */

