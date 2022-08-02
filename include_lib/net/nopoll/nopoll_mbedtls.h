#ifndef __NOPOLL_MBEDDTLS_H__
#define __NOPOLL_MBEDDTLS_H__
#include <nopoll.h>

#include <mbedtls/debug.h>
#include <mbedtls/net.h>
#include <mbedtls/ssl.h>
#include <mbedtls/sha1.h>
#include <mbedtls/base64.h>
#include <mbedtls/x509_crt.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#ifndef EVP_MAX_MD_SIZE
#define EVP_MAX_MD_SIZE 20
#endif

struct nopoll_ssl {
    mbedtls_x509_crt ca_cert;
    mbedtls_x509_crt own_cert;
    mbedtls_pk_context pkey;
    mbedtls_ssl_context ssl;
    mbedtls_ssl_config conf;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    int fd;
};

unsigned long ERR_get_error(void);

void ERR_error_string_n(unsigned long e, char *buf, unsigned long len);

#endif // __NOPOLL_MBEDDTLS_H__
