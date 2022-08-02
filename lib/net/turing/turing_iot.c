#include "turing_iot.h"
#include "turing_mqtt.h"
#include "http/http_cli.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "os/os_api.h"
#include "server/ai_server.h"

struct turing_info {
    httpcli_ctx ctx;
    const char *api_key;
    const char *device_id;
    u8   auth_flag;
    char client_id[65];
    char topic[129];
    char res_media_id[65];
    int  song_id;
    char song_url[1024];
    char song_title[256];
};

static OS_MUTEX iot_mutex;
static u8 turing_iot_close_flag = 1;

static int __http_post_mothed(const char *url, char *buffer, u32 length, int (*cb)(char *, void *), void *priv)
{
    int error = 0;
    http_body_obj http_body_buf;
    struct turing_info *f = (struct turing_info *)priv;
    httpcli_ctx *ctx = &f->ctx;

    log_d("url->%s\n", url);

    memset(&http_body_buf, 0x0, sizeof(http_body_obj));
    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = 4 * 1024;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *) malloc(http_body_buf.buf_len * sizeof(char));
    if (http_body_buf.p == NULL) {
        return -1;
    }

    os_mutex_pend(&iot_mutex, 0);
    if (turing_iot_close_flag) {
        free(http_body_buf.p);
        os_mutex_post(&iot_mutex);
        return -1;
    }
    memset(ctx, 0x0, sizeof(httpcli_ctx));
    ctx->url = url;
    ctx->priv = &http_body_buf;
    ctx->post_data = buffer;
    ctx->data_len  = length;
    ctx->data_format  = "application/json";
    ctx->connection = "close";
    ctx->timeout_millsec = 6000;

    error = httpcli_post(ctx);
    os_mutex_post(&iot_mutex);
    if (error == HERROR_OK) {
        log_d("recv : %s\n", http_body_buf.p);
        if (cb) {
            error = cb(http_body_buf.p, priv);
        }
    } else {
        error = -1;
    }

    if (http_body_buf.p) {
        free(http_body_buf.p);
    }

    return error;
}

int turing_iot_set_key(void *priv, const char *api_key, const char *device_id)
{
    struct turing_info *f = (struct turing_info *)priv;
    f->api_key = api_key;
    f->device_id = device_id;
    f->auth_flag = 1;
    return 0;
}

void *turing_iot_init(void)
{
    struct turing_info *f = (struct turing_info *)calloc(sizeof(struct turing_info), 1);
    if (!f) {
        return NULL;
    }

    if (!os_mutex_valid(&iot_mutex)) {
        os_mutex_create(&iot_mutex);
    }
    turing_iot_close_flag = 0;

    return f;
}

int turing_iot_uninit(void *priv)
{
    free(priv);
    return 0;
}

int turing_iot_qiut_notify(void *priv)
{
    int err;
    struct turing_info *f = (struct turing_info *)priv;
    turing_iot_close_flag = 1;

    if (!f) {
        return -1;
    }

    do {
        http_cancel_dns(&f->ctx);
        err = os_mutex_pend(&iot_mutex, 10);
        if (!err) {
            break;
        }
    } while (1);
    os_mutex_post(&iot_mutex);

    return 0;
}

__attribute__((weak)) const char *get_turing_version_code(void)
{
    return "0.0.0";
}

//49965
#define AUSTR "{\"apiKey\":\"%s\",\"productId\":\"%d\",\"devices\":[{\"deviceId\":\"%s\",\"mac\":\"%s\"}]}"

int turing_iot_authorize(void *priv, int product_id, const char *mac)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[512];

    int length = snprintf(buffer, sizeof(buffer), AUSTR, f->api_key, product_id, f->device_id, mac);

    ret = __http_post_mothed(IOT_ADDR"/vendor/new/authorize", buffer, length, NULL, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
        return -1;
    }

    return ret;
}

#define CHAUSTR "{\"apiKey\":\"%s\",\"deviceId\":\"%s\"}"

int turing_iot_check_authorize(void *priv)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[256];

    int length = snprintf(buffer, sizeof(buffer), CHAUSTR, f->api_key, f->device_id);

    ret = __http_post_mothed(IOT_ADDR"/vendor/new/device/status", buffer, length, NULL, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
        return -1;
    }

    return ret;
}

/************************************************************/

/*获取微信accessToke*/

/************************************************************/

