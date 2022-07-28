/*
 * Tencent is pleased to support the open source community by making IoT Hub
 available.
 * Copyright (C) 2018-2020 THL A29 Limited, a Tencent company. All rights
 reserved.

 * Licensed under the MIT License (the "License"); you may not use this file
 except in
 * compliance with the License. You may obtain a copy of the License at
 * http://opensource.org/licenses/MIT

 * Unless required by applicable law or agreed to in writing, software
 distributed under the License is
 * distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 KIND,
 * either express or implied. See the License for the specific language
 governing permissions and
 * limitations under the License.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "qcloud_iot_export.h"
#include "qcloud_iot_import.h"
#include "utils_param_check.h"
#include "tvs_api_config.h"
#include "asm/crc16.h"
#include "asm/sfc_norflash_api.h"

/* Enable this macro (also control by cmake) to use static string buffer to
 * store device info */
/* To use specific storing methods like files/flash, disable this macro and
 * implement dedicated methods */

#ifdef DEBUG_DEV_INFO_USED
/* product Id  */
static char sg_product_id[MAX_SIZE_OF_PRODUCT_ID + 1] =  "BWBTAK8PMS";	//请跟腾讯方获取

/* device name */
#if USE_MAC_DEVICE_NAME
static char sg_device_name[MAX_SIZE_OF_DEVICE_NAME + 1] = "";	//请跟腾讯方获取
#else
static char sg_device_name[MAX_SIZE_OF_DEVICE_NAME + 1] = "51595800000A";	//请跟腾讯方获取
#endif

/* region */
static char sg_region[MAX_SIZE_OF_REGION + 1] = "china";

#ifdef DEV_DYN_REG_ENABLED
/* product secret for device dynamic Registration  */
static char sg_product_secret[MAX_SIZE_OF_PRODUCT_SECRET + 1] = "BpDWMc0voCaVwrD82xVbFNWD";	//请跟腾讯方获取
#endif

#ifdef AUTH_MODE_CERT
/* public cert file name of certificate device */
static char sg_device_cert_file_name[MAX_SIZE_OF_DEVICE_CERT_FILE_NAME + 1] = "YOUR_DEVICE_NAME_cert.crt";
/* private key file name of certificate device */
static char sg_device_privatekey_file_name[MAX_SIZE_OF_DEVICE_SECRET_FILE_NAME + 1] = "YOUR_DEVICE_NAME_private.key";
#else
/* device secret of PSK device */
#if USE_MAC_DEVICE_NAME
static char sg_device_secret[MAX_SIZE_OF_DEVICE_SECRET + 1] =   "YOUR_IOT_PSK";	//请跟腾讯方获取
#else
static char sg_device_secret[MAX_SIZE_OF_DEVICE_SECRET + 1] =   "euUkNITZG6HiYqr6qOl01w==";// "YOUR_IOT_PSK";	//请跟腾讯方获取
#endif

#endif

/* tvs pid */
static char sg_tvs_pid[MAX_SIZE_OF_TVS_PRODUCT_ID + 1] = "fc694160768011eb84241ffd26f47136:a9c8308c077848d9b58b4273f61077b1";	//请跟腾讯方获取
//"ff4c73700dee11eba6785bb42856a755:bade1292e59e4e7eba846d990e034d93";
#ifdef GATEWAY_ENABLED
/* sub-device product id  */
static char sg_sub_device_product_id[MAX_SIZE_OF_PRODUCT_ID + 1] = "PRODUCT_ID";
/* sub-device device name */
static char sg_sub_device_name[MAX_SIZE_OF_DEVICE_NAME + 1] = "YOUR_SUB_DEV_NAME";
#endif

#if USE_MAC_DEVICE_NAME
void get_mac_device_name(void)
{
    extern int wifi_get_mac(u8 * mac);
    u8 mac_addr[6];
    u8 mac_name[12] = {0};
    wifi_get_mac(mac_addr);
    sprintf(mac_name, "%02X%02X%02X%02X%02X%02X", mac_addr[0],
            mac_addr[1],
            mac_addr[2],
            mac_addr[3],
            mac_addr[4],
            mac_addr[5]);
    if (!strcmp(sg_device_name, mac_name)) {
        return ;
    }
    memcpy(sg_device_name, mac_name, 12);

}


