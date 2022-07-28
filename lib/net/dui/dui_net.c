#include "server/ai_server.h"
#include "system/timer.h"
#include "http/http_cli.h"
#include "json_c/json.h"
#include "json_c/json_tokener.h"
#include "dui.h"
#include "iot.h"

#ifdef DUI_MEMHEAP_DEBUG
#include "mem_leak_test.h"
#endif

#define USER_HTTP_HEADER    \
    "POST /auth/device/register?productKey=%s&format=%s&productId=%s&timestamp=%s&nonce=%s&sig=%s HTTP/1.1\r\n"\
    "Content-Type: application/json\r\n"\
    "Content-Length: %d\r\n"\
    "Host: auth.dui.ai\r\n"\
    "Accept: */*\r\n"\
    "Connection: close\r\n\r\n"   //对于设备端，一般要使用这个属性

#define USER_HTTP_BODY     \
    "{\"platform\"  : \"rtos\",\"deviceName\" : \"%s\"}"


#define PLAY_MAX_LIST        20

typedef struct  {
    struct list_head entry;
    char linkUrl[256];//不够用再加
    char imageUrl[256];
    char title[128];
    char album[128];
    char subTitle[128];
    int match;
} PLAY_LIST_INFO;

//汉字编码的utf-8编码
//提醒
static const char tixing[] = {0xE6, 0x8F, 0x90, 0xE9, 0x86, 0x92, 0x00};
//内置闹钟提醒
static const char nznztx[] = {0xE5, 0x86, 0x85, 0xE7, 0xBD, 0xAE, 0xE9, 0x97, 0xB9, 0xE9, 0x92, 0x9F, 0xE6, 0x8F, 0x90, 0xE9, 0x86, 0x92, 0x00};
//同步提醒
static const char tbtx[] = {0xE5, 0x90, 0x8C, 0xE6, 0xAD, 0xA5, 0xE6, 0x8F, 0x90, 0xE9, 0x86, 0x92, 0x00};
//播报提醒
static const char bbtx[] = {0xE6, 0x92, 0xAD, 0xE6, 0x8A, 0xA5, 0xE6, 0x8F, 0x90, 0xE9, 0x86, 0x92, 0x00};
//数量
static const char shuliang[] = {0xE6, 0x95, 0xB0, 0xE9, 0x87, 0x8F, 0x00};

static OS_MUTEX  play_list_op_mutex;
static OS_MUTEX  websockets_op_mutex;

static int dui_net_thread_pid;

struct list_head play_list_head = LIST_HEAD_INIT(play_list_head);
static int play_list_max, play_list_match; //从0开始

static u8 reopen_record;
static u8 alarm_ring_flag;
static u8 alarm_type;


extern media_item_t *get_media_item(void);
extern u8 get_s_is_record(void);
extern int get_app_music_volume(void);
extern int is_net_music_mode(void);
extern unsigned int random32(int type);

u8 get_alarm_type(void)
{
    return alarm_type;
}
void set_alarm_type(u8 value)
{
    alarm_type = value;
}

u8 get_alarm_ring_flag(void)
{
    return alarm_ring_flag;
}

void set_alarm_ring_flag(u8 value)
{
    alarm_ring_flag = value;
}


u8 get_reopen_record()
{
    return reopen_record;
}
void set_reopen_record(u8 value)
{
    reopen_record = value;
}

int get_play_list_match(void)
{
    return play_list_match;
}

void set_play_list_match(int value)
{
    play_list_match = value;
}

int get_play_list_max(void)
{
    return play_list_max;
}

void dui_delete_playlist(void)
{
    PLAY_LIST_INFO *pos, *n;
    os_mutex_pend(&play_list_op_mutex, 0);
    if (!list_empty(&play_list_head)) {
        list_for_each_entry_safe(pos, n, &play_list_head, entry) {
            list_del(&pos->entry);
            free(pos);
        }
    }
    os_mutex_post(&play_list_op_mutex);
}

void dui_add_playlist(PLAY_LIST_INFO *info)
{
    os_mutex_pend(&play_list_op_mutex, 0);
    list_add_tail(&info->entry, &play_list_head);
    os_mutex_post(&play_list_op_mutex);
}


void dui_play_playlist_for_match(media_item_t *info, int  match)
{
    PLAY_LIST_INFO *pos, *n;
    os_mutex_pend(&play_list_op_mutex, 0);
    if (list_empty(&play_list_head)) {
        goto exit;
    }
    list_for_each_entry_safe(pos, n, &play_list_head, entry) {
        if (pos->match == match) {
            printf("\n play_list_match = %d\n", match);
            info->linkUrl = pos->linkUrl;
            info->imageUrl = pos->imageUrl;
            info->title = pos->title;
            info->album = pos->album;
            info->subTitle = pos->subTitle;
            break;
        }
    }
exit:
    os_mutex_post(&play_list_op_mutex);
}

static int get_hmacsha1(const char *src_buf, const char *key, char *dst_buf)
{
    int ret;
    int i;
    u8 value;
    unsigned char digest[20];
    mbedtls_md_context_t sha_ctx;
    mbedtls_md_init(&sha_ctx);
    ret = mbedtls_md_setup(&sha_ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA1), 1);
    if (ret != 0) {
        printf("  ! mbedtls_md_setup() returned -0x%x\n", -ret);
        goto exit;
    }
    mbedtls_md_hmac_starts(&sha_ctx, (const unsigned char *)key, strlen(key));
    mbedtls_md_hmac_update(&sha_ctx, (const unsigned char *)src_buf, strlen(src_buf));
    mbedtls_md_hmac_finish(&sha_ctx, (unsigned char *)digest);

    for (i = 0; i < sizeof(digest) ; i++) {
        value = (digest[i] >> 4) & 0x0F ;
        dst_buf[i * 2] = value < 10 ? value + '0' : value - 10  + 'A';
        value = digest[i] & 0x0F;
        dst_buf[i * 2 + 1] = value < 10 ? value + '0' : value - 10 + 'A';
    }
    dst_buf[i * 2] = 0;