static int get_topic_callback(char *data, void *priv)
{
    struct turing_info *f = (struct turing_info *)priv;
    int err = -1;
    json_object *new_obj = NULL;
    json_object *data1 = NULL;
    json_object *clientId = NULL;
    json_object *topic = NULL;

    new_obj = json_tokener_parse(data);
    if (!json_object_object_get_ex(new_obj, "payload", &data1)) {
        goto __result_exit;
    }

    if (!json_object_object_get_ex(data1, "clientId", &clientId)) {
        goto __result_exit;
    }

    if (!json_object_get_string(clientId) || !json_object_get_string_len(clientId)) {
        goto __result_exit;
    }

    if (!json_object_object_get_ex(data1, "topic", &topic)) {
        goto __result_exit;
    }

    if (!json_object_get_string(topic) || !json_object_get_string_len(topic)) {
        goto __result_exit;
    }

    strncpy(f->client_id, json_object_get_string(clientId), sizeof(f->client_id));
    f->client_id[sizeof(f->client_id) - 1] = 0;
    strncpy(f->topic, json_object_get_string(topic), sizeof(f->topic));
    f->topic[sizeof(f->topic) - 1] = 0;

    err = 0;

__result_exit:

    json_object_put(new_obj);

    return err;
}

#define GETOSTR "{\"apiKey\":\"%s\",\"deviceId\":\"%s\"}"

int turing_iot_get_topic(void *priv)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[256];

    int length = snprintf(buffer, sizeof(buffer), GETOSTR, f->api_key, f->device_id);

    ret = __http_post_mothed(IOT_ADDR"/iot/mqtt/topic", buffer, length, get_topic_callback, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
    }

    return ret;
}

#define NOSTSTR "{\"apiKey\":\"%s\",\"deviceId\":\"%s\",\"type\":%d,\"status\":{%s}}"

int turing_iot_notify_status(void *priv, u8 type, const char *status_buffer)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;

    char *buffer = malloc(1024);
    if (!buffer) {
        return -1;
    }

    int length = snprintf(buffer, 1024, NOSTSTR, f->api_key, f->device_id, type, status_buffer);

    ret = __http_post_mothed(IOT_ADDR"/iot/status/notify", buffer, length, NULL, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
    }

    free(buffer);
    return ret;
}

/************************************************************/
#define BOUNDARY        			"--jieli&boundary&e3#due*hf$n"
static const char http_head_tab[] =
    "POST /resources/file/upload?apiKey=%s HTTP/1.1\r\n"
    "Host: iot-ai.tuling123.com\r\n"
    "User-Agent: */*\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: close\r\n"
    "Content-type: multipart/form-data; boundary="BOUNDARY"\r\n"
    "Content-Length: %d\r\n"
    "\r\n";

static const char http_data_f[] =
    "--"BOUNDARY"\r\n"
    "Content-Disposition: form-data; name=\"speech\"; filename=\"%s\"\r\n"
    "Content-Type: %s\r\n"
    "\r\n";

static const char http_data_b[] =
    "\r\n--"BOUNDARY"--\r\n";

static int __post_resources(const char *url, u8 *buffer, u32 length, int (*cb)(char *, void *), void *priv)
{
    int error = 0;
    int url_buf_len = 2048;
    char *url_buf = NULL;
    http_body_obj http_body_buf;

    struct turing_info *f = (struct turing_info *)priv;
    httpcli_ctx *ctx = &f->ctx;

    char *http_body_before_buf = NULL;
    int http_body_before_len = 0;

    memset(&http_body_buf, 0x0, sizeof(http_body_obj));

    url_buf = calloc(1, url_buf_len + sizeof(http_data_f) + 64);
    if (!url_buf) {
        goto __checkin_exit;
    }
    http_body_before_buf = url_buf + url_buf_len;

    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = 1024;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *) malloc(http_body_buf.buf_len * sizeof(char));
    if (http_body_buf.p == NULL) {
        goto __checkin_exit;
    }

    http_body_before_len = snprintf(http_body_before_buf, sizeof(http_data_f) + 64, http_data_f, "voice.amr", "application/octet-stream");
    if (http_body_before_len >= sizeof(http_data_f) + 64) {
        goto __checkin_exit;
    }

    int http_more_data_addr[3];
    int http_more_data_len[3 + 1];
    http_more_data_addr[0] = (int)(http_body_before_buf);
    http_more_data_addr[1] = (int)(buffer);
    http_more_data_addr[2] = (int)(http_data_b);
    http_more_data_len[0] = http_body_before_len;
    http_more_data_len[1] = length;
    http_more_data_len[2] = strlen(http_data_b);
    http_more_data_len[3] = 0;

    memset(ctx, 0x0, sizeof(httpcli_ctx));
    ctx->more_data = http_more_data_addr;
    ctx->more_data_len = http_more_data_len;
    ctx->timeout_millsec = 8000;

    ctx->url = url;
    ctx->priv = &http_body_buf;

    snprintf(url_buf, url_buf_len, http_head_tab, f->api_key, (int)(http_body_before_len + length + strlen(http_data_b)));
    ctx->user_http_header = url_buf;

    error = httpcli_post(ctx);
    if (error == HERROR_OK) {
        if (cb) {
            error = cb(http_body_buf.p, priv);
        }
    } else {
        error = -1;
    }

