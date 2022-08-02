/**
 * @file tuya_os_adapt_ota.c
 * @brief ota底层操作接口
 *
 * @copyright Copyright (c) {2018-2020} 涂鸦科技 www.tuya.com
 *
 */
#include "asm/cpu.h"
#include "tuya_os_adapt_ota.h"
#include "tuya_os_adapter_error_code.h"
#include "tuya_os_adapt_system.h"
#include "update/net_update.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
static void *update_fd = NULL;

/***********************************************************
*************************variable define********************
***********************************************************/
static const TUYA_OS_OTA_INTF m_tuya_os_ota_intfs = {
    .start      = tuya_os_adapt_ota_start_inform,
    .process    = tuya_os_adapt_ota_data_process,
    .end        = tuya_os_adapt_ota_end_inform,
};

/***********************************************************
*************************function define********************
***********************************************************/
/**
 * @brief 升级开始通知函数
 *
 * @param[in] file_size 升级固件大小
 * @param[in] type      升级类型
 *            AIR_OTA_TYPE     远程升级
              UART_OTA_TYPE    串口升级
 * @retval  =0      成功
 * @retval  <0      错误码
 */
//extern char USER_SW_VER[16];
int tuya_os_adapt_ota_start_inform(unsigned int file_size, OTA_TYPE type)
{
    if (UART_OTA_TYPE == type) {
        tuya_os_adapt_output_log("tuya_os_adapt_ota_start_inform[UART_OTA_TYPE] : Not supported yet!\n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    if (file_size == 0) {
        tuya_os_adapt_output_log("tuya_os_adapt_ota_start_inform : PARM ERROR!\n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    update_fd = net_fopen(CONFIG_UPGRADE_OTA_FILE_NAME, "w");
    if (!update_fd) {
        tuya_os_adapt_output_log("tuya_os_adapt_ota_start_inform : net_fopen ERROR!\n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief ota数据包处理
 *
 * @param[in] total_len ota升级包总大小
 * @param[in] offset 当前data在升级包中的偏移
 * @param[in] data ota数据buffer指针
 * @param[in] len ota数据buffer长度
 * @param[out] remain_len 内部已经下发但该函数还未处理的数据长度
 * @param[in] pri_data 保留参数
 *
 * @retval  =0      成功
 * @retval  <0      错误码
 */
int tuya_os_adapt_ota_data_process(const unsigned int total_len, const unsigned int offset,
                                   const unsigned char *data, const unsigned int len, unsigned int *remain_len, void *pri_data)
{
    char sock_err = 0;
    int w_len;

    if (NULL == update_fd) {
        tuya_os_adapt_output_log("tuya_os_adapt_ota_data_process, PARM ERROR!");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    w_len = net_fwrite(update_fd, data, len, 0);
    if (w_len < 0) {
        tuya_os_adapt_output_log("tuya_os_adapt_ota_data_process, net_fwrite ERROR!");
        net_fclose(update_fd, 1);
        return OPRT_OS_ADAPTER_OTA_PROCESS_FAILED;
    }

    *remain_len = len - w_len;

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief 固件ota数据传输完毕通知
 *        用户可以做固件校验以及设备重启
 * param[in]        reset       是否需要重启
 * @retval  =0      成功
 * @retval  <0      错误码
 */
int tuya_os_adapt_ota_end_inform(bool reset)
{
    if (NULL != update_fd && reset) {
        net_fclose(update_fd, 0);
    }

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief tuya_os_adapt_reg_ota_intf 接口注册
 * @return int
 */
int tuya_os_adapt_reg_ota_intf(void)
{
    return tuya_os_adapt_reg_intf(INTF_OTA, (void *)&m_tuya_os_ota_intfs);
}

