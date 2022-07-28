/*
 * dahua DDNS client
 */

#include "DDNS.h"
#include "lwip.h"
#include "lwip/sys.h"
/*
*functin :  init global var
*parameter: -void
*description:
*success return 0,fill ddns-temp ,ddns-buf.
*fail return 1
*/
int DDNS_InitBuff()
{
    int status = 0;

    ddns_buf = (unsigned char *)malloc(DDNS_BUFSIZE);
    if (NULL == ddns_buf) {
        status = 1;
        printf("DDNS Client initialized failed, status = %d\n", status);
        return DH_ERR_OUT_OF_MEMORY;
    }
    memset(ddns_buf, 0, DDNS_BUFSIZE);

    ddns_temp = (struct tagDDNS_Client_Setup *)malloc(DDNS_CFGSIZE);
    if (NULL == ddns_temp) {
        status = 1;
        printf("DDNS Client initialized failed, status = %d\n", status);
        D_FREE(ddns_buf);
        return DH_ERR_OUT_OF_MEMORY;
    }
    memset(ddns_temp, 0, DDNS_CFGSIZE);

    return DH_OK;
}

/*
*functin :  get dvr mac
*parameter: -
*description:
*success return 0,fill getmacaddr.
*fail return -1
*/
int DDNS_GetMac(char *getmacaddr)
{
    int temp = 0;
    int j = 0;
    int i = 0;
    int k = 0;
    char macaddr[6];
    char mac[16] = {0};
    int ipaddr;

    get_netif_macaddr_and_ipaddr(&ipaddr, macaddr, 1);

    for (i = 0; i < 6; i ++) {
        if (0 == macaddr[i]) {
            mac[k] = '0';
            k++;
            mac[k] = '0';
            k++;
            mac[k] = ':';
            k++;
        } else {
            for (j = 0; j < 2; j++) {
                temp = (macaddr[i] >> ((1 - j) * 4)) & 0x0f;
                if ((temp >= 0) && (temp <= 9)) {
                    mac[k] = temp + 0x30;
                    k++;
                } else if ((temp >= 10) && (temp <= 15)) {
                    mac[k] = temp + 0x57;
                    k++;
                } else {
                    return -1;
                }
            }
            if (5 == i) {
                break;
            }
            mac[k] = ':';
            k++;
        }
    }
    mac[k] = '\0';

    memset(getmacaddr, 0, sizeof(getmacaddr));
    memcpy(getmacaddr, mac, strlen(mac));
    return 0;
}

/*
*functin :  setup socket & connect
*parameter: -
*description: open networking
*success return sockd
*fail return -1
*/
int DDNS_OpenNet(char *ddns_ip, WORD port)
{
    struct sockaddr_in addr;
    int sockd = -1;
    int iTemp = -1;

    sockd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockd) {
        ERROR("Error:unable to get network socket!\n");
        return -1;
    }

    memset((char *)&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ddns_ip);  //char to int

    iTemp = connect(sockd, (struct sockaddr *)&addr, sizeof(addr));
    if (0 != iTemp) {
        ERROR("Error:unable to connect!\n");
        close(sockd);
        return -1;
    }

    return sockd;
}

/*
 *functin :  set DDNSClient info to memory
 *parameter: struct  tagDDNS_Client_Setup *pSetup
 *description:
 *success return DH_OK
 *malloc fail return DH_ERR_OUT_OF_MEMORY
 */
