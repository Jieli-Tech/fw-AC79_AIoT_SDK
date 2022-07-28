#include "aes/rijndael.h"
#include "asm/dv16.h"
#include "generic/typedef.h"

#ifndef GET_UINT32_LE
#define GET_UINT32_LE(n,b,i)                            \
{                                                       \
    (n) = ( (uint32_t) (b)[(i)    ]  <<24     )             \
        | ( (uint32_t) (b)[(i) + 1] <<  16 )             \
        | ( (uint32_t) (b)[(i) + 2] << 8)             \
        | ( (uint32_t) (b)[(i) + 3]  );            \
}
#endif

#ifndef PUT_UINT32_LE
#define PUT_UINT32_LE(n,b,i)                                    \
{                                                               \
    (b)[(i)    ] = (unsigned char) ( ( (n) >> 24 ) & 0xFF );    \
    (b)[(i) + 1] = (unsigned char) ( ( (n) >> 16 ) & 0xFF );    \
    (b)[(i) + 2] = (unsigned char) ( ( (n) >>  8 ) & 0xFF );    \
    (b)[(i) + 3] = (unsigned char) ( ( (n)       ) & 0xFF );    \
}
#endif

#if defined(DTLS_AES_SETKEY_ENC_ALT)
int
rijndaelKeySetupEnc1(rijndael_ctx *ctx, const aes_u8 cipherKey[], int keyBits)
{
    switch (keyBits) {
    case 128:
        ctx->Nr = 10;
        memcpy((void *)ctx->aes_enc_key, (void *)cipherKey, 16);
        break;
    case 192:
        ctx->Nr = 12;
        printf("not support aes192\n");
        break;
    case 256:
        ctx->Nr = 14;
        printf("not support aes256\n");
        break;
    default :
        return (-1);
    }
    return ctx->Nr;

}
#endif

#if defined(DTLS_AES_SETKEY_DEC_ALT)
int
rijndaelKeySetupDec(rijndael_ctx *ctx, const aes_u8 cipherKey[], int keyBits)
{
    switch (keyBits) {
    case 128:
        ctx->Nr = 10;
        memcpy(ctx->aes_dec_key, cipherKey, 16);
        break;
    case 192:
        ctx->Nr = 12;
        printf("not support aes192\n");
        break;
    case 256:
        ctx->Nr = 14;
        printf("not support aes256\n");
        break;
    default :
        return (-1);
    }
    return ctx->Nr;
}
#endif



#if defined(DTLS_AES_DECRYPT_ALT)

void
rijndael_decrypt(rijndael_ctx *ctx, const u_char *input, u_char *output)
{
    GET_UINT32_LE(AES_KEY, ctx->aes_dec_key, 0);
    GET_UINT32_LE(AES_KEY, ctx->aes_dec_key, 4);
    GET_UINT32_LE(AES_KEY, ctx->aes_dec_key, 8);
    GET_UINT32_LE(AES_KEY, ctx->aes_dec_key, 12);
    AES_CON |= (1 << 4);
    while (!(AES_CON & (1 << 5)));

    GET_UINT32_LE(AES_DAT0, input,  12);
    GET_UINT32_LE(AES_DAT1, input,  8);
    GET_UINT32_LE(AES_DAT2, input,  4);
    GET_UINT32_LE(AES_DAT3, input, 0);

    AES_CON |= (1 << 1); //
    while (!(AES_CON & (1 << 3)));

    PUT_UINT32_LE(AES_DECRES0, output,  12);
    PUT_UINT32_LE(AES_DECRES1, output,  8);
    PUT_UINT32_LE(AES_DECRES2, output,  4);
    PUT_UINT32_LE(AES_DECRES3, output,  0);


}
#endif

