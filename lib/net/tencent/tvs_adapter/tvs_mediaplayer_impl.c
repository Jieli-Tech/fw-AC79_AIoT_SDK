#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_api/tvs_api.h"
#include "tvs_mediaplayer_impl.h"
#include "tvs_api_config.h"
#include "tvs_api_config.h"

#include "os/os_api.h"
#include "server/audio_server.h"
#include "server/server_core.h"
#include "server/ai_server.h"
#include "network_download/net_audio_buf.h"


#define AUDIO_PLAYER_DBG 1

#define LOG(flags, fmt, arg...)	\
	do {								\
		if (flags) 						\
			TVS_ADAPTER_PRINTF(fmt, ##arg);		\
	} while (0)

#define AUDIO_PLAYER_DEBUG(fmt, arg...)	\
			LOG(AUDIO_PLAYER_DBG, "[tvs_audio] "fmt, ##arg)

typedef enum {
    PLAYER_URL_PLAYING = 4001,
    PLAYER_URL_PAUSED,
    PLAYER_URL_FINISHED,
    PLAYER_URL_STOPPED,
    PLAYER_URL_SEEKING,
    PLAYER_URL_ERROR,
    PLAYER_URL_UNSUPPORTED_FORMAT,
    PLAYER_URL_RESUME,
} PLAYER_STATUS_E;

// 网络流媒体Url
static char *g_player_url = NULL;
// 网络流媒体唯一标识
static char *g_player_token = NULL;
// 下发的seek进度
static uint32_t g_player_offset_sec = 0;
//新url标志位
static u8 new_url_flag = 0;

//yii:引用tencent平台api
extern const struct ai_sdk_api tc_tvs_api;
extern u8 alarm_rings;
static int g_total_tts_size = 0;
static bool g_tts_play_start = false;

#define DEC_CBUF_SIZE (1024 * 10)

static struct tvs_dec_t {
    void *dec_server;
    OS_SEM end_sem;
    void *net_buf;
    int state;
} dec_hdl;


__attribute__((weak)) int get_app_music_prompt_dec_status(void)
{
    return 0;
}

__attribute__((weak)) int get_app_music_playtime(void)
{
    return 0;
}

__attribute__((weak)) int get_app_music_playtime1(void)
{
    return 0;
}

__attribute__((weak)) int get_app_music_total_time(void)
{
    return 0;
}

__attribute__((weak)) int get_app_music_volume(void)
{
    return 100;
}

__attribute__((weak)) int get_listen_flag()
{
    return 0;
}

static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        puts("tvs_tts: AUDIO_SERVER_EVENT_ERR\n");
    case AUDIO_SERVER_EVENT_END:
        puts("tvs_tts: AUDIO_SERVER_EVENT_END\n");
        os_sem_post(&((struct tvs_dec_t *)priv)->end_sem);
        break;
    default:
        break;
    }
}

static int tts_vfs_flen(void *priv)
{
    return -1;
}

static int tts_vfs_fread(void *file, void *data, u32 len)
{
    int rlen = net_buf_read(data, len, file);
    if (rlen == -2) {
        return rlen;	//暂时读不到数
    } else if (rlen < 0) {
        return 0;
    }
    return rlen;
}

static int tts_vfs_fseek(void *priv, u32 offset, int orig)
{
    int ret = net_buf_seek(offset, orig, priv);

    if (ret == -2) {	//暂时读不到数
        return ret;
    }
    return 0;
}

static int tts_vfs_fclose(void *file)
{
    return 0;
}


static const struct audio_vfs_ops tts_vfs_ops = {
    .fread  = tts_vfs_fread,
    .fclose = tts_vfs_fclose,
    .fseek  = tts_vfs_fseek,
    .flen   = tts_vfs_flen,
};

static int tvs_api_impl_media_player_init()
{
    return 0;
}

static void do_release_music()
{
    if (g_player_url != NULL) {
        TVS_FREE(g_player_url);
        g_player_url = NULL;
    }

    if (g_player_token != NULL) {
        TVS_FREE(g_player_token);
        g_player_token = NULL;
    }
}

static int mediaplayer_adapter_set_source(const char *url, const char *token, uint32_t offset_sec)
{
    if (NULL == url || NULL == token) {
        return -1;
    }
    if (!g_player_url || (g_player_url && strcmp(url, g_player_url))) {	//如果是新的url,则标志位置1
        new_url_flag = 1;
    }

    // 后台下发了新的媒体
    do_release_music();

    g_player_url = strdup(url);
    g_player_token = strdup(token);
    g_player_offset_sec = offset_sec;

    AUDIO_PLAYER_DEBUG("set new music %s, token %s, seek %d seconds\n", g_player_url == NULL ? "null" : g_player_url,
                       g_player_token == NULL ? "null" : g_player_token, g_player_offset_sec);		//yii:歌曲格式不支持快进快退,所以这里的offset没用

    return 0;
}

