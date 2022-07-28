#include "system/includes.h"
/*#include "os/os_compat.h"*/
#include "action.h"
#include "server/net_server.h"

/* #include "tutk_server.h" */
/* #include "ctp.h" */

#include "IOTCAPIs.h"
#include "AVAPIs.h"
#include "P2PCam/AVIOCTRLDEFs.h"
#include "P2PCam/AVFRAMEINFO.h"
#include "video_rt_tutk.h"
#include "mssdp/mssdp.h"
#include "app_config.h"

/* int tutk_platform_init(const char *username, const char *password) */
/* { */

/* return 0;	 */
/* } */
/* int tutk_noitfy_cli(void *hdl, char *msg, int msg_len) */
/* { */

/* return 0;	 */
/* } */
/* void *tutk_session_cli_get(void) */
/* { */
/* } */
int pthread_mutex_init(int *mutex, const int *attr)
{
    OS_MUTEX *pmutex;

    pmutex = (OS_MUTEX *)malloc(sizeof(OS_MUTEX));

    if (pmutex == NULL) {
        *mutex = 0;
        return -1;
    }

    os_mutex_create(pmutex);

    *mutex = (int)pmutex;

    return 0;
}

int pthread_mutex_destroy(int *mutex)
{
    if (mutex != NULL && ((void *)*mutex != NULL)) {
        os_mutex_del((OS_MUTEX *)(*mutex), OS_DEL_ALWAYS);
        free((void *)(*mutex));
        *mutex = 0;
    }

    return 0;
}

int pthread_mutex_lock(int *mutex)
{
    return os_mutex_pend((OS_MUTEX *)(*mutex), 0);
}

int pthread_mutex_unlock(int *mutex)
{
    return os_mutex_post((OS_MUTEX *)(*mutex));
}

int usleep(unsigned int t)
{
    os_time_dly(t / 10000);
    return 0;
}

#if 1
/***最大客户端数量***/
#define MAX_CLIENT_NUMBER	    1
#define CMD_CHANNEL   0x0
#define FILE_CHANNEL  0x1


struct tutk_info {
    struct list_head head;
    u8 cli_num;
    OS_MUTEX mutex;//用于保护客户端链表
    u8 kill_task;
    int login_pid;
    u32 inited;


    char username[64];
    char password[64];

};

/*
CZYUBD1CPRV4SN6GY1E1
EFYUBD1WPBV4BM6GY1MJ
FZYABH3WPXVMBMPGYHZJ
DBPUAN2WV5VMBMPGUHZJ
*/

/* #define TEST_UID  "DBPUAN2WV5VMBMPGUHZJ" */
/* #define TEST_UID  "CZYUBD1CPRV4SN6GY1E1" */
/* #define TEST_UID  "EFYUBD1WPBV4BM6GY1MJ" */
//mingxiao used
#define TEST_UID  "HH25H2GNP4DTECTK111A"



//外部引用
void __cmd_pro(void *cli, const char *content);



enum {
    TUTK_CMD_TIMEOUT = 0x1,
    TUTK_ERR = 0x2,
    TUTK_KILL_TASK   = 0xff,


};




u32 OSTimeGet()
{
    return OSGetTime();
}

static struct tutk_info tinfo;

static void tutk_login_info_cb(unsigned int nLoginInfo)
{
    if ((nLoginInfo & 0x04)) {
        printf("I can be connected via Internet\n");
    } else if ((nLoginInfo & 0x08)) {
        printf("I am be banned by IOTC Server because UID multi-login\n");
    }

    printf("loginInfo:0x%x\n", nLoginInfo);
}

static void tutk_login_task(void *arg)
{
    int cnt = 0;
    int ret = 0;
    puts("\n thread_Login \n");
    while (1) {
        if (tinfo.kill_task) {
            break;
        }
        ret = IOTC_Device_Login(TEST_UID, "aaaa", "12345678");
        if (ret == IOTC_ER_NoERROR) {
            printf("tutk platform login success !@!!\n");
            //os_time_dly(100);
        } else if (ret == IOTC_ER_LOGIN_ALREADY_CALLED) {
            /* printf("logined\n"); */
        }
        msleep(1000);

    }
}


