#ifndef __AES_HW_H_
#define __AES_HW_H_

//input
//onput
//key
//keylen 16   32
void jl_aes_encrypt_hw(const unsigned char input[16], unsigned char output[16],
                       unsigned char *key, unsigned short keylen);


void jl_aes_decrypt_hw(const unsigned char input[16], unsigned char output[16],
                       unsigned char *key, unsigned short keylen);
#endif
