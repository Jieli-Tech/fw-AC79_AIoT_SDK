#include "system/os/os_api.h"
#include "server/server_core.h"
#include "echo_cloud.h"
#include "json_c/json_tokener.h"
#include "server/ai_server.h"
#include "update/net_update.h"

static int __httpcli_cb(void *ctx, void *buf, unsigned int size, void *priv, httpin_status status)
{
    return 0;
}

#define PER_RECV_SIZE   (16 * 1024)

int echo_cloud_get_firmware_data(httpcli_ctx *ctx, const char *url, const char *version)
{
    int error = 0;
    int ret = 0;
    int offset = 0;
    int remain = 0;
    void *fd = NULL;
    u8 *buffer = NULL;
    int data_offset = 0;
    int total_len = 0;
    const struct net_download_ops *ops = &http_ops;
    void *update_fd = NULL;

    memset(ctx, 0, sizeof(httpcli_ctx));

    ctx->url = url;
    ctx->connection = "close";
    ctx->timeout_millsec = 10000;
    ctx->cb = __httpcli_cb;

    error = ops->init(ctx);
    if (error != HERROR_OK) {
        goto __exit;
    }

    error = -1;
    update_fd = net_fopen(CONFIG_UPGRADE_OTA_FILE_NAME, "w");
    if (!update_fd) {
        goto __exit;
    }

    buffer = (u8 *)malloc(PER_RECV_SIZE);
    if (!buffer) {
        goto __exit;
    }

    total_len = ctx->content_length;
    if (total_len <= 0) {
        goto __exit;
    }

    JL_echo_cloud_upgrade_notify(AI_SERVER_EVENT_UPGRADE, NULL);

    while (total_len > 0) {
        if (total_len >= PER_RECV_SIZE) {
            remain = PER_RECV_SIZE;
            total_len -= PER_RECV_SIZE;
        } else {
            remain = total_len;
            total_len = 0;
        }

        do {
            ret = ops->read(ctx, (char *)buffer + offset, remain - offset);
            if (ret < 0) {
                log_e("ota update fail\n");
                goto __exit;
            }
            offset += ret;
        } while (remain != offset);

        if (data_offset == 0) {
            os_time_dly(500);
        }

        ret = net_fwrite(update_fd, buffer, offset, 0);
        if (ret != offset) {
            goto __exit;
        }

        data_offset += offset;
        offset = 0;
    }

    char *msg = NULL;
    if (version && strlen(version) > 0) {
        msg = (char *)malloc(strlen(version) + 1);
        if (msg) {
            strcpy(msg, version);
        }
    }
    JL_echo_cloud_upgrade_notify(AI_SERVER_EVENT_UPGRADE_SUCC, msg);

    error = 0;

__exit:
    if (error) {
        JL_echo_cloud_upgrade_notify(AI_SERVER_EVENT_UPGRADE_FAIL, NULL);
    }
    if (buffer) {
        free(buffer);
    }
    if (update_fd) {
        net_fclose(update_fd, error);
    }
    ops->close(ctx);

    return error;
}

