#ifndef _ECDH_H_
#define _ECDH_H_

#include "bigint_impl.h"

#define ECDH_MEM_POOL_SIZE   0x750

typedef struct {
    bigint *x;
    bigint *y;
    bigint *z;
    int Z_is_one;
    int Z_is_zero;
} EC_POINT;

struct ECDH_CTX_st {
    BI_CTX ctx;
    EC_POINT G;
    EC_POINT PubKey;
    bigint *ECDHKey;
    bigint *PriKey;
};

typedef struct ECDH_CTX_st ECDH_CTX;
//In:Prikey 24byte
//Out:PubKeyx PubKeyy 24byte
void ecdh_Generate_PublicKey(ECDH_CTX *ecdh_ctx, const unsigned char *PriKey, unsigned char *PubKeyx, unsigned char *PubKeyy);
//In:PublicKeyBx PublicKeyBy 24byte
//Out:DHKey 24byte
void ecdh_Compute_DHKey(ECDH_CTX *ecdh_ctx, unsigned char *PublicKeyBx, unsigned char *PublicKeyBy, unsigned char *DHKey);

///由于计算和用于发送的buffer存在大小端相反问题，所以再封装一层函数
void ecdh_PublicKey(ECDH_CTX *ecdh_ctx, const unsigned char *PriKey, unsigned char *PubKeyx, unsigned char *PubKeyy);
void ecdh_DHKey(ECDH_CTX *ecdh_ctx, unsigned char *PublicKeyBx, unsigned char *PublicKeyBy, unsigned char *DHKey);


//ECDH_CTX *ecdh_init(void);
void ecdh_init(ECDH_CTX *ec_ctx, char *bigint_mem_pool);
void ecdh_free(ECDH_CTX *ecdh);

#endif
