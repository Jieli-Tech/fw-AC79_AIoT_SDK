/* coap -- simple implementation of the Constrained Application Protocol (CoAP)
 *         as defined in draft-ietf-core-coap
 *
 * Copyright (C) 2010--2013 Olaf Bergmann <bergmann@tzi.org>
 *
 * This file is part of the CoAP library libcoap. Please see
 * README for terms of use.
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "printf.h"
//#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include "coap_config.h"
#include "resource.h"
#include "coap.h"
//#include "coap_net.h"
#include "dtls_config.h"
#include "dtls.h"
#include "dtls_debug.h"

#define perror printf
#define COAP_RESOURCE_CHECK_TIME 2

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/* temporary storage for dynamic resource representations */
static int quit = 0;

/* changeable clock base (see handle_put_time()) */
static time_t my_clock_base = 0;

struct coap_resource_t *time_resource = NULL;
//#define WITHOUT_ASYNC
#ifndef WITHOUT_ASYNC
/* This variable is used to mimic long-running tasks that require
 * asynchronous responses. */
static coap_async_state_t *async = NULL;
#endif /* WITHOUT_ASYNC */

/* SIGINT handler: set quit to 1 for graceful termination */
static void
handle_sigint(int signum)
{
    quit = 1;
}

#define COAP_INDEX "This is a combination of libcoap's and tinydtl's server implementations.\n"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DTLS_PSK

static const unsigned char ecdsa_priv_key[] = {
    0xD9, 0xE2, 0x70, 0x7A, 0x72, 0xDA, 0x6A, 0x05,
    0x04, 0x99, 0x5C, 0x86, 0xED, 0xDB, 0xE3, 0xEF,
    0xC7, 0xF1, 0xCD, 0x74, 0x83, 0x8F, 0x75, 0x70,
    0xC8, 0x07, 0x2D, 0x0A, 0x76, 0x26, 0x1B, 0xD4
};

static const unsigned char ecdsa_pub_key_x[] = {
    0xD0, 0x55, 0xEE, 0x14, 0x08, 0x4D, 0x6E, 0x06,
    0x15, 0x59, 0x9D, 0xB5, 0x83, 0x91, 0x3E, 0x4A,
    0x3E, 0x45, 0x26, 0xA2, 0x70, 0x4D, 0x61, 0xF2,
    0x7A, 0x4C, 0xCF, 0xBA, 0x97, 0x58, 0xEF, 0x9A
};

static const unsigned char ecdsa_pub_key_y[] = {
    0xB4, 0x18, 0xB6, 0x4A, 0xFE, 0x80, 0x30, 0xDA,
    0x1D, 0xDC, 0xF4, 0xF4, 0x2E, 0x2F, 0x26, 0x31,
    0xD0, 0x43, 0xB1, 0xFB, 0x03, 0xE2, 0x2F, 0x4D,
    0x17, 0xDE, 0x43, 0xF9, 0xF9, 0xAD, 0xEE, 0x70
};


/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int
get_psk_info(struct dtls_context_t *ctx, const session_t *session,
             dtls_credentials_type_t type,
             const unsigned char *id, size_t id_len,
             unsigned char *result, size_t result_length)
{

    struct keymap_t {
        unsigned char *id;
        size_t id_length;
        unsigned char *key;
        size_t key_length;
    } psk[3] = {
        {
            (unsigned char *)"Client_identity", 15,
            (unsigned char *)"secretPSK", 9
        },
        {
            (unsigned char *)"default identity", 16,
            (unsigned char *)"\x11\x22\x33", 3
        },
        {
            (unsigned char *)"\0", 2,
            (unsigned char *)"", 1
        }
    };

    if (type != DTLS_PSK_KEY) {
        return 0;
    }

    if (id) {
        int i;
        for (i = 0; i < sizeof(psk) / sizeof(struct keymap_t); i++) {
            if (id_len == psk[i].id_length && memcmp(id, psk[i].id, id_len) == 0) {
                if (result_length < psk[i].key_length) {
                    dtls_warn("buffer too small for PSK");
                    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
                }

                memcpy(result, psk[i].key, psk[i].key_length);
                return psk[i].key_length;
            }
        }
    }

    return dtls_alert_fatal_create(DTLS_ALERT_DECRYPT_ERROR);
}

