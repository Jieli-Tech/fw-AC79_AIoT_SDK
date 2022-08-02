#ifndef __DNSSD_CLIENTLIB_H
#define __DNSSD_CLIENTLIB_H

#include "dnssd.h"
#include "mdnsembeddedapi.h"

#define txtRec ((TXTRecordRefRealType*)txtRecord)
#define MDNS_BUILDINGSTUBLIBRARY 0
//#define	LogMsg( ARGS... )
/*****************************
        Global Variables
*****************************/
typedef struct _TXTRecordRefRealType {
    uint8_t  *buffer;		// Pointer to data
    uint16_t buflen;		// Length of buffer
    uint16_t datalen;		// Length currently in use
    uint16_t malloced;	// Non-zero if buffer was allocated via malloc()
} TXTRecordRefRealType;

typedef struct mDNS_DirectOP_struct mDNS_DirectOP;

typedef void mDNS_DirectOP_Dispose(mDNS_DirectOP *op);

struct mDNS_DirectOP_struct {
    mDNS_DirectOP_Dispose  *disposefn;
};

typedef struct {
    mDNS_DirectOP_Dispose  *disposefn;
    DNSServiceRegisterReply callback;
    void                   *context;
    mDNSBool                autoname;		// Set if this name is tied to the Computer Name
    mDNSBool                autorename;		// Set if we just got a name conflict and now need to automatically pick a new name
    domainlabel             name;
    domainname              host;
    ServiceRecordSet        s;
} mDNS_DirectOP_Register;

typedef struct {
    mDNS_DirectOP_Dispose  *disposefn;
    DNSServiceBrowseReply   callback;
    void                   *context;
    DNSQuestion             q;
} mDNS_DirectOP_Browse;

typedef struct {
    mDNS_DirectOP_Dispose  *disposefn;
    DNSServiceResolveReply  callback;
    void                   *context;
    const ResourceRecord   *SRV;
    const ResourceRecord   *TXT;
    DNSQuestion             qSRV;
    DNSQuestion             qTXT;
} mDNS_DirectOP_Resolve;

typedef struct {
    mDNS_DirectOP_Dispose      *disposefn;
    DNSServiceQueryRecordReply  callback;
    void                       *context;
    DNSQuestion                 q;
} mDNS_DirectOP_QueryRecord;

/*****************************
        Function Declare
*****************************/
extern void  TXTRecordCreate(TXTRecordRef *txtRecord, uint16_t bufferLen, void *buffer);
extern DNSServiceErrorType TXTRecordSetValue(TXTRecordRef *txtRecord, const char *key, uint8_t valueSize, const void *value);
extern void  TXTRecordDeallocate(TXTRecordRef *txtRecord);
extern void DNSServiceRefDeallocate(DNSServiceRef sdRef);
extern DNSServiceErrorType DNSServiceRegister
(
    DNSServiceRef                       *sdRef,
    DNSServiceFlags                     flags,
    uint32_t                            interfaceIndex,
    const char                          *name,         /* may be NULL */
    const char                          *regtype,
    const char                          *domain,       /* may be NULL */
    const char                          *host,         /* may be NULL */
    uint16_t                            notAnIntPort,
    uint16_t                            txtLen,
    const void                          *txtRecord,    /* may be NULL */
    DNSServiceRegisterReply             callback,      /* may be NULL */
    void                                *context       /* may be NULL */
);


#endif
