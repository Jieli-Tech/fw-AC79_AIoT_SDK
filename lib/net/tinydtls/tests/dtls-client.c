#include "tinydtls.h"

/* This is needed for apple */
#define __APPLE_USE_RFC_3542

#include "printf.h"
//#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>

#include <lwip/sockets.h>
#include <lwip/netdb.h>

#include "global.h"
#include "dtls_debug.h"
#include "dtls.h"

#define NI_MAXHOST      1025
# define NI_MAXSERV      32

#define DEFAULT_PORT 20220

#define PSK_DEFAULT_IDENTITY "Client_identity"
#define PSK_DEFAULT_KEY      "secretPSK"
#define PSK_OPTIONS          "i:k:"

#ifdef __GNUC__
#define UNUSED_PARAM __attribute__((unused))
#else
#define UNUSED_PARAM
#endif /* __GNUC__ */

static char buf[1500];
static size_t len = 0;

typedef struct {
    size_t length;               /* length of string */
    unsigned char *s;            /* string data */
} dtls_str;

static dtls_str output_file = { 0, NULL }; /* output file name */

static dtls_context_t *dtls_context = NULL;


static const unsigned char ecdsa_priv_key[] = {
    0x41, 0xC1, 0xCB, 0x6B, 0x51, 0x24, 0x7A, 0x14,
    0x43, 0x21, 0x43, 0x5B, 0x7A, 0x80, 0xE7, 0x14,
    0x89, 0x6A, 0x33, 0xBB, 0xAD, 0x72, 0x94, 0xCA,
    0x40, 0x14, 0x55, 0xA1, 0x94, 0xA9, 0x49, 0xFA
};

static const unsigned char ecdsa_pub_key_x[] = {
    0x36, 0xDF, 0xE2, 0xC6, 0xF9, 0xF2, 0xED, 0x29,
    0xDA, 0x0A, 0x9A, 0x8F, 0x62, 0x68, 0x4E, 0x91,
    0x63, 0x75, 0xBA, 0x10, 0x30, 0x0C, 0x28, 0xC5,
    0xE4, 0x7C, 0xFB, 0xF2, 0x5F, 0xA5, 0x8F, 0x52
};

static const unsigned char ecdsa_pub_key_y[] = {
    0x71, 0xA0, 0xD4, 0xFC, 0xDE, 0x1A, 0xB8, 0x78,
    0x5A, 0x3C, 0x78, 0x69, 0x35, 0xA7, 0xCF, 0xAB,
    0xE9, 0x3F, 0x98, 0x72, 0x09, 0xDA, 0xED, 0x0B,
    0x4F, 0xAB, 0xC3, 0x6F, 0xC7, 0x72, 0xF8, 0x29
};

#ifdef DTLS_PSK

/* The PSK information for DTLS */
#define PSK_ID_MAXLEN 256
#define PSK_MAXLEN 256
static unsigned char psk_id[PSK_ID_MAXLEN];
static size_t psk_id_length = 0;
static unsigned char psk_key[PSK_MAXLEN];
static size_t psk_key_length = 0;

/* This function is the "key store" for tinyDTLS. It is called to
 * retrieve a key for the given identity within this particular
 * session. */
