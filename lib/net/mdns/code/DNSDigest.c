#include "mdnsembeddedapi.h"
#include "DNSCommon.h"
#include "mbedtls/md5.h"

mDNSlocal mDNSu16 NToH16(mDNSu8 *bytes)
{
    return (mDNSu16)((mDNSu16)bytes[0] << 8 | (mDNSu16)bytes[1]);
}

mDNSlocal mDNSu32 NToH32(mDNSu8 *bytes)
{
    return (mDNSu32)((mDNSu32) bytes[0] << 24 | (mDNSu32) bytes[1] << 16 | (mDNSu32) bytes[2] << 8 | (mDNSu32)bytes[3]);
}


#define MD5_CTX 				mbedtls_md5_context
#define MD5_Init(x)				do {mbedtls_md5_init(x);mbedtls_md5_starts(x);} while(0)
#define MD5_Update(x, y, z) 	mbedtls_md5_update(x, y, z)
#define	MD5_Final(x, y)			mbedtls_md5_finish(y, x)


static const char Base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const char Pad64 = '=';

#define mDNSisspace(x) (x == '\t' || x == '\n' || x == '\v' || x == '\f' || x == '\r' || x == ' ')

mDNSlocal const char *mDNSstrchr(const char *s, int c)
{
    while (1) {
        if (c == *s) {
            return s;
        }
        if (!*s) {
            return mDNSNULL;
        }
        s++;
    }
}

// skips all whitespace anywhere.
// converts characters, four at a time, starting at (or after)
// src from base - 64 numbers into three 8 bit bytes in the target area.
// it returns the number of data bytes stored at the target, or -1 on error.
// adapted from BIND sources

mDNSlocal mDNSs32 DNSDigest_Base64ToBin(const char *src, mDNSu8 *target, mDNSu32 targsize)
{
    int tarindex, state, ch;
    const char *pos;

    state = 0;
    tarindex = 0;

    while ((ch = *src++) != '\0') {
        if (mDNSisspace(ch)) {	/* Skip whitespace anywhere. */
            continue;
        }

        if (ch == Pad64) {
            break;
        }

        pos = mDNSstrchr(Base64, ch);
        if (pos == 0) {	/* A non-base64 character. */
            return (-1);
        }

        switch (state) {
        case 0:
            if (target) {
                if ((mDNSu32)tarindex >= targsize) {
                    return (-1);
                }
                target[tarindex] = (mDNSu8)((pos - Base64) << 2);
            }
            state = 1;
            break;
        case 1:
            if (target) {
                if ((mDNSu32)tarindex + 1 >= targsize) {
                    return (-1);
                }
                target[tarindex]   |= (pos - Base64) >> 4;
                target[tarindex + 1]  = (mDNSu8)(((pos - Base64) & 0x0f) << 4);
            }
            tarindex++;
            state = 2;
            break;
        case 2:
            if (target) {
                if ((mDNSu32)tarindex + 1 >= targsize) {
                    return (-1);
                }
                target[tarindex]   |= (pos - Base64) >> 2;
                target[tarindex + 1]  = (mDNSu8)(((pos - Base64) & 0x03) << 6);
            }
            tarindex++;
            state = 3;
            break;
        case 3:
            if (target) {
                if ((mDNSu32)tarindex >= targsize) {
                    return (-1);
                }
                target[tarindex] |= (pos - Base64);
            }
            tarindex++;
            state = 0;
            break;
        default:
            return -1;
        }
    }

    /*
     * We are done decoding Base-64 chars.  Let's see if we ended
     * on a byte boundary, and/or with erroneous trailing characters.
     */

    if (ch == Pad64) {		/* We got a pad char. */
        ch = *src++;		/* Skip it, get next. */
        switch (state) {
        case 0:		/* Invalid = in first position */
        case 1:		/* Invalid = in second position */
            return (-1);

        case 2:		/* Valid, means one byte of info */
            /* Skip any number of spaces. */
            for ((void)mDNSNULL; ch != '\0'; ch = *src++)
                if (!mDNSisspace(ch)) {
                    break;
                }
            /* Make sure there is another trailing = sign. */
            if (ch != Pad64) {
                return (-1);
            }
            ch = *src++;		/* Skip the = */
        /* Fall through to "single trailing =" case. */
        /* FALLTHROUGH */

        case 3:		/* Valid, means two bytes of info */
            /*
             * We know this char is an =.  Is there anything but
             * whitespace after it?
             */
            for ((void)mDNSNULL; ch != '\0'; ch = *src++)
                if (!mDNSisspace(ch)) {
                    return (-1);
                }

            /*
             * Now make sure for cases 2 and 3 that the "extra"
             * bits that slopped past the last full byte were
             * zeros.  If we don't check them, they become a
             * subliminal channel.
             */
            if (target && target[tarindex] != 0) {
                return (-1);
            }
        }
    } else {
        /*
         * We ended by seeing the end of the string.  Make sure we
         * have no partial bytes lying around.
         */
        if (state != 0) {
            return (-1);
        }
    }

    return (tarindex);
}

