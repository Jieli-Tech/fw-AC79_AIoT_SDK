#ifndef _CC_H_
#define _CC_H_

#include "arch/cpu.h"

/* ------------------------ Type definitions (lwIP) ----------------------- */
//typedef unsigned char u8_t;
//typedef signed char s8_t;
//typedef unsigned short u16_t;
//typedef signed short s16_t;
//typedef unsigned long u32_t;
//typedef signed long s32_t;
//typedef u32_t   mem_ptr_t;
//typedef int     sys_prot_t;


/*----------------------------------------------------------------------------*/

/**********使用GCC开发工具时的宏定义**************/
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT  __attribute__((__packed__)) //_GNU_PACKED_
//  __attribute__ ((__packed__))
#define PACK_STRUCT_END

#define PACK_STRUCT_FIELD( x )  x

#define ALIGN_STRUCT_8_BEGIN
#define ALIGN_STRUCT_8      ALIGNE(8) //   __attribute__ ((aligned (8)))
#define ALIGN_STRUCT_8_END

/* Define (sn)printf formatters for these lwIP types */
#define U16_F                   "u"
#define S16_F                   "d"
#define X16_F                   "x"
#define U32_F                   "u"
#define S32_F                   "d"
#define X32_F                   "x"



#ifndef LWIP_PLATFORM_ASSERT
#define LWIP_PLATFORM_ASSERT(x) \
    do \
    {   printf("\n\n\n\n\n\n\n\n Assertion \"%s\" failed at line %d in %s\n\n\n\n\n\n\n\n\n", x, __LINE__, __FILE__); \
    } while(0)
#endif

#ifndef LWIP_PLATFORM_DIAG
#define LWIP_PLATFORM_DIAG(x)  do {printf x;} while(0)
#endif


#endif //#ifndef _CC_H_