exit:
    mbedtls_md_free(&sha_ctx);
    return ret;
}
/*
 * 获取随机字符串
 */
static void get_rand_str(char strRand[], int iLen)
{
    int i = 0;
    char metachar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";

    memset(strRand, 0, iLen + 1);
    for (i = 0; i < iLen; i++) {
        *(strRand + i) = metachar[random32(0) % 62];
    }
}

static int get_dui_token_cb(u8 *buf, void *priv)
{
    /* printf("\n buf = %s\n",buf); */
    int ret = 0;
    struct dui_para *para = (struct dui_para *)priv;

    json_object *obj = NULL;
    json_object *key = NULL;
    obj = json_tokener_parse((const char *)buf);
    if (!obj) {
        return -1;
    }
    if (!json_object_object_get_ex(obj, "deviceSecret", &key)) {
        ret = -1;
        goto exit;
    }
    /* log_d("\n deviceSecret = %s\n", json_object_get_string(key)); */
    sprintf(para->reply.deviceSecret, "%s", json_object_get_string(key));

    para->hdl.connect_status = 1;

exit:
    json_object_put(obj);
    return ret;
}

int get_dui_token(struct dui_para *para)
{
    int ret = 0;
    http_body_obj http_recv_body = {0};
    char *http_send_body_begin = NULL, *user_head_buf = NULL;
    char timestamp[24] = {0};//以u64来计算ulinux毫秒时间戳
    char nonce[24] = {0};//随机数
    char sig[64] = {0};
    const char format[] = "plain";//默认值为plain
    char src_buf[256] = {0};

    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (!ctx) {
        return -1;
    }

    //dui鉴权
    time_t t = time(NULL);
    snprintf(timestamp, sizeof(timestamp), "%ld000", t);
    get_rand_str(nonce, 8);
    snprintf((char *)src_buf, 256, "%s%s%s%s%s",
             para->param.productKey,
             format,
             nonce,
             para->param.productId,
             timestamp);
    get_hmacsha1((const char *)src_buf, para->param.ProductSecret, sig);

    http_send_body_begin = malloc(256);
    if (!http_send_body_begin) {
        log_e("\n %s %d mallloc err\n", __func__, __LINE__);
        ret = -1;
        goto exit;
    }
    snprintf(http_send_body_begin, 256, USER_HTTP_BODY, para->param.deviceID);
    user_head_buf = malloc(512);
    if (!user_head_buf) {
        log_e("\n %s %d malloc fail \n", __func__, __LINE__);
        ret = -1;
        goto exit;
    }
    snprintf(user_head_buf, 512, USER_HTTP_HEADER,
             para->param.productKey,
             format,
             para->param.productId,
             timestamp,
             nonce,
             sig, (int)strlen(http_send_body_begin));
    //存放http回复
    http_recv_body.recv_len = 0;
    http_recv_body.buf_len = 2048;
    http_recv_body.buf_count = 1;
    http_recv_body.p = (char *)malloc(http_recv_body.buf_len * http_recv_body.buf_count);
    if (http_recv_body.p == NULL) {
        log_e("\n %s %d mallloc err\n", __func__, __LINE__);
        ret = -1;
        goto exit;
    }

    //body的传输方式
    int http_more_data_addr[1];
    int http_more_data_len[1 + 1];
    http_more_data_addr[0] = (int)(http_send_body_begin);
    http_more_data_len[0] = strlen(http_send_body_begin);
    http_more_data_len[1] = 0;

    ctx->more_data = http_more_data_addr;
    ctx->more_data_len = http_more_data_len;
    ctx->timeout_millsec = 5000;
    ctx->priv = &http_recv_body;
    ctx->user_http_header = user_head_buf;
    ctx->url = "https://auth.dui.ai/auth/device/register";//后面需要改成https
    ret = httpcli_post(ctx);
    if (ret) {
        goto exit;
    }
    ret = get_dui_token_cb((unsigned char *)http_recv_body.p, para);

exit:
    if (http_send_body_begin) {
        free(http_send_body_begin);
    }
    if (user_head_buf) {
        free(user_head_buf);
    }
    if (http_recv_body.p) {
        free(http_recv_body.p);
    }
    if (ctx) {
        free(ctx);
    }
    return ret;
}