static int mediaplayer_adapter_start_play(const char *token)
{
    if (g_player_url == NULL) {
        return 0;
    }
    // 开始播放后台下发的媒体
    AUDIO_PLAYER_DEBUG("start play music %s, token %s, seek %d seconds\n", g_player_url == NULL ? "null" : g_player_url,
                       g_player_token == NULL ? "null" : g_player_token, g_player_offset_sec);

    //调用播放器播放流媒体
    if (!new_url_flag && g_player_offset_sec) {
        ai_server_event_url(&tc_tvs_api, NULL, AI_SERVER_EVENT_RESUME_PLAY);	//yii:若有offset(只有同一个歌曲才有offset) 则继续播放
    } else {
        new_url_flag = 0;
        ai_server_event_url(&tc_tvs_api, g_player_url, AI_SERVER_EVENT_URL);
    }


    //网络流媒体开始播放的时候，接入方需要主动调用
    tvs_mediaplayer_adapter_on_play_started(token);

    return 0;
}

static int mediaplayer_adapter_stop_play(const char *token)
{
    if (g_player_url == NULL) {
        return 0;
    }

    // 后台下发了结束播放命令
    AUDIO_PLAYER_DEBUG("stop play music %s, token %s\n", g_player_url == NULL ? "null" : g_player_url,
                       g_player_token == NULL ? "null" : g_player_token);

    //调用播放器停止播放流媒体
    ai_server_event_url(&tc_tvs_api, NULL, AI_SERVER_EVENT_PAUSE);

    //网络流媒体停止或者播放出错的时候，接入方需要主动调用
    tvs_mediaplayer_adapter_on_play_stopped(0, token);
    return 0;
}

static int mediaplayer_adapter_resume_play(const char *token)
{
    if (g_player_url == NULL) {
        return 0;
    }
    AUDIO_PLAYER_DEBUG("resume play music %s, token %s\n", g_player_url == NULL ? "null" : g_player_url,
                       g_player_token == NULL ? "null" : g_player_token);

    //调用播放器继续播放流媒体
    ai_server_event_url(&tc_tvs_api, NULL, AI_SERVER_EVENT_RESUME_PLAY);

    //网络流媒体开始播放的时候，接入方需要主动调用
    tvs_mediaplayer_adapter_on_play_started(token);

    return 0;
}

static int mediaplayer_adapter_pause_play(const char *token)
{
    if (g_player_url == NULL) {
        return 0;
    }
    AUDIO_PLAYER_DEBUG("pause play music %s, token %s\n", g_player_url == NULL ? "null" : g_player_url,
                       g_player_token == NULL ? "null" : g_player_token);

    ai_server_event_url(&tc_tvs_api, NULL, AI_SERVER_EVENT_PAUSE);


    //网络流媒体暂停播放的时候，接入方需要主动调用
    tvs_mediaplayer_adapter_on_play_paused(token);


    return 0;
}

bool mediaplayer_adapter_is_player_valid()
{
    return true;
}

char *mediaplayer_adapter_get_url_token()
{
    return g_player_token;
}

static int mediaplayer_adapter_get_offset(const char *token)
{
    //获取网络流媒体的播放进度
    int offset = get_app_music_playtime1();
    printf("------------%s------------%d    offset = %d", __func__, __LINE__, offset);
    return offset;
}












/*********************************播放mp3格式音频****************************/

int tts_volume_change(void)
{
    if (!g_tts_play_start) {
        return -1;
    }
    int err = -1;
    union audio_req req = {0};
    req.dec.cmd		= AUDIO_DEC_SET_VOLUME;
    req.dec.volume	= get_app_music_volume();

    err = server_request(dec_hdl.dec_server, AUDIO_REQ_DEC, &req);

    return err;
}

int on_play_tts_start(int type)
{
    //TTS mp3数据开始下发，在此处开始进行MP3解码和播放
    AUDIO_PLAYER_DEBUG("*******tts start play mp3********\n");

    union audio_req req = {0};

    while (get_app_music_prompt_dec_status()) {
        os_time_dly(10);
    }
    dec_hdl.dec_server = server_open("audio_server", "dec");
    if (!dec_hdl.dec_server) {
        return -1;
    }

    server_register_event_handler_to_task(dec_hdl.dec_server, &dec_hdl, dec_server_event_handler, "tc_tvs_task");

    os_sem_create(&dec_hdl.end_sem, 0);
    dec_hdl.state = 0;
    u32 bufsize = DEC_CBUF_SIZE;
    dec_hdl.net_buf = net_buf_init(&bufsize, NULL);
    if (!dec_hdl.net_buf) {
        server_close(dec_hdl.dec_server);
        dec_hdl.dec_server = NULL;
        return -1;
    }
    net_buf_set_time_out(0, dec_hdl.net_buf);
    net_buf_active(dec_hdl.net_buf);

    g_total_tts_size = 0;
    g_tts_play_start = true;
    return 0;
}

void record_sem_post(void)
{
    if (os_sem_valid(&dec_hdl.end_sem)) {
        os_sem_post(&dec_hdl.end_sem);
    }
}

