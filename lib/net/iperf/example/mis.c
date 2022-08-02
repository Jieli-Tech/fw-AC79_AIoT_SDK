#include "iperf_config.h"


#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include <iperf_api.h>

int
iperf_mis_main(int argc, char **argv)
{
    char *argv0;
    int port;
    struct iperf_test *test;
    int consecutive_errors;

    argv0 = strrchr(argv[0], '/');
    if (argv0 != (char *) 0) {
        ++argv0;
    } else {
        argv0 = argv[0];
    }

    if (argc != 2) {
        printf("usage: %s [port]\n", argv0);
        return (EXIT_FAILURE);
    }
    port = atoi(argv[1]);

    test = iperf_new_test();
    if (test == NULL) {
        printf("%s: failed to create test\n", argv0);
        return (EXIT_FAILURE);
    }
    iperf_defaults(test);
    iperf_set_verbose(test, 1);
    iperf_set_test_role(test, 's');
    iperf_set_test_server_port(test, port);

    consecutive_errors = 0;
    for (;;) {
        if (iperf_run_server(test) < 0) {
            printf("%s: error - %s\n\n", argv0, iperf_strerror(i_errno));
            ++consecutive_errors;
            if (consecutive_errors >= 5) {
                printf("%s: too many errors, exiting\n", argv0);
                break;
            }
        } else {
            consecutive_errors = 0;
        }
        iperf_reset_test(test);
    }

    iperf_free_test(test);
    return (EXIT_SUCCESS);
}