int Handle_IOCTRL_Cmd(struct tutk_client_info *cli, int type, char *content)
{
    int ret = 0;
    switch (type) {
    case IOTYPE_USER_IPCAM_START:
        break;
    case IOTYPE_USER_IPCAM_STOP:
        break;
    case IOTYPE_USER_CTP_CMD_MSG:
        __cmd_pro(cli, content);

        break;
    case IOTYPE_USER_CTP_CMD_KEEP_ALIVE:
        puts("KKKKKK\n");
        /* if ((ret = avSendIOCtrl(cli->cmdindex, IOTYPE_USER_CTP_CMD_KEEP_ALIVE, "CTP_KEEP_ALIVE", strlen("CTP_KEEP_ALIVE"))) < 0) { */
        /* } */
        break;
    default:
        printf("\ntype = %d content = %s\n", type, content);
        break;
    }
    return 0;
exit:
    return -1;
}

static int AuthCallBackFn(char *viewAcc, char *viewPwd)
{
    if (strcmp(viewAcc, tinfo.username) == 0 && strcmp(viewPwd, tinfo.password) == 0) {
        printf("\n %s suc\n", __func__);
        return 1;
    }
    printf("\n tutk_srv.username = %s viewAcc = %s\n", tinfo.username, viewAcc);
    printf("\n tutk_srv.password = %s viewPwd = %s \n", tinfo.password, viewPwd);
    return 0;
}



static void thread_ForCMDServerStart(void *arg)
{
    int ret;
    int nResend = -1;
    unsigned int ioType;
    struct st_SInfo Sinfo = {0};


    struct tutk_client_info *cinfo = (struct tutk_client_info *)arg;



    cinfo->cmdindex = avServStart3(cinfo->session_id
                                   , AuthCallBackFn
                                   , 20
                                   , 0
                                   , CMD_CHANNEL
                                   , &nResend);
    if (cinfo->cmdindex < 0) {
        goto exit;
    }

    printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
    char *ioCtrlBuf = malloc(1024);
    printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
    if (!ioCtrlBuf) {
        goto exit2;
    }



    while (1) {

        if (cinfo->task_kill) {
            goto exit2;
        }
        /***接收IO命令，ioType为命令号，ioCtrlBuf为参数buf，1000表示1秒超时***/
        ret = avRecvIOCtrl(cinfo->cmdindex, &ioType, (char *)ioCtrlBuf, 1024, 10 * 1000);
        if (ret < 0) {
            if (ret == AV_ER_TIMEOUT) {

                printf("tutk recv timeout\n");
                os_taskq_post("tutk_state_task", 2, TUTK_CMD_TIMEOUT, (u32)cinfo);
                goto exit2;
            } else {
                os_taskq_post("tutk_state_task", 2, TUTK_ERR, (u32)cinfo);
                printf("avRecvIOCtrl ret = %d", ret);
                goto exit2;

            }
        }

        log_d("cinfo->session_id=%d\n", cinfo->session_id);
        Handle_IOCTRL_Cmd(cinfo, ioType, ioCtrlBuf);

    }


exit2:
    if (ioCtrlBuf != NULL) {
        free(ioCtrlBuf);
    }
    avServStop(cinfo->cmdindex);
exit:



    printf("\n thread_ForCMDServerStart exit\n");
}


#if 1
/* char tmpbuf[5*1024*1024]; */
#define PCM_TYPE_AUDIO      1
#define JPEG_TYPE_VIDEO     2
#define H264_TYPE_VIDEO     3
#define PREVIEW_TYPE        4
#define DATE_TIME_TYPE      5
#define MEDIA_INFO_TYPE     6
#define PLAY_OVER_TYPE      7
#define GPS_INFO_TYPE       8
#define NO_GPS_DATA_TYPE    9
#define G729_TYPE_AUDIO    10
#define META_DATA          11


struct frm_head {
    u8 type;
    u8 res;
#if 0
    u8 sample_seq;
#endif
    u16 payload_size;
    u32 seq;
    u32 frm_sz;
    u32 offset;
    u32 timestamp;
} __attribute__((packed));


