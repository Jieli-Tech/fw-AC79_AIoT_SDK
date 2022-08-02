/**
 * @file crypto.h
 * @brief General definitions for cryptographic algorithms
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

#ifndef _CRYPTO_H
#define _CRYPTO_H

//Dependencies
//#include "..\ECDH\ssp_typedef.h.h"
#include "endian.h"


typedef int error_t;




//Maximum context size (hash functions)

#define MAX_HASH_CONTEXT_SIZE sizeof(Sha256Context)


//Maximum block size (hash functions)

#define MAX_HASH_BLOCK_SIZE SHA256_BLOCK_SIZE


//Maximum digest size (hash functions)

#define MAX_HASH_DIGEST_SIZE SHA256_DIGEST_SIZE

//Rotate left operation
#define ROL32(a, n) (((a) << (n)) | ((a) >> (32 - (n))))
#define ROL64(a, n) (((a) << (n)) | ((a) >> (64 - (n))))
//Rotate right operation
#define ROR32(a, n) (((a) >> (n)) | ((a) << (32 - (n))))
#define ROR64(a, n) (((a) >> (n)) | ((a) << (64 - (n))))

//Shift left operation
#define SHL32(a, n) ((a) << (n))
#define SHL64(a, n) ((a) << (n))
//Shift right operation
#define SHR32(a, n) ((a) >> (n))
#define SHR64(a, n) ((a) >> (n))


/**
 * @brief Encryption algorithm type
 **/

//typedef enum
//{
//   CIPHER_ALGO_TYPE_STREAM = 0,
//   CIPHER_ALGO_TYPE_BLOCK  = 1
//} CipherAlgoType;


/**
 * @brief Cipher operation modes
 **/
//
//typedef enum
//{
//   CIPHER_MODE_STREAM = 0,
//   CIPHER_MODE_ECB    = 1,
//   CIPHER_MODE_CBC    = 2,
//   CIPHER_MODE_CFB    = 3,
//   CIPHER_MODE_OFB    = 4,
//   CIPHER_MODE_CTR    = 5,
//   CIPHER_MODE_CCM    = 6,
//   CIPHER_MODE_GCM    = 7
//} CipherMode;


//Common API for hash algorithms
typedef error_t (*HashAlgoCompute)(const void *data, int length, u8 *digest);
typedef void (*HashAlgoInit)(void *context);
typedef void (*HashAlgoUpdate)(void *context, const void *data, int length);
typedef void (*HashAlgoFinal)(void *context, u8 *digest);

////Common API for encryption algorithms
//typedef error_t (*CipherAlgoInit)(void *context, const uint8_t *key, size_t keyLength);
//typedef void (*CipherAlgoEncryptStream)(void *context, const uint8_t *input, uint8_t *output, size_t length);
//typedef void (*CipherAlgoDecryptStream)(void *context, const uint8_t *input, uint8_t *output, size_t length);
//typedef void (*CipherAlgoEncryptBlock)(void *context, const uint8_t *input, uint8_t *output);
//typedef void (*CipherAlgoDecryptBlock)(void *context, const uint8_t *input, uint8_t *output);
//
////Common API for pseudo-random number generators
//typedef error_t (*PrngAlgoInit)(void *context);
//typedef void (*PrngAlgoRelease)(void *context);
//typedef error_t (*PrngAlgoSeed)(void *context, const uint8_t *input, size_t length);
//typedef error_t (*PrngAlgoAddEntropy)(void *context, uint_t source, const uint8_t *input, size_t length, size_t entropy);
//typedef error_t (*PrngAlgoRead)(void *context, uint8_t *output, size_t length);


/**
 * @brief Generic hash algorithm context
 **/

typedef struct {
    u8 digest[1];
} HashContext;


/**
 * @brief Common interface for hash algorithms
 **/

typedef struct {
    const s8 *name;
    const u8 *oid;
    int oidSize;
    int contextSize;
    int blockSize;
    int digestSize;
    HashAlgoCompute compute;
    HashAlgoInit init;
    HashAlgoUpdate update;
    HashAlgoFinal final;
} HashAlgo;


/**
 * @brief Common interface for encryption algorithms
 **/

//typedef struct
//{
//   const char_t *name;
//   size_t contextSize;
//   CipherAlgoType type;
//   size_t blockSize;
//   CipherAlgoInit init;
//   CipherAlgoEncryptStream encryptStream;
//   CipherAlgoDecryptStream decryptStream;
//   CipherAlgoEncryptBlock encryptBlock;
//   CipherAlgoDecryptBlock decryptBlock;
//} CipherAlgo;


/**
 * @brief Common interface for pseudo-random number generators
 **/

//typedef struct
//{
//   const char_t *name;
//   size_t contextSize;
//   PrngAlgoInit init;
//   PrngAlgoRelease release;
//   PrngAlgoSeed seed;
//   PrngAlgoAddEntropy addEntropy;
//   PrngAlgoRead read;
//} PrngAlgo;

#endif

