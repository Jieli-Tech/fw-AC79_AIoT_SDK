
/*

  stupid-ftpd.h
  ------------


  Header for stupid-ftpd.c
  User status informations.

 */

#ifndef __STUPID_FTPD_H__
#define __STUPID_FTPD_H__

#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include "os/os_api.h"
enum FTPD_EVENT {
    FTP_CLI_CONNECTED,
    FTP_CLI_CLOSED,
};

struct ftpd_vfs_cfg {
    void *(*fopen)(const char *path, const char *mode);
    int (*fwrite)(void  *file, void  *buf, u32 len);
    int (*fread)(void  *file, void  *buf, u32 len);
    int (*flen)(void *file);
    int (*fclose)(void  *file, char is_socket_err);
};

void stupid_ftpd_log(const char *format, ...);
void stupid_ftpd_err(const char *format, ...);


int stupid_ftpd_init(const char *conf_file, int (*ftpd_cb)(enum FTPD_EVENT event, struct sockaddr_in *dst));

void stupid_ftpd_uninit(void);
int stupid_vfs_reg(struct ftpd_vfs_cfg *info);
void http_get_server_discpnnect_cli(struct sockaddr_in *dst_addr);
#endif
