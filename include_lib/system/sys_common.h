#ifndef  __COMMON_H__
#define  __COMMON_H__

//用于放common_lib外部调用的函数
#include "asm/clock.h"
#include "generic/jiffies.h"


unsigned int time_lapse(unsigned int *handle, unsigned int time_out);//2^32/1000/60/60/24 后超时

int asprintf(char **ret, const char *format, ...);
u32 timer_get_sec(void);
u32 timer_get_ms(void);

// extern int gettimeofday (struct timeval *__restrict __tv,
// __timezone_ptr_t __tz);

/*int gettimeofday(struct timeval *__restrict __tv, void *__tz);*/

int cal_days(int year, int month);
//size_t strftime_2(char *ptr, size_t maxsize, const char *format, const struct tm *timeptr);
int rand(void);
// int vprintf(const char *fmt, __builtin_va_list va);
// int vsnprintf(char *, unsigned long, const char *, __builtin_va_list);
// int snprintf(char *buf, unsigned int size, const char *fmt, ...);
// //int snprintf(char *, unsigned long, const char *, ...);

// int sscanf(const char *buf, const char *fmt, ...);   //BUG: 多个参数? 最后又空格?

void *conf_file_open(const char *path, const char *mode);

void conf_file_close(void *fp);

char *conf_file_gets(void *fp, char *buf, unsigned int buf_len);



#endif  /*COMMON_H*/