ErrorCode_t DDNS_Client_Setup(struct  tagDDNS_Client_Setup *pSetup)
{
    int tempLen = 0;

    if (NULL == pSetup) {
        ERROR("pSetup NULL\n");
        return DH_ERR_BAD_PARAM;
    }

    ddns_temp->serverPort = pSetup->serverPort;

    ddns_temp->TCPPort = pSetup->TCPPort;
    ddns_temp->HTTPPort = pSetup->HTTPPort;

    if (NULL != pSetup->serverIp) {
        ddns_temp->serverIp = (char *)(ddns_temp + 4);
        memcpy(ddns_temp->serverIp, pSetup->serverIp, strlen(pSetup->serverIp));
        tempLen = 4 + strlen(pSetup->serverIp);
    }
    if (NULL != pSetup->username) {
        ddns_temp->username = (char *)(ddns_temp + tempLen);
        memcpy(ddns_temp->username, pSetup->username, strlen(pSetup->username));
        tempLen += strlen(pSetup->username);
    }
    if (NULL != pSetup->password) {
        ddns_temp->password = (char *)(ddns_temp + tempLen);
        memcpy(ddns_temp->password, pSetup->password, strlen(pSetup->password));
        tempLen += strlen(pSetup->password);
    }
    if (NULL != pSetup->ddnsName) {
        ddns_temp->ddnsName = (char *)(ddns_temp + tempLen);
        memcpy(ddns_temp->ddnsName, pSetup->ddnsName, strlen(pSetup->ddnsName));
        tempLen += strlen(pSetup->ddnsName);
    }
    if (NULL != pSetup->deviceType) {
        ddns_temp->deviceType = (char *)(ddns_temp + tempLen);
        memcpy(ddns_temp->deviceType, pSetup->deviceType, strlen(pSetup->deviceType));
        tempLen += strlen(pSetup->deviceType);
    }
    if (NULL != pSetup->hardwareId) {
        ddns_temp->hardwareId = (char *)(ddns_temp + tempLen);
        memcpy(ddns_temp->hardwareId, pSetup->hardwareId, strlen(pSetup->hardwareId));
        tempLen += strlen(pSetup->hardwareId);
    }
    if (0 != pSetup->extSize) {
        ddns_temp->extInfo = (char *)(ddns_temp + tempLen);
        memcpy(ddns_temp->extInfo, pSetup->extInfo, pSetup->extSize);
    }

    return DH_OK;
}

/*Begin:chen_jianqun(10855)Task:8101,7939,06.08.03*/
/*
 *functin :  set DDNSClient info to memory
 *parameter: struct  tagDDNS_Client_Setup *pSetup
 *description: set ddns_temp value and malloc memory for ddns_buf
 *use function :DDNS_GetMac,DDNS_Client_Setup
 *success return DH_OK
 *fail return -1
 */
