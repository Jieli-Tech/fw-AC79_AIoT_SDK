#include "sock_api.h"
#include  "net_sock.h"

#if 1
#define log_info(x, ...)    printf("\n\n>>>>>>[uchttp_debug]" x " ", ## __VA_ARGS__)
#else
#define log_info(...)
#endif

extern void Get_IPAddress(u8_t lwip_netif, char *ipaddr);

struct http_table {
    int id;
    u8 mode;    //0 = http, 1 = https
    u8 is_used; //0 = 未使用, 1 =已使用
};

/*ssl_tsl*/
struct secure_param {
    char *CaPtr;
    char *CliPtr;
    char *KeyPtr;
    int TimeOut;
};

typedef  enum  net_tcp_state {
    NET_TCP_CONN_STATE_NONE               = 1u,
    NET_TCP_CONN_STATE_FREE               = 2u,

    NET_TCP_CONN_STATE_CLOSED             = 10u,

    NET_TCP_CONN_STATE_LISTEN             = 20u,

    NET_TCP_CONN_STATE_SYNC_RXD           = 30u,
    NET_TCP_CONN_STATE_SYNC_RXD_PASSIVE   = 31u,
    NET_TCP_CONN_STATE_SYNC_RXD_ACTIVE    = 32u,

    NET_TCP_CONN_STATE_SYNC_TXD           = 35u,

    NET_TCP_CONN_STATE_CONN               = 40u,

    NET_TCP_CONN_STATE_FIN_WAIT_1         = 50u,
    NET_TCP_CONN_STATE_FIN_WAIT_2         = 51u,
    NET_TCP_CONN_STATE_CLOSING            = 52u,
    NET_TCP_CONN_STATE_TIME_WAIT          = 53u,

    NET_TCP_CONN_STATE_CLOSE_WAIT         = 55u,
    NET_TCP_CONN_STATE_LAST_ACK           = 56u,

    NET_TCP_CONN_STATE_CLOSING_DATA_AVAIL = 59u
} NET_TCP_CONN_STATE;

typedef  struct  net_app_sock_secure_mutual_cfg {
    NET_SOCK_SECURE_CERT_KEY_FMT  Fmt;
    CPU_CHAR                     *CertPtr;
    CPU_INT32U                    CertSize;
    CPU_BOOLEAN                   CertChained;
    CPU_CHAR                     *KeyPtr;
    CPU_INT32U                    KeySize;
    /*增加参数设置*/
    CPU_CHAR                     *CliPtr;
    CPU_INT32U                    CliSize;
} NET_APP_SOCK_SECURE_MUTUAL_CFG;

typedef  struct  net_app_sock_secure_cfg {
    CPU_CHAR                        *CommonName;
    NET_SOCK_SECURE_TRUST_FNCT       TrustCallback;
    NET_APP_SOCK_SECURE_MUTUAL_CFG  *MutualAuthPtr;
    void *UserDataPtr; 								//增加
} NET_APP_SOCK_SECURE_CFG;

typedef  unsigned int  NET_TCP_CONN_ID;
typedef  CPU_INT16U  NET_TCP_TIMEOUT_SEC;

#define HTTP_TABLES_SIZE 128
static struct http_table http_tables[HTTP_TABLES_SIZE] = {0};

/********************************https安全传输*******************************************/
#include "http_cli.h"
#include <stdlib.h>
#include "generic/ascii.h"
#include "fs/fs.h"

#include "mbedtls/mbedtls_config.h"
#include "mbedtls/net.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"
#include "mbedtls/certs.h"

struct mbedtls_ctx {
    mbedtls_x509_crt cacert;
    mbedtls_x509_crt clicert;
    mbedtls_pk_context pkey;
    mbedtls_ssl_context ssl;
    mbedtls_net_context server_fd;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config conf;
    const char *pers;
    mbedtls_ssl_session saved_session;
};

#ifdef HTTPIN_DEBUG_EN
#define httpin_debug            printf
#define httpin_debug_pbuf       put_buf
#else
#define httpin_debug            printf
#define httpin_debug_pbuf       put_buf
#endif