__checkin_exit:
    if (url_buf) {
        free(url_buf);
    }
    if (http_body_buf.p) {
        free(http_body_buf.p);
    }

    return error;
}

static int post_res_callback(char *data, void *priv)
{
    struct turing_info *f = (struct turing_info *)priv;
    int err = -1;
    json_object *new_obj = NULL;
    json_object *data1 = NULL;

    new_obj = json_tokener_parse(data);
    if (!json_object_object_get_ex(new_obj, "payload", &data1)) {
        goto __result_exit;
    }

    if (!json_object_get_string(data1) || !json_object_get_string_len(data1)) {
        goto __result_exit;
    }

    strncpy(f->res_media_id, json_object_get_string(data1), sizeof(f->res_media_id));
    f->res_media_id[sizeof(f->res_media_id) - 1] = 0;

    log_d("f->res_media_id=%s\n", f->res_media_id);

    err = 0;

__result_exit:

    json_object_put(new_obj);

    return err;
}

/*不使用 toUser参数*/
#define MESSSTR "{\"apiKey\":\"%s\",\"deviceId\":\"%s\",\"type\":%d,\"message\":{\"mediaId\":\"%s\"}}"

int turing_iot_send_message(void *priv, u8 type)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[512];

    int length = snprintf(buffer, sizeof(buffer), MESSSTR, f->api_key, f->device_id, type, f->res_media_id);

    ret = __http_post_mothed(IOT_ADDR"/iot/message/send", buffer, length, NULL, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
        return -1;
    }

    return ret;
}

int turing_iot_post_res(u8 *buf, u32 len)
{
    int ret = 0;
    char buffer[256];

    struct turing_info *f = get_turing_iot_hdl();

    if (!f || turing_iot_close_flag || !f->auth_flag) {
        return -1;
    }

    os_mutex_pend(&iot_mutex, 0);	//find xueyong
    if (turing_iot_close_flag) {
        os_mutex_post(&iot_mutex);
        return -1;
    }

    int length = snprintf(buffer, sizeof(buffer), IOT_ADDR"/resources/file/upload?apiKey=%s", f->api_key);

    ret = __post_resources(buffer, buf, len, post_res_callback, f);

    if (ret) {
        log_e("%s  http_post res fail\n", __func__);
        goto exit;
    }

    length = snprintf(buffer, sizeof(buffer), MESSSTR, f->api_key, f->device_id, 0, f->res_media_id);

    ret = __http_post_mothed(IOT_ADDR"/iot/message/send", buffer, length, NULL, f);

    if (ret) {
        log_e("%s  http_post send message fail\n", __func__);
    }

exit:
    os_mutex_post(&iot_mutex);

    return ret;
}

/************************************************************/
static int get_music_res_callback(char *data, void *priv)
{
    struct turing_info *f = (struct turing_info *)priv;
    int err = -1;
    json_object *new_obj = NULL;
    json_object *data1 = NULL;
    json_object *url = NULL;
    json_object *title = NULL;
    json_object *id = NULL;
    json_object *tip = NULL;
    char buf[128];

    new_obj = json_tokener_parse(data);
    if (!json_object_object_get_ex(new_obj, "payload", &data1)) {
        goto __result_exit;
    }

    if (!json_object_object_get_ex(data1, "url", &url)) {
        goto __result_exit;
    }

    if (!json_object_get_string(url) || !json_object_get_string_len(url)) {
        goto __result_exit;
    }

    if (!json_object_object_get_ex(data1, "name", &title)) {
        goto __result_exit;
    }

    if (!json_object_get_string(title) || !json_object_get_string_len(title)) {
        goto __result_exit;
    }

    if (!json_object_object_get_ex(data1, "id", &id)) {
        goto __result_exit;
    }

    strncpy(f->song_url, json_object_get_string(url), sizeof(f->song_url));
    f->song_url[sizeof(f->song_url) - 1] = 0;
    strncpy(f->song_title, json_object_get_string(title), sizeof(f->song_title));
    f->song_title[sizeof(f->song_title) - 1] = 0;
    f->song_id = json_object_get_int(id);

    snprintf(buf, sizeof(buf), "\"title\":\"%s\",\"mediaId\":%d,\"play\":1", json_object_get_string(title), json_object_get_int(id));

    if (0 == turing_iot_notify_status(f, MUSIC_STATE, buf)) {
        err = 0;
    }

__result_exit:

    json_object_put(new_obj);

    return err;
}