#endif /* DTLS_PSK */

#ifdef DTLS_ECC
static int
get_ecdsa_key(struct dtls_context_t *ctx,
              const session_t *session,
              const dtls_ecdsa_key_t **result)
{
    static const dtls_ecdsa_key_t ecdsa_key = {
        .curve = DTLS_ECDH_CURVE_SECP256R1,
        .priv_key = ecdsa_priv_key,
        .pub_key_x = ecdsa_pub_key_x,
        .pub_key_y = ecdsa_pub_key_y
    };

    *result = &ecdsa_key;
    return 0;
}

static int
verify_ecdsa_key(struct dtls_context_t *ctx,
                 const session_t *session,
                 const unsigned char *other_pub_x,
                 const unsigned char *other_pub_y,
                 size_t key_size)
{
    return 0;
}
#endif /* DTLS_ECC */

#define DTLS_SERVER_CMD_CLOSE "server:close"
#define DTLS_SERVER_CMD_RENEGOTIATE "server:renegotiate"

static int dtls_event_cb(struct dtls_context_t *ctx, session_t *session,
                         dtls_alert_level_t level, unsigned short code)
{
    printf("dtls_event_cb->%d,%d\n", level, code);

    if (code == DTLS_EVENT_CONNECTED) {
        puts("\n\n |DTLS_EVENT_CONNECTED| \n");
    }

    return 0;
}

static int
read_from_peer(struct dtls_context_t *ctx,
               session_t *session, uint8 *data, size_t len)
{
    coap_context_t *coap_ctx = (coap_context_t *)dtls_get_app_data(ctx);
    printf("\n read_from_peer->%s\n", data);
    if (len >= strlen(DTLS_SERVER_CMD_CLOSE) &&
        !memcmp(data, DTLS_SERVER_CMD_CLOSE, strlen(DTLS_SERVER_CMD_CLOSE))) {
        printf("server: closing connection\n");
        dtls_close(ctx, session);
        return len;
    } else if (len >= strlen(DTLS_SERVER_CMD_RENEGOTIATE) &&
               !memcmp(data, DTLS_SERVER_CMD_RENEGOTIATE, strlen(DTLS_SERVER_CMD_RENEGOTIATE))) {
        printf("server: renegotiate connection\n");
        dtls_renegotiate(ctx, session);
        return len;
    }

    coap_readfrom(coap_ctx, data, len, &session->addr.sa, session->size);

    return len;
}

static int
send_to_peer(struct dtls_context_t *ctx,
             session_t *session, uint8 *data, size_t len)
{

    int fd = ((coap_context_t *)dtls_get_app_data(ctx))->sockfd;
    return sendto(fd, data, len, MSG_DONTWAIT,
                  &session->addr.sa, session->size);
}

static int
dtls_handle_read(struct dtls_context_t *ctx)
{
    int fd;
    session_t session;
    static uint8 buf[DTLS_MAX_BUF];
    int len;

    fd = ((coap_context_t *)dtls_get_app_data(ctx))->sockfd;

    memset(&session, 0, sizeof(session_t));
    session.size = sizeof(session.addr);
    len = recvfrom(fd, buf, sizeof(buf), 0,
                   &session.addr.sa, &session.size);

    if (len < 0) {
        printf("recvfrom");
        return -1;
    } else {
        dtls_debug("got %d bytes from port %d\n", len,
                   ntohs(session.addr.sin.sin_port));
        if (sizeof(buf) < len) {
            dtls_warn("packet was truncated (%d bytes lost)\n", len - sizeof(buf));
        }
    }

    return dtls_handle_message(ctx, &session, buf, len);
}