#define HTTPIN_DEBUG_ERROR_EN
#ifdef HTTPIN_DEBUG_ERROR_EN
#define httpin_err_debug            printf
#define httpin_err_debug_pbuf       put_buf
#else
#define httpin_err_debug            printf
#define httpin_err_debug_pbuf       put_buf
#endif
/********************************https安全传输*******************************************/

static void table_add(const int id, const u8 mode)
{
    int i;
    for (i = 0; i < HTTP_TABLES_SIZE; i++) {
        if (http_tables[i].is_used == 0) {
            break;
        }
    }
    http_tables[i].id = id;
    http_tables[i].mode = mode;
    http_tables[i].is_used = 1;
}

static void table_delete(const int id)
{
    int i;
    for (i = 0; i < HTTP_TABLES_SIZE; i++) {
        if (http_tables[i].id == id) {
            break;
        }
    }

    http_tables[i].is_used = 0;
}

const static struct http_table *table_find(const int id)
{
    int i;
    for (i = 0; i < HTTP_TABLES_SIZE; i++) {
        if (http_tables[i].id == id) {
            return &http_tables[i];
        }
    }

    return NULL;
}


/* extern int httpin_sock_cb(enum sock_api_msg_type type, void *priv); */

static void my_debug(void *ctx, int level,
                     const char *file, int line,
                     const char *str)
{
    mbedtls_printf("%s", str);
}

static void *ssl_connect(char *hostname, char *port, int to_ms, void *ctx, char *server_name);
static int ssl_write(void *hdl, const void *buf, u32_t len, int flag);
static int ssl_read(void *hdl, void *buf, u32_t len, int flag);
static void ssl_close_notify(void *hdl);

NET_SOCK_RTN_CODE  NetSock_TxDataTo(NET_SOCK_ID          sock_id,
                                    void                *p_data,
                                    CPU_INT16U           data_len,
                                    NET_SOCK_API_FLAGS   flags,
                                    NET_SOCK_ADDR       *p_addr_remote,
                                    NET_SOCK_ADDR_LEN    addr_len,
                                    NET_ERR             *p_err)
{
    ssize_t  rtn_code;
    const struct http_table *tablePtr = NULL;

    tablePtr = table_find(sock_id);
    if (tablePtr == NULL && !(tablePtr->is_used)) {
        log_info("%s %d->  table_find\n", __FUNCTION__, __LINE__);
    }

    if (tablePtr->mode) {
        rtn_code = ssl_write((void *)sock_id, p_data, data_len, 0);
        *p_err = NET_SOCK_ERR_NONE;
    } else {
        rtn_code = (ssize_t)sock_sendto((void *)sock_id, p_data, data_len, 0, (struct sockaddr *)p_addr_remote, addr_len);
        if (rtn_code <= 0) {
            log_info("%s %d->  sock_sendto: %d\n", __FUNCTION__, __LINE__, (int)rtn_code);
            *p_err =  NET_SOCK_ERR_INVALID_TYPE;
        }

        *p_err = NET_SOCK_ERR_NONE;
    }

    return (rtn_code);
}

