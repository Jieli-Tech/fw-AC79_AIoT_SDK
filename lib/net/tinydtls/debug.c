/* debug.c -- debug utilities
 *
 * Copyright (C) 2011--2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "tinydtls.h"
#include "dtls_config.h"

#if defined(HAVE_ASSERT_H) && !defined(assert)
#include <assert.h>
#endif

#include <stdarg.h>
//#include <stdio.h>

#include "printf.h"
#ifdef HAVE_ARPA_INET_H
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#endif

#include <time.h>

#include "global.h"
#include "dtls_debug.h"

static int maxlog = DTLS_LOG_WARN;	/* default maximum log level */

const char *dtls_package_name()
{
    return PACKAGE_NAME;
}

const char *dtls_package_version()
{
    return PACKAGE_VERSION;
}

log_t
dtls_get_log_level()
{
    return maxlog;
}

void
dtls_set_log_level(log_t level)
{
    maxlog = level;
}

/* this array has the same order as the type log_t */
static char *loglevels[] = {
    "EMRG", "ALRT", "CRIT", "WARN", "NOTE", "INFO", "DEBG"
};

//static inline size_t
//print_timestamp(char *s, size_t len, time_t t) {
//  struct tm *tmp;
//  tmp = localtime(&t);
//  return strftime(s, len, "%b %d %H:%M:%S", tmp);
//}

/**
 * A length-safe strlen() fake.
 *
 * @param s      The string to count characters != 0.
 * @param maxlen The maximum length of @p s.
 *
 * @return The length of @p s.
 */
static inline size_t
dtls_strnlen(const char *s, size_t maxlen)
{
    size_t n = 0;
    while (*s++ && n < maxlen) {
        ++n;
    }
    return n;
}

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

static size_t
dsrv_print_addr(const session_t *addr, char *buf, size_t len)
{
#ifdef HAVE_ARPA_INET_H
    const void *addrptr = NULL;
    in_port_t port;
    char *p = buf;

    switch (addr->addr.sa.sa_family) {
    case AF_INET:
        if (len < INET_ADDRSTRLEN) {
            return 0;
        }

        addrptr = &addr->addr.sin.sin_addr;
        port = ntohs(addr->addr.sin.sin_port);
        break;
#if LWIP_IPV6
    case AF_INET6:
        if (len < INET6_ADDRSTRLEN + 2) {
            return 0;
        }

        *p++ = '[';

        addrptr = &addr->addr.sin6.sin6_addr;
        port = ntohs(addr->addr.sin6.sin6_port);

        break;
#endif
    default:
        memcpy(buf, "(unknown address type)", min(22, len));
        return min(22, len);
    }

    if (inet_ntop(addr->addr.sa.sa_family, addrptr, p, len) == 0) {
        printf("dsrv_print_addr");
        return 0;
    }

    p += dtls_strnlen(p, len);

    if (addr->addr.sa.sa_family == AF_INET6) {
        if (p < buf + len) {
            *p++ = ']';
        } else {
            return 0;
        }
    }

    p += snprintf(p, buf + len - p + 1, ":%d", port);

    return p - buf;
#else /* HAVE_ARPA_INET_H */
    /* TODO: output addresses manually */
#   warning "inet_ntop() not available, network addresses will not be included in debug output"
    return 0;
#endif
}


void
dsrv_log(log_t level, char *format, ...)
{
//  static char timebuf[32];
    va_list ap;

    if (maxlog < level) {
        return;
    }

//  if (print_timestamp(timebuf,sizeof(timebuf), time(NULL)))
//    PRINTF("%s ", timebuf);

    if (level <= DTLS_LOG_DEBUG) {
        PRINTF("%s ", loglevels[level]);
    }

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}

#ifndef NDEBUG
/** dumps packets in usual hexdump format */
void hexdump(const unsigned char *packet, int length)
{
    int n = 0;

    while (length--) {
        if (n % 16 == 0) {
            printf("%08X ", n);
        }

        printf("%02X ", *packet++);

        n++;
        if (n % 8 == 0) {
            if (n % 16 == 0) {
                printf("\n");
            } else {
                printf(" ");
            }
        }
    }
}

/** dump as narrow string of hex digits */
void dump(unsigned char *buf, size_t len)
{
    while (len--) {
        printf("%02x", *buf++);
    }
}

void dtls_dsrv_log_addr(log_t level, const char *name, const session_t *addr)
{
    char addrbuf[73];
    int len;

    len = dsrv_print_addr(addr, addrbuf, sizeof(addrbuf));
    if (!len) {
        return;
    }
    dsrv_log(level, "%s: %s\n", name, addrbuf);
}


void
dtls_dsrv_hexdump_log(log_t level, const char *name, const unsigned char *buf, size_t length, int extend)
{
//  static char timebuf[32];
    int n = 0;

    if (maxlog < level) {
        return;
    }

//  if (print_timestamp(timebuf, sizeof(timebuf), time(NULL)))
//    printf( "%s ", timebuf);

    if (level <= DTLS_LOG_DEBUG) {
        printf("%s ", loglevels[level]);
    }

    if (extend) {
        printf("%s: (%u bytes):\n", name, length);

        while (length--) {
            if (n % 16 == 0) {
                printf("%08X ", n);
            }

            printf("%02X ", *buf++);

            n++;
            if (n % 8 == 0) {
                if (n % 16 == 0) {
                    printf("\n");
                } else {
                    printf(" ");
                }
            }
        }
    } else {
        printf("%s: (%u bytes): ", name, length);
        while (length--) {
            printf("%02X", *buf++);
        }
    }
    printf("\n");
}

#endif /* NDEBUG */
