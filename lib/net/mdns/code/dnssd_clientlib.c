#include "dnssd_clientlib.h"
#include "string.h"
#include "malloc.h"

void  TXTRecordCreate(TXTRecordRef *txtRecord, uint16_t bufferLen, void *buffer)
{
    txtRec->buffer   = buffer;
    txtRec->buflen   = buffer ? bufferLen : (uint16_t)0;
    txtRec->datalen  = 0;
    txtRec->malloced = 0;
}

static uint8_t *InternalTXTRecordSearch
(
    uint16_t         txtLen,
    const void       *txtRecord,
    const char       *key,
    unsigned long    *keylen
)
{
    uint8_t *p = (uint8_t *)txtRecord;
    uint8_t *e = p + txtLen;
    *keylen = (unsigned long) strlen(key);
    while (p < e) {
        uint8_t *x = p;
        p += 1 + p[0];
        if (p <= e && *keylen <= x[0] && !strncasecmp(key, (char *)x + 1, *keylen))
            if (*keylen == x[0] || x[1 + *keylen] == '=') {
                return (x);
            }
    }
    return (NULL);
}

DNSServiceErrorType  TXTRecordRemoveValue(TXTRecordRef *txtRecord, const char *key)
{
    unsigned long keylen, itemlen, remainder;
    uint8_t *item = InternalTXTRecordSearch(txtRec->datalen, txtRec->buffer, key, &keylen);
    if (!item) {
        return (kDNSServiceErr_NoSuchKey);
    }
    itemlen   = (unsigned long)(1 + item[0]);
    remainder = (unsigned long)((txtRec->buffer + txtRec->datalen) - (item + itemlen));
    // Use memmove because memcpy behaviour is undefined for overlapping regions
    memmove(item, item + itemlen, remainder);
    txtRec->datalen -= itemlen;
    return (kDNSServiceErr_NoError);
}

DNSServiceErrorType TXTRecordSetValue(
    TXTRecordRef     *txtRecord,
    const char       *key,
    uint8_t          valueSize,
    const void       *value
)
{
    uint8_t *start, *p;
    const char *k;
    unsigned long keysize, keyvalsize;
    for (k = key; *k; k++) if (*k < 0x20 || *k > 0x7E || *k == '=') {
            return (kDNSServiceErr_Invalid);
        }
    keysize = (unsigned long)(k - key);
    keyvalsize = 1 + keysize + (value ? (1 + valueSize) : 0);
    if (keysize < 1 || keyvalsize > 255) {
        return (kDNSServiceErr_Invalid);
    }
    (void)TXTRecordRemoveValue(txtRecord, key);
    if (txtRec->datalen + keyvalsize > txtRec->buflen) {
        unsigned char *newbuf;
        unsigned long newlen = txtRec->datalen + keyvalsize;
        if (newlen > 0xFFFF) {
            return (kDNSServiceErr_Invalid);
        }

        newbuf = malloc((unsigned int)newlen);
        if (!newbuf) {
            return (kDNSServiceErr_NoMemory);
        }
        memcpy(newbuf, txtRec->buffer, txtRec->datalen);
        if (txtRec->malloced) {
            free(txtRec->buffer);
        }
        txtRec->buffer = newbuf;
        txtRec->buflen = (uint16_t)(newlen);
        txtRec->malloced = 1;
    }
    start = txtRec->buffer + txtRec->datalen;
    p = start + 1;
    memcpy(p, key, keysize);
    p += keysize;
    if (value) {
        *p++ = '=';
        memcpy(p, value, valueSize);
        p += valueSize;
    }
    *start = (uint8_t)(p - start - 1);
    txtRec->datalen += p - start;
    return (kDNSServiceErr_NoError);
}

void  TXTRecordDeallocate(TXTRecordRef *txtRecord)
{
    if (txtRec->malloced) {
        free(txtRec->buffer);
    }
}
