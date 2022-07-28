/* debug.c -- debug utilities
 *
 * Copyright (C) 2010--2012 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use.
 */

#include "coap_config.h"

#include <assert.h>

#include <stdarg.h>
#include "printf.h"
//#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include <time.h>

#include "coap_debug.h"
#include "coap_net.h"

#define perror printf
static coap_log_t maxlog = LOG_WARNING;	/* default maximum log level */

const char *coap_package_name()
{
    return COAP_PACKAGE_NAME;
}

const char *coap_package_version()
{
    return COAP_PACKAGE_STRING;
}

coap_log_t
coap_get_log_level()
{
    return maxlog;
}

void
coap_set_log_level(coap_log_t level)
{
    maxlog = level;
}

/* this array has the same order as the type log_t */
static char *loglevels[] = {
    "EMRG", "ALRT", "CRIT", "ERR", "WARN", "NOTE", "INFO", "DEBG"
};

//static inline size_t
//print_timestamp(char *s, size_t len, coap_tick_t t)
//{
//    struct tm *tmp;
//    time_t now = clock_offset + (t / COAP_TICKS_PER_SECOND);
//    tmp = localtime(&now);
//    return strftime(s, len, "%b %d %H:%M:%S", tmp);
//}

#ifndef NDEBUG

unsigned int
print_readable(const unsigned char *data, unsigned int len,
               unsigned char *result, unsigned int buflen, int encode_always)
{
    const unsigned char hex[] = "0123456789ABCDEF";
    unsigned int cnt = 0;
    assert(data || len == 0);

    if (buflen == 0 || len == 0) {
        return 0;
    }

    while (len) {
        if (!encode_always && isprint(*data)) {
            if (cnt == buflen) {
                break;
            }
            *result++ = *data;
            ++cnt;
        } else {
            if (cnt + 4 < buflen) {
                *result++ = '\\';
                *result++ = 'x';
                *result++ = hex[(*data & 0xf0) >> 4];
                *result++ = hex[*data & 0x0f];
                cnt += 4;
            } else {
                break;
            }
        }

        ++data;
        --len;
    }

    *result = '\0';
    return cnt;
}

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

size_t
coap_print_addr(const struct coap_address_t *addr, unsigned char *buf, size_t len)
{
    const void *addrptr = NULL;
    in_port_t port;
    unsigned char *p = buf;

    switch (addr->addr.sa.sa_family) {
    case AF_INET:
        addrptr = &addr->addr.sin.sin_addr;
        port = ntohs(addr->addr.sin.sin_port);
        break;
#if LWIP_IPV6
    case AF_INET6:
        if (len < 7) { /* do not proceed if buffer is even too short for [::]:0 */
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

    if (inet_ntop(addr->addr.sa.sa_family, addrptr, (char *)p, len) == 0) {
        perror("coap_print_addr");
        return 0;
    }

    extern size_t strnlen(const char *__string, size_t __maxlen);
    p += strnlen((char *)p, len);

    if (addr->addr.sa.sa_family == AF_INET6) {
        if (p < buf + len) {
            *p++ = ']';
        } else {
            return 0;
        }
    }

    p += snprintf((char *)p, buf + len - p + 1, ":%d", port);

    return buf + len - p;
}

void
coap_show_pdu(const coap_pdu_t *pdu)
{
    unsigned char buf[COAP_MAX_PDU_SIZE]; /* need some space for output creation */
    int encode = 0, have_options = 0;
    coap_opt_iterator_t opt_iter;
    coap_opt_t *option;

    printf("v:%d t:%d tkl:%d c:%d id:%u",
           pdu->hdr->version, pdu->hdr->type,
           pdu->hdr->token_length,
           pdu->hdr->code, ntohs(pdu->hdr->id));

    /* show options, if any */
    coap_option_iterator_init((coap_pdu_t *)pdu, &opt_iter, COAP_OPT_ALL);

    while ((option = coap_option_next(&opt_iter))) {
        if (!have_options) {
            have_options = 1;
            printf(" o: [");
        } else {
            printf(",");
        }

        if (opt_iter.type == COAP_OPTION_URI_PATH ||
            opt_iter.type == COAP_OPTION_PROXY_URI ||
            opt_iter.type == COAP_OPTION_URI_HOST ||
            opt_iter.type == COAP_OPTION_LOCATION_PATH ||
            opt_iter.type == COAP_OPTION_LOCATION_QUERY ||
            opt_iter.type == COAP_OPTION_URI_PATH ||
            opt_iter.type == COAP_OPTION_URI_QUERY) {
            encode = 0;
        } else {
            encode = 1;
        }

        if (print_readable(COAP_OPT_VALUE(option),
                           COAP_OPT_LENGTH(option),
                           buf, sizeof(buf), encode)) {
            printf(" %d:'%s'", opt_iter.type, buf);
        }
    }

    if (have_options) {
        printf(" ]");
    }

    if (pdu->data) {
        assert(pdu->data < (unsigned char *)pdu->hdr + pdu->length);
        print_readable(pdu->data,
                       (unsigned char *)pdu->hdr + pdu->length - pdu->data,
                       buf, sizeof(buf), 0);
        printf(" d:%s", buf);
    }
    printf("\n");
}

#endif /* NDEBUG */


void
coap_log_impl(coap_log_t level, const char *format, ...)
{
//    char timebuf[32];
//    coap_tick_t now;
    va_list ap;

    if (maxlog < level) {
        return;
    }

//    coap_ticks(&now);
//    if (print_timestamp(timebuf,sizeof(timebuf), now))
//        printf("%s ", timebuf);

    if (level <= LOG_DEBUG) {
        printf("%s ", loglevels[level]);
    }

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}
