
#ifndef __IPERF_ASSERT_H
#define __IPERF_ASSERT_H


extern void __assert_func_cmpt(const char *file, int line, const char *func, const char *_e);
# define assert_cmpt(__e) ((__e) ? (void)0 : __assert_func_cmpt(__FILE__, __LINE__, \
						       __FUNCTION__, #__e))


#endif /* !__IPERF_ASSERT_H */