static void websockets_callback(u8 *buf, u32 len, u8 type)
{
    PLAY_LIST_INFO *info;
    media_item_t item = {0};
    json_object *root_node = NULL;
    json_object *first_node, *second_node, *third_node, *fourth_node, *fifth_node;
    struct dui_var *dui_hdl = (struct dui_var *)get_dui_hdl();
    struct dui_para *para = &dui_hdl->para;
    u8 speekurl_exist = 0, linkUrl_exist = 0;

    if (get_record_break_flag()) {
        return;
    }

    printf("\n buf = %s", (const char *)buf);
    root_node = json_tokener_parse((const char *)buf);
    if (!root_node) {

        return;
    }

    if (json_object_object_get_ex(root_node, "sessionId", &first_node)) {

        strcpy(para->reply.sessionId, json_object_get_string(first_node));
    }

    if (json_object_object_get_ex(root_node, "pinyin", &first_node)) {
        printf("\n pinyin = %s\n", json_object_get_string(first_node));  //语音识别结果
#if CONFIG_CLOUD_VAD_ENABLE
        //云端返回识别结果,停止录音
        if (strcmp("", json_object_get_string(first_node)) && get_s_is_record()) {
            DUI_OS_TASKQ_POST("dui_app_task", 1, DUI_RECORD_STOP);
        }
#endif
    }

    if (json_object_object_get_ex(root_node, "speakUrl", &first_node)) {
        printf("\n speakUrl = %s\n", json_object_get_string(first_node));
        dui_media_speak_play(json_object_get_string(first_node));
        speekurl_exist = 1;
    }

    if (json_object_object_get_ex(root_node, "dm", &first_node)) {

        if (json_object_object_get_ex(first_node, "task", &second_node)) {
            strcpy(para->reply.task, json_object_get_string(second_node));
        }

        if (json_object_object_get_ex(first_node, "shouldEndSession", &second_node)) {

            if (json_object_get_boolean(second_node) == TRUE) {
                para->reply.sessionId[0] = 0;
                reopen_record = 0;
            } else {
                reopen_record = 1;
            }
        }

        if (json_object_object_get_ex(first_node, "widget", &second_node)) {

            if (json_object_object_get_ex(second_node, "content", &third_node)) {

                if (json_object_get_string(third_node) != NULL && json_object_array_length(third_node)) {

                    for (int i = 0; i < json_object_array_length(third_node) && i < PLAY_MAX_LIST; i++) {

                        fourth_node = json_object_array_get_idx(third_node, i);

                        if (json_object_object_get_ex(fourth_node, "linkUrl", &fifth_node)) {

                            info = calloc(1, sizeof(PLAY_LIST_INFO));
                            ASSERT(info != NULL, "%s %d calloc fail", __func__, __LINE__);
                            ASSERT(strlen(json_object_get_string(fifth_node)) <= sizeof(info->linkUrl), "%s %d", __func__, __LINE__);
                            strcpy((char *)info->linkUrl, json_object_get_string(fifth_node));
                            if (json_object_object_get_ex(fourth_node, "imageUrl", &fifth_node)) {
                                if (json_object_is_type(fifth_node, json_type_string)) {
                                    ASSERT(strlen(json_object_get_string(fifth_node)) <= sizeof(info->imageUrl), "%s %d", __func__, __LINE__);
                                    strcpy((char *)info->imageUrl, json_object_get_string(fifth_node));
                                }
                            }
                            if (json_object_object_get_ex(fourth_node, "title", &fifth_node)) {
                                if (json_object_is_type(fifth_node, json_type_string)) {
                                    ASSERT(strlen(json_object_get_string(fifth_node)) <= sizeof(info->title), "%s %d", __func__, __LINE__);
                                    strcpy((char *)info->title, json_object_get_string(fifth_node));
                                }
                            }
                            if (json_object_object_get_ex(fourth_node, "album", &fifth_node)) {
                                if (json_object_is_type(fifth_node, json_type_string)) {
                                    ASSERT(strlen(json_object_get_string(fifth_node)) <= sizeof(info->album), "%s %d", __func__, __LINE__);
                                    strcpy((char *)info->album, json_object_get_string(fifth_node));
                                }
                            }
                            if (json_object_object_get_ex(fourth_node, "subTitle", &fifth_node)) {
                                if (json_object_is_type(fifth_node, json_type_string)) {
                                    ASSERT(strlen(json_object_get_string(fifth_node)) <= sizeof(info->subTitle), "%s %d", __func__, __LINE__);
                                    strcpy((char *)info->subTitle, json_object_get_string(fifth_node));
                                }
                            }
                            if (i == 0) {
                                //清空播放列表
                                linkUrl_exist = 1;
                                play_list_max = 0;
                                dui_delete_playlist();
                            }
                            play_list_max++;
                            info->match = i;
                            dui_add_playlist(info);
                        } else {
                            printf("\n no linkUrl break\n");
                            break;
                        }
                    }
                    if (linkUrl_exist) {
                        dui_play_playlist_for_match(&item, 0);
                        if (item.linkUrl) {
                            play_list_match = 0;
                            printf("\n play_list_max = %d\n", play_list_max);
                            dui_media_set_source(MEDIA_SOURCE_AI);
                            strcpy(para->reply.url, item.linkUrl);
                        }
                    }
                    /*如果不存在提示音，但是存在媒体音*/
                    if (!speekurl_exist && linkUrl_exist && item.linkUrl) {
                        dui_ai_media_audio_play(item.linkUrl);
                    }
                }
            }
        }

        if (json_object_object_get_ex(first_node, "command", &second_node)) {

            if (json_object_object_get_ex(second_node, "api", &third_node)) {

                printf("\n[1] api = %s\n", json_object_get_string(third_node));
                if (!strncmp(json_object_get_string(third_node), "DUI.MediaController", strlen("DUI.MediaController"))) {

                    if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.Prev")) {
                        dui_media_audio_prev_play();
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.Next") || !strcmp(json_object_get_string(third_node), "DUI.MediaController.Switch")) {
                        dui_media_audio_next_play();
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.Pause")) {
                        dui_media_audio_pause_play(NULL);
                        iot_ctl_play_pause();
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.Play")) {
                        dui_media_audio_continue_play(NULL);
                        iot_ctl_play_resume();
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.SetVolume")) {
                        if (json_object_object_get_ex(second_node, "param", &third_node)) {
                            int volume;
                            if (json_object_object_get_ex(third_node, "volume", &fourth_node)) {
                                printf("\nvolume = %s\n", json_object_get_string(fourth_node));
                                if (!strcmp(json_object_get_string(fourth_node), "+")) {
                                    volume = get_app_music_volume() + 5;
                                } else if (!strcmp(json_object_get_string(fourth_node), "-")) {
                                    volume = get_app_music_volume() - 5;
                                } else if (!strcmp(json_object_get_string(fourth_node), "max")) {
                                    volume = 100;
                                } else if (!strcmp(json_object_get_string(fourth_node), "min")) {
                                    volume = 0;
                                } else {
                                    volume = atoi(json_object_get_string(fourth_node));
                                }
                                if (volume < 0 || volume > 100) {
                                    volume = get_app_music_volume();
                                }

                                dui_volume_change_notify(volume);
                                dui_media_audio_resume_play();
                            }
                        }
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.SetPlayMode")) {

                        if (json_object_object_get_ex(second_node, "param", &third_node)) {

                            if (json_object_object_get_ex(third_node, "mode", &fourth_node)) {
                                printf("\n mode = %s\n", json_object_get_string(fourth_node));
                                int mode = ORDERPLAY_MODE;
                                if (!strcmp(json_object_get_string(fourth_node), "orderPlay")) {
                                    mode = ORDERPLAY_MODE;
                                } else if (!strcmp(json_object_get_string(fourth_node), "loopPlay")) {
                                    mode = LOOPPLAY_MODE;
                                } else if (!strcmp(json_object_get_string(fourth_node), "singleLoop")) {
                                    mode = SINGLELOOP_MODE;
                                } else if (!strcmp(json_object_get_string(fourth_node), "randomPlay")) {
                                    mode = RANDOMPLAY_MODE;
                                }
                                dui_media_music_mode_set(mode);
                                dui_media_audio_resume_play();
                            }
                        }
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.Forward")) {

                        int hour = 0, min = 0, sec = 10;
                        if (json_object_object_get_ex(second_node, "param", &third_node)) {

                            if (json_object_object_get_ex(third_node, "relativeTime", &fourth_node)) {
                                printf("\n time = %s\n", json_object_get_string(fourth_node));
                                sscanf(json_object_get_string(fourth_node), "%d:%d:%d", &hour, &min, &sec);
                                printf("\nhour = %d min = %d sec = %d\n", hour, min, sec);
                            }
                            dui_media_audio_play_seek(hour * 60 * 60 + min * 60 + sec);
                        }
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.Backward")) {
                        int hour = 0, min = 0, sec = 10;
                        if (json_object_object_get_ex(second_node, "param", &third_node)) {
                            if (json_object_object_get_ex(third_node, "relativeTime", &fourth_node)) {
                                printf("\n time = %s\n", json_object_get_string(fourth_node));
                                sscanf(json_object_get_string(fourth_node), "%d:%d:%d", &hour, &min, &sec);
                                printf("\nhour = %d min = %d sec = %d\n", hour, min, sec);
                            }
                            dui_media_audio_play_seek(-(hour * 60 * 60 + min * 60 + sec));
                        }

                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.AddCollectionList")) {

                        if (is_net_music_mode()) {
                            media_item_t *item = get_media_item();
                            if (item->linkUrl) {
                                iot_ctl_box_like(item);
                            }
                        } else {
                            dui_event_notify(AI_SERVER_EVENT_PLAY_BEEP, "not_support.mp3");
                        }
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.RemoveCollectionList")) {
                        if (is_net_music_mode()) {
                            media_item_t *gitem = get_media_item();
                            if (gitem->linkUrl) {
                                iot_ctl_box_like(gitem);
                            }
                        } else {
                            dui_event_notify(AI_SERVER_EVENT_PLAY_BEEP, "not_support.mp3");

                        }

                    } else if (!strcmp(json_object_get_string(third_node), "DUI.MediaController.PlayCollectionList")) {
                        if (is_net_music_mode()) {
                            media_item_t *gitem = get_media_item();
                            if (gitem->linkUrl) {
                                iot_ctl_box_like(gitem);
                            }
                        } else {
                            dui_event_notify(AI_SERVER_EVENT_PLAY_BEEP, "not_support.mp3");
                        }
                    }
                } else if (!strncmp(json_object_get_string(third_node), "ai.dui.dskdm.reminder", strlen("ai.dui.dskdm.reminder"))) {

                    if (!strcmp(json_object_get_string(third_node), "ai.dui.dskdm.reminder.sync")) {

                        if (json_object_object_get_ex(second_node, "param", &third_node)) {
                            if (json_object_object_get_ex(third_node, "extra", &fourth_node)) {
                                dui_alarm_sync(json_object_get_string(fourth_node));
                            }
                        }
                    } else if (!strcmp(json_object_get_string(third_node), "ai.dui.dskdm.reminder.query")) {

                    } else if (!strcmp(json_object_get_string(third_node), "ai.dui.dskdm.reminder.insert")) {
                        if (json_object_object_get_ex(second_node, "param", &third_node)) {
                            if (json_object_object_get_ex(third_node, "extra", &fourth_node)) {
                                int dui_alarm_add(const char *extra);
                                dui_alarm_add(json_object_get_string(fourth_node));
                            }
                        }
                        DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_ALARM_SYNC);
                    } else if (!strcmp(json_object_get_string(third_node), "ai.dui.dskdm.reminder.remove")) {
                        if (json_object_object_get_ex(second_node, "param", &third_node)) {
                            if (json_object_object_get_ex(third_node, "extra", &fourth_node)) {
                                int dui_alarm_del(const char *extra);
                                dui_alarm_del(json_object_get_string(fourth_node));
                            }
                        }
                        DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_ALARM_SYNC);
                    } else if (!strcmp(json_object_get_string(third_node), "ai.dui.dskdm.reminder.ring")) {
                        set_alarm_ring_flag(1);
                    }
                } else if (!strncmp(json_object_get_string(third_node), "DUI.System", strlen("DUI.System"))) {

                    extern void dui_shutdown(void *priv);
                    if (!strcmp(json_object_get_string(third_node), "DUI.System.Shutdown")) {
                        if (json_object_object_get_ex(second_node, "param", &third_node)) {
                            if (json_object_object_get_ex(third_node, "relative", &fourth_node)) {
                                int hour, min, sec;
                                sscanf(json_object_get_string(fourth_node), "%d:%d:%d", &hour, &min, &sec);
                                printf("\nhour = %d min = %d sec = %d\n", hour, min, sec);
                                u32 time = hour * 3600 + min * 60 + sec;
                                sys_timeout_add_to_task("sys_timer", NULL, dui_shutdown, time * 1000);
                            }
                        } else {
                            dui_shutdown(NULL);//如果直接在这里执行，将不会播放speakurl
                        }
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.System.Exit")) {

                    } else if (!strcmp(json_object_get_string(third_node), "DUI.System.Cancel")) {

                    } else if (!strcmp(json_object_get_string(third_node), "DUI.System.Connectivity.OpenBluetooth")) {
                        dui_event_notify(AI_SERVER_EVENT_BT_OPEN, NULL);
                    } else if (!strcmp(json_object_get_string(third_node), "DUI.System.Connectivity.CloseBluetooth")) {
                        dui_event_notify(AI_SERVER_EVENT_BT_CLOSE, NULL);
                    }
                } else if (!speekurl_exist && !item.linkUrl && !reopen_record) {
                    printf("not find api\n");
                    dui_event_notify(AI_SERVER_EVENT_PLAY_BEEP, "not_support.mp3");
                }
            }
        }

        if (json_object_object_get_ex(first_node, "api", &second_node)) {
            printf("\n[2] api = %s\n", json_object_get_string(second_node));
            if (!strcmp(json_object_get_string(second_node), "audio.info.search")) {
                set_reopen_record(0);
                DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_INFO_SEARCH);
            }
        }

        if (json_object_object_get_ex(root_node, "error", &first_node)) {
            if (json_object_object_get_ex(first_node, "errMsg", &second_node)) {
                printf("\nerrMsg = %s\n", json_object_get_string(second_node));
            }
            if (json_object_object_get_ex(first_node, "errId", &second_node)) {
                printf("\nerrId = %s\n", json_object_get_string(second_node));
                if (!strcmp(json_object_get_string(second_node), "010403")) {
                    reopen_record = 0;
                    para->reply.url[0] = 0;
                    dui_delete_playlist();
                    dui_media_audio_stop_play(NULL);
                } else if (!speekurl_exist && !item.linkUrl && !reopen_record) {
                    dui_media_audio_resume_play();
                }
            }
        }
        printf("\n reopen_record = %d\n", reopen_record);
        if (!speekurl_exist && reopen_record && !get_s_is_record()) {
            DUI_OS_TASKQ_POST("dui_app_task", 1, DUI_RECORD_START);
        }
    }
    json_object_put(root_node);
}

