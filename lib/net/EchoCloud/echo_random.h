#ifndef ECHO_RANDM_H
#define ECHO_RANDM_H

#include <stdint.h>

#define MD5_SUPPORT	1
#define USE_CPU_RANDOM

/***********************
*** PUBLIC FUNCTIONS ***
***********************/
/*
 * Initialize the random number generator.
 */
void avRandomInit(void);

/*
 * Churn the randomness pool on a random event.  Call this early and often
 * on random and semi-random system events to build randomness in time for
 * usage.  For randomly timed events, pass a null pointer and a zero length
 * and this will use the system timer and other sources to add randomness.
 * If new random data is available, pass a pointer to that and it will be
 * included.
 */
void avChurnRand(char *randData, uint32_t randLen);

/*
 * Randomize our random seed value.  To be called for truely random events
 * such as user operations and network traffic.
 */
#if MD5_SUPPORT
#define avRandomize() avChurnRand(NULL, 0)
#else  /* MD5_SUPPORT */
void avRandomize(void);
#endif /* MD5_SUPPORT */

/*
 * Use the random pool to generate random data.  This degrades to pseudo
 * random when used faster than randomness is supplied using churnRand().
 * Thus it's important to make sure that the results of this are not
 * published directly because one could predict the next result to at
 * least some degree.  Also, it's important to get a good seed before
 * the first use.
 */
void avGenRand(char *buf, uint32_t bufLen);

/*
 * Return a new random number.
 */
uint32_t avRandom(void);

int echo_cloud_get_hmac_sha256(const char *device_id, const char *channel_uuid, const char *nonce, char *signature, const char *hash_key);

#endif /* ECHO_RANDM_H */

