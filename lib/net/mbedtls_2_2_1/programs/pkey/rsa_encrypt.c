/*
 *  RSA simple data encryption program
 *
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#define mbedtls_fprintf    fprintf
#define mbedtls_printf     printf
#endif

#if defined(MBEDTLS_BIGNUM_C) && defined(MBEDTLS_RSA_C) && \
    defined(MBEDTLS_ENTROPY_C) && defined(MBEDTLS_FS_IO) && \
    defined(MBEDTLS_CTR_DRBG_C)
#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

#include <stdio.h>
#include <string.h>
#endif

#if !defined(MBEDTLS_BIGNUM_C) || !defined(MBEDTLS_RSA_C) ||  \
    !defined(MBEDTLS_ENTROPY_C) || !defined(MBEDTLS_FS_IO) || \
    !defined(MBEDTLS_CTR_DRBG_C)
int main(void)
{
    mbedtls_printf("MBEDTLS_BIGNUM_C and/or MBEDTLS_RSA_C and/or "
                   "MBEDTLS_ENTROPY_C and/or MBEDTLS_FS_IO and/or "
                   "MBEDTLS_CTR_DRBG_C not defined.\n");
    return (0);
}
#else
int main(int argc, char *argv[])
{
    FILE *f;
    int ret;
    size_t i;
    mbedtls_rsa_context rsa;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char input[1024];
    unsigned char buf[512];
    const char *pers = "rsa_encrypt";

    mbedtls_ctr_drbg_init(&ctr_drbg);
    ret = 1;

    if (argc != 2) {
        mbedtls_printf("usage: rsa_encrypt <string of max 100 characters>\n");

#if defined(_WIN32)
        mbedtls_printf("\n");
#endif

        goto exit;
    }

    mbedtls_printf("\n  . Seeding the random number generator...");
    fflush(stdout);

    mbedtls_entropy_init(&entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *) pers,
                                     strlen(pers))) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto exit;
    }

    mbedtls_printf("\n  . Reading public key from rsa_pub.txt");
    fflush(stdout);

    if ((f = fopen("rsa_pub.txt", "rb")) == NULL) {
        ret = 1;
        mbedtls_printf(" failed\n  ! Could not open rsa_pub.txt\n" \
                       "  ! Please run rsa_genkey first\n\n");
        goto exit;
    }

    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);

    if ((ret = mbedtls_mpi_read_file(&rsa.N, 16, f)) != 0 ||
        (ret = mbedtls_mpi_read_file(&rsa.E, 16, f)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_mpi_read_file returned %d\n\n", ret);
        fclose(f);
        goto exit;
    }

    rsa.len = (mbedtls_mpi_bitlen(&rsa.N) + 7) >> 3;

    fclose(f);

    if (strlen(argv[1]) > 100) {
        mbedtls_printf(" Input data larger than 100 characters.\n\n");
        goto exit;
    }

    memcpy(input, argv[1], strlen(argv[1]));

    /*
     * Calculate the RSA encryption of the hash.
     */
    mbedtls_printf("\n  . Generating the RSA encrypted value");
    fflush(stdout);

    if ((ret = mbedtls_rsa_pkcs1_encrypt(&rsa, mbedtls_ctr_drbg_random, &ctr_drbg,
                                         MBEDTLS_RSA_PUBLIC, strlen(argv[1]),
                                         input, buf)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_rsa_pkcs1_encrypt returned %d\n\n", ret);
        goto exit;
    }

    /*
     * Write the signature into result-enc.txt
     */
    if ((f = fopen("result-enc.txt", "wb+")) == NULL) {
        ret = 1;
        mbedtls_printf(" failed\n  ! Could not create %s\n\n", "result-enc.txt");
        goto exit;
    }

    for (i = 0; i < rsa.len; i++)
        mbedtls_fprintf(f, "%02X%s", buf[i],
                        (i + 1) % 16 == 0 ? "\r\n" : " ");

    fclose(f);

    mbedtls_printf("\n  . Done (created \"%s\")\n\n", "result-enc.txt");

exit:
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

#if defined(_WIN32)
    mbedtls_printf("  + Press Enter to exit this program.\n");
    fflush(stdout);
    getchar();
#endif

    return (ret);
}
#endif /* MBEDTLS_BIGNUM_C && MBEDTLS_RSA_C && MBEDTLS_ENTROPY_C &&
          MBEDTLS_FS_IO && MBEDTLS_CTR_DRBG_C */
