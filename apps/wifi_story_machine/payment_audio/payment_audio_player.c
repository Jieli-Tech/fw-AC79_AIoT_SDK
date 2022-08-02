#include "system/includes.h"
#include "server/audio_server.h"
#include <stdlib.h>
#include "fs/fs.h"
#include "storage_device.h"
#include "os/os_api.h"
#include "app_config.h"
#include "malloc.h"

#if 0
#define	log_info(x, ...)	printf("\n------------[payment_audio_player]###" x "----------------\r\n",##__VA_ARGS__)
#else
#define	log_info(...)
#endif

#define SHUTDOWN_CMD    (1000)
#define TASK_QUIT       (2000)

#define DATA_BUF_SIZE   (50*1024)

/*if want to interrupt ,please open it*/

/* #define	PAYMENT_INTERRUPT_MODE */

#ifdef PAYMENT_INTERRUPT_MODE
u8	payment_interrupt_flag;
#define SET_INTERRUPT_FLAG (1001)
#endif

OS_SEM payment_sem;

struct payment_audio_hdl {
    struct server *dec_server;  		//解码服务
    cbuffer_t data_cbuf;        		//音频数据cycle buffer
    char data_buf[DATA_BUF_SIZE];    	//音频数据buffer
    char *ptr;					//
    u32 recv_size;						//数据缓存buffer
    u32 total_size;             		//音频文件总长度
    u32 play_size;              		//当前播放进度
    OS_SEM r_sem;               		//读音频数据信号量
    OS_SEM w_sem;               		//写音频数据信号量
    u8 run_flag;                		//播放标志
    u8 dec_ready_flag;          		//解码器就绪标志位
    u8 dec_volume;              		//播放音量
};
static struct payment_audio_hdl hdl;

static char buf0[50];
static char *buf1 = buf0 + 30, *buf2 = buf0 + 35, *buf3 = buf0 + 40;
static char recv_buf[30] = {0};
static u8 payment_uninit_flag;


struct	payment_msg {
    char buf[30];
    char msgid[30];
    u8 volume;
    u8 type;
};
static struct	payment_msg *pend_msg;

struct music_data_parm {
    const char buf;
    const char *str;
    u32 len;
};

extern const char music_data_0[1440];
extern const char music_data_1[1296];
extern const char music_data_2[1296];
extern const char music_data_3[1440];
extern const char music_data_4[1512];
extern const char music_data_5[1440];
extern const char music_data_6[1296];
extern const char music_data_7[1512];
extern const char music_data_8[1296];
extern const char music_data_9[1368];
extern const char music_data_b[1296];
extern const char music_data_p[1368];
extern const char music_data_q[1440];
extern const char music_data_s[1512];
extern const char music_data_w[1224];
extern const char music_data_y[1296];
extern const char music_data_wxsk[2736];
extern const char music_data_zfbdz[2808];
extern const char music_data_yuan[1440];

static const struct music_data_parm music_data[] = {
    {'0', music_data_0, sizeof(music_data_0)},
    {'1', music_data_1, sizeof(music_data_1)},
    {'2', music_data_2, sizeof(music_data_2)},
    {'3', music_data_3, sizeof(music_data_3)},
    {'4', music_data_4, sizeof(music_data_4)},
    {'5', music_data_5, sizeof(music_data_5)},
    {'6', music_data_6, sizeof(music_data_6)},
    {'7', music_data_7, sizeof(music_data_7)},
    {'8', music_data_8, sizeof(music_data_8)},
    {'9', music_data_9, sizeof(music_data_9)},
    {'b', music_data_b, sizeof(music_data_b)},
    {'p', music_data_p, sizeof(music_data_p)},
    {'q', music_data_q, sizeof(music_data_q)},
    {'s', music_data_s, sizeof(music_data_s)},
    {'w', music_data_w, sizeof(music_data_w)},
    {'y', music_data_y, sizeof(music_data_y)},

};



static int payment_audio_vfs_fread(void *priv, void *data, u32 len);
static int payment_audio_vfs_flen(void *priv);
static int payment_audio_vfs_seek(void *priv, u32 offset, int orig);
static void dec_server_event_handler(void *priv, int argc, int *argv);

static int close_audio_dec(void);
static int open_audio_dec(void);
static void payment_audio_buf_write(void);
static void local_audio_play_function(const char *buf, int len);
static void payment_audio_play_task(void);
static void buff_integration(char *buffer, char *src_buf, u8 len);



