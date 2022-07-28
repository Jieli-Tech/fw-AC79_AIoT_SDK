#include "app_config.h"

#ifdef USE_DevKitBoard_TEST_DEMO

#include "device/device.h"//u8
#include "system/includes.h"//GPIO
#include "sys_common.h"
#include "storage_device.h" //fs

/*********播放flash中的mp3资源****用于按键播放提示音和开机音乐**************************/
#ifdef CONFIG_MP3_DEC_ENABLE
#include "server/audio_server.h"
#define SHUTDOWN_CMD    (1001)

struct flash_mp3_hdl {
    struct server *dec_server;
    char *file_name;
    char  file_path[64];
    u8 dec_volume;
    FILE *file;
};

static struct flash_mp3_hdl *mp3_info = {0};

static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    int msg = 0;
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        printf("AUDIO_SERVER_EVENT_ERR\n");
        break;
    case AUDIO_SERVER_EVENT_END:
        printf("AUDIO_SERVER_EVENT_END\n");
        msg = SHUTDOWN_CMD;
        os_taskq_post(os_current_task(), 1, msg);
        break;
    case AUDIO_SERVER_EVENT_CURR_TIME:
        printf("play_time: %d\n", argv[1]);
        break;
    default:
        break;
    }
}

static void play_mp3_task(void *priv)
{
    int msg[32] = {0};
    int err;
    printf("<<<<<<<<<<<<<<<<<<<path = %s", mp3_info->file_path);
    mp3_info->file = fopen(mp3_info->file_path, "r");
    if (!mp3_info->file) {
        puts("no this mp3!\n");
    }

    mp3_info->dec_server = server_open("audio_server", "dec");
    if (!mp3_info->dec_server) {
        puts("play_music open audio_server fail!\n");
        goto __err;
    }

    server_register_event_handler(mp3_info->dec_server, NULL, dec_server_event_handler);

    union audio_req req = {0};

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = mp3_info->dec_volume;
    req.dec.output_buf      = NULL;
    req.dec.output_buf_len  = 12 * 1024;
    req.dec.channel         = 0;
    req.dec.sample_rate     = 0;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = NULL;
    req.dec.file            = mp3_info->file;
    req.dec.dec_type		= "mp3";
    req.dec.sample_source   = "dac";
    //打开解码器
    if (server_request(mp3_info->dec_server, AUDIO_REQ_DEC, &req) != 0) {
        puts("1open_audio_dec_err!!!");
        return ;
    }
    //开始解码
    req.dec.cmd = AUDIO_DEC_START;
    if (server_request(mp3_info->dec_server, AUDIO_REQ_DEC, &req) != 0) {
        puts("2open_audio_dec_err!!!");
        return ;
    }

    for (;;) {
        os_task_pend("taskq", msg, ARRAY_SIZE(msg));
        if (msg[1] == SHUTDOWN_CMD) {
            //关闭音频
            union audio_req req = {0};
            req.dec.cmd = AUDIO_DEC_STOP;
            //关闭解码器
            printf("stop dec.\n");
            server_request(mp3_info->dec_server, AUDIO_REQ_DEC, &req);
            server_close(mp3_info->dec_server);

            if (mp3_info->file) {
                fclose(mp3_info->file);
            }
            os_taskq_post("flash_mp3_play_task", 1, mp3_info);
            printf(">>>>>dec_server stop");
            break;
        }
    }

    return;
__err:
    fclose(mp3_info->file);
    server_close(mp3_info->dec_server);
    return;

}
static void flash_mp3_play_task(void *priv)
{
    int err;
    int msg[32] = {0};

    while (1) {
        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }

        mp3_info = (struct flash_mp3_hdl *)msg[1];
        thread_fork("play_mp3_task", 10, 1024, 32, NULL, play_mp3_task, NULL);

        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }
    }
}

static void flash_mp3_open(void)
{
    mp3_info = (struct flash_mp3_hdl *)calloc(1, sizeof(struct flash_mp3_hdl));
    thread_fork("flash_mp3_play_task", 10, 1024, 32, NULL, flash_mp3_play_task, NULL);
}
late_initcall(flash_mp3_open);

/*=调用该函数即可完成flash中的资源播放 例如post_msg_play_flash_mp3("music_test.mp3",50)50是播放音量最大100=*/
void post_msg_play_flash_mp3(char *file_name, u8 dec_volume)
{
    int msg = 0;
    snprintf(mp3_info->file_path, 64, CONFIG_VOICE_PROMPT_FILE_PATH"%s", file_name);
    mp3_info->dec_volume = dec_volume;
    msg = SHUTDOWN_CMD;
    os_taskq_post("play_mp3_task", 1, msg);
    os_taskq_post("flash_mp3_play_task", 1, mp3_info);
}
#endif
/***************上面为提示音播放部分*********************************************/

#endif
