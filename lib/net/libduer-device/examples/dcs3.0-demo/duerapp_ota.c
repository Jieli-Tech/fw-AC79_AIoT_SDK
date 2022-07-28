#include <string.h>
#include <stdlib.h>

#include "duerapp_config.h"
#include "lightduer_dcs.h"
#include "lightduer_memory.h"
#include "lightduer_types.h"
#include "lightduer_dev_info.h"
#include "lightduer_ota_updater.h"
#include "lightduer_ota_unpacker.h"
#include "lightduer_ota_notifier.h"
#include "lightduer_ota_installer.h"
#include "server/server_core.h"
#include "server/ai_server.h"
#include "update/net_update.h"

static char g_firmware_version[FIRMWARE_VERSION_LEN + 1];

static unsigned char update_succ_flag = 0;

typedef struct {
    duer_ota_updater_t *updater;
    void *update_fd;
    void *buffer;
    u32 remain;
    u32 w_offset;
    u32 firmware_size;
    duer_ota_module_info_t module_info;
} duer_ota_handler_t;

static duer_package_info_t s_package_info = {
    .product = "WIFI_STORY",
    .batch   = "12",
    .os_info = {
        .os_name        = "FreeRTOS",
        .developer      = "lyx",
        .os_version     = "0.0.0.0", // This version is that you write in the Duer Cloud
        .staged_version = "0.0.0.0", // This version is that you write in the Duer Cloud
    }
};

void set_duer_package_info(const char *product,
                           const char *batch,
                           const char *os_name,
                           const char *developer,
                           const char *os_version,
                           const char *staged_version)
{
    strncpy(s_package_info.product, product, NAME_LEN);
    s_package_info.product[NAME_LEN] = 0;
    strncpy(s_package_info.batch, batch, BATCH_LEN);
    s_package_info.batch[BATCH_LEN] = 0;
    strncpy(s_package_info.os_info.os_name, os_name, NAME_LEN);
    s_package_info.os_info.os_name[NAME_LEN] = 0;
    strncpy(s_package_info.os_info.developer, developer, NAME_LEN);
    s_package_info.os_info.developer[NAME_LEN] = 0;
    strncpy(s_package_info.os_info.os_version, os_version, FIRMWARE_VERSION_LEN);
    s_package_info.os_info.os_version[FIRMWARE_VERSION_LEN] = 0;
    strncpy(s_package_info.os_info.staged_version, staged_version, FIRMWARE_VERSION_LEN);
    s_package_info.os_info.staged_version[FIRMWARE_VERSION_LEN] = 0;
    strncpy(g_firmware_version, os_version, FIRMWARE_VERSION_LEN);
    g_firmware_version[FIRMWARE_VERSION_LEN] = 0;
}

#define PER_RECV_SIZE   (4 * 1024)

extern void JL_duer_upgrade_notify(int event, void *arg);

