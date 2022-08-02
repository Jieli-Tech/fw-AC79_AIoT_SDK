#include "app_config.h"
#if (defined CONFIG_UI_ENABLE) && (defined RAW_STREAM_ENABLE)
#include "wifi_raw_stream.h"
#include "system/includes.h"
#include "wifi/wifi_connect.h"
#include "event/net_event.h"
#include "net/assign_macaddr.h"

static u32 total_payload_len = 0;
static u32 jpeg_total_payload_len = 0;
static u8 pcm_finish = 0;
static u8 jpeg_finish = 0;
static u32 old_frame_seq = 0;//记录音频数据包号
static u32 jpeg_old_frame_seq = 0;//记录视频数据包号
#define PCM_BUF_SIZE   9000 //接收音频包
#define JPG_MAX_SIZE 100*1024
static u8 recBuf[JPG_MAX_SIZE] = {0};
#define MAX_PAYLOAD (500-sizeof(struct frm_head))//最后4字节属于h264流的
static u32 seq = 1;

static OS_MUTEX mutex1;
static OS_MUTEX mutex2;
static OS_MUTEX mutex3;
#define WIFI_TX_MAX_LEN 2510 //一次最大发送字节数

static int send_cnt = 0;
static int fps_cnt = 0;
static int wifi_raw_send(u8 *buf, int len);
static int startJpeg = 0;
static int checkPing = 0;
static int pingTime = 0;

#define LOSE_PING_TIME_CHECK 60 * 1000 //10s
#define SEND_PING_TIME 2 * 1000 //10s
void jpgImageDataRec(char *buf, int len);
extern int jpeg2yuv_jpeg_frame_write(u8 *buf, u32 len);

#define PCM_DATA_TYPE  1
#define JPEG_DATA_TYPE 2

static void send_status(void *p)
{
    printf("-->U  = %d KB/S \r\n", send_cnt * 27);
    send_cnt = 0;
}

#ifdef RAW_STREAM_RECEIVER
static void fps_status(void *p)
{
    printf("-->FPS  = %d fps \r\n", fps_cnt);
    fps_cnt = 0;
}
#endif

static void lose_online_ping(void)
{
    printf("-->lose online ping");
    startJpeg = 0;
    seq = 0;
}

static void send_online_ping(void)
{
    struct frm_head ping_head = {0};
    ping_head.type |= ONLINE_PING_TYPE;
    wifi_raw_send((u8 *)&ping_head, sizeof(struct frm_head));
}

static void send_reset_rsp(void)
{
    printf("-->send RESET RSP");
    struct frm_head ping_head = {0};
    ping_head.type |= RESET_RSP_TYPE;
    wifi_raw_send((u8 *)&ping_head, sizeof(struct frm_head));
}

//发送复位请求
static void send_reset_req(void)
{
    printf("-->send RESET REQ");
    struct frm_head pong_head = {0};
    pong_head.type |= RESET_REQ_TYPE;
    wifi_raw_send((u8 *)&pong_head, sizeof(struct frm_head));
}