static void websockets_disconnect(struct websocket_struct *websockets_info);

static void websockets_client_reg(struct websocket_struct *websockets_info, char mode)
{
    memset(websockets_info, 0, sizeof(struct websocket_struct));
    websockets_info->_init           = websockets_client_socket_init;
    websockets_info->_exit           = websockets_client_socket_exit;
    websockets_info->_handshack      = webcockets_client_socket_handshack;
    websockets_info->_send           = websockets_client_socket_send;
    websockets_info->_recv_thread    = websockets_client_socket_recv_thread;
    websockets_info->_heart_thread   = websockets_client_socket_heart_thread;
    websockets_info->_recv_cb        = websockets_callback;
    websockets_info->_recv           = NULL;
    websockets_info->websocket_mode  = mode;
}

static int websockets_client_init(struct websocket_struct *websockets_info, u8 *url, const char *origin_str)
{
    websockets_info->ip_or_url = url;
    websockets_info->origin_str = origin_str;
    websockets_info->recv_time_out = 1000;
    return websockets_info->_init(websockets_info);
}

static int websockets_client_handshack(struct websocket_struct *websockets_info)
{
    return websockets_info->_handshack(websockets_info);
}

static int websockets_client_send(struct websocket_struct *websockets_info, u8 *buf, int len, char type)
{
    int ret  = -1;
    os_mutex_pend(&websockets_op_mutex, 0);
    if (websockets_info->websocket_valid == ESTABLISHED) {
        //SSL加密时一次发送数据不能超过16K，用户需要自己分包
        ret = websockets_info->_send(websockets_info, buf, len, type);
    }
    os_mutex_post(&websockets_op_mutex);
    if (ret <= 0) {
        websockets_disconnect(websockets_info);
    }
    return ret;
}