#if defined(DTLS_AES_ENCRYPT_ALT)
void
rijndael_encrypt(rijndael_ctx *ctx, const u_char *input, u_char *output)
{
    GET_UINT32_LE(AES_KEY, ctx->aes_enc_key, 0);
    GET_UINT32_LE(AES_KEY, ctx->aes_enc_key, 4);
    GET_UINT32_LE(AES_KEY, ctx->aes_enc_key, 8);
    GET_UINT32_LE(AES_KEY, ctx->aes_enc_key, 12);

    AES_CON |= (1 << 4);
    while (!(AES_CON & (1 << 5)));

    GET_UINT32_LE(AES_DAT3, input,  0);
    GET_UINT32_LE(AES_DAT2, input,  4);
    GET_UINT32_LE(AES_DAT1, input,  8);
    GET_UINT32_LE(AES_DAT0, input,  12);

    AES_CON |= (1 << 0);

    while (!(AES_CON & (1 << 2)));

    PUT_UINT32_LE(AES_ENCRES3, output,  0);
    PUT_UINT32_LE(AES_ENCRES2, output,  4);
    PUT_UINT32_LE(AES_ENCRES1, output,  8);
    PUT_UINT32_LE(AES_ENCRES0, output,  12);
}
#endif


#if defined(DTLS_AES_TEST)
static const unsigned char aes_test_ecb_dec[3][16] = {
    {
        0x44, 0x41, 0x6A, 0xC2, 0xD1, 0xF5, 0x3C, 0x58,
        0x33, 0x03, 0x91, 0x7E, 0x6B, 0xE9, 0xEB, 0xE0
    },
    {
        0x48, 0xE3, 0x1E, 0x9E, 0x25, 0x67, 0x18, 0xF2,
        0x92, 0x29, 0x31, 0x9C, 0x19, 0xF1, 0x5B, 0xA4
    },
    {
        0x05, 0x8C, 0xCF, 0xFD, 0xBB, 0xCB, 0x38, 0x2D,
        0x1F, 0x6F, 0x56, 0x58, 0x5D, 0x8A, 0x4A, 0xDE
    }
};
static const unsigned char aes_test_ecb_enc[3][16] = {
    {
        0xC3, 0x4C, 0x05, 0x2C, 0xC0, 0xDA, 0x8D, 0x73,
        0x45, 0x1A, 0xFE, 0x5F, 0x03, 0xBE, 0x29, 0x7F
    },
    {
        0xF3, 0xF6, 0x75, 0x2A, 0xE8, 0xD7, 0x83, 0x11,
        0x38, 0xF0, 0x41, 0x56, 0x06, 0x31, 0xB1, 0x14
    },
    {
        0x8B, 0x79, 0xEE, 0xCC, 0x93, 0xA0, 0xEE, 0x5D,
        0xFF, 0x30, 0xB4, 0xEA, 0x21, 0x63, 0x6D, 0xA4
    }
};
int mbedtls_aes_crypt_ecb(rijndael_ctx *ctx,
                          int mode,
                          const unsigned char input[16],
                          unsigned char output[16])
{
    if (mode == 0) {
        rijndael_decrypt(ctx, input, output);
    } else {
        rijndael_encrypt(ctx, input, output);
    }

    return (0);
}

int dtls_aes_test(int verbose)
{
    int ret = 0, i, j, u, v;
    unsigned char key[32];
    unsigned char buf[64];
    rijndael_ctx ctx;
    memset(key, 0, 32);


    for (i = 0; i < 2; i++) {
        u = 0;
        v = i  & 1;

        if (verbose != 0)
            printf("DTLS  AES-ECB-%3d (%s): ", 128 + u * 64,
                   (v == 0) ? "dec" : "enc");

        memset(buf, 0, 16);
        if (v == 0) {
            rijndaelKeySetupDec(&ctx, key, 128 + u * 64);

            for (j = 0; j < 10000; j++) {
                mbedtls_aes_crypt_ecb(&ctx, v, buf, buf);
            }

            if (memcmp(buf, aes_test_ecb_dec[u], 16) != 0) {
                if (verbose != 0) {
                    printf("failed\n");
                }

                ret = 1;
                goto exit;
            }
        } else {
            rijndaelKeySetupEnc1(&ctx, key, 128 + u * 64);

            for (j = 0; j < 10000; j++) {
                mbedtls_aes_crypt_ecb(&ctx, v, buf, buf);
            }

            if (memcmp(buf, aes_test_ecb_enc[u], 16) != 0) {
                if (verbose != 0) {
                    printf("failed\n");
                }

                ret = 1;
                goto exit;
            }
        }

        if (verbose != 0) {
            printf("passed\n");
        }
    }

    if (verbose != 0) {
        printf("\n");
    }

exit:
    return 0;
}


#endif // DTLS_AES_TEST
