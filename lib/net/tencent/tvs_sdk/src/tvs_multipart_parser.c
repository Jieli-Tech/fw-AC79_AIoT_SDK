#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#define TVS_LOG_DEBUG_MODULE  "MULTP"

#include "tvs_multipart_parser.h"

#include "multipart_parser.h"
#if _WIN32
#include "../common/mongoose.h"

#define tvs_str_casecmp _strnicmp
#else
#include "mongoose.h"
#define tvs_str_casecmp strncasecmp
#endif

#define TVS_LOG_DEBUG  0
#include "tvs_log.h"

#define CONTENT_TYPE_HEADER               "Content-Type"
#define CONTENT_TYPE_HEADER_JSON          "application/json"
#define CONTENT_TYPE_HEADER_DATA          "application/octet-stream"

typedef struct {
    multipart_parser *parser;
    multipart_parser_settings settings;
    tvs_multipart_callback *callback;
    tvs_multipart_content_type content_type;
    struct mbuf buffer;
    void *param;
    struct mbuf header;
    bool down_channel;
    int type;
} tvs_mutipart_info;

static int mul_on_body_end(multipart_parser *parser)
{
    TVS_LOG_PRINTF("call %s\n", __FUNCTION__);
    if (NULL == parser) {
        return 0;
    }

    void *data = multipart_parser_get_data(parser);
    if (NULL == data) {
        return 0;
    }
    tvs_mutipart_info *info = (tvs_mutipart_info *)data;
    if (info->callback != NULL && info->callback->on_all_complete != NULL) {
        info->callback->on_all_complete(info, info->down_channel);
    }

    mbuf_free(&info->buffer);
    mbuf_free(&info->header);
    return 0;
}

static int mul_on_headers_complete(multipart_parser *parser)
{
    TVS_LOG_PRINTF("call %s\n", __FUNCTION__);
    if (NULL == parser) {
        return 0;
    }
    void *data = multipart_parser_get_data(parser);
    if (NULL == data) {
        return 0;
    }

    tvs_mutipart_info *info = (tvs_mutipart_info *)data;
    if (info->callback == NULL) {
        return 0;
    }
    switch (info->content_type) {
    case TVS_MULTIPART_CONTENT_TYPE_BINARY:
        if (info->callback != NULL && info->callback->on_recv_binary_begin != NULL) {
            info->callback->on_recv_binary_begin(info, info->down_channel);
        }
        break;
    default:
        break;
    }

    return 0;
}

static int mul_on_header_field(multipart_parser *parser, const char *at, size_t length)
{
    TVS_LOG_PRINTF("call %s -- %.*s\n", __FUNCTION__, length, at);

    if (NULL == parser || NULL == at || 0 == length) {
        return 0;
    }

    void *data = multipart_parser_get_data(parser);
    if (NULL == data) {
        return 0;
    }

    tvs_mutipart_info *info = (tvs_mutipart_info *)data;

    if (info->content_type == TVS_MULTIPART_CONTENT_TYPE_UNKNOWN) {
        mbuf_init(&info->header, 0);
    }
    return 0;
}

static int mul_on_header_value(multipart_parser *parser, const char *at, size_t length)
{
    TVS_LOG_PRINTF("call %s -- %.*s\n", __FUNCTION__, length, at);

    if (NULL == parser || NULL == at || 0 == length) {
        return 0;
    }

    void *data = multipart_parser_get_data(parser);
    if (NULL == data) {
        return 0;
    }

    tvs_mutipart_info *info = (tvs_mutipart_info *)data;

    if (info->content_type != TVS_MULTIPART_CONTENT_TYPE_UNKNOWN) {
        return 0;
    }

    mbuf_append(&info->header, at, length);

    length = info->header.len;
    at = info->header.buf;

    TVS_LOG_PRINTF("call %s >>> header %.*s\n", __FUNCTION__, length, at);

    int cthlen = strlen(CONTENT_TYPE_HEADER_JSON);
    if (length >= cthlen && 0 == tvs_str_casecmp(at, CONTENT_TYPE_HEADER_JSON, cthlen)) {
        info->content_type = TVS_MULTIPART_CONTENT_TYPE_JSON;
        TVS_LOG_PRINTF("call %s, find json part header\n", __FUNCTION__);
        return 0;
    }

    cthlen = strlen(CONTENT_TYPE_HEADER_DATA);
    if (length >= cthlen && 0 == tvs_str_casecmp(at, CONTENT_TYPE_HEADER_DATA, cthlen)) {
        info->content_type = TVS_MULTIPART_CONTENT_TYPE_BINARY;
        TVS_LOG_PRINTF("call %s, find binary part header\n", __FUNCTION__);
        return 0;
    }

    return 0;
}

