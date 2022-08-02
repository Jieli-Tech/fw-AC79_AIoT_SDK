#include "sock_api/sock_api.h"
#include <string.h>
#include <stdlib.h>
#include "turing.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "os/os_api.h"
#include "mbedtls/aes.h"
#include "server/ai_server.h"
#include "turing_alarm.h"

#ifdef TURING_MEMORY_DEBUG
#include "mem_leak_test.h"
#endif

enum {
    RESP_WAIT_NORMAL,
    RESP_NO_WAIT,
    RESP_WAIT_END,
};

struct resp_header {
    int status_code;//HTTP/1.1 '200' OK
    char content_type[128];//Content-Type: application/gzip
    long content_length;//Content-Length: 11683079
};

struct turing_var {
    struct turing_para para;
    u8 exit_flag;
    u8 connect_status;
    u8 pic_req;
    u8 rec_stopping;
    void *sock_hdl;
    unsigned int timeout_ms;
    u32 album_id;
    int (*func_cb)(int, struct json_object *, const char *asr);
    char url[1024];
};

static struct turing_var turing_hdl;
static volatile u8 msg_notify_disable;
static u8 turing_app_init_flag = 0;
static int turing_app_task_pid;

#define __this 	    (&turing_hdl)

#define TURING_SERVER_HOST	"smartdevice.ai.tuling123.com"
#define PIC_SERVER_HOST	    "iot.turingos.cn"

static const char upload_head[] =
    "POST /speech/chat HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Length: %d\r\n"
    "Connection: keep-alive\r\n"
    "Cache-Control: no-cache\r\n"
    "Content-Type: multipart/form-data; boundary=%s\r\n"
    "Accept: */*\r\n"
    "Accept-Language: zh-CN\r\n"
    "\r\n";

static const char pic_upload_head[] =
    "POST /mmui/picbook HTTP/1.1\r\n"
    "Host: %s\r\n"
    "Content-Length: %d\r\n"
    "Connection: keep-alive\r\n"
    "Cache-Control: no-cache\r\n"
    "Content-Type: multipart/form-data; boundary=%s\r\n"
    "Accept: */*\r\n"
    "Accept-Language: zh-CN\r\n"
    "\r\n";

#define UPLOAD_PARAMETERS "Content-Disposition: form-data; name=\"parameters\"\r\n\r\n%s"

#define UPLOAD_SPEECH "Content-Disposition: form-data; name=\"speech\"; filename=\"speech.%s\"\r\nContent-Type: application/octet-stream\r\n\r\n"


extern char *strdup(const char *s);
extern void Get_IPAddress(u8 is_wireless, char *ipaddr);
extern int get_turing_typeflag(void);
extern unsigned int random32(int type);

static void AES128_CBC_encrypt_buffer(u8 *output, u8 *input, u32 length, const u8 *key, u8 *iv)
{
    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);
    mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, length, iv, input, output);
    mbedtls_aes_free(&aes_ctx);
}

/*
 * 获取随机字符串
 */
static void get_rand_str(char strRand[], int iLen)
{
    int i = 0;
    char metachar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";

    memset(strRand, 0, iLen);
    for (i = 0; i < (iLen - 1); i++) {
        *(strRand + i) = metachar[random32(0) % 62];
    }
}

static int sock_cb_func(enum sock_api_msg_type type, void *priv)
{
    if (__this->exit_flag) {
        return -1;
    }
    return 0;
}

