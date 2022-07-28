#ifndef _CRYPTO_HASH_H_
#define _CRYPTO_HASH_H_

#include "generic/typedef.h"

#define HMAC_SHA_256_SUPPORT_P192       0
#define HMAC_SHA_256_SUPPORT_P256       1

#define HMAC_SHA_256_SUPPORT    HMAC_SHA_256_SUPPORT_P192

#if (HMAC_SHA_256_SUPPORT == HMAC_SHA_256_SUPPORT_P192)
#define BITS        192
#endif

#if (HMAC_SHA_256_SUPPORT == HMAC_SHA_256_SUPPORT_P256)
#define BITS        256
#endif


void g_hash_function(u8 *U, u8 *V, u8 *X, u8 *Y, u8 *output);
void f1_hash_function(u8 *U, u8 *V, u8 *X, u8 *Z, u8 *re);
void f2_hash_function(u8 *W, u8 *N1, u8 *N2, u8 *keyID, u8 *A1, u8 *A2, u8 *re);
void f3_hash_function(u8 *W, u8 *N1, u8 *N2, u8 *R, u8 *IOcap, u8 *A1, u8 *A2, u8 *re);
void h2_hash_function(u8 *W, u8 *KeyID, u8 *L, u8 *re);
void h3_hash_function(u8 *W, u8 *keyID, u8 *A1, u8 *A2, u8 *ACO, u8 *re);
void h4_hash_function(u8 *W, u8 *keyID, u8 *A1, u8 *A2, u8 *re);
void h5_hash_function(u8 *W, u8 *, u8 *, u8 *re);
void SSP_Heap_init(u8 *p, int len);

u32 g_function(u8 *U, u8 *V, u8 *X, u8 *Y);
void f1_function(u8 *U, u8 *V, u8 *X, u8 *Z, u8 *re);
void f2_function(u8 *W, u8 *N1, u8 *N2, u8 *A1, u8 *A2, u8 *re);
void f3_function(u8 *W, u8 *N1, u8 *N2, u8 *R, u8 *IOcap, u8 *A1, u8 *A2, u8 *re);

#endif
