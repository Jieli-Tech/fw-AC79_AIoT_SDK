/* address.h -- representation of network addresses
 *
 * Copyright (C) 2010,2011 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use.
 */

/**
 * @file address.h
 * @brief representation of network addresses
 */

#ifndef _COAP_ADDRESS_H_
#define _COAP_ADDRESS_H_

#include "coap_config.h"
#include <assert.h>

#include <string.h>
//#include <stdint.h>

#include <lwip/sockets.h>
#include <lwip/netdb.h>

/** multi-purpose address abstraction */
typedef struct coap_address_t {
    socklen_t size;		/**< size of addr */
    union {
        struct sockaddr     sa;
        struct sockaddr_storage st;
        struct sockaddr_in  sin;
#if LWIP_IPV6
        struct sockaddr_in6 sin6;
#endif
    } addr;
} coap_address_t;

static inline int
_coap_address_equals_impl(const coap_address_t *a,
                          const coap_address_t *b)
{
    if (a->size != b->size || a->addr.sa.sa_family != b->addr.sa.sa_family) {
        return 0;
    }

    /* need to compare only relevant parts of sockaddr_in6 */
    switch (a->addr.sa.sa_family) {
    case AF_INET:
        return
            a->addr.sin.sin_port == b->addr.sin.sin_port &&
            memcmp(&a->addr.sin.sin_addr, &b->addr.sin.sin_addr,
                   sizeof(struct in_addr)) == 0;
#if LWIP_IPV6
    case AF_INET6:
        return a->addr.sin6.sin6_port == b->addr.sin6.sin6_port &&
               memcmp(&a->addr.sin6.sin6_addr, &b->addr.sin6.sin6_addr,
                      sizeof(struct in6_addr)) == 0;
#endif
    default: /* fall through and signal error */
        ;
    }
    return 0;
}

static inline int
_coap_is_mcast_impl(const coap_address_t *a)
{
    if (!a) {
        return 0;
    }

    switch (a->addr.sa.sa_family) {
    case AF_INET:
        return IN_MULTICAST(a->addr.sin.sin_addr.s_addr);
#if LWIP_IPV6
    case  AF_INET6:
#define IN6_IS_ADDR_MULTICAST(a) (((const uint8_t *) (a))[0] == 0xff)
        return IN6_IS_ADDR_MULTICAST(&a->addr.sin6.sin6_addr);
#endif
    default:			/* fall through and signal error */
        ;
    }
    return 0;
}

/**
 * Resets the given coap_address_t object @p addr to its default
 * values.  In particular, the member size must be initialized to the
 * available size for storing addresses.
 *
 * @param addr The coap_address_t object to initialize.
 */
static inline void
coap_address_init(coap_address_t *addr)
{
    assert(addr);
    memset(addr, 0, sizeof(coap_address_t));
    /* lwip has constandt address sizes and doesn't need the .size part */
    addr->size = sizeof(addr->addr);
}

/**
 * Compares given address objects @p a and @p b. This function returns
 * @c 1 if addresses are equal, @c 0 otherwise. The parameters @p a
 * and @p b must not be @c NULL;
 */
static inline int
coap_address_equals(const coap_address_t *a, const coap_address_t *b)
{
    assert(a);
    assert(b);
    return _coap_address_equals_impl(a, b);
}

/**
 * Checks if given address @p a denotes a multicast address.  This
 * function returns @c 1 if @p a is multicast, @c 0 otherwise.
 */
static inline int
coap_is_mcast(const coap_address_t *a)
{
    return a && _coap_is_mcast_impl(a);
}

#endif /* _COAP_ADDRESS_H_ */
