#include "http_cli.h"
#include "vt_types.h"
#include "os/os_api.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"

static httpcli_ctx s_ctx;

struct MemoryStruct {
    char *memory;
    size_t size;
};

static char cookie[256];

#define BOUNDARY        			"----WebKitFormBoundary7MA4YWxkTrZu0gW"

#ifdef HTTP_KEEP_ALIVE
#define HTTP_HEAD_OPTION        \
"POST /%s HTTP/1.1\r\n"\
"Host: %s\r\n"\
"Cookie: %s\r\n"\
"Content-Type: multipart/form-data; boundary="BOUNDARY"\r\n"\
"Content-Length: %lu\r\n"\
"Expect: 100-continue\r\n"\
"Connection: keep-alive\r\n\r\n"
#else
#define HTTP_HEAD_OPTION        \
"POST /%s HTTP/1.1\r\n"\
"Host: %s\r\n"\
"Cookie: %s\r\n"\
"Content-Type: multipart/form-data; boundary="BOUNDARY"\r\n"\
"Content-Length: %lu\r\n"\
"Expect: 100-continue\r\n"\
"Connection: close\r\n\r\n"
#endif


#define BODY_OPTION_ONE                                                     \
"--"BOUNDARY"\r\n"                                                          \
"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"  \
"Content-Type: %s\r\n\r\n"

#define BODY_OPTION_TWO       \
"\r\n--"BOUNDARY"--\r\n"

#if 0
#define BODY_OPTION_TWO       \
"\r\n--"BOUNDARY"\r\n"\
"Content-Disposition: form-data; name=\"bookId\"\r\n\r\n"\
"%d"\
"\r\n--"BOUNDARY"\r\n"\
"Content-Disposition: form-data; name=\"dstate\"\r\n\r\n"\
"%d"\
"\r\n--"BOUNDARY"--\r\n"
#endif


static int httpsin_user_cb(void *_ctx, void *buf, u32 size, void *priv, httpin_status status)
{
    http_body_obj *b = (http_body_obj *)priv;
    httpcli_ctx *ctx = (httpcli_ctx *)_ctx;

    if (!b) {
        printf("httpsin_default_cb http_body_obj buffer is null!\n");
        return -1;
    }

    switch (status) {
    case HTTPIN_HEADER:
        if (ctx->content_length >= 0) {
            printf("HTTPIN_HEADER content_length = %d\n", ctx->content_length);
            if (ctx->content_length == 0) {
                return -1;
            }
            if (ctx->content_length > b->buf_len * b->buf_count - 32) {
                b->p = realloc(b->p, ctx->content_length + 32);
                printf("realloc!!!\n");
                if (b->p == NULL) {
                    printf("httpsin_default_cb realloc memory error!\n");
                    return -1;
                }
                b->buf_len = ctx->content_length + 32;
                b->buf_count = 1;
            }
        }

        if (ctx->content_type[0]) {
            printf("HTTPIN_HEADER content_type = %s\n", ctx->content_type);
        }
        if (ctx->transfer_encoding[0]) {
            printf("HTTPIN_HEADER transfer_encoding = %s\n", ctx->transfer_encoding);
        }

        printf("\nhead_buf = %s\n", buf);
        char *str_b, *str_e;
        str_b = strstr(buf, "Set-Cookie: ");
        if (str_b) {
            str_e = strchr(str_b, ';');
            ++str_e;
            memcpy(cookie, str_b + strlen("Set-Cookie: "), str_e - str_b - strlen("Set-Cookie: "));
            printf("\ncookie = %s\n", cookie);
        }

        break;
    case HTTPIN_PROGRESS:
        if (b->recv_len + size >= b->buf_len * b->buf_count - 4) {
            do {
                b->buf_count++;
            } while (b->recv_len + size >= b->buf_len * b->buf_count);
            b->p = realloc(b->p, b->buf_len * b->buf_count);
            printf("realloc!!!\n");
            if (b->p == NULL) {
                return -1;
            }
        }
        memcpy(b->p + b->recv_len, buf, size);
        b->recv_len += size;
#if  0
        // http接收到数据后写文件
        if (priv) {
            fwrite(priv, buf, size);
        }
#endif
        break;
    case HTTPIN_FINISHED:
        printf("HTTPIN_FINISHED\n");
        *(b->p + b->recv_len) = 0;
        //printf("\n\n%s\n", b->p);
        if (!strcmp(ctx->transfer_encoding, "chunked")) {
            if (-1 == transfer_chunked_analysis(b->p, b->recv_len)) {
                return HERROR_BODY_ANALYSIS;
            }
        }
        break;
    case HTTPIN_ABORT:
        printf("HTTPIN_ABORT\n");
        break;
    case HTTPIN_ERROR:
        printf("HTTPIN_ERROR\n");
        break;
    case HTTPIN_NON_BLOCK:
        //printf("HTTPIN_NON_BLOCK\n");
        break;
    default:
        break;
    }

    return 0;
}

