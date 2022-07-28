#include  "http-c_app.h"
#include  "http-c_hooks.h"
#include  "static_files.h"

#if (HTTPc_APP_FS_MODULE_PRESENT == DEF_YES)
#include  <fs_app.h>
#endif

#include "http_cli.h"
#include <stdlib.h>
#include "circular_buf.h"

#ifdef  HTTP_CLI_C_EN

typedef unsigned int u32_t;
typedef signed int s32_t;
typedef unsigned char u8_t;

/*******************************************************/
#define POST_DATA_SIZE 4096
#define LARGE_CLIENT_HEADER 512 //http 头部长度
/*******************************************************/
extern const struct net_http_socket_ops net_sock_ops_https;

/*函数声明*/
static void *uc_httpin_sock_create(httpcli_ctx *ctx, char *buf, u32_t bufsize);
s32_t uchttpin_get_host_from_url(httpcli_ctx *ctx, char *buf, u32_t bufsize);
static void uc_sock_send(void);
static void uc_sock_recv(void);
static void uc_sock_unreg(void *hdl);
extern CPU_BOOLEAN HTTPc_Is_InitDone(void);

static const struct net_http_socket_ops net_sock_ops_http = {
    .sock_create = uc_httpin_sock_create,
    .sock_send = uc_sock_send,
    .sock_recv = uc_sock_recv,
    .sock_close = uc_sock_unreg,
};

CPU_BOOLEAN  UCHTTPc_Init(void)
{
    CPU_BOOLEAN  success;
    HTTPc_ERR    httpc_err;

    HTTPc_Init(&HTTPc_Cfg, DEF_NULL, DEF_NULL, &httpc_err);
    if (httpc_err != HTTPc_ERR_NONE) {
        return (DEF_FAIL);
    }

    (void)success;

    return (DEF_OK);
}

static void *uc_httpin_sock_create(httpcli_ctx *ctx, char *buf, u32_t bufsize)
{
    char *pA = NULL;
    int port;

    HTTPc_CONN_OBJ         *http_conn = NULL;
    HTTPc_ERR              err;
    CPU_SIZE_T             str_len;
    CPU_BOOLEAN            result;


    if (HTTPc_Is_InitDone() != DEF_YES) {
        HTTPc_Init(&HTTPc_Cfg, DEF_NULL, DEF_NULL, &err);
        if (err != HTTPc_ERR_NONE) {
            HTTPc_APP_TRACE_INFO(("HTTPc_Init fail.\n\r"));
            return NULL;
        }
    }

    /*获取hostname*/
    if (0 != uchttpin_get_host_from_url(ctx, ctx->host_name, sizeof(ctx->host_name))) {
        HTTPc_APP_TRACE_INFO(("uchttpin_get_host_from_url fail.\n\r"));
        return NULL;
    }

    /*创建连接对象*/
    http_conn = (HTTPc_CONN_OBJ *)malloc(sizeof(HTTPc_CONN_OBJ));

    Mem_Clr(http_conn, sizeof(HTTPc_CONN_OBJ));
    HTTPc_ConnClr(http_conn, &err);
    if (err != HTTPc_ERR_NONE) {
        goto EXIT;
    }

    /*如果有端口号，进行端口号设置，否则默认为80端口*/
    pA = strstr(ctx->url, ctx->host_name) + strlen(ctx->host_name);
    if (':' == *pA) {
        pA ++;
        port = atoi(pA);
        /* printf("uchttpin_get_host_from_url %d", port); */

        /*设置端口*/
        HTTPc_ConnSetParam(http_conn,
                           HTTPc_PARAM_TYPE_SERVER_PORT,
                           &port,
                           &err);
        if (err != HTTPc_ERR_NONE) {
            HTTPc_APP_TRACE_INFO(("HTTPc_PARAM_TYPE_SERVER_PORT err.\n\r"));
            goto EXIT;
        }
    }

    /*增加用户参数项*/
    HTTPc_ConnSetParam(http_conn,
                       HTTPc_PARAM_TYPE_USER_SECURE_DATA,
                       ctx,
                       &err);
    if (err != HTTPc_ERR_NONE) {
        HTTPc_APP_TRACE_INFO(("HTTPc_ConnSetParam.\n\r"));
        goto EXIT;
    }

    str_len = Str_Len(ctx->host_name);
    result  = HTTPc_ConnOpen(http_conn,
                             buf,
                             bufsize,
                             ctx->host_name,
                             str_len,
                             HTTPc_FLAG_NONE,
                             &err);
    if (err != HTTPc_ERR_NONE) {
        HTTPc_APP_TRACE_INFO(("HTTPc_ConnOpen err.\n\r"));
        goto EXIT;
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("Connection to server succeeded.\n\r"));
    } else {
        HTTPc_APP_TRACE_INFO(("Connection to server failed.\n\r"));
        goto EXIT;
    }

    return http_conn;

