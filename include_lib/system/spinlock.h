#ifndef SYS_SPINLOCK_H
#define SYS_SPINLOCK_H

#include "generic/typedef.h"

struct __spinlock {
    volatile u8 rwlock;
};

typedef struct __spinlock spinlock_t;

#if CPU_CORE_NUM > 1


#define preempt_disable() \
	__local_irq_disable()

#define preempt_enable() \
	__local_irq_enable()

#else

#define preempt_disable() \
	local_irq_disable()

#define preempt_enable() \
	local_irq_enable()




#endif


#if CPU_CORE_NUM > 1

#define spin_acquire(lock) 	\
	do { \
		arch_spin_lock(lock); \
	}while(0)

#define spin_release(lock) \
	do { \
		arch_spin_unlock(lock); \
	}while(0)

#else

#define spin_acquire(lock) 	\
	do { \
		while ((lock)->rwlock); \
		(lock)->rwlock = 1; \
	}while(0)


#define spin_release(lock) \
	do { \
		(lock)->rwlock = 0; \
	}while(0)

#endif


#define DEFINE_SPINLOCK(x) \
	spinlock_t x = { .rwlock = 0 }


__attribute__((always_inline))
static inline void spin_lock_init(spinlock_t *lock)
{
    lock->rwlock = 0;
}

__attribute__((always_inline))
static inline void spin_lock(spinlock_t *lock)
{
    preempt_disable();
    /*ASSERT(spin_lock_cnt[current_cpu_id()] == 0);
    spin_lock_cnt[current_cpu_id()] = 1;*/
    spin_acquire(lock);
}

__attribute__((always_inline))
static inline void spin_unlock(spinlock_t *lock)
{
    /*spin_lock_cnt[current_cpu_id()] = 0;*/
    spin_release(lock);
    preempt_enable();
}


#endif