/*不使用 toUser参数*/
#define AURESTR "{\"apiKey\":\"%s\",\"deviceId\":\"%s\",\"type\":%d}"

int turing_iot_get_music_res(void *priv, u8 type)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[256];

    int length = snprintf(buffer, sizeof(buffer), AURESTR, f->api_key, f->device_id, type);

    ret = __http_post_mothed(IOT_ADDR"/v2/iot/audio", buffer, length, get_music_res_callback, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
    }

    return ret;
}

#define COLLECT "{\"apiKey\":\"%s\",\"deviceId\":\"%s\",\"id\":%d}"
#define PLAY_COLLECT "{\"apiKey\":\"%s\",\"deviceId\":\"%s\",\"type\":0}"

int turing_iot_collect_resource(void *priv, int album_id, u8 is_play)
{
    int length = 0;
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[256];

    if (is_play) {
        length = snprintf(buffer, sizeof(buffer), PLAY_COLLECT, f->api_key, f->device_id);
        ret = __http_post_mothed(IOT_ADDR"/v2/iot/collect", buffer, length, get_music_res_callback, priv);
    } else {
        length = snprintf(buffer, sizeof(buffer), COLLECT, f->api_key, f->device_id, album_id);
        ret = __http_post_mothed(IOT_ADDR"/v2/iot/audio/collect", buffer, length, NULL, priv);
    }

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
    }

    return ret;
}

#define INITVER "{\"apiKey\":\"%s\",\"deviceId\":\"%s\",\"currentVersion\":\"%s\"}"

int turing_iot_report_version(void *priv)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[256];

    int length = snprintf(buffer, sizeof(buffer), INITVER, f->api_key, f->device_id, get_turing_version_code());

    ret = __http_post_mothed(IOT_ADDR"/iot/v2/ota/initVersion", buffer, length, NULL, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
    }

    return ret;
}

#define UPDATESUCC "{\"apiKey\":\"%s\",\"deviceId\":\"%s\",\"id\":%d,\"updateStatus\":1}"

static int turing_iot_update_success(void *priv, int version_id)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[256];

    int length = snprintf(buffer, sizeof(buffer), UPDATESUCC, f->api_key, f->device_id, version_id);

    ret = __http_post_mothed(IOT_ADDR"/iot/v2/ota/updateStatus", buffer, length, NULL, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
    }

    return ret;
}

static int ota_update_version_info(void *priv, const char *buf)
{
    json_object *new_obj = NULL;
    json_object *payload = NULL;
    json_object *data = NULL;
    json_object *data2 = NULL;
    int id = 0;
    int ret = -1;

    new_obj = json_tokener_parse(buf);
    if (!new_obj) {
        return -1;
    }
    if (!json_object_object_get_ex(new_obj, "payload", &payload)) {
        goto __result_exit;
    }
    if (!json_object_object_get_ex(payload, "updateStatus", &data)) {
        goto __result_exit;
    }

    if (0 != json_object_get_int(data)) {
        goto __result_exit;
    }
    if (!json_object_object_get_ex(payload, "agree", &data)) {
        goto __result_exit;
    }
    if (1 != json_object_get_int(data)) {
        goto __result_exit;
    }
    if (!json_object_object_get_ex(payload, "id", &data)) {
        goto __result_exit;
    }
    id = json_object_get_int(data);
    if (0 == id) {
        goto __result_exit;
    }
    if (!json_object_object_get_ex(payload, "nextVersion", &data2)) {
        goto __result_exit;
    }
    if (NULL == json_object_get_string(data2) || 0 == json_object_get_string_len(data2)) {
        goto __result_exit;
    }
    if (!json_object_object_get_ex(payload, "currentVersion", &data)) {
        goto __result_exit;
    }
    if (NULL == json_object_get_string(data) || 0 == json_object_get_string_len(data)) {
        goto __result_exit;
    }
    if (strcmp(json_object_get_string(data), get_turing_version_code())) {
        goto __result_exit;
    }
    if (!strcmp(json_object_get_string(data2), json_object_get_string(data))) {
        goto __result_exit;
    }
    if (!json_object_object_get_ex(payload, "firmwareUrl", &data)) {
        goto __result_exit;
    }
    if (NULL == json_object_get_string(data) || 0 == json_object_get_string_len(data)) {
        goto __result_exit;
    }

    extern void turing_upgrade_notify(int event, void *arg);
    turing_upgrade_notify(AI_SERVER_EVENT_UPGRADE, NULL);

    extern int get_update_data(const char *url);
    ret = get_update_data(json_object_get_string(data));
    if (!ret) {
        ret = turing_iot_update_success(priv, id);
        char *msg = NULL;
        msg = (char *)malloc(json_object_get_string_len(data2) + 1);
        if (msg) {
            strcpy(msg, json_object_get_string(data2));
        }
        turing_upgrade_notify(AI_SERVER_EVENT_UPGRADE_SUCC, msg);
    } else {
        turing_upgrade_notify(AI_SERVER_EVENT_UPGRADE_FAIL, NULL);
    }

__result_exit:

    if (new_obj) {
        json_object_put(new_obj);
    }

    return ret;
}

