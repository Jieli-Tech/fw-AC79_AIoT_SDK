#include "asm/includes.h"
#include "system/includes.h"
#include "app_config.h"


#ifdef USE_CRYPTO_TEST_DEMO

static void cmain(void *priv)
{


    puts("aes 128 test\n");

    char in[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
                };
    char enc_out[16];

    char dec_out[16];
    char key[16] = {0x12, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11,
                    0x11, 0x13, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11
                   };

    //aes编码
    jl_aes_encrypt_hw(in, enc_out, key, 16); // 16: 128  32 256
    put_buf(enc_out, 16);

    //aes解码
    jl_aes_decrypt_hw(enc_out, dec_out, key, 16);
    put_buf(dec_out, 16);





    char sha1_out[20];

    char sha256_out[32];

    printf("sha1 test\n");
    jl_sha1("ZhuHai JieLi Technology Co.,Ltd", 31, sha1_out);
    put_buf(sha1_out, 20);



    puts("sha224 test\n");
    jl_sha256("ZhuHai JieLi Technology Co.,Ltd", 31, sha256_out, 1);
    put_buf(sha256_out, 28);



    puts("sha256 test\n");
    jl_sha256("ZhuHai JieLi Technology Co.,Ltd", 31, sha256_out, 0);
    put_buf(sha256_out, 32);



}
late_initcall(cmain);
#endif
