#include "app_config.h"
#include "ftpserver/stupid-ftpd.h"
#include "net_update.h"
#include "wifi/wifi_connect.h"
#include "system/includes.h"

#ifdef USE_FTP_UPGRADE_DEMO

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
static void ftpd_vfs_interface_cfg(void)
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

static void ftp_update_start(void *priv)
{

    //注册读写操作集
    ftpd_vfs_interface_cfg();

    //这里设置用户名为：FTPX, 密码为：12345678
    stupid_ftpd_init("MAXUSERS=2\nUSER=FTPX 12345678     0:/      2   A\n", NULL);
}

const char *get_root_path(void)
{
    return CONFIG_ROOT_PATH;
}

//示例需要运行在ap模式
void ftp_update_test(void)
{
    if (thread_fork("ftp_update_start", 10, 512, 0, NULL, ftp_update_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

#endif //USE_FTP_UPGRADE_DEMO
