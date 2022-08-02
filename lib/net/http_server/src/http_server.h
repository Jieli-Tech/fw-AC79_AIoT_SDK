#ifndef  __HTTP_GET_SERVER_H__
#define  __HTTP_GET_SERVER_H__

#include <stddef.h>
#include <list.h>
#include <sock_api.h>

#define MAX_HTTP_BUFFER      1460


struct http_cli_t {
    struct list_head entry;
    void *sock_hdl;
    struct sockaddr_in  dst_addr;
    int req_exit_flag;
    int pid;
};

enum file_type {
    VIR_FILE,
    FLASH_FILE,
    SD_CARD_FILE,
};

typedef struct {
    enum file_type type;
    void *file;
} http_file_hdl;


void unexpected_end_tran(void *sock_hdl);

void badRequest(void *sock_hdl);

void notImplemented(void *sock_hdl);

void forbidden(void *sock_hdl);

void notFound(void *sock_hdl);

void fileFound(void *sock_hdl, char *cType, int content_len, int lowRange, int highRange);

int http_fattrib_isidr(const char *file_name, u8 *isdir);

struct vfscan *http_f_opendir(const char *path, const char *arg);

void *http_f_readdir(struct vfscan *fs, int set_mode, int arg);

void http_f_closedir(struct vfscan *fs);

http_file_hdl *http_fopen(const char *path, const char *mode);

void http_fclose(http_file_hdl *file_hdl);

int http_fread(void *buffer, size_t size, size_t count, http_file_hdl *file_hdl);

int http_flen(http_file_hdl *file_hdl);

int http_fseek(http_file_hdl *file_hdl, int offset, int origin);

int http_virfile_reg(const char *path, const char *contents, unsigned long len);

#endif

