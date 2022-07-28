#ifndef _DDNS_H
#define _DDNS_H

#if 0 /* UNUSED */
#include    <signal.h>
#include    <time.h>
#include    <sys/time.h>
#include    <features.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <sys/ioctl.h>
#include    <netinet/in.h>
#include    <net/if_arp.h>
#include    <errno.h>
#include    <fcntl.h>
#include    <ctype.h>
#include    <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif /* UNUSED */

#include "typedef.h"
#include  "sock_api.h"



#define DEBUG 0
#define DEBUG_VIEW 1

#define TRACE printf
#define ERROR printf

#define DDNS_REQ_CMD        0x81
#define DDNS_ACK_CMD        0x82

#define DDNS_SEND_ACTIVE    0x01
#define DDNS_SEND_HANDSHAKE 0x02
#define DDNS_SEND_VERIFY    0x03
#define DDNS_SEND_START     0x64
#define DDNS_SEND_ID        0x65
#define DDNS_SEND_HOSTNAME  0x66
#define DDNS_SEND_TCPPORT   0x67
#define DDNS_SEND_HTTPPORT  0x68
#define DDNS_SEND_FINISH    0x96
#define DDNS_SEND_QUIT      0xc8

#define DDNS_VERIFY_MODE    0x00
#define DDNS_UNKNOWN_ERROR  0x01
#define DDNS_NO_ID          0x01
#define DDNS_RECV_HANDSHAKE_OK  0x00
#define DDNS_RECV_FINISH        0x00
#define DDNS_RECV_DATA_OK       0X00
#define DDNS_ACTIVE_OK          0x00

#define DDNS_CONN       0x03        //connect server times
#define DATA_HEADER     0x20        //data header

#define DDNS_BUFSIZE    2048
#define DDNS_CFGSIZE    1024
#define DDNS_PORT       7070

#define TCPPORT         37777
#define HTTPPORT        80

#define DDNS_MAC_LEN    17

#define D_FREE(p) if(NULL != (p)) {free((p));(p)=NULL;}

/*Begin:chen_jianqun(10855)Task:8101,7939,06.08.03*/
#if defined(DVR_LB)
#define DDNS_CFG_FNAME "/mnt/mtd/Config/ddns-server"
#define DVR_CFG_FNAME "/mnt/mtd/Config/network"
#endif

#if defined(DVR_HB) || defined(DVR_GB)
#define DDNS_CFG_FNAME "/opt/sav/config/ddns-server"
#define DVR_CFG_FNAME "/opt/sav/config/network"
#endif

#define KEEPTIMES   600

#define DDNS_SLEEP_TIME 5
/*End:chen_jianqun(10855)Task:8101,7939*/

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned char BOOL;
typedef int ErrorCode_t;

struct xSoDat {
    BYTE Command;
    BYTE Reserv[3];     //reserved
    DWORD ExtLen;       //ExtDat length
    BYTE Params[24];    //parameter
    BYTE ExtDat[64];    //extern data
};

typedef struct tagDDNS_Client_Setup {
    int  cbSize;
    int  serverPort;
    WORD TCPPort;
    WORD HTTPPort;
    char *serverIp;
    char *username;
    char *password;
    char *ddnsName;
    char *deviceType;
    char *hardwareId;
    char *extInfo;
    int   extSize;
} DDNS_Client_Setup_t, *pDDNS_Client_Setup_t;

typedef struct tagDDNS_Query_Setup {
    int  cbSize;
    char *serverIp;
    int  serverPort;
    char *username;
    char *password;
} DDNS_Query_Setup_t, *pDDNS_Query_Setup_t;

typedef struct DDNS_Query_ExtInfo {
    char *deviceIp;
    char *extInfo;
    int   extSize;
} DDNS_Query_Info_t, *pDDNS_Query_Info_t;

enum ErrorCode_t {
    DH_OK = 0,                  //success
    DH_ERR_OUT_OF_MEMORY,       //memory malloc error
    DH_ERR_BAD_PARAM,           //parm error
    DDNS_ERR_NET_FAILUR,        //networking error
    DDNS_ERR_INVALID_CREDIT,    //user/passwd error
    DDNS_ERR_SESSION_TIMEOUT,   //timeout
    DH_ERR_UNKNOWN,
};

static unsigned char *ddns_buf = NULL;
static pDDNS_Client_Setup_t ddns_temp = NULL;
/*全局变量，ddns_temp配置参数，ddns_buf服务器端回复过来的数据缓冲区*/

/*
该函数为分配全局变量内存空间ddns_temp，ddns_buf
*/
int DDNS_InitBuff(void);

/*
该函数用于建立socket
*/
int DDNS_OpenNet(char *ddns_ip, WORD port);

/*
该函数用于获得DDNS客户端的MAC地址
*/
int DDNS_GetMac(char *getmacaddr);

/*
该函数用于设置DDNS注册所需要的信息，如用户名、密码、服务器的IP地址信息等。
在调用该函数后，pSetup中的信息必须在内部ddns_temp保存，否则上层应用释放pSetup相关
的内存后，会导致信息不可访问，最终导致程序出错。
*/
int DDNS_Client_Setup(struct tagDDNS_Client_Setup *pSetup);


/*begin by chen_jianqun*/
/*
该函数用于从设备底层读取设备参数文件填充全局变量ddns_temp，
以及将ddns-buf分配内存空间，注册DDNS信息
调用了DDNS_OpenNet，DDNS_GetMac,DDNS_Client_Setup函数
*/
int DDNS_GET_CFG(void);
/*end by chen_jianqun*/

/*
该函数用于向服务器注册DDNS信息，在调用该函数之前必须已经用
DDNS_GET_CFG()函数设置好相关信息。
*/
int DDNS_Client_Register(void);

/*
该函数用于与DDNS服务器保持激活状态，如果在一定的时间内设备没有与DDNS服务器通讯，
DDNS服务器会认为该注册信息已经过期，应用程序必须在DDNS_Client_Register()函数注
册成功后每隔10分钟调用一次该函数。在该函数的内部实现中根据服务器返回的Keep-Alive
时间信息，向服务器发送心跳信号，如果因为服务器崩溃等原因导致原有的注册信息失效，
该函数必须在内部负责向服务器重新注册。
*/
int DDNS_Client_KeepAlive(void);

/*
该函数为定时保活处理函数
*/
void DDNS_TimeKeephandle(void);

/*
在DDNS 服务器上进行注册直到成功为止
*/
void DDNS_ClientRegister_UntilSuccess(void);

/*
获取配置信息直到成功为止
*/
void DDNS_GetCFG_UntilSuccess(void);
#endif