NET_SOCK_RTN_CODE  NetSock_RxDataFrom(NET_SOCK_ID          sock_id,
                                      void                *p_data_buf,
                                      CPU_INT16U           data_buf_len,
                                      NET_SOCK_API_FLAGS   flags,
                                      NET_SOCK_ADDR       *p_addr_remote,
                                      NET_SOCK_ADDR_LEN   *p_addr_len,
                                      void                *p_ip_opts_buf,
                                      CPU_INT08U           ip_opts_buf_len,
                                      CPU_INT08U          *p_ip_opts_len,
                                      NET_ERR             *p_err)
{
    ssize_t rtn_code;
    const struct http_table *tablePtr = NULL;

    tablePtr = table_find(sock_id);
    if (tablePtr == NULL && !(tablePtr->is_used)) {
        log_info("%s %d->  table_find\n", __FUNCTION__, __LINE__);
    }

    if (tablePtr->mode) {
        rtn_code = ssl_read((void *)sock_id, (void *)p_data_buf, (size_t)data_buf_len, 0);
        if (rtn_code < 0) {
            rtn_code = 0;
        }
        *p_err = NET_SOCK_ERR_NONE;
    } else {
        struct sockaddr_in dest_addr;
        socklen_t dest_len = sizeof(dest_addr);

        rtn_code = sock_recvfrom((void *)sock_id, (void *)p_data_buf, (size_t)data_buf_len, 0, (struct sockaddr *)&dest_addr, &dest_len);
        if (rtn_code == -1) {
            log_info("%s %d->  sock_recvfrom: %d\n", __FUNCTION__, __LINE__, (int)rtn_code);
        }
        /* log_info("NetSock_RxDataFrom  data: \n%s\n", p_data_buf); */

        *p_err = NET_SOCK_ERR_NONE;
    }

    return (rtn_code);
}

/*todo*/
NET_SOCK_RTN_CODE  NetSock_OptGet(NET_SOCK_ID         sock_id,
                                  NET_SOCK_PROTOCOL   level,
                                  NET_SOCK_OPT_NAME   opt_name,
                                  void               *p_opt_val,
                                  NET_SOCK_OPT_LEN   *p_opt_len,
                                  NET_ERR            *p_err)
{
    const struct http_table *tablePtr = NULL;

    tablePtr = table_find(sock_id);
    if (tablePtr == NULL && !(tablePtr->is_used)) {
        log_info("%s %d->  table_find\n", __FUNCTION__, __LINE__);
    }

    if (tablePtr->mode) {
        *p_err = NET_SOCK_ERR_NONE;
        return (0);
    } else {
        int rtn_code;
        rtn_code = sock_getsockopt((void *)sock_id, (int)level, (int)opt_name, (void *)p_opt_val, (socklen_t *)p_opt_len);
        *p_err = NET_SOCK_ERR_NONE;
        return (rtn_code);
    }
}

/*todo*/
NET_SOCK_RTN_CODE  NetSock_OptSet(NET_SOCK_ID         sock_id,
                                  NET_SOCK_PROTOCOL   level,
                                  NET_SOCK_OPT_NAME   opt_name,
                                  const  void               *p_opt_val,
                                  NET_SOCK_OPT_LEN    opt_len,
                                  NET_ERR            *p_err)
{
    const struct http_table *tablePtr = NULL;
    tablePtr = table_find(sock_id);
    if (tablePtr == NULL && !(tablePtr->is_used)) {
        log_info("%s %d->  table_find\n", __FUNCTION__, __LINE__);
    }

    if (tablePtr->mode) {
        *p_err = NET_SOCK_ERR_NONE;
        return 0;
    } else {
        int opt_name_ch;
        int opt = 1;
        int rtn_code;

        switch (opt_name) {
        case NET_SOCK_OPT_TCP_NO_DELAY :
            opt_name_ch = 0x01;
            break;

        case NET_SOCK_OPT_SOCK_KEEP_ALIVE :
            opt_name_ch = 0x02;
            break;

        case NET_SOCK_OPT_TCP_KEEP_IDLE :
            opt_name_ch = 0x03;
            break;

        case NET_SOCK_OPT_TCP_KEEP_INTVL :
            opt_name_ch = 0x04;
            break;

        case NET_SOCK_OPT_TCP_KEEP_CNT :
            opt_name_ch = 0x05;
            break;

        default:
            log_info("not defined");
            return 1;
            break;
        }

        rtn_code = sock_setsockopt((void *)sock_id, (int)level, opt_name_ch, &opt, sizeof(opt));
        *p_err = NET_SOCK_ERR_NONE;
        return (rtn_code);
    }

}

