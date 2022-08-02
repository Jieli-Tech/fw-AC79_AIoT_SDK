/**
 * @file hmac.h
 * @brief HMAC (Keyed-Hashing for Message Authentication)
 *
 * @section License
 *
 * Copyright (C) 2010-2014 Oryx Embedded. All rights reserved.
 *
 * This file is part of CycloneCrypto Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded (www.oryx-embedded.com)
 * @version 1.5.0
 **/

#ifndef _HMAC_H
#define _HMAC_H

//Dependencies
#include "crypto.h"
#include "sha256.h"

//Inner padding (ipad)
#define HMAC_IPAD 0x36
//Outer padding (opad)
#define HMAC_OPAD 0x5C


/**
 * @brief HMAC algorithm context
 **/

typedef struct {
    const HashAlgo *hash;
    u8 hashContext[MAX_HASH_CONTEXT_SIZE];
    u8 key[MAX_HASH_BLOCK_SIZE];
    u8 digest[MAX_HASH_DIGEST_SIZE];
} HmacContext;


//HMAC related constants
//extern const uint8_t HMAC_WITH_MD5_OID[8];
//extern const uint8_t HMAC_WITH_TIGER_OID[8];
//extern const uint8_t HMAC_WITH_RIPEMD160_OID[8];
//extern const uint8_t HMAC_WITH_SHA1_OID[8];
//extern const uint8_t HMAC_WITH_SHA224_OID[8];
//extern const uint8_t HMAC_WITH_SHA256_OID[8];
//extern const uint8_t HMAC_WITH_SHA384_OID[8];
//extern const uint8_t HMAC_WITH_SHA512_OID[8];

//HMAC related functions
error_t hmacCompute(const HashAlgo *hash, const void *key, int keyLength,
                    const void *data, int dataLength, u8 *digest);

void hmacInit(HmacContext *context, const HashAlgo *hash,
              const void *key, int length);

void hmacUpdate(HmacContext *context, const void *data, int length);
void hmacFinal(HmacContext *context, u8 *digest);

#endif

