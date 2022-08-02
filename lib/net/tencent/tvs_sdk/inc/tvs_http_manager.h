#ifndef __TVS_HTTP_MANAGER_H__
#define __TVS_HTTP_MANAGER_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct mg_connection mg_connection_t;

unsigned int tvs_htonl(unsigned int ip);

int tvs_http_send_normal_multipart_request(mg_connection_t *conn, const char *path, int path_len,
        const char *host, int host_len, const char *metadata);

int tvs_http_send_normal_get_request(mg_connection_t *conn, const char *path, int path_len,
                                     const char *host, int host_len);

int tvs_http_send_normal_post(mg_connection_t *conn, const char *path, int path_len,
                              const char *host, int host_len, const char *payload);

void tvs_http_send_multipart_start(mg_connection_t *conn, const char *path, int path_len,
                                   const char *host, int host_len, const char *metadata);

void tvs_http_send_multipart_data(mg_connection_t *conn, const char *bin_data, int bin_data_len);

void tvs_http_send_multipart_end(mg_connection_t *conn);

#endif