//如果服务器说没有配置文件，就报错
static int logon_response_parase(const char *resp_buf)
{
//{"code":309,"msg":"No more license can be used","data":null}
    int err = 0;
    json_object *__new_obj = NULL;
    json_object *__code = NULL;
    json_object *__msg = NULL;

    __new_obj = json_tokener_parse(resp_buf);
    if (!__new_obj) {
        return -1;
    }
    if (!json_object_object_get_ex(__new_obj, "code", &__code)) {
        goto __result_exit;
    }
    if (!json_object_object_get_ex(__new_obj, "msg", &__msg)) {
        goto __result_exit;
    }

    if (!strcmp(json_object_get_string(__msg), "No more license can be used")) {
        //出问题了
        printf("no licnese\n");
        err = -1;
    }

    int code = json_object_get_int(__code);
    if (code == 309) {
        //出问题了
        printf("code err\n");
        err = -1;
    }

__result_exit:
    json_object_put(__new_obj);

    return err;
}

static int wt_http_get_(str_vt_networkPara *param, struct MemoryStruct *chunk, int set_cookie)
{
    int error = 0;
    http_body_obj http_body_buf;

    printf("wt get url: %s\n", param->url);

    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (!ctx) {
        return -1;
    }

    memset(&http_body_buf, 0x0, sizeof(http_body_obj));
    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = 8 * 1024;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *)malloc(http_body_buf.buf_len * sizeof(char));
    if (http_body_buf.p == NULL) {
        free(ctx);
        ctx = NULL;
        return -1;
    }

    if (set_cookie) {
        ctx->cookie = cookie;
    }
    ctx->url = param->url;
    ctx->priv = &http_body_buf;
    ctx->connection = "close";
    ctx->timeout_millsec = param->timeout * 1000;
    ctx->cb = httpsin_user_cb;
    error = httpcli_get(ctx);
    if (error == HERROR_OK) {
        chunk->memory = http_body_buf.p;
        chunk->size = http_body_buf.recv_len;
        chunk->memory[chunk->size] = 0;
        printf("login result chunk.memory = %s\n", chunk->memory);
        logon_response_parase(chunk->memory);
    } else {
        error = -1;
    }

    free(ctx);

    return error;
}

static int requestServerVDS(str_vt_networkPara *param, struct MemoryStruct *chunk)
{
    int error = -1;
    int user_head_buf_len = 1024 * 2;
    int url_len = 1460;
    char http_body_begin[320];
    char *url = NULL;
    char *user_head_buf = NULL;
    httpcli_ctx *ctx = &s_ctx;
    const char *http_body_end = BODY_OPTION_TWO;
    u8 first_try = 1;

    if (NULL == param->url) {
        return -1;
    }

    printf("VDS request:url=%s\n", param->url);

    http_body_obj http_body_buf;

    memset(&http_body_buf, 0x0, sizeof(http_body_obj));

    user_head_buf = calloc(1, user_head_buf_len);
    if (!user_head_buf) {
        return -1;
    }

    url = calloc(1, url_len);
    if (!url) {
        free(user_head_buf);
        return -1;
    }

    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = 1024 * 8;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *)malloc(http_body_buf.buf_len * sizeof(char));
    if (http_body_buf.p == NULL) {
        goto __exit;
    }

    snprintf(http_body_begin, sizeof(http_body_begin), BODY_OPTION_ONE, param->mime_name, param->mime_filename, param->mime_type);

    //body的传输方式
    int http_more_data_addr[3];
    int http_more_data_len[3 + 1];
    http_more_data_addr[0] = (int)(http_body_begin);
    http_more_data_addr[1] = (int)(param->mime_data);
    http_more_data_addr[2] = (int)(http_body_end);
    http_more_data_len[0] = strlen(http_body_begin);
    http_more_data_len[1] = param->data_size;
    http_more_data_len[2] = strlen(http_body_end);
    http_more_data_len[3] = 0;

    snprintf(url, url_len, "%s", param->url);
    const char *host = url + strlen("http://");
    char *path = strchr(host, '/');
    if (!host || !path) {
        goto __exit;
    }
    *path++ = '\0';

    snprintf(user_head_buf, user_head_buf_len, HTTP_HEAD_OPTION, path, host, cookie, strlen(http_body_begin) + param->data_size + strlen(http_body_end));

    /* extern u32 timer_get_ms(void); */
    /* u32 time = timer_get_ms(); */