void on_play_tts_stop(bool force_stop)
{
    // force_stop为false，代表TTS mp3数据下发结束
    // force_stop为true，代表SDK需要强制停止TTS的播放，此时不管mp3数据是否解码播放完毕，都要立即停止
    if (!g_tts_play_start) {
        return;
    }
    AUDIO_PLAYER_DEBUG("*******tts end, total %d bytes********\n", g_total_tts_size);

    union audio_req req = {0};

    if (!force_stop) {
        net_buf_set_file_end(dec_hdl.net_buf);
        os_sem_pend(&dec_hdl.end_sem, 0);
    }

    g_tts_play_start = false;

    if (dec_hdl.dec_server != NULL) {
        net_buf_set_file_end(dec_hdl.net_buf);
        req.dec.cmd = AUDIO_DEC_STOP;
        server_request(dec_hdl.dec_server, AUDIO_REQ_DEC, &req);
        server_close(dec_hdl.dec_server);
        dec_hdl.dec_server = NULL;
    }

    if (dec_hdl.net_buf != NULL) {
        net_buf_uninit(dec_hdl.net_buf);
        dec_hdl.net_buf = NULL;
    }
}

int on_play_tts_recv_data(char *data, int data_len)
{
    //解码并播放mp3数据

    //AUDIO_PLAYER_DEBUG("*******tts playing, recv %d bytes, total %d bytes********\n", data_len, g_total_tts_size);
    if (!g_tts_play_start) {
        return -1;
    }


    int err = 0;
    union audio_req req = {0};

    if (dec_hdl.state == 0 && g_total_tts_size >= 4 * 1024) {	//yii：需要一定大小的数据检测格式
        dec_hdl.state = 1;
        req.dec.cmd             = AUDIO_DEC_OPEN;
        req.dec.volume          = get_app_music_volume();
        req.dec.output_buf_len  = 4 * 1024;
        req.dec.sample_source   = "dac";
        req.dec.dec_type        = "mp3";
        req.dec.file            = (FILE *)dec_hdl.net_buf;
        req.dec.vfs_ops 		= &tts_vfs_ops;

        err = server_request(dec_hdl.dec_server, AUDIO_REQ_DEC, &req);
        if (err) {
            g_tts_play_start = 0;
            server_close(dec_hdl.dec_server);
            dec_hdl.dec_server = NULL;
            net_buf_uninit(dec_hdl.net_buf);
            dec_hdl.net_buf = NULL;
            return -1;
        }
        req.dec.cmd = AUDIO_DEC_START;

        err = server_request(dec_hdl.dec_server, AUDIO_REQ_DEC, &req);
    }

    err = net_buf_write((u8 *)data, data_len, dec_hdl.net_buf);
    g_total_tts_size += data_len;

    return err;
}


/*************************************************************/

bool tvs_tts_adapter_play_inner()
{
    // 设置是否由适配层接管TTS的播放
    return CONFIG_DECODE_TTS_IN_SDK;
}

void tvs_init_mediaplayer_adapter_impl(tvs_mediaplayer_adapter *ad)
{
    if (ad == NULL) {
        return;
    }

    // adapter实现
    ad->get_offset = mediaplayer_adapter_get_offset;
    ad->set_source = mediaplayer_adapter_set_source;
    ad->stop_play = mediaplayer_adapter_stop_play;
    ad->start_play = mediaplayer_adapter_start_play;
    ad->pause_play = mediaplayer_adapter_pause_play;
    ad->resume_play = mediaplayer_adapter_resume_play;

    ad->play_inner = tvs_tts_adapter_play_inner;
    ad->tts_start = on_play_tts_start;
    ad->tts_end = on_play_tts_stop;
    ad->tts_data = on_play_tts_recv_data;

    // 初始化媒体播放器
    tvs_api_impl_media_player_init();
}

// TO-DO 监听播放器状态，调用此函数
void notify_player_status_change(const char *url, PLAYER_STATUS_E status)
{
    if (g_player_url == NULL || strlen(g_player_url) == 0 || url == NULL) {
        return;
    }

    int len = strlen(url);
    if (len == 0) {
        return;
    }

    len = strlen(g_player_url) > len ? len : strlen(g_player_url);

    if (strncmp(g_player_url, url, len) != 0) {
        return;
    }

    switch (status) {
    case PLAYER_URL_FINISHED:
        tvs_mediaplayer_adapter_on_play_finished(g_player_token);
        do_release_music();
        break;
    case PLAYER_URL_PLAYING:
        tvs_mediaplayer_adapter_on_play_started(g_player_token);
        break;
    case PLAYER_URL_PAUSED:
        tvs_mediaplayer_adapter_on_play_paused(g_player_token);
        break;
    case PLAYER_URL_STOPPED:
    case PLAYER_URL_ERROR:
        tvs_mediaplayer_adapter_on_play_stopped(-1, g_player_token);
        break;
    default:
        break;
    }
}

