// Copyright (2017) Baidu Inc. All rights reserved.
//
// File: baidu_ca_memory_adp.c
// Auth: Zhang Leliang (zhangleliang @baidu.com)
// Desc: Adapt the memory function to linux.


#include "baidu_ca_adapter_internal.h"

void *bcamem_alloc(duer_context ctx, duer_size_t size)
{
    return malloc(size);
}

void *bcamem_realloc(duer_context ctx, void *ptr, duer_size_t size)
{
    return realloc(ptr, size);
}

void bcamem_free(duer_context ctx, void *ptr)
{
    free(ptr);
}
