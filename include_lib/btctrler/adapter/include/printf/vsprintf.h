#ifndef ADAPTER_VSPRINTF_H
#define ADAPTER_VSPRINTF_H


#include "generic/typedef.h"
#include "stdarg.h"



int __vsprintf_len(const char *format, va_list argptr);
int __vsprintf(u8 *ret, const char *format, va_list argptr);





#endif