static void websockets_client_exit(struct websocket_struct *websockets_info)
{
    websockets_info->_exit(websockets_info);
}
static int  websockets_connect(struct dui_para *para)
{
    int ret;
    char mode = WEBSOCKETS_MODE;
    char timestamp[24];//以u64来计算ulinux毫秒时间戳
    char nonce[24];//随机数
    const char format[] = "plain";//默认值为plain
    char sig[64] = {0};
    char src_buf[256] = {0};
    static time_t t;
    struct websocket_struct *websockets_info = &para->hdl.websockets_info;
    char *url = NULL;
    get_rand_str(nonce, 8);
    os_mutex_pend(&websockets_op_mutex, 0);
    if (websockets_info->websocket_valid == ESTABLISHED) {
        ret = -1;
        goto exit;
    }
    if (websockets_info->websocket_valid == INVALID_ESTABLISHED) {
        websockets_disconnect(websockets_info);
    }
    url = calloc(1, 1460);
    if (!url) {
        ret = -1;
        goto exit;
    }

    t = time(NULL);
    snprintf(timestamp, sizeof(timestamp), "%ld000", t);
    snprintf((char *)src_buf, 256, "%s%s%s%s",
             para->param.deviceID,
             nonce,
             para->param.productId,
             timestamp);

    get_hmacsha1((char *)src_buf, para->reply.deviceSecret, sig);
    snprintf((char *)url, 1460, "wss://dds.dui.ai/dds/v1/prod?"\
             "serviceType=%s&"\
             "productId=%s"\
             "&deviceName=%s"\
             "&nonce=%s"\
             "&sig=%s"\
             "&timestamp=%s"\
             ,
             "websocket",
             para->param.productId,
             para->param.deviceID,
             nonce,
             sig,
             timestamp);
    /* 1 . register */
    websockets_client_reg(websockets_info, mode);
    /* 2 . init */
    ret = websockets_client_init(websockets_info, (unsigned char *)url, NULL);
    if (FALSE == ret) {
        printf("  . ! Cilent websocket init error !!!\r\n");
        ret = -1;
        goto exit;
    }
    /* 3 . hanshack */
    ret = websockets_client_handshack(websockets_info);
    if (FALSE == ret) {
        ret = -1;
        printf("  . ! Handshake error !!!\r\n");
        goto exit;
    }
    printf(" . Handshake success \r\n");
    /* 4 . CreateThread */
    snprintf((char *)src_buf, sizeof(src_buf), "websocket_client_heart_%d", (u16)(random32(0) & 0xFFFF));
    thread_fork((const char *)src_buf, 19, 1024, 0,
                &websockets_info->ping_thread_id,
                websockets_info->_heart_thread,
                websockets_info);
    snprintf((char *)src_buf, sizeof(src_buf), "websocket_client_recv_%d", (u16)(random32(0) & 0xFFFF));
    thread_fork((const char *)src_buf, 18, 1024, 0,
                &websockets_info->recv_thread_id,
                websockets_info->_recv_thread,
                websockets_info);
    ret = 0;
exit:
    if (url) {
        free(url);
    }
    os_mutex_post(&websockets_op_mutex);
    return ret;
}

