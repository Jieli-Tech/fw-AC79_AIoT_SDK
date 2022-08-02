#include "sock_api/sock_api.h"
#include "lwip/sockets.h"//struct sockaddr_in
#include "server/rt_stream_pkg.h"   //head info .h
#include "server/video_server.h"//app_struct
#include "server/video_dec_server.h"//dec_struct
#include "server/ctp_server.h"//enum ctp_cli_msg_type
#include "time_compile.h"

#define CLOSE_STREAM 1
#define CLOSE_TALK   2

#define DEST_PORT 3333
#define CONN_PORT 2229
#define DEST_IP_SERVER "192.168.1.1"
#define HEAD_DATA 20

#define JPG_MAX_SIZE 100*1024
#define JPG_FPS 30
#define JPG_SRC_H 480
#define JPG_SRC_W 640


#define CHECK_CODE 0x88
#define CHECK_CODE_NUM 32

#define RECV_TIME_OUT  3*1000

struct __NET_CTP_INFO {
    u8 state;
    void *video_dec;
};
static struct __NET_CTP_INFO  net_ctp_info = {0};

struct __NET_ACTIVITY {
    u8 choice ;
};
static struct __NET_ACTIVITY  net_activity = {0};

enum ctp_state {
    NOT_CONNECT,
    CONNECTED_SUCC,
    SERVER_CLOSE,
};
struct __JPG_HW {
    u32 src_w;
    u32 src_h;
    u32 buf_len;
    u8  buf[];
};
struct ctp_hdl {
    struct list_head entry;
    int id;
    int recv_thread_pid;
    void *sock_hdl;
    enum ctp_state state;
    struct sockaddr_in dest_addr;
    int (*cb_func)(void *hdl, enum ctp_cli_msg_type type, const char *topic, const char *content, void *priv);
    void *priv;
    char *topic;
    unsigned int timeout;
    unsigned int timehdl;
    unsigned int timecheck;
    unsigned int timecheckout;
};

static void *rx_hdl = NULL;


static void close_activity(void *priv, u8 choice_close)
{
    void *sockfd = NULL;

    struct ctp_hdl *hdl = (struct ctp_hdl *)priv;

    /* 命令 */
    const char close_topic_1[] = {"CLOSE_RT_STREAM"};
    const char close_topic_2[] = {"RT_TALK_CTL"};//

    /* 命令参数 */
    const char close_content_1[] = {"{\"op\":\"PUT\",\"param\":{\"status\":\"1\"}}"};
    const char close_content_2[] = {"{\"op\":\"PUT\",\"param\":{\"status\":\"0\"}}"};

    extern int close_audio_dec(void *priv);
    extern void mic_record_stop(void);

    switch (choice_close) {
    case CLOSE_STREAM:

        if (ctp_cli_send(hdl, (char *)close_topic_1, (char *)&close_content_1) == 0) {
        } else {
            printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
        }

        break;

    case CLOSE_TALK:

        if (ctp_cli_send(hdl, (char *)close_topic_2, (char *)&close_content_2) == 0) {
        } else {
            printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
        }

        //关闭解码服务 和 接收数据的线程
        close_audio_dec(rx_hdl);
        mic_record_stop();

        break;
    }
}
int ctp_callback(void *hdl, enum ctp_cli_msg_type type, const char *topic, const char *parm_list, void *priv)
{
    static  u16 time_out = 0;
    static u16 close_time = 0;

    puts("CTP_callback~~");

    const char topic_0[] = {"KEEP_ALIVE_INTERVAL"};//命令名保持间隔活动
    const char content_0[] = {"{\"op\":\"GET\"}"};

    time_out++;

    /* 长时间没有发送这个命令会丢失连接 */
    if (time_out == 100) { //接收到合适的数量后发送一次ALIVE命令
        time_out = 0 ;

        if (ctp_cli_send(hdl, (char *)topic_0, (char *)&content_0) == 0) {
        } else {
            printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
        }
    }

    close_time++;
    printf("close_time=%d", close_time);

    /* 300次回调测试关闭对讲命令 */
    if (close_time == 300) {
        net_activity.choice = CLOSE_TALK;

        close_activity(hdl, net_activity.choice);


    }
    /* 500次回调测试关闭视频流 */
    if (close_time == 500) {
        net_activity.choice = CLOSE_STREAM;

        close_activity(hdl, net_activity.choice);
    }

    return 0;
}