static int do_file_server_event(int fileindex, FILE_t *file_info)
{
    /* struct FILE_t * file_info = (struct FILE_t *)arg; */
    /* printf("fileclient start\n"); */
    printf("path:%s\n", file_info->path);
    printf("offset:%d\n", file_info->offset);
    printf("size:%d\n", file_info->size);
    /* put_buf((void *)&file_info->size, 4); */
    int ret = 0;
    int ret1 = 0;
    struct frm_head f_head = {0};
    FILE *fp = fopen(file_info->path, "r");
    if (!fp) {
        printf("%s open fail\n", file_info->path);
        return -1;
    }
    if (!file_info->size) {
        fclose(fp);
        return 0;
    }

    fseek(fp, file_info->offset, SEEK_SET);

    printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
    char *buf = malloc(file_info->size);//这内存看来申请吧,不够就小点做点处理
    printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
    if (!buf) {
        printf("malloc fail\n");
        fclose(fp);
        return -1;
    }


    ret = fread(buf, file_info->size, 1, fp);
    if (ret <= 0) {
        printf("read all read all \n\n");
        f_head.type = PLAY_OVER_TYPE;
        do {
            /* printf("fileindex=%d  ret=%d\n", fileindex, ret); */
            memset(buf, 0xff, 32);
            ret1 = avSendFrameData(fileindex, buf, 32, &f_head, sizeof(f_head));
            printf("ret1=%d\n", ret1);
            if (ret1 < 0) {
                if (ret1 == AV_ER_EXCEED_MAX_SIZE) {
                    putchar('K');
                    os_time_dly(1);
                    continue;

                }
                log_e("\n\n\n tutk_send err ret=%d\n", ret);
                fclose(fp);
                free(buf);
                return ret;

            }

        } while (ret1 != AV_ER_NoERROR);

        printf("read all read all end end\n\n");


        fclose(fp);
        free(buf);
        return -1;
    }

    do {
        f_head.type = META_DATA;
        /* printf("fileindex=%d  ret=%d\n", fileindex, ret); */
        ret1 = avSendFrameData(fileindex, buf, ret, &f_head, sizeof(f_head));
        /* printf("ret1=%d\n", ret1); */
        if (ret1 < 0) {
            if (ret1 == AV_ER_EXCEED_MAX_SIZE) {
                putchar('K');
                os_time_dly(1);
                continue;

            }
            log_e("\n\n\n tutk_send err ret=%d\n", ret);
            fclose(fp);
            free(buf);
            return ret;

        }

    } while (ret1 != AV_ER_NoERROR);

    fclose(fp);
    free(buf);

    /* printf("file client send end\n"); */
    return 0;

}

static void thread_ForFileServerStart(void *arg)
{
    int ret;
    int nResend = -1;


    FILE_t file_info;
    unsigned int frmNo;
    int outBufSize = 0;
    int outFrmSize = 0;
    int outFrmInfoSize = 0;
    int count = 0;

    char file_info_buf[1024];
    int ioType = 0;



    struct tutk_client_info *cinfo = (struct tutk_client_info *)arg;


    //start file server channel
    cinfo->fileindex = avServStart3(cinfo->session_id
                                    , AuthCallBackFn
                                    , 5
                                    , 0
                                    , FILE_CHANNEL
                                    , &nResend);
    if (cinfo->fileindex < 0) {
        if (AV_ER_TIMEOUT == cinfo->fileindex) {
            printf("TTTTT\n");
            return ;
            /* continue; */
        }

        printf("cinfo->fileindex=%d\n", cinfo->fileindex);
        goto exit;
    }

    avServSetResendSize(cinfo->fileindex, 5 * 1024 * 1024);

    printf("nResend=%d\n", nResend);


    while (1) {
        if (cinfo->task_kill) {
            goto exit2;
        }

        ret = avRecvIOCtrl(cinfo->fileindex, &ioType, (char *)&file_info, sizeof(FILE_t), 10 * 1000);
        if (ret < 0) {
            if (ret == AV_ER_TIMEOUT) {

                printf("tutk recv timeout\n");
                continue;
            }
            printf("avRecvIOCtrl ret = %d", ret);
            goto exit2;
        }
        do_file_server_event(cinfo->fileindex, &file_info);

        /* avServStop(cinfo->fileindex); */

    }


exit2:
    avServStop(cinfo->fileindex);
exit:
    /* IOTC_Session_Close(cinfo->session_id); */

    printf("\n thread_ForFileServerStart exit\n");
}
#endif








