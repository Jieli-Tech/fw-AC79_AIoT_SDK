/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "adaptation.h"

#define LOG_TAG             "[MESH-utils]"
/* #define LOG_INFO_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_WARN_ENABLE */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DUMP_ENABLE */
#include "mesh_log.h"


#if ADAPTATION_COMPILE_DEBUG

const char *bt_hex_real(const void *buf, size_t len)
{
    return NULL;
}

int bt_rand(void *buf, size_t len)
{
    return 0;
}

const char *bt_uuid_str(const struct bt_uuid *uuid)
{
    return NULL;
}

#else

const char *bt_hex_real(const void *buf, size_t len)
{
    static const char hex[] = "0123456789abcdef";
    static char str[130];
    const u8_t *b = buf;
    int i;

    if (len > (sizeof(str) / 2) - 1) {
        return "\"Output hex error\"";
    }

    for (i = 0; i < len; i++) {
        str[i * 2]     = hex[b[i] >> 4];
        str[i * 2 + 1] = hex[b[i] & 0xf];
    }

    str[i * 2] = '\0';

    return str;
}

int bt_rand(void *buf, size_t len)
{
    u8_t *buf8 = buf;

    while (len) {
        u32_t v = rand32();

        if (len >= sizeof(v)) {
            memcpy(buf8, &v, sizeof(v));

            buf8 += sizeof(v);
            len -= sizeof(v);
        } else {
            memcpy(buf8, &v, len);
            break;
        }
    }

    return 0;
}

const char *bt_uuid_str(const struct bt_uuid *uuid)
{
    char *str = NULL;

    switch (uuid->type) {
    case BT_UUID_TYPE_16:
        break;
    case BT_UUID_TYPE_32:
        break;
    case BT_UUID_TYPE_128:
        break;
    }

    return str;
}

void sys_memcpy_swap(void *dst, const void *src, size_t length)
{
    u8_t *pdst = (u8_t *)dst;
    const u8_t *psrc = (const u8_t *)src;

    __ASSERT(((psrc < pdst && (psrc + length) <= pdst) ||
              (psrc > pdst && (pdst + length) <= psrc)),
             "Source and destination buffers must not overlap");

    psrc += length - 1;

    for (; length > 0; length--) {
        *pdst++ = *psrc--;
    }
}

#endif /* ADAPTATION_COMPILE_DEBUG */
