#ifndef AES_H
#define AES_H 1

#include "system/includes.h"

#define AES_DEBUG 0
#define AES_DECRYPT 0
#define AES_ENCRYPT 1
#define KEY_SIZE 128

#if defined(__cplusplus)
extern "C" {
#endif
typedef struct _aes_context {
    int nr;
    unsigned int *rk;
    unsigned int buf[68];
} aes_context;
#if (AES_DECRYPT == 1)
//void aes_setkey_dec(aes_context* ctx, const unsigned char* key, int keysize);
#endif
#if (AES_ENCRYPT == 1)
//void aes_setkey_enc(aes_context* ctx, const unsigned char* key, int keysize);
int encode_length(int slen);
unsigned char *encode(aes_context *ctx, int mode, const unsigned char *key, const unsigned char *input, int slen);
#endif
void aes_crypt_ecb_update(aes_context *ctx, int mode, const unsigned char input[16], unsigned char output[16]);
unsigned char *aes_crypt_ecb(aes_context *ctx, int mode, const unsigned char *input, int slen, int *dlen);
#if defined(__cplusplus)
}
#endif
#endif