// Constants
#define HMAC_IPAD   0x36
#define HMAC_OPAD   0x5c
#define MD5_LEN     16

#define HMAC_MD5_AlgName (*(const domainname*) "\010" "hmac-md5" "\007" "sig-alg" "\003" "reg" "\003" "int")

// Adapted from Appendix, RFC 2104
mDNSlocal void DNSDigest_ConstructHMACKey(DomainAuthInfo *info, const mDNSu8 *key, mDNSu32 len)
{
    MD5_CTX k;
    mDNSu8 buf[MD5_LEN];
    int i;

    // If key is longer than HMAC_LEN reset it to MD5(key)
    if (len > HMAC_LEN) {
        MD5_Init(&k);
        MD5_Update(&k, key, len);
        MD5_Final(buf, &k);
        key = buf;
        len = MD5_LEN;
    }

    // store key in pads
    mDNSPlatformMemZero(info->keydata_ipad, HMAC_LEN);
    mDNSPlatformMemZero(info->keydata_opad, HMAC_LEN);
    mDNSPlatformMemCopy(info->keydata_ipad, key, len);
    mDNSPlatformMemCopy(info->keydata_opad, key, len);

    // XOR key with ipad and opad values
    for (i = 0; i < HMAC_LEN; i++) {
        info->keydata_ipad[i] ^= HMAC_IPAD;
        info->keydata_opad[i] ^= HMAC_OPAD;
    }
}

mDNSexport mDNSs32 DNSDigest_ConstructHMACKeyfromBase64(DomainAuthInfo *info, const char *b64key)
{
    mDNSu8 keybuf[128];
    mDNSs32 keylen = DNSDigest_Base64ToBin(b64key, keybuf, sizeof(keybuf));
    if (keylen < 0) {
        return (keylen);
    }
    DNSDigest_ConstructHMACKey(info, keybuf, (mDNSu32)keylen);
    return (keylen);
}