static const struct audio_vfs_ops payment_audio_vfs_ops = {
    .fread  = payment_audio_vfs_fread,
    .flen   = payment_audio_vfs_flen,
    .fseek  = payment_audio_vfs_seek,
};

static int payment_audio_vfs_fread(void *priv, void *data, u32 len)
{
    u32 rlen = 0;

    while (hdl.run_flag && hdl.dec_ready_flag) {
        if (hdl.total_size - hdl.play_size < 512) {
            len = hdl.total_size - hdl.play_size;
        }

        rlen = cbuf_read(&hdl.data_cbuf, data, len);
        if (rlen) {
            hdl.play_size += rlen;

            //发送写信号
            os_sem_set(&hdl.r_sem, 0);
            os_sem_post(&hdl.w_sem);

            if (hdl.play_size >= hdl.total_size) {
                log_info("net audio play complete.\n");
                log_info("play size = %d.\n", hdl.play_size);
                hdl.run_flag = 0;
                hdl.dec_ready_flag = 0;
                hdl.play_size = 0;
                //
                cbuf_clear(&hdl.data_cbuf);
                //
            }
            break;
        }

        os_sem_pend(&hdl.r_sem, 0);
    }

    return ((hdl.play_size >= hdl.total_size) ? 0 : len);
}

static int payment_audio_vfs_flen(void *priv)
{
    return hdl.total_size;
}


static int payment_audio_vfs_seek(void *priv, u32 offset, int orig)
{
    if (offset > 0) {
        hdl.dec_ready_flag = 1; //解码器开始偏移数据后，再开始播放，避免音频播放开端吞字
    }
    return 0;
}





static void dec_server_event_handler(void *priv, int argc, int *argv)
{
    int msg = 0;

    switch (argv[0]) {
    case AUDIO_SERVER_EVENT_ERR:
        log_info("AUDIO_SERVER_EVENT_ERR\n");
        break;

    case AUDIO_SERVER_EVENT_END:
        log_info("AUDIO_SERVER_EVENT_END\n");
        msg = SHUTDOWN_CMD;
        os_taskq_post(os_current_task(), 1, msg);

        break;

    case AUDIO_SERVER_EVENT_CURR_TIME:
        log_info("play_time: %d\n", argv[1]);
        break;

    default:
        break;
    }
}

static int close_audio_dec(void)
{
    union audio_req req = {0};
    req.dec.cmd = AUDIO_DEC_STOP;

    //关闭解码器
    log_info("stop dec.\n");
    server_request(hdl.dec_server, AUDIO_REQ_DEC, &req);

    return 0;


}

static int open_audio_dec(void)
{
    union audio_req req = {0};

    req.dec.cmd             = AUDIO_DEC_OPEN;
    req.dec.volume          = hdl.dec_volume;
    req.dec.output_buf      = NULL;
    req.dec.output_buf_len  = 12 * 1024;
    req.dec.channel         = 0;
    req.dec.sample_rate     = 0;
    req.dec.priority        = 1;
    req.dec.vfs_ops         = &payment_audio_vfs_ops;
    req.dec.file            = (FILE *)&hdl;
    req.dec.dec_type		= "mp3";
    req.dec.sample_source   = "dac";

    //打开解码器
    if (server_request(hdl.dec_server, AUDIO_REQ_DEC, &req) != 0) {
        return -1;
    }

    //开始解码
    req.dec.cmd = AUDIO_DEC_START;
    if (server_request(hdl.dec_server, AUDIO_REQ_DEC, &req) != 0) {
        return -1;
    }

    return 0;
}



//将音频数据写入data_cbuf
static void payment_audio_buf_write(void)
{
    u32 wlen = 0;
    char *data = hdl.ptr;
    u32 len = hdl.total_size;

    while (hdl.run_flag) {
        wlen = cbuf_write(&hdl.data_cbuf, data, len);
        if (wlen) {
            os_sem_set(&hdl.w_sem, 0);
            os_sem_post(&hdl.r_sem);

            hdl.recv_size += wlen;
            log_info("recv_size = %d", hdl.recv_size);

            /* 音频下载成功，或缓存成功，打开解码器，开始播放 */
            if (hdl.recv_size >= hdl.total_size) {
                log_info("audio dec open!\n");
                hdl.recv_size = 0;
                open_audio_dec();
            }
            break;
        }

        os_sem_pend(&hdl.w_sem, 0);
    }


}

