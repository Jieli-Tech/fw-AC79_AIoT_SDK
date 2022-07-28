// Copyright (2017) Baidu Inc. All rights reserveed.
/**
 * File: lightduer_random_impl.cpp
 * Auth: Zhang Leliang(zhangleliang@baidu.com)
 * Desc: Provide the random APIs.
 */

#include "lightduer_random_impl.h"


duer_s32_t duer_random_impl(void)
{
    duer_s32_t rs = 0;

#if 0
    struct timeval tpstart;
    gettimeofday(&tpstart, NULL);
    pid_t tid = gettid();
    how to generate seed fro rand
    duer_u32_t seed = (tid << 5) | tpstart.tv_usec;
    srand(seed);
#endif

#if 0
    rs = rand();
    return rs;
#else
    extern unsigned int random32(int type);
    return random32(0) & 0x7fffffff;
#endif
}