NET_SOCK_RTN_CODE  NetSock_Close(NET_SOCK_ID   sock_id,
                                 NET_ERR      *p_err)
{
    const struct http_table *tablePtr = NULL;
    tablePtr = table_find(sock_id);
    if (tablePtr == NULL && !(tablePtr->is_used)) {
        log_info("%s %d->  table_find\n", __FUNCTION__, __LINE__);
    }

    if (tablePtr->mode) {
        ssl_close_notify((void *)sock_id);
        *p_err = NET_SOCK_ERR_NONE;
    } else {
        int rtn_code;
        sock_unreg((void *)sock_id);
        *p_err = NET_SOCK_ERR_NONE;
    }

    table_delete((unsigned int)sock_id);

    return (0);
}

/*todo*/
NET_CONN_ID  NetSock_GetConnTransportID(NET_SOCK_ID   sock_id,
                                        NET_ERR      *p_err)
{
    log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    printf("NetSock_GetConnTransportID : %d", sock_id);
    *p_err = NET_SOCK_ERR_NONE;
    return sock_id;
}

/* NET_SOCK_RTN_CODE  NetSock_Sel (NET_SOCK_QTY       sock_nbr_max, */
/* NET_SOCK_DESC     *p_sock_desc_rd, */
/* NET_SOCK_DESC     *p_sock_desc_wr, */
/* NET_SOCK_DESC     *p_sock_desc_err, */
/* NET_SOCK_TIMEOUT  *p_timeout, */
/* NET_ERR           *p_err) */
/* { */
/* log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__); */
/* int rtn_code; */
/* rtn_code = (int)sock_select(sock_cli, (struct fd_set*)p_sock_desc_rd, (struct fd_set*)p_sock_desc_wr, NULL, NULL); */
/* printf("rtn_code : %d", rtn_code); */
/* *p_err = NET_SOCK_ERR_NONE; */
/* return (rtn_code); */
/* } */

/*todo*/
CPU_BOOLEAN  NetSock_CfgBlock(NET_SOCK_ID   sock_id,
                              CPU_INT08U    block,
                              NET_ERR      *p_err)
{
    log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    *p_err = NET_SOCK_ERR_NONE;
    return 0;
}

/*todo*/
CPU_BOOLEAN NetTCP_ConnCfgMSL_Timeout(NET_TCP_CONN_ID      conn_id_tcp,
                                      NET_TCP_TIMEOUT_SEC  msl_timeout_sec,
                                      NET_ERR             *p_err)
{
    log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    /* log_info("NetTCP_ConnCfgMSL_Timeout : %d", conn_id_tcp); */
    *p_err = NET_TCP_ERR_NONE;
    return 0;
}




