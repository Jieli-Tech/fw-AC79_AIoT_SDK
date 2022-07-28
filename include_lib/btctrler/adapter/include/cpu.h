#ifndef ADAPTER_CPU_H
#define ADAPTER_CPU_H

#include "os/os_cpu.h"

#define TRIGGER         __asm__ volatile ("trigger")
#define SYNC            __asm__ volatile ("csync")

#define MULU(Rm,Rn)
#define MUL(Rm,Rn)
#define MLA(Rm,Rn)
#define MLUA(Rm,Rn)
#define MACCLR()
#define MACCSDIV()
#define MACCUDIV()
#define MLA0(Rm,Rn) MUL(Rm,Rn);
#define MLS(Rm,Rn)
#define MLS0(Rm,Rn) MUL(-Rm,Rn);
#define MRSIS(Rm,Rn)
#define MRSRS(Rm,Rn)
#define MRSI(Rm,Rn)
#define MRSR(Rm,Rn)
#define MACSET(h,l)
#define MACRL(l)
#define MACRH(l)
#define MULSIS(Ro,Rm,Rn,Rp) MUL(Rm, Rn); MRSIS(Ro, Rp);
#define MULSRS(Ro,Rm,Rn,Rp) MUL(Rm, Rn); MRSRS(Ro, Rp);

// extern void vPortEnterCritical(void);
// extern void vPortExitCritical(void);

//#define OS_SR_ALLOC(...)

// #define OS_ENTER_CRITICAL()     vPortEnterCritical();         [> 关中断<]
// #define OS_EXIT_CRITICAL()      vPortExitCritical();          [> 开中断<]

#endif
