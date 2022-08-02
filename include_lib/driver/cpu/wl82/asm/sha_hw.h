#ifndef __SHA_HW_H
#define __SHA_HW_H


void jl_sha1_process(unsigned int *is_start, unsigned int state[5], const unsigned char data[64]);


void jl_sha1(const unsigned char *input, size_t ilen, unsigned char output[20]);

void jl_sha256_process(unsigned int *is_start, unsigned int state[8], const unsigned char data[64]);

void jl_sha256(const unsigned char *input, size_t ilen, unsigned char output[32], int is224);


#endif