static void websockets_disconnect(struct websocket_struct *websockets_info)
{
    int ret;

    if (websockets_info->websocket_valid == NOT_ESTABLISHED) {
        return;
    }
retry_again:
    puts("\nwebsockets_disconnect enter\n");
    ret = os_mutex_pend(&websockets_op_mutex, 10);
    if (ret == OS_NO_ERR) {
        if (websockets_info->websocket_valid != NOT_ESTABLISHED) {
            /* 6 . exit */
            puts("[1]");
            websockets_info->websocket_valid = INVALID_ESTABLISHED;
            if (websockets_info->recv_thread_id) {
                thread_kill(&websockets_info->recv_thread_id, KILL_WAIT);
            }
            puts("[2]");
            if (websockets_info->ping_thread_id) {
                thread_kill(&websockets_info->ping_thread_id, KILL_WAIT);
            }
            puts("[3]");
            websockets_client_exit(websockets_info);
            websockets_info->websocket_valid = NOT_ESTABLISHED;
            puts("[4]");
        }
        os_mutex_post(&websockets_op_mutex);
    } else {
        if (websockets_info->websocket_valid != NOT_ESTABLISHED) {
            log_e("\n %s %d\n", __func__, __LINE__);
            goto retry_again;
        }
    }
    puts("\nwebsockets_disconnect exit\n");
}

