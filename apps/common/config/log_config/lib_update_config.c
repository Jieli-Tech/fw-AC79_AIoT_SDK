
#include "app_config.h"
#include "system/includes.h"
#include "update_loader_download.h"

#ifdef CONFIG_256K_FLASH
const int config_update_mode = UPDATE_UART_EN | UPDATE_STORAGE_DEV_EN;
#else
#ifdef CONFIG_BT_ENABLE
const int config_update_mode = UPDATE_BT_LMP_EN | UPDATE_STORAGE_DEV_EN | UPDATE_BLE_TEST_EN | UPDATE_APP_EN | UPDATE_UART_EN;
#else
const int config_update_mode = UPDATE_STORAGE_DEV_EN | UPDATE_APP_EN | UPDATE_UART_EN;
#endif
#endif

//是否采用双备份升级方案:0-单备份;1-双备份
#if CONFIG_DOUBLE_BANK_ENABLE
const int support_dual_bank_update_en = 1;
#else
const int support_dual_bank_update_en = 0;
#endif  //CONFIG_DOUBLE_BANK_ENABLE

//是否支持外挂flash升级,需要打开Board.h中的TCFG_NOR_FS_ENABLE
const int support_norflash_update_en = 0;

//支持从外挂flash读取ufw文件升级使能
const int support_norflash_ufw_update_en = 0;

#if OTA_TWS_SAME_TIME_NEW       //使用新的同步升级流程
const int support_ota_tws_same_time_new = 1;
#else
const int support_ota_tws_same_time_new = 0;
#endif

//支持预留区域文件双备份升级
const int support_reaserved_zone_file_dual_bank_update_en = 1;

//支持被动升级新文件结构
const int support_passive_update_new_file_structure = 1;

//开启备份区空间不够时，支持资源强制写入预留区
const int support_reserved_zone_forced_update = 1;

//检测升级的文件是否与本地相同
const int update_file_check_same = 0;

/* ================================================================================
关于support_passive_update_new_file_structure和support_reaserved_zone_file_dual_bank_update_en配置说明:
1.双备份support_dual_bank_update_en变量使能后:
2.support_passive_update_new_file_structure使能后需要用新的isd_download()工具生成文件结构;
3.support_reaserved_zone_file_dual_bank_update_en改变量开的前提是support_passive_update_new_file_structure打开
================================================================================= */

//是否支持升级之后保留vm数据
const int support_vm_data_keep = 1;

const char log_tag_const_v_UPDATE AT(.LOG_TAG_CONST) = LIB_DEBUG & FALSE;
const char log_tag_const_i_UPDATE AT(.LOG_TAG_CONST) = LIB_DEBUG & TRUE;
const char log_tag_const_d_UPDATE AT(.LOG_TAG_CONST) = LIB_DEBUG & FALSE;
const char log_tag_const_w_UPDATE AT(.LOG_TAG_CONST) = LIB_DEBUG & TRUE;
const char log_tag_const_e_UPDATE AT(.LOG_TAG_CONST) = LIB_DEBUG & TRUE;
