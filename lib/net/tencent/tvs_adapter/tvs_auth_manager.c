#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_api/tvs_api.h"
#include "tvs_authorize.h"

#include "os_wrapper.h"
#include "tvs_auth_manager.h"
#include "tvs_api_config.h"
#include "qcloud_iot_export.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"
#include "asm/crc16.h"
#include "device/device.h"
#include "wifi/wifi_connect.h"
#include "tvs_api_config.h"

int tvs_get_devInfo(char       *tvs_productId, char *tvs_dsn)
{
    return HAL_GetTvsInfo(tvs_productId, tvs_dsn);
}

int tvs_set_devInfo(char       *tvs_productId, char *tvs_dsn)
{
    return HAL_SetTvsInfo(tvs_productId, tvs_dsn);
}

void save_auth_info(char *account_info, int len)
{
    //将account信息持久化，以便在之后每次重启时初始化SDK
    // account_info是个Json字符串，len为字符串长度+1

    printf("%s", __func__);

    u32 addr;

    u8 *auth_data = (u8 *)calloc(2, 1024);
    if (!auth_data) {
        return;
    }

    FILE *profile_fp = fopen(TENCENT_PATH, "r");
    if (profile_fp == NULL) {
        free(auth_data);
        return;
    }
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);
    //preference 8k | (account_info  1k  psk_info 1k) |
    addr += 8 * 1024;

    norflash_read(NULL, auth_data, 2 * 1024, addr);

    norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, addr);


    u16 *auth_crc = (u16 *)&auth_data[0];
    u16 *auth_len = (u16 *)&auth_data[2];

    memcpy(auth_data + 4, account_info, len);
    *auth_len = (u16)len;
    *auth_crc = CRC16((const void *)account_info, len);
    //写flash
    norflash_write(NULL, auth_data, 2 * 1024, addr);
    free(auth_data);
}

static char *load_auth_info(int *len)
{
    //读取之前持久化的account信息，如果是首次开机或者未配对，account信息返回NULL，终端将根据传入的各种ID/DSN信息生成新的account信息
    printf("%s", __func__);

    u32 addr;
    char *auth_buffer = NULL;
    *len = 0;

    u8 *auth_data = (u8 *)calloc(1, 1024);
    if (!auth_data) {
        return NULL;
    }

    FILE *profile_fp = fopen(TENCENT_PATH, "r");
    if (profile_fp == NULL) {
        goto exit;
    }
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);
    //preference 8k | (account_info  1k  psk_info 1k) |
    addr += 8 * 1024;

    norflash_read(NULL, auth_data, 1 * 1024, addr);

    u16 *auth_crc = (u16 *)&auth_data[0];
    u16 *auth_len = (u16 *)&auth_data[2];
    if (*auth_len > 1 * 1024) {
        printf("-------------%s-----------------%d  no data , auth_len = %d \n", __func__, __LINE__, *auth_len);
        goto exit;
    }

    //yii:CRC校验比较
    if (*auth_crc != CRC16((const void *)&auth_data[4], (u32)*auth_len)) {
        printf("----------%s--------%d-------CRC16 failed", __func__, __LINE__);
        goto exit;
    }
    auth_buffer = (char *)calloc(1, (u32) * auth_len);
    if (auth_buffer) {
        *len = *auth_len;
        memcpy(auth_buffer, &auth_data[4], *len);
    }


exit:
    free(auth_data);
    return auth_buffer;
}

void my_authorize_callback(bool ok, char *account_info, int len, const char *client_id, int error)
{
    if (ok) {
        TVS_ADAPTER_PRINTF("tvs authorize success\n");
        printf("save auth = %s   ,len = %d", account_info, len);
        save_auth_info(account_info, len);
    } else {
        // 授权失败
        TVS_ADAPTER_PRINTF("tvs authorize failed\n");
    }
}

static void process_authorize_ret(int ret)
{
    if (ret == 0) {
        // 开始执行授权，授权结果将通过my_authorize_callback回调
    } else if (ret == TVS_API_ERROR_NETWORK_INVALID) {
        // 网络未连接，在网络重连之后会自动启动授权
    } else {
        // 未发起授权
        my_authorize_callback(false, NULL, 0, "", ret);
    }
}

void init_authorize_guest(const char *produce_id, const char *dsn)
{
    int auth_len = 0;
    // 加载之前存储的授权信息，可以返回NULL
    char *authorize_info = load_auth_info(&auth_len);

    tvs_authorize_manager_initalize(produce_id, dsn,
                                    authorize_info, auth_len, my_authorize_callback);
    // 发起访客授权
    int ret = tvs_authorize_manager_guest_login();

    process_authorize_ret(ret);
}

//yii:设备授权
void init_authorize_normal(const char *produce_id, const char *dsn)
{
    int auth_len = 0;
    // 加载之前存储的授权信息，可以返回NULL
    char *authorize_info = load_auth_info(&auth_len);
    tvs_authorize_manager_initalize(produce_id, dsn,
                                    authorize_info, auth_len, my_authorize_callback);			//yii:从参数中获取数据存到RAM中，前提参数是要有有效数据

    if (authorize_info == NULL || auth_len == 0) {
        // 未授权，需要与手机APP连接并下发client ID，再发起设备授权
        TVS_ADAPTER_PRINTF("this device is not authorized\n");
        return;
    }
    // 发起设备授权
    int ret = tvs_authorize_manager_login();

    process_authorize_ret(ret);
}

void init_authorize_on_boot()
{
    char product_id[MAX_SIZE_OF_TVS_PRODUCT_ID + 1];
    char dsn[MAX_SIZE_OF_TVS_DEVICE_NAME + 1];

    if (tvs_get_devInfo(product_id, dsn) < 0) {
        printf("tvs_get_devInfo err");
        return;
    }

    printf("product_id:%s dsn:%s\n", product_id, dsn);

    if (CONFIG_USE_GUEST_AUTHORIZE) {
        // 访客授权
        init_authorize_guest(product_id, dsn);
    } else {
        // 设备授权
        init_authorize_normal(product_id, dsn);
    }
}

// 通过client id授权
void start_authorize_with_client_id(const char *new_client_id)
{
    if (new_client_id == NULL || strlen(new_client_id) == 0) {
        return;
    }

    if (!CONFIG_USE_GUEST_AUTHORIZE) {
        // 首先注销
        tvs_authorize_manager_logout();
        // 重新设置账号
        tvs_authorize_manager_set_client_id(new_client_id);
        // 发起设备授权
        int ret = tvs_authorize_manager_login();

        process_authorize_ret(ret);
    }
}

//通过厂商账号授权(需要厂商手机app接入我们DMSDK)
void start_authorize_with_manuf(const char *client_id, const char *authRespInfo)
{
    if (client_id == NULL || strlen(client_id) == 0) {
        printf("%s error,client is null!!!", __func__);
        return;
    }

    if (authRespInfo == NULL || strlen(authRespInfo) == 0) {
        printf("%s error,authRespInfo is null!!!", __func__);
        return;
    }
    // 首先注销
    tvs_authorize_manager_logout();
    // 重新设置账号
    if (0 != tvs_authorize_manager_set_manuf_client_id(client_id, authRespInfo)) {
        return ;
    }
    // 发起设备授权
    int ret = tvs_authorize_manager_login();

    process_authorize_ret(ret);
}

