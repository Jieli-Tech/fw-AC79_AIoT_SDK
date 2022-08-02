#include "mbedtls/md5.h"
#include "mbedtls/md.h"
#include "mbedtls/md_internal.h"

#include "http/http_cli.h"
#include "aligenie_os.h"
#include "json_c/json_tokener.h"

static char authcode[10] = {0};

static int URLEncode(const char *str, const int strSize, char *result, const int resultSize)
{
    int i;
    int j = 0;//for result index
    char ch;

    if ((str == NULL) || (result == NULL) || (strSize <= 0) || (resultSize <= 0)) {
        return 0;
    }

    for (i = 0; (i < strSize) && (j < resultSize); ++i) {
        ch = str[i];
        if (((ch >= 'A') && (ch < 'Z')) ||
            ((ch >= 'a') && (ch < 'z')) ||
            ((ch >= '0') && (ch < '9'))) {
            result[j++] = ch;
        } else if (ch == ' ') {
            result[j++] = '+';
        } else if (ch == '.' || ch == '-' || ch == '_' || ch == '*') {
            result[j++] = ch;
        } else {
            if (j + 3 < resultSize) {
                sprintf(result + j, "%%%02X", (unsigned char)ch);
                j += 3;
            } else {
                return 0;
            }
        }
    }

    result[j] = '\0';
    return j;
}

static int http_callback(char *buf, void *priv)
{
    printf("http boby :%s\n", buf);

    json_object *new_obj = NULL;
    json_object *parm = NULL;
    json_object *model = NULL;

    new_obj = json_tokener_parse(buf);
    if (!new_obj) {
        return -1;
    }

    parm = json_object_object_get(new_obj, "ailab_aicloud_top_device_authcode_get_response");
    if (parm) {
        model = json_object_object_get(parm, "model");
        if (model) {
            const char *model_value = json_object_get_string(model);
            printf("model_value  %s \n", model_value);
            memcpy(authcode, model_value, json_object_get_string_len(model));
        }
    }

    json_object_put(new_obj);

    return 0;
}

static int __https_get_mothed(const char *url, int (*cb)(char *, void *), void *priv)
{
    int error = 0;
    http_body_obj http_body_buf;
    httpcli_ctx ctx;
    memset(&http_body_buf, 0x0, sizeof(http_body_obj));
    memset(&ctx, 0x0, sizeof(httpcli_ctx));

    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = 4 * 1024;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *)malloc(http_body_buf.buf_len * sizeof(char));

    ctx.url = url;
    ctx.priv = &http_body_buf;
    ctx.connection = "close";
    ctx.timeout_millsec = 10000;

    error = httpcli_get(&ctx);

    if (error == HERROR_OK) {
        if (cb) {
            error = cb(http_body_buf.p, priv);
        } else {
            error = 0;
        }
    } else {
        error = -1;
    }

    if (http_body_buf.p) {
        free(http_body_buf.p);
    }

    return error;
}

struct http_parm {
    char key[64];
    char value[256];
};

static struct http_parm http_parm_array[] = {
    {"app_key", "31235509"},
    {"ext", "sadsada"},
    {"force_sensitive_param_fuzzy", "true"},
    {"format", "json"},
    {"method", "taobao.ailab.aicloud.top.device.authcode.get"},
    {"partner_id", "top-apitools"},
    {"schema", "26A9F86171B72633E335011DAFE44753"},
    {"sign_method", "hmac"},
    {"timestamp", "2020-09-19 11:34:40"},
    {"user_id", "DARSTPK9X8ID"},
    {"utd_id", "1111"},
    {"v", "2.0"},
};

