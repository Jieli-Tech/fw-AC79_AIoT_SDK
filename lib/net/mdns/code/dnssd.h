#ifndef _DNSSD_H_
#define _DNSSD_H_

#include "typedef.h"
#include <stdint.h>

#define _DNS_SD_H 3331000
#define kDNSServiceProperty_DaemonVersion "DaemonVersion"
#define MAX_SERVNAME 256

#define kDNSServiceMaxServiceName 64

/* Maximum length, in bytes, of a domain name represented as an *escaped* C-String */
/* including the final trailing dot, and the C-String terminating NULL at the end. */

#define kDNSServiceMaxDomainName 1009

#define DNSSD_ERROR_NOERROR       0
#define DNSSD_ERROR_HWADDRLEN     1
#define DNSSD_ERROR_OUTOFMEM      2
#define DNSSD_ERROR_LIBNOTFOUND   3
#define DNSSD_ERROR_PROCNOTFOUND  4

#define kDNSServiceInterfaceIndexAny 0
#define kDNSServiceInterfaceIndexLocalOnly ((uint32_t)-1)
#define kDNSServiceInterfaceIndexUnicast   ((uint32_t)-2)
#define kDNSServiceInterfaceIndexP2P       ((uint32_t)-3)
/*****************************
        Global Variables
*****************************/
//typedef  long long int64_t;
//typedef unsigned long long uint64_t;
//typedef unsigned int uint32_t,u_int;
//typedef int int32_t;
//typedef unsigned short uint16_t;
//typedef signed short int16_t;
//typedef unsigned char uint8_t,u_char;

typedef uint32_t DNSServiceFlags;
typedef uint32_t DNSServiceProtocol;
typedef int32_t  DNSServiceErrorType;

typedef struct dnssd_s dnssd_t;
typedef union _TXTRecordRef_t {
    char PrivateData[16];
    char *ForceNaturalAlignment;
} TXTRecordRef;
typedef struct _DNSServiceRef_t {
    int dref;
    void *ss;
} *DNSServiceRef;
typedef struct _DNSRecordRef_t {
    int dnsref;
    void *yy;
} *DNSRecordRef;

typedef void (*DNSServiceBrowseReply)
(
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *serviceName,
    const char                          *regtype,
    const char                          *replyDomain,
    void                                *context
);

typedef void (*DNSServiceRegisterReply)
(
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    DNSServiceErrorType                 errorCode,
    const char                          *name,
    const char                          *regtype,
    const char                          *domain,
    void                                *context
);
typedef void (*DNSServiceResolveReply)
(
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *fullname,
    const char                          *hosttarget,
    uint16_t                            port,        /* In network byte order */
    uint16_t                            txtLen,
    const unsigned char                 *txtRecord,
    void                                *context
);
typedef void (*DNSServiceQueryRecordReply)
(
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *fullname,
    uint16_t                            rrtype,
    uint16_t                            rrclass,
    uint16_t                            rdlen,
    const void                          *rdata,
    uint32_t                            ttl,
    void                                *context
);

