#include "system/includes.h"
#include "system/task.h"
#include "app_config.h"
#include "net_update.h"
#include "update/update_loader_download.h"
#include "dual_bank_updata_api.h"
#include "generic/errno-base.h"
#include "update.h"
#include "timer.h"
#include "fs/fs.h"
#include "os/os_api.h"

//=======================net_update==========================================================
#define NET_UPDATE_STATE_NONE	0
#define NET_UPDATE_STATE_OK		1
#define NET_UPDATE_STATE_ERR	2
#define NET_UPDATE_STATE_CLOSE	3

#define NET_UPDATE_BUF_CHECK	0x12345678 //防止写超buf检测

#define FLASH_SECTOR_SIZE 	(4*1024)
#define NET_UPDATE_BUFF_SIZE_MAX	(4*1024)

/* #define DUAL_BANK_VERIFY_ENABLE */    //回读flash进行校验

struct net_update {
    u8 update_doing;
    u8 update_state;
    u8 write_to_flash;
    u8 *recv_buf;
    u32 recv_buf_size;
    u32 offset;
    u32 recv_cnt;
    u32 req_seek;
    OS_SEM sem;
    FILE *fd;
};
static struct net_update *net_update_info = NULL;
int storage_device_ready(void);
u32 get_target_udate_addr(void);

//=======================net_update==========================================================

#ifdef DUAL_BANK_VERIFY_ENABLE
static int dual_bank_verify_hdl(int result)
{
    if (result) {
        printf("app core verify OK!");
        //回读校验正确，更新启动信息
        dual_bank_update_burn_boot_info(net_update_finish_callback);
    } else if (net_update_info) {
        //回读校验错误
        net_update_info->update_state = NET_UPDATE_STATE_ERR;
        os_sem_post(&net_update_info->sem);
        printf("net_update err\n\n");
        return -1;
    }

    return 0;
}
#endif

