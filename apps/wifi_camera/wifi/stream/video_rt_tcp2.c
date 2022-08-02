#include "system/spinlock.h"
#include "os/os_api.h"
#include "server/rt_stream_pkg.h"
#include "lwip.h"
#include "server/net2video.h"
#include "os/os_api.h"
#include "sys_common.h"
#include "sock_api/sock_api.h"
#include "stream_core.h"
#include "app_config.h"


#define H264_CLANNEL   10000
#define JPEG_CLANNEL   10001
#define CMD_CLANNEL    10002

extern int atoi(const char *__nptr);
struct cli_info {
    struct list_head entry;
    void *fd;
    struct sockaddr_in addr;
    int id;
};


struct video_rt_tcp_server_info {
    struct list_head cli_head;
    struct sockaddr_in local_addr;
    void *fd;
    int inited;
    int (*callback)(int cmd, char *buffer, int len, void *priv);
    OS_SEM sem;
    u32 flag;
};




static struct video_rt_tcp_server_info server_info = {
    .cli_head = {
        .next = &server_info.cli_head,
        .prev = &server_info.cli_head,
    },
};

static struct cli_info *get_tcp_net_info(u16 port);

static int  path_analyze(struct rt_stream_info *info, const *path)
{

    char *tmp = NULL;
    char *tmp2 = NULL;
    char ip[15] = {0};
    u16 port = 0;
    tmp = strstr(path, "cty://");
    if (!tmp) {
        return -1;
    }

    tmp += strlen("cty://");

    tmp2 = strchr(tmp, ':');
    printf("tmp=%s  len=%d\n", tmp, tmp2 - tmp);
    strncpy(ip, tmp, tmp2 - tmp);
    port = atoi(tmp2 + 1);

    printf("remote ip:%s  port:%d\n", ip, port);


    info->addr.sin_family = AF_INET;
    info->addr.sin_addr.s_addr = inet_addr(ip);
    info->addr.sin_port = htons(port);
    return 0;

}


static void *video_rt_tcp_init(const char *path, const char *mode)
{
    int ret = 0;
    u16 port = 0;
    log_i("video_rt_tcp_init222\n");
    struct rt_stream_info *__dump = calloc(1, sizeof(struct rt_stream_info));

    if (__dump == NULL) {
        log_e("%s malloc fail\n", __FILE__);
        return NULL;
    }


    path_analyze(__dump, path);

    struct cli_info *info = get_tcp_net_info(ntohs(__dump->addr.sin_port));


    if (info == NULL) {
        log_e("%s get_tcp_net_info\n", __FILE__);
        free(__dump);
        return NULL;
    }

    memcpy((void *)&__dump->addr, (void *)&info->addr, sizeof(struct sockaddr));
    __dump->fd = info->fd;

    return (void *)__dump;
}
static int video_rt_tcp_send(void *file, void *buf, u32 len, u8 type)
{
    struct rt_stream_info *__dump = (struct rt_stream_info *)file;
    int ret = 0;
    u32 data_size = len;
    if (type == H264_TYPE_VIDEO || type == JPEG_TYPE_VIDEO) {
        data_size |= (1 << 31);
    } else if (type == PCM_TYPE_AUDIO) {
        data_size &= ~(1 << 31);
    }

    ret = sock_send(__dump->fd, (char *)&data_size, 4, 0);
    if (ret <= 0) {
        return ret;
    }
    ret = sock_send(__dump->fd, buf, len, 0);

    return ret;

}

static int video_rt_tcp_uninit(void *file)
{
    log_i("video_rt_tcp_uninit2222\n\n\n\n\n\n");
    int ret = 0;
    struct rt_stream_info *__dump = (struct rt_stream_info *)file;
    u32 end_frame = 0x12348765;
    ret = sock_send(__dump->fd, (char *)&end_frame, 4, 0);
    if (ret <= 0) {
        return -1;
    }
    free(__dump);
    return 0;
}
REGISTER_NET_VIDEO_STREAM_SUDDEV(tcp2_video_stream_sub) = {
    .name = "cty",
    .open =  video_rt_tcp_init,
    .write = video_rt_tcp_send,
    .close = video_rt_tcp_uninit,
};



struct cli_info *get_tcp_net_info(u16 port)
{
    struct list_head *pos = NULL;
    struct cli_info *cli = NULL;
    int count = 0;
    list_for_each(pos, &server_info.cli_head) {
        cli = list_entry(pos, struct cli_info, entry);
        if (port == cli->id) {
            log_i("get id:%d ip:%s   port:%d\n\n", cli->id, inet_ntoa(cli->addr.sin_addr.s_addr), htons(cli->addr.sin_port));
            /* sock_clr_quit(cli->fd); */
            return cli;
        }

    }

    log_w("not find cli info \n");
    return cli;

}

/*
 *命令格式
 *|len|cmd|data|
 *|4byte|4byte|Nbyte|
 *cmd:命令号
 *len:总长度
 *data:数据（主要是给start_app传给启动video4的参数）
 * */

