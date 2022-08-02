#ifndef HEADER_CURL_MBEDTLS_H
#define HEADER_CURL_MBEDTLS_H
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 2012 - 2016, Daniel Stenberg, <daniel@haxx.se>, et al.
 * Copyright (C) 2010, Hoi-Ho Chan, <hoiho.chan@gmail.com>
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include "curl_setup.h"

#ifdef USE_MBEDTLS
extern const struct Curl_ssl Curl_ssl_mbedtls;
// static int Curl_mbedtls_init(void);
// static void Curl_mbedtls_cleanup(void);
// static size_t Curl_mbedtls_version(char *buffer, size_t size);
// static bool Curl_mbedtls_data_pending(const struct connectdata *conn, int sockindex);
// static CURLcode Curl_mbedtls_random(struct Curl_easy *data, unsigned char *entropy, size_t length);
// static CURLcode Curl_mbedtls_connect(struct connectdata *conn, int sockindex);
// static CURLcode Curl_mbedtls_connect_nonblocking(struct connectdata *conn, int sockindex, bool *done);
// static CURLcode Curl_mbedtls_sha256sum(const unsigned char *input, size_t inputlen, unsigned char *sha256sum, size_t sha256len UNUSED_PARAM);
// static void *Curl_mbedtls_get_internals(struct ssl_connect_data *connssl, CURLINFO info UNUSED_PARAM);
// static void Curl_mbedtls_close_all(struct Curl_easy *data);
// static void Curl_mbedtls_close(struct connectdata *conn, int sockindex);
// static void Curl_mbedtls_session_free(void *ptr);

#endif /* USE_MBEDTLS */
#endif /* HEADER_CURL_MBEDTLS_H */