static int notify_ota_begin(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;
    duer_ota_handler_t *ota_handler = NULL;

    if (custom_data == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    DUER_LOGI("OTA Flash: Notify OTA begin");

    ota_handler = (duer_ota_handler_t *)custom_data;

    ota_handler->update_fd = net_fopen(CONFIG_UPGRADE_OTA_FILE_NAME, "w");
    if (ota_handler->update_fd == NULL) {
        DUER_LOGE("open upgrade server err \n");
        installer->err_msg = "Open upgrade Server Failed";
        return DUER_ERR_FAILED;
    }

    ota_handler->buffer = DUER_MALLOC(PER_RECV_SIZE);
    if (ota_handler->buffer == NULL) {
        DUER_LOGE("malloc upgrade buffer err \n");
        net_fclose(ota_handler->update_fd, 1);
        ota_handler->update_fd = NULL;
        installer->err_msg = "Malloc OTA Handler buffer failed";
        return DUER_ERR_MEMORY_OVERLOW;
    }

    /* JL_duer_upgrade_notify(AI_SERVER_EVENT_UPGRADE, NULL); */

    return DUER_OK;
}

static int get_module_info(
    duer_ota_installer_t *installer,
    void **custom_data,
    duer_ota_module_info_t const *info)
{
    int ret = DUER_OK;
    duer_ota_handler_t *ota_handler = NULL;

    if (custom_data == NULL || info == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    DUER_LOGI("OTA Flash: Module info");
    DUER_LOGI("Module name: %s", info->module_name);
    DUER_LOGI("Module size: %d", info->module_size);
    DUER_LOGI("Module signature: %s", info->module_signature);
    DUER_LOGI("Module version: %s", info->module_version);
    DUER_LOGI("Module HW version: %s", info->module_support_hardware_version);

    strncpy(g_firmware_version, (const char *)info->module_version, FIRMWARE_VERSION_LEN);
    g_firmware_version[FIRMWARE_VERSION_LEN] = 0;

    ota_handler = (duer_ota_handler_t *)DUER_CALLOC(1, sizeof(*ota_handler));
    if (ota_handler == NULL) {
        DUER_LOGE("OTA Unpack OPS: Malloc failed");
        installer->err_msg = "Malloc OTA Handler failed";
        return DUER_ERR_MEMORY_OVERLOW;
    }

    DUER_MEMMOVE(&ota_handler->module_info, info, sizeof(*info));

    ota_handler->firmware_size = info->module_size;

    *custom_data = ota_handler;

    return ret;
}

static int get_module_data(
    duer_ota_installer_t *installer,
    void *custom_data,
    unsigned int offset,
    unsigned char const *data,
    size_t size)
{
    duer_ota_handler_t *ota_handler = NULL;
    u32 w_size = 0;
    int ret = DUER_OK;
    u32 left = 0;

    if (installer == NULL || custom_data == NULL || data == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ota_handler = (duer_ota_handler_t *)custom_data;

    if (ota_handler->buffer == NULL) {
        return DUER_ERR_INVALID_PARAMETER;
    }

    if (ota_handler->remain + size >= PER_RECV_SIZE) {
        left = PER_RECV_SIZE - ota_handler->remain;
        memcpy(ota_handler->buffer + ota_handler->remain, data, left);
        w_size = PER_RECV_SIZE;
        ota_handler->remain = 0;
    } else {
        memcpy(ota_handler->buffer + ota_handler->remain, data, size);
        ota_handler->remain += size;
        return DUER_OK;
    }

    if (ota_handler->w_offset == 0) {

    }

    if (ota_handler->update_fd) {
        ret = net_fwrite(ota_handler->update_fd, ota_handler->buffer, w_size, 0);
        if (ret != w_size) {
            net_fclose(ota_handler->update_fd, 1);
            ota_handler->update_fd = NULL;
            DUER_LOGE("upgrade core error : %d", ret);
            installer->err_msg = "Write Flash Failed";
            return DUER_ERR_FAILED;
        }
    }
    ota_handler->w_offset += w_size;

    if (w_size == PER_RECV_SIZE) {
        memcpy(ota_handler->buffer, data + left, size - left);
        ota_handler->remain = (size - left);
    }

    return DUER_OK;
}

static int verify_module_data(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Verify module data");

    return ret;
}

static int update_img_begin(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Update image begin");

    return ret;
}

static int update_img(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Updating image");

    return ret;
}

static int verify_image(duer_ota_installer_t *installer, void *custom_data)
{
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Verify image");

    return ret;
}

static int notify_ota_end(duer_ota_installer_t *installer, void *custom_data)
{
    duer_ota_handler_t *ota_handler = NULL;
    int ret = DUER_OK;

    DUER_LOGI("OTA Flash: Notify OTA end");

    if (custom_data == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ota_handler = (duer_ota_handler_t *)custom_data;

    if (ota_handler->buffer == NULL) {
        return DUER_ERR_INVALID_PARAMETER;
    }

    if (ota_handler->remain && ota_handler->update_fd) {
        ret = net_fwrite(ota_handler->update_fd, ota_handler->buffer, ota_handler->remain, 0);
        if (ret != ota_handler->remain) {
            net_fclose(ota_handler->update_fd, 1);
            ota_handler->update_fd = NULL;
            DUER_LOGE("upgrade core error : %d", ret);
            installer->err_msg = "Final Write Flash Failed";
            return DUER_ERR_FAILED;
        }
        ota_handler->w_offset += ota_handler->remain;
    }

    if (ota_handler->update_fd) {
        update_succ_flag = 1;
        net_fclose(ota_handler->update_fd, 0);
        ota_handler->update_fd = NULL;
    }

    if (ota_handler->buffer) {
        DUER_FREE(ota_handler->buffer);
        ota_handler->buffer = NULL;
    }

    DUER_FREE(ota_handler);

    installer->custom_data = NULL;

    return DUER_OK;
}

static int cancel_ota_update(duer_ota_installer_t *installer, void *custom_data)
{
    duer_ota_handler_t *ota_handler = NULL;

    DUER_LOGI("OTA Flash: Cancel OTA update");

    if (custom_data == NULL || installer == NULL) {
        DUER_LOGE("OTA Flash: Argument error");

        return DUER_ERR_INVALID_PARAMETER;
    }

    ota_handler = (duer_ota_handler_t *)custom_data;

    if (ota_handler->update_fd) {
        net_fclose(ota_handler->update_fd, 1);
        ota_handler->update_fd = NULL;
    }

    if (ota_handler->buffer) {
        DUER_FREE(ota_handler->buffer);
        ota_handler->buffer = NULL;
    }

    update_succ_flag = 0;

    DUER_FREE(ota_handler);

    installer->custom_data = NULL;

    JL_duer_upgrade_notify(AI_SERVER_EVENT_UPGRADE_FAIL, NULL);

    return DUER_OK;
}

static int duer_ota_reboot_cb(void *arg)
{
    int ret = DUER_OK;
    DUER_LOGE("OTA Unpack OPS: Prepare to restart system");

    if (update_succ_flag) {
        char *msg = NULL;
        if (strlen(g_firmware_version) > 0) {
            msg = (char *)DUER_MALLOC(strlen(g_firmware_version) + 1);
            if (msg) {
                strcpy(msg, g_firmware_version);
            }
        }
        JL_duer_upgrade_notify(AI_SERVER_EVENT_UPGRADE_SUCC, msg);
    } else {
        JL_duer_upgrade_notify(AI_SERVER_EVENT_UPGRADE_FAIL, NULL);
    }

    return ret;
}

static int duer_ota_register_installer(void)
{
    int ret = DUER_OK;

    duer_ota_installer_t *installer = NULL;

    installer = duer_ota_installer_create_installer("JL_OTA.bin");
    if (installer == NULL) {
        DUER_LOGE("OTA Flash: Create installer failed");

        return DUER_ERR_FAILED;
    }

    installer->custom_data 		  = NULL;
    installer->notify_ota_begin   = notify_ota_begin;
    installer->notify_ota_end     = notify_ota_end;
    installer->get_module_info    = get_module_info;
    installer->get_module_data    = get_module_data;
    installer->verify_module_data = verify_module_data;
    installer->update_image_begin = update_img_begin;
    installer->update_image       = update_img;
    installer->verify_image       = verify_image;
    installer->cancel_ota_update  = cancel_ota_update;

    ret = duer_ota_installer_register_installer(installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Flash: Register installer failed");

        ret = duer_ota_installer_destroy_installer(installer);
        if (ret != DUER_OK) {
            DUER_LOGE("OTA Flash: Destroy installer failed");
        }

        return DUER_ERR_FAILED;
    }

    return ret;
}

static int duer_ota_unregister_installer(void)
{
    int ret = DUER_OK;
    duer_ota_installer_t *installer = NULL;

    installer = duer_ota_installer_get_installer("JL_OTA.bin");
    if (installer == NULL) {
        DUER_LOGE("OTA Flash: Get installer failed");

        return DUER_ERR_FAILED;
    }

    ret = duer_ota_installer_unregister_installer("JL_OTA.bin");
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Flash: Unregister installer failed");

        return DUER_ERR_FAILED;
    }

    ret = duer_ota_installer_destroy_installer(installer);
    if (ret != DUER_OK) {
        DUER_LOGE("OTA Flash: Destroy installer failed");

        return DUER_ERR_FAILED;
    }

    return ret;
}

static const duer_ota_init_ops_t ota_ops = {
    .register_installer = duer_ota_register_installer,
    .unregister_installer = duer_ota_unregister_installer,
    .reboot = duer_ota_reboot_cb,
};


static int get_package_info(duer_package_info_t *info)
{
    int ret = DUER_OK;
    char firmware_version[FIRMWARE_VERSION_LEN + 1];

    if (info == NULL) {
        DUER_LOGE("Argument Error");

        ret = DUER_ERR_INVALID_PARAMETER;

        goto out;
    }

    memset(firmware_version, 0, sizeof(firmware_version));

    ret = duer_get_firmware_version(firmware_version, FIRMWARE_VERSION_LEN);
    if (ret != DUER_OK) {
        DUER_LOGE("Get firmware version failed");

        goto out;
    }

    strncpy((char *)&s_package_info.os_info.os_version,
            firmware_version,
            FIRMWARE_VERSION_LEN + 1);
    memcpy(info, &s_package_info, sizeof(*info));

out:
    return ret;
}

static const duer_package_info_ops_t s_package_info_ops = {
    .get_package_info = get_package_info,
};

static int get_firmware_version(char *firmware_version)
{
    strncpy(firmware_version, g_firmware_version, FIRMWARE_VERSION_LEN);

    return DUER_OK;
}

static const struct DevInfoOps dev_info_ops = {
    .get_firmware_version = get_firmware_version,
};

int duer_app_ota_init(void)
{
    int ret = DUER_OK;

    ret = duer_register_device_info_ops(&dev_info_ops);
    if (ret != DUER_OK) {
        DUER_LOGE("Register device info ops failed");
    }

    ret = duer_report_device_info();
    if (ret != DUER_OK) {
        DUER_LOGE("Report device info failed");
    }

    ret = duer_init_ota(&ota_ops);
    if (ret != DUER_OK) {
        DUER_LOGE("Init OTA failed");
    }

    ret = duer_ota_register_package_info_ops(&s_package_info_ops);
    if (ret != DUER_OK) {
        DUER_LOGE("Register OTA package info ops failed");
    }

    ret = duer_ota_notify_package_info();
    if (ret != DUER_OK) {
        DUER_LOGE("Report package info failed");
    }

    return ret;
}