static void save_psk_info(void)
{
    printf("%s\r\n", __func__);

    u32 addr;
    int len = MAX_SIZE_OF_DEVICE_SECRET;

    u8 *psk_data = (u8 *)calloc(2, 1024);
    if (!psk_data) {
        printf("malloc read psk buf failed ,%s-----%d", __func__, __LINE__);
        return ;
    }

    FILE *profile_fp = fopen(TENCENT_PATH, "r");
    if (profile_fp == NULL) {
        free(psk_data);
        return ;
    }
    struct vfs_attr file_attr;
    fget_attrs(profile_fp, &file_attr);
    addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    fclose(profile_fp);

    //preference 8k | (account_info  1k  psk_info 1k) |
    addr += 8 * 1024;

    norflash_read(NULL, psk_data, 2 * 1024, addr);

    //擦除flash 4k
    norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, addr);

    u16 *psk_crc = (u16 *)&psk_data[1024 + 0];
    u16 *psk_len = (u16 *)&psk_data[1024 + 2];

    memcpy(psk_data + 1024 + 4, sg_device_secret, len);
    *psk_len = (u16)len;
    *psk_crc = CRC16((const void *)sg_device_secret, len);
    //写flash
    norflash_write(NULL, psk_data,  2 * 1024, addr);

    free(psk_data);

}