NET_IP_ADDR_FAMILY  NetApp_ClientStreamOpenByHostname(unsigned int             *p_sock_id,
        CPU_CHAR                 *p_remote_host_name,
        NET_PORT_NBR              remote_port_nbr,
        NET_SOCK_ADDR            *p_sock_addr,
        NET_APP_SOCK_SECURE_CFG  *p_secure_cfg,
        CPU_INT32U                req_timeout_ms,
        NET_ERR                  *p_err)
{
    struct hostent *hent = NULL;
    struct secure_param param = {0};

#ifdef  HTTP_CLI_C_EN
    httpcli_ctx *ctx = p_secure_cfg->UserDataPtr;
#endif

    NET_APP_SOCK_SECURE_MUTUAL_CFG *MuturalParam = p_secure_cfg->MutualAuthPtr;
    static int use_ssl_tsl = 0;

    if (MuturalParam && MuturalParam->CertChained == DEF_YES && !use_ssl_tsl) {
        /*设置证书,key路径*/
        param.CaPtr = MuturalParam->CertPtr;
        param.CliPtr = MuturalParam->CliPtr;
        param.KeyPtr = MuturalParam->KeyPtr;
        if (req_timeout_ms == 0) {
            param.TimeOut = 60;
        } else {
            param.TimeOut = req_timeout_ms;
        }
        use_ssl_tsl = 1;
    }


#ifdef  HTTP_CLI_C_EN
    if (ctx && ctx->mode == MODE_HTTPS && !use_ssl_tsl) {
        /*设置证书,key路径*/
        param.CaPtr = ctx->cas_pem_path;
        param.CliPtr = ctx->cas_pem_path;
        param.KeyPtr = ctx->cas_pem_path;

        if (ctx->timeout_millsec == 0) {
            param.TimeOut = 60;
        } else {
            param.TimeOut = ctx->timeout_millsec;
        }

        use_ssl_tsl = 1;
    }
#endif

    if (use_ssl_tsl) {
        log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
        use_ssl_tsl = 0;
        char tmp_buf[128];
        struct sockaddr_in dest;
        struct mbedtls_ctx *tls_ctx = NULL;
        char host[16] = {0};
        char pA[16];

        NET_IP_ADDR_FAMILY   addr_rtn = NET_IP_ADDR_FAMILY_UNKNOWN;

        hent = gethostbyname(p_remote_host_name);
        if (NULL == hent) {
            log_info("gethostbyname fail! \n");
            *p_err = NET_APP_ERR_FAULT;
            return addr_rtn;
        }

        memset(&dest, 0, sizeof(dest));
        dest.sin_family = AF_INET;

        if (remote_port_nbr == 80) {
            dest.sin_port = htons(443);
            sprintf(pA, "%d", 443);
        } else {
            dest.sin_port = htons(remote_port_nbr);
            sprintf(pA, "%d", remote_port_nbr);
        }

        memcpy(&dest.sin_addr, hent->h_addr, sizeof(struct in_addr));

        tls_ctx = ssl_connect(inet_ntoa_r(dest.sin_addr.s_addr, host, sizeof(host)), pA, param.TimeOut, &param, p_remote_host_name);

        if (tls_ctx == NULL) {
            *p_err = NET_ERR_FAULT_NULL_PTR;
        } else {
            *p_sock_id = (unsigned int)tls_ctx;
            *p_err = NET_APP_ERR_NONE;
            addr_rtn = NET_IP_ADDR_FAMILY_IPv4;
            table_add((unsigned int)tls_ctx, 1);
        }
        return addr_rtn ;
    }

    /******************************************http 连接************************************************/
    log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    void *sock_cli = NULL;
    struct sockaddr_in remote_dest;
    struct sockaddr_in local;
    char ipaddr[20];

    log_info("remote_host_name : %s\n", p_remote_host_name);
    log_info("remote_port_nbr : %d", remote_port_nbr);
    hent = gethostbyname((char *)p_remote_host_name);

    memset(&remote_dest, 0, sizeof(remote_dest));

    /*目的sockaddr*/
    remote_dest.sin_family = AF_INET;
    remote_dest.sin_port = htons(remote_port_nbr);
    memcpy(&remote_dest.sin_addr, hent->h_addr, sizeof(struct in_addr));

    log_info("remote_dest ip %s", inet_ntoa(remote_dest.sin_addr.s_addr));

    sock_cli = sock_reg(AF_INET, SOCK_STREAM, 0, NULL, NULL);
    if (sock_cli == NULL) {
        log_info("sock_reg err!!!");
        *p_err = NET_APP_ERR_FAULT;
        goto EXIT;
        return -1;
    }

    if (0 != sock_set_reuseaddr(sock_cli)) {
        log_info("sock_set_reuseaddr err!!!");
        *p_err = NET_APP_ERR_FAULT;
        goto EXIT;
        return -1;
    }

    /*本地sockaddr*/
    Get_IPAddress(1, ipaddr);
    local.sin_addr.s_addr = inet_addr(ipaddr);
    local.sin_port = htons(0);
    local.sin_family = AF_INET;

    if (0 != sock_bind(sock_cli, (struct sockaddr *)&local, sizeof(struct sockaddr_in))) {
        log_info("%s sock_bind fail\n",  __FILE__);
        *p_err = NET_APP_ERR_FAULT;
        goto EXIT;
    }

    if (0 != sock_connect(sock_cli, (struct sockaddr *)&remote_dest, sizeof(struct sockaddr_in))) {
        log_info("sock_connect fail.\n");
        *p_err = NET_APP_ERR_FAULT;
        goto EXIT;
    }

    *p_sock_id = (unsigned int)sock_cli;
    table_add((unsigned int)sock_cli, 0);

    *p_err = NET_APP_ERR_NONE;

    return 0;

EXIT:
    sock_unreg(sock_cli);
    return -1;
}

