#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "http/http_cli.h"
#include "app_config.h"

#ifdef USE_HTTP_POST_GET_TEST
//选择其中一种进行测试
#define HTTP_GET_TEST
//#define HTTP_POST_TEST
//#define HTTP_PUT_TEST

#define HTTP_DEMO_URL  "http://www.baidu.com"
#define HTTP_DEMO_DATA "{\"name\":\"xiao ming\", \"age\":12}"

httpin_error httpcli_put(httpcli_ctx *ctx)
{
    return httpcli_post(ctx);
}

static void http_test_task(void *priv)
{
    int ret = 0;
    http_body_obj http_body_buf;
    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (!ctx) {
        printf("calloc faile\n");
        return;
    }

    memset(&http_body_buf, 0x0, sizeof(http_body_obj));

    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = 4 * 1024;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *) malloc(http_body_buf.buf_len * http_body_buf.buf_count);
    if (!http_body_buf.p) {
        free(ctx);
        return;
    }

    ctx->url = HTTP_DEMO_URL;
    ctx->timeout_millsec = 5000;
    ctx->priv = &http_body_buf;
    ctx->connection = "close";

#if (defined HTTP_POST_TEST)
    printf("http post test start\n\n");
    ctx->data_format = "application/json";
    ctx->post_data = HTTP_DEMO_DATA;
    ctx->data_len = strlen(HTTP_DEMO_DATA);
    ret = httpcli_post(ctx);
#elif (defined HTTP_GET_TEST)
    printf("http get test start\n\n");
    ret = httpcli_get(ctx);
#elif (defined HTTP_PUT_TEST)
    printf("http put test start\n\n");
    ret = httpcli_put(ctx);
#endif

    if (ret != HERROR_OK) {
        printf("http client test faile\n");
    } else {
        if (http_body_buf.recv_len > 0) {
            printf("\nReceive %d Bytes from (%s)\n", http_body_buf.recv_len, HTTP_DEMO_URL);
            printf("%s\n", http_body_buf.p);
        }
    }

    //关闭连接
    httpcli_close(ctx);

    if (http_body_buf.p) {
        free(http_body_buf.p);
        http_body_buf.p = NULL;
    }

    if (ctx) {
        free(ctx);
        ctx = NULL;
    }
}

static void http_test_start(void *priv)
{
    enum wifi_sta_connect_state state;
    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }
        os_time_dly(1000);
    }

    if (thread_fork("http_test_task", 10, 1024, 0, NULL, http_test_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

//应用程序入口,需要运行在STA模式下
void c_main(void *priv)
{
    if (thread_fork("http_test_start", 10, 512, 0, NULL, http_test_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(c_main);
#endif //USE_HTTP_POST_GET_TEST