//接收音频视频数据
static int get_jpeg_packet(char *buf, int len, char *pcm_buf, int *pcm_buf_len, struct __JPG_INFO *info)
{
    u32 position = 0;
    u32 jpg_position = 0;
    u32 frame_head_size = sizeof(struct frm_head);
    u8 frame_type;
    u32 cur_frame_seq;
    u32 frame_offset;
    u32 slice_data_len;
    u32 frame_size;

    static  u8 ps = 0;

    if (len < frame_head_size) {
        printf("\n%s %d->data err\n", __func__, __LINE__);
        goto ERROR;
    }

    do {
        struct frm_head  *head_info = (struct frm_head *)(buf + position);
        frame_type = head_info->type & 0x7F; //数据类型 是音频 还是 视频
        cur_frame_seq = head_info->seq;//帧序号
        frame_offset = head_info->offset;//当前帧偏移
        slice_data_len = head_info->payload_size;//当前数据长度
        frame_size = head_info->frm_sz;//帧头长度
        len -= (frame_head_size + slice_data_len);//如果帧头长+数据长度!=包长度

        if (len < 0) {
            printf("\n%s %d->data err\n", __func__, __LINE__); //认为是错误的数据包
            goto ERROR;
        }

        switch (frame_type) {
        case PCM_AUDIO_TYPE:
            if (cur_frame_seq < old_frame_seq) { //如果当前的seq小于旧的seq,说明是旧的数据包,跳过不处理
                printf("\n%s %d->recv old seq\n", __func__, __LINE__);
                goto continue_deal;

            } else if (cur_frame_seq > old_frame_seq) { //如果当前seq大于旧的seq,认为是新的数据包,接收包初始化
                if (total_payload_len && (pcm_finish == 0)) {
                    printf("\n%s %d->recv old seq\n", __func__, __LINE__);
                }

                old_frame_seq = cur_frame_seq;
                total_payload_len = 0;
                pcm_finish = 0;
                memset(pcm_buf, CHECK_CODE, PCM_BUF_SIZE);
            }

            if (bytecmp((unsigned char *)pcm_buf + frame_offset, CHECK_CODE, CHECK_CODE_NUM) != 0) {
                printf("\n%s %d->repeat seq\n", __func__, __LINE__);
                goto continue_deal;

            }

            if (frame_offset + slice_data_len > PCM_BUF_SIZE) {
                printf("\n%s %d->large data pcm buf too small\n", __func__, __LINE__);
                goto ERROR;
            }

            memcpy(pcm_buf + frame_offset, (buf + position) + frame_head_size, slice_data_len);//对应数据放置到对应的位置
            total_payload_len += slice_data_len;//累加总长度

            if (total_payload_len == frame_size) { //如果数据量相等,说明帧数据接收完成
                *pcm_buf_len = total_payload_len;
                pcm_finish = 1;
                /*printf("\n%s %d->finish\n", __func__, __LINE__);*/
                return PCM_DATA_TYPE;
            }
            goto continue_deal;

        case JPEG_VIDEO_TYPE:
            if (frame_offset + slice_data_len > JPG_MAX_SIZE) {
                printf("\n%s %d->large data pcm buf too small\n", __func__, __LINE__);
                goto ERROR;
            }

            if (cur_frame_seq < jpeg_old_frame_seq) { //如果当前的seq小于旧的seq,说明是旧的数据包,跳过不处理
                printf("\n%s %d->jpeg_recv old seq\n", __func__, __LINE__);
                goto continue_deal;

            } else if (cur_frame_seq > jpeg_old_frame_seq) { //如果当前seq大于旧的seq,认为是新的数据包,接收包初始化

                if (jpeg_total_payload_len && (jpeg_finish == 0)) {
                    printf("\n [MSG] lose packet or disorder packet\n");
                }

                jpeg_old_frame_seq = cur_frame_seq;
                jpeg_total_payload_len = 0;
                jpeg_finish = 0;
                memset(info->buf, CHECK_CODE, JPG_MAX_SIZE);
            }

            if (bytecmp((unsigned char *)info->buf + frame_offset, CHECK_CODE, CHECK_CODE_NUM) != 0) {
                /* printf("\n%s %d->jpeg_repeat seq\n", __func__, __LINE__); */
                goto continue_deal;

            }

            memcpy(info->buf + frame_offset, (buf + position) + frame_head_size, slice_data_len);//对应数据放置到对应的位置
            jpeg_total_payload_len += slice_data_len;//累加总长度

            if (jpeg_total_payload_len == frame_size) { //如果数据量相等,说明帧数据接收完成
                info->buf_len = jpeg_total_payload_len;
                jpeg_finish = 1;

                return JPEG_DATA_TYPE;
            }

continue_deal:
            position += (frame_head_size + slice_data_len);
            break;

        case ONLINE_PING_TYPE:
            printf("-->receive ONLINE PING\r\n");
            if (checkPing) {
                sys_timer_modify(checkPing, LOSE_PING_TIME_CHECK);
            }
            break;

        case RESET_REQ_TYPE:
            printf("-->receive RESET REQ \r\n");
            old_frame_seq = 0;
            jpeg_old_frame_seq = 0;
            send_reset_rsp();
            if (!pingTime) {
                pingTime = sys_timer_add(NULL, send_online_ping, SEND_PING_TIME);
            } else {
                sys_timer_modify(pingTime, SEND_PING_TIME);
            }

            break;

        case RESET_RSP_TYPE:
            printf("-->receive RESET RSP \r\n");
            startJpeg = 1;

            if (!checkPing) {
                checkPing = sys_timer_add(NULL, lose_online_ping, LOSE_PING_TIME_CHECK);
            } else {
                sys_timer_modify(checkPing, LOSE_PING_TIME_CHECK);
            }

            break;

        default:
            printf("\n%s %d---type >> %d\n", __func__, __LINE__, frame_type);
            break;
        }
    } while (len > 0);

    return 0;
ERROR:

    return -1;

}