static int load_psk_info(void)
{
    printf("%s", __func__);

    u8 ret = -1;
    u32 addr;

    u8 *psk_data = (u8 *)calloc(1, 1024);
    if (!psk_data) {
        printf("malloc read auth buf failed ,%s-----%d", __func__, __LINE__);
        return -1;
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
    addr += 9 * 1024;

    //读取psk_info的信息
    norflash_read(NULL, psk_data, 1 * 1024, addr);

    u16 *psk_crc = (u16 *)&psk_data[0];
    u16 *psk_len = (u16 *)&psk_data[2];
    if (*psk_len != MAX_SIZE_OF_DEVICE_SECRET) {
        printf("-------------%s-----------------%d  no data , psk_len = %d \n", __func__, __LINE__, *psk_len);
        goto exit;
    }

    //yii:CRC校验比较
    if (*psk_crc != CRC16((const void *)&psk_data[4], (u32)*psk_len)) {
        printf("----------%s--------%d-------CRC16 failed", __func__, __LINE__);
        goto exit;
    }
    memcpy(sg_device_secret, &psk_data[4], *psk_len);

    ret = 0;
exit:
    if (psk_data) {
        free(psk_data);
    }
    return ret;

}

u8 is_psk_available(void)
{
    if (strcmp(sg_device_secret, "YOUR_IOT_PSK")) {
        return 1;
    }
    if (!load_psk_info()) {
        return 1;
    }
    return 0;
}
#endif

static int device_info_copy(void *pdst, void *psrc, uint8_t max_len)
{
    if (strlen(psrc) > max_len) {
        return QCLOUD_ERR_FAILURE;
    }
    memset(pdst, '\0', max_len);
    strncpy(pdst, psrc, max_len);
    return QCLOUD_RET_SUCCESS;
}

#endif

int HAL_SetDevInfo(void *pdevInfo)
{
    POINTER_SANITY_CHECK(pdevInfo, QCLOUD_ERR_DEV_INFO);
    int         ret;
    DeviceInfo *devInfo = (DeviceInfo *)pdevInfo;

#ifdef DEBUG_DEV_INFO_USED
    ret = device_info_copy(sg_product_id, devInfo->product_id,
                           MAX_SIZE_OF_PRODUCT_ID);  // set product ID
    ret |= device_info_copy(sg_device_name, devInfo->device_name,
                            MAX_SIZE_OF_DEVICE_NAME);  // set dev name

#ifdef AUTH_MODE_CERT
    ret |= device_info_copy(sg_device_cert_file_name, devInfo->dev_cert_file_name,
                            MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);  // set dev cert file name
    ret |= device_info_copy(sg_device_privatekey_file_name, devInfo->dev_key_file_name,
                            MAX_SIZE_OF_DEVICE_SECRET_FILE_NAME);  // set dev key file name
#else
    ret |= device_info_copy(sg_device_secret, devInfo->device_secret,
                            MAX_SIZE_OF_DEVICE_SECRET);  // set dev secret
#if USE_MAC_DEVICE_NAME
    save_psk_info();
#endif
#endif

#else
    Log_e("HAL_SetDevInfo not implement yet");
    ret = QCLOUD_ERR_DEV_INFO;
#endif

    if (QCLOUD_RET_SUCCESS != ret) {
        Log_e("Set device info err");
        ret = QCLOUD_ERR_DEV_INFO;
    }
    return ret;
}

int HAL_GetDevInfo(void *pdevInfo)
{
    POINTER_SANITY_CHECK(pdevInfo, QCLOUD_ERR_DEV_INFO);
    int         ret;
    DeviceInfo *devInfo = (DeviceInfo *)pdevInfo;
    memset((char *)devInfo, '\0', sizeof(DeviceInfo));

#ifdef DEBUG_DEV_INFO_USED
    ret = device_info_copy(devInfo->product_id, sg_product_id,
                           MAX_SIZE_OF_PRODUCT_ID);  // get product ID
    ret |= device_info_copy(devInfo->device_name, sg_device_name,
                            MAX_SIZE_OF_DEVICE_NAME);  // get dev name
    ret |= device_info_copy(devInfo->region, sg_region,
                            MAX_SIZE_OF_REGION);  // get region

#ifdef DEV_DYN_REG_ENABLED
    ret |= device_info_copy(devInfo->product_secret, sg_product_secret,
                            MAX_SIZE_OF_PRODUCT_SECRET);  // get product ID
#endif

#ifdef AUTH_MODE_CERT
    ret |= device_info_copy(devInfo->dev_cert_file_name, sg_device_cert_file_name,
                            MAX_SIZE_OF_DEVICE_CERT_FILE_NAME);  // get dev cert file name
    ret |= device_info_copy(devInfo->dev_key_file_name, sg_device_privatekey_file_name,
                            MAX_SIZE_OF_DEVICE_SECRET_FILE_NAME);  // get dev key file name
#else
    ret |= device_info_copy(devInfo->device_secret, sg_device_secret,
                            MAX_SIZE_OF_DEVICE_SECRET);  // get dev secret
#endif

#else
    Log_e("HAL_GetDevInfo not implement yet");
    ret = QCLOUD_ERR_DEV_INFO;
#endif

    if (QCLOUD_RET_SUCCESS != ret) {
        Log_e("Get device info err");
        ret = QCLOUD_ERR_DEV_INFO;
    }
    return ret;
}

#ifdef GATEWAY_ENABLED
int HAL_GetGwDevInfo(void *pgwDeviceInfo)
{
    POINTER_SANITY_CHECK(pgwDeviceInfo, QCLOUD_ERR_DEV_INFO);
    int                ret;
    GatewayDeviceInfo *gwDevInfo = (GatewayDeviceInfo *)pgwDeviceInfo;
    memset((char *)gwDevInfo, 0, sizeof(GatewayDeviceInfo));

#ifdef DEBUG_DEV_INFO_USED
    ret = HAL_GetDevInfo(&(gwDevInfo->gw_info));  // get gw dev info
    // only one sub-device is supported now
    gwDevInfo->sub_dev_num  = 1;
    gwDevInfo->sub_dev_info = (DeviceInfo *)HAL_Malloc(sizeof(DeviceInfo) * (gwDevInfo->sub_dev_num));
    memset((char *)gwDevInfo->sub_dev_info, '\0', sizeof(DeviceInfo));
    // copy sub dev info
    ret = device_info_copy(gwDevInfo->sub_dev_info->product_id, sg_sub_device_product_id, MAX_SIZE_OF_PRODUCT_ID);
    ret |= device_info_copy(gwDevInfo->sub_dev_info->device_name, sg_sub_device_name, MAX_SIZE_OF_DEVICE_NAME);

#else
    Log_e("HAL_GetDevInfo not implement yet");
    ret = QCLOUD_ERR_DEV_INFO;
#endif

    if (QCLOUD_RET_SUCCESS != ret) {
        Log_e("Get gateway device info err");
        ret = QCLOUD_ERR_DEV_INFO;
    }
    return ret;
}
#endif

int HAL_GetTvsInfo(char       *tvs_productId, char *tvs_dsn)
{
    strncpy(tvs_productId, sg_tvs_pid, MAX_SIZE_OF_TVS_PRODUCT_ID);
    HAL_Snprintf(tvs_dsn, MAX_SIZE_OF_TVS_DEVICE_NAME, "%s_%s", sg_product_id, sg_device_name);

    return QCLOUD_RET_SUCCESS;
}

int HAL_SetTvsInfo(char       *tvs_productId, char *tvs_dsn)
{
    memset(sg_tvs_pid, 0, MAX_SIZE_OF_TVS_PRODUCT_ID);
    strncpy(sg_tvs_pid, tvs_productId, MAX_SIZE_OF_TVS_PRODUCT_ID);

    return QCLOUD_RET_SUCCESS;
}

