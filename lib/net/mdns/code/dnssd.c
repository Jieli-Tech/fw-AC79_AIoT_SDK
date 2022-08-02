#include "dnssd_clientlib.h"
#include "dnssd_int.h"

extern int utils_hwaddr(char *str, int strlen, const char *hwaddr, int hwaddrlen);

uint16_t  TXTRecordGetLength(const TXTRecordRef *txtRecord)
{
    return (txtRec->datalen);
}

const void *TXTRecordGetBytesPtr(const TXTRecordRef *txtRecord)
{
    return (txtRec->buffer);
}

dnssd_t *dnssd_init(int *error)
{
    dnssd_t *dnssd;
    if (error) {
        *error = DNSSD_ERROR_NOERROR;
    }
    dnssd = calloc(1, sizeof(dnssd_t));
    if (!dnssd) {
        if (error) {
            *error = DNSSD_ERROR_OUTOFMEM;
        }
        return NULL;
    }
    dnssd->DNSServiceRegister = &DNSServiceRegister;
    dnssd->DNSServiceRefDeallocate = &DNSServiceRefDeallocate;
    dnssd->TXTRecordCreate = &TXTRecordCreate;
    dnssd->TXTRecordSetValue = &TXTRecordSetValue;
    dnssd->TXTRecordGetLength = &TXTRecordGetLength;
    dnssd->TXTRecordGetBytesPtr = &TXTRecordGetBytesPtr;
    dnssd->TXTRecordDeallocate = &TXTRecordDeallocate;
    return dnssd;
}

int dnssd_register_raop(dnssd_t *dnssd, const char *name,
                        unsigned short port, const char *hwaddr,
                        int hwaddrlen, int password)
{
    TXTRecordRef txtRecord;
    char servname[MAX_SERVNAME];
    int ret;

    dnssd->TXTRecordCreate(&txtRecord, 0, NULL);
    dnssd->TXTRecordSetValue(&txtRecord, "txtvers", strlen(RAOP_TXTVERS), RAOP_TXTVERS);
    dnssd->TXTRecordSetValue(&txtRecord, "ch", strlen(RAOP_CH), RAOP_CH);
    dnssd->TXTRecordSetValue(&txtRecord, "cn", strlen(RAOP_CN), RAOP_CN);
    dnssd->TXTRecordSetValue(&txtRecord, "et", strlen(RAOP_ET), RAOP_ET);
    dnssd->TXTRecordSetValue(&txtRecord, "sv", strlen(RAOP_SV), RAOP_SV);
    dnssd->TXTRecordSetValue(&txtRecord, "da", strlen(RAOP_DA), RAOP_DA);
    dnssd->TXTRecordSetValue(&txtRecord, "sr", strlen(RAOP_SR), RAOP_SR);
    dnssd->TXTRecordSetValue(&txtRecord, "ss", strlen(RAOP_SS), RAOP_SS);
    if (password) {
        dnssd->TXTRecordSetValue(&txtRecord, "pw", strlen("true"), "true");
    } else {
        dnssd->TXTRecordSetValue(&txtRecord, "pw", strlen("false"), "false");
    }
    dnssd->TXTRecordSetValue(&txtRecord, "vn", strlen(RAOP_VN), RAOP_VN);
    dnssd->TXTRecordSetValue(&txtRecord, "tp", strlen(RAOP_TP), RAOP_TP);
    dnssd->TXTRecordSetValue(&txtRecord, "md", strlen(RAOP_MD), RAOP_MD);
    dnssd->TXTRecordSetValue(&txtRecord, "vs", strlen(GLOBAL_VERSION), GLOBAL_VERSION);
    dnssd->TXTRecordSetValue(&txtRecord, "sm", strlen(RAOP_SM), RAOP_SM);
    dnssd->TXTRecordSetValue(&txtRecord, "ek", strlen(RAOP_EK), RAOP_EK);

    /* Convert hardware address to string */
    ret = utils_hwaddr(servname, sizeof(servname), hwaddr, hwaddrlen);
    if (ret < 0) {
        /* FIXME: handle better */
        return -1;
    }

    /* Check that we have bytes for 'hw@name' format */
    if (sizeof(servname) < strlen(servname) + 1 + strlen(name) + 1) {
        /* FIXME: handle better */
        return -2;
    }

    strncat(servname, "@", sizeof(servname) - strlen(servname) - 1);
    strncat(servname, name, sizeof(servname) - strlen(servname) - 1);

    /* Register the service */
    dnssd->DNSServiceRegister(&dnssd->raopService, 0, 0,
                              servname, "_raop._tcp",
                              NULL, NULL,
                              htons(port),
                              dnssd->TXTRecordGetLength(&txtRecord),
                              dnssd->TXTRecordGetBytesPtr(&txtRecord),
                              NULL, NULL);

    /* Deallocate TXT record */
    dnssd->TXTRecordDeallocate(&txtRecord);
    return 0;
}