static void net_update_system_reset(void *priv)
{
    printf("cpu reset ....\n\n");
    system_reset();
}
static int net_update_doing_callback(void *result)
{
    if (result) {
        if (net_update_info) {
            net_update_info->update_state = NET_UPDATE_STATE_ERR;
            os_sem_post(&net_update_info->sem);
        }
        printf("net_update doing err\n\n");
    }

    return 0;
}
static int net_update_finish_callback(int result)
{
    if (!result) {
        if (net_update_info) {
            net_update_info->update_state = NET_UPDATE_STATE_OK;
            os_sem_post(&net_update_info->sem);
        }
        printf("net_update successfuly\n\n");
    } else if (net_update_info) {
        net_update_info->update_state = NET_UPDATE_STATE_ERR;
        os_sem_post(&net_update_info->sem);
        printf("net_update err\n\n");
        return -1;
    }
    return 0;
}
int net_update_request(void)
{
    if (net_update_info) {
        return net_update_info->update_doing;
    }
    return 0;
}
void *net_fopen(char *path, char *mode)//net_fopen支持写flash固件升级和写到SD卡，当名字字符有CONFIG_UPGRADE_OTA_FILE_NAME时是固件升级
{
    //防止超时时，在没退出时，客户端再次重入
    int ret = net_update_request();
    if (ret) {
        return NULL;
    }

    struct net_update *net_update = zalloc(sizeof(struct net_update));
    if (!net_update) {
        printf("err in no mem \n\n");
        goto err;
    }
    if (strstr(path, CONFIG_UPGRADE_OTA_FILE_NAME)) {
        net_update->recv_buf = malloc(NET_UPDATE_BUFF_SIZE_MAX + 4);
        if (!net_update->recv_buf) {
            goto err;
        }
        net_update->update_doing = true;
        net_update->write_to_flash = true;
        net_update->offset = 0;
        net_update->req_seek = 0;
        net_update->recv_cnt = 0;
        net_update->update_state = NET_UPDATE_STATE_NONE;
        net_update->recv_buf_size = NET_UPDATE_BUFF_SIZE_MAX;
        os_sem_create(&net_update->sem, 0);
        net_update_info = net_update;
        u32 *check = (u32 *)(net_update->recv_buf + net_update->recv_buf_size);
        *check = NET_UPDATE_BUF_CHECK;
        return (void *)net_update;
    } else if (storage_device_ready()) {
        net_update->fd = fopen(path, mode);
        if (!net_update->fd) {
            goto err;
        }
        return (void *)net_update;
    }
err:
    if (net_update) {
        if (net_update->recv_buf) {
            free(net_update->recv_buf);
        }
        free(net_update);
    }
    net_update_info = NULL;
    return NULL;
}
int net_fclose(void *fd, char is_socket_err)
{
    struct net_update *net_update = (struct net_update *)fd;
    int err = 0;

    if (!net_update) {
        return -1;
    }
    if (net_update->write_to_flash) {
        u32 *check = (u32 *)(net_update->recv_buf + net_update->recv_buf_size);
        if (*check != NET_UPDATE_BUF_CHECK) {
            printf("err check in %s, check = 0x%x \n\n\n", __func__, *check);
        }
        printf("net close %s \n", is_socket_err ? "is_socket_err" : "normal");
        if (net_update->offset && net_update->update_state == NET_UPDATE_STATE_NONE && !is_socket_err) {
            err = net_fwrite(net_update, NULL, net_update->offset, 1);
            if (err != net_update->offset) {
                err = -1;
                printf("err net_fclose write  = %d \n", err);
            }
        }
        if (net_update->update_state == NET_UPDATE_STATE_NONE && !is_socket_err) {

#ifdef DUAL_BANK_VERIFY_ENABLE
            dual_bank_update_verify(NULL, NULL, dual_bank_verify_hdl);
#else
            //更新启动信息
            dual_bank_update_burn_boot_info(net_update_finish_callback);
#endif //DUAL_BANK_VERIFY_ENABLE

            err = os_sem_pend(&net_update_info->sem, 500);
            if (err) {
                err = -1;
                printf("net_update verify wait time out err\n\n\n\n");
            }
        }
        net_update->update_doing = 0;
        if (net_update->update_state == NET_UPDATE_STATE_OK) {
            sys_timeout_add_to_task("sys_timer", NULL, net_update_system_reset, 2000);
            printf(">>>>>>>>>>>net_update successfuly , system will reset after 2s ...\n\n");
        } else {
            err = -1;
            printf(">>>>>>>>>>>net_update err\n\n\n\n");
        }
        net_update->update_state = NET_UPDATE_STATE_CLOSE;
        os_sem_del(&net_update->sem, 0);
        dual_bank_passive_update_exit(NULL);
    } else if (net_update->fd) {
        fclose(net_update->fd);
    }
    if (net_update->recv_buf) {
        free(net_update->recv_buf);
    }
    net_update_info = NULL;
    free(net_update);
    return err;
}
int net_flen(void *fd)
{
    struct net_update *net_update = (struct net_update *)fd;
    if (net_update && !net_update->write_to_flash && net_update->fd) {
        return flen(net_update->fd);
    }
    return 0;
}
int net_fread(void *fd, char *buf, int len)
{
    struct net_update *net_update = (struct net_update *)fd;
    if (net_update && !net_update->write_to_flash && net_update->fd) {
        fread(buf, 1, len, net_update->fd);
    }
    return 0;
}
int net_fwrite(void *fd, unsigned char *buf, int len, int end)
{
    struct net_update *net_update = (struct net_update *)fd;
    int ret = len;
    int err;
    int wr_len = 0;
    int wr_offset = 0;
    int rd_len = 0;
    int rd_offset = 0;
    int buf_offset = 0;
    int cp_remain = net_update->recv_buf_size;
    u8 *cp_buf = (u8 *)net_update->recv_buf;
    u32 *check = (u32 *)(net_update->recv_buf + net_update->recv_buf_size);

    if (net_update->write_to_flash) {
        if (net_update->update_state == NET_UPDATE_STATE_OK) {
            net_update->offset = 0;
            return ret;
        } else if (net_update->update_state == NET_UPDATE_STATE_ERR || net_update->update_state == NET_UPDATE_STATE_CLOSE) {
            return -EINVAL;
        }
        while (len) {
            if (net_update->offset < net_update->recv_buf_size && !end) {
                rd_len = MIN(net_update->recv_buf_size - net_update->offset, len);
                memcpy(net_update->recv_buf + net_update->offset, buf + rd_offset, rd_len);
                net_update->offset += rd_len;
                rd_offset += rd_len;
                len -= rd_len;
                if (*check != NET_UPDATE_BUF_CHECK) {
                    printf("err check in %s, check = 0x%x \n\n\n", __func__, *check);
                }
                if (net_update->offset < net_update->recv_buf_size) {
                    return ret;
                }
            }
            if (!end) {
                net_update->offset = 0;
                cp_remain = net_update->recv_buf_size - net_update->offset;
            } else {
                cp_remain = net_update->offset;
                len -= cp_remain;
            }
            while (cp_remain) {
                if (!net_update->recv_cnt) {
                    err = dual_bank_passive_update_init(0, 0, NET_UPDATE_BUFF_SIZE_MAX, NULL);
                    if (err || dual_bank_update_allow_check(0)) {
                        net_update->update_state = NET_UPDATE_STATE_ERR;
                        printf("net_fwrite init check err \n\n");
                        return err;
                    }
                    u32 target_addr = get_target_udate_addr();
                    u32 addr = ADDR_ALIGNE(target_addr, FLASH_SECTOR_SIZE);
                    u32 first_size = addr - target_addr;
                    if (first_size) {
                        //要先写前4K对齐剩下字节，否则出现数据包不完整
                        err = dual_bank_update_write(cp_buf, first_size, NULL);
                        if (err) {
                            net_update->update_state = NET_UPDATE_STATE_ERR;
                            return err;
                        }

                        memcpy(cp_buf, cp_buf + first_size, \
                               cp_remain > first_size ? (cp_remain - first_size) : cp_remain);

                        net_update->offset = (cp_remain > first_size ? (cp_remain - first_size) : cp_remain);
                        net_update->recv_cnt += first_size;
                        break;
                    }
                }
                if (net_update->update_state) {
                    printf("net_update update_state = %d \n\n", net_update->update_state);
                    net_update->offset = 0;
                    return net_update->update_state == NET_UPDATE_STATE_OK ? ret : -EINVAL;
                }
                if (*check != NET_UPDATE_BUF_CHECK) {
                    printf("err check in %s, check = 0x%x \n\n\n", __func__, *check);
                }
                err = dual_bank_update_write(cp_buf, cp_remain, net_update_doing_callback);
                if (err) {
                    net_update->update_state = NET_UPDATE_STATE_ERR;
                    return err;
                }
                net_update->recv_cnt += cp_remain;
                cp_remain -= cp_remain;
            }
        }
    } else if (net_update->fd) {
        fwrite(buf, 1, len, net_update->fd);
    }
    return ret;
}