static void local_audio_play_function(const char *buf, int len)
{
#ifdef	PAYMENT_INTERRUPT_MODE
    if (payment_interrupt_flag == 0) {
#endif
        if (payment_uninit_flag == 0) {
            int msg[32] = {0};
            server_register_event_handler(hdl.dec_server, &hdl, dec_server_event_handler);

            hdl.ptr = buf;
            hdl.total_size = len;
            hdl.run_flag = 1;

            thread_fork("payment_audio_buf_write", 20, 2048, 0, NULL, payment_audio_buf_write, NULL);

            for (;;) {
                os_task_pend("taskq", msg, ARRAY_SIZE(msg));
                if (msg[1] == SHUTDOWN_CMD) {
                    //关闭音频
                    close_audio_dec();
                    log_info("dec_server stop");
                    break;
                }
#ifdef	PAYMENT_INTERRUPT_MODE
                else if (msg[1] == SET_INTERRUPT_FLAG) {
                    payment_interrupt_flag = 1;
                    //关闭音频
                    close_audio_dec();
                    log_info("dec_server stop");
                    break;
                }
#endif
            }
        }
#ifdef	PAYMENT_INTERRUPT_MODE
    }
#endif
}

static void payment_audio_play_task(void)
{

#ifdef PAYMENT_INTERRUPT_MODE
    payment_interrupt_flag = 0;
#endif
    int msg[32];
    //初始化cycle buffer
    cbuf_init(&hdl.data_cbuf, hdl.data_buf, DATA_BUF_SIZE);
    os_sem_create(&hdl.r_sem, 0);
    os_sem_create(&hdl.w_sem, 0);

    hdl.dec_volume = pend_msg->volume;

    hdl.dec_server = server_open("audio_server", "dec");
    if (!hdl.dec_server) {
        log_info("audio server open fail.\n");
        goto play_task_exit;
    } else {
        log_info("open success");
    }

    //application
    if (pend_msg->type == 1) {
        local_audio_play_function(music_data_wxsk, sizeof(music_data_wxsk));
    } else if (pend_msg->type == 2) {
        local_audio_play_function(music_data_zfbdz, sizeof(music_data_zfbdz));
    }

    for (int i = 0; i < strlen(pend_msg->buf); i++) {
        for (int j = 0; j < ARRAY_SIZE(music_data); j++) {
            if (music_data[j].buf == pend_msg->buf[i]) {
                local_audio_play_function(music_data[j].str, music_data[j].len);
            }
        }
    }

    //yuan
    local_audio_play_function(music_data_yuan, sizeof(music_data_yuan));

    os_taskq_post("payment_jl_cloud_task", 2, 2, pend_msg->msgid);



play_task_exit:


    if (hdl.dec_server) {
        server_close(hdl.dec_server);
        hdl.dec_server = NULL;
    }

    if (pend_msg) {
        free(pend_msg);
    }

    os_sem_del(&hdl.r_sem, 0);
    os_sem_del(&hdl.w_sem, 0);
    memset(&hdl, 0, sizeof(struct payment_audio_hdl));

    os_sem_post(&payment_sem);


}





void payment_audio_kill()
{

    payment_uninit_flag = 1;

}

void payment_audio_open(void *priv)
{
    int msg[32];
    u8 err, integer_len;

    payment_uninit_flag = 0;

    os_sem_create(&payment_sem, 1);



    while (1) {
        os_sem_pend(&payment_sem, 0);

        err = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (err != OS_TASKQ || msg[0] != Q_USER) {
            continue;
        }
        if (payment_uninit_flag == 1) {
            break;
        }

        pend_msg = (struct payment_msg *)msg[1];

        memset(recv_buf, 0, sizeof(recv_buf));
        integer_len = strchr(pend_msg->buf, '.') - pend_msg->buf;
        strcpy(recv_buf, pend_msg->buf);
        recv_buf[integer_len] = 'p';
        memset(pend_msg->buf, 0, sizeof(pend_msg->buf));
        buff_integration(pend_msg->buf, recv_buf, integer_len);

        thread_fork("payment_audio_play_task", 20, 1024, 512, NULL, payment_audio_play_task, NULL);
    }

    os_sem_del(&payment_sem, 0);

}



