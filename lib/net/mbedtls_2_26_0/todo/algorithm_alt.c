#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/mbedtls_config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_AES_C)

#include <string.h>

#include "mbedtls/aes.h"
#include "mbedtls/sha1.h"
#include "mbedtls/sha256.h"
#if defined(MBEDTLS_PADLOCK_C)
#include "mbedtls/padlock.h"
#endif
#if defined(MBEDTLS_AESNI_C)
#include "mbedtls/aesni.h"
#endif
#include "asm/cpu.h"
#include "system/spinlock.h"

static DEFINE_SPINLOCK(aes_lock);


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

#if defined(MBEDTLS_AES_ENCRYPT_ALT)
void mbedtls_aes_encrypt_alt(mbedtls_aes_context *ctx,
                             const unsigned char input[16],
                             unsigned char output[16])
{
    spin_lock(&aes_lock);

    GET_UINT32_LE(JL_AES->KEY, ctx->aes_enc_key, 0);
    GET_UINT32_LE(JL_AES->KEY, ctx->aes_enc_key, 4);
    GET_UINT32_LE(JL_AES->KEY, ctx->aes_enc_key, 8);
    GET_UINT32_LE(JL_AES->KEY, ctx->aes_enc_key, 12);
    if (ctx->nr == 14) {
        GET_UINT32_LE(JL_AES->KEY, ctx->aes_enc_key, 16);
        GET_UINT32_LE(JL_AES->KEY, ctx->aes_enc_key, 20);
        GET_UINT32_LE(JL_AES->KEY, ctx->aes_enc_key, 24);
        GET_UINT32_LE(JL_AES->KEY, ctx->aes_enc_key, 28);
        JL_AES->CON |= BIT(6);
    } else {
        JL_AES->CON &= ~BIT(6);
    }

    JL_AES->CON |= BIT(4);
    while ((JL_AES->CON & BIT(5)) == 0);
    JL_AES->CON |= BIT(13);

    GET_UINT32_LE(JL_AES->DATIN, input, 0);
    GET_UINT32_LE(JL_AES->DATIN, input, 4);
    GET_UINT32_LE(JL_AES->DATIN, input, 8);
    GET_UINT32_LE(JL_AES->DATIN, input, 12);

    JL_AES->CON |= BIT(0);
    while ((JL_AES->CON & BIT(2)) == 0);
    JL_AES->CON |= BIT(12);

    PUT_UINT32_LE(JL_AES->ENCRES3, output, 0);
    PUT_UINT32_LE(JL_AES->ENCRES2, output, 4);
    PUT_UINT32_LE(JL_AES->ENCRES1, output, 8);
    PUT_UINT32_LE(JL_AES->ENCRES0, output, 12);

    spin_unlock(&aes_lock);
}
#endif

#if defined(MBEDTLS_AES_DECRYPT_ALT)
void mbedtls_aes_decrypt_alt(mbedtls_aes_context *ctx,
                             const unsigned char input[16],
                             unsigned char output[16])
{
    spin_lock(&aes_lock);

    GET_UINT32_LE(JL_AES->KEY, ctx->aes_dec_key, 0);
    GET_UINT32_LE(JL_AES->KEY, ctx->aes_dec_key, 4);
    GET_UINT32_LE(JL_AES->KEY, ctx->aes_dec_key, 8);
    GET_UINT32_LE(JL_AES->KEY, ctx->aes_dec_key, 12);
    if (ctx->nr == 14) {
        GET_UINT32_LE(JL_AES->KEY, ctx->aes_dec_key, 16);
        GET_UINT32_LE(JL_AES->KEY, ctx->aes_dec_key, 20);
        GET_UINT32_LE(JL_AES->KEY, ctx->aes_dec_key, 24);
        GET_UINT32_LE(JL_AES->KEY, ctx->aes_dec_key, 28);
        JL_AES->CON |= BIT(6);
    } else {
        JL_AES->CON &= ~BIT(6);
    }

    JL_AES->CON |= BIT(4);
    while ((JL_AES->CON & BIT(5)) == 0);
    JL_AES->CON |= BIT(13);

    GET_UINT32_LE(JL_AES->DATIN, input, 0);
    GET_UINT32_LE(JL_AES->DATIN, input, 4);
    GET_UINT32_LE(JL_AES->DATIN, input, 8);
    GET_UINT32_LE(JL_AES->DATIN, input, 12);

    JL_AES->CON |= BIT(1);
    while ((JL_AES->CON & BIT(3)) == 0);
    JL_AES->CON |= BIT(11);

    PUT_UINT32_LE(JL_AES->DECRES3, output, 0);
    PUT_UINT32_LE(JL_AES->DECRES2, output, 4);
    PUT_UINT32_LE(JL_AES->DECRES1, output, 8);
    PUT_UINT32_LE(JL_AES->DECRES0, output, 12);

    spin_unlock(&aes_lock);
}
#endif

