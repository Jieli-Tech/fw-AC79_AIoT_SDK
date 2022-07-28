/**
* @file  tvs_authorizer.c
* @brief TVS SDK授权逻辑
* @date  2019-5-10
*/

#include "stdio.h"
#include "stdlib.h"
#include "time.h"
#include<string.h>
#include "sha256.h"
#include "base64.h"
#include "os_wrapper.h"
#include "tvs_authorize_infos.h"

#define TVS_SEED_LEN			32
#define TVS_AUTH_RESP_LEN		150

static char *g_codeVerifier = NULL;
static char *g_authRespInfo = NULL;
static char *g_sessionId = NULL;
static const char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

/*url-safe base64 encode replace '+' '/' to '-' '_' and no '=' pad*/
static int url_safe_b64encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen)
{
    int i;
    unsigned char *p = dst;

    if ((dlen < (slen * 4 / 3 + 1)) || (NULL == dst)) {
        *olen = 0;
        return -1;
    }

    for (i = 0; i < slen - 2; i += 3) {
        *p++ = basis_64[(src[i] >> 2) & 0x3F];
        *p++ = basis_64[((src[i] & 0x3) << 4) | ((int)(src[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((src[i + 1] & 0xF) << 2) | ((int)(src[i + 2] & 0xC0) >> 6)];
        *p++ = basis_64[src[i + 2] & 0x3F];
    }

    if (i < slen) {
        *p++ = basis_64[(src[i] >> 2) & 0x3F];
        if (i == (slen - 1)) {
            *p++ = basis_64[((src[i] & 0x3) << 4)];
            *p++ = '=';		//yii:添加补丁
        } else {
            *p++ = basis_64[((src[i] & 0x3) << 4) | ((int)(src[i + 1] & 0xF0) >> 4)];
            *p++ = basis_64[((src[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';				//yii:添加补丁
    }
    *olen = p - dst;
    *p++ = '\0';

    return 0;
}

int tvs_auth_req_infos_build(char *authReqInfo, int len)
{
    unsigned char seed_src[TVS_SEED_LEN];
    unsigned char seed_dst[TVS_SEED_LEN + TVS_SEED_LEN / 2];

    //生成32个随机字符
    srand((unsigned)time(NULL));
    for (int i = 0; i < sizeof(seed_src); i++) {
        seed_src[i] = rand();
    }
    //base64编码
    size_t rel_len = 0;

    // if(mbedtls_base64_encode(seed_dst,sizeof(seed_dst),&rel_len,seed_src,TVS_SEED_LEN) != 0){
    if (url_safe_b64encode(seed_dst, sizeof(seed_dst), &rel_len, seed_src, TVS_SEED_LEN) != 0) {
        printf("base64 encode error!!!!\n");
        return -1;
    }
    //保存codeVerifier
    if (g_codeVerifier != NULL) {
        TVS_FREE(g_codeVerifier);
    }
    g_codeVerifier = TVS_MALLOC(rel_len + 1);
    memcpy(g_codeVerifier, seed_dst, rel_len);
    g_codeVerifier[rel_len] = '\0';
    //sha256
    memset(seed_src, 0, sizeof(seed_src));
    mbedtls_sha256(seed_dst, rel_len, seed_src, 0);
    //base64编码
    rel_len = 0;
    memset(seed_dst, 0, sizeof(seed_dst));
    // if(mbedtls_base64_encode(seed_dst,sizeof(seed_dst),&rel_len,seed_src,TVS_SEED_LEN) != 0){
    if (url_safe_b64encode(seed_dst, sizeof(seed_dst), &rel_len, seed_src, TVS_SEED_LEN) != 0) {
        printf("base64 encode error!!!!\n");
        return -1;
    }
    seed_dst[rel_len] = '\0';
    //生成session id
    if (g_sessionId != NULL) {
        TVS_FREE(g_sessionId);
    }
    g_sessionId = TVS_MALLOC(TVS_SEED_LEN);
    for (int i = 0; i < TVS_SEED_LEN - 1; i++) {
        g_sessionId[i] = rand() % 10 + '0';
    }
    g_sessionId[TVS_SEED_LEN - 1] = '\0';
    //生成authReqInfos
    if (authReqInfo != NULL) {
        snprintf(authReqInfo, len, "{\"codeChallenge\":\"%s\",\"sessionId\":\"%s\"}", seed_dst, g_sessionId);
    }


    printf("build auth info,authReqInfos:%s\n", (authReqInfo == NULL ? "" : authReqInfo));
    printf("build auth info,codeVerifier:%s\n", g_codeVerifier);
    return 0;

}

char *tvs_auth_get_authCode()
{
    return g_authRespInfo;
}

char *tvs_auth_get_session_id()
{
    return g_sessionId;
}

int tvs_auth_set_authCode(const char *authCode, int size)
{
    if (authCode == NULL || size < 1 || size > 200) {
        printf("set authCode error:%p,%d", authCode, size);
        return -1;
    }
    if (g_authRespInfo != NULL) {
        TVS_FREE(g_authRespInfo);
        g_authRespInfo = NULL;
    }
    g_authRespInfo = TVS_MALLOC(size + 1);
    memcpy(g_authRespInfo, authCode, size);
    g_authRespInfo[size] = '\0';
    printf("set authCode success:%s\n", g_authRespInfo);
    return 0;
}

char *tvs_auth_get_codeVerifier()
{
    return g_codeVerifier;
}

