/* Copyright 2015, Kenneth MacKay. Licensed under the BSD 2-clause license. */

#ifndef _XSPECIFIC_H_
#define _XSPECIFIC_H_

#include "system/includes.h"
#include "crypto_toolbox/micro-ecc/uECC_vli.h"
#include "crypto_toolbox/micro-ecc/uECC_types.h"
cmpresult_t uECC_vli_cmp_unsafe(const uECC_word_t *left,
                                const uECC_word_t *right,
                                wordcount_t num_words) ;
#if (uECC_WORD_SIZE == 1)
#if uECC_SUPPORTS_secp160r1
#define uECC_MAX_WORDS 21 /* Due to the size of curve_n. */
#endif
#if uECC_SUPPORTS_secp192r1
#undef uECC_MAX_WORDS
#define uECC_MAX_WORDS 24
#endif
#if uECC_SUPPORTS_secp224r1
#undef uECC_MAX_WORDS
#define uECC_MAX_WORDS 28
#endif
#if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
#undef uECC_MAX_WORDS
#define uECC_MAX_WORDS 32
#endif
#elif (uECC_WORD_SIZE == 4)
#if uECC_SUPPORTS_secp160r1
#define uECC_MAX_WORDS 6 /* Due to the size of curve_n. */
#endif
#if uECC_SUPPORTS_secp192r1
#undef uECC_MAX_WORDS
#define uECC_MAX_WORDS 6
#endif
#if uECC_SUPPORTS_secp224r1
#undef uECC_MAX_WORDS
#define uECC_MAX_WORDS 7
#endif
#if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
#undef uECC_MAX_WORDS
#define uECC_MAX_WORDS 8
#endif
#elif (uECC_WORD_SIZE == 8)
#if uECC_SUPPORTS_secp160r1
#define uECC_MAX_WORDS 3
#endif
#if uECC_SUPPORTS_secp192r1
#undef uECC_MAX_WORDS
#define uECC_MAX_WORDS 3
#endif
#if uECC_SUPPORTS_secp224r1
#undef uECC_MAX_WORDS
#define uECC_MAX_WORDS 4
#endif
#if (uECC_SUPPORTS_secp256r1 || uECC_SUPPORTS_secp256k1)
#undef uECC_MAX_WORDS
#define uECC_MAX_WORDS 4
#endif
#endif /* uECC_WORD_SIZE */
struct uECC_Curve_t {
    wordcount_t num_words;
    wordcount_t num_bytes;
    bitcount_t num_n_bits;
    uECC_word_t p[uECC_MAX_WORDS];
    uECC_word_t n[uECC_MAX_WORDS];
    uECC_word_t G[uECC_MAX_WORDS * 2];
    uECC_word_t b[uECC_MAX_WORDS];
    void (*double_jacobian)(uECC_word_t *X1,
                            uECC_word_t *Y1,
                            uECC_word_t *Z1,
                            uECC_Curve curve);
#if uECC_SUPPORT_COMPRESSED_POINT
    void (*mod_sqrt)(uECC_word_t *a, uECC_Curve curve);
#endif
    void (*x_side)(uECC_word_t *result, const uECC_word_t *x, uECC_Curve curve);
#if (uECC_OPTIMIZATION_LEVEL > 0)
    void (*mmod_fast)(uECC_word_t *result, uECC_word_t *product);
#endif
};
#include "crypto_toolbox/specific_rom_inc/curve-specific_rom.inc"
#include "crypto_toolbox/specific_rom_inc/platform-specific_rom.inc"

#endif /* _UECC_TYPES_H_ */
