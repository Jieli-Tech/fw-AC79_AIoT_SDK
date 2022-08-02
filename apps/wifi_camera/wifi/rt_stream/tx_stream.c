#include "system/includes.h"//cbuf
#include "sock_api/sock_api.h"
#include "server/audio_server.h"//static const struct audio_vfs_ops
#include "server/rt_stream_pkg.h"//head info .h
#include "lwip/sockets.h"//struct sockaddr_in

static struct {
    struct server *enc_server;  //编码server
    u8 enc_volume;              //录音音量
    char *data_buf;             //buf，存储录音数据
    u32 buf_len;                //buf长度
    cbuffer_t record_cbuf;      //cycle buf
    OS_SEM r_sem;
} record_hdl = {
    .enc_volume = 100,
    .buf_len = 5 * 1024,
};
#define __this  (&record_hdl)

#define DATA_BUF_SIZE  5*1024

struct __tx_talk {

    void *sock;
    u8 data_buf[DATA_BUF_SIZE];
    int thread_pid;
    u32 recv_time;
    cbuffer_t cbuf_net_data;
};
struct __tx_talk  *tx_talk;

//功  能：MIC驱动底层将调用该函数，将录音数据写进指定的文件中
//参  数：void *file：文件描述符
//        void *data：data指针
//        u32 len   : data大小
//返回值：写入字节数
static int record_vfs_fwrite(void *file, void *data, u32 len)
{
    cbuf_write(&__this->record_cbuf, data, len);
    os_sem_set(&__this->r_sem, 0);
    os_sem_post(&__this->r_sem);

    return len;
}

static int record_vfs_fclose(void *file)
{
    return 0;
}

//录音文件操作方法集。传递给MIC底层驱动调用
static const struct audio_vfs_ops record_vfs_ops = {
    .fwrite = record_vfs_fwrite,
    .fclose = record_vfs_fclose,
};
//功能：底层驱动回调函数
static void enc_server_event_handler(void *priv, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
    case AUDIO_SERVER_EVENT_END:
        break;

    case AUDIO_SERVER_EVENT_CURR_TIME:
        printf("play time : %d\n", argv[1]);
        break;

    default:
        break;
    }
}
//功  能：开启MIC数据采样
//参  数：int sample_rate：采样率（单位：Hz）
//        u32 msec       ：采样时间（单位：ms）
//注意同一个MIC采样率保持一致 打断唤醒16000
static int mic_record_start(int sample_rate, u32 msec)
{
    int err = -1;
    union audio_req req = {0};

    //打开enc_server
    if (!__this->enc_server) {
        __this->enc_server = server_open("audio_server", "enc");

        if (!__this->enc_server) {
            goto _mic_err_;
        }

        server_register_event_handler(__this->enc_server, NULL, enc_server_event_handler);
    }

    //申请数据存储buf
    __this->data_buf = (char *)malloc(__this->buf_len);

    if (__this->data_buf == NULL) {
        goto _mic_err_;
    }

    //初始化cycle_buf
    cbuf_init(&__this->record_cbuf, __this->data_buf, __this->buf_len);

    os_sem_create(&__this->r_sem, 0);

    //MIC数据采集配置
    req.enc.cmd = AUDIO_ENC_OPEN;          //命令
    req.enc.channel = 1;                   //采样通道数目
    req.enc.volume = __this->enc_volume;   //录音音量
    req.enc.output_buf = NULL;             //缓存buf
    req.enc.output_buf_len = 8 * 1024;    //缓存buf大小
    req.enc.sample_rate = sample_rate;     //采样率
    req.enc.format = "pcm";                //录音数据格式
    req.enc.frame_size = sample_rate / 25; //数据帧大小
    req.enc.sample_source = "mic";         //采样源
    req.enc.vfs_ops = &record_vfs_ops;     //文件操作方法集
    req.enc.msec = msec;                   //采样时间

    //发送录音请求
    err = server_request(__this->enc_server, AUDIO_REQ_ENC, &req);

    if (err) {
        goto _mic_err_;
    }

    return 0;

_mic_err_:

    if (__this->enc_server) {
        server_close(__this->enc_server);
        __this->enc_server = NULL;
    }

    if (__this->data_buf) {
        free(__this->data_buf);
        __this->data_buf = NULL;
    }

    return -1;

}

//功  能：关闭MIC数据采样
//参  数：无
//返回值：无
void mic_record_stop(void)
{
    union audio_req req = {0};

    os_sem_post(&__this->r_sem);

    //关闭enc_server
    if (__this->enc_server) {
        req.enc.cmd = AUDIO_ENC_CLOSE;
        server_request(__this->enc_server, AUDIO_REQ_ENC, &req);
        server_close(__this->enc_server);
        __this->enc_server = NULL;
    }

    //释放buf
    if (__this->data_buf) {
        free(__this->data_buf);
        __this->data_buf = NULL;
    }
    //关闭数据接收线程
    if (tx_talk->thread_pid) {
        puts("kill recv task");
        thread_kill(&tx_talk->thread_pid, KILL_FORCE);
    }
}

void send_mic_data_handler(void *priv)
{
    u8 read_buf[660] = {0}; //buf大小与 frame_size 相等
    u32 rlen;

    mic_record_start(8000, 0);

    struct sockaddr_in dest_addr = {0};

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("192.168.1.1");
    dest_addr.sin_port = htons(2231);

    static u32 v_seq = 0, a_seq = 0;

    struct frm_head frame_head = {0};
    u32 frame_head_size = sizeof(struct frm_head);
    frame_head.type = PCM_TYPE_AUDIO ;

    while (1) {

        /* 收集够640个数据 */
        os_sem_pend(&__this->r_sem, 0);
        cbuf_get_data_size(&__this->record_cbuf);
        os_sem_pend(&__this->r_sem, 0);
        cbuf_get_data_size(&__this->record_cbuf + 320);


        rlen = cbuf_read(&__this->record_cbuf, read_buf + frame_head_size, sizeof(read_buf) - frame_head_size);

        frame_head.seq = (a_seq ++);
        frame_head.payload_size = rlen;
        frame_head.frm_sz = rlen;
        frame_head.timestamp += 0;
        memcpy(read_buf, &frame_head, frame_head_size);
        sock_sendto(tx_talk->sock, read_buf, rlen + frame_head_size, 0, &dest_addr, sizeof(dest_addr));
    }
}
void init_sock_receive_voice(void)
{
    struct sockaddr_in dest_addr;
    int err;

    /* 初始化结构体 */
    if (tx_talk != NULL) {
        printf("\nAPP not allow open again\n");
    }

    tx_talk = calloc(sizeof(struct __tx_talk), 1);

    if (tx_talk == NULL) {
        printf("\nnot enough space\n");
    }

    tx_talk->sock = sock_reg(AF_INET, SOCK_DGRAM, 0, NULL, NULL);

    if (tx_talk->sock == NULL) {
        printf("\n%s %d ->Error in socket()\n", __func__, __LINE__);
    }

    /* 绑定数据通道 */
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = htons(2231);
    err = sock_bind(tx_talk->sock, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));

    if (err) {
        printf("%s %d->Error in bind(),errno=%d\n", __func__, __LINE__, errno);
    }

    /* 建立MIC数据线程 */
    thread_fork("send_mic_data_handler", 25, 5 * 1024, 0, &tx_talk->thread_pid, send_mic_data_handler, NULL);
}
