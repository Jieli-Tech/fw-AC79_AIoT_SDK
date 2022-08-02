#include "system/includes.h"
#include "app_config.h"
#include "update/update.h"
#include "fs/fs.h"
#include "update/update_loader_download.h"
#include "update/net_update.h"

#ifdef CONFIG_NET_ENABLE

#include "http/http_cli.h"

#define PER_RECV_SIZE   (8 * 1024)

static int __httpcli_cb(void *ctx, void *buf, unsigned int size, void *priv, httpin_status status)
{
    return 0;
}

int get_update_data(const char *url)
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

    httpcli_ctx *ctx = (httpcli_ctx *)calloc(1, sizeof(httpcli_ctx));
    if (NULL == ctx) {
        return -1;
    }

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
                goto __exit;
            }
            offset += ret;
        } while (remain != offset);

        if (data_offset == 0) {
            os_time_dly(500);	//此处延时是为了避免播放提示音时刷写flash导致卡音问题
        }

        ret = net_fwrite(update_fd, buffer, offset, 0);
        if (ret != offset) {
            log_e("upgrade core error : 0x%x\n", ret);
            goto __exit;
        }
        data_offset += offset;
        offset = 0;
    }

    error = 0;

__exit:
    if (buffer) {
        free(buffer);
    }
    if (update_fd) {
        net_fclose(update_fd, error);
    }
    ops->close(ctx);
    free(ctx);

    return error;
}

#endif

