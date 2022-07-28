#include <stdlib.h>
#include <string.h>
#include "typedef.h"
#include "iot_rsa.h"
#include "mbedtls/rsa.h"
#include "mbedtls/bn_mul.h"
#include "mbedtls/bignum.h"
#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif

#define MBEDTLS_PKCS1_V15
#define KEY_LEN 128

#define RSA_N   "00b90cae86cff2a171f70750d0f9af"  \
                "106ab5b8643da0e0386b8009e59f0d"  \
                "533b9ec71312555c07669cb875f6ac"  \
                "13a3b96b239f5ff2b482e157b6a643"  \
                "42a691e1fbdfdf78abcf4cfee5e7f3"  \
                "73f72cdee09d8530109808dc573184"  \
                "750876f16b255a28e10913255f651b"  \
                "3816654329a78fe5d54af458027ef6"  \
                "52614d18a04693349d"

/*
#define RSA_N   "00b2e55e788ccae977e65bfd849772"  \
                "0936a72cf3ae3d86be9efcdd6c861c"  \
                "f4dddcb0bebfc4994836bfcfc5b129"  \
                "73ed78559b43b3b9ba37bf1938a59d"  \
                "874ba41eb2e94a698c6e32f1ba0106"  \
                "b3ecc51730baf48f7b18207707d1dc"  \
                "7b0122f2aab7c05f79b1b6b66af17b"  \
                "8f595f3f6fb3ec644a21d53d652973"  \
                "3ee4f607fdef0ba25b"
*/
#define RSA_E   "10001"

#define RSA_D  "75f9831276aebdc65f38dcf81ab38e"   \
                "55e76b628bce7154b61e459b72da17"  \
                "b553fb8edf82341f86537a1e8215b7"  \
                "28ac89afe9b6d54d73c3e74b0f14d2"  \
                "8b5481151ab36a211f0ee6e7e57dac"  \
                "73af8ac7469029dce26bb4d478167b"  \
                "ae62103837842915ecb8402fb5b7f4"  \
                "46c87fa12a4db4efcc6304fd4e75c3"  \
                "3d72806e754d81e1"

#define RSA_P   "00e35fdcf65351e9297fdeea71e3bee0cca44d1523027d203cacc398f8edc560f39b956509372d9db36e239ecbfb3dccc31a3bbadf6efd6fdc846bd3677f270811"

#define RSA_Q   "00c96b130ae4dd38139d68020d2fb226fe29665d48b8a4fb1e4cb96beed1610b12eadde2be77c846efa1e3c63ace684e21d83544ee06d4e1799cf105ec88bc4fab"

#define RSA_DP  "36e57b44e1c3020769ff191d9c3e06aa81f4b668b87e1b5d6adce2bf1f312b82458b2154c344b9318c22ff81024cde76308c414716d60bbef31dd171c88a54f1"

#define RSA_DQ  "6faacb5de8d0b4bc3b3264a0c6e6b0338ce451a775a7120a1463607180e79a6a1c8873a3416969da851870d83d831a7e2d0e2b6f039e967b0405a45124e5b20d"

#define RSA_QP  "289bb7bfc2073051b5e7c56966214b0cb23fb0cac77a24a505442d72fc8c231db2e5eb0afc25b830b1cd47aec6ec0e356bff4f5bda118a875c0f4d6c7bbcb84f"


#define RSA_PT "helloword"

#define PT_LEN  strlen(RSA_PT)


static int _rand(void *rng_state, unsigned char *output, size_t len)
{
    size_t i;

    if (rng_state != NULL) {
        rng_state = NULL;
    }

    for (i = 0; i < len; ++i) {
        output[i] = rand();
    }

    return (0);
}

int speech_rsa_encrypt(const char *in, int in_size, char *out)
{
    int ret = 0;
    int len = -1;
    mbedtls_rsa_context rsa;

    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);
    rsa.len = KEY_LEN;
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.N, 16, RSA_N));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.E, 16, RSA_E));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.D, 16, RSA_D));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.P, 16, RSA_P));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.Q, 16, RSA_Q));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.DP, 16, RSA_DP));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.DQ, 16, RSA_DQ));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.QP, 16, RSA_QP));

    if (mbedtls_rsa_check_pubkey(&rsa) != 0) {
        printf("speech rsa check pubkey fail\n");
        goto cleanup;
    }

    if (mbedtls_rsa_pkcs1_encrypt(&rsa, _rand, NULL, MBEDTLS_RSA_PUBLIC, in_size,
                                  (unsigned char *)in, (unsigned char *)out) != 0) {
        printf("speech rsa encrypt faild\n");
        goto cleanup;
    }

    len = KEY_LEN;

