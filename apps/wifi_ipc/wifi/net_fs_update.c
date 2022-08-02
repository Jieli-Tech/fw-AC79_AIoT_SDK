#include "system/includes.h"
#include "system/task.h"
#include "app_config.h"
#include "net_update.h"
#include "update/update_loader_download.h"
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

#define NET_UPDATE_BUFF_SIZE_MAX	(32*1024)
struct net_update {
    u8 update_doing;
    u8 update_state;
    u8 update_nopend;
    u8 write_to_flash;
    u8 *req_buf;
    u32 req_len;
    u8 *recv_buf;
    u32 recv_buf_size;
    u32 offset;
    u32 req_seek;
    u32 recv_read_addr;
    OS_SEM recv_sem;
    OS_SEM wait_sem;
    FILE *fd;
};
static struct net_update *net_update_info = NULL;
int storage_device_ready(void);

//=======================net_fs_pdate_ops==========================================================
static u16 net_update_fopen(void)
{
    if (!net_update_info) {
        return false;
    }
    return true;
}
static u16 net_update_fread(void *fp, u8 *buff, u16 len)
{
    int ret;
    if (!net_update_info || net_update_info->update_state == NET_UPDATE_STATE_ERR || net_update_info->update_state == NET_UPDATE_STATE_CLOSE) {
        return -1;
    }
    net_update_info->req_buf = buff;
    net_update_info->req_len = (u32)len;
    ASSERT(len <= net_update_info->recv_buf_size, "err no buf in req len = %d , recv_buf_size = %d\n\n", len, net_update_info->recv_buf_size);
    os_sem_post(&net_update_info->recv_sem);
    ret = os_sem_pend(&net_update_info->wait_sem, 500);
    if (ret) {
        net_update_info->update_state = NET_UPDATE_STATE_ERR;
        printf("err net_fread os_sem_pend time out \n\n");
        return -1;
    }
    return len;
}
static int net_update_fseek(void *fp, u8 type, u32 offset)
{
    if (!net_update_info || net_update_info->update_state == NET_UPDATE_STATE_ERR || net_update_info->update_state == NET_UPDATE_STATE_CLOSE) {
        return -1;
    }
    if (net_update_info->recv_read_addr <= offset && offset <= (net_update_info->recv_buf_size + net_update_info->recv_read_addr)) {
        net_update_info->req_seek = offset - net_update_info->recv_read_addr;
    } else {
        printf("\n err : net_update not support fseek , offset = %d , recv_read_addr = %d !\n\n", offset, net_update_info->recv_read_addr);
    }
    return 0;
}
static u16 net_update_fstop(u8 err)
{
    printf("net_update_fstop\n");
    if (!net_update_info) {
        return -1;
    }
    if (err & UPDATE_RESULT_FLAG_BITMAP) {
        net_update_info->update_state = NET_UPDATE_STATE_ERR;
    } else {
        net_update_info->update_state = NET_UPDATE_STATE_CLOSE;
    }
    os_sem_post(&net_update_info->wait_sem);
    os_sem_post(&net_update_info->recv_sem);
    return 0;
}
static const update_op_api_t net_update_op = {
    .f_open = net_update_fopen,
    .f_read = net_update_fread,
    .f_seek = net_update_fseek,
    .f_stop = net_update_fstop,
    .notify_update_content_size = NULL,
    .ch_init = NULL,
    .ch_exit = NULL,
};
//=======================net_fs_update_ops==========================================================