static void tutk_do_listen(void *arg)
{
    int ret;
    int SID;
    struct st_SInfo Sinfo;

    while (1) {
        SID = IOTC_Listen(0);  //0表示不超时
        if (SID < 0) {
            if (SID == IOTC_ER_EXCEED_MAX_SESSION) {
                os_time_dly(100);
                continue;
            }
            printf("SID=%d\n", SID);
            break;
        }



        if (IOTC_Session_Check(SID, &Sinfo) == IOTC_ER_NoERROR) {
            char *mode[3] = {"P2P", "RLY", "LAN"};
            // print session information(not a must)
//		if( isdigit( Sinfo.RemoteIP[0] ))
            /* printf("Client is from[IP:%s, Port:%d] Mode[%s] VPG[%d:%d:%d] VER[%X] NAT[%d] AES[%d]\n", Sinfo.RemoteIP, Sinfo.RemotePort, mode[(int)Sinfo.Mode], Sinfo.VID, Sinfo.PID, Sinfo.GID, Sinfo.IOTCVersion, Sinfo.NatType, Sinfo.isSecure); */
        }

        printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
        struct tutk_client_info *cinfo = calloc(1, sizeof(struct tutk_client_info));
        printf("\n [ERROR] %s -yuyu %d\n", __FUNCTION__, __LINE__);
        if (!cinfo) {
            printf("tutk malloc fail\n");
            IOTC_Session_Close(SID);
            continue;

        }


        cinfo->session_id = SID;

        os_mutex_pend(&tinfo.mutex, 0);
        list_add_tail(&cinfo->entry, &tinfo.head);

        os_mutex_post(&tinfo.mutex);
        char thread_name[64];
        static u32 count;
#if 1
        sprintf(thread_name, "CmdServerStart_%X", count++);
        printf("\n thread_name = %s \n", thread_name);
        ret = thread_fork(thread_name, 28, 0x1000, 64, &cinfo->cmd_task_pid, thread_ForCMDServerStart, (void *)cinfo);
        if (ret != OS_NO_ERR) {
            printf("\n create %s err ,ret = %d", thread_name, ret);
        }
#endif


#if 1
        //文件系统子线程
        sprintf(thread_name, "FileServerStart_%X", count++);
        printf("\n thread_name = %s \n", thread_name);
        ret = thread_fork(thread_name, 10, 0x1000, 64, &cinfo->file_task_pid, thread_ForFileServerStart, (void *)cinfo);
        if (ret != OS_NO_ERR) {
            printf("\n create %s err ,ret = %d", thread_name, ret);
        }

#endif

    }
    printf("end tutk_do_listen_task \n\n");
}


void *tutk_session_cli_get(void)
{

    struct tutk_client_info *cinfo = NULL;

    if (list_empty(&tinfo.head)) {

        return NULL;
    }

    cinfo = list_first_entry(&tinfo.head, struct tutk_client_info, entry);

    return (void *)cinfo;

}


static int tutk_session_cli_exit(int arg)
{

    struct tutk_client_info *cinfo = (struct tutk_client_info *)arg;
    os_mutex_pend(&tinfo.mutex, 0);
    list_del(&cinfo->entry);
    os_mutex_post(&tinfo.mutex);

    cinfo->task_kill = 1;
    //关闭所有应用

    /* video0_tutk_stop(cinfo); */
    /* tutk_speak_uninit(cinfo); */

    thread_kill(&cinfo->file_task_pid, KILL_WAIT);
    thread_kill(&cinfo->cmd_task_pid, KILL_WAIT);
    cinfo->task_kill = 0;

    IOTC_Session_Close(cinfo->session_id);

    free(cinfo);

    return 0;

}

int tutk_noitfy_cli(void *hdl, char *msg, int msg_len)
{
    struct tutk_client_info *info = (struct tutk_client_info *)hdl;
    struct tutk_client_info *p = NULL;
    int ret = 0;

    if (!tinfo.inited) {
        return -1;
    }

    if (info) {
        if ((ret = avSendIOCtrl(info->cmdindex, IOTYPE_USER_CTP_CMD_MSG, msg, msg_len)) < 0) {
            printf("start_ipcam_stream failed[%d]\n", ret);
            return -1;
        }
        return 0;

    }



    os_mutex_pend(&tinfo.mutex, 0);
    if (list_empty(&tinfo.head)) {
        os_mutex_post(&tinfo.mutex);
        return -1;
    }

    list_for_each_entry(p, &tinfo.head, entry) {
        if ((ret = avSendIOCtrl(p->cmdindex, IOTYPE_USER_CTP_CMD_MSG, msg, msg_len)) < 0) {
            printf("start_ipcam_stream failed[%d]\n", ret);
        }
    }

    os_mutex_post(&tinfo.mutex);

    return 0;


}



static void tutk_state_task(void *arg)
{
    int res;
    int msg[8];

    while (1) {
        res = os_task_pend("taskq", msg, ARRAY_SIZE(msg));

        switch (res) {
        case OS_TASKQ:
            switch (msg[1]) {
            case TUTK_CMD_TIMEOUT:

                tutk_session_cli_exit(msg[2]);
                break;
            case TUTK_ERR:

                tutk_session_cli_exit(msg[2]);
                break;



            case TUTK_KILL_TASK:
                printf("end tutk_state_task \n\n");
                return;
            default:
                break;
            }
            break;
        case OS_TIMER:
            break;
        case OS_TIMEOUT:
            break;
        }
    }


}



