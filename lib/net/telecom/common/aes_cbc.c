/* crypto/aes/aes_cbc.c -*- mode:C; c-file-style: "eay" -*- */
/* ====================================================================
 * Copyright (c) 1998-2002 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 */

#ifndef AES_DEBUG
# ifndef NDEBUG
#  define NDEBUG
# endif
#endif
#include <assert.h>

#include "aes.h"
#include "aes_locl.h"
#include <string.h>

void AES_cbc_core(const unsigned char *in, unsigned char *out,
                  const unsigned int length, unsigned int *outlength, const AES_KEY *key,
                  unsigned char *ivec, const int enc)
{

    unsigned long n;
    unsigned long len = length;
    unsigned char tmp[AES_BLOCK_SIZE];
    unsigned char tempiv[AES_BLOCK_SIZE];
    const unsigned char *iv;
    unsigned int paddingLen;

    assert(in && out && key && ivec);
    assert((AES_ENCRYPT == enc) || (AES_DECRYPT == enc));
    memcpy(tempiv, ivec, AES_BLOCK_SIZE);
    iv = tempiv;
    if (AES_ENCRYPT == enc) {
        while (len >= AES_BLOCK_SIZE) {
            for (n = 0; n < AES_BLOCK_SIZE; ++n) {
                out[n] = in[n] ^ iv[n];
            }
            AES_encrypt(out, out, key);
            iv = out;
            len -= AES_BLOCK_SIZE;
            in += AES_BLOCK_SIZE;
            out += AES_BLOCK_SIZE;
        }
        if (len) {
            paddingLen = AES_BLOCK_SIZE - len;
            for (n = 0; n < len; ++n) {
                out[n] = in[n] ^ iv[n];
            }
            for (n = len; n < AES_BLOCK_SIZE; ++n) {
                out[n] = paddingLen ^ iv[n];
            }
            AES_encrypt(out, out, key);
            iv = out;
            *outlength = length + paddingLen;
        } else if (0 == len) {
            paddingLen = AES_BLOCK_SIZE;
            for (n = 0; n < AES_BLOCK_SIZE; ++n) {
                out[n] = paddingLen ^ iv[n];
            }
            AES_encrypt(out, out, key);
            iv = out;
            *outlength = length + paddingLen;
        }

    } else if (in != out) {
        while (len >= AES_BLOCK_SIZE) {
            AES_decrypt(in, out, key);
            for (n = 0; n < AES_BLOCK_SIZE; ++n) {
                out[n] ^= iv[n];
            }
            iv = in;
            len -= AES_BLOCK_SIZE;
            in  += AES_BLOCK_SIZE;
            out += AES_BLOCK_SIZE;
        }
        if (len) {
            AES_decrypt(in, tmp, key);
            for (n = 0; n < len; ++n) {
                out[n] = tmp[n] ^ iv[n];
            }
            iv = in;
            *outlength = length - out[len - 1];
        } else if (0 == len) {
            *outlength = length - *(out - 1);
        }

    } else {
        while (len >= AES_BLOCK_SIZE) {
            memcpy(tmp, in, AES_BLOCK_SIZE);
            AES_decrypt(in, out, key);
            for (n = 0; n < AES_BLOCK_SIZE; ++n) {
                out[n] ^= ivec[n];
            }
            memcpy(ivec, tmp, AES_BLOCK_SIZE);
            len -= AES_BLOCK_SIZE;
            in += AES_BLOCK_SIZE;
            out += AES_BLOCK_SIZE;
        }
        if (len) {
            memcpy(tmp, in, AES_BLOCK_SIZE);
            AES_decrypt(tmp, out, key);
            for (n = 0; n < len; ++n) {
                out[n] ^= ivec[n];
            }
            for (n = len; n < AES_BLOCK_SIZE; ++n) {
                out[n] = tmp[n];
            }
            memcpy(ivec, tmp, AES_BLOCK_SIZE);
            *outlength = length - out[len - 1];
        } else if (0 == len) {
            *outlength = length - *(out - 1);
        }
    }
}

void CT_AES_CBC_Encrypt(
    unsigned char *PlainText,
    unsigned int PlainTextLength,
    unsigned char *Key,
    unsigned int KeyLength,
    unsigned char *IV,
    unsigned int IVLength,
    unsigned char *CipherText,
    unsigned int *CipherTextLength)
{
    AES_KEY struKey;
    AES_set_encrypt_key(Key, KeyLength * 8,
                        &struKey);
    AES_cbc_core(PlainText, CipherText,
                 PlainTextLength, CipherTextLength, &struKey,
                 IV, AES_ENCRYPT);
    return;
}

void CT_AES_CBC_Decrypt(
    unsigned char *CipherText,
    unsigned int CipherTextLength,
    unsigned char *Key,
    unsigned int KeyLength,
    unsigned char *IV,
    unsigned int IVLength,
    unsigned char *PlainText,
    unsigned int *PlainTextLength)
{
    AES_KEY struKey;
    AES_set_decrypt_key(Key, KeyLength * 8,
                        &struKey);
    AES_cbc_core(CipherText, PlainText,
                 CipherTextLength, PlainTextLength, &struKey,
                 IV, AES_DECRYPT);
    return;
}