int turing_iot_check_update(void *priv)
{
    int error = 0;
    http_body_obj http_body_buf;
    struct turing_info *f = (struct turing_info *)priv;
    httpcli_ctx *ctx = &f->ctx;
    char url[128];

    memset(&http_body_buf, 0x0, sizeof(http_body_obj));
    http_body_buf.recv_len = 0;
    http_body_buf.buf_len = 4 * 1024;
    http_body_buf.buf_count = 1;
    http_body_buf.p = (char *) malloc(http_body_buf.buf_len * sizeof(char));
    if (http_body_buf.p == NULL) {
        return -1;
    }

    os_mutex_pend(&iot_mutex, 0);
    if (turing_iot_close_flag) {
        free(http_body_buf.p);
        os_mutex_post(&iot_mutex);
        return -1;
    }

    snprintf(url, sizeof(url), "http://iot-ai.tuling123.com/iot/v2/ota/version?apiKey=%s&deviceId=%s", f->api_key, f->device_id);
    memset(ctx, 0x0, sizeof(httpcli_ctx));
    ctx->url = url;
    ctx->priv = &http_body_buf;
    ctx->connection = "close";

    error = httpcli_get(ctx);
    os_mutex_post(&iot_mutex);
    if (error == HERROR_OK) {
        log_d("ota recv : %s\n", http_body_buf.p);
        error = ota_update_version_info(priv, http_body_buf.p);
    }

    if (http_body_buf.p) {
        free(http_body_buf.p);
    }

    return error;
}

#define BT_CFG_BIND "[{\"apiKey\":\"%s\",\"deviceId\":\"%s\",\"openId\":\"%s\"}]"

int turing_iot_bt_cfg_bind(void *priv, const char *openid)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[256];

    int length = snprintf(buffer, sizeof(buffer), BT_CFG_BIND, f->api_key, f->device_id, openid);

    ret = __http_post_mothed(IOT_ADDR"/open/device/bind-wechat-user", buffer, length, NULL, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
    }

    return ret;
}

#define SINVOICE_CFG_BIND "{\"apiKey\":\"%s\",\"deviceId\":\"%s\",\"openId\":\"%s\"}"

int turing_iot_sinvoice_cfg_bind(void *priv, const char *openid)
{
    int ret = 0;
    struct turing_info *f = (struct turing_info *)priv;
    char buffer[256];

    int length = snprintf(buffer, sizeof(buffer), SINVOICE_CFG_BIND, f->api_key, f->device_id, openid);

    ret = __http_post_mothed(IOT_ADDR"/iot/bind_from_sinvoice", buffer, length, NULL, priv);

    if (ret) {
        log_e("%s  http_post fail\n", __func__);
    }

    return ret;
}


/************************************************************/
char *turing_iot_get_client_id(void *priv)
{
    struct turing_info *f = (struct turing_info *)priv;
    return f->client_id;
}

char *turing_iot_get_topic_uuid(void *priv)
{
    struct turing_info *f = (struct turing_info *)priv;
    return f->topic;
}

char *turing_iot_get_url(void *priv)
{
    struct turing_info *f = (struct turing_info *)priv;
    return f->song_url;
}

int turing_iot_get_song_title(void *priv, char *title, int *id)
{
    struct turing_info *f = (struct turing_info *)priv;
    strcpy(title, f->song_title);
    *id = f->song_id;
    return 0;
}