static void *get_socket_fd(const char *host, u32 timeout_ms)
{
    void *sock_hdl;
    struct hostent *server;
    struct sockaddr_in serv_addr;

    sock_hdl = sock_reg(AF_INET, SOCK_STREAM, 0, sock_cb_func, NULL);
    if (!sock_hdl) {
        return NULL;
    }

    sock_set_send_timeout(sock_hdl, timeout_ms);
    sock_set_recv_timeout(sock_hdl, timeout_ms);

    char ipaddr[20];
    Get_IPAddress(1, ipaddr);
    struct sockaddr_in local_ipaddr;
    local_ipaddr.sin_addr.s_addr = inet_addr(ipaddr);
    local_ipaddr.sin_port = htons(0);
    local_ipaddr.sin_family = AF_INET;
    if (sock_bind(sock_hdl, (struct sockaddr *)&local_ipaddr, sizeof(local_ipaddr))) {
        sock_unreg(sock_hdl);
        return NULL;
    }

    /* lookup the ip address */
    server = gethostbyname(host);
    if (server == NULL) {
        sock_unreg(sock_hdl);
        return NULL;
    }

    struct ip4_addr *ip4_addr = NULL;
    ip4_addr = (struct ip4_addr *)server->h_addr_list[0];
    TR_DEBUG_I("DNS lookup succeeded. IP=%s\n", inet_ntoa(*ip4_addr));

    /* fill in the structure */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(80);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (sock_connect(sock_hdl, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        TR_DEBUG_E("Turing connecting error\n");
        sock_unreg(sock_hdl);
        return NULL;
    }

    return sock_hdl;
}

static struct json_object *create_cjson(struct turing_para *para)
{
    struct json_object *root = json_object_new_object();
    struct json_object *array = NULL;
    struct json_object *extra = NULL;

    json_object_object_add(root, "ak", json_object_new_string(para->api_key));
    json_object_object_add(root, "tts", json_object_new_int(para->tts));
    json_object_object_add(root, "flag", json_object_new_int(para->flag));
    json_object_object_add(root, "type", json_object_new_int(para->type));
    json_object_object_add(root, "tone", json_object_new_int(para->tone));
    json_object_object_add(root, "speed", json_object_new_int(para->speed));
    json_object_object_add(root, "pitch", json_object_new_int(para->pitch));
    json_object_object_add(root, "volume", json_object_new_int(para->volume));

    if (para->type == SMARTCHAT) {
        json_object_object_add(root, "asr", json_object_new_int(para->asr));
        json_object_object_add(root, "asr_lan", json_object_new_int(para->asr_lan));
        if (para->real_time) {
            json_object_object_add(root, "identify", json_object_new_string(para->identify));
            json_object_object_add(root, "index", json_object_new_int(para->index));
            json_object_object_add(root, "realTime", json_object_new_int(para->real_time));
        }
        if (para->voice_mode == TRANSLATE_MODE) {
            json_object_object_add(root, "tts_lan", json_object_new_int(1));
            array = json_object_new_array();
            json_object_array_add(array, json_object_new_int(20028));
            json_object_object_add(root, "seceneCodes", array);
        } else {
            json_object_object_add(root, "tts_lan", json_object_new_int(para->tts_lan));
        }
    } else if (para->type == INPUT_TEXT) {
        if (para->cmd_code == PLAY_NEXT) {
            static const char input_play_next[] = {
                0xE6, 0x92, 0xAD, 0xE6, 0x94, 0xBE, 0xE4, 0xB8, 0x8B, 0xE4, 0xB8, 0x80, 0xE9, 0xA6, 0x96, 0x00
            };
            json_object_object_add(root, "textStr", json_object_new_string(input_play_next));
        } else if (para->cmd_code == PLAY_PREV) {
            static const char input_play_prev[] = {
                0xE6, 0x92, 0xAD, 0xE6, 0x94, 0xBE, 0xE4, 0xB8, 0x8A, 0xE4, 0xB8, 0x80, 0xE9, 0xA6, 0x96, 0x00
            };
            json_object_object_add(root, "textStr", json_object_new_string(input_play_prev));
        } else if (para->cmd_code == CONNECT_SUCC) {
            static const char input_connect_succ[] = {
                0xE6, 0x92, 0xAD, 0xE6, 0x94, 0xBE, 0xE5, 0x84, 0xBF, 0xE6, 0xAD, 0x8C, 0x00
            };
            json_object_object_add(root, "textStr", json_object_new_string(input_connect_succ));
        }
    } else if (para->type == PICTURE_RECOG) {
        extra = json_object_new_object();
        if (extra) {
            char imgid[33];
            get_rand_str(imgid, sizeof(imgid));
            json_object_object_add(extra, "imgFlagId", json_object_new_string(imgid));
            json_object_object_add(extra, "cameraId", json_object_new_int(para->camera_id));
            if (para->book_id) {
                json_object_object_add(extra, "bookId", json_object_new_int(para->book_id));
            }
            json_object_object_add(extra, "innerUrlFlag", json_object_new_int(1));
#if 1
            json_object_object_add(extra, "debug", json_object_new_int(1));
            json_object_object_add(extra, "accessModel", json_object_new_int(1));
            json_object_object_add(extra, "modelOrder", json_object_new_int(2));
            json_object_object_add(extra, "languageOrder", json_object_new_int(1));
            json_object_object_add(extra, "textFlag", json_object_new_int(1));
#endif
            json_object_object_add(extra, "typeFlag", json_object_new_int(get_turing_typeflag()));
            json_object_object_add(root,  "extra", extra);
        }
    } else if (para->type == ORAL_EVALUATION) {
        json_object_object_add(root, "asr", json_object_new_int(para->asr));
        if (para->real_time) {
            json_object_object_add(root, "identify", json_object_new_string(para->identify));
            json_object_object_add(root, "index", json_object_new_int(para->index));
            json_object_object_add(root, "realTime", json_object_new_int(para->real_time));
        }
        extra = json_object_new_object();
        if (extra) {
            json_object_object_add(extra, "typeOperate", json_object_new_int(para->oral_flag ? 1101 : 1100));
            json_object_object_add(extra, "hopeText", json_object_new_string(para->oral_string));
            json_object_object_add(root,  "extra", extra);
        }
    }

    uint8_t in[16];
    uint8_t out[16];
    uint8_t aes_key_1[16];
    uint8_t iv[16];
    memcpy(in, para->user_id, sizeof(in));
    memcpy(aes_key_1, para->aes_key, sizeof(aes_key_1));
    memcpy(iv, para->api_key, sizeof(iv));
    AES128_CBC_encrypt_buffer(out, in, 16, aes_key_1, iv);

    uint8_t outStr[64] = {0};
    int i, aseLen = 0;
    for (i = 0; i < 16; i++) {
        aseLen += snprintf((char *)(outStr + aseLen), 64, "%.2x", out[i]);
    }

    char null = 0;
    json_object_object_add(root, "uid", json_object_new_string((const char *)outStr));

    //第一次token值填""
    if (para->token[0] == 0) {
        json_object_object_add(root, "token", json_object_new_string(&null));
    } else {
        json_object_object_add(root, "token", json_object_new_string(para->token));
    }

    return root;
}

static int buildRequest(void *sock_hdl, const unsigned char *file_data, int data_len, struct turing_para *para)
{
    int err	= TURING_ERR_NON;
    struct json_object *str_js = NULL;
    char *parameter_data = NULL;
    char *boundary = NULL;
    char *header_data = NULL;

    char *boundary_header = "----AiWiFiBoundary";
    char *end = "\r\n"; 			//结尾换行
    char *twoHyphens = "--";		//两个连字符

    char s[16 + 1] = {0};
    get_rand_str(s, sizeof(s));

    boundary = (char *)calloc(1, strlen(boundary_header) + strlen(s) + 1);
    if (!boundary) {
        TR_DEBUG_E("boundary malloc err\n");
        err = TURING_ERR_MALLOC;
        goto _EXIT;
    }
    strcat(boundary, boundary_header);
    strcat(boundary, s);

    char firstBoundary[128] = {0};
    char secondBoundary[128] = {0};
    char endBoundary[128] = {0};

    sprintf(firstBoundary, "%s%s%s", twoHyphens, boundary, end);
    sprintf(secondBoundary, "%s%s%s%s", end, twoHyphens, boundary, end);
    sprintf(endBoundary, "%s%s%s%s%s", end, twoHyphens, boundary, twoHyphens, end);

    str_js = create_cjson(para);

    parameter_data = (char *)malloc(strlen(firstBoundary) + strlen(UPLOAD_PARAMETERS) + strlen(json_object_to_json_string(str_js)) + strlen(secondBoundary) + strlen(UPLOAD_SPEECH) + strlen(endBoundary) + 64);
    if (NULL == parameter_data) {
        TR_DEBUG_E("parameter_data malloc err");
        err = TURING_ERR_MALLOC;
        goto _EXIT;
    }

    int content_length = 0;
    if (para->type == INPUT_TEXT) {
        content_length = sprintf(parameter_data, "%s"UPLOAD_PARAMETERS"%s", firstBoundary, json_object_to_json_string(str_js), endBoundary);
    } else {
        content_length = sprintf(parameter_data, "%s"UPLOAD_PARAMETERS"%s", firstBoundary, json_object_to_json_string(str_js), secondBoundary);
    }

    if (para->type == INPUT_TEXT) {
        content_length = strlen(parameter_data);
    } else {
        if (para->type == PICTURE_RECOG) {
            sprintf(parameter_data + content_length, UPLOAD_SPEECH, "jpg");
        } else if (para->asr == ASR_SPEEX) {
            sprintf(parameter_data + content_length, UPLOAD_SPEECH, "speex");
        } else if (para->asr == ASR_OPUS) {
            sprintf(parameter_data + content_length, UPLOAD_SPEECH, "opus");
        }
        content_length = data_len + strlen(parameter_data) + strlen(endBoundary);
    }

    header_data = (char *)malloc(1024);
    if (!header_data) {
        err = TURING_ERR_MALLOC;
        goto _EXIT;
    }

    int send_len = 0;

    if (para->type == PICTURE_RECOG) {
        send_len = snprintf(header_data, 1024, pic_upload_head, PIC_SERVER_HOST, content_length, boundary);
    } else {
        send_len = snprintf(header_data, 1024, upload_head, TURING_SERVER_HOST, content_length, boundary);
    }

#if 0
    printf("\r\nrequst date\r\n");
    printf("%s\n", header_data);
    printf("%s\n", firstBoundary);
    printf("%s\n", parameter_data);
    printf("%s\n", filename);
    printf("%s\n", endBoundary);
#endif

    //send header_data
    if (sock_send(sock_hdl, header_data, send_len, 0) != send_len) {
        TR_DEBUG_E("sent header_data err");
        err = TURING_ERR_SEND_DATA;
        goto _EXIT;
    }

    //send parameter_data
    send_len = strlen(parameter_data);
    if (sock_send(sock_hdl, parameter_data, send_len, 0) != send_len) {
        TR_DEBUG_E("sent parameter_data err");
        err = TURING_ERR_SEND_DATA;
        goto _EXIT;
    }

    //send file_data
    if (file_data && data_len) {
        if (sock_send(sock_hdl, file_data, data_len, 0) != data_len) {
            TR_DEBUG_E("sent file_data err");
            err = TURING_ERR_SEND_DATA;
            goto _EXIT;
        }
    }

    if (para->type != INPUT_TEXT) {
        //send endBoundary
        send_len = strlen(endBoundary);
        if (sock_send(sock_hdl, endBoundary, send_len, 0) != send_len) {
            TR_DEBUG_E("sent endBoundary err");
            err = TURING_ERR_SEND_DATA;
            goto _EXIT;
        }
    }

    err = 0;

_EXIT:
    if (boundary) {
        free(boundary);
    }
    if (parameter_data) {
        free(parameter_data);
    }
    if (str_js) {
        json_object_put(str_js);
    }
    if (header_data) {
        free(header_data);
    }

    return err;
}

static void get_resp_header(const char *response, struct resp_header *resp)
{
    /*获取响应头的信息*/
    char *pos = strstr(response, "HTTP/1.1 ");
    if (pos) {
        pos += strlen("HTTP/1.1 ");
        resp->status_code = atoi(pos);
    }
    pos = strstr(response, "Content-Type: ");
    if (pos) {
        char *pos1 = strstr(pos, "\r\n");
        pos += strlen("Content-Type: ");
        if (pos1 - pos < sizeof(resp->content_type)) {
            memcpy(resp->content_type, pos, pos1 - pos);
            resp->content_type[pos1 - pos] = 0;
        }
    }
    pos = strstr(response, "Content-Length: ");
    if (pos) {
        pos += strlen("Content-Length: ");
        resp->content_length = atoi(pos);
    }
}

static int getResponseWaitNormal(void *sock_hdl, char **text)
{
    int err	= TURING_ERR_NON;

    /* receive the response */
    char *response = NULL;
    char *code = NULL;
    char *find = NULL;

    response = (char *)calloc(1, 1024);
    if (!response) {
        return TURING_ERR_MALLOC;
    }

    int length = 0;
    int temp_length = 0;

    struct resp_header resp;
    int ret = 0;

    while (1) {
        ret = sock_recv(sock_hdl, response + length, 1024 - length, 0);
        if (ret <= 0) {
            TR_DEBUG_E("recv timeout !!! \n");
            err = TURING_ERR_RECV_DATA;
            goto __getResponse_exit;
        }
        length += ret;
        find = strstr(response, "\r\n\r\n");
        if (find) {
            find += 4;
            temp_length = length - (find - response);
            break;
        }
        //找到响应头的头部信息, 两个"\r\n"为分割点
        if (length >= 1024 - 1) {
            err =  TURING_ERR_MALLOC;
            goto __getResponse_exit;
        }
    }

    memset(&resp, 0, sizeof(struct resp_header));
    get_resp_header(response, &resp);
    if (resp.status_code != 200 || resp.content_length == 0) {
        TR_DEBUG_E("\r\nresp.content_length = %ld status_code = %d\r\n", resp.content_length, resp.status_code);
        err = TURING_ERR_HTTP_CODE;
        goto __getResponse_exit;
    }

    code = (char *)calloc(1, resp.content_length + 1);
    if (code == NULL) {
        err = TURING_ERR_MALLOC;
        goto __getResponse_exit;
    }

    length = temp_length;
    memcpy(code, find, length);

    while (resp.content_length - length > 0) {
        ret = sock_recv(sock_hdl, code + length, resp.content_length - length, 0);
        if (ret <= 0) {
            err = TURING_ERR_RECV_DATA;
            goto __getResponse_exit;
        }
        length += ret;
        if (length == resp.content_length) {
            break;
        }
    }
    TR_DEBUG_I("\r\nresponse is %s\r\n", code);

__getResponse_exit:
    if (response) {
        free(response);
    }
    if (err == 0) {
        *text = code;
    } else {
        if (code) {
            free(code);
            *text = NULL;
        }
    }

    return err;
}

static int getResponseNoWait(void *sock_hdl, struct turing_para *p)
{
    int err	= TURING_ERR_NON;

    /* receive the response */
    char *response = NULL;
    char *code = NULL;
    char *find = NULL;

    response = (char *)calloc(1, 1024);
    if (!response) {
        return TURING_ERR_MALLOC;
    }

    int length = 0;
    int temp_length = 0;

    struct resp_header resp;
    int ret = 0;
    int recv_flag = MSG_DONTWAIT;

    while (1) {
        ret = sock_recv(sock_hdl, response + length, 1024 - length, recv_flag);
        if (ret <= 0) {
            puts("now no rsp, check next rsp !!! \n");
            err = 0;
            goto __getResponse_exit;
        }
        length += ret;
        find = strstr(response, "\r\n\r\n");
        if (find) {
            find += 4;
            temp_length = length - (find - response);
            break;
        }
        //找到响应头的头部信息, 两个"\r\n"为分割点
        if (length >= 1024 - 1) {
            err = TURING_ERR_MALLOC;
            goto __getResponse_exit;
        }
        recv_flag = 0;
    }

    memset(&resp, 0, sizeof(struct resp_header));
    get_resp_header(response, &resp);
    if (resp.status_code != 200 || resp.content_length == 0) {
        TR_DEBUG_E("\r\nresp.content_length = %ld status_code = %d\r\n", resp.content_length, resp.status_code);
        err = TURING_ERR_HTTP_CODE;
        goto __getResponse_exit;
    }

    code = (char *)calloc(1, resp.content_length + 1);
    if (code == NULL) {
        err = TURING_ERR_MALLOC;
        goto __getResponse_exit;
    }

    length = temp_length;
    if (length > resp.content_length) {
        length = resp.content_length;
    }
    memcpy(code, find, length);

    while (resp.content_length - length > 0) {
        ret = sock_recv(sock_hdl, code + length, resp.content_length - length, 0);
        if (ret <= 0) {
            err = TURING_ERR_RECV_DATA;
            goto __getResponse_exit;
        }
        length += ret;
        if (length == resp.content_length) {
            break;
        }
    }

    if (!strstr(code, "40000")) {
        err = TURING_ERR_HTTP_CODE;
    } else {
        ++p->recv_rsp_cnt;
    }

__getResponse_exit:
    if (response) {
        free(response);
    }
    if (code) {
        free(code);
    }

    return err;
}

static int getResponseWaitEnd(void *sock_hdl, char **text, struct turing_para *p)
{
    int err	= TURING_ERR_NON;

    /* receive the response */
    char *response = NULL;
    char *code = NULL;
    char *find = NULL;
    int length = 0;
    int temp_length = 0;
    int ret = 0;
    int ext = 0;
    struct resp_header resp;

    response = (char *)malloc(1024);
    if (!response) {
        return TURING_ERR_MALLOC;
    }

recv_again:
    if (0 == ext) {
        length = 0;
        temp_length = 0;
        memset(response, 0, 1024);
    }

    while (1) {
        find = strstr(response, "\r\n\r\n");
        if (find) {
            find += 4;
            temp_length = length - (find - response);
            break;
        }
        //找到响应头的头部信息, 两个"\r\n"为分割点
        if (length >= 1024 - 1) {
            err = TURING_ERR_MALLOC;
            goto __getResponse_exit;
        }
        ret = sock_recv(sock_hdl, response + length, 1024 - length, 0);
        if (ret <= 0) {
            TR_DEBUG_E("recv timeout !!! \n");
            err = TURING_ERR_RECV_DATA;
            goto __getResponse_exit;
        }
        length += ret;
    }

    memset(&resp, 0, sizeof(struct resp_header));
    get_resp_header(response, &resp);
    if (resp.status_code != 200 || resp.content_length == 0) {
        TR_DEBUG_E("\r\nresp.content_length = %ld status_code = %d\r\n", resp.content_length, resp.status_code);
        err = TURING_ERR_HTTP_CODE;
        goto __getResponse_exit;
    }

    code = (char *)calloc(1, resp.content_length + 1);
    if (code == NULL) {
        err = TURING_ERR_MALLOC;
        goto __getResponse_exit;
    }

    ext = 0;
    length = temp_length;
    if (length > resp.content_length) {
        length = resp.content_length;
        ext = 1;
    }
    memcpy(code, find, length);

    while (resp.content_length - length > 0) {
        ret = sock_recv(sock_hdl, code + length, resp.content_length - length, 0);
        if (ret <= 0) {
            err = TURING_ERR_RECV_DATA;
            goto __getResponse_exit;
        }
        length += ret;
        if (length == resp.content_length) {
            break;
        }
    }
    TR_DEBUG_I("\r\nresponse is %s\r\n", code);

    if ((u8)(-p->index) > ++p->recv_rsp_cnt && (ext || strstr(code, "\"code\":40000,"))) {
        puts("check next rsp !!! \n");
        free(code);
        code = NULL;
        if (ext) {
            memcpy(response, find + resp.content_length, temp_length - resp.content_length);
            response[temp_length - resp.content_length] = 0;
            length = temp_length - resp.content_length;
            temp_length = 0;
        }
        goto recv_again;
    }

__getResponse_exit:
    if (response) {
        free(response);
    }
    if (err == 0) {
        *text = code;
    } else {
        if (code) {
            free(code);
            *text = NULL;
        }
    }

    return err;
}

static int parseJson_string(const char *pMsg, struct turing_var *p)
{
    int err	= TURING_ERR_NON;
    int code = 0;

    if (NULL == pMsg) {
        return TURING_ERR_POINT_NULL;
    }

    struct json_object *pJson = json_tokener_parse(pMsg);
    if (NULL == pJson) {
        return TURING_ERR_POINT_NULL;
    }

    struct json_object *pSub = NULL;//获取token的值，用于下一次请求时上传的校验值
    if (json_object_object_get_ex(pJson, "token", &pSub)) {
        const char *token = json_object_get_string(pSub);
        if (token && token[0]) {
            strcpy(p->para.token, token);
        }
    }

    if (!json_object_object_get_ex(pJson, "code", &pSub)) {
        err = TURING_ERR_POINT_NULL;
        goto exit;
    }

    code = json_object_get_int(pSub);
    TR_DEBUG_I("code : %d\r\n", code);

    switch (code) {
    case 40000:
    case 40001:
    case 40002:
    case 40003:
    case 40005:
    case 40006:
    case 40007:
    case 40008:
    case 40010:
    case 40011:
    case 40012:
    case 40013:
    case 43000:
    case 43010:
    case 43020:
    case 43030:
    case 43040:
    case 49999:
        if (p->func_cb) {
            err = p->func_cb(code, NULL, NULL);
        }
        goto exit;
    default:
        break;
    }

    struct json_object *asr = NULL;

    //语义识别文本结果
    if (json_object_object_get_ex(pJson, "asr", &asr)) {
        TR_DEBUG_I("asr: %s \n", json_object_get_string(asr));
    }

    //语义解析文本结果
    if (json_object_object_get_ex(pJson, "tts", &pSub)) {
        TR_DEBUG_I("tts: %s \n", json_object_get_string(pSub));
    }

    //func analysis
    if (json_object_object_get_ex(pJson, "func", &pSub)) {
    }

    if (p->func_cb) {
        err = p->func_cb(code, pSub, json_object_get_string(asr));
        if (err) {
            if (1 == err) {
                err = 0;	//播放tip
                goto exit;
            }
        }
    }

    //nlp url
    if (json_object_object_get_ex(pJson, "nlp", &pSub)) {
        struct json_object *subitem = json_object_array_get_idx(pSub, 0);
        if (subitem) {
            TR_DEBUG_I("url is %s\n", json_object_get_string(subitem));
            JL_turing_media_speak_play(json_object_get_string(subitem));
        }
    }

exit:
    if (pJson) {
        json_object_put(pJson);
    }

    return err;
}

/*
 * 发送请求，读取响应
 */
static int requestAndResponse(void *sock_hdl, const unsigned char *file_data, int len, struct turing_var *p, u8 wait_opt)
{
    char *text = NULL;
    int err;

    err = buildRequest(sock_hdl, file_data, len, &p->para);
    if (err) {
        return err;
    }

    if (wait_opt == RESP_WAIT_NORMAL) {
        err = getResponseWaitNormal(sock_hdl, &text);
    } else if (wait_opt == RESP_WAIT_END) {
        err = getResponseWaitEnd(sock_hdl, &text, &p->para);
    } else {
        err = getResponseNoWait(sock_hdl, &p->para);
    }

    if (err) {
        return err;
    }
    if (text) {
        err = parseJson_string(text, p);
        free(text);
    }

    return err;
}

static void update_identify(struct turing_para *para)
{
    char identify[20] = {0};
    para->real_time = STREAM_IDENTIFY;
    para->index = 0;
    para->recv_rsp_cnt = 0;
    get_rand_str(identify, sizeof(identify));
    strcpy(para->identify, identify);
}

static int turing_play_control(struct turing_var *p, CMD_DODE cmd)
{
    int err = 0;

    msg_notify_disable = 1;

    void *sock_hdl = get_socket_fd(TURING_SERVER_HOST, p->timeout_ms);
    if (!sock_hdl) {
        TR_DEBUG_E("turing sock_hdl create fail !!!\n");
        msg_notify_disable = 0;
        return -1;
    }

    p->url[0] = 0;
    p->para.type = INPUT_TEXT;
    p->para.cmd_code = cmd;

    err = requestAndResponse(sock_hdl, NULL, 0, p, RESP_WAIT_NORMAL);
    if (err) {
        TR_DEBUG_E("turing play control request fail !!!\n");
    }

    sock_unreg(sock_hdl);
    p->para.type = SMARTCHAT;

    msg_notify_disable = 0;

    return err;
}

static int turing_picture_recognition(struct turing_var *p, u8 *data, int data_length)
{
    int err = 0;

    p->exit_flag &= ~BIT(2);
    p->pic_req = 1;

    void *sock_hdl = get_socket_fd(PIC_SERVER_HOST, p->timeout_ms);
    if (!sock_hdl) {
        TR_DEBUG_E("turing sock_hdl create fail !!!\n");
        free(data);
        p->pic_req = 0;
        return -1;
    }

    p->url[0] = 0;
    p->para.type = PICTURE_RECOG;

    err = requestAndResponse(sock_hdl, data, data_length, p, RESP_WAIT_NORMAL);
    if (err) {
        TR_DEBUG_E("turing play control request fail !!!\n");
    }

    sock_unreg(sock_hdl);
    p->para.type = SMARTCHAT;
    p->pic_req = 0;

    free(data);

    return err;
}

__attribute__((weak)) int get_app_music_volume(void)
{
    return 100;
}

static int turing_cb(int code, struct json_object *func, const char *asr)
{
    struct json_object *val = NULL;
    struct json_object *val1 = NULL;
    struct json_object *val2 = NULL;
    struct json_object *val3 = NULL;
    struct json_object *val4 = NULL;
    struct json_object *new = NULL;
    struct json_object *url = NULL;
    struct json_object *pageUrl = NULL;
    struct json_object *tip = NULL;
    struct json_object *isPlay = NULL;
    struct json_object *nameUrl = NULL;
    struct json_object *bookId = NULL;
    struct json_object *pageNum = NULL;

    int ret = 0;
    int play_page_url = 0;

    switch (code) {
    case TURING_EVENT_FUN_SLEEP_CTL:
    case TURING_EVENT_FUN_DANCE:
    case TURING_EVENT_FUN_SPORT_CTL:
        break;
    case TURING_EVENT_FUN_ALARM_CTL:
        if (!json_object_object_get_ex(func, "action", &val1)) {
            break;
        }
        if (!strcmp(json_object_get_string(val1), "delete")) {
            turing_alarm_del_all();
            break;
        }
        if (json_object_object_get_ex(func, "endDate", &val1) &&
            json_object_object_get_ex(func, "time", &val2) &&
            json_object_object_get_ex(func, "cycleType", &val3)) {
            json_object_object_get_ex(func, "timeLen", &val4);
            turing_alarm_set(json_object_get_string(val1), json_object_get_string(val2), json_object_get_string(val3), json_object_get_string(val4), NULL);
        }
        break;
    case TURING_EVENT_FUN_VOL_CTL:
        if (json_object_object_get_ex(func, "operate", &new)) {
            int is = json_object_get_int(new);
            if (json_object_object_get_ex(func, "volumn", &new)) {
                if (is) {
                    JL_turing_volume_change_notify(100);
                } else {
                    JL_turing_volume_change_notify(0);
                }
                break;
            }
            if (json_object_object_get_ex(func, "arg", &new)) {
                ret = json_object_get_int(new);
                if (is) {
                    JL_turing_volume_change_notify(get_app_music_volume() + ret);
                } else {
                    JL_turing_volume_change_notify(get_app_music_volume() - ret);
                }
            }
        }
        break;
    case TURING_EVENT_FUN_SCREEN_LIGHT:
        if (json_object_object_get_ex(func, "operate", &new)) {
            int is = json_object_get_int(new);
            if (json_object_object_get_ex(func, "arg", &new)) {
                ret = json_object_get_int(new);
                if (is) {
                    //增大亮度
                } else {
                    //减小亮度
                }
            }
        }
        break;
    case TURING_EVENT_FUN_PLAY_MUSIC:
    case TURING_EVENT_FUN_PLAY_STORY:
        if (!json_object_object_get_ex(func, "operate", &new)) {
            return -1;
        }
        int operate = json_object_get_int(new);
        if (!json_object_object_get_ex(func, "isPlay", &isPlay)) {
            return -1;
        }
        int is = json_object_get_int(isPlay);
        if (is == 1 && json_object_object_get_ex(func, "url", &url)) {
            if (json_object_get_string(url) && json_object_get_string_len(url) > 0) {
                strncpy(__this->url, json_object_get_string(url), sizeof(__this->url) - 1);
                __this->url[sizeof(__this->url) - 1] = 0;
                if (!json_object_object_get_ex(func, "tip", &tip)) {
                    return 0;	//播nlp的url
                }
                if (json_object_object_get_ex(func, "id", &val1)) {
                    __this->album_id = json_object_get_int(val1);
                }
                if (json_object_get_string(tip) && json_object_get_string_len(tip) > 0) {
                    JL_turing_media_speak_play(json_object_get_string(tip));
                    return 1;	//播tip的url
                }
                return 0;
            }
        }
        return -1;
    case TURING_EVENT_FUN_ANIMAL_SOUND:
    case TURING_EVENT_FUN_GUESS_SOUND:
        if (json_object_object_get_ex(func, "url", &url)) {
            if (json_object_get_string(url) && json_object_get_string_len(url) > 0) {
                strncpy(__this->url, json_object_get_string(url), sizeof(__this->url) - 1);
                __this->url[sizeof(__this->url) - 1] = 0;
            }
        }
        break;
    case TURING_EVENT_FUN_PICTURE_RECOGNITION:
        ret = -1;
        static int count = 0;
        if (json_object_object_get_ex(func, "intentCode", &val)) {
            TR_DEBUG_I("intentCode : %d\n", json_object_get_int(val));
        }
        if (json_object_object_get_ex(func, "operateState", &val)) {
            TR_DEBUG_I("operateState : %d\n", json_object_get_int(val));
        }
        if (json_object_object_get_ex(func, "funcData", &val) && json_object_object_get_ex(val, "ocr", &val1)) {
            __this->para.ocr_flag = json_object_get_int(val1);
        }
        if (json_object_object_get_ex(func, "funcData", &val) && json_object_object_get_ex(val, "finger", &val1)) {
            __this->para.finger_flag = json_object_get_int(val1);
        }
        if (json_object_object_get_ex(func, "titleData", &val)) {
            if (!json_object_object_get_ex(val, "nameUrl", &nameUrl) ||
                !json_object_object_get_ex(val, "bookId", &bookId)) {
                return -1;
            }
            if (__this->para.book_id != json_object_get_int(bookId)) {
                count = 0;
                JL_turing_picture_audio_play(json_object_get_string(nameUrl));
                if (__this->para.oral_string) {
                    free((void *)__this->para.oral_string);
                    __this->para.oral_string = NULL;
                }
            } else if (++count >= 15) { //连续十五次识别到同一本书，此时也需要播放封页音
                count = 0;
                JL_turing_picture_audio_play(json_object_get_string(nameUrl));
            }
            __this->para.book_id = json_object_get_int(bookId);
            ret = 1;
        }

        if (json_object_object_get_ex(func, "innerData", &val)) {
            count = 15;
            if (!json_object_object_get_ex(val, "pageNum", &pageNum) ||
                (!json_object_object_get_ex(val, "url", &url) &&
                 !json_object_object_get_ex(val, "pageUrl", &pageUrl)) ||
                !json_object_object_get_ex(val, "bookId", &bookId)) {
                return -1;
            }
            if (__this->para.book_id != json_object_get_int(bookId) || __this->para.page_num != json_object_get_int(pageNum)) {
                if (__this->para.oral_string) {
                    free((void *)__this->para.oral_string);
                    __this->para.oral_string = NULL;
                }
                if (url) {
                    JL_turing_picture_audio_play(json_object_get_string(url));
                }
                if (pageUrl) {
                    play_page_url = 1;
                    /* JL_turing_picture_audio_play(json_object_get_string(pageUrl)); */
                }
            }
            __this->para.book_id = json_object_get_int(bookId);
            __this->para.page_num = json_object_get_int(pageNum);
            __this->para.piece_index = -1;
            ret = 1;
        }

        if (json_object_object_get_ex(func, "hotZone", &val)) {
            if (!json_object_object_get_ex(val, "pieceIndex", &val1) ||
                !json_object_object_get_ex(val, "pieceUrl", &url)) {
                return ret;
            }
            if (url && __this->para.piece_index != json_object_get_int(val1)) {
                __this->para.piece_index = json_object_get_int(val1);
                JL_turing_picture_audio_play(json_object_get_string(url));
                play_page_url = 0;
            }
            if (json_object_object_get_ex(val, "pieceContent", &val2)) {
                if (__this->para.oral_string) {
                    free((void *)__this->para.oral_string);
                    __this->para.oral_string = NULL;
                }
                if (json_object_get_string_len(val2) > 0) {
                    __this->para.oral_flag = strchr(json_object_get_string(val2), ' ') ? 1 : 0;
                    __this->para.oral_string = strdup(json_object_get_string(val2));
                }
            }
            ret = 1;
        }

        if (play_page_url) {
            JL_turing_picture_audio_play(json_object_get_string(pageUrl));
        }
        return ret;
    default:
        break;
    }

    return 0;
}

u8 get_turing_ocr_flag(void)
{
    return __this->para.ocr_flag;
}

u8 get_turing_finger_flag(void)
{
    return __this->para.finger_flag;
}

__attribute__((weak)) int ai_platform_if_support_poweron_recommend(void)
{
    return 1;
}

__attribute__((weak)) int get_turing_camera_id(void)
{
    return 0;
}

__attribute__((weak)) int get_turing_typeflag(void)
{
    return 0;
}

__attribute__((weak)) void set_turing_request_para(u8 *speed, u8 *pitch, u8 *volume, u8 *tone)
{
}

static void init_turing_para(void)
{
    struct turing_para para = {0};

#if TURING_ENC_USE_OPUS == 0
    para.asr = ASR_SPEEX;
#else
    para.asr = ASR_OPUS;
#endif
    para.tts = TTS_MP3_16;
    para.flag = OUTPUT_ARS_TTS_TXT;
    para.real_time = STREAM_IDENTIFY;
    para.encode = NORMAL_ENCODE;
    para.type = SMARTCHAT;
    para.speed = 5;
    para.pitch = 5;
    para.volume = 8;
    para.tone = 21;	//1, 14, 21
    para.asr_lan = 0;
    para.tts_lan = 0;
    para.camera_id = get_turing_camera_id();	//用户需要填写上传给图灵的camera_id
    set_turing_request_para(&para.speed, &para.pitch, &para.volume, &para.tone);

    memset(__this, 0, sizeof(struct turing_var));
    __this->timeout_ms = 6000;
    __this->func_cb = turing_cb;
    memcpy(&__this->para, &para, sizeof(struct turing_para));
}

int __attribute__((weak)) get_turing_profile(char *api_key, char *user_id, char *aes_key)
{
    puts("no turing profile !!! \n");

    return -1;
}

static void turing_app_task(void *priv)
{
    struct turing_var *p = (struct turing_var *)priv;
    int msg[32];
    int err;
    u8 s_is_record = false;

    turing_alarm_init();
    init_turing_para();

#if 1
    while (0 != get_turing_profile(p->para.api_key, p->para.user_id, p->para.aes_key)) {
        puts("turing get profile fail ! \n");
        os_time_dly(200);
        if (p->exit_flag) {
            return;
        }
    }
#else
    strcpy(p->para.api_key, "d7a85956006c4b8699ad8e0d0c716dcc");
    strcpy(p->para.user_id, "ai00000000000000");
    strcpy(p->para.aes_key, "0P6v1GDfpYh4TxD3");
#endif

    p->connect_status = 1;

    if (ai_platform_if_support_poweron_recommend()) {
        turing_play_control(p, CONNECT_SUCC);
    }

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        switch (msg[1]) {
        case TURING_SPEAK_END:
            puts("===========TURING_SPEAK_END\n");
            if (p->url[0]) {
                JL_turing_media_audio_play(p->url);
            }
            break;
        case TURING_PICTURE_PLAY_END:
            break;
        case TURING_RECORD_START:
            if (!s_is_record) {
                p->para.voice_mode = msg[2] & 0x3;
                p->rec_stopping = 0;
                if (WECHAT_MODE == p->para.voice_mode) {
                    turing_recorder_start(8000, msg[2]);
                    s_is_record = true;
                    msg_notify_disable = 1;
                    break;
                }
                update_identify(&p->para);
                p->para.type = SMARTCHAT;
                if (ORAL_MODE == p->para.voice_mode) {
                    if (!p->para.oral_string) {
                        break;
                    }
                    p->para.type = ORAL_EVALUATION;	//口语评测
                }
                p->url[0] = 0;
                ASSERT(p->sock_hdl == NULL);
                msg_notify_disable = 1;
                p->sock_hdl = get_socket_fd(TURING_SERVER_HOST, p->timeout_ms);
                if (p->sock_hdl) {
                    turing_recorder_start(16000, msg[2]);
                    s_is_record = true;
                } else {
                    msg_notify_disable = 0;
                }
            }
            break;
        case TURING_RECORD_SEND:
            if (s_is_record) {
                if (p->rec_stopping) {
                    turing_recorder_stop(p->para.voice_mode);
                    s_is_record = false;
                }
                if (p->sock_hdl) {
                    p->para.index++;
                    if (p->rec_stopping) {
                        p->para.index = -p->para.index;
                        if (p->para.index == -1) {	//只有一包数据使用非流模式识别
                            p->para.real_time = NOT_STREAM_IDENTIFY;
                            p->para.index = 0;
                        }
                    }

                    u8 wait_opt = p->para.index < 0 ? RESP_WAIT_END : (p->para.real_time == NOT_STREAM_IDENTIFY ? RESP_WAIT_NORMAL : RESP_NO_WAIT);
                    err = requestAndResponse(p->sock_hdl, (const u8 *)msg[2], msg[3], p, wait_opt);
                    if (err || p->para.index <= 0) {
                        sock_unreg(p->sock_hdl);
                        p->sock_hdl = NULL;
                    }
                }
                if (p->rec_stopping) {
                    msg_notify_disable = 0;
                }
                os_sem_post((OS_SEM *)msg[4]);
            }
            break;
        case TURING_RECORD_STOP:
            if (s_is_record) {
                p->rec_stopping = 1;
                if (WECHAT_MODE == p->para.voice_mode) {
                    turing_recorder_stop(p->para.voice_mode);
                    if (p->sock_hdl) {
                        sock_unreg(p->sock_hdl);
                        p->sock_hdl = NULL;
                    }
                    s_is_record = false;
                    msg_notify_disable = 0;
                }
            }
            break;
        case TURING_RECORD_ERR:
            if (s_is_record) {
                s_is_record = false;
                turing_recorder_stop(p->para.voice_mode);
                if (p->sock_hdl) {
                    sock_unreg(p->sock_hdl);
                    p->sock_hdl = NULL;
                }
                if (p->para.voice_mode != WECHAT_MODE) {
                    /* JL_turing_rec_err_notify(NULL); */
                }
                msg_notify_disable = 0;
            }
            break;
        case TURING_PREVIOUS_SONG:
            puts("===========TURING_PREVIOUS_SONG\n");
            turing_play_control(p, PLAY_PREV);
            break;
        case TURING_NEXT_SONG:
        case TURING_MEDIA_END:
            puts("===========TURING_NEXT_SONG\n");
            turing_play_control(p, PLAY_NEXT);
            break;
        case TURING_PICTURE_RECOG:
            puts("===========TURING_PICTURE_RECOGNITION\n");
            static u8 count;
            if (msg[2] && msg[3] > 0) {
                /*播放无法识别提示音*/
                if (turing_picture_recognition(p, (u8 *)msg[2], msg[3])) {
                    if (++count > 30) {
                        count = 0;
                        /* page_turning_play_voice("030.mp3"); */
                    }
                } else {
                    count = 0;
                }
            }
            break;
        case TURING_COLLECT_RESOURCE:
            os_taskq_post("wechat_api_task", 3, WECHAT_COLLECT_RESOURCE, 0, __this->album_id);
            break;
        case TURING_QUIT:
            if (p->sock_hdl) {
                sock_unreg(p->sock_hdl);
                p->sock_hdl = NULL;
            }
            if (__this->para.oral_string) {
                free((void *)__this->para.oral_string);
                __this->para.oral_string = NULL;
            }
            return;
        default:
            break;
        }
    }
}

