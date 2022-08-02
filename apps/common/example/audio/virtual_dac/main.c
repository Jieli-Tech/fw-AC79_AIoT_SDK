#include "system/includes.h"
#include "server/audio_server.h"
#include "app_config.h"

#ifdef USE_VIRTUAL_DAC_TEST

static FILE *test_file;
static FILE *file;
static void *virtual_hdl;
static int virtual_pid;
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

static void virtual_get_data(void *priv)
{
    struct audio_cbuf_t *virtual = (struct audio_cbuf_t *)priv;
    if (!virtual) {
        return;
    }
    int rlen = 0;
    u8 buf[1024];
    while (1) {
        os_sem_post(virtual->wr_sem);
        rlen = cbuf_read(virtual->cbuf, buf, 1024);
        if (rlen == 1024) {
            fwrite(buf, 1, rlen, test_file);
            continue;
        }
        //收到end为1, 把剩余的数据读出
        if (virtual->end) {
            virtual->end = 2;
            os_sem_post(virtual->wr_sem);
            rlen = cbuf_get_data_size(virtual->cbuf);
            cbuf_read(virtual->cbuf, buf, rlen);
            fwrite(buf, 1, rlen, test_file);
            break;
        }
        putchar('<');
        //等待解码器post信号量
        os_sem_pend(virtual->rd_sem, 0);
        putchar('>');
    }



    return;
}


static int virtual_stop()
{
    union audio_req req = {0};

    struct audio_cbuf_t *virtual = (struct audio_cbuf_t *)virtual_hdl;
    os_sem_post(virtual->rd_sem);

    thread_kill(&virtual_pid, KILL_WAIT);
    if (dec_server) {
        req.dec.cmd = AUDIO_DEC_STOP;
        server_request(dec_server, AUDIO_REQ_DEC, &req);
    }
    virtual_hdl = NULL;
    fclose(test_file);
    fclose(file);
    test_file = NULL;
    file = NULL;
}

static int virtual_play()
{
    int err;
    union audio_req req = {0};

    //3. 打开输入文件
    file = fopen(CONFIG_ROOT_PATH"1.mp3", "r");
    if (!file) {
        printf("open file fail\n\r");
        return -1;
    }

    //4. 打开输出文件
    test_file = fopen(CONFIG_ROOT_PATH"2.pcm", "w+");
    if (!test_file) {
        printf("open test file fail\n\r");
        return -1;
    }


    //5. 打开解码器
    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = 100;
    req.dec.output_buf      = NULL;
    req.dec.output_buf_len  = 1024 * 12;
    req.dec.file            = (FILE *)file;
    req.dec.channel         = 0;
    req.dec.sample_rate     = 0;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = NULL;
    req.dec.sample_source   = "virtual";

    err = server_request(dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        log_e("audio_dec_open: err = %d\n", err);
        fclose((FILE *)file);
        fclose(test_file);
        return err;
    }


    //6. 获取虚拟句柄
    virtual_hdl = req.dec.virtual_audio;
    //7. 创建数据接收线程
    thread_fork("test", 20, 1024, 0, &virtual_pid, virtual_get_data, virtual_hdl);

    //8. 开始解码
    req.dec.cmd = AUDIO_DEC_START;

    err = server_request(dec_server, AUDIO_REQ_DEC, &req);
    if (err) {
        log_e("audio_dec_start: err = %d\n", err);
        fclose((FILE *)file);
        fclose(test_file);
        return err;
    }


    puts("virtual_file: suss\n");


    return 0;

}

static void virtual_thread()
{
    //1. 等待SD卡挂载
    while (!storage_device_ready()) {
        os_time_dly(20);
    }
    //2. 打开dec服务
    dec_server = server_open("audio_server", "dec");
    if (!dec_server) {
        printf(" open audio_server failed \n");
        return ;
    }
    server_register_event_handler_to_task(dec_server, NULL, dec_server_event_handler, "app_core");

    int ret = virtual_play();
    if (ret) {
        printf("virtual_play failed\n");
        goto exit;
    }

    struct audio_cbuf_t *virtual = (struct audio_cbuf_t *)virtual_hdl;
    //9. 等待解码完成
    while (!virtual->end) {
        os_time_dly(20);
    }
    //10. 关闭文件, post信号量, 关闭解码器
    virtual_stop();

exit:
    if (dec_server) {
        server_close(dec_server);
        dec_server = NULL;
    }
    if (test_file) {
        fclose(test_file);
    }
    if (file) {
        fclose(file);
    }

}

static void main()
{
    thread_fork("virtual_thread", 20, 1024, 0, 0, virtual_thread, 0);
}
late_initcall(main);

#endif
