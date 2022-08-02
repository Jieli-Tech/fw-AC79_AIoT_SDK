#include "system/includes.h"
#include "app_config.h"
#include "audio_server.h"

#ifdef USE_PITCH_SPEED_TEST

static void *dec_server;

static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        puts("AUDIO_SERVER_EVENT_ERR\n");
    case AUDIO_SERVER_EVENT_END:
        puts("AUDIO_SERVER_EVENT_END\n");
        os_time_dly(3);
        union audio_req r = {0};
        r.dec.cmd = AUDIO_DEC_PAUSE;
        if (dec_server) {
            server_request(dec_server, AUDIO_REQ_DEC, &r);
        }
        break;
    case AUDIO_SERVER_EVENT_CURR_TIME:
        log_d("play_time: %d\n", argv[1]);
        break;
    }
}

static int play_local_music(void *file)
{
    int err;
    union audio_req req = {0};

    if (!file) {
        return -1;
    }

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = 60;
    req.dec.output_buf      = NULL;
    req.dec.output_buf_len  = 24 * 1024;
    req.dec.file            = (FILE *)file;
    req.dec.channel         = 0;
    req.dec.sample_rate     = 0;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = NULL;
    req.dec.sample_source   = "dac";
#if 1	//变声变调功能
    req.dec.speedV = 80; // >80是变快，<80是变慢，建议范围：30到130
    req.dec.pitchV = 32768; // >32768是音调变高，<32768音调变低，建议范围20000到50000
    req.dec.attr = AUDIO_ATTR_PS_EN;
#endif



    err = server_request(dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        log_e("audio_dec_open: err = %d\n", err);
        fclose((FILE *)file);
        return err;
    }

    req.dec.cmd = AUDIO_DEC_START;

    err = server_request(dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        log_e("audio_dec_start: err = %d\n", err);
        fclose((FILE *)file);
        return err;
    }

    return 0;
}





static void pitch_speed_config(int speed, int pitch)
{

    union audio_req req = {0};

    req.dec.cmd			= AUDIO_DEC_PS_PARM_SET;
    req.dec.speedV 		= speed; // >80是变快，<80是变慢，建议范围：30到130
    req.dec.pitchV 		= pitch; // >32768是音调变高，<32768音调变低，建议范围20000到50000
    req.dec.attr 		= AUDIO_ATTR_PS_EN;

    server_request(dec_server, AUDIO_REQ_DEC, &req);
}



static void pitch_speed_thread()
{
    //1. 等待SD卡挂载
    while (!storage_device_ready()) {
        os_time_dly(20);
    }

    //2. 打开dec服务
    dec_server = server_open("audio_server", "dec");
    if (!dec_server) {
        printf("open audio_server failed\n\r");
        return ;
    }
    server_register_event_handler_to_task(dec_server, NULL, dec_server_event_handler, "app_core");

    FILE *fd = NULL;
    fd = fopen(CONFIG_ROOT_PATH"1.mp3", "r");
    if (!fd) {
        printf("open fd failed \n\r");
        goto exit;
    }

    //4. 打开本地歌曲
    int ret = play_local_music(fd);
    if (ret) {
        printf("play local music failed\n\r");
        goto exit;
    }

    os_time_dly(2000);

    printf("130 32768\n\r");
    //5. 变快
    pitch_speed_config(130, 32768);

    os_time_dly(1000);

    printf("80 50000\n\r");
    //6. 升调
    pitch_speed_config(80, 50000);

    os_time_dly(1000);

    printf("80 20000\n\r");
    //7. 降调
    pitch_speed_config(80, 20000);

    os_time_dly(1000);

    printf("30 32768\n\r");
    //8. 变慢
    pitch_speed_config(30, 32768);

    return;

exit:
    if (fd) {
        fclose(fd);
    }
    if (dec_server) {
        server_close(dec_server);
        dec_server = NULL;
    }

}



static void main()
{
    thread_fork("pitch_speed_thread", 20, 1024, 0, 0, pitch_speed_thread, 0);
}
late_initcall(main);


#endif

