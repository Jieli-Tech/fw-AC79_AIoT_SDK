#include "system/includes.h"
#include "app_config.h"
#include "update/update.h"
#include "fs/fs.h"
#include "update/update_loader_download.h"

#ifdef USE_SD_UPGRADE_DEMO

struct fs_update {
    void *fd;
    const char *update_path;
};
static struct fs_update fs_update = {0};
#define __this 		(&fs_update)

static u16 fs_open(void)
{
    if (!__this->update_path) {
        printf("file path err \n\n");
        return false;
    }
    if (__this->fd) {
        return true;
    }
    __this->fd = fopen(__this->update_path, "r");
    if (!__this->fd) {
        printf("file open err \n\n");
        return false;
    }
    return true;
}

static u16 fs_read(void *fp, u8 *buff, u16 len)
{
    if (!__this->fd) {
        return (u16) - 1;
    }
    int ret = fread(buff, 1, len, __this->fd);
    if (ret != len) {
        return -1;
    }
    return len;
}
static int fs_seek(void *fp, u8 type, u32 offset)
{
    if (!__this->fd) {
        return (int) - 1;
    }
    return fseek(__this->fd, offset, type);
}
static u16 fs_stop(u8 err)
{
    if (__this->fd) {
        fclose(__this->fd);
        __this->fd = NULL;
    }
    return true;
}
static const update_op_api_t fs_update_opt = {
    .f_open = fs_open,
    .f_read = fs_read,
    .f_seek = fs_seek,
    .f_stop = fs_stop,
};

static int fs_update_set_file_path(const char *update_path)
{
    void *fd;
    fd = fopen(update_path, "r");
    if (!fd) {
        printf("update file open err ");
        return false;
    }
    fclose(fd);
    __this->update_path = update_path;
    return true;
}

#if !CONFIG_DOUBLE_BANK_ENABLE
//单备份升级
static void fs_update_param_private_handle(UPDATA_PARM *p)
{
    u16 up_type = p->parm_type;
    void *start = (void *)((u32)p + sizeof(UPDATA_PARM));
    extern const char updata_file_name[];

    if ((up_type == SD0_UPDATA) || (up_type == SD1_UPDATA)) {
        UPDATA_SD sd = {0};
#if TCFG_SD0_ENABLE
        sd.control_type = SD_CONTROLLER_0;
        sd.control_io = (TCFG_SD_PORTS == 'A') ? SD0_IO_A : ((TCFG_SD_PORTS == 'B') ? SD0_IO_B : ((TCFG_SD_PORTS == 'C') ? SD0_IO_C : SD0_IO_D));
#elif TCFG_SD1_ENABLE
        sd.control_type = SD_CONTROLLER_1;
        sd.control_io = (TCFG_SD_PORTS == 'A') ? SD1_IO_A : SD1_IO_B;
#endif
        sd.power = 0;//是否启用SDPG电源引脚，1则开启SDPG电源
        memcpy(start, &sd, sizeof(UPDATA_SD));
    } else if (up_type == USB_UPDATA) {
        //
    }
    memcpy(p->file_patch, updata_file_name, strlen(updata_file_name));
}
#endif
static void fs_update_state_cbk(int type, u32 state, void *priv)
{
    update_ret_code_t *ret_code = (update_ret_code_t *)priv;
    switch (state) {
    case UPDATE_CH_EXIT:
        if ((0 == ret_code->stu) && (0 == ret_code->err_code)) {
            UPDATA_PARM *update_ram = UPDATA_FLAG_ADDR;
            memset(update_ram, 0, 32);
            if (support_dual_bank_update_en) {
                memset(update_ram, 0, sizeof(UPDATA_PARM));
                update_ram->magic = type;
                update_result_set(UPDATA_SUCC);
                printf(">>>>>>>>>>>>>>>>>>update ok , cpu reset ...\n");
            } else {
#if !CONFIG_DOUBLE_BANK_ENABLE
                //单备份
                update_mode_api_v2(type, fs_update_param_private_handle, NULL);
#endif
                printf(">>>>>>>>>>>>>>>>>> cpu reset , uboot todo update ...\n");
            }
            system_reset();
        } else {
            update_result_set(UPDATA_DEV_ERR);
            printf("\nupdate err !!! \n");
        }
        break;
    }
}

#if CONFIG_DOUBLE_BANK_ENABLE
static void get_update_percent_info(update_percent_info *info)
{
    printf("update >>> %d%%\n", info->percent);
}
#endif

//sd卡/u盘文件系统升级例子
static int fs_update_check(const char *path)
{
    if (update_success_boot_check() == true) {
        return UPDATA_NON;
    }

    printf("\n>>>>>>>>>>>>check update_path: %s\n", path);
    if (fs_update_set_file_path(path) == false) {
        return -1;
    }
    update_mode_info_t info = {
#if TCFG_UDISK_ENABLE
        .type = USB_UPDATA,
#else
#if TCFG_SD0_ENABLE
        .type = SD0_UPDATA,
#elif TCFG_SD1_ENABLE
        .type = SD1_UPDATA,
#endif
#endif
        .state_cbk = fs_update_state_cbk,
        .p_op_api = &fs_update_opt,
        .task_en = 0,
    };

#if CONFIG_DOUBLE_BANK_ENABLE
    register_update_percent_info_callback_handle(get_update_percent_info);
#endif

    app_active_update_task_init(&info);
    return 0;
}

static int update_test(void)
{
#if TCFG_UDISK_ENABLE
    const char *path = CONFIG_UDISK_ROOT_PATH\
                       CONFIG_UPGRADE_FILE_NAME;
#else
    const char *path = CONFIG_UPGRADE_PATH;
#endif
    return fs_update_check(path);
}

extern int storage_device_ready(void);
static void fs_update_start(void *priv)
{
    os_time_dly(300);
    while (!storage_device_ready()) {//等待文件系统挂载完成
        os_time_dly(30);
    }

    printf(">>>SD card is ready <<<");

    update_test();
}

void c_main(void)
{
    if (thread_fork("fs_update_start", 10, 512, 0, NULL, fs_update_start, NULL) != OS_NO_ERR) {
        printf("thread fork fail\n");
    }
}

late_initcall(c_main);

#endif