#define BIND_TOKEN  "174f9d4e-b5bf-4194-8158-f00bf00ea619"
#define DEV_INFO_JSON "{\"device_uuid\":\"%s\",\"bind_token\":\"%s\"}"
static char buffer[1024];
static char *dev_info()
{
    /*后面改成都VM*/
    sprintf(buffer, DEV_INFO_JSON, TEST_UID, BIND_TOKEN);
    return buffer;
}
static void network_ssdp_cb(u32 dest_ipaddr, enum mssdp_recv_msg_type type, char *buf, void *priv)
{
    if (type == MSSDP_SEARCH_MSG) {
        printf("ssdp client[0x%x] search, %s\n", dest_ipaddr, buf);
    }
}






int tutk_platform_init(const char *username, const char *password)
{
    int ret;
    strcpy(tinfo.username, username);
    strcpy(tinfo.password, password);

    IOTC_Set_Max_Session_Number(MAX_CLIENT_NUMBER);

    ret = IOTC_Initialize2(55556);
    if (ret != IOTC_ER_NoERROR) {
        printf("\n IOTC_Initialize2(), ret=[%d]\n", ret);
        goto exit;
    }

    ret = os_mutex_create(&tinfo.mutex);
    if (ret != OS_NO_ERR) {
        printf("\n create list mutex err\n");
        goto exit;
    }


    unsigned int iotcVer;
    IOTC_Get_Version(&iotcVer);
    int avVer = avGetAVApiVer();
    unsigned char *p = (unsigned char *)&iotcVer;
    unsigned char *p2 = (unsigned char *)&avVer;
    char szIOTCVer[16], szAVVer[16];
    sprintf(szIOTCVer, "%d.%d.%d.%d", p[3], p[2], p[1], p[0]);
    sprintf(szAVVer, "%d.%d.%d.%d", p2[3], p2[2], p2[1], p2[0]);
    printf("IOTCAPI version[%s] AVAPI version[%s]\n", szIOTCVer, szAVVer);

    printf("UUID:%s\n", TEST_UID);


    IOTC_Get_Login_Info_ByCallBackFn(tutk_login_info_cb);

    avInitialize(MAX_CLIENT_NUMBER * 4);

    INIT_LIST_HEAD(&tinfo.head);

    ret = thread_fork("tutk_login_task", 10, 0x2000, 0, &tinfo.login_pid, tutk_login_task, NULL);
    if (ret != OS_NO_ERR) {
        printf("\n create thread_Login err , ret = %d \n", ret);
    }


    ret = thread_fork("tutk_do_listen_task", 10, 0x2000, 0, 0, tutk_do_listen, NULL);
    if (ret != OS_NO_ERR) {
        printf("\n create thread_listen err , ret = %d \n", ret);
    }


    ret = thread_fork("tutk_state_task", 25, 2048, 64, 0, tutk_state_task, NULL);
    if (ret != OS_NO_ERR) {
        printf("\n create tutk_state_task err , ret = %d \n", ret);
    }



    mssdp_init("MSSDP_SEARCH ", "MSSDP_NOTIFY ", "MSSDP_REGISTER ", 3889, network_ssdp_cb, NULL);
    mssdp_set_notify_msg(dev_info(), 60);



    tinfo.inited = 1;






    return 0;
exit:
    return -1;
}

int tutk_platform_uninit()
{

    struct tutk_client_info *p = NULL;

    if (!tinfo.inited) {
        return 0;
    }

    if (!list_empty(&tinfo.head)) {
        list_for_each_entry(p, &tinfo.head, entry) {
            tutk_session_cli_exit((int)p);
        }
    }

    mssdp_uninit();

    IOTC_Listen_Exit();


    os_taskq_post("tutk_state_task", 1, TUTK_KILL_TASK);


    tinfo.kill_task = 1;

    thread_kill(&tinfo.login_pid, KILL_WAIT);

    tinfo.kill_task  = 0;
    /* DeInitAVInfo(); */
    /* DeInitAVInfo(); */
    avDeInitialize();
    IOTC_DeInitialize();


    os_mutex_del(&tinfo.mutex, 0);
    tinfo.inited = 0;

    return 0;
}
#endif


