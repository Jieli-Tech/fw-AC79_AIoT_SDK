

typedef unsigned long size_t ;

#include <stdarg.h>
#include "printf.h"
extern "C" {
    void *malloc(size_t);
    void free(void *);
    //int vsnprintf(char *, unsigned long, const char *, __builtin_va_list);
}

extern "C" {
    __attribute__((noreturn))
    void __cxa_pure_virtual(void)
    {
        // puts("Pure virutal function called!");
        while (1);
    }

    __attribute__((noreturn))
    void __cxa_deleted_virtual(void)
    {
        // puts("Deleted virtual function called!");
        while (1);
    }

    int __cxa_atexit(void (*destructor)(void *), void *arg, void *dso)
    {
        // 调用这个函数来注册全局变量的析构函数
        // 析构全局变量的时候，会调用这里注册的东西
        return 0;
    }
    void __cxa_finalize(void *f)
    {
        // puts("FIX ME: (__cxa_finalize) not yet supported\n");
    }

    void *__dso_handle __attribute__((__visibility__("hidden")));

    int swprintf(wchar_t *ws, size_t len, const wchar_t *format, ...)
    {
        // puts("FIX ME: (swprintf) not yet supported\n");
        //return 0;
        return -1;
    }

    int fputc(int c, void *stream)   // should be FILE *stream
    {
        // puts("FIX ME: (fputc) not yet supported\n");
        //return 0;111
        return -1;
    }

    void *_malloc_r(size_t sz)
    {
        return malloc(sz);
    }

    void _free_r(void *p)
    {
        return free(p);
    }

    void *_calloc_r(size_t sz)
    {
        return malloc(sz);
    }

    // int vasprintf(char **strp, const char *fmt, va_list ap) {
    // puts("FIX ME: (vasprintf) not yet supported\n");
    // va_list ap_copy;
    // *strp = 0;

    // va_copy(ap_copy, ap);
    // int cnt = vsnprintf(NULL, 0, fmt, ap);
    // if (cnt >= 0)
    // {
    // char* buffer = (char *)malloc(cnt + 1);
    // if (buffer != NULL)
    // {
    // cnt = vsnprintf(buffer, cnt + 1, fmt, ap_copy);
    // if (cnt < 0)
    // free(buffer);
    // else
    // *strp = buffer;
    // }
    // }

    // va_end(ap_copy);

    // return cnt;

    // }
}

__attribute__((__weak__, __visibility__("default")))
void *
operator new (size_t size)
{
    if (size == 0) {
        size = 1;
    }
    void *p = malloc(size);
    if (p == (void *)0) {
        // puts("malloc failed");
        while (1);
    }
    return p;
}

__attribute__((__weak__, __visibility__("default")))
void *
operator new[](size_t size)
{
    return ::operator new (size);
}

__attribute__((__weak__, __visibility__("default")))
void
operator delete (void *ptr)
{
    if (ptr) {
        free(ptr);
    }
}

__attribute__((__weak__, __visibility__("default")))
void
operator delete[](void *ptr)
{
    ::operator delete (ptr);
}