#ifdef HTTP_KEEP_ALIVE
__req_again:
    if (ctx->sock_hdl == NULL) {
        memset(ctx, 0x0, sizeof(httpcli_ctx));
        ctx->url = param->url;
        ctx->timeout_millsec = param->timeout * 1000;
        ctx->priv = &http_body_buf;
        error = httpcli_post_keepalive_init(ctx);
        if (error != HERROR_OK) {
            goto __exit;
        }
    }
#else
    memset(ctx, 0x0, sizeof(httpcli_ctx));
#endif

    ctx->more_data = http_more_data_addr;
    ctx->more_data_len = http_more_data_len;
    ctx->timeout_millsec = param->timeout * 1000;
    ctx->priv = &http_body_buf;
    ctx->url = param->url;
    ctx->user_http_header = user_head_buf;

#ifdef HTTP_KEEP_ALIVE
    error = httpcli_post_keepalive_send(ctx);
#else
    error = httpcli_post(ctx);
#endif
    if (error == HERROR_OK) {
        chunk->memory = http_body_buf.p;
        chunk->size = http_body_buf.recv_len;
        chunk->memory[chunk->size] = 0;
        /* printf("^^^^^^^^^^^^^^^^^^ time : %d ^^^^^^^^^^^^^\n", timer_get_ms() - time); */
        printf("result chunk.memory = %s\n", chunk->memory);
    } else {
#ifdef HTTP_KEEP_ALIVE
        if (first_try && error != HERROR_CALLBACK) {
            first_try = 0;
            goto __req_again;
        }
#endif
        error = -1;
    }

__exit:
    if (url) {
        free(url);
    }
    if (user_head_buf) {
        free(user_head_buf);
    }
    if (error && http_body_buf.p) {
        free(http_body_buf.p);
    }

    return error;
}

static int wt_http_post_(str_vt_networkPara *param, struct MemoryStruct *chunk)
{
    int error = 0;
    http_body_obj http_body_buf;

    if (param->method != POST) {
        return -1;
    }

    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (!ctx) {
        return -1;
    }

    memset(&http_body_buf, 0x0, sizeof(http_body_obj));
    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = 8 * 1024;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *)malloc(http_body_buf.buf_len * sizeof(char));
    if (http_body_buf.p == NULL) {
        free(ctx);
        ctx = NULL;
        return -1;
    }

    ctx->url = param->url;
    ctx->priv = &http_body_buf;
    ctx->post_data = param->mime_data;
    ctx->data_len = param->data_size;
    ctx->data_format = "application/json";
    ctx->connection = "close";
    ctx->cookie = cookie;
    ctx->timeout_millsec = param->timeout * 1000;

    printf("postDataToServer url=%s perform_time_out: %d\n", param->url, param->timeout);

    error = httpcli_post(ctx);
    if (error == HERROR_OK) {
        chunk->memory = http_body_buf.p;
        chunk->size = http_body_buf.recv_len;
        chunk->memory[chunk->size] = 0;
        printf("result chunk.memory = %s\n", chunk->memory);
    } else {
        error = -1;
    }

    free(ctx);

    return error;
}

int vt_httpRequest_cb(str_vt_networkPara *param, str_vt_networkRespond *res)
{
    int ret = 0;
    struct MemoryStruct chunk = {NULL, 0};//chunk->memory 如果不为空 由SDK释放此内存
    ret = wt_http_get_(param, &chunk, 1);
    res->data = chunk.memory;
    res->size = chunk.size;
    return ret;
}

int vt_reconghttpRequest_cb(str_vt_networkPara *param, str_vt_networkRespond *res)
{
    int ret = 0;
    struct MemoryStruct chunk = {NULL, 0};//chunk->memory 如果不为空 由SDK释放此内存
    ret = requestServerVDS(param, &chunk);
    res->data = chunk.memory;
    res->size = chunk.size;
    return ret;
}

int vt_httpCommonRequest_cb(str_vt_networkPara *param, str_vt_networkRespond *res)
{
    int ret = 0;
    struct MemoryStruct chunk = {NULL, 0};//chunk->memory 如果不为空 由SDK释放此内存
    if (param->method == POST) {
        ret = wt_http_post_(param, &chunk);
    } else {
        ret = wt_http_get_(param, &chunk, 0);
    }
    res->data = chunk.memory;
    res->size = chunk.size;
    return ret;
}

void vt_reconghttpRequest_http_request_exit(void)
{
    http_cancel_dns(&s_ctx);
}

void vt_reconghttpRequest_http_close(void)
{
    httpcli_close(&s_ctx);
}

void vt_reconghttpRequest_http_request_break(void)
{
    /* s_ctx.req_exit_flag = HTTPCLI_REQ_EXIT; */
    httpcli_cancel(&s_ctx);
}