//=======================net_fs_update==========================================================
static void net_update_system_reset(void)
{
    printf("cpu reset ....\n\n");
    cpu_reset();
}
static void net_update_callback(int type, u32 state, void *priv)
{
    update_ret_code_t *ret_code = (update_ret_code_t *)priv;
    switch (state) {
    case UPDATE_CH_EXIT:
        if ((0 == ret_code->stu) && (0 == ret_code->err_code)) {
            u8 *update_ram = UPDATA_FLAG_ADDR;
            memset(update_ram, 0, 32);
            if (net_update_info) {
                net_update_info->update_state = NET_UPDATE_STATE_OK;
                os_sem_post(&net_update_info->recv_sem);
            }
            printf("net update ok \n");
        } else {
            printf("\nnet update err !!! \n");
            if (net_update_info) {
                net_update_info->update_state = NET_UPDATE_STATE_ERR;
                os_sem_post(&net_update_info->recv_sem);
            }
        }
        break;
    }
}
static int net_update_loader_download_init(void)
{
    update_mode_info_t info = {
        .type = NET_UFW_UPDATA,//DUAL_BANK_UPDATA,
        .state_cbk = net_update_callback,
        .p_op_api = &net_update_op,
        .task_en = 1,
    };
    app_active_update_task_init(&info);
    return 0;
}
static void net_update_loader_download_uninit(void)
{
    app_active_update_task_uninit();
}
int net_update_request(void)
{
    if (net_update_info) {
        return net_update_info->update_doing;
    }
    return 0;
}
void *net_fopen(char *path, char *mode)//net_fopen支持写flash固件升级和写到SD卡，当名字字符有CONFIG_UPGRADE_FILE_NAME时是固件升级
{
    struct net_update *net_update = zalloc(sizeof(struct net_update));
    if (!net_update) {
        printf("err in no mem \n\n");
        goto err;
    }
    if (strstr(path, CONFIG_UPGRADE_FILE_NAME)) {
        net_update->recv_buf = malloc(NET_UPDATE_BUFF_SIZE_MAX + 4);
        if (!net_update->recv_buf) {
            goto err;
        }
        net_update->update_doing = true;
        net_update->write_to_flash = true;
        net_update->offset = 0;
        net_update->req_seek = 0;
        net_update->recv_read_addr = 0;
        net_update->update_state = NET_UPDATE_STATE_NONE;
        net_update->recv_buf_size = NET_UPDATE_BUFF_SIZE_MAX;
        os_sem_create(&net_update->recv_sem, 0);
        os_sem_create(&net_update->wait_sem, 0);
        net_update_info = net_update;
        net_update_loader_download_init();

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
    int err;

    if (!net_update) {
        return 0;
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
                printf("err net_fclose write  = %d \n", err);
            }
        }
        if (net_update->update_state == NET_UPDATE_STATE_NONE) {
            net_update->update_state = NET_UPDATE_STATE_CLOSE;
            os_sem_post(&net_update->wait_sem);
            printf("wait net_fclose\n");
            err = os_sem_pend(&net_update->recv_sem, 500);
            if (err) {
                printf("err wait time out net_fclose \n\n\n\n");
            }
        }
        net_update->update_doing = 0;
        net_update_loader_download_uninit();
        if (net_update->update_state == NET_UPDATE_STATE_OK) {
            sys_timeout_add_to_task("sys_timer", NULL, net_update_system_reset, 2000);
            printf(">>>>>>>>>>>net_update successfuly , system will reset after 2s ...\n\n");
        } else {
            printf(">>>>>>>>>>>net_update err\n\n\n\n");
        }
        os_sem_del(&net_update->recv_sem, 0);
        os_sem_del(&net_update->wait_sem, 0);
    } else if (net_update->fd) {
        fclose(net_update->fd);
    }
    if (net_update->recv_buf) {
        free(net_update->recv_buf);
    }
    net_update_info = NULL;
    free(net_update);
    return 0;
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
int net_fwrite(void *fd, char *buf, int len, int end)
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
                if (!net_update->update_nopend) {
                    err = os_sem_pend(&net_update->recv_sem, 500);
                    if (err) {
                        net_update->update_state = NET_UPDATE_STATE_ERR;
                        printf("net_fwrite recv_sem wait time out err \n\n");
                        return err;
                    }
                } else {
                    net_update->update_nopend = 0;
                }
                if (net_update->update_state) {
                    printf("net_update update_state = %d \n\n", net_update->update_state);
                    net_update->offset = 0;
                    return net_update->update_state == NET_UPDATE_STATE_OK ? ret : -EINVAL;
                }
                if (net_update->req_seek) {
                    cp_remain = net_update->recv_buf_size - net_update->req_seek;
                    wr_offset = net_update->req_seek;
                    net_update->req_seek = 0;
                }
                if (*check != NET_UPDATE_BUF_CHECK) {
                    printf("err check in %s, check = 0x%x \n\n\n", __func__, *check);
                }
                if (net_update->recv_buf) {
                    wr_len = MIN(cp_remain, net_update->req_len);
                }
                if (wr_len == cp_remain && net_update->req_len != cp_remain) {
                    wr_len = MIN(cp_remain, net_update->recv_buf_size);
                    memcpy(net_update->recv_buf + net_update->offset, cp_buf + wr_offset, wr_len);
                    net_update->offset += wr_len;
                    wr_offset = 0;
                    net_update->update_nopend = true;
                    break;
                } else if (net_update->req_len) {
                    memcpy(net_update->req_buf, cp_buf + wr_offset, wr_len);
                    wr_offset += wr_len;
                    cp_remain -= wr_len;
                    os_sem_post(&net_update->wait_sem);
                }
            }
            net_update->recv_read_addr += net_update->recv_buf_size;
        }
    } else if (net_update->fd) {
        fwrite(buf, 1, len, net_update->fd);
    }
    return ret;
}