static dtls_handler_t cb = {
    .net_write = send_to_peer,
    .net_read  = read_from_peer,
    .event = dtls_event_cb,
#ifdef DTLS_PSK
    .get_psk_info = get_psk_info,
#endif /* DTLS_PSK */
#ifdef DTLS_ECC
    .get_ecdsa_key = get_ecdsa_key,
    .verify_ecdsa_key = verify_ecdsa_key
#endif /* DTLS_ECC */
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


static void
hnd_get_index(coap_context_t  *ctx, struct coap_resource_t *resource,
              coap_address_t *peer, coap_pdu_t *request, str *token,
              coap_pdu_t *response)
{
    unsigned char buf[3];

    response->hdr->code = COAP_RESPONSE_CODE(205);

    coap_add_option(response, COAP_OPTION_CONTENT_TYPE,
                    coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

    coap_add_option(response, COAP_OPTION_MAXAGE,
                    coap_encode_var_bytes(buf, 0x2ffff), buf);

    coap_add_data(response, strlen(COAP_INDEX), (unsigned char *)COAP_INDEX);
}

static void
hnd_get_time(coap_context_t  *ctx, struct coap_resource_t *resource,
             coap_address_t *peer, coap_pdu_t *request, str *token,
             coap_pdu_t *response)
{
    coap_opt_iterator_t opt_iter;
    coap_opt_t *option;
    unsigned char buf[40];
    size_t len;
    time_t now;
    coap_tick_t t;
    coap_subscription_t *subscription;

    /* FIXME: return time, e.g. in human-readable by default and ticks
     * when query ?ticks is given. */

    /* if my_clock_base was deleted, we pretend to have no such resource */
    response->hdr->code =
        my_clock_base ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(404);

    if (request != NULL &&
        coap_check_option(request, COAP_OPTION_OBSERVE, &opt_iter)) {
        subscription = coap_add_observer(resource, peer, token);
        if (subscription) {
            subscription->non = request->hdr->type == COAP_MESSAGE_NON;
            coap_add_option(response, COAP_OPTION_OBSERVE, 0, NULL);
        }
    }
    if (resource->dirty == 1)
        coap_add_option(response, COAP_OPTION_OBSERVE,
                        coap_encode_var_bytes(buf, ctx->observe), buf);


    if (my_clock_base)
        coap_add_option(response, COAP_OPTION_CONTENT_FORMAT,
                        coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

    coap_add_option(response, COAP_OPTION_MAXAGE,
                    coap_encode_var_bytes(buf, 0x01), buf);

    if (my_clock_base) {

        /* calculate current time */
        coap_ticks(&t);
        now = my_clock_base + (t / COAP_TICKS_PER_SECOND);

        if (request != NULL
            && (option = coap_check_option(request, COAP_OPTION_URI_QUERY, &opt_iter))
            && memcmp(COAP_OPT_VALUE(option), "ticks",
                      min(5, COAP_OPT_LENGTH(option))) == 0) {
            /* output ticks */
            len = snprintf((char *)buf,
                           min(sizeof(buf), response->max_size - response->length),
                           "%u", (unsigned int)now);
            coap_add_data(response, len, buf);

        } else {			/* output human-readable time */
            struct tm *tmp;
            struct tm result;
            tmp = gmtime_r(&now, &result);
            len = strftime((char *)buf,
                           min(sizeof(buf), response->max_size - response->length),
                           "%b %d %H:%M:%S", tmp);
            coap_add_data(response, len, buf);
        }
    }
}

static void
hnd_put_time(coap_context_t  *ctx, struct coap_resource_t *resource,
             coap_address_t *peer, coap_pdu_t *request, str *token,
             coap_pdu_t *response)
{
    coap_tick_t t;
    size_t size;
    unsigned char *data;

    /* FIXME: re-set my_clock_base to clock_offset if my_clock_base == 0
     * and request is empty. When not empty, set to value in request payload
     * (insist on query ?ticks). Return Created or Ok.
     */

    /* if my_clock_base was deleted, we pretend to have no such resource */
    response->hdr->code =
        my_clock_base ? COAP_RESPONSE_CODE(204) : COAP_RESPONSE_CODE(201);

    resource->dirty = 1;

    coap_get_data(request, &size, &data);

    if (size == 0) {	/* re-init */
        my_clock_base = clock_offset;
    } else {
        my_clock_base = 0;
        coap_ticks(&t);
        while (size--) {
            my_clock_base = my_clock_base * 10 + *data++;
        }
        my_clock_base -= t / COAP_TICKS_PER_SECOND;
    }
}

static void
hnd_delete_time(coap_context_t  *ctx, struct coap_resource_t *resource,
                coap_address_t *peer, coap_pdu_t *request, str *token,
                coap_pdu_t *response)
{
    my_clock_base = 0;		/* mark clock as "deleted" */

    /* type = request->hdr->type == COAP_MESSAGE_CON  */
    /*   ? COAP_MESSAGE_ACK : COAP_MESSAGE_NON; */
}

#ifndef WITHOUT_ASYNC
static void
hnd_get_async(coap_context_t  *ctx, struct coap_resource_t *resource,
              coap_address_t *peer, coap_pdu_t *request, str *token,
              coap_pdu_t *response)
{
    coap_opt_iterator_t opt_iter;
    coap_opt_t *option;
    unsigned long delay = 5;
    size_t size;

    if (async) {
        if (async->id != request->hdr->id) {
            coap_opt_filter_t f;
            coap_option_filter_clear(f);
            response->hdr->code = COAP_RESPONSE_CODE(503);
        }
        return;
    }

    option = coap_check_option(request, COAP_OPTION_URI_QUERY, &opt_iter);
    if (option) {
        unsigned char *p = COAP_OPT_VALUE(option);

        delay = 0;
        for (size = COAP_OPT_LENGTH(option); size; --size, ++p) {
            delay = delay * 10 + (*p - '0');
        }
    }

    async = coap_register_async(ctx, peer, request,
                                COAP_ASYNC_SEPARATE | COAP_ASYNC_CONFIRM,
                                (void *)(COAP_TICKS_PER_SECOND * delay));
}

static void
check_async(coap_context_t  *ctx, coap_tick_t now)
{
    coap_pdu_t *response;
    coap_async_state_t *tmp;

    size_t size = sizeof(coap_hdr_t) + 13;

    if (!async || now < async->created + (unsigned long)async->appdata) {
        return;
    }

    response = coap_pdu_init(async->flags & COAP_ASYNC_CONFIRM
                             ? COAP_MESSAGE_CON
                             : COAP_MESSAGE_NON,
                             COAP_RESPONSE_CODE(205), 0, size);
    if (!response) {
        debug("check_async: insufficient memory, we'll try later\n");
        async->appdata =
            (void *)((unsigned long)async->appdata + 15 * COAP_TICKS_PER_SECOND);
        return;
    }

    response->hdr->id = coap_new_message_id(ctx);

    if (async->tokenlen) {
        coap_add_token(response, async->tokenlen, async->token);
    }

    coap_add_data(response, 4, (unsigned char *)"done");

    if (coap_send(ctx, &async->peer, response) == COAP_INVALID_TID) {
        debug("check_async: cannot send response for message %d\n",
              response->hdr->id);
    }
    coap_delete_pdu(response);
    coap_remove_async(ctx, async->id, &tmp);
    coap_free_async(async);
    async = NULL;
}
#endif /* WITHOUT_ASYNC */

static void
init_resources(coap_context_t *ctx)
{
    coap_resource_t *r;

    r = coap_resource_init(NULL, 0, 0);
    coap_register_handler(r, COAP_REQUEST_GET, hnd_get_index);

    coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
    coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"General Info\"", 14, 0);
    coap_add_resource(ctx, r);

    /* store clock base to use in /time */
    my_clock_base = clock_offset;

    r = coap_resource_init((unsigned char *)"time", 4, 0);
    coap_register_handler(r, COAP_REQUEST_GET, hnd_get_time);
    coap_register_handler(r, COAP_REQUEST_PUT, hnd_put_time);
    coap_register_handler(r, COAP_REQUEST_DELETE, hnd_delete_time);

    coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
    coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"Internal Clock\"", 16, 0);
    coap_add_attr(r, (unsigned char *)"rt", 2, (unsigned char *)"\"Ticks\"", 7, 0);
    r->observable = 1;
    coap_add_attr(r, (unsigned char *)"if", 2, (unsigned char *)"\"clock\"", 7, 0);

    coap_add_resource(ctx, r);
    time_resource = r;

#ifndef WITHOUT_ASYNC
    r = coap_resource_init((unsigned char *)"async", 5, 0);
    coap_register_handler(r, COAP_REQUEST_GET, hnd_get_async);

    coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
    coap_add_resource(ctx, r);
#endif /* WITHOUT_ASYNC */
}

static void
usage(const char *program, const char *version)
{
    const char *p;

    p = strrchr(program, '/');
    if (p) {
        program = ++p;
    }

    printf("%s v%s -- a small CoAP implementation\n"
           "(c) 2010,2011 Olaf Bergmann <bergmann@tzi.org>\n\n"
           "usage: %s [-A address] [-p port]\n\n"
           "\t-A address\tinterface address to bind to\n"
           "\t-p port\t\tlisten on specified port\n"
           "\t-v num\t\tverbosity level (default: 3)\n",
           program, version, program);
}

static coap_context_t *
get_context(const char *node, const char *port)
{
    coap_context_t *ctx = NULL;
    int s;
    struct addrinfo hints;
    struct addrinfo *result, *rp;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_DGRAM; /* Coap uses UDP */
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

    s = getaddrinfo(node, port, &hints, &result);
    if (s != 0) {
        printf("getaddrinfo: %s\n", strerror(s));
        return NULL;
    }

    /* iterate through results until success */
    for (rp = result; rp != NULL; rp = rp->ai_next) {
        coap_address_t addr;

        if (rp->ai_addrlen <= sizeof(addr.addr)) {
            coap_address_init(&addr);
            addr.size = rp->ai_addrlen;
            memcpy(&addr.addr, rp->ai_addr, rp->ai_addrlen);

            ctx = coap_new_context(&addr);
            if (ctx) {
                /* TODO: output address:port for successful binding */
                goto finish;
            }
        }
    }

    printf("no context available for interface '%s'\n", node);

finish:
    freeaddrinfo(result);
    return ctx;
}

int coap_server_dtls_test(void)
{
    dtls_context_t *the_context;
    coap_context_t  *ctx;
    fd_set readfds;
    struct timeval tv, *timeout;
    int result;
    coap_tick_t now;
    coap_queue_t *nextpdu;

    coap_log_t log_level = LOG_DEBUG;

    dtls_set_log_level((log_t)log_level);
    coap_set_log_level(log_level);

    ctx = get_context(NULL, COAP_DEFAULT_PORT_STR);
    if (!ctx) {
        return -1;
    }

    init_resources(ctx);

//  signal(SIGINT, handle_sigint);

    dtls_init();
    the_context = dtls_new_context(ctx);
    dtls_set_handler(the_context, &cb);
    coap_with_dtls(the_context);
    while (!quit) {
        FD_ZERO(&readfds);
        FD_SET(ctx->sockfd, &readfds);

        nextpdu = coap_peek_next(ctx);

        coap_ticks(&now);
        while (nextpdu && nextpdu->t <= now - ctx->sendqueue_basetime) {
            coap_retransmit(ctx, coap_pop_next(ctx));
            nextpdu = coap_peek_next(ctx);
        }

        if (nextpdu && nextpdu->t <= COAP_RESOURCE_CHECK_TIME) {
            /* set timeout if there is a pdu to send before our automatic timeout occurs */
            tv.tv_usec = ((nextpdu->t) % COAP_TICKS_PER_SECOND) * 1000000 / COAP_TICKS_PER_SECOND;
            tv.tv_sec = (nextpdu->t) / COAP_TICKS_PER_SECOND;
            timeout = &tv;
        } else {
            tv.tv_usec = 0;
            tv.tv_sec = COAP_RESOURCE_CHECK_TIME;
            timeout = &tv;
        }
        result = select(FD_SETSIZE, &readfds, 0, 0, timeout);

        if (result < 0) {		/* error */
            if (errno != EINTR) {
                printf("select");
            }
        } else if (result > 0) {	/* read from socket */
            if (FD_ISSET(ctx->sockfd, &readfds)) {
                dtls_handle_read(the_context);	/* read received data */
                // TODO: ADD THE DTLS READY EVENT!
                coap_dispatch(ctx);	/* and dispatch PDUs from receivequeue */
            }
        } else {			/* timeout */
            if (time_resource) {
                time_resource->dirty = 1;
            }
        }

#ifndef WITHOUT_ASYNC
        /* check if we have to send asynchronous responses */
        check_async(ctx, now);
#endif /* WITHOUT_ASYNC */

#ifndef WITHOUT_OBSERVE
        /* check if we have to send observe notifications */
        coap_check_notify(ctx);
#endif /* WITHOUT_OBSERVE */
    }

    debug("Freeing contexts\n");
    dtls_free_context(the_context);
    coap_free_context(ctx);

    return 0;
}