static void buff_integration(char *buffer, char *src_buf, u8 len)
{
    u8 i = 0, zero_flag = 0;
    memset(buf0, 0, 50);

    snprintf(buf0, len + 1, "%s", src_buf);

    if (len > 4) {
        if (len > 8) {
            snprintf(buf1, len - 8 + 1, "%s", buf0);
            snprintf(buf2, 4 + 1, "%s", &buf0[len - 8]);
            snprintf(buf3, 4 + 1, "%s", &buf0[len - 4]);
        } else {
            snprintf(buf2, len - 4 + 1, "%s", buf0);
            snprintf(buf3, 4 + 1, "%s", &buf0[len - 4]);
        }
    } else {
        snprintf(buf3, len + 1, "%s", buf0);
    }



    //billion
    if (len == 10) {
        if (buf1[0] != '1') {
            buffer[i++] = buf1[0];
        }
        buffer[i++] = 's';

        if (buf1[1] != '0') {
            buffer[i++] = buf1[1];
        }
        buffer[i++] = 'y';
    } else if (len == 9) {

        buffer[i++] = buf1[0];
        buffer[i++] = 'y';
    }



    //million
    if (len >= 8) {
        if (buf2[0] != '0') {
            buffer[i++] = buf2[0];
            buffer[i++] = 'q';
        } else {
            zero_flag += 1;
        }

        if (buf2[1] != '0') {
            if (zero_flag != 0) {
                buffer[i++] = '0';
            }
            buffer[i++] = buf2[1];
            buffer[i++] = 'b';
            zero_flag = 0;
        } else {
            zero_flag += 1;
        }

        if (buf2[2] != '0') {
            if (zero_flag != 0) {
                buffer[i++] = '0';
            }
            buffer[i++] = buf2[2];
            buffer[i++] = 's';
            zero_flag = 0;
        } else {
            zero_flag += 1;
        }

        if (buf2[3] != '0') {
            if (zero_flag != 0) {
                buffer[i++] = '0';
            }
            buffer[i++] = buf2[3];
            zero_flag = 0;
            buffer[i++] = 'w';
        } else {
            if (zero_flag != 3) {
                buffer[i++] = 'w';
            }

        }
        zero_flag = 0;

    } else if (len == 7) {
        buffer[i++] = buf2[0];
        buffer[i++] = 'b';

        if (buf2[1] != '0') {
            buffer[i++] = buf2[1];
            buffer[i++] = 's';
        } else {
            zero_flag += 1;
        }

        if (buf2[2] != '0') {
            if (zero_flag != 0) {
                buffer[i++] = '0';
            }
            buffer[i++] = buf2[2];
        }
        buffer[i++] = 'w';
        zero_flag = 0;
    } else if (len == 6) {

        if (buf2[0] != '1') {
            buffer[i++] = buf2[0];
        }
        buffer[i++] = 's';

        if (buf2[1] != '0') {
            buffer[i++] = buf2[1];
        }
        buffer[i++] = 'w';
    } else if (len == 5) {

        buffer[i++] = buf2[0];
        buffer[i++] = 'w';
    }


    //thousand
    if (len >= 4) {
        if (buf3[0] != '0') {
            buffer[i++] = buf3[0];
            buffer[i++] = 'q';
        } else {
            zero_flag += 1;
        }

        if (buf3[1] != '0') {
            if (zero_flag != 0) {
                buffer[i++] = '0';
            }
            buffer[i++] = buf3[1];
            buffer[i++] = 'b';
            zero_flag = 0;
        } else {
            zero_flag += 1;
        }

        if (buf3[2] != '0') {
            if (zero_flag != 0) {
                buffer[i++] = '0';
            }
            buffer[i++] = buf3[2];
            buffer[i++] = 's';
            zero_flag = 0;
        } else {
            zero_flag += 1;
        }

        if (buf3[3] != '0') {
            if (zero_flag != 0) {
                buffer[i++] = '0';
            }
            buffer[i++] = buf3[3];
            zero_flag = 0;
        }
        zero_flag = 0;
    } else if (len == 3) {

        buffer[i++] = buf3[0];
        buffer[i++] = 'b';


        if (buf3[1] != '0') {
            buffer[i++] = buf3[1];
            buffer[i++] = 's';
        } else {
            zero_flag += 1;
        }

        if (buf3[2] != '0') {
            if (zero_flag != 0) {
                buffer[i++] = '0';
            }
            buffer[i++] = buf3[2];
        }
    } else if (len == 2) {

        if (buf3[0] != '1') {
            buffer[i++] = buf3[0];
        }
        buffer[i++] = 's';

        if (buf3[1] != '0') {
            buffer[i++] = buf3[1];
        }
    } else if (len == 1) {
        buffer[i++] = buf3[0];
    }

    if (src_buf[len + 2] == '0') {
        if (src_buf[len + 1] != '0') {
            snprintf(&buffer[i], 3, "%s", &src_buf[len]);
        }
    } else {
        snprintf(&buffer[i], 4, "%s", &src_buf[len]);
    }



    log_info("buffer = %s\r\n", buffer);
}