ErrorCode_t DDNS_GET_CFG()
{
    DDNS_Client_Setup_t DDNSClient_Setup;

    int status = 0;
    int serverport = 6060;
    int enable = 0;

    char ddns_mac[32] = {0};
    char ddns_name[32] = "my.ddns.name.net";
    char serverip[32] = "202.118.12.105";
    char cTemp[8] = {0};

    int httpport = 0;
    int tcpport = 0;

//    FILE *fp = NULL;

#ifdef __uClinux_
    int *ret = NULL;
    if ((getbenv("HWADDR0")) == NULL) {
        fprintf(stderr, "Cannot find in armboot\n");
        return -1;
    }
    strncpy(ddns_mac, ret, 17);
#endif

    status = DDNS_GetMac(ddns_mac);
    if (status != 0) {
        return -1;
    }
#if 0   /*unused*/
    fp = fopen(DDNS_CFG_FNAME, "rb");
    if (NULL == fp) {
        printf("Open file fail: %s\n", DDNS_CFG_FNAME);
        return -1;
    }

    fgets(cTemp, sizeof(cTemp), fp);
    cTemp[strlen(cTemp) - 1] = 0;
    enable = atoi(cTemp);

    if (0 == enable) {
        fclose(fp);
        return -1;
    }

    memset(serverip, 0, sizeof(serverip));
    fgets(serverip, sizeof(serverip), fp);
    serverip[strlen(serverip) - 1] = 0;

    memset(ddns_name, 0, sizeof(ddns_name));
    fgets(ddns_name, sizeof(ddns_name), fp);
    ddns_name[strlen(ddns_name) - 1] = 0;

    fgets(cTemp, sizeof(cTemp), fp);
    serverport = atoi(cTemp);

    fclose(fp);

    fp = fopen(DVR_CFG_FNAME, "rb");
    if (NULL == fp) {
        TRACE("Open file fail: %s\n", DVR_CFG_FNAME);
        return -1;
    }

    int iRet = 0;
    char srcstr[50] = {0};

    char *strtmp = NULL;
    char *dststr = NULL;

    char flag = 1;
    int findnumber = 0;
    char *delim = "= ";

    memset(srcstr, 0, sizeof(srcstr));

    while (flag) {
        fgets(srcstr, sizeof(srcstr), fp);
        srcstr[strlen(srcstr) - 1] = 0;

        strtmp = strtok(srcstr, delim);

        iRet = strcmp(strtmp, "HTTPPORT");
        if (iRet == 0) {
            dststr = strtok(NULL, delim);
            httpport = atoi(dststr);
            findnumber++;
        }

        iRet = strcmp(strtmp, "TCPPORT");
        if (iRet == 0) {
            dststr = strtok(NULL, delim);
            tcpport = atoi(dststr);
            findnumber++;
        }

        if (findnumber == 2) {
            flag = 0;
        }
    }

    fclose(fp);
#endif
    memset((pDDNS_Client_Setup_t)&DDNSClient_Setup, 0, sizeof(DDNS_Client_Setup_t));
    DDNSClient_Setup.cbSize = sizeof(DDNS_Client_Setup_t);
    DDNSClient_Setup.hardwareId = ddns_mac;
    DDNSClient_Setup.ddnsName = ddns_name;

    if (httpport == 0 || tcpport == 0) {
        DDNSClient_Setup.TCPPort = TCPPORT;
        DDNSClient_Setup.HTTPPort = HTTPPORT;
    } else {
        DDNSClient_Setup.TCPPort = tcpport;
        DDNSClient_Setup.HTTPPort = httpport;
    }

    DDNSClient_Setup.serverIp = serverip;
    DDNSClient_Setup.serverPort = serverport;

    DDNSClient_Setup.username = NULL;
    DDNSClient_Setup.password = NULL;

    DDNSClient_Setup.deviceType = NULL;
    DDNSClient_Setup.extSize = 0;

    status = DDNS_Client_Setup(&DDNSClient_Setup);

    if (DH_OK != status) {
        printf("DDNS Client set ddns_temp init value failed, status = %d\n", status);
        D_FREE(ddns_temp);
        return -1;
    }

    if (1) {
        printf("HTTPPort:%d\n", ddns_temp->HTTPPort);
        printf("TCPPort:%d\n", ddns_temp->TCPPort);
        printf("ddnsName:%s\n", ddns_temp->ddnsName);
        printf("serverIp:%s\n", ddns_temp->serverIp);
        printf("serverPort:%d\n", ddns_temp->serverPort);
    }

    return DH_OK;
}
/*End:chen_jianqun(10855)Task:8101,7939*/

/*
 *functin :  register info to DDNS server
 *parameter: -
 *description: register info to server
 *use fuction:DDNS_OpenNet
 *success return DH_OK
 *fail return DDNS_ERR_NET_FAILUR
 */