#endif  //MBEDTLS_AES_C


#if defined (MBEDTLS_SHA1_C)

#if defined(MBEDTLS_SHA1_PROCESS_ALT)

static DEFINE_SPINLOCK(sha2_lock);

void mbedtls_sha1_process(mbedtls_sha1_context *ctx, const unsigned char data[64])
{
    u8 tempbuf[96];

    u8 *p = ((u32)tempbuf % 32) ? (u8 *)((32 - (u32)tempbuf % 32) + (u32)tempbuf) : tempbuf;
    memcpy(p, data, 64);

    spin_lock(&sha2_lock);

    JL_SHA->RESULT7 = ctx->state[0];
    JL_SHA->RESULT6 = ctx->state[1];
    JL_SHA->RESULT5 = ctx->state[2];
    JL_SHA->RESULT4 = ctx->state[3];
    JL_SHA->RESULT3 = ctx->state[4];
    JL_SHA->RESULT2 = 0;
    JL_SHA->RESULT1 = 0;
    JL_SHA->RESULT0 = 0;

    JL_SHA->ADR = (unsigned int)p;

    JL_SHA->CON |= BIT(14);
    JL_SHA->CON |= BIT(3);

    if (ctx->sha1_is_start == 0) {
        JL_SHA->CON |= BIT(0);
        ctx->sha1_is_start = -1;
    } else {
        JL_SHA->CON |= BIT(1);
    }

    while ((JL_SHA->CON & BIT(15)) == 0);
    JL_SHA->CON |= BIT(14);

    ctx->state[0] = JL_SHA->RESULT7;
    ctx->state[1] = JL_SHA->RESULT6;
    ctx->state[2] = JL_SHA->RESULT5;
    ctx->state[3] = JL_SHA->RESULT4;
    ctx->state[4] = JL_SHA->RESULT3;

    spin_unlock(&sha2_lock);
}
#endif

#endif


#if defined (MBEDTLS_SHA256_C)

#if defined(MBEDTLS_SHA256_PROCESS_ALT)

#if !defined (MBEDTLS_SHA1_C) || !defined(MBEDTLS_SHA1_PROCESS_ALT)
static DEFINE_SPINLOCK(sha2_lock);
#endif

/*
static u8 sha_256_data[64] ALIGNE(32);
void mbedtls_sha256_clone_alt(mbedtls_sha256_context *ctx)
{
    SHA2ARES7 = ctx->state[0];
    SHA2ARES6 = ctx->state[1];
    SHA2ARES5 = ctx->state[2];
    SHA2ARES4 = ctx->state[3];
    SHA2ARES3 = ctx->state[4];
    SHA2ARES2 = ctx->state[5];
    SHA2ARES1 = ctx->state[6];
    SHA2ARES0 = ctx->state[7];
}
*/