int net_ctp_devinit(void)
{
    struct application *app = NULL;
    struct video_dec_arg arg_recv = {0};

    if (net_ctp_info.state) {
        printf("\n [WARING] %s multiple init\n", __func__);
        return 0 ;
    }

    arg_recv.dev_name = "video_dec";
    arg_recv.audio_buf_size = 0;
    arg_recv.video_buf_size = JPG_MAX_SIZE;

    net_ctp_info.video_dec = server_open("video_dec_server", &arg_recv);

    if (net_ctp_info.video_dec == NULL) {
        printf("\n [ERROR] video_dec_server open err\n");
        goto EXIT;
    }

    net_ctp_info.state = 1;
    return 0;

EXIT:

    if (net_ctp_info.video_dec) {
        server_close(net_ctp_info.video_dec);
        net_ctp_info.video_dec = NULL;
    }

    return -1;
}

void thread_socket_recv(void *sockfd)
{

    struct frm_head *head = NULL;
    struct __JPG_HW *jpg_recv = (struct __JPG_HW *) calloc(1, sizeof(struct __JPG_HW) + sizeof(u8) * (JPG_MAX_SIZE)); // no free

    if (!jpg_recv) {
        printf("\n[ERROR]  jpg calloc \n");
    }

    jpg_recv->src_w = JPG_SRC_W;
    jpg_recv->src_h = JPG_SRC_H;
    memset(jpg_recv->buf, 0, JPG_MAX_SIZE);

    char buf_head[HEAD_DATA] = {0};
    char ret;

    extern void rx_play_voice(void *priv, void *buf, u32 len);
    extern void *open_audio_server(void);
    rx_hdl = open_audio_server();

    while (1) {

        ret = sock_recv(sockfd, buf_head, HEAD_DATA, MSG_WAITALL);

        if (ret) {

            head = (struct frm_head *)buf_head;// Forced Conversion

            if (sock_recv(sockfd, jpg_recv->buf, head->frm_sz, MSG_WAITALL)) {

                switch (head->type) {
                case JPEG_TYPE_VIDEO :
                    printf("JPEG_TYPE_VIDEO~~~~");
                    break;

                case PCM_TYPE_AUDIO:

                    printf("AUDIO_start~~~~~");

                    if (net_activity.choice != CLOSE_TALK) {
                        rx_play_voice(rx_hdl, jpg_recv->buf, head->frm_sz);
                    }

                    break;

                default:
                    break;
                }

                head->type = 0;
            } else {
                printf("\nErr ret sock_recv - data\n");
                goto EXIT;
            }

        } else {
            printf("\n-------recv fail --------\n");
            goto EXIT;
        }
    }

EXIT:
    free(jpg_recv);
    printf("\n\n\n----free jpg_recv-----\n\n\n");
    ctp_cli_uninit();
}

void net_ctp_recv(void)
{
    void  *sockfd = NULL;
    struct sockaddr_in conn_addr;
    int ret;
    int pid;

    conn_addr.sin_family = AF_INET;
    conn_addr.sin_addr.s_addr = inet_addr(DEST_IP_SERVER) ;
    conn_addr.sin_port = htons(CONN_PORT);
    sockfd = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);

    //SOCK_STREAM  表示的是创建一个TCP的socket  然后对应的回调函数connect_callback表示在这条socket上的一些操作都会调用回调函数
    if (sockfd == NULL) {
        printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
    }

    if (sock_connect(sockfd, &conn_addr, sizeof(struct sockaddr_in)) == -1) {
        printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
    } else {
        ret = net_ctp_devinit();

        if (ret == 0) {
            printf("\n-----net-ctp-dev-init-Sueccess\n");

            if ((thread_fork("thread_socket_recv", 10, 10 * 1024, 0, &pid, thread_socket_recv, sockfd))) {
                printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
            }
        } else {
            printf("\n[ERR]- net-scr-devinit\n");
        }

    }
}