int turing_app_init(void)
{
    if (!turing_app_init_flag) {
        turing_app_init_flag = 1;
        __this->exit_flag = 0;
        msg_notify_disable = 0;
        __this->connect_status = 0;
        __this->pic_req = 0;
        return thread_fork("turing_app_task", 22, 1536, 128, &turing_app_task_pid, turing_app_task, __this);
    }
    return -1;
}

void turing_app_uninit(void)
{
    if (turing_app_init_flag) {
        turing_app_init_flag = 0;
        __this->connect_status = 0;
        __this->exit_flag = 1;
        while (__this->sock_hdl) {
            os_time_dly(10);
        }
        do {
            if (OS_Q_FULL != os_taskq_post("turing_app_task", 1, TURING_QUIT)) {
                break;
            }
            TR_DEBUG_I("turing_app_task send msg QUIT timeout \n");
            os_time_dly(5);
        } while (1);
        thread_kill(&turing_app_task_pid, KILL_WAIT);
    }
}

__attribute__((weak)) int page_turning_recognition(void *data, u32 length, u8 buf_copy)
{
    int err;
    int msg[4];

    if (!__this->connect_status || msg_notify_disable) {
        return -1;
    }

    u8 *buffer = NULL;
    if (buf_copy) {
        buffer = (u8 *)malloc(length);
        if (!buffer) {
            return -1;
        }
        memcpy(buffer, data, length);
    } else {
        buffer = (u8 *)data;
    }

    if (__this->pic_req) {
        __this->exit_flag |= BIT(2);
    }

    msg[0] = TURING_PICTURE_RECOG;
    msg[1] = (int)buffer;
    msg[2] = length;

    err = os_taskq_post_type("turing_app_task", Q_USER, 3, msg);
    if (err != OS_NO_ERR) {
        puts("turing pic req full \n");
    }

    if (err != OS_NO_ERR && buf_copy) {
        free(buffer);
    }

    return err;
}

u8 turing_app_get_connect_status(void)
{
    return __this->connect_status;
}

u8 get_turing_msg_notify(void)
{
    return msg_notify_disable;
}

u8 get_turing_rec_is_stopping(void)
{
    return __this->rec_stopping;
}
