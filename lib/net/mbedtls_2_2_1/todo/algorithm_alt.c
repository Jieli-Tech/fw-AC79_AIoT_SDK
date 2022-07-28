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
#include "asm/aes_hw.h"
#include "asm/sha_hw.h"


#if defined(MBEDTLS_AES_ENCRYPT_ALT)
void mbedtls_aes_encrypt_alt(mbedtls_aes_context *ctx,
                             const unsigned char input[16],
                             unsigned char output[16])
{
    jl_aes_encrypt_hw(input, output, ctx->aes_enc_key, ctx->nr == 14 ? 32 : 16);
}
#endif

#if defined(MBEDTLS_AES_DECRYPT_ALT)
void mbedtls_aes_decrypt_alt(mbedtls_aes_context *ctx,
                             const unsigned char input[16],
                             unsigned char output[16])
{

    jl_aes_decrypt_hw(input, output, ctx->aes_dec_key, ctx->nr == 14 ? 32 : 16);
}
#endif

#endif  //MBEDTLS_AES_C


#if defined (MBEDTLS_SHA1_C)

#if defined(MBEDTLS_SHA1_PROCESS_ALT)


void mbedtls_sha1_process(mbedtls_sha1_context *ctx, const unsigned char data[64])
{
    jl_sha1_process(&ctx->sha1_is_start, ctx->state, data);
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
    jl_sha256_process(&ctx->sha256_is_start, ctx->state, data);
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

