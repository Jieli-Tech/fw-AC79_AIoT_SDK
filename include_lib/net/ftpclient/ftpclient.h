#ifndef CYGONCE_NET_FTPCLIENT_FTPCLIENT_H
#define CYGONCE_NET_FTPCLIENT_FTPCLIENT_H

// User-defined function used to provide data
typedef int (*ftp_read_t)(void *ftp_ctx, char *buf, unsigned int bufsize, void *priv);
// User-defined function used to process data
typedef int (*ftp_write_t)(void *ftp_ctx, char *buf, unsigned int bufsize, void *priv);
typedef int (*ftp_read_dir_t)(void *ftp_ctx, char *file_name, void *priv);


typedef struct {
    //public
    char *hostname;
    char *username;
    char *passwd;
    unsigned int timeout_millsec;

    union {
        struct {
            char *filename;
            char *buf;
            unsigned int buf_size;
            ftp_read_t ftp_read;
            void *ftp_priv;
        } read;
        struct {
            char *buf;
            char *filename;
            unsigned int buf_size;
            ftp_read_t ftp_write;
            void *ftp_priv;
        } write;
        struct {
            char *list_cmd;
            char *dir_name;
            ftp_read_dir_t ftp_read_dir;
            void *ftp_priv;
        } list;
        struct {
            char *filename;
            char *dir_name;
        } del;
        struct {
            char *dir_name;
        } make_dir;
        struct {
            char *dir_name;
            char *old_name;
            char *new_name;
        } rename;
    } method;

    //priv
    int exit_flag;
    int req_exit_flag;
} ftp_ctx;

/* FTP 连接登录测试接口 */
int ftp_conn_login_test(ftp_ctx *ctx);

/* Use the FTP protocol to retrieve a file from a server. Only binary
   mode is supported. The filename can include a directory name. Only
   use unix style / not M$'s \. The file is placed into buf. buf has
   maximum size buf_size. If the file is bigger than this, the
   transfer fails and FTP_TOOBIG is returned. Other error codes as
   listed below can also be returned. If the transfer is succseful the
   number of bytes received is returned. */
int ftp_get(ftp_ctx *ctx);

/*Use the FTP protocol to send a file from a server. Only binary mode
  is supported. The filename can include a directory name. Only use
  unix style / not M$'s \. The contents of buf is placed into the file
  on the server. If an error occurs one of the codes as listed below
  will be returned. If the transfer is succseful zero is returned.*/

int ftp_put(ftp_ctx *ctx);

int ftp_del_file(ftp_ctx *ctx);

int ftp_make_dir(ftp_ctx *ctx);

int ftp_rename_file(ftp_ctx *ctx);

int ftp_list_dir(ftp_ctx *ctx);

void ftp_cancel(ftp_ctx *ctx);


int ftps_get(char *filename,
             char *buf,
             unsigned int buf_size,
             char *hostname,
             char *username,
             char *passwd,
             ftp_read_t ftp_read,
             void *ftp_read_priv
            );



/* Error codes */

#define FTP_BAD         -2 /* Catch all, socket errors etc. */
#define FTP_NOSUCHHOST  -3 /* The server does not exist. */
#define FTP_BADUSER     -4 /* Username/Password failed */
#define FTP_TOOBIG      -5 /* Out of buffer space or disk space */
#define FTP_BADFILENAME -6 /* The file does not exist */
#define FTP_NOMEMORY    -7 /* Unable to allocate memory for internal buffers */

/* ftp连接测试接口返回值 */
#define FTP_LOGIN_OK     0
#define FTP_LOGIN_ERR   -1
#define FTP_CONNECT_ERR -2
#define FTP_SRV_REFUSED -3

#endif // CYGONCE_NET_FTPCLIENT_FTPCLIENT