/*todo*/
NET_TCP_CONN_STATE  NetTCP_ConnStateGet(NET_TCP_CONN_ID  conn_id)
{
    log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    NET_TCP_CONN_STATE   state;

    state       =  NET_TCP_CONN_STATE_FREE;
    return (state);
}

/*todo*/
/* NET_SOCK_ID  NetSock_Open (NET_SOCK_PROTOCOL_FAMILY   protocol_family, */
/* NET_SOCK_TYPE              sock_type, */
/* NET_SOCK_PROTOCOL          protocol, */
/* NET_ERR                   *p_err) */
/* { */
/* log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__); */
/* return 0; */
/* } */

static void *ssl_connect(char *hostname, char *port, int to_ms, void *ctx, char *server_name)
{
    int ret;
    uint32_t flags;
    unsigned char *buf = NULL;
    struct secure_param *param = (struct secure_param *)ctx;

    if (!hostname) {
        return NULL;
    }

    struct mbedtls_ctx *tls_ctx = calloc(1, sizeof(struct mbedtls_ctx));
    if (tls_ctx == NULL) {
        return NULL;
    }

    tls_ctx->pers = "https";

    /*
     * 0. Initialize the RNG and the session data
     */
    mbedtls_net_init(&tls_ctx->server_fd);
    /* mbedtls_net_set_cb(&tls_ctx->server_fd, httpin_sock_cb, ctx); */
    mbedtls_net_set_timeout(&tls_ctx->server_fd, to_ms, to_ms);

    mbedtls_ssl_init(&tls_ctx->ssl);
    mbedtls_ssl_config_init(&tls_ctx->conf);

    mbedtls_x509_crt_init(&tls_ctx->cacert);
    mbedtls_x509_crt_init(&tls_ctx->clicert);
    mbedtls_pk_init(&(tls_ctx->pkey));

    mbedtls_ctr_drbg_init(&tls_ctx->ctr_drbg);
    mbedtls_entropy_init(&tls_ctx->entropy);

#ifdef MBEDTLS_SSL_EXPORT_KEYS
    /* mbedtls_ssl_conf_export_keys_cb(&tls_ctx->conf, mbedtls_ssl_export_keys, (void *)(&tls_ctx->conf)); */
#endif

    if ((ret = mbedtls_ctr_drbg_seed(&tls_ctx->ctr_drbg, mbedtls_entropy_func, &tls_ctx->entropy,
                                     (const unsigned char *) tls_ctx->pers,
                                     strlen(tls_ctx->pers))) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto exit;
    }

    buf = malloc(4 * 1024);
    if (!buf) {
        goto exit;
    }

    void *cacert_fd = fopen(param->CaPtr, "r");
    if (cacert_fd) {
        log_d("LOAD  CA\n");

        int cacert_len = fread(buf, 1, 4 * 1024, cacert_fd);

        printf("cacert_len=>%d\n", cacert_len);

        fclose(cacert_fd);

        buf[cacert_len] = '\0';
        ret = mbedtls_x509_crt_parse(&tls_ctx->cacert, buf, cacert_len + 1);
        if (ret < 0) {
            mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
            goto exit;
        }
    } else {
        ret = mbedtls_x509_crt_parse(&tls_ctx->cacert, (unsigned char *)mbedtls_test_cas_pem, mbedtls_test_cas_pem_len);
        if (ret < 0) {
            mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
            goto exit;
        }
    }

    /* void *clicert_fd = fopen(ctx->cli_pem_path, "r"); */
    void *clicert_fd = fopen(param->CliPtr, "r");
    if (clicert_fd) {
        log_d("LOAD  PEM\n");

        int clicert_len = fread(buf, 1, 4 * 1024, clicert_fd);

        printf("clicert_len=>%d\n", clicert_len);

        fclose(clicert_fd);

        buf[clicert_len] = '\0';

        ret = mbedtls_x509_crt_parse(&tls_ctx->clicert, buf, clicert_len + 1);
        if (ret < 0) {
            mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
            goto exit;
        }
    } else {
        /* log_w("open cli pem fail\n"); */
    }

    /* void *pkey_fd = fopen(ctx->pkey_path, "r"); */
    void *pkey_fd = fopen(param->KeyPtr, "r");
    if (pkey_fd) {
        log_d("LOAD  KEY\n");

        int pkey_len = fread(buf, 1, 4 * 1024, pkey_fd);

        printf("pkey_len=>%d\n", pkey_len);

        fclose(pkey_fd);

        buf[pkey_len] = '\0';

        ret = mbedtls_pk_parse_key(&tls_ctx->pkey, buf, pkey_len + 1, NULL, 0);
        if (ret < 0) {
            mbedtls_printf(" failed\n  !  mbedtls_x509_crt_parse returned -0x%x\n\n", -ret);
            goto exit;
        }
    } else {
        /* log_w("open pkey fail\n"); */
    }

    free(buf);
    buf = NULL;

    /*
     * 1. Start the connection
     */
    mbedtls_printf("  . Connecting to tcp/%s/%s...\r\n", hostname, port);

    if ((ret = mbedtls_net_connect(&tls_ctx->server_fd, hostname,
                                   port, MBEDTLS_NET_PROTO_TCP)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_net_connect returned %d\n\n", ret);
        goto exit;
    }

    /*
     * 2. Setup stuff
     */
    /* mbedtls_printf("  . Setting up the SSL/TLS structure...\r\n"); */
    if ((ret = mbedtls_ssl_config_defaults(&tls_ctx->conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_config_defaults returned %d\n\n", ret);
        goto exit;
    }

    /* OPTIONAL is not optimal for security,
     * but makes interop easier in this simplified example */
    mbedtls_ssl_conf_authmode(&tls_ctx->conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    mbedtls_ssl_conf_ca_chain(&tls_ctx->conf, &tls_ctx->cacert, NULL);
    mbedtls_ssl_conf_rng(&tls_ctx->conf, mbedtls_ctr_drbg_random, &tls_ctx->ctr_drbg);
    mbedtls_ssl_conf_dbg(&tls_ctx->conf, my_debug, 0);


    if (pkey_fd  && clicert_fd) {
        ret = mbedtls_ssl_conf_own_cert(&(tls_ctx->conf), &(tls_ctx->clicert), &(tls_ctx->pkey));
        if (ret < 0) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_conf_own_cert returned %d\n\n", ret);
            goto exit;
        }
    }

    const char *alpnProtocols[] = { "x-amzn-http-ca", NULL };
    ret = mbedtls_ssl_conf_alpn_protocols(&(tls_ctx->conf), alpnProtocols);
    if (ret < 0) {
        mbedtls_printf(" failed\n  !  returne_ssl_conf_alpn_protocols returned %d\n\n", ret);
        goto exit;
    }

    if ((ret = mbedtls_ssl_setup(&tls_ctx->ssl, &tls_ctx->conf)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_setup returned %d\n\n", ret);
        goto exit;
    }

    if ((ret = mbedtls_ssl_set_hostname(&tls_ctx->ssl, server_name)) != 0) {
        mbedtls_printf(" failed\n  ! mbedtls_ssl_set_hostname returned %d\n\n", ret);
        goto exit;
    }

    mbedtls_ssl_set_bio(&tls_ctx->ssl, &tls_ctx->server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

    /*
     * 4. Handshake
     */
    /* mbedtls_printf("  . Performing the SSL/TLS handshake...\r\n"); */

    while ((ret = mbedtls_ssl_handshake(&tls_ctx->ssl)) != 0) {
        if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            mbedtls_printf(" failed\n  ! mbedtls_ssl_handshake returned -0x%x\n\n", -ret);
            goto exit;
        }
    }

    /*
     * 5. Verify the server certificate
     */
    /* mbedtls_printf("  . Verifying peer X.509 certificate...\r\n"); */

    /* In real life, we probably want to bail out when ret != 0 */
    if ((flags = mbedtls_ssl_get_verify_result(&tls_ctx->ssl)) != 0) {
        char vrfy_buf[512];

        /* mbedtls_printf(" failed\r\n"); */

        mbedtls_x509_crt_verify_info(vrfy_buf, sizeof(vrfy_buf), "  ! ", flags);

        /* mbedtls_printf("%s\r\n", vrfy_buf); */
    } else {
        mbedtls_printf(" ok\r\n");
    }

    return tls_ctx;

exit:

#ifdef MBEDTLS_ERROR_C
    if (ret != 0) {
        char error_buf[100];
        mbedtls_strerror(ret, error_buf, 100);
        mbedtls_printf("Last error was: %d - %s\n\n", ret, error_buf);
    }
#endif

    mbedtls_net_free(&tls_ctx->server_fd);
    mbedtls_x509_crt_free(&tls_ctx->cacert);
    mbedtls_ssl_free(&tls_ctx->ssl);
    mbedtls_ssl_config_free(&tls_ctx->conf);
    mbedtls_ctr_drbg_free(&tls_ctx->ctr_drbg);
    mbedtls_entropy_free(&tls_ctx->entropy);

    free(tls_ctx);
    if (buf) {
        free(buf);
    }

    return (NULL);
}

static int ssl_write(void *hdl, const void *buf, u32_t len, int flag)
{
    log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    struct mbedtls_ctx *tls_ctx = (struct mbedtls_ctx *)hdl;
    int ret;
    int remain_len = len;
    int payload = 0;
    int offset = 0;

    while (remain_len) {
        if (remain_len >= MBEDTLS_SSL_MAX_CONTENT_LEN) {
            payload = MBEDTLS_SSL_MAX_CONTENT_LEN;
            remain_len -= MBEDTLS_SSL_MAX_CONTENT_LEN;
        } else {
            payload = remain_len;
            remain_len = 0;
        }

        ret = mbedtls_ssl_write(&tls_ctx->ssl, (unsigned char *)buf + offset, payload);

        if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            return 0;
        }

        if (ret < 0) {
            httpin_err_debug("https_ssl_write fail:%d \n", ret);
            return ret;
        }
        offset += payload;
    }

    return ret;
}

static int ssl_read(void *hdl, void *buf, u32_t len, int flag)
{
    log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    struct mbedtls_ctx *tls_ctx = (struct mbedtls_ctx *)hdl;

    if (flag != 0) {
        return -1;
    }

    int ret = mbedtls_ssl_read(&tls_ctx->ssl, (unsigned char *)buf, len);
    printf("mbedtls_ssl_read : %d", ret);

    if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        return 0;
    }
    if (ret < 0) {
        httpin_err_debug("https_ssl_read fail:0x%x \n", ret);
    }

    return ret;
}

static void ssl_close_notify(void *hdl)
{
    log_info(">>>>>>>>>>>>>>>>>>>>>>>>>%s, %d", __FUNCTION__, __LINE__);
    struct mbedtls_ctx *tls_ctx = (struct mbedtls_ctx *)hdl;

    mbedtls_ssl_close_notify(&tls_ctx->ssl);
    mbedtls_net_free(&tls_ctx->server_fd);
    mbedtls_x509_crt_free(&tls_ctx->cacert);
    mbedtls_ssl_free(&tls_ctx->ssl);
    mbedtls_ssl_config_free(&tls_ctx->conf);
    mbedtls_ctr_drbg_free(&tls_ctx->ctr_drbg);
    mbedtls_entropy_free(&tls_ctx->entropy);
    free(tls_ctx);
}

