#include "app_config.h"
#ifdef CONFIG_UI_ENABLE //上电执行则打开app_config.h TCFG_DEMO_UI_RUN = 1

#include "device/device.h"//u8
#include "event/key_event.h"

#include "asm/gpio.h"
#include "system/includes.h"


#if CONFIG_MP3_DEC_ENABLE
#include "server/audio_server.h"
#endif //CONFIG_MP3_DEC_ENABLE

/*********播放flash中的mp3资源****用于按键播放提示音和开机音乐**************************/

#if CONFIG_MP3_DEC_ENABLE

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

/****************按键部分**********************/
enum data_mode {
    camera,
    ui,
    ui_camera,
};

extern u8 data_mode;

extern int ui_key_control(u8 value, u8 event);
extern int ui_camera_key_control(u8 value, u8 event);

static int camera_ui_key_click(struct key_event *key)
{
    static u8 mode = 0; //0为摄像头输出模式 1为UI输出模式

    int err;

    switch (key->action) {
    case KEY_EVENT_CLICK:
        switch (key->value) {
        case KEY_MODE: //IO_key//键值
            printf(">>>>>>>>>>>>>>>>key power_off");
            break;
        case KEY_UP:
            printf(">>>>>>>>>>>>>>>key1");
            break;
        case KEY_ENC:
            printf(">>>>>>>>>>>>>>>>key2");
            break;
        case KEY_F1:
            printf(">>>>>>>>>>>>>>>>key3");
            break;
        case KEY_MENU:
            mode = !mode;//按键切显示模式
            printf(">>>>>>>>>>>>>>>key4");
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }

    if (!mode) { //按键分支 一个模式对应一个按键分支 便于编写
        printf(">>>>>>>>ui_camera_key");
        ui_camera_key_control(key->value, key->action);
    } else {
        printf(">>>>>>>>ui_key");
        ui_key_control(key->value, key->action);
    }
    return false;
}

int camera_ui_key_event_handler(struct key_event *key)
{
    char ret;
    switch (key->action) {
    case KEY_EVENT_CLICK:
        ret = camera_ui_key_click(key);
        break;
    case KEY_EVENT_LONG:
        break;
    case KEY_EVENT_HOLD:
        break;
    case KEY_EVENT_UP:
        break;
    default:
        break;
    }
    return 0;
}
#endif
