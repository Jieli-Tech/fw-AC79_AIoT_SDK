#ifndef _PRINTF_H_
#define _PRINTF_H_

#include <stdarg.h>

extern void putbyte(char a);

extern int putchar(int a);

extern int puts(const char *out);

extern void put_u4hex(unsigned char dat);

extern void put_u8hex(unsigned char dat);

extern void put_u16hex(unsigned short dat);

extern void put_u32hex(unsigned int dat);

extern void put_buf(const unsigned char *buf, int len);

extern int printf(const char *format, ...);

extern int assert_printf(const char *format, ...);

extern int sprintf(char *out, const char *format, ...);

extern int vprintf(const char *fmt, __builtin_va_list va);

extern int vsnprintf(char *, unsigned long, const char *, __builtin_va_list);

extern int snprintf(char *buf, unsigned long size, const char *fmt, ...);

extern int print(char **out, char *end, const char *format, va_list args);

//extern int snprintf(char *, unsigned long, const char *, ...);

extern int sscanf(const char *buf, const char *fmt, ...);   //BUG: 多个参数? 最后又空格?

//int perror(const char *fmt, ...);








int user_putchar(int a);
void user_put_buf(const unsigned char *buf, int len);
int user_puts(const char *out);
int user_printf(const char *format, ...);
int user_sprintf(char *out, const char *format, ...);
int user_vprintf(const char *fmt, __builtin_va_list va);
int user_vsnprintf(char *, unsigned long, const char *, __builtin_va_list);
int user_snprintf(char *buf, unsigned long size, const char *fmt, ...);
int user_print(char **out, char *end, const char *format, va_list args);
int user_sscanf(const char *buf, const char *fmt, ...);   //BUG: 多个参数? 最后又空格?
int user_perror(const char *fmt, ...);

#endif
