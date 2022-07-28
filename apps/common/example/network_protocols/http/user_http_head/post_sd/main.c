#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "http/http_cli.h"
#include "fs/fs.h"
#include "app_config.h"

#ifdef USE_HTTP_POST_SD_DATA_TEST
static char http_body_begin[512];
extern int storage_device_ready(void);

#define BOUNDARY    "----WebKitFormBoundary7MA4YWxkTrZu0gW"

#define HTTP_HEAD_OPTION        \
"POST /third/file/shardUpload HTTP/1.1\r\n"\
"User-Agent: PostmanRuntime/7.11.0\r\n"\
"Accept: */*\r\n"\
"Cache-Control: no-cache\r\n"\
"Postman-Token: f8ddd5ec-1ba1-457b-ab3c-5e6f3686f9dc\r\n"\
"Host: leadread-api.aitec365.com\r\n"\
"accept-encoding: gzip, deflate\r\n"\
"content-type: multipart/form-data; boundary="BOUNDARY"\r\n"\
"content-length: %d\r\n"\
"Connection: close\r\n\r\n"

#define BODY_OPTION_ONE  \
"--"BOUNDARY"\r\n" \
"Content-Disposition: form-data; name=\"filename\"\r\n\r\n"\
"test.mp3\r\n"  \
"--"BOUNDARY"\r\n"   \
"Content-Disposition: form-data; name=\"file\"; filename=\"test.mp3\"\r\n"\
"Content-Type: audio/mpeg\r\n\r\n"

#define BODY_OPTION_END       \
"\r\n--"BOUNDARY"--\r\n"

#define USER_HEADER_BUF_SIZE 2 * 1024
#define HTTP_DEMO_URL "https://leadread-api.aitec365.com/third/file/shardUpload"
#define READ_BUF_SIZE 4094

int http_cb(void *httpcli_ctx, void *buf, unsigned int size, void *priv, httpin_status status)
{
    return 0;
}

static void http_test_task(void *priv)
{
    int ret = 0;
    char *user_header_buf = NULL;
    httpcli_ctx ctx = {0};
    FILE *fd = NULL;
    int f_size = 0, r_len = 0;

    user_header_buf = calloc(1, USER_HEADER_BUF_SIZE);
    if (!user_header_buf) {
        goto exit;
    }

    fd = fopen(CONFIG_STORAGE_PATH"/C/test.mp3", "r");
    if (fd == NULL) {
        puts("fopen err!");
        goto exit;
    }

    //获取文件大小
    f_size = flen(fd);

    sprintf(http_body_begin, BODY_OPTION_ONE);
    sprintf(user_header_buf, HTTP_HEAD_OPTION"%s",  strlen(http_body_begin) + f_size + strlen(BODY_OPTION_END), http_body_begin);

    ctx.url = HTTP_DEMO_URL;
    ctx.cb = http_cb;
    ctx.timeout_millsec = 5000;
    ctx.connection = "close";

    //自定义头部
    ctx.user_http_header = user_header_buf;

    //发送头部信息
    if (HERROR_OK != httpcli_post_header(&ctx)) {
        goto exit;
    }

    char *read_buf = malloc(READ_BUF_SIZE);
    if (!read_buf) {
        goto exit;
    }

    memset(read_buf, 0, READ_BUF_SIZE);

    while (f_size) {
        r_len = fread(read_buf, READ_BUF_SIZE, 1, fd);
        if (r_len > 0) {
            //发送数据
            if (ctx.ops->sock_send(ctx.sock_hdl, read_buf, r_len, 0) <= 0) {
                ctx.ops->sock_close(ctx.sock_hdl);
                goto exit;
            }

            f_size -= r_len;
        } else {
            goto exit;
        }
        //put_buf(read_buf, r_len);
    }

    //发送结束符
    if (ctx.ops->sock_send(ctx.sock_hdl, BODY_OPTION_END, strlen(BODY_OPTION_END), 0) <= 0) {
        ctx.ops->sock_close(ctx.sock_hdl);
        goto exit;
    }

    memset(read_buf, 0, READ_BUF_SIZE);

    if (ctx.ops->sock_recv(ctx.sock_hdl, read_buf, READ_BUF_SIZE, 0) <= 0) {
        ctx.ops->sock_close(ctx.sock_hdl);
        goto exit;
    }

    //printf("received data: \n%s\n", read_buf);

    char *ptr = strstr(read_buf, "\r\n\r\n");
    printf("received data: \n%s\n", ptr + 4);

    ctx.ops->sock_close(ctx.sock_hdl);

exit:
    if (user_header_buf) {
        free(user_header_buf);
        user_header_buf = NULL;
    }

    if (read_buf) {
        free(read_buf);
        read_buf = NULL;
    }
}

static void http_user_header_test(void *priv)
{
    //等待获取ip成功
    enum wifi_sta_connect_state state;
    while (1) {
        printf("Connecting to the network...\n");
        state = wifi_get_sta_connect_state();
        if (WIFI_STA_NETWORK_STACK_DHCP_SUCC == state) {
            printf("Network connection is successful!\n");
            break;
        }
        os_time_dly(100);
    }

    //等待存储设备就绪
    while (1) {
        if (!storage_device_ready()) {
            printf("Wait for the storage device to be ready !");
            os_time_dly(10);
            continue;
        }
        printf("the storage device is ready!");
        break;
    }

    if (thread_fork("http_test_task", 10, 4 * 1024, 0, NULL, http_test_task, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

//应用程序入口,需要运行在STA模式下
void c_main(void *priv)
{
    if (thread_fork("http_user_header_test", 10, 512, 0, NULL, http_user_header_test, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(c_main);

#endif//USE_HTTP_POST_SD_DATA_TEST
