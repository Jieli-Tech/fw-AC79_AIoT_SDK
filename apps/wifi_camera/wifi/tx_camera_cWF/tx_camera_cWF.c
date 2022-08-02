#include "app_config.h"

#ifdef CONFIG_MASS_PRODUCTION_ENABLE

#include "device/device.h"//u8
#include "system/includes.h"//GPIO
#include "sys_common.h"
#include "storage_device.h" //fs

#include "lcd_config.h"//LCD_h
#include "yuv_soft_scalling.h"//YUV420p_Soft_Scaling
#include "get_yuv_data.h"//get_yuv_init
#include "wifi/wifi_connect.h"

#include "sock_api/sock_api.h"
#include "lwip/sockets.h"//struct sockaddr_in
#include "server/rt_stream_pkg.h"   //head info .h
#include "server/video_server.h"//app_struct
#include "server/video_dec_server.h"//dec_struct
#include "server/ctp_server.h"//enum ctp_cli_msg_type

#define DEST_PORT 3333
#define CONN_PORT 2229
#define USER_IP_SERVER "192.168.1.2"

#define UDP_SEND_BUF_SIZE  (50 * 1472)

struct cWF_hdl {
    void *udp_sockfd;
    u8 *udp_send_buf;
    u32 cWF_ip;
    u8 udp_send_ready;
    u8 wifi_cont_flog;
};

static struct cWF_hdl *__this;

extern void set_video_rt_cb(u32(*cb)(void *, u8 *, u32), void *priv);
extern int user_video_rec0_open(void);
extern int user_video_rec0_close(void);

void cWF_get_ip_addr(u32 ipaddr)
{
    __this->cWF_ip = ipaddr;
}

void cWF_set_ip_send_ready(u8 ready)
{
    __this->udp_send_ready = ready;
}

void cWF_wifi_cont_flog(u8 cont_flog)
{
    __this->wifi_cont_flog = cont_flog;
}

int cWF_send_jpeg_by_udp(char *buffer, u32 len)
{
    struct sockaddr_in dest_addr = {0};

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = __this->cWF_ip;
    dest_addr.sin_addr.s_addr = inet_addr(inet_ntoa(dest_addr.sin_addr.s_addr));
    /*dest_addr.sin_addr.s_addr = inet_addr("192.168.95.1");*/
    dest_addr.sin_port = htons(CONN_PORT);

    os_time_dly(1);
    return sock_sendto(__this->udp_sockfd, buffer, len, 0, &dest_addr, sizeof(struct sockaddr_in));
}

void cWF_udp_init(void)
{
    printf(">>>>>>>>>>cWF_udp_init<<<<<<<<<<<");
    __this->udp_sockfd = sock_reg(AF_INET, SOCK_DGRAM, 0, NULL, NULL);
    if (__this->udp_sockfd == NULL) {
        printf("\n [ERROR] %s - %d\n", __func__, __LINE__);
    }

    struct sockaddr_in dest_addr = {0};
    /* 绑定数据通道 */
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_port = htons(2231);

    sock_bind(__this->udp_sockfd, (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
}

static void cWF_jpeg_cb(void *hdr, void *data, u32 len, u8 type)
{
    static u8 time = 0;
    u32 remain_len = len;
    if (__this->udp_send_ready) {
        remain_len = len;
        memcpy(__this->udp_send_buf,  data, len);
        cWF_send_jpeg_by_udp(__this->udp_send_buf, remain_len);
    }
}

void cWF_task(void *priv)
{
    __this = (struct cWF_hdl *)calloc(1, sizeof(struct cWF_hdl));
    if (__this == NULL) {
        printf(">>>>>>>>>>cWF_malloc _fail");
    }
    __this->udp_send_buf = (char *)malloc(UDP_SEND_BUF_SIZE);

    printf(">>>>>>>>>>>>>>video_start");
    __this->udp_send_ready = 0;
    os_time_dly(30);
    user_video_rec0_open();//打开摄像头
    set_video_rt_cb(cWF_jpeg_cb, NULL);
    while (!__this->wifi_cont_flog) {
        os_time_dly(30);
    }
    cWF_udp_init();
}

static int camera_cWF(void)
{
    puts("camera_cWF \n\n");
    return thread_fork("cWF_task", 11, 1024, 32, 0, cWF_task, NULL);
}

late_initcall(camera_cWF);

#endif
