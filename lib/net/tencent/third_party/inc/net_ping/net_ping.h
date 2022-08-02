#ifndef PING_H
#define PING_H

#ifdef __linux__
#include "sys/types.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

#else
#include "lwip/inet.h"
#include "lwip/ip_addr.h"
#endif


#ifdef __cplusplus
extern "C" {//}
#endif


#ifdef __linux__

typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   long    u32_t;
typedef signed     long    s32_t;

struct ip4_addr {
    u32_t addr;
};
typedef struct ip4_addr ip4_addr_t;

struct ip6_addr {
    u32_t addr[4];
};
typedef struct ip6_addr ip6_addr_t;


typedef struct _ip_addr {
    union {
        ip6_addr_t ip6;
        ip4_addr_t ip4;
    } u_addr;
    u8_t type;
} ip_addr_t;

struct net_ping_data {
    ip4_addr_t sin_addr;
    u32_t count;                /* number of ping */
    u32_t data_long;          /* the ping packet data long */
};


#else

struct net_ping_data {
    ip_addr_t sin_addr;
    u32_t count;                /* number of ping */
    u32_t data_long;          /* the ping packet data long */
};

#endif
s32_t net_ping(struct net_ping_data *data);

#ifdef __cplusplus
}
#endif

#endif