static int
get_psk_info(struct dtls_context_t *ctx UNUSED_PARAM,
             const session_t *session UNUSED_PARAM,
             dtls_credentials_type_t type,
             const unsigned char *id, size_t id_len,
             unsigned char *result, size_t result_length)
{

    switch (type) {
    case DTLS_PSK_IDENTITY:
        if (id_len) {
            dtls_debug("got psk_identity_hint: '%.*s'\n", id_len, id);
        }

        if (result_length < psk_id_length) {
            dtls_warn("cannot set psk_identity -- buffer too small\n");
            return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
        }

        memcpy(result, psk_id, psk_id_length);
        return psk_id_length;
    case DTLS_PSK_KEY:
        if (id_len != psk_id_length || memcmp(psk_id, id, id_len) != 0) {
            dtls_warn("PSK for unknown id requested, exiting\n");
            return dtls_alert_fatal_create(DTLS_ALERT_ILLEGAL_PARAMETER);
        } else if (result_length < psk_key_length) {
            dtls_warn("cannot set psk -- buffer too small\n");
            return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
        }

        memcpy(result, psk_key, psk_key_length);
        return psk_key_length;
    default:
        dtls_warn("unsupported request type: %d\n", type);
    }

    return dtls_alert_fatal_create(DTLS_ALERT_INTERNAL_ERROR);
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

static void
try_send(struct dtls_context_t *ctx, session_t *dst)
{
    int res;
    res = dtls_write(ctx, dst, (uint8 *)buf, len);
    if (res >= 0) {
        memmove(buf, buf + res, len - res);
        len -= res;
    }
}

static int dtls_event_cb(struct dtls_context_t *ctx, session_t *session,
                         dtls_alert_level_t level, unsigned short code)
{
    printf("dtls_event_cb->%d,%d\n", level, code);
    return 0;
}

static int
read_from_peer(struct dtls_context_t *ctx,
               session_t *session, uint8 *data, size_t len)
{
    size_t i;
    for (i = 0; i < len; i++) {
        printf("%c", data[i]);
    }
    return 0;
}

static int
send_to_peer(struct dtls_context_t *ctx,
             session_t *session, uint8 *data, size_t len)
{

    int fd = *(int *)dtls_get_app_data(ctx);
    return sendto(fd, data, len, MSG_DONTWAIT,
                  &session->addr.sa, session->size);
}

static int
dtls_handle_read(struct dtls_context_t *ctx)
{
    int fd;
    session_t session;
#define MAX_READ_BUF 2000
    static uint8 buf[MAX_READ_BUF];
    int len;

    fd = *(int *)dtls_get_app_data(ctx);

    if (!fd) {
        return -1;
    }

    memset(&session, 0, sizeof(session_t));
    session.size = sizeof(session.addr);
    len = recvfrom(fd, buf, MAX_READ_BUF, 0,
                   &session.addr.sa, &session.size);

    if (len < 0) {
        printf("recvfrom");
        return -1;
    } else {
        dtls_dsrv_log_addr(DTLS_LOG_DEBUG, "peer", &session);
        dtls_debug_dump("bytes from peer", buf, len);
    }

    return dtls_handle_message(ctx, &session, buf, len);
}

/* stolen from libcoap: */
static int
resolve_address(const char *server, struct sockaddr *dst)
{

    struct addrinfo *res, *ainfo;
    struct addrinfo hints;
    int error;

    if (server == NULL) {
        return -1;
    }

    memset((char *)&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;

    error = getaddrinfo(server, NULL, &hints, &res);

    if (error != 0) {
        printf("getaddrinfo: %s\n", strerror(error));
        return error;
    }

    for (ainfo = res; ainfo != NULL; ainfo = ainfo->ai_next) {

        switch (ainfo->ai_family) {
        case AF_INET6:
        case AF_INET:

            memcpy(dst, ainfo->ai_addr, ainfo->ai_addrlen);
            return ainfo->ai_addrlen;
        default:
            ;
        }
    }

    freeaddrinfo(res);
    return -1;
}

/*---------------------------------------------------------------------------*/
static void
usage(const char *program, const char *version)
{
    const char *p;

    p = strrchr(program, '/');
    if (p) {
        program = ++p;
    }

    printf("%s v%s -- DTLS client implementation\n"
           "(c) 2011-2014 Olaf Bergmann <bergmann@tzi.org>\n\n"
#ifdef DTLS_PSK
           "usage: %s [-i file] [-k file] [-o file] [-p port] [-v num] addr [port]\n"
#else /*  DTLS_PSK */
           "usage: %s [-o file] [-p port] [-v num] addr [port]\n"
#endif /* DTLS_PSK */
#ifdef DTLS_PSK
           "\t-i file\t\tread PSK identity from file\n"
           "\t-k file\t\tread pre-shared key from file\n"
#endif /* DTLS_PSK */
           "\t-o file\t\toutput received data to this file (use '-' for STDOUT)\n"
           "\t-p port\t\tlisten on specified port (default is %d)\n"
           "\t-v num\t\tverbosity level (default: 3)\n",
           program, version, program, DEFAULT_PORT);
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

#define DTLS_CLIENT_CMD_CLOSE "client:close"
#define DTLS_CLIENT_CMD_RENEGOTIATE "client:renegotiate"

int dtls_client_test(const char *cliaddr)
{
    fd_set rfds, efds;
    struct timeval timeout;
    log_t log_level = DTLS_LOG_WARN;//DTLS_LOG_DEBUG;
    int fd, result;
    int on = 1;
    int  res;
    session_t dst;

    dtls_init();

#ifdef DTLS_PSK
    psk_id_length = strlen(PSK_DEFAULT_IDENTITY);
    psk_key_length = strlen(PSK_DEFAULT_KEY);
    memcpy(psk_id, PSK_DEFAULT_IDENTITY, psk_id_length);
    memcpy(psk_key, PSK_DEFAULT_KEY, psk_key_length);
#endif /* DTLS_PSK */

    dtls_set_log_level(log_level);

    memset(&dst, 0, sizeof(session_t));
    /* resolve destination address where server should be sent */
    res = resolve_address(cliaddr, &dst.addr.sa);
    if (res < 0) {
        dtls_emerg("failed to resolve address\n");
        return (-1);
    }
    dst.size = res;

    /* use port number from command line when specified or the listen
       port, otherwise */
    dst.addr.sin.sin_port = htons(DEFAULT_PORT);

    /* init socket and set it to non-blocking */
    fd = socket(dst.addr.sa.sa_family, SOCK_DGRAM, 0);

    if (fd < 0) {
        dtls_alert("socket: %s\n", strerror(errno));
        return 0;
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        dtls_alert("setsockopt SO_REUSEADDR: %s\n", strerror(errno));
    }
#if 0
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        dtls_alert("fcntl: %s\n", strerror(errno));
        goto error;
    }
#endif
    on = 1;

#if LWIP_IPV6
#ifdef IPV6_RECVPKTINFO
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_RECVPKTINFO, &on, sizeof(on)) < 0) {
#else /* IPV6_RECVPKTINFO */
    if (setsockopt(fd, IPPROTO_IPV6, IPV6_PKTINFO, &on, sizeof(on)) < 0) {
#endif /* IPV6_RECVPKTINFO */
        dtls_alert("setsockopt IPV6_PKTINFO: %s\n", strerror(errno));
    }
#endif

//  if (signal(SIGINT, dtls_handle_signal) == SIG_ERR) {
//    dtls_alert("An error occurred while setting a signal handler.\n");
//    return EXIT_FAILURE;
//  }

    dtls_context = (dtls_context_t *)dtls_new_context(&fd);
    if (!dtls_context) {
        dtls_emerg("cannot create context\n");
        return (-1);
    }

    dtls_set_handler(dtls_context, &cb);

    dtls_connect(dtls_context, &dst);

    while (1) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        result = select(fd + 1, &rfds, NULL, 0, &timeout);

        if (result < 0) {		/* error */
            if (errno != EINTR) {
                printf("select");
            }
        } else if (result == 0) {	/* timeout */
            puts("dtls_client_timeout send zxczxczxcz\n");
            dtls_write(dtls_context, &dst, "zxczxczxcz", 1 + strlen("zxczxczxcz"));
        } else {			/* ok */
            if (FD_ISSET(fd, &rfds)) {
                dtls_handle_read(dtls_context);
            }
        }

        if (len) {
            if (len >= strlen(DTLS_CLIENT_CMD_CLOSE) &&
                !memcmp(buf, DTLS_CLIENT_CMD_CLOSE, strlen(DTLS_CLIENT_CMD_CLOSE))) {
                printf("client: closing connection\n");
                dtls_close(dtls_context, &dst);
                len = 0;
            } else if (len >= strlen(DTLS_CLIENT_CMD_RENEGOTIATE) &&
                       !memcmp(buf, DTLS_CLIENT_CMD_RENEGOTIATE, strlen(DTLS_CLIENT_CMD_RENEGOTIATE))) {
                printf("client: renegotiate connection\n");
                dtls_renegotiate(dtls_context, &dst);
                len = 0;
            } else {
                try_send(dtls_context, &dst);
            }
        }
    }

    dtls_free_context(dtls_context);

    return (0);
}