int aligenie_get_authcode(char *_authcode)
{
    int record_i = 0;
    char timebuf[64];
    int ret = 0;

    char *tmp = calloc(4096, 1);
    if (!tmp) {
        return -1;
    }

    /*搞时间戳*/
    for (int i = 0; i < ARRAY_SIZE(http_parm_array); i++) {
        if (!strcmp("timestamp", http_parm_array[i].key)) {
            struct tm pt;
            time_t t = time(NULL);
            t += 28800;//没有时区，自己加
            localtime_r(&t, &pt);

            sprintf(timebuf, "%04d-%02d-%02d %02d:%02d:%02d", pt.tm_year + 1900, pt.tm_mon + 1, pt.tm_mday, pt.tm_hour, pt.tm_min, pt.tm_sec);
            memset(http_parm_array[i].value, 0, sizeof(http_parm_array[i].value));

            memcpy(http_parm_array[i].value, timebuf, strlen("YYYY-MM-DD HH:MM:SS"));
            record_i = i;
        }
    }

    for (int i = 0; i < ARRAY_SIZE(http_parm_array); i++) {
        sprintf(tmp, "%s%s%s", tmp, http_parm_array[i].key, http_parm_array[i].value);
    }

    printf("tmp=>%s\n", tmp);

    u8 out[16] = {0};
    if (0 != mbedtls_md_hmac(&mbedtls_md5_info, (unsigned char *)"7c1d98a8a4a6a16bb7dfe4a72ba089e7", strlen("7c1d98a8a4a6a16bb7dfe4a72ba089e7"), (unsigned char *)tmp, strlen(tmp), out)) {
        return -1;
    }

    URLEncode(timebuf + strlen("YYYY-MM-DD"), strlen(" HH:MM:SS"), http_parm_array[record_i].value + strlen("YYYY-MM-DD"), 256); //只有这串urlencode 其他一起不行

    memset(tmp, 0, 4096);

    char sign[128] = {0};

    for (int i = 0 ; i < 16 ; i++) {
        sprintf(sign, "%s%02X", sign, out[i]); //一定要大写才行
    }

    printf("sign=>%s\n", sign);

    //url

    int url_len = sprintf(tmp, "https://gw.api.taobao.com/router/rest?sign=%s", sign);

    // int url_len = sprintf(tmp,"https://203.119.217.43/router/rest?sign=%s",sign);

    for (int i = 0; i < ARRAY_SIZE(http_parm_array); i++) {
        sprintf(tmp, "%s&%s=%s", tmp, http_parm_array[i].key, http_parm_array[i].value);
    }
    printf("url=>%s\n", tmp);

    ret = __https_get_mothed(tmp, http_callback, NULL);

    strcpy(_authcode, authcode);

    free(tmp);

    return ret;
}

//aligenie开发网站上有 认真找
int aligenie_set_app_key(const char *app_key)
{
    for (int i = 0; i < ARRAY_SIZE(http_parm_array); i++) {
        if (!strcmp("app_key", http_parm_array[i].key)) {
            memset(http_parm_array[i].value, 0, sizeof(http_parm_array[i].value));
            memcpy(http_parm_array[i].value, app_key, strlen(app_key));
            return 0;
        }
    }
    return -1;
}
//随便写
int aligenie_set_ext(const char *ext)
{
    for (int i = 0; i < ARRAY_SIZE(http_parm_array); i++) {
        if (!strcmp("ext", http_parm_array[i].key)) {
            memset(http_parm_array[i].value, 0, sizeof(http_parm_array[i].value));
            memcpy(http_parm_array[i].value, ext, strlen(ext));
            return 0;
        }
    }
    return -1;

}
//aligenie开发网站上有 认真找
int aligenie_set_schema(const char *schema)
{
    for (int i = 0; i < ARRAY_SIZE(http_parm_array); i++) {
        if (!strcmp("schema", http_parm_array[i].key)) {
            memset(http_parm_array[i].value, 0, sizeof(http_parm_array[i].value));
            memcpy(http_parm_array[i].value, schema, strlen(schema));
            return 0;
        }
    }
    return -1;
}
//extern AGRET_E ag_sdk_set_register_info(const char* mac, const char * extUserId, const char * authCode, char forcelySend);
//跟这一样就行  extUserId
int aligenie_set_user_id(const char *user_id)
{
    for (int i = 0; i < ARRAY_SIZE(http_parm_array); i++) {
        if (!strcmp("user_id", http_parm_array[i].key)) {
            memset(http_parm_array[i].value, 0, sizeof(http_parm_array[i].value));
            memcpy(http_parm_array[i].value, user_id, strlen(user_id));
            return 0;
        }
    }
    return -1;
}
//随便写
int aligenie_set_utd_id(const char *utd_id)
{
    for (int i = 0; i < ARRAY_SIZE(http_parm_array); i++) {
        if (!strcmp("utd_id", http_parm_array[i].key)) {
            memset(http_parm_array[i].value, 0, sizeof(http_parm_array[i].value));
            memcpy(http_parm_array[i].value, utd_id, strlen(utd_id));
            return 0;
        }
    }
    return -1;
}