cleanup:
    mbedtls_rsa_free(&rsa);

    return len;
}

int speech_rsa_decrypt(const char *in, int in_size, char *out, int out_max_size)
{
    int ret = 0;
    int len = -1;
    mbedtls_rsa_context rsa;

    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);
    rsa.len = KEY_LEN;
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.N, 16, RSA_N));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.E, 16, RSA_E));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.D, 16, RSA_D));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.P, 16, RSA_P));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.Q, 16, RSA_Q));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.DP, 16, RSA_DP));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.DQ, 16, RSA_DQ));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.QP, 16, RSA_QP));
    if (mbedtls_rsa_check_privkey(&rsa) != 0) {
        printf("speech rsa check privkey fail\n");
        goto cleanup;
    }

    if (mbedtls_rsa_pkcs1_decrypt(&rsa, _rand, NULL, MBEDTLS_RSA_PRIVATE, (size_t *)&len,
                                  (unsigned char *)in, (unsigned char *)out, (size_t)out_max_size) != 0) {
        printf("speech rsa decrypt faild\n");
        goto cleanup;
    }

cleanup:
    mbedtls_rsa_free(&rsa);

    return len;
}

#if 0
int rsa_self_test(int verbose)
{
    int ret = 0;
    size_t len;
    mbedtls_rsa_context rsa;
    unsigned char rsa_plaintext[PT_LEN + 1];
    unsigned char rsa_decrypted[PT_LEN + 1];
    unsigned char rsa_ciphertext[KEY_LEN];

    mbedtls_rsa_init(&rsa, MBEDTLS_RSA_PKCS_V15, 0);

    rsa.len = KEY_LEN;
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.N, 16, RSA_N));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.E, 16, RSA_E));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.D, 16, RSA_D));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.P, 16, RSA_P));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.Q, 16, RSA_Q));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.DP, 16, RSA_DP));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.DQ, 16, RSA_DQ));
    MBEDTLS_MPI_CHK(mbedtls_mpi_read_string(&rsa.QP, 16, RSA_QP));


    if (verbose != 0) {
        printf("  RSA key validation: ");
    }

    if (mbedtls_rsa_check_pubkey(&rsa) != 0)
        //|| mbedtls_rsa_check_privkey( &rsa ) != 0 )
    {
        printf("[%s][%d]\n", __func__, __LINE__);
        if (verbose != 0) {
            printf("failed\n");
        }

        return (1);
    }


    if (verbose != 0) {
        printf("passed\n  PKCS#1 encryption : ");
    }

    memset(rsa_plaintext, 0, sizeof(rsa_plaintext));
    memset(rsa_decrypted, 0, sizeof(rsa_decrypted));
    memset(rsa_ciphertext, 0, sizeof(rsa_ciphertext));


    memcpy(rsa_plaintext, RSA_PT, PT_LEN);

    rsa_plaintext[PT_LEN] = '\0';

    if (mbedtls_rsa_pkcs1_encrypt(&rsa, _rand, NULL, MBEDTLS_RSA_PUBLIC, PT_LEN,
                                  rsa_plaintext, rsa_ciphertext) != 0) {
        if (verbose != 0) {
            printf("failed\n");
        }

        return (1);
    }

    printf("before encrypt\n%s\n", rsa_plaintext);

    printf("[%s][%d]\n", __func__, __LINE__);
    if (verbose != 0) {
        printf("passed\n  PKCS#1 decryption : ");
    }

    if (mbedtls_rsa_pkcs1_decrypt(&rsa, _rand, NULL, MBEDTLS_RSA_PRIVATE, &len,
                                  rsa_ciphertext, rsa_decrypted,
                                  sizeof(rsa_decrypted)) != 0) {
        if (verbose != 0) {
            printf("failed\n");
        }

        return (1);
    }

    rsa_decrypted[PT_LEN] = '\0';


    printf("\r\n");
    printf("after decrypt\n%s\n", rsa_decrypted);
    printf("\r\n");
    printf("[%s][%d]\n", __func__, __LINE__);
    if (memcmp(rsa_decrypted, rsa_plaintext, len) != 0) {
        if (verbose != 0) {
            printf("failed\n");
        }

        return (1);
    }

    printf("[%s][%d]\n", __func__, __LINE__);

    if (verbose != 0) {
        printf("passed\n");
    }

    if (verbose != 0) {
        printf("\n");
    }

cleanup:
    printf("[%s][%d]\n", __func__, __LINE__);

    mbedtls_rsa_free(&rsa);
    return (ret);
}
#endif

