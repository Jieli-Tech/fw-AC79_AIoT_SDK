
/*******************************************************************************************
 File Name: lock.h

 Version: 1.00

 Discription:


 Author:yulin deng

 Email :flowingfeeze@163.com

 Date:2013-10-10 17:38:33

 Copyright:(c)JIELI  2011  @ , All Rights Reserved.
*******************************************************************************************/
#ifndef _DSP_LOCK_H_
#define _DSP_LOCK_H_

#include "typedef.h"

#ifdef   DSP_LOCK_GLOBALS
#define  DSP_LOCK_EXT
#else
#define  DSP_LOCK_EXT  extern
#endif


extern void hold_cpu(void);
extern void release_cpu(void);

typedef  u8 lock_sr;

#define DSP_LOCK(lock_index)    do{asm("TESTSET (%0) ;if !cc jump -2 ":: "p"(lock_index));}while(0)
#define DSP_UNLOCK(lock_index)  do{*lock_index = 0;}while(0)

DSP_LOCK_EXT lock_sr dma0_cpy_lock_index             __attribute__((section(".dsp_lock_index")));
#define DMA0_CPY_DSP_LOCK()                          DSP_LOCK(&dma0_cpy_lock_index)
#define DMA0_CPY_DSP_UNLOCK()                        DSP_UNLOCK(&dma0_cpy_lock_index)

DSP_LOCK_EXT lock_sr dma1_cpy_lock_index             __attribute__((section(".dsp_lock_index")));
#define DMA1_CPY_DSP_LOCK()                          DSP_LOCK(&dma1_cpy_lock_index)
#define DMA1_CPY_DSP_UNLOCK()                        DSP_UNLOCK(&dma1_cpy_lock_index)

DSP_LOCK_EXT lock_sr checksum_lock_index		__attribute__((section(".dsp_lock_index")));
#define CHECKSUM_DSP_LOCK()                         DSP_LOCK(&checksum_lock_index)
#define CHECKSUM_DSP_UNLOCK()                       DSP_UNLOCK(&checksum_lock_index)


#endif  //_DSP_LOCK_H_