static int mul_on_part_data(multipart_parser *parser, const char *at, size_t length)
{
    if (NULL == parser || NULL == at || 0 == length) {
        return 0;
    }

    void *data = multipart_parser_get_data(parser);
    if (NULL == data) {
        return 0;
    }

    tvs_mutipart_info *info = (tvs_mutipart_info *)data;

    switch (info->content_type) {
    case TVS_MULTIPART_CONTENT_TYPE_JSON:
        mbuf_append(&info->buffer, at, length);
        break;
    case TVS_MULTIPART_CONTENT_TYPE_BINARY:
        if (info->callback != NULL && info->callback->on_recv_binary_chunked != NULL) {
            info->callback->on_recv_binary_chunked(info, info->down_channel, at, length);
        }
        break;
    default:
        break;
    }
    return 0;
}

static int mul_on_part_data_begin(multipart_parser *parser)
{
    TVS_LOG_PRINTF("call %s\n", __FUNCTION__);
    if (NULL == parser) {
        return 0;
    }

    void *data = multipart_parser_get_data(parser);
    if (NULL == data) {
        return 0;
    }

    tvs_mutipart_info *info = (tvs_mutipart_info *)data;
    info->content_type = TVS_MULTIPART_CONTENT_TYPE_UNKNOWN;
    mbuf_free(&info->buffer);
    mbuf_free(&info->header);
    return 0;
}

static int mul_on_part_data_end(multipart_parser *parser)
{
    TVS_LOG_PRINTF("call %s\n", __FUNCTION__);
    if (NULL == parser) {
        return 0;
    }

    void *data = multipart_parser_get_data(parser);
    if (NULL == data) {
        return 0;
    }

    tvs_mutipart_info *info = (tvs_mutipart_info *)data;
    if (info->callback == NULL) {
        return 0;
    }
    switch (info->content_type) {
    case TVS_MULTIPART_CONTENT_TYPE_JSON:
        if (info->callback != NULL && info->callback->on_recv_metadata != NULL) {
            info->callback->on_recv_metadata(info, info->down_channel, "", info->buffer.buf, info->buffer.len);
        }
        break;
    case TVS_MULTIPART_CONTENT_TYPE_BINARY:
        if (info->callback != NULL && info->callback->on_recv_binary_end != NULL) {
            info->callback->on_recv_binary_end(info, info->down_channel);
        }
        break;
    default:
        break;
    }

    mbuf_free(&info->buffer);
    return 0;
}

void *tvs_multipart_parser_init(const char *boundary, tvs_multipart_callback *callback, void *param, bool down_channel)
{
    if (NULL == callback || callback == NULL) {
        return NULL;
    }

    tvs_mutipart_info *mutipart_info = TVS_MALLOC(sizeof(tvs_mutipart_info));
    memset(mutipart_info, 0, sizeof(tvs_mutipart_info));
    mutipart_info->settings.on_body_end = mul_on_body_end;
    mutipart_info->settings.on_headers_complete = mul_on_headers_complete;
    mutipart_info->settings.on_header_field = mul_on_header_field;
    mutipart_info->settings.on_header_value = mul_on_header_value;
    mutipart_info->settings.on_part_data = mul_on_part_data;
    mutipart_info->settings.on_part_data_begin = mul_on_part_data_begin;
    mutipart_info->settings.on_part_data_end = mul_on_part_data_end;
    mutipart_info->parser = multipart_parser_init(boundary, &mutipart_info->settings);
    mutipart_info->callback = callback;
    mutipart_info->param = param;
    mutipart_info->down_channel = down_channel;
    multipart_parser_set_data(mutipart_info->parser, mutipart_info);

    return mutipart_info;
}

size_t tvs_multipart_parser_excute(void *multipart_info, const char *chunk, int length)
{
    if (NULL == multipart_info) {
        return 0;
    }

    tvs_mutipart_info *info = (tvs_mutipart_info *)multipart_info;
    if (NULL == info->parser) {
        return 0;
    }
    return multipart_parser_execute(info->parser, chunk, length);
}

void tvs_multipart_parser_release(void *multipart_info)
{
    if (NULL == multipart_info) {
        return;
    }

    tvs_mutipart_info *info = (tvs_mutipart_info *)multipart_info;

    multipart_parser_free(info->parser);

    mbuf_free(&info->buffer);
    mbuf_free(&info->header);

    TVS_FREE(multipart_info);
}

void *tvs_multipart_parser_get_param(void *multipart_info)
{
    if (NULL == multipart_info) {
        return NULL;
    }

    tvs_mutipart_info *info = (tvs_mutipart_info *)multipart_info;
    return info->param;
}