ErrorCode_t DDNS_Client_Register()
{
    struct xSoDat ddns_sodp;
    int n = 0;
    int i = 0;
    int sockd = -1;

    /*connect DDNS server*/
    for (i = 0; i < DDNS_CONN; i++) {
        sockd = DDNS_OpenNet(ddns_temp->serverIp, ddns_temp->serverPort);
        if (0 < sockd) {
            break;
        }
        sleep(DDNS_SLEEP_TIME);
    }

    if (DDNS_CONN == i) {
        ERROR("Connect failed!\n");
        return DDNS_ERR_NET_FAILUR;
    }

    /* send hangshake */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = 0;
    ddns_sodp.Params[0] = DDNS_SEND_HANDSHAKE;
    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER + ddns_sodp.ExtLen, 0);
    if (DATA_HEADER + ddns_sodp.ExtLen != n) {
        ERROR("Send packet error!\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* responsion handshake */
    memset(ddns_buf, 0, DDNS_BUFSIZE);
    n = recv(sockd, ddns_buf, DDNS_BUFSIZE, 0);
    if ((0 > n) || (DDNS_BUFSIZE <= n)) {
        ERROR("Recv handshake error.\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    memcpy((struct xSoDat *)&ddns_sodp, (struct xSoDat *)ddns_buf, n);
    if ((DDNS_ACK_CMD == (ddns_sodp.Command & 0xff)) &&
        (DDNS_SEND_HANDSHAKE == (ddns_sodp.Params[0] & 0xff)) &&
        (DDNS_RECV_HANDSHAKE_OK == (ddns_sodp.Params[4] & 0xff))) {
#if DEBUG
        TRACE("Handshake success!\n\n");
#endif
    } else {
        ERROR("Hangshake fail!\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* send verify */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = 0;
    ddns_sodp.Params[0] = DDNS_SEND_VERIFY;
    ddns_sodp.Params[4] = DDNS_VERIFY_MODE;
    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER + ddns_sodp.ExtLen, 0);
    if (DATA_HEADER + ddns_sodp.ExtLen != n) {
        ERROR("Send error\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* recv verify */
    memset(ddns_buf, 0, DDNS_BUFSIZE);
    n = recv(sockd, ddns_buf, DDNS_BUFSIZE, 0);
    if ((0 > n) || (DDNS_BUFSIZE <= n)) {
        ERROR("Recv verify error.\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    memcpy((struct xSoDat *)&ddns_sodp, (struct xSoDat *)ddns_buf, n);
    if ((DDNS_ACK_CMD == (ddns_sodp.Command & 0xff)) &&
        (DDNS_SEND_VERIFY == (ddns_sodp.Params[0] & 0xff)) &&
        (DDNS_RECV_HANDSHAKE_OK == (ddns_sodp.Params[4] & 0xff))) {
#if DEBUG
        TRACE("Verify ok!\n\n");
#endif
    } else {
        ERROR("Verify fail!\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* start send info */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = 0;
    ddns_sodp.Params[0] = DDNS_SEND_START;
    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER + ddns_sodp.ExtLen, 0);

    if (DATA_HEADER + ddns_sodp.ExtLen != n) {
        ERROR("Start info packet error!\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* send ID / MAC */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = strlen(ddns_temp->hardwareId);
    ddns_sodp.Params[0] = DDNS_SEND_ID;
    memcpy((char *)&ddns_sodp.ExtDat, ddns_temp->hardwareId, ddns_sodp.ExtLen);
#if DEBUG
    TRACE("mac = %s,len = %ld\n", ddns_sodp.ExtDat, ddns_sodp.ExtLen);
#endif

    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER + ddns_sodp.ExtLen, 0);
    if (DATA_HEADER + ddns_sodp.ExtLen != n) {
        ERROR("Send ID packet error!\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* send HostName */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = strlen(ddns_temp->ddnsName);
    ddns_sodp.Params[0] = DDNS_SEND_HOSTNAME;
    memcpy((char *)&ddns_sodp.ExtDat, ddns_temp->ddnsName, ddns_sodp.ExtLen);
#if DEBUG
    TRACE("HostName = %s,len = %ld\n", ddns_sodp.ExtDat, ddns_sodp.ExtLen);
#endif
    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER + ddns_sodp.ExtLen, 0);
    if (DATA_HEADER + ddns_sodp.ExtLen != n) {
        ERROR("Send host name packet error!\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* send TCPPort */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = sizeof(ddns_temp->TCPPort);
    ddns_sodp.Params[0] = DDNS_SEND_TCPPORT;

    ddns_sodp.ExtDat[1] = ((ddns_temp->TCPPort) & 0xff00) >> 8;
    ddns_sodp.ExtDat[0] = ((ddns_temp->TCPPort) & 0xff);
    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER + ddns_sodp.ExtLen, 0);
    if (DATA_HEADER + ddns_sodp.ExtLen != n) {
        ERROR("Send TCP port packet error!\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* send WEBPort */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = sizeof(ddns_temp->HTTPPort);
    ddns_sodp.Params[0] = DDNS_SEND_HTTPPORT;

    ddns_sodp.ExtDat[1] = ((ddns_temp->HTTPPort) & 0xff00) >> 8;
    ddns_sodp.ExtDat[0] = ((ddns_temp->HTTPPort) & 0xff);
    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER + ddns_sodp.ExtLen, 0);
    if (DATA_HEADER + ddns_sodp.ExtLen != n) {
        ERROR("Send web port packet error!\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* send finish symbol */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = 0;
    ddns_sodp.Params[0] = DDNS_SEND_FINISH;
    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER + ddns_sodp.ExtLen, 0);
    if (DATA_HEADER + ddns_sodp.ExtLen != n) {
        ERROR("Send finish symbol packet error!\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* recv data ack */
    memset(ddns_buf, 0, DDNS_BUFSIZE);
    n = recv(sockd, ddns_buf, DDNS_BUFSIZE, 0);
    if ((0 > n) || (n >= DDNS_BUFSIZE)) {
        ERROR("Receive data ack error.\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }
    memcpy((struct xSoDat *)&ddns_sodp, (struct xSoDat *)ddns_buf, n);

#if DEBUG
    for (i = 0; i < n; i++) {
        if (i % 6 == 0) {
            TRACE("\n");
        }
        TRACE("buf[%d] = %x", i, ddns_buf[i]);
    }
#endif

    if ((DDNS_ACK_CMD == (ddns_sodp.Command & 0xff)) &&
        (DDNS_SEND_FINISH == (ddns_sodp.Params[0] & 0xff)) &&
        (DDNS_RECV_DATA_OK == (ddns_sodp.Params[4] & 0xff))) {
#if DEBUG
        TRACE("Add DDNS node successed!\n\n");
#endif
    } else {
        if (DDNS_UNKNOWN_ERROR == (ddns_sodp.Params[4] & 0xff)) {
            ERROR("Unknown error!\n");
            close(sockd);
            return DDNS_ERR_NET_FAILUR;
        } else {
            ERROR("Add DDNS node failed!\n");
            close(sockd);
            return DDNS_ERR_NET_FAILUR;
        }
    }

    /* quit socket */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = 0;
    ddns_sodp.Params[0] = DDNS_SEND_QUIT;
    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER, 0);
    if (DATA_HEADER != n) {
        ERROR("Send error\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    close(sockd);
    return DH_OK;
}

/*
 *functin :  keeplive info to DDNS server
 *parameter: -
 *description: keeplive info to server
 *use fuction:DDNS_OpenNet,DDNS_GET_CFG,DDNS_Client_Register
 *success return DH_OK
 *fail return DDNS_ERR_NET_FAILUR
 *NO ID return DDNS_NO_ID
 */
ErrorCode_t DDNS_Client_KeepAlive()
{
    struct xSoDat ddns_sodp;

    int n = 0;
    int i = 0;
    int sockd = -1;

    /*
    获取配置信息直到成功为止
    */
    DDNS_GetCFG_UntilSuccess();

    /* connect DDNS server */
    for (i = 0; i < DDNS_CONN; i++) {
        sockd = DDNS_OpenNet(ddns_temp->serverIp, ddns_temp->serverPort);
        if (0 < sockd) {
            break;
        }
        sleep(DDNS_SLEEP_TIME); /*wait 5s & reconnect*/
    }

    if (i == DDNS_CONN) {
        return DDNS_ERR_NET_FAILUR;
    }
    /*  send active */
    memset((char *)&ddns_sodp, 0, sizeof(ddns_sodp));
    ddns_sodp.Command = DDNS_REQ_CMD;
    ddns_sodp.ExtLen = strlen(ddns_temp->hardwareId);
    ddns_sodp.Params[0] = DDNS_SEND_ACTIVE;
    memcpy((char *)&ddns_sodp.ExtDat, ddns_temp->hardwareId, ddns_sodp.ExtLen);

    n = send(sockd, (char *)&ddns_sodp, DATA_HEADER + ddns_sodp.ExtLen, 0);

    if (DATA_HEADER + ddns_sodp.ExtLen != n) {
        ERROR("Send keepActive packet error\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }

    /* recv ack */
    memset(ddns_buf, 0, DDNS_BUFSIZE);
    n = recv(sockd, ddns_buf, DDNS_BUFSIZE, 0);

    if ((0 > n) || (DDNS_BUFSIZE < n)) {
        ERROR("Receive active ack error.\n");
        close(sockd);
        return DDNS_ERR_NET_FAILUR;
    }


#if DEBUG
    for (i = 0; i < n; i++) {
        if (i % 6 == 0) {
            TRACE("\n");
        }
        TRACE("buf[%d] = %x", i, ddns_buf[i]);
    }
#endif
    memcpy((struct xSoDat *)&ddns_sodp, (struct xSoDat *)ddns_buf, n);

    if ((DDNS_ACK_CMD == (ddns_sodp.Command & 0xff)) &&
        (DDNS_SEND_ACTIVE == (ddns_sodp.Params[0] & 0xff)) &&
        (DDNS_ACTIVE_OK == (ddns_sodp.Params[4] & 0xff))) {
#if DEBUG
        TRACE("Add DDNS node successed!\n\n");
#endif
    } else {
        if (DDNS_NO_ID == (ddns_sodp.Params[4] & 0xff)) {
            ERROR("Unknown node ID!\n");
            close(sockd);

            /*
            在DDNS 服务器上进行注册直到成功为止
            */
            DDNS_ClientRegister_UntilSuccess();

            return DH_OK;
        } else {
            ERROR("Add DDNS node failed!\n");
            close(sockd);
            return DDNS_ERR_NET_FAILUR;
        }
    }

    close(sockd);
    return DH_OK;
}

/*
获取配置信息直到成功为止
*/
void DDNS_GetCFG_UntilSuccess(void)
{
    int iErrCode = DH_OK;

    /* 获取配置信息 */
    while (1) {
        iErrCode = DDNS_GET_CFG();
        if (DH_OK != iErrCode) {
            printf("DDNS Client Get confige failed, status = %d\n", iErrCode);
            sleep(DDNS_SLEEP_TIME * DDNS_SLEEP_TIME);
        } else {
            printf("DDNS Client Get confige successed, status = %d\n", iErrCode);
            return;
        }
    }

    return;
}

/*
在DDNS 服务器上进行注册直到成功为止
*/
void DDNS_ClientRegister_UntilSuccess(void)
{
    int iErrCode = DH_OK;

    /* 在DDNS 服务器上进行注册 */
    while (1) {
        iErrCode = DDNS_Client_Register();
        if (DH_OK != iErrCode) {
            printf("DDNS Client register failed, status = %d\n", iErrCode);
            sleep(DDNS_SLEEP_TIME);
        } else {
            printf("DDNS Client register successed, status = %d\n", iErrCode);
            return;
        }
    }

    return;
}

/*
在DDNS 服务器上进行保活，保活125s后如果还没保活退出，否则保活成功
*/
void DDNS_TimeKeephandle(void)
{
    int status = 0;
    int iCount = 0;

    while (1) {
        status = DDNS_Client_KeepAlive();

        if (DH_OK != status) {
            printf("DDNS Client KeepAlive failed, status = %d\n", status);
            sleep(DDNS_SLEEP_TIME);
            iCount ++;
            if (DDNS_SLEEP_TIME * DDNS_SLEEP_TIME == iCount) {
                return;
            }
        } else {
            printf("DDNS Client KeepAlive successed, status = %d\n", status);
            return;
        }
    }
}




