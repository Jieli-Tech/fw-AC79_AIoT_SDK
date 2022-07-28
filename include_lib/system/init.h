#ifndef SYS_INIT_H
#define SYS_INIT_H







typedef int (*initcall_t)(void);

#define __initcall(fn)  \
	const initcall_t __initcall_##fn SEC_USED(.initcall) = fn

#define early_initcall(fn)  \
	const initcall_t __initcall_##fn SEC_USED(.early.initcall) = fn


#define late_initcall(fn)  \
	const initcall_t __initcall_##fn SEC_USED(.late.initcall) = fn


#define platform_initcall(fn) \
	const initcall_t __initcall_##fn SEC_USED(.platform.initcall) = fn


#define module_initcall(fn) \
	const initcall_t __initcall_##fn SEC_USED(.module.initcall) = fn




#define __do_initcall(prefix) \
	do { \
		initcall_t *init; \
		extern initcall_t prefix##_begin[], prefix##_end[]; \
		for (init=prefix##_begin; init<prefix##_end; init++) { \
			(*init)();	\
		} \
	}while(0)







#endif