mDNSexport void DNSDigest_SignMessage(DNSMessage *msg, mDNSu8 **end, DomainAuthInfo *info, mDNSu16 tcode)
{
    AuthRecord tsig;
    mDNSu8  *rdata, *const countPtr = (mDNSu8 *)&msg->h.numAdditionals;	// Get existing numAdditionals value
    mDNSu32 utc32;
    mDNSu8 utc48[6];
    mDNSu8 digest[MD5_LEN];
    mDNSu8 *ptr = *end;
    mDNSu32 len;
    mDNSOpaque16 buf;
    MD5_CTX c;
    mDNSu16 numAdditionals = (mDNSu16)((mDNSu16)countPtr[0] << 8 | countPtr[1]);

    // Init MD5 context, digest inner key pad and message
    MD5_Init(&c);
    MD5_Update(&c, info->keydata_ipad, HMAC_LEN);
    MD5_Update(&c, (mDNSu8 *)msg, (unsigned long)(*end - (mDNSu8 *)msg));

    // Construct TSIG RR, digesting variables as apporpriate
    mDNS_SetupResourceRecord(&tsig, mDNSNULL, 0, kDNSType_TSIG, 0, kDNSRecordTypeKnownUnique, AuthRecordAny, mDNSNULL, mDNSNULL);

    // key name
    AssignDomainName(&tsig.namestorage, &info->keyname);
    MD5_Update(&c, info->keyname.c, DomainNameLength(&info->keyname));

    // class
    tsig.resrec.rrclass = kDNSQClass_ANY;
    buf = mDNSOpaque16fromIntVal(kDNSQClass_ANY);
    MD5_Update(&c, buf.b, sizeof(mDNSOpaque16));

    // ttl
    tsig.resrec.rroriginalttl = 0;
    MD5_Update(&c, (mDNSu8 *)&tsig.resrec.rroriginalttl, sizeof(tsig.resrec.rroriginalttl));

    // alg name
    AssignDomainName(&tsig.resrec.rdata->u.name, &HMAC_MD5_AlgName);
    len = DomainNameLength(&HMAC_MD5_AlgName);
    rdata = tsig.resrec.rdata->u.data + len;
    MD5_Update(&c, HMAC_MD5_AlgName.c, len);

    // time
    // get UTC (universal time), convert to 48-bit unsigned in network byte order
    utc32 = (mDNSu32)mDNSPlatformUTC();
    if (utc32 == (unsigned) - 1) {
        LogMsg("ERROR: DNSDigest_SignMessage - mDNSPlatformUTC returned bad time -1");
        *end = mDNSNULL;
    }
    utc48[0] = 0;
    utc48[1] = 0;
    utc48[2] = (mDNSu8)((utc32 >> 24) & 0xff);
    utc48[3] = (mDNSu8)((utc32 >> 16) & 0xff);
    utc48[4] = (mDNSu8)((utc32 >>  8) & 0xff);
    utc48[5] = (mDNSu8)(utc32        & 0xff);

    mDNSPlatformMemCopy(rdata, utc48, 6);
    rdata += 6;
    MD5_Update(&c, utc48, 6);

    // 300 sec is fudge recommended in RFC 2485
    rdata[0] = (mDNSu8)((300 >> 8)  & 0xff);
    rdata[1] = (mDNSu8)(300        & 0xff);
    MD5_Update(&c, rdata, sizeof(mDNSOpaque16));
    rdata += sizeof(mDNSOpaque16);

    // digest error (tcode) and other data len (zero) - we'll add them to the rdata later
    buf.b[0] = (mDNSu8)((tcode >> 8) & 0xff);
    buf.b[1] = (mDNSu8)(tcode       & 0xff);
    MD5_Update(&c, buf.b, sizeof(mDNSOpaque16));  // error
    buf.NotAnInteger = 0;
    MD5_Update(&c, buf.b, sizeof(mDNSOpaque16));  // other data len

    // finish the message & tsig var hash
    MD5_Final(digest, &c);

    // perform outer MD5 (outer key pad, inner digest)
    MD5_Init(&c);
    MD5_Update(&c, info->keydata_opad, HMAC_LEN);
    MD5_Update(&c, digest, MD5_LEN);
    MD5_Final(digest, &c);

    // set remaining rdata fields
    rdata[0] = (mDNSu8)((MD5_LEN >> 8)  & 0xff);
    rdata[1] = (mDNSu8)(MD5_LEN        & 0xff);
    rdata += sizeof(mDNSOpaque16);
    mDNSPlatformMemCopy(rdata, digest, MD5_LEN);                          // MAC
    rdata += MD5_LEN;
    rdata[0] = msg->h.id.b[0];                                            // original ID
    rdata[1] = msg->h.id.b[1];
    rdata[2] = (mDNSu8)((tcode >> 8) & 0xff);
    rdata[3] = (mDNSu8)(tcode       & 0xff);
    rdata[4] = 0;                                                         // other data len
    rdata[5] = 0;
    rdata += 6;

    tsig.resrec.rdlength = (mDNSu16)(rdata - tsig.resrec.rdata->u.data);
    *end = PutResourceRecordTTLJumbo(msg, ptr, &numAdditionals, &tsig.resrec, 0);
    if (!*end) {
        LogMsg("ERROR: DNSDigest_SignMessage - could not put TSIG");
        *end = mDNSNULL;
        return;
    }

    // Write back updated numAdditionals value
    countPtr[0] = (mDNSu8)(numAdditionals >> 8);
    countPtr[1] = (mDNSu8)(numAdditionals &  0xFF);
}

