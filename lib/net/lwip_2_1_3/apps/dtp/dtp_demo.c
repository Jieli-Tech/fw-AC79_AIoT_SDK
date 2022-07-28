#include "os_api.h"
#include "os_compat.h"
#include "dtp.h"
#include "list.h"
#include <string.h>

#if 0

void *dtp_cli_hdl;

static int dtp_cli_callback_demo(void *hdl, enum dtp_cli_msg_type type, u8 *buf, u32 len, void *priv)
{
    int ret;
    switch (type) {
    case DTP_CLI_SET_BIND_PORT:
        puts("DTP_CLI_SET_BIND_PORT.\n");
        //返回需要绑定本地端口, 返回0使用系统分配端口,
        return 0;
    case DTP_CLI_SET_CONNECT_TO:
        puts("DTP_CLI_SET_CONNECT_TO.\n");
        return 15; //设置连接超时时间,0为默认时间(15S)
    case DTP_CLI_BIND_FAIL:
        puts("DTP_CLI_BIND_FAIL.\n");
        //绑定本地端口失败处理, 可以调用dtp_cli_set_local_port绑定其他端口,返回0重试.
        //返回-1 退出.
        return -1;
    case DTP_CLI_SEND_TO:
    //发送超时处理,用于检测线程退出, 返回-1关闭连接, 返回0重试
    case DTP_CLI_RECV_TO:
        //接收超时处理,用于检测线程退出, 返回-1关闭连接, 返回0重试
        return 0;
    case DTP_CLI_BEFORE_RECV:
        puts("DTP_CLI_BEFORE_RECV.\n");
        //检测到有服务器发送数据过来,还未接收, 可以在这里建立收数循环避免退出回调函数, 返回0成功, 返回-1关闭连接
        //可以调用 dtp_cli_set_recvbuf/dtp_cli_set_recv_wait_all 设置接收数据使用的buf.
        return 0;
    case DTP_CLI_RECV_DATA:
        //检测到有服务器发送数据过来,已经接收到buf, 返回0成功, 返回-1关闭连接
        printf("dtp_cli_callback_demo recv DATA!<0x%x>\n", len);
        hexdump(buf, len);
        return 0;
    case DTP_CLI_CONNECT_SUCC:
        //连接服务器成功
        puts("DTP_CLI_CONNECT_SUCC.\n");
        return 0;
        break;

    case DTP_CLI_CONNECT_FAIL:
        //连接服务器失败, 返回-1关闭连接,
        //返回0指定默认间隔3秒后重连
        //在此调用dtp_cli_set_connect_interval返回0指定一定间隔后重连
        puts("DTP_CLI_CONNECT FAIL\n");
        return 0;

    case DTP_CLI_SEND_DATA:
        puts("DTP_CLI_SEND_DATA.\n");
        //外面线程调用dtp_cli_send,才会会进来这里,然后可以在此发送数据
        break;
    case DTP_CLI_UNREG:
        //客户端主动注销连接, 用于检测线程退出.
        puts("DTP_CLI_UNREG.\n");
        *(int *)priv = 0;
        return 0;
        break;
    case DTP_CLI_SRV_DISCONNECT:
        //服务器断开, 返回-1关闭连接,
        //返回0指定默认间隔3秒后重连
        //在此调用dtp_cli_set_connect_interval返回0指定一定间隔后重连
        puts("DTP_CLI_SRV_DISCONNECT.\n");
//            *(int *)priv = 0;
        return 0;
        break;
    default:
        break;
    }
EXIT:

    return -1;
}

int dtp_cli_connect_demo(void)
{
    int ret;
    struct sockaddr_in dest_addr;

    inet_pton(AF_INET, "192.168.2.227", &dest_addr.sin_addr);
    dest_addr.sin_port = htons(1234);

    os_time_dly(300);
    puts("\n\n MARK0\n");

    dtp_cli_hdl = dtp_cli_reg(&dest_addr, dtp_cli_callback_demo, &dtp_cli_hdl, DTP_WRITE | DTP_READ | DTP_CONNECT_NON_BLOCK);

    puts("\n\n MARK1\n");

//    os_time_dly(1000);
//    dtp_cli_unreg(dtp_cli_hdl);

//     puts("\n\n MARK2\n");

    while (1) {
        os_time_dly(500);
        puts("send...\n");
        ret = dtp_cli_send_buf(dtp_cli_hdl, "$$$", 3, 0);
        if (ret <= 0) {
            puts("dtp_cli_send_buf ERR");
        }
    }
}

#endif
