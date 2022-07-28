/*******************************************************************************************
 File Name: cache.h

 Version: 1.00

 Discription:


 Author:yulin deng

 Email :flowingfeeze@163.com

 Date:2014-01-14 09:55:04

 Copyright:(c)JIELI  2011  @ , All Rights Reserved.
*******************************************************************************************/
#ifndef ASM_CACHE_H
#define ASM_CACHE_H

#include "typedef.h"

#define CACHE_LINE_COUNT   32

void flush_dcache(void *ptr, int len);

void flushinv_dcache(void *ptr, int len);

void dcache_way_use_select(u8 cpu_way, u8 prp_way);

void dcache_cwt_set(u8 enable, u32 addr_start, u32 addr_end);

void dcache_cwt_buffer_sync(void *bufa, u32 len);
#endif