static u8 buffer[1024];
static void cmd_cli_task(void *arg)
{
    struct list_head *pos = NULL;
    struct cli_info *cli = NULL;
    int ret = 0;
    int len = 0;
    int cmd = 0;
    int find = 0;
    // 查找是否已经存在
    list_for_each(pos, &server_info.cli_head) {
        cli = list_entry(pos, struct cli_info, entry);
        if (CMD_CLANNEL ==  cli->id) {
            find = 1;
            break;
        }
    }
    if (!find) {
        return;
    }

    log_d("find it ip:%s   port:%d\n\n", inet_ntoa(cli->addr.sin_addr.s_addr), htons(cli->addr.sin_port));

    while (1) {
        //总长度
        ret = sock_recv(cli->fd, (char *)&len, 4, 0);
        if (ret != 4) {
            log_e("sock_recv fail1\n");
            goto exit_;
        }
        log_i("len=%d\n", len);
        //命令号
        ret = sock_recv(cli->fd, (char *)&cmd, 4, 0);
        if (ret !=  4) {
            log_e("sock_recv fail2\n");
            goto exit_;
        }
        log_i("cmd=%d\n", cmd);
        //数据
        //
        memset(buffer, 0, sizeof(buffer));
        if (len - 4 != 0) {
            ret = sock_recv(cli->fd, buffer, len - 4, 0);
            if (ret < 0) {
                log_e("sock_recv fail4\n");
                goto exit_;
            }
        }
        if (server_info.callback) {
            ret = server_info.callback(cmd, buffer, len - 4, NULL);
        }
        if (ret != CMD_NO_ERR) {
            log_e("cmd err=%d\n", ret);
        }

    }

exit_:
    log_d("cmd_cli_task exit\n");

}


int cmd_send(struct cmd_ctl *cinfo)
{
    struct list_head *pos = NULL;
    struct cli_info *cli = NULL;
    int ret = 0;
    int len = 0;

    // 查找是否已经存在
    list_for_each(pos, &server_info.cli_head) {
        cli = list_entry(pos, struct cli_info, entry);
        if (CMD_CLANNEL ==  cli->id) {
            break;
        }
    }
    log_i("send0\n");
    ret = sock_send(cli->fd, (char *)cinfo, 8, 0);
    if (ret != 8) {
        log_e("sock send fail1111\n");
        return -1;
    }

    log_i("send1  len=%d\n", cinfo->len - 4);
    if (cinfo->len - 4 != 0) {
        ret = sock_send(cli->fd, cinfo->data, cinfo->len - 4, 0);
        if (ret != cinfo->len - 4) {
            log_e("sock send fail22222 ret=%d\n", ret);
            return -1;
        }
    }
    log_i("send2\n");
    return 0;

}




static void __do_sock_accpet(void *arg)
{
    u32 find  = 0;
    struct list_head *pos = NULL;
    struct cli_info *cli = NULL;
    int ret = 0;
    u8 task_name[64];
    u32 count = 0;

    socklen_t len = sizeof(server_info.local_addr);
    while (1) {

        struct cli_info *__cli = calloc(1, sizeof(sizeof(struct cli_info)));
        if (__cli == NULL) {
            log_e("malloc fail\n");
            while (1);
        }


        __cli->fd  = sock_accept(server_info.fd, (struct sockaddr *)&__cli->addr, &len, NULL, NULL);
        if (__cli->fd == NULL) {
            log_w("some error in here\n\n");
            free(__cli);
            continue;
        }
        if (server_info.flag) {
            free(__cli);
            break;
        }

        ret = sock_recv(__cli->fd, (char *)&__cli->id, 4, 0);
        if (ret != 4) {
            sock_unreg(__cli->fd);
            free(__cli->fd);
            continue;
        }
        log_i("__cli->id = %d\n", __cli->id);

        list_add_tail(&__cli->entry, &server_info.cli_head);

        //命令通道,另开线程处理
        if (__cli->id == CMD_CLANNEL) {
            sprintf(task_name, "cmd_cli_task%x", count++);
            ret = thread_fork(task_name, 28, 0x1000, 0, 0, cmd_cli_task, NULL);
            if (ret != OS_NO_ERR) {
                log_e("thread_fork fail\n");
                continue;
            }

        }

    }

}


int video_rt_tcp_server_init2(int port, int (*callback)(int, char *, int, void *))
{
    int ret = 0;
    log_i("video_rt_tcp_server_init2\n");

    memset(&server_info, 0x0, sizeof(server_info));
    os_sem_create(&server_info.sem, 0);

    server_info.local_addr.sin_family = AF_INET;
    server_info.local_addr.sin_addr.s_addr = htonl(INADDR_ANY) ;
    server_info.local_addr.sin_port = htons(port);


    server_info.fd = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);

    if (server_info.fd == NULL) {
        return -1;
    }

    u32 opt = 1;
    if (sock_setsockopt(server_info.fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_e("%s sock_bind fail\n", __FILE__);
        return -1;
    }


    if (sock_bind(server_info.fd, (struct sockaddr *)&server_info.local_addr, sizeof(struct sockaddr))) {
        log_e("%s sock_bind fail\n", __FILE__);
        return -1;
    }
    sock_listen(server_info.fd, 0x5);

    INIT_LIST_HEAD(&server_info.cli_head);
    server_info.inited = 1;
    server_info.callback = callback;

    ret = thread_fork("__do_sock_accpet", 28, 0x1000, 0, 0, __do_sock_accpet, NULL);
    if (ret != OS_NO_ERR) {
        log_e("%s thread fork fail\n", __FILE__);
        return -1;

    }

    return 0;

}
void video_rt_tcp_server_uninit2()
{
    printf("video_rt_tcp_server_uninit\n");
    server_info.flag = 1;
    sock_set_quit(server_info.fd);
    sock_unreg(server_info.fd);
    server_info.flag = 0;
    // 断开所有客户端和删除链表
}

void video_disconnect_all_cli()
{
    struct list_head *pos = NULL, *n = NULL;
    struct cli_info *cf = NULL;
    if (!server_info.inited) {
        return;
    }
    log_d("disconnect all client\n");
    list_for_each_safe(pos, n, &server_info.cli_head) {
        cf = list_entry(pos, struct cli_info, entry);
        list_del(&cf->entry);
        if (cf->fd != NULL) {
            sock_unreg(cf->fd);
            cf->fd = NULL;
        }
        if (cf != NULL) {
            free(cf);
            cf = NULL;
        }
    }
}





