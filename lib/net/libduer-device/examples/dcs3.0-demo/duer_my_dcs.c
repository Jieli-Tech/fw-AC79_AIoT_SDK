#include "printf.h"
#include <stdlib.h>
#include <string.h>
#include "lightduer_dcs.h"
#include "lightduer_dcs_local.h"
#include "lightduer_dcs_router.h"
#include "lightduer_connagent.h"
#include "lightduer_log.h"
#include "lightduer_memory.h"
#include "lightduer_lib.h"
#include "duerapp_event.h"
#include "os/os_compat.h"

static struct {
    u8 playnext;
    u8 playprev;
    u8 power;
    u8 pause;
} my_dcs_status = { 0 };

extern u8 get_asr_app_volume(void);
extern void set_asr_app_volume(u8 volume);

static duer_status_t duer_volume_response(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    char volume_data[4];
    duer_handler handler = (duer_handler)ctx;
    DUER_MEMSET(volume_data, 0, sizeof(volume_data));

    if (handler && msg) {
        int msg_code;
        if (msg->msg_code == DUER_MSG_REQ_GET) {
            DUER_LOGI("volume DUER_MSG_REQ_GET");
            /* sprintf(volume_data, "%d", get_asr_app_volume()); */
            msg_code = DUER_MSG_RSP_CONTENT;
            duer_response(msg, msg_code, volume_data, strlen(volume_data));
        } else if (msg->msg_code == DUER_MSG_REQ_PUT) {
            DUER_LOGI("volume DUER_MSG_REQ_PUT");
            if (msg->payload && msg->payload_len > 0) {
                char *action = (char *)DUER_MALLOC(msg->payload_len + 1);
                if (action) {
                    DUER_MEMCPY(action, msg->payload, msg->payload_len);
                    action[msg->payload_len] = '\0';
                    DUER_MEMCPY(volume_data, action, sizeof(volume_data));
                    /* set_asr_app_volume(atoi(action) * 2); */
                    DUER_FREE(action);
                    msg_code = DUER_MSG_RSP_CONTENT;
                    duer_response(msg, msg_code, volume_data, strlen(volume_data));
                } else {
                    msg_code = DUER_MSG_RSP_FORBIDDEN;
                    duer_response(msg, msg_code, NULL, 0);
                }
            } else {
                msg_code = DUER_MSG_RSP_BAD_REQUEST;
                duer_response(msg, msg_code, NULL, 0);
            }
        } else {
            msg_code = DUER_MSG_RSP_METHOD_NOT_ALLOWED;
            duer_response(msg, msg_code, NULL, 0);
        }
    } else {
        DUER_LOGE("TODO what's the purpose of the parameter ctx here??");
    }
    return DUER_OK;
}

static duer_status_t duer_next_response(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    duer_handler handler = (duer_handler)ctx;
    u8 temp = '0';

    if (handler && msg) {
        int msg_code;
        if (msg->msg_code == DUER_MSG_REQ_GET) {
            DUER_LOGI("next DUER_MSG_REQ_GET");
            msg_code = DUER_MSG_RSP_CONTENT;
            duer_response(msg, msg_code, &temp, 1);
        } else if (msg->msg_code == DUER_MSG_REQ_PUT) {
            DUER_LOGI("next DUER_MSG_REQ_PUT");
            if (msg->payload && msg->payload_len > 0) {
                os_taskq_post("duer_app_task", 1, DUER_NEXT_SONG) ;
                msg_code = DUER_MSG_RSP_CONTENT;
                duer_response(msg, msg_code, &temp, 1);
            } else {
                msg_code = DUER_MSG_RSP_BAD_REQUEST;
                duer_response(msg, msg_code, NULL, 0);
            }
        } else {
            msg_code = DUER_MSG_RSP_METHOD_NOT_ALLOWED;
            duer_response(msg, msg_code, NULL, 0);
        }
    } else {
        DUER_LOGE("TODO what's the purpose of the parameter ctx here??");
    }
    return DUER_OK;
}

static duer_status_t duer_prev_response(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    duer_handler handler = (duer_handler)ctx;
    u8 temp = '0';

    if (handler && msg) {
        int msg_code;
        if (msg->msg_code == DUER_MSG_REQ_GET) {
            DUER_LOGI("next DUER_MSG_REQ_GET");
            msg_code = DUER_MSG_RSP_CONTENT;
            duer_response(msg, msg_code, &temp, 1);
        } else if (msg->msg_code == DUER_MSG_REQ_PUT) {
            DUER_LOGI("next DUER_MSG_REQ_PUT");
            if (msg->payload && msg->payload_len > 0) {
                os_taskq_post("duer_app_task", 1, DUER_PREVIOUS_SONG) ;
                msg_code = DUER_MSG_RSP_CONTENT;
                duer_response(msg, msg_code, &temp, 1);
            } else {
                msg_code = DUER_MSG_RSP_BAD_REQUEST;
                duer_response(msg, msg_code, NULL, 0);
            }
        } else {
            msg_code = DUER_MSG_RSP_METHOD_NOT_ALLOWED;
            duer_response(msg, msg_code, NULL, 0);
        }
    } else {
        DUER_LOGE("TODO what's the purpose of the parameter ctx here??");
    }
    return DUER_OK;
}

static duer_status_t duer_pause_response(duer_context ctx, duer_msg_t *msg, duer_addr_t *addr)
{
    duer_handler handler = (duer_handler)ctx;
    u8 temp = '0';

    if (handler && msg) {
        int msg_code;
        if (msg->msg_code == DUER_MSG_REQ_GET) {
            DUER_LOGI("next DUER_MSG_REQ_GET");
            msg_code = DUER_MSG_RSP_CONTENT;
            duer_response(msg, msg_code, &temp, 1);
        } else if (msg->msg_code == DUER_MSG_REQ_PUT) {
            DUER_LOGI("next DUER_MSG_REQ_PUT");
            if (msg->payload && msg->payload_len > 0) {
                msg_code = DUER_MSG_RSP_CONTENT;
                duer_response(msg, msg_code, &temp, 1);
            } else {
                msg_code = DUER_MSG_RSP_BAD_REQUEST;
                duer_response(msg, msg_code, NULL, 0);
            }
        } else {
            msg_code = DUER_MSG_RSP_METHOD_NOT_ALLOWED;
            duer_response(msg, msg_code, NULL, 0);
        }
    } else {
        DUER_LOGE("TODO what's the purpose of the parameter ctx here??");
    }
    return DUER_OK;
}

void duer_my_dcs_init()
{
    duer_res_t res[] = {
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT | DUER_RES_OP_GET, "volume", .res.f_res = duer_volume_response},
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT | DUER_RES_OP_GET, "next", .res.f_res = duer_next_response},
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT | DUER_RES_OP_GET, "prev", .res.f_res = duer_prev_response},
        {DUER_RES_MODE_DYNAMIC, DUER_RES_OP_PUT | DUER_RES_OP_GET, "pause", .res.f_res = duer_pause_response},
    };

    duer_add_resources(res, sizeof(res) / sizeof(res[0]));
}