static struct json_object *create_cjson(struct dui_para *para)
{
    json_object *root = json_object_new_object();
    json_object *new_node;
    if (para->type == TEXT_REQUEST) {
        json_object_object_add(root, "topic", json_object_new_string("nlu.input.text"));
        json_object_object_add(root, "refText", json_object_new_string(para->param.text));
        if (para->reply.sessionId[0] != 0) {
            json_object_object_add(root, "sessionId", json_object_new_string(para->reply.sessionId));
        }
    } else if (para->type == AUDIO_REQUEST) {
        json_object *data = json_object_new_object();
        json_object_object_add(root, "topic", json_object_new_string("recorder.stream.start"));
        json_object_object_add(root, "audio", data);
        /* json_object_object_add(data, "audioType", json_object_new_string("amr"));//只支持8000 */
        json_object_object_add(data, "audioType", json_object_new_string("opus"));
        json_object_object_add(data, "sampleRate", json_object_new_int(16000));
        json_object_object_add(data, "channel", json_object_new_int(1));
        json_object_object_add(data, "sampleBytes", json_object_new_int(2));

#if CONFIG_CLOUD_VAD_ENABLE
        json_object *asrParams = json_object_new_object();
        json_object_object_add(root, "asrParams", asrParams);
        json_object_object_add(asrParams, "enableCloudVAD", json_object_new_boolean(1));
#endif

        if (para->reply.task[0] != 0) {
            json_object *context = json_object_new_object();
            json_object_object_add(root, "context", context);
            json_object *skill = json_object_new_object();
            json_object_object_add(context, "skill", skill);
            json_object_object_add(skill, "task", json_object_new_string(para->reply.task));
        }

        if (para->reply.sessionId[0] != 0) {
            json_object_object_add(root, "sessionId", json_object_new_string(para->reply.sessionId));
        }

    } else if (para->type == INTENT_REQUEST) {
        if (para->intent == ALARM_SYNC) {
            json_object_object_add(root, "task", json_object_new_string(tixing));
            json_object_object_add(root, "topic", json_object_new_string("dm.input.intent"));
            json_object_object_add(root, "skill", json_object_new_string(nznztx));
            json_object_object_add(root, "intent", json_object_new_string(tbtx));
            new_node = json_object_new_object();
            json_object_object_add(root, "slots", new_node);
            json_object_object_add(new_node, shuliang, json_object_new_int(10));
        } else if (para->intent == ALARM_RING) {
            json_object_object_add(root, "task", json_object_new_string(tixing));
            json_object_object_add(root, "topic", json_object_new_string("dm.input.intent"));
            json_object_object_add(root, "skill", json_object_new_string(nznztx));
            json_object_object_add(root, "intent", json_object_new_string(bbtx));
            new_node = json_tokener_parse(para->param.slots);
            json_object_object_add(root, "slots", new_node);
        } else if (para->intent == ALARM_OPERATE) {
            json_object_object_add(root, "task", json_object_new_string(tixing));
            json_object_object_add(root, "topic", json_object_new_string("dm.input.intent"));
            json_object_object_add(root, "skill", json_object_new_string(nznztx));
            json_object_object_add(root, "intent", json_object_new_string(para->param.intent));
            new_node = json_tokener_parse(para->param.slots);
            json_object_object_add(root, "slots", new_node);

        }
        if (para->reply.sessionId[0] != 0) {
            json_object_object_add(root, "sessionId", json_object_new_string(para->reply.sessionId));
        }
    } else if (para->type == SPEECH_RECOGNITION) {
        json_object *data = json_object_new_object();
        json_object_object_add(root, "topic", json_object_new_string("recorder.stream.start"));
        json_object_object_add(root, "audio", data);
        json_object_object_add(data, "audioType", json_object_new_string("opus"));
        json_object_object_add(data, "sampleRate", json_object_new_int(16000));
        json_object_object_add(data, "channel", json_object_new_int(1));
        json_object_object_add(data, "sampleBytes", json_object_new_int(2));

        json_object_object_add(root, "aiType",  json_object_new_string("asr"));
        if (para->reply.sessionId[0] != 0) {
            json_object_object_add(root, "sessionId", json_object_new_string(para->reply.sessionId));
        }

    } else if (para->type == CUSTOM_REQUEST) {
        json_object_object_add(root, "topic", json_object_new_string("dm.input.data"));
        json_object_object_add(root, "duiWidget", json_object_new_string("custom"));
        new_node = json_tokener_parse(para->param.extra);
        json_object_object_add(root, "extra", new_node);
        if (para->reply.sessionId[0] != 0) {
            json_object_object_add(root, "sessionId", json_object_new_string(para->reply.sessionId));
        }

    } else if (para->type == SKILL_SETTING) {
        json_object_object_add(root, "topic", json_object_new_string("skill.settings"));
        json_object_object_add(root, "skillId", json_object_new_string("2019042500000544"));//技能ID
        json_object_object_add(root, "option", json_object_new_string("set"));

        json_object *array = json_object_new_array();

        json_object_object_add(root, "settings", array);

        json_object *array0 = json_object_new_object();
        json_object_object_add(array0, "key", json_object_new_string("city"));
        json_object_object_add(array0,   "value", json_object_new_string("zhuhai"));
        json_object_array_add(array, array0);
        json_object *array1 = json_object_new_object();
        json_object_object_add(array1, "key", json_object_new_string("city"));
        json_object_object_add(array1,   "value", json_object_new_string("shenzhen"));
        json_object_array_add(array, array1);
    } else if (para->type == SYSTEM_SETTING) {
        json_object_object_add(root, "topic", json_object_new_string("system.settings"));
        json_object_object_add(root, "option", json_object_new_string("set"));

        json_object *array = json_object_new_array();
        json_object_object_add(root, "settings", array);
        json_object *array0 = json_object_new_object();
        json_object_object_add(array0, "key", json_object_new_string("city"));
        json_object_object_add(array0,   "value", json_object_new_string("zhuhai"));
        json_object_array_add(array, array0);
        json_object *array1 = json_object_new_object();
        json_object_object_add(array1, "key", json_object_new_string("city"));
        json_object_object_add(array1,   "value", json_object_new_string("shenzhen"));
        json_object_array_add(array, array1);
    }

    return root;
}