#ifdef RAW_STREAM_SENDER
int net_rt_send_frame(struct raw_stream_info *info, char *buffer, size_t len, u8 type)
{
    u16 payload_len = 0;
    u32 total_udp_send = 0;
    struct frm_head frame_head = {0};

    u32 remain_len = len;

    if (info == NULL || info->udp_send_buf == NULL) {
        printf("use net_rt_stream_open_pkg\n");
        return -1;
    }

    os_mutex_pend(&mutex2, 0);

    frame_head.offset = 0;
    frame_head.frm_sz = len;
    frame_head.type &= ~LAST_FREG_MAKER;
    frame_head.type |=  type;

    frame_head.seq = seq++;
    while (remain_len) {

        if (remain_len < MAX_PAYLOAD) {
            payload_len = remain_len;
            frame_head.type |= LAST_FREG_MAKER;
        } else {
            payload_len = MAX_PAYLOAD;
        }

        frame_head.payload_size = payload_len;
        total_udp_send = 0;
        memcpy(info->udp_send_buf + total_udp_send, &frame_head, sizeof(struct frm_head));
        total_udp_send += sizeof(struct frm_head);

        memcpy(info->udp_send_buf + total_udp_send, buffer + frame_head.offset, payload_len);

        total_udp_send += payload_len;

        if (total_udp_send <= UDP_SEND_SIZE_MAX) {
            wifi_raw_send(info->udp_send_buf, total_udp_send);
        } else {
            printf("total_udp_send over limit !!! \n");
            break;
        }

        remain_len -= payload_len;
        frame_head.offset += payload_len;
    }

    os_mutex_post(&mutex2);

    return len;
}
#endif

static void wifi_rx_cb(void *rxwi, struct ieee80211_frame *wh, void *data, u32 len, struct netif *netif)
{
#if 0
    static const u8 pkg_head_fill_magic[] = {
        /*dst*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,/*src*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,/*BSSID*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, /*Seq,Frag num*/0x88, 0x88,
    };
#endif

    static const u8 pkg_head_fill_magic[] = {
        /*dst*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,/*src*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,/*BSSID*/ 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    };

    if (len < 25 || memcmp(&((u8 *)data)[28], pkg_head_fill_magic, sizeof(pkg_head_fill_magic))) {
        return;
    }

    u8 *payload = &((u8 *)data)[48];
    u32 payload_len = len - 24;

    jpgImageDataRec(payload, payload_len);
}

static int wifi_raw_send(u8 *buf, int len)
{
    os_mutex_pend(&mutex1, 0);
    if (len > WIFI_TX_MAX_LEN) {
        return -1;
    }

    u8 *pos = wifi_get_payload_ptr();

    memcpy(pos, buf, len);

    wifi_send_data(len, WIFI_TXRATE_54M);

    os_mutex_post(&mutex1);
    return len;
}

void video_init_task(void *p)
{
    int user_video_rec0_open(void);
    int user_video_rec0_close(void);
    u32 user_video_frame_callback(void *data, u32 len, u8 type);

    os_time_dly(10);//延时10个os_tick，等待摄像头电源稳定
    user_video_rec0_open();//打开摄像头
}

int wifi_raw_init(void)
{
    printf(">>>>>>>wifi_raw_init<<<<<<<<<<");
    wifi_raw_on(0);
    wifi_set_rts_threshold(0xffff);//配置即使长包也不发送RTS
    wifi_set_channel(14);//配置WIFI RF 通信信道
    wifi_set_long_retry(10);
    wifi_set_short_retry(30);
    wifi_set_frame_cb(wifi_rx_cb, NULL); //注册接收802.11数据帧回调
    os_mutex_create(&mutex1);
    os_mutex_create(&mutex2);
    os_mutex_create(&mutex3);

#ifdef RAW_STREAM_RECEIVER
    sys_timer_add(NULL, fps_status, 1 * 1000);
    pingTime = sys_timer_add(NULL, send_online_ping, SEND_PING_TIME);
    void raw_stream_to_lcd(void);
    raw_stream_to_lcd();
#endif

#ifdef RAW_STREAM_SENDER
    thread_fork("video_init_task", 12, 1000, 0, NULL, video_init_task, NULL);
#endif
}

#ifdef RAW_STREAM_SENDER
static u8 sendBuf[4 * 1024];
void jpgImageDataSend(char *buffer, size_t len)
{
    if (startJpeg) {
        struct raw_stream_info stream_info;
        stream_info.udp_send_buf = sendBuf;
        net_rt_send_frame(&stream_info, buffer, len, JPEG_VIDEO_TYPE);
    } else {
        send_reset_req();
    }
}
#endif

static struct __JPG_INFO jpg_info = {
    .buf_len = sizeof(recBuf),
    .buf = recBuf,
};


void jpgImageDataRec(char *buf, int len)
{
    int ret;
    os_mutex_pend(&mutex3, 0);
    ret = get_jpeg_packet(buf, len, NULL, NULL, &jpg_info);
    if (JPEG_DATA_TYPE == ret) {
#ifdef RAW_STREAM_RECEIVER
        jpeg2yuv_jpeg_frame_write(jpg_info.buf, jpg_info.buf_len);
        fps_cnt++;
#endif
    } else if (PCM_DATA_TYPE == ret) {

    }
    os_mutex_post(&mutex3);
}
#endif


