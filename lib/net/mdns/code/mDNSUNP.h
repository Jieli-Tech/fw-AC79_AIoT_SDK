#ifndef __mDNSUNP_h
#define __mDNSUNP_h

#include "lwip/sockets.h"
#include "dnssd.h"

#ifdef NOT_HAVE_SOCKLEN_T
typedef unsigned int socklen_t;
#endif

#define packedstruct struct
#define packedunion  union

#define SOCKET_GET_ERROR()      (errno)
#define SOCKET_SET_ERROR(value) (errno = (value))

#if !defined(_SS_MAXSIZE)
#if HAVE_IPV6
#define sockaddr_storage sockaddr_in6
#else
#define sockaddr_storage sockaddr
#endif // HAVE_IPV6
#endif // !defined(_SS_MAXSIZE)

#ifndef NOT_HAVE_SA_LEN
#define GET_SA_LEN(X) (sizeof(struct sockaddr) > ((struct sockaddr*)&(X))->sa_len ? \
                       sizeof(struct sockaddr) : ((struct sockaddr*)&(X))->sa_len   )
#elif HAVE_IPV6
#define GET_SA_LEN(X) (((struct sockaddr*)&(X))->sa_family == AF_INET  ? sizeof(struct sockaddr_in) : \
                       ((struct sockaddr*)&(X))->sa_family == AF_INET6 ? sizeof(struct sockaddr_in6) : sizeof(struct sockaddr))
#else
#define GET_SA_LEN(X) (((struct sockaddr*)&(X))->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr))
#endif

#define IFI_NAME    16          /* same as IFNAMSIZ in <net/if.h> */
#define IFI_HADDR    8          /* allow for 64-bit EUI-64 in future */

// Renamed from my_in_pktinfo because in_pktinfo is used by Linux.

struct my_in_pktinfo {
    struct sockaddr_storage ipi_addr;
    int                     ipi_ifindex;            /* received interface index */
    char                    ipi_ifname[IFI_NAME];   /* received interface name  */
};

struct ifi_info {
    char    ifi_name[IFI_NAME];   /* interface name, null terminated */
//  unsigned char  ifi_haddr[IFI_HADDR]; /* hardware address */
//  unsigned short ifi_hlen;             /* #bytes in hardware address: 0, 6, 8 */
//  short   ifi_flags;            /* IFF_xxx constants from <net/if.h> */
//  short   ifi_myflags;          /* our own IFI_xxx flags */
    int     ifi_index;            /* interface index */
    struct sockaddr  *ifi_addr;   /* primary address */
    struct sockaddr  *ifi_netmask;
//  struct sockaddr  *ifi_brdaddr;/* broadcast address */
//  struct sockaddr  *ifi_dstaddr;/* destination address */
//  struct ifi_info  *ifi_next;   /* next of these structures */
};

#if defined(AF_INET6) && HAVE_IPV6 && HAVE_LINUX
#define PROC_IFINET6_PATH "/proc/net/if_inet6"
extern struct ifi_info  *get_ifi_info_linuxv6(int family, int doaliases);
#endif

#if defined(AF_INET6) && HAVE_IPV6
#define INET6_ADDRSTRLEN 46 /*Maximum length of IPv6 address */
#endif



#define IFI_ALIAS   1           /* ifi_addr is an alias */

/* From the text (Stevens, section 16.6): */
/* 'Since many programs need to know all the interfaces on a system, we will develop a */
/* function of our own named get_ifi_info that returns a linked list of structures, one */
/* for each interface that is currently "up."' */
extern struct ifi_info  *get_ifi_info(int family, int doaliases);

/* 'The free_ifi_info function, which takes a pointer that was */
/* returned by get_ifi_info and frees all the dynamic memory.' */
extern void             free_ifi_info(struct ifi_info *);

#ifdef NOT_HAVE_DAEMON
extern int daemon(int nochdir, int noclose);
#endif

#endif