void mbedtls_sha256_process(mbedtls_sha256_context *ctx, const unsigned char data[64])
{
    u8 tempbuf[96];

    u8 *p = ((u32)tempbuf % 32) ? (u8 *)((32 - (u32)tempbuf % 32) + (u32)tempbuf) : tempbuf;
    memcpy(p, data, 64);

    spin_lock(&sha2_lock);

    JL_SHA->RESULT7 = ctx->state[0];
    JL_SHA->RESULT6 = ctx->state[1];
    JL_SHA->RESULT5 = ctx->state[2];
    JL_SHA->RESULT4 = ctx->state[3];
    JL_SHA->RESULT3 = ctx->state[4];
    JL_SHA->RESULT2 = ctx->state[5];
    JL_SHA->RESULT1 = ctx->state[6];
    JL_SHA->RESULT0 = ctx->state[7];

    JL_SHA->ADR = (unsigned int)p;

    JL_SHA->CON |= BIT(14);
    JL_SHA->CON &= ~BIT(3);

    if (ctx->sha256_is_start == 0) {
        JL_SHA->CON |= BIT(0);
        ctx->sha256_is_start = -1;
    } else {
        JL_SHA->CON |= BIT(1);
    }

    while ((JL_SHA->CON & BIT(15)) == 0);
    JL_SHA->CON |= BIT(14);

    ctx->state[0] = JL_SHA->RESULT7;
    ctx->state[1] = JL_SHA->RESULT6;
    ctx->state[2] = JL_SHA->RESULT5;
    ctx->state[3] = JL_SHA->RESULT4;
    ctx->state[4] = JL_SHA->RESULT3;
    ctx->state[5] = JL_SHA->RESULT2;
    ctx->state[6] = JL_SHA->RESULT1;
    ctx->state[7] = JL_SHA->RESULT0;

    spin_unlock(&sha2_lock);
}
#endif

#endif




#if defined (MBEDTLS_SSL_EXPORT_KEYS)
#include "mbedtls/ssl.h"

#if defined (MBEDTLS_SSL_EXPORT_KEYS_BY_SPECUART)
extern int spec_uart_init(void);
extern int spec_uart_recv(char *buf, u32 len);
extern int spec_uart_send(char *buf, u32 len);

static u8 spec_uart_init_flag = 0;
#endif

/**********使用方法**********/
//mbedtls_ssl_config conf;
//mbedtls_ssl_config_init(&conf);
//mbedtls_ssl_conf_export_keys_cb(&conf, mbedtls_ssl_export_keys, (void *)(&conf));
/***************************/

int mbedtls_ssl_export_keys(void *priv,
                            const unsigned char *ms,
                            const unsigned char *kb,
                            size_t maclen,
                            size_t keylen,
                            size_t ivlen)
{
    struct mbedtls_ssl_config *conf = (struct mbedtls_ssl_config *)priv;
    u16 kb_len =  2 * maclen + 2 * keylen + 2 * ivlen;
    u8 *src = NULL;
    u8 *dst = NULL;

    if (priv == NULL) {
        return -1;
    }

#if defined (MBEDTLS_SSL_EXPORT_KEYS_BY_SPECUART)
    if (!spec_uart_init_flag) {
        spec_uart_init_flag = 1;
        spec_uart_init();
    }
#endif

    if (kb == NULL) {
        memset(conf->key_parse_buf, 0, sizeof(conf->key_parse_buf));
        strcat((char *)conf->key_parse_buf, "CLIENT_RANDOM ");
        src = (u8 *)ms;
        dst = conf->key_parse_buf + strlen("CLIENT_RANDOM ");
        for (int i = 0; i < 32; i++) {
            *(dst++) = *((char *)"0123456789abcdef" + (*src >> 4));
            *(dst++) = *((char *)"0123456789abcdef" + (*src++ & 0x0f));
        }
        *dst = ' ';
    } else {
        src = (u8 *)ms;
        dst = conf->key_parse_buf + strlen((const char *)conf->key_parse_buf);
        for (int i = 0; i < 48; i++) {
            *(dst++) = *((char *)"0123456789abcdef" + (*src >> 4));
            *(dst++) = *((char *)"0123456789abcdef" + (*src++ & 0x0f));
        }
        *(dst++) = '\r';
        *(dst++) = '\n';
        *dst = 0;
#if defined (MBEDTLS_SSL_EXPORT_KEYS_BY_SPECUART)
        spec_uart_send((char *)(conf->key_parse_buf), strlen((const char *)conf->key_parse_buf));
#endif
        puts(">>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
        printf("%s\n", conf->key_parse_buf);
        puts("<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
    }

    return 0;
}

#endif