static void dui_net_task(void *priv)
{
    struct dui_para *para = (struct dui_para *)priv;
    struct websocket_struct *websockets_info = &para->hdl.websockets_info;
    DUI_AUDIO_INFO *info = NULL;
    json_object *root = NULL;
    int err;
    int msg[32];

    dui_alarm_init();
    DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_ALARM_SYNC);
    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }
        switch (msg[1]) {
        case DUI_RECORD_START_MSG:
            websockets_connect(para);
            para->type = AUDIO_REQUEST;
            root = create_cjson(para);
            err = websockets_client_send(websockets_info, (u8 *)json_object_get_string(root), strlen(json_object_get_string(root)), WCT_TXTDATA);
            json_object_put(root);
            if (err  <= 0 && get_s_is_record()) {
                DUI_OS_TASKQ_POST("dui_app_task", 1, DUI_RECORD_STOP);
            }
            break;
        case DUI_RECORD_SEND_MSG: //录音消息直接发给这个任务，不经过dui_app_task
            info = get_dui_audio_info();
            if (info) {
                if (info->sessionid == get_record_sessionid()) {
                    err  =  websockets_client_send(websockets_info, info->buf, info->len, WCT_BINDATA);
                }
                put_dui_audio_info(info);
            }
            if (err  <= 0 && get_s_is_record()) {
                DUI_OS_TASKQ_POST("dui_app_task", 1, DUI_RECORD_STOP);
            }
            break;
        case DUI_RECORD_STOP_MSG:
            /*这里必须要执行send，否则会导致内存泄漏*/
            do {
                info = get_dui_audio_info();
                if (info) {
                    if (info->sessionid == get_record_sessionid()) {
                        websockets_client_send(websockets_info, info->buf, info->len, WCT_BINDATA);
                    }
                    put_dui_audio_info(info);
                }
            } while (info);
            websockets_client_send(websockets_info, 0, 0, WCT_BINDATA);

            break;
        case DUI_ALARM_SYNC:
            //发一条查询提醒闹钟的消息
            websockets_connect(para);
            para->type = INTENT_REQUEST;
            para->intent = ALARM_SYNC;
            root = create_cjson(para);
            websockets_client_send(websockets_info, (u8 *)json_object_get_string(root), strlen(json_object_get_string(root)), WCT_TXTDATA);
            json_object_put(root);
            break;
        case DUI_ALARM_RING:
            if (msg[2]) {
                printf("\n>>>>>>>>>>>>>>>vid = %s\n", (char *)msg[2]);
                set_record_break_flag(0);
                websockets_connect(para);
                para->type = INTENT_REQUEST;
                para->intent = ALARM_RING;
                para->param.slots = (char *)msg[2];
                set_alarm_type(msg[3]);
                /* put_buf(__this->para.param.object, strlen(__this->para.param.object)); */
                root = create_cjson(para);
                websockets_client_send(websockets_info, (u8 *)json_object_get_string(root), strlen(json_object_get_string(root)), WCT_TXTDATA);
                json_object_put(root);
                free((void *)msg[2]);
            }
            err = dui_alarm_query();
            if (err) {
                extern void set_alarm_notify_flag(u8 value);
                set_alarm_notify_flag(1);
                DUI_OS_TASKQ_POST("dui_net_task", 1, DUI_ALARM_SYNC);
            }
            break;

        case DUI_ALARM_OPERATE:
            if (msg[2]) {
                printf("\n>>>>>>>>>>>>>>>msg = %s\n", (char *)msg[2]);
                websockets_connect(para);
                para->param.intent = (char *)msg[2];
                para->param.slots = (char *)msg[3];
                para->type = INTENT_REQUEST;
                para->intent = ALARM_OPERATE;
                root = create_cjson(para);
                printf("\n json_object_get_string(root) = %s\n", json_object_get_string(root));
                websockets_client_send(websockets_info, (u8 *)json_object_get_string(root), strlen(json_object_get_string(root)), WCT_TXTDATA);
                json_object_put(root);
                free((void *)msg[2]);
                free((void *)msg[3]);
            }

            break;
        case DUI_INFO_SEARCH:

            websockets_connect(para);
            para->param.intent = (char *)msg[2];
            para->param.slots = (char *)msg[3];
            para->type = CUSTOM_REQUEST;
            para->param.extra = malloc(1024);
            if (!para->param.extra) {
                break;
            }
            media_item_t *item = get_media_item();
            snprintf(para->param.extra, 1024, "{\"title\":\"%s\",\"album\":\"%s\",\"subTitle\":\"%s\",\"errId\":\"0\"}",
                     item->title ? item->title : "",
                     item->album ? item->album : "",
                     item->subTitle ? item->subTitle : "");
            root = create_cjson(para);
            printf("\n json_object_get_string(root) = %s\n", json_object_get_string(root));
            websockets_client_send(websockets_info, (u8 *)json_object_get_string(root), strlen(json_object_get_string(root)), WCT_TXTDATA);
            json_object_put(root);
            free(para->param.extra);
            break;
        case DUI_QUIT_MSG:
            websockets_disconnect(websockets_info);
            goto exit;
            break;
        default:
            break;
        }
    }
exit:
    printf("\n %s  exit \n", __func__);
}

int dui_net_thread_run(void *priv)
{
    if (!os_mutex_valid(&play_list_op_mutex)) {
        os_mutex_create(&play_list_op_mutex);
        os_mutex_create(&websockets_op_mutex);
    }
    return thread_fork("dui_net_task", 15, 1536, 256, &dui_net_thread_pid, dui_net_task, priv);
}

void dui_net_thread_kill(void)
{
    thread_kill(&dui_net_thread_pid, 0);
}