typedef void (*DNSServiceDomainEnumReply)
(
    DNSServiceRef                       sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    DNSServiceErrorType                 errorCode,
    const char                          *replyDomain,
    void                                *context
);
typedef void (*DNSServiceRefDeallocate_t)(DNSServiceRef sdRef);
typedef DNSServiceErrorType(*DNSServiceRegister_t)
(
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *name,
    const char                          *regtype,
    const char                          *domain,
    const char                          *host,
    uint16_t                            port,
    uint16_t                            txtLen,
    const void                          *txtRecord,
    DNSServiceRegisterReply             callBack,
    void                                *context
);
//typedef void ( *DNSServiceGetAddrInfoReply)
//    (
//    DNSServiceRef                    sdRef,
//    DNSServiceFlags                  flags,
//    uint32_t                         interfaceIndex,
//    DNSServiceErrorType              errorCode,
//    const char                       *hostname,
//    const struct sockaddr            *address,
//    uint32_t                         ttl,
//    void                             *context
//    );
typedef void (*TXTRecordCreate_t)
(
    TXTRecordRef     *txtRecord,
    uint16_t         bufferLen,
    void             *buffer
);
typedef DNSServiceErrorType(*TXTRecordSetValue_t)
(
    TXTRecordRef     *txtRecord,
    const char       *key,
    uint8_t          valueSize,
    const void       *value
);
typedef void (*DNSServiceRegisterRecordReply)
(
    DNSServiceRef                       sdRef,
    DNSRecordRef                        RecordRef,
    DNSServiceFlags                     flags,
    DNSServiceErrorType                 errorCode,
    void                                *context
);
typedef void (*DNSServiceNATPortMappingReply)
(
    DNSServiceRef                    sdRef,
    DNSServiceFlags                  flags,
    uint32_t                         interfaceIndex,
    DNSServiceErrorType              errorCode,
    uint32_t                         externalAddress,   /* four byte IPv4 address in network byte order */
    DNSServiceProtocol               protocol,
    uint16_t                         internalPort,      /* In network byte order */
    uint16_t                         externalPort,      /* In network byte order and may be different than the requested port */
    uint32_t                         ttl,               /* may be different than the requested ttl */
    void                             *context
);
typedef uint16_t (*TXTRecordGetLength_t)(const TXTRecordRef *txtRecord);
typedef const void *(*TXTRecordGetBytesPtr_t)(const TXTRecordRef *txtRecord);
typedef void (*TXTRecordDeallocate_t)(TXTRecordRef *txtRecord);
struct dnssd_s {
    DNSServiceRegister_t       DNSServiceRegister;
    DNSServiceRefDeallocate_t  DNSServiceRefDeallocate;
    TXTRecordCreate_t          TXTRecordCreate;
    TXTRecordSetValue_t        TXTRecordSetValue;
    TXTRecordGetLength_t       TXTRecordGetLength;
    TXTRecordGetBytesPtr_t     TXTRecordGetBytesPtr;
    TXTRecordDeallocate_t      TXTRecordDeallocate;

    DNSServiceRef raopService;
    DNSServiceRef airplayService;
};

/* possible error code values */
enum {
    kDNSServiceErr_NoError                   = 0,
    kDNSServiceErr_Unknown                   = -65537,  /* 0xFFFE FFFF */
    kDNSServiceErr_NoSuchName                = -65538,
    kDNSServiceErr_NoMemory                  = -65539,
    kDNSServiceErr_BadParam                  = -65540,
    kDNSServiceErr_BadReference              = -65541,
    kDNSServiceErr_BadState                  = -65542,
    kDNSServiceErr_BadFlags                  = -65543,
    kDNSServiceErr_Unsupported               = -65544,
    kDNSServiceErr_NotInitialized            = -65545,
    kDNSServiceErr_AlreadyRegistered         = -65547,
    kDNSServiceErr_NameConflict              = -65548,
    kDNSServiceErr_Invalid                   = -65549,
    kDNSServiceErr_Firewall                  = -65550,
    kDNSServiceErr_Incompatible              = -65551,  /* client library incompatible with daemon */
    kDNSServiceErr_BadInterfaceIndex         = -65552,
    kDNSServiceErr_Refused                   = -65553,
    kDNSServiceErr_NoSuchRecord              = -65554,
    kDNSServiceErr_NoAuth                    = -65555,
    kDNSServiceErr_NoSuchKey                 = -65556,
    kDNSServiceErr_NATTraversal              = -65557,
    kDNSServiceErr_DoubleNAT                 = -65558,
    kDNSServiceErr_BadTime                   = -65559,  /* Codes up to here existed in Tiger */
    kDNSServiceErr_BadSig                    = -65560,
    kDNSServiceErr_BadKey                    = -65561,
    kDNSServiceErr_Transient                 = -65562,
    kDNSServiceErr_ServiceNotRunning         = -65563,  /* Background daemon not running */
    kDNSServiceErr_NATPortMappingUnsupported = -65564,  /* NAT doesn't support NAT-PMP or UPnP */
    kDNSServiceErr_NATPortMappingDisabled    = -65565,  /* NAT supports NAT-PMP or UPnP but it's disabled by the administrator */
    kDNSServiceErr_NoRouter                  = -65566,  /* No router currently configured (probably no network connectivity) */
    kDNSServiceErr_PollingMode               = -65567,
    kDNSServiceErr_Timeout                   = -65568