mDNSexport mDNSBool DNSDigest_VerifyMessage(DNSMessage *msg, mDNSu8 *end, LargeCacheRecord *lcr, DomainAuthInfo *info, mDNSu16 *rcode, mDNSu16 *tcode)
{
    mDNSu8				*ptr = (mDNSu8 *) &lcr->r.resrec.rdata->u.data;
    mDNSs32				now;
    mDNSs32				then;
    mDNSu8				thisDigest[MD5_LEN];
    mDNSu8				thatDigest[MD5_LEN];
    mDNSu32				macsize;
    mDNSOpaque16 		buf;
    mDNSu8				utc48[6];
    mDNSs32				delta;
    mDNSu16				fudge;
    domainname			*algo;
    MD5_CTX				c;
    mDNSBool			ok = mDNSfalse;

    // We only support HMAC-MD5 for now

    algo = (domainname *) ptr;

    if (!SameDomainName(algo, &HMAC_MD5_AlgName)) {
        LogMsg("ERROR: DNSDigest_VerifyMessage - TSIG algorithm not supported: %##s", algo->c);
        *rcode = kDNSFlag1_RC_NotAuth;
        *tcode = TSIG_ErrBadKey;
        ok = mDNSfalse;
        goto exit;
    }

    ptr += DomainNameLength(algo);

    // Check the times

    now = mDNSPlatformUTC();
    if (now == -1) {
        LogMsg("ERROR: DNSDigest_VerifyMessage - mDNSPlatformUTC returned bad time -1");
        *rcode = kDNSFlag1_RC_NotAuth;
        *tcode = TSIG_ErrBadTime;
        ok = mDNSfalse;
        goto exit;
    }

    // Get the 48 bit time field, skipping over the first word

    utc48[0] = *ptr++;
    utc48[1] = *ptr++;
    utc48[2] = *ptr++;
    utc48[3] = *ptr++;
    utc48[4] = *ptr++;
    utc48[5] = *ptr++;

    then  = (mDNSs32)NToH32(utc48 + sizeof(mDNSu16));

    fudge = NToH16(ptr);

    ptr += sizeof(mDNSu16);

    delta = (now > then) ? now - then : then - now;

    if (delta > fudge) {
        LogMsg("ERROR: DNSDigest_VerifyMessage - time skew > %d", fudge);
        *rcode = kDNSFlag1_RC_NotAuth;
        *tcode = TSIG_ErrBadTime;
        ok = mDNSfalse;
        goto exit;
    }

    // MAC size

    macsize = (mDNSu32) NToH16(ptr);

    ptr += sizeof(mDNSu16);

    // MAC

    mDNSPlatformMemCopy(thatDigest, ptr, MD5_LEN);

    // Init MD5 context, digest inner key pad and message

    MD5_Init(&c);
    MD5_Update(&c, info->keydata_ipad, HMAC_LEN);
    MD5_Update(&c, (mDNSu8 *) msg, (unsigned long)(end - (mDNSu8 *) msg));

    // Key name

    MD5_Update(&c, lcr->r.resrec.name->c, DomainNameLength(lcr->r.resrec.name));

    // Class name

    buf = mDNSOpaque16fromIntVal(lcr->r.resrec.rrclass);
    MD5_Update(&c, buf.b, sizeof(mDNSOpaque16));

    // TTL

    MD5_Update(&c, (mDNSu8 *) &lcr->r.resrec.rroriginalttl, sizeof(lcr->r.resrec.rroriginalttl));

    // Algorithm

    MD5_Update(&c, algo->c, DomainNameLength(algo));

    // Time

    MD5_Update(&c, utc48, 6);

    // Fudge

    buf = mDNSOpaque16fromIntVal(fudge);
    MD5_Update(&c, buf.b, sizeof(mDNSOpaque16));

    // Digest error and other data len (both zero) - we'll add them to the rdata later

    buf.NotAnInteger = 0;
    MD5_Update(&c, buf.b, sizeof(mDNSOpaque16));  // error
    MD5_Update(&c, buf.b, sizeof(mDNSOpaque16));  // other data len

    // Finish the message & tsig var hash

    MD5_Final(thisDigest, &c);

    // perform outer MD5 (outer key pad, inner digest)

    MD5_Init(&c);
    MD5_Update(&c, info->keydata_opad, HMAC_LEN);
    MD5_Update(&c, thisDigest, MD5_LEN);
    MD5_Final(thisDigest, &c);

    if (!mDNSPlatformMemSame(thisDigest, thatDigest, MD5_LEN)) {
        LogMsg("ERROR: DNSDigest_VerifyMessage - bad signature");
        *rcode = kDNSFlag1_RC_NotAuth;
        *tcode = TSIG_ErrBadSig;
        ok = mDNSfalse;
        goto exit;
    }

    // set remaining rdata fields
    ok = mDNStrue;

exit:

    return ok;
}

