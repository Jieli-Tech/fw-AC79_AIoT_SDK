#ifndef __TVS_MULTIPART_PARSER_FWEARFZFATA__
#define __TVS_MULTIPART_PARSER_FWEARFZFATA__

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


typedef enum {
    TVS_MULTIPART_CONTENT_TYPE_UNKNOWN,
    TVS_MULTIPART_CONTENT_TYPE_JSON,
    TVS_MULTIPART_CONTENT_TYPE_BINARY,
} tvs_multipart_content_type;

typedef void(*tvs_multipart_on_recv_metadata)(void *multipart_info, bool down_channel, const char *name, const char *metadata, int metadata_len);
typedef void(*tvs_multipart_on_recv_binary_begin)(void *multipart_info, bool down_channel);
typedef void(*tvs_multipart_on_recv_binary_chunked)(void *multipart_info, bool down_channel, const char *bin, int bin_len);
typedef void(*tvs_multipart_on_recv_binary_end)(void *multipart_info, bool down_channel);
typedef void(*tvs_multipart_on_all_complete)(void *multipart_info, bool down_channel);

typedef struct {
    tvs_multipart_on_recv_metadata on_recv_metadata;
    tvs_multipart_on_recv_binary_begin on_recv_binary_begin;
    tvs_multipart_on_recv_binary_chunked on_recv_binary_chunked;
    tvs_multipart_on_recv_binary_end on_recv_binary_end;
    tvs_multipart_on_all_complete on_all_complete;
} tvs_multipart_callback;

// 返回multipart_info指针
void *tvs_multipart_parser_init(const char *boundary, tvs_multipart_callback *callback, void *param, bool down_channel);

void tvs_multipart_parser_release(void *multipart_info);

size_t tvs_multipart_parser_excute(void *multipart_info, const char *chunk, int length);

void *tvs_multipart_parser_get_param(void *multipart_info);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