            /* mDNS Error codes are in the range
             * FFFE FF00 (-65792) to FFFE FFFF (-65537) */
};
enum {
    kDNSServiceFlagsMoreComing          = 0x1,
    kDNSServiceFlagsAdd                 = 0x2,
    kDNSServiceFlagsDefault             = 0x4,
    kDNSServiceFlagsNoAutoRename        = 0x8,
    kDNSServiceFlagsShared              = 0x10,
    kDNSServiceFlagsUnique              = 0x20,
    kDNSServiceFlagsBrowseDomains       = 0x40,
    kDNSServiceFlagsRegistrationDomains = 0x80,
    kDNSServiceFlagsLongLivedQuery      = 0x100,
    kDNSServiceFlagsAllowRemoteQuery    = 0x200,
    kDNSServiceFlagsForceMulticast      = 0x400,
    kDNSServiceFlagsForce               = 0x800,
    kDNSServiceFlagsReturnIntermediates = 0x1000,
    kDNSServiceFlagsNonBrowsable        = 0x2000,
    kDNSServiceFlagsShareConnection     = 0x4000,
    kDNSServiceFlagsSuppressUnusable    = 0x8000,
    kDNSServiceFlagsTimeout            = 0x10000,
    kDNSServiceFlagsIncludeP2P          = 0x20000,
    kDNSServiceFlagsWakeOnResolve      = 0x40000
};
/* Possible protocols for DNSServiceNATPortMappingCreate(). */
enum {
    kDNSServiceProtocol_IPv4 = 0x01,
    kDNSServiceProtocol_IPv6 = 0x02,
    /* 0x04 and 0x08 reserved for future internetwork protocols */

    kDNSServiceProtocol_UDP  = 0x10,
    kDNSServiceProtocol_TCP  = 0x20
};

enum {
    kDNSServiceClass_IN       = 1       /* Internet */
};

