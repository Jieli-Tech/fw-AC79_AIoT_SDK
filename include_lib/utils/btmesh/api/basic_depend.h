#ifndef __BASIC_DEPEND_H__
#define __BASIC_DEPEND_H__

#include "generic/typedef.h"
#include "adapter/include/common/sys_timer.h"

/*******************************************************************/
/*
 *-------------------   Typedef
 */

typedef unsigned char       bit1, uint8_t, u8_t;
typedef char                s8_t;
typedef unsigned short      uint16_t, u16_t;
typedef unsigned int        tu8, tu16, tbool, tu32, uint32_t, u32_t;
typedef signed int          s32_t;
typedef unsigned long long  u64_t;
typedef signed long long    s64_t;


/*******************************************************************/
/*
 *-------------------   k_delayed_work
 */
// typedef int             sys_timer;

struct k_work {
    sys_timer   systimer;
    void        *callback;
};

struct k_delayed_work {
    struct k_work work;
    u32 end_time;
};


#endif /* __BASIC_DEPEND_H__ */
