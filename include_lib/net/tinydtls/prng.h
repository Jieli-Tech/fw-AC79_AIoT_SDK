/* prng.h -- Pseudo Random Numbers
 *
 * Copyright (C) 2010--2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the library tinydtls. Please see
 * README for terms of use.
 */

/**
 * @file prng.h
 * @brief Pseudo Random Numbers
 */

#ifndef _DTLS_PRNG_H_
#define _DTLS_PRNG_H_

#include "tinydtls.h"

/**
 * @defgroup prng Pseudo Random Numbers
 * @{
 */

#include <stdlib.h>

/**
 * Fills \p buf with \p len random bytes. This is the default
 * implementation for prng().  You might want to change prng() to use
 * a better PRNG on your specific platform.
 */
static inline int
dtls_prng(unsigned char *buf, size_t len)
{
    while (len--) {
        *buf++ = rand() & 0xFF;
    }
    return 1;
}

static inline void
dtls_prng_init(unsigned short seed)
{
    srand(seed);
}

/** @} */

#endif /* _DTLS_PRNG_H_ */
