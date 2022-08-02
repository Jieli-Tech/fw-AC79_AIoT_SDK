#include "echo_random.h"
#include "mbedtls/md_internal.h"
#include "mbedtls/md5.h"
#include <string.h>
#include "generic/printf.h"

extern unsigned int random32(int type);

#ifndef USE_CPU_RANDOM

#define LWIP_MAX(x , y)  (((x) > (y)) ? (x) : (y))
#define LWIP_MIN(x , y)  (((x) < (y)) ? (x) : (y))

#define RANDPOOLSZ 16   /* Bytes stored in the pool of randomness. */


/*****************************/
/*** LOCAL DATA STRUCTURES ***/
/*****************************/
static char randPool[RANDPOOLSZ];   /* Pool of randomness. */
static long randCount = 0;      /* Pseudo-random incrementer */

#endif

/***********************************/
/*** PUBLIC FUNCTION DEFINITIONS ***/
/***********************************/
/*
 * Initialize the random number generator.
 *
 * Since this is to be called on power up, we don't have much
 *  system randomess to work with.  Here all we use is the
 *  real-time clock.  We'll accumulate more randomness as soon
 *  as things start happening.
 */
void avRandomInit()
{
#ifndef USE_CPU_RANDOM
    avChurnRand(NULL, 0);
#endif
}

/*
 * Churn the randomness pool on a random event.  Call this early and often
 *  on random and semi-random system events to build randomness in time for
 *  usage.  For randomly timed events, pass a null pointer and a zero length
 *  and this will use the system timer and other sources to add randomness.
 *  If new random data is available, pass a pointer to that and it will be
 *  included.
 *
 */
void avChurnRand(char *randData, uint32_t randLen)
{
#ifndef USE_CPU_RANDOM
    mbedtls_md5_context md5;

    mbedtls_md5_init(&md5);
    mbedtls_md5_starts(&md5);
    mbedtls_md5_update(&md5, (uint8_t *)randPool, sizeof(randPool));

    if (randData) {
        mbedtls_md5_update(&md5, (uint8_t *)randData, randLen);
    } else {
        struct {
            /* INCLUDE fields for any system sources of randomness */
            char foobar;
        } sysData;

        /* Load sysData fields here. */
        mbedtls_md5_update(&md5, (uint8_t *)&sysData, sizeof(sysData));
    }

    mbedtls_md5_finish(&md5, (uint8_t *)randPool);
#endif
}

/*
 * Use the random pool to generate random data.  This degrades to pseudo
 *  random when used faster than randomness is supplied using churnRand().
 * Note: It's important that there be sufficient randomness in randPool
 *  before this is called for otherwise the range of the result may be
 *  narrow enough to make a search feasible.
 *
 * XXX Why does he not just call churnRand() for each block?  Probably
 *  so that you don't ever publish the seed which could possibly help
 *  predict future values.
 * XXX Why don't we preserve md5 between blocks and just update it with
 *  randCount each time?  Probably there is a weakness but I wish that
 *  it was documented.
 */
void avGenRand(char *buf, uint32_t bufLen)
{
#ifndef USE_CPU_RANDOM
    mbedtls_md5_context md5;
    uint8_t tmp[16];
    uint32_t n;

    while (bufLen > 0) {
        n = LWIP_MIN(bufLen, RANDPOOLSZ);
        mbedtls_md5_init(&md5);
        mbedtls_md5_starts(&md5);
        mbedtls_md5_update(&md5, (uint8_t *)randPool, sizeof(randPool));
        mbedtls_md5_update(&md5, (uint8_t *)&randCount, sizeof(randCount));
        mbedtls_md5_finish(&md5, tmp);
        randCount++;
        memcpy(buf, tmp, n);
        buf += n;
        bufLen -= n;
    }
#else
    const char *metachar = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

    memset(buf, 0, bufLen);
    for (uint32_t i = 0; i < bufLen; i++) {
        *(buf + i) = metachar[random32(0) % 62];
    }
#endif
}

/*
 * Return a new random number.
 */
uint32_t avRandom(void)
{
    uint32_t newRand;

#ifndef USE_CPU_RANDOM
    avGenRand((char *)&newRand, sizeof(newRand));
#else
    newRand = random32(0);
#endif

    return newRand;
}

int echo_cloud_get_hmac_sha256(const char *device_id, const char *channel_uuid, const char *nonce, char *signature, const char *hash_key)
{
    unsigned char output[32] = {0};
    char input[128 + 1] = {0};

    snprintf(input, sizeof(input), "%s%s%s", channel_uuid, device_id, nonce);

    if (0 != mbedtls_md_hmac(&mbedtls_sha256_info, (unsigned char *)hash_key, strlen(hash_key), (unsigned char *)input, strlen(input), output)) {
        return -1;
    }
    for (unsigned char i = 0; i < 32; i++) {
        sprintf(signature + i * 2, "%02x", (unsigned int)output[i]);
    }

    return 0;
}
