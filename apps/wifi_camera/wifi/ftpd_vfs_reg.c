#include "app_config.h"
#include "ftpserver/stupid-ftpd.h"
#include "net_update.h"



static void *stupid_vfs_open(char *path,  char *mode)
{
    printf("\n stupid_vfs_open : path is %s , mode is %s\n", path, mode);
    return net_fopen(path, mode);
}

static int stupid_vfs_write(void  *file, void  *buf, u32 len)
{
    return net_fwrite(file, buf, len, 0);
}

static int stupid_vfs_read(void  *file, void  *buf, u32 len)
{
    return net_fread(file, buf, len);
}

static int stupid_vfs_flen(void  *file)
{
    return net_flen(file);
}

static int stupid_vfs_close(void *file, char is_socket_err)
{
    return net_fclose(file, is_socket_err);
}

//注册一个ftpd的vfs接口

void ftpd_vfs_interface_cfg(void)
{
    struct ftpd_vfs_cfg info;
    info.fopen = stupid_vfs_open;
    info.fwrite = stupid_vfs_write;
    info.fread = stupid_vfs_read;
    info.flen = stupid_vfs_flen;
    info.fclose = stupid_vfs_close;
//注册接口到ftp中
    stupid_vfs_reg(&info);
}