int tx_talk_net_init(void)
{
    extern void init_sock_receive_voice(void);
    init_sock_receive_voice();

    return 0;
}

void *net_ctp_init(void)
{
    void *sockfd = NULL;

    struct ctp_hdl *hdl;

    struct sockaddr_in dest_addr;

    /* 定义命令数组 */
    const char topic_0[] = {"KEEP_ALIVE_INTERVAL"};//命令名保持间隔活动
    const char topic_1[] = {"APP_ACCESS"};//app创建命令
    const char topic_2[] = {"DATE_TIME"};//时间命令
    const char topic_3[] = {"OPEN_RT_STREAM"};//打开数据流命令 包括音频视频
    const char topic_4[] = {"RT_TALK_CTL"};//打开对讲命令

    /* 各个命令的参数 */
    const char content_0[] = {"{\"op\":\"GET\"}"};
    const char content_1[] = {"{\"op\":\"PUT\",\"param\":{\"type\":\"1\",\"ver\":\"1.0\"}}"};
    char *Str_time;
    sprintf((char *)&Str_time, "{\"op\":\"PUT\",\"param\":{\"date\":\"%d%02d%02d%02d%02d%02d\"}}", YEAR, MONTH, DAY, HOUR, MINUTE, SECOND);
    printf("\n\n DATA - %d:%d:%d  - %d:%d:%d \n\n", YEAR, MONTH, DAY, HOUR, MINUTE, SECOND);
    const char content_3[] = {"{\"op\":\"PUT\",\"param\":{\"rate\":\"8000\",\"w\":\"640\",\"fps\":\"30\",\"h\":\"480\",\"format\":\"1\"}}"};
    const char content_4[] = {"{\"op\":\"PUT\",\"param\":{\"status\":\"1\"}}"};

    memset(&dest_addr, 0x0, sizeof(dest_addr)); //对server_info初始化操作，全部清零

    ctp_cli_init();//初始化连接结构体参数等
    net_ctp_recv();//建立sock并初始化视频解码服务

    /* 建立CTP连接 */
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(DEST_IP_SERVER) ;
    dest_addr.sin_port = htons(DEST_PORT);

    sockfd = ctp_cli_reg(1, &dest_addr, ctp_callback, &hdl);

    if (sockfd == NULL) {
        printf("\n--------------CTP connect failed----------------\n");
    } else {
        printf("\n--------------CTP connect success---------------\n");
    }

    /* 发送命令和命令参数 */
    if (ctp_cli_send(sockfd, (char *)&topic_0, (char *)&content_0) == 0) {
    } else {
        printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
        goto ERROR;
    }

    if (ctp_cli_send(sockfd, (char *)&topic_1, (char *)&content_1) == 0) {
    } else {
        printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
        goto ERROR;
    }

    if (ctp_cli_send(sockfd, (char *)&topic_2, (char *)&Str_time) == 0) {
    } else {
        printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
        goto ERROR;
    }

    if (ctp_cli_send(sockfd, (char *)&topic_3, (char *)&content_3) == 0) {
    } else {
        printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
        goto ERROR;
    }

    if (ctp_cli_send(sockfd, (char *)&topic_4, (char *)&content_4) == 0) {
        tx_talk_net_init();
    } else {
        printf("\n [ERROR] %s - %d\n", __FUNCTION__, __LINE__);
        goto ERROR;
    }

    return sockfd;

ERROR:

    return NULL;
}