EXIT:
    if (http_conn) {
        free(http_conn);
    }

    return NULL;
}

/*todo*/
static void uc_sock_send(void)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
}

/*todo*/
static void uc_sock_recv(void)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
}

void uc_sock_unreg(void *hdl)
{
    HTTPc_ERR  err;
    HTTPc_CONN_OBJ  *p_conn = (HTTPc_CONN_OBJ *)hdl;

    HTTPc_ConnClose(p_conn, HTTPc_FLAG_NONE, &err);
    if (err != HTTPc_ERR_NONE) {
        HTTPc_APP_TRACE_INFO(("uc_sock_unreg err.\n\r"));
    }
}

/*todo*/
static s32_t uchttpin_default_cb(void *_ctx, void *buf, u32_t size, void *priv, httpin_status status)
{
    http_body_obj *b = (http_body_obj *)priv;
    httpcli_ctx *ctx = (httpcli_ctx *)_ctx;

    if (!b) {
        return -1;
    }

    switch (status) {
    case HTTPIN_HEADER:
        printf(">>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
        break;

    case HTTPIN_PROGRESS:
        printf(">>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
        break;

    case HTTPIN_FINISHED:
        printf(">>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
        break;

    case HTTPIN_ABORT:
        printf(">>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
        break;

    case HTTPIN_ERROR:
        printf(">>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
        break;

    case HTTPIN_NON_BLOCK:
        printf(">>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
        break;

    default:
        break;
    }

    return 0;
}

static int httpcli_get_protocol(httpcli_ctx *ctx)
{
    if (NULL == ctx->url) {
        return -1;
    }

    if (!strncmp(ctx->url, "http://", strlen("http://"))) {
        ctx->mode = MODE_HTTP;
        ctx->ops = &net_sock_ops_http;
    } else if (!strncmp(ctx->url, "https://", strlen("https://"))) {
        ctx->mode = MODE_HTTPS;
        ctx->ops = &net_sock_ops_http;
    } else {
        return -1;
    }

    return 0;
}

s32_t uchttpin_get_host_from_url(httpcli_ctx *ctx, char *buf, u32_t bufsize)
{
    u32_t cnt;
    char *host;
    char *pA;
    const char *HTTP_PROTOCAL = NULL;

    if (ctx->mode == MODE_HTTP) {
        HTTP_PROTOCAL = "http://";
    } else {
        HTTP_PROTOCAL = "https://";
    }

    host = strstr(ctx->url, HTTP_PROTOCAL);
    if (NULL == host) {
        return -1;
    }

    host += strlen(HTTP_PROTOCAL);
    pA = strpbrk(host, ":/");
    if (NULL == pA) {
        cnt = strlen(host);
    } else {
        cnt = pA - host;
    }
    if (cnt >= bufsize) {
        return -2;
    }

    memcpy(buf, host, cnt);
    buf[cnt] = 0;

    return 0;
}

/*成功接收到数据后会调用该钩子函数*/
CPU_INT32U HTTPc_POST_RespBodyHook(HTTPc_CONN_OBJ     *p_conn,
                                   HTTPc_REQ_OBJ      *p_req,
                                   HTTP_CONTENT_TYPE   content_type,
                                   void               *p_data,
                                   CPU_INT16U          data_len,
                                   CPU_BOOLEAN         last_chunk)
{
    httpcli_ctx *ctx = p_conn->UserData;
    http_body_obj *uchttp_recv = (http_body_obj *)ctx->priv;

    /*对接收的数据进行处理*/
    if (uchttp_recv && uchttp_recv->p) {
        if ((data_len + 1) < uchttp_recv->buf_len * uchttp_recv->buf_count) {
            Mem_Copy(uchttp_recv->p + uchttp_recv->recv_len, p_data, data_len);
            uchttp_recv->recv_len += data_len;
        }
    }

    /*判断是否为最后一个数据块*/
    if (last_chunk == DEF_YES) {
        uchttp_recv->p[uchttp_recv->recv_len] = '\0';
    }

    return data_len;
}

/*获取发送的数据正文钩子函数*/
static CPU_BOOLEAN  POST_Buf_ReqBodyHook(HTTPc_CONN_OBJ     *p_conn,
        HTTPc_REQ_OBJ      *p_req,
        void              **p_data,
        CPU_CHAR           *p_buf,
        CPU_INT16U          buf_len,
        CPU_INT16U         *p_data_len)
{
    CPU_SIZE_T str_len;

    *p_data = p_req->UserDataPtr;

    str_len = Str_Len(*p_data);
    *p_data_len = str_len;

    Str_Copy_N(p_buf, *p_data, str_len);

    return (DEF_YES);
}

/*post*/
httpin_error httpcli_post(httpcli_ctx *ctx)
{
    int ret;
    HTTPc_CONN_OBJ  *http_conn = NULL;
    HTTPc_REQ_OBJ    http_req;  /*请求对象*/
    HTTPc_RESP_OBJ   http_resp;
    HTTP_CONTENT_TYPE content_type;
    HTTPc_ERR              err;
    CPU_BOOLEAN            result;
    char *pA = NULL;
    char *sA = NULL;
    CPU_SIZE_T resource_path_len = 0;
    int post_data_size = 0;

    if (httpcli_get_protocol(ctx)) {
        ret = HERROR_PARAM;
        goto EXIT;
    }

    ret = HERROR_OK;

    /*如果没有指定数据长度，默认发送缓存区为4k*/
    if (ctx->data_len) {
        ctx->p_buf = (char *)malloc(LARGE_CLIENT_HEADER + ctx->data_len);
        post_data_size = (LARGE_CLIENT_HEADER + ctx->data_len);
    } else {
        ctx->p_buf = (char *)malloc(POST_DATA_SIZE);
        post_data_size = POST_DATA_SIZE;
    }

    if (ctx->p_buf == NULL) {
        HTTPc_APP_TRACE_INFO(("malloc fail.\n\r"));
        return HERROR_MEM;
    }

    memset(ctx->p_buf, 0, post_data_size);

    if (NULL == ctx->cb) {
        ctx->cb = uchttpin_default_cb;
    }

    http_conn = (HTTPc_CONN_OBJ *)ctx->ops->sock_create(ctx, ctx->p_buf, post_data_size);
    if (!http_conn) {
        HTTPc_APP_TRACE_INFO(("httpcli_post sock_create fail.\n\r"));
        goto EXIT;
    }

    Mem_Clr(&http_req,  sizeof(http_req));

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        ret = HERROR_SOCK;
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqClr fail.\n\r"));
        goto EXIT;
    }

    if (ctx->post_data != NULL && ctx->data_len != 0) {
        http_req.UserDataPtr = ctx->post_data;      /*用户数据*/

        /*设置正文数据长度*/
        HTTPc_ReqSetParam(&http_req,
                          HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_LEN,
                          &(ctx->data_len),
                          &err);

        if (err != HTTPc_ERR_NONE) {
            HTTPc_APP_TRACE_INFO(("HTTPc_ReqSetParam.\n\r"));
            ret = HERROR_SOCK;
            goto EXIT;
        }
    }

    /*设置发送的数据格式*/
    if (ctx->data_format) {
        /*这里暂时设置为一种类型，todo*/
        content_type = HTTP_CONTENT_TYPE_JSON; /*类型*/

        /*设置正文内容类型*/
        HTTPc_ReqSetParam(&http_req,
                          HTTPc_PARAM_TYPE_REQ_BODY_CONTENT_TYPE,
                          &content_type,
                          &err);
        if (err != HTTPc_ERR_NONE) {
            HTTPc_APP_TRACE_INFO(("HTTPc_ReqSetParam.\n\r"));
            ret = HERROR_SOCK;
            goto EXIT;
        }
    }

    /*设置接收回调函数*/
    HTTPc_ReqSetParam(&http_req,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPc_POST_RespBodyHook,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqSetParam.\n\r"));
        ret = HERROR_SOCK;
        goto EXIT;
    }

    /*设置获取正文数据的钩子函数*/
    HTTPc_ReqSetParam(&http_req,
                      HTTPc_PARAM_TYPE_REQ_BODY_HOOK,
                      (void *)&POST_Buf_ReqBodyHook,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqSetParam.\n\r"));
        ret = HERROR_SOCK;
        goto EXIT;
    }

    /*获取资源路径*/
    pA = strstr(ctx->url, ctx->host_name) + strlen(ctx->host_name);
    sA = strstr(pA, "/");
    if ('/' == *sA) {
        resource_path_len = strlen(sA);
        /* printf("sA : %s", sA);	 */
    }

    /***************************开始发送数据请求******************************************/
    result  = HTTPc_ReqSend(http_conn,
                            &http_req,
                            &http_resp,
                            HTTP_METHOD_POST,
                            sA,
                            resource_path_len,
                            HTTPc_FLAG_NONE,
                            &err);
    if (err != HTTPc_ERR_NONE) {
        ret = HERROR_SOCK;
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqSend err.\n\r"));
        ctx->ops->sock_close(http_conn);
        goto EXIT;
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("%s\n\r", http_resp.ReasonPhrasePtr));
    }

EXIT:

    if (http_conn) {
        free(http_conn);
    }

    if (ctx->p_buf) {
        free(ctx->p_buf);
        ctx->p_buf = NULL;
    }
    return ret;
}

CPU_INT32U HTTPc_GET_RespBodyHook(HTTPc_CONN_OBJ     *p_conn,
                                  HTTPc_REQ_OBJ      *p_req,
                                  HTTP_CONTENT_TYPE   content_type,
                                  void               *p_data,
                                  CPU_INT16U          data_len,
                                  CPU_BOOLEAN         last_chunk)
{
    httpcli_ctx *ctx = p_conn->UserData;
    http_body_obj *uchttp_recv = (http_body_obj *)ctx->priv;

    /*对接收的数据进行处理*/
    if (uchttp_recv && uchttp_recv->p) {
        /* if((data_len +1) < uchttp_recv->buf_len * uchttp_recv->buf_count) */
        /* { */
        Mem_Copy(uchttp_recv->p + uchttp_recv->recv_len, p_data, data_len);
        uchttp_recv->recv_len += data_len;
        /* } */
    }

    /*判断是否为最后一个数据块*/
    if (last_chunk == DEF_YES) {
        uchttp_recv->p[uchttp_recv->recv_len] = '\0';
    }

    return data_len;
}

/*get*/
httpin_error httpcli_get(httpcli_ctx *ctx)
{
    int ret;
    HTTPc_CONN_OBJ  *http_conn = NULL;
    HTTPc_REQ_OBJ    http_req;  /*请求对象*/
    HTTPc_RESP_OBJ   http_resp;
    HTTP_CONTENT_TYPE content_type;
    HTTPc_ERR              err;
    CPU_BOOLEAN            result;
    char *pA = NULL;
    char *sA = NULL;
    CPU_SIZE_T resource_path_len = 0;
    int post_data_size = 0;
    http_body_obj *uchttp_recv = NULL;

    if (httpcli_get_protocol(ctx)) {
        ret = HERROR_PARAM;
        goto EXIT;
    }

    ret = HERROR_OK;

    /*用户私有数据设置*/
    if (ctx->priv) {
        uchttp_recv = (http_body_obj *)ctx->priv;
        ctx->p_buf = (char *)malloc(LARGE_CLIENT_HEADER + uchttp_recv->buf_len);
        post_data_size = (LARGE_CLIENT_HEADER + uchttp_recv->buf_len);
    } else {
        ctx->p_buf = (char *)malloc(POST_DATA_SIZE);
        post_data_size = POST_DATA_SIZE;
    }

    memset(ctx->p_buf, 0, post_data_size);

    if (ctx->p_buf == NULL) {
        HTTPc_APP_TRACE_INFO(("malloc fail.\n\r"));
        return HERROR_MEM;
    }

    http_conn = (HTTPc_CONN_OBJ *)ctx->ops->sock_create(ctx, ctx->p_buf, post_data_size);
    if (!http_conn) {
        HTTPc_APP_TRACE_INFO(("httpcli_post sock_create fail.\n\r"));
        goto EXIT;
    }

    Mem_Clr(&http_req,  sizeof(http_req));

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        ret = HERROR_SOCK;
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqClr fail.\n\r"));
        goto EXIT;
    }

    /*设置接收回调函数*/
    HTTPc_ReqSetParam(&http_req,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPc_GET_RespBodyHook,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqSetParam.\n\r"));
        ret = HERROR_SOCK;
        goto EXIT;
    }

    /*获取资源路径*/
    pA = strstr(ctx->url, ctx->host_name) + strlen(ctx->host_name);
    sA = strstr(pA, "/");
    if ('/' == *sA) {
        resource_path_len = strlen(sA);
        /* printf("sA : %s", sA);	 */
    }

    /*该接口会等数据接收完成后才会返回*/
    result  = HTTPc_ReqSend(http_conn,
                            &http_req,
                            &http_resp,
                            HTTP_METHOD_GET,
                            sA,
                            resource_path_len,
                            HTTPc_FLAG_NONE,
                            &err);
    if (err != HTTPc_ERR_NONE) {
        ret = HERROR_SOCK;
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqSend err.\n\r"));
        ctx->ops->sock_close(http_conn);
        goto EXIT;
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("%s\n\r", http_resp.ReasonPhrasePtr));
    }

EXIT:

    if (http_conn) {
        free(http_conn);
    }

    if (ctx->p_buf) {
        free(ctx->p_buf);
        ctx->p_buf = NULL;
    }
    return ret;
}


/******************************************文件下载接口********************************************************/
#include "os/os_api.h"

/*********************/
/*fixme: 该策略不太好！！！*/
cbuffer_t data_cbuf;
OS_SEM w_sem;
OS_SEM r_sem;
/*********************/

#define HTTPCLI_ENTER           0xaaa55555
#define HTTPCLI_EXIT            0x555aaaaa
#define HTTPCLI_REQ_EXIT        0x1ab45cd8

#define DWONLOAD_BUF_SIZE 4*1024  /*4k接收缓存*/
extern int thread_fork(const char *thread_name, int prio, int stk_size, u32 q_size, int *pid, void (*func)(void *), void *parm);
static void uc_httpcli_recv_task(void *priv);

/*一次接收4k数据*/
CPU_INT32U HTTPc_DownLoad_RespBodyHook(HTTPc_CONN_OBJ     *p_conn,
                                       HTTPc_REQ_OBJ      *p_req,
                                       HTTP_CONTENT_TYPE   content_type,
                                       void               *p_data,
                                       CPU_INT16U          data_len,
                                       CPU_BOOLEAN         last_chunk)
{
    int w_len = 0;
    httpcli_ctx *ctx = p_conn->UserData;

    while (!cbuf_is_write_able(&data_cbuf, data_len)) {
        os_time_dly(2);
    }

    /*判断是否为最后一个数据块*/
    if (last_chunk == DEF_YES) {
        ctx->req_exit_flag = 1;
    }

    /*写缓存*/
    w_len = cbuf_write(&data_cbuf, p_data, data_len);
    if (w_len) {
        os_sem_post(&r_sem);
    }

    if (last_chunk != DEF_YES) {
        os_sem_pend(&w_sem, 0);
    }

    return data_len;
}

httpin_error uc_httpcli_init(httpcli_ctx *ctx)
{
    s32_t ret;

    ctx->p_buf = (char *)calloc(1, DWONLOAD_BUF_SIZE);
    if (!ctx->p_buf) {
        HTTPc_APP_TRACE_INFO(("malloc fail.\n\r"));
        return HERROR_MEM ;
    }

    /*初始化循环缓存*/
    cbuf_init(&data_cbuf, ctx->p_buf, DWONLOAD_BUF_SIZE);

    os_sem_create(&r_sem, 0);
    os_sem_create(&w_sem, 0);

    if (NULL == ctx->cb) {
        ctx->cb = uchttpin_default_cb;
    }

    /*开启一个接收线程*/
    if (thread_fork("uc_httpcli_recv_task", 10, 2 * 1024, 0, NULL, uc_httpcli_recv_task, ctx) != OS_NO_ERR) {
        HTTPc_APP_TRACE_INFO(("thread fork fail .\n\r"));
        return -1;
    }

    ret = HERROR_OK;
    return ret;
}

/*为了提高接收数据速度，接收空间需要大于4k*/
int uc_httpcli_read(httpcli_ctx *ctx, char *recvbuf, u32_t len)
{
    int r_len = 0;

    if (len < 4 * 1024) {
        HTTPc_APP_TRACE_INFO(("The memory space must be greater than 4k .\n\r"));
    }

    if (!(ctx->req_exit_flag)) {
        os_sem_pend(&r_sem, 0);
    };

    r_len = cbuf_read(&data_cbuf, recvbuf, data_cbuf.data_len);
    if (r_len > 0) {
        os_sem_post(&w_sem);
    }

    os_time_dly(10);

    return r_len;
}

static void uc_httpcli_recv_task(void *priv)
{
    int ret;
    HTTPc_CONN_OBJ  *http_conn = NULL;
    HTTPc_REQ_OBJ    http_req;  /*请求对象*/
    HTTPc_RESP_OBJ   http_resp;
    HTTP_CONTENT_TYPE content_type;
    HTTPc_ERR              err;
    CPU_BOOLEAN            result;
    char *pA = NULL;
    char *sA = NULL;
    CPU_SIZE_T resource_path_len = 0;
    httpcli_ctx *ctx = (httpcli_ctx *)priv;

    if (httpcli_get_protocol(ctx)) {
        ret = HERROR_PARAM;
        goto EXIT;
    }

    if (ctx->p_buf == NULL) {
        HTTPc_APP_TRACE_INFO(("Not initialized yet .\n\r"));
        goto EXIT;
    }

    ret = HERROR_OK;

    ctx->exit_flag = HTTPCLI_ENTER;
    ctx->req_exit_flag = 0;

    http_conn = (HTTPc_CONN_OBJ *)ctx->ops->sock_create(ctx, ctx->p_buf, DWONLOAD_BUF_SIZE);
    if (!http_conn) {
        HTTPc_APP_TRACE_INFO(("httpcli_post sock_create fail.\n\r"));
        goto EXIT;
    }

    Mem_Clr(&http_req,  sizeof(http_req));

    HTTPc_ReqClr(&http_req, &err);
    if (err != HTTPc_ERR_NONE) {
        ret = HERROR_SOCK;
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqClr fail.\n\r"));
        ctx->ops->sock_close(http_conn);
        goto EXIT;
    }

    /*设置接收回调函数*/
    HTTPc_ReqSetParam(&http_req,
                      HTTPc_PARAM_TYPE_RESP_BODY_HOOK,
                      HTTPc_DownLoad_RespBodyHook,
                      &err);
    if (err != HTTPc_ERR_NONE) {
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqSetParam.\n\r"));
        ret = HERROR_SOCK;
        ctx->ops->sock_close(http_conn);
        goto EXIT;
    }

    /*获取资源路径*/
    pA = strstr(ctx->url, ctx->host_name) + strlen(ctx->host_name);
    sA = strstr(pA, "/");
    if ('/' == *sA) {
        resource_path_len = strlen(sA);
        /* printf("sA : %s", sA);	 */
    }

    /*数据接收完成或出错时才会返回*/
    result  = HTTPc_ReqSend(http_conn,
                            &http_req,
                            &http_resp,
                            HTTP_METHOD_GET,
                            sA,
                            resource_path_len,
                            HTTPc_FLAG_NONE,
                            &err);
    if (err != HTTPc_ERR_NONE) {
        ret = HERROR_SOCK;
        HTTPc_APP_TRACE_INFO(("HTTPc_ReqSend err.\n\r"));
        goto EXIT;
    }

    if (result == DEF_OK) {
        HTTPc_APP_TRACE_INFO(("%s\n\r", http_resp.ReasonPhrasePtr));
    }

EXIT:

    if (ctx->p_buf) {
        free(ctx->p_buf);
        ctx->p_buf = NULL;
    }

    if (http_conn) {
        free(http_conn);
    }
}

/*外部有调用fixme*/
void httpcli_close(httpcli_ctx *ctx)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
}

/*外部有调用fixme*/
void httpcli_cancel(httpcli_ctx *ctx)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
}

/*todo*/
void http_cancel_dns(httpcli_ctx *ctx)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
}

/*todo*/
int transfer_chunked_analysis(char *receive_buf, u32 recv_length)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    return 1;
}

/*todo*/
int transfer_chunked_parse(httpcli_ctx *ctx,
                           char *receive_buf,
                           int recv_length,
                           u32(*cb)(u8 *, int, void *),
                           void *priv)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    return 1;
}

/*todo*/
httpin_error httpcli_post_header(httpcli_ctx *ctx)
{
    printf(">>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    s32_t ret;
    ret = HERROR_OK;
    return ret;
}

const struct net_download_ops http_ops = {
    .init  = uc_httpcli_init,
    .read  = uc_httpcli_read,
    .close = httpcli_close, /*fixme:外部有调用*/
    .quit  = httpcli_cancel, /*fixme:外部有调用*/
};

#endif