enum {
    kDNSServiceType_A          = 1,      /* Host address. */
    kDNSServiceType_NS         = 2,      /* Authoritative server. */
    kDNSServiceType_MD         = 3,      /* Mail destination. */
    kDNSServiceType_MF         = 4,      /* Mail forwarder. */
    kDNSServiceType_CNAME      = 5,      /* Canonical name. */
    kDNSServiceType_SOA        = 6,      /* Start of authority zone. */
    kDNSServiceType_MB         = 7,      /* Mailbox domain name. */
    kDNSServiceType_MG         = 8,      /* Mail group member. */
    kDNSServiceType_MR         = 9,      /* Mail rename name. */
    kDNSServiceType_NULL       = 10,     /* Null resource record. */
    kDNSServiceType_WKS        = 11,     /* Well known service. */
    kDNSServiceType_PTR        = 12,     /* Domain name pointer. */
    kDNSServiceType_HINFO      = 13,     /* Host information. */
    kDNSServiceType_MINFO      = 14,     /* Mailbox information. */
    kDNSServiceType_MX         = 15,     /* Mail routing information. */
    kDNSServiceType_TXT        = 16,     /* One or more text strings (NOT "zero or more..."). */
    kDNSServiceType_RP         = 17,     /* Responsible person. */
    kDNSServiceType_AFSDB      = 18,     /* AFS cell database. */
    kDNSServiceType_X25        = 19,     /* X_25 calling address. */
    kDNSServiceType_ISDN       = 20,     /* ISDN calling address. */
    kDNSServiceType_RT         = 21,     /* Router. */
    kDNSServiceType_NSAP       = 22,     /* NSAP address. */
    kDNSServiceType_NSAP_PTR   = 23,     /* Reverse NSAP lookup (deprecated). */
    kDNSServiceType_SIG        = 24,     /* Security signature. */
    kDNSServiceType_KEY        = 25,     /* Security key. */
    kDNSServiceType_PX         = 26,     /* X.400 mail mapping. */
    kDNSServiceType_GPOS       = 27,     /* Geographical position (withdrawn). */
    kDNSServiceType_AAAA       = 28,     /* IPv6 Address. */
    kDNSServiceType_LOC        = 29,     /* Location Information. */
    kDNSServiceType_NXT        = 30,     /* Next domain (security). */
    kDNSServiceType_EID        = 31,     /* Endpoint identifier. */
    kDNSServiceType_NIMLOC     = 32,     /* Nimrod Locator. */
    kDNSServiceType_SRV        = 33,     /* Server Selection. */
    kDNSServiceType_ATMA       = 34,     /* ATM Address */
    kDNSServiceType_NAPTR      = 35,     /* Naming Authority PoinTeR */
    kDNSServiceType_KX         = 36,     /* Key Exchange */
    kDNSServiceType_CERT       = 37,     /* Certification record */
    kDNSServiceType_A6         = 38,     /* IPv6 Address (deprecated) */
    kDNSServiceType_DNAME      = 39,     /* Non-terminal DNAME (for IPv6) */
    kDNSServiceType_SINK       = 40,     /* Kitchen sink (experimental) */
    kDNSServiceType_OPT        = 41,     /* EDNS0 option (meta-RR) */
    kDNSServiceType_APL        = 42,     /* Address Prefix List */
    kDNSServiceType_DS         = 43,     /* Delegation Signer */
    kDNSServiceType_SSHFP      = 44,     /* SSH Key Fingerprint */
    kDNSServiceType_IPSECKEY   = 45,     /* IPSECKEY */
    kDNSServiceType_RRSIG      = 46,     /* RRSIG */
    kDNSServiceType_NSEC       = 47,     /* Denial of Existence */
    kDNSServiceType_DNSKEY     = 48,     /* DNSKEY */
    kDNSServiceType_DHCID      = 49,     /* DHCP Client Identifier */
    kDNSServiceType_NSEC3      = 50,     /* Hashed Authenticated Denial of Existence */
    kDNSServiceType_NSEC3PARAM = 51,     /* Hashed Authenticated Denial of Existence */

    kDNSServiceType_HIP        = 55,     /* Host Identity Protocol */

    kDNSServiceType_SPF        = 99,     /* Sender Policy Framework for E-Mail */
    kDNSServiceType_UINFO      = 100,    /* IANA-Reserved */
    kDNSServiceType_UID        = 101,    /* IANA-Reserved */
    kDNSServiceType_GID        = 102,    /* IANA-Reserved */
    kDNSServiceType_UNSPEC     = 103,    /* IANA-Reserved */

    kDNSServiceType_TKEY       = 249,    /* Transaction key */
    kDNSServiceType_TSIG       = 250,    /* Transaction signature. */
    kDNSServiceType_IXFR       = 251,    /* Incremental zone transfer. */
    kDNSServiceType_AXFR       = 252,    /* Transfer zone of authority. */
    kDNSServiceType_MAILB      = 253,    /* Transfer mailbox records. */
    kDNSServiceType_MAILA      = 254,    /* Transfer mail agent records. */
    kDNSServiceType_ANY        = 255     /* Wildcard match. */
};

/*****************************
        Function Declare
*****************************/
dnssd_t *dnssd_init(int *error);

int dnssd_register_raop(dnssd_t *dnssd, const char *name,
                        unsigned short port, const char *hwaddr,
                        int hwaddrlen, int password);

#endif
