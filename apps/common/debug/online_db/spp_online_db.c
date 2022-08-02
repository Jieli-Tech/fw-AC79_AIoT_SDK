#include "app_config.h"
#include "spp_user.h"
#include "bt_common.h"
#include "online_db_deal.h"

#if APP_ONLINE_DEBUG

#define LOG_TAG_CONST       ONLINE_DB
#define LOG_TAG             "[ONLINE_DB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static struct db_online_api_t *db_api;
static struct spp_operation_t *spp_api = NULL;
static u8 spp_state;

int online_spp_send_data(u8 *data, u16 len)
{
    if (spp_api) {
        /* log_info("spp_api_tx(%d) \n", len); */
        /* log_info_hexdump(data, len); */
        return spp_api->send_data(NULL, data, len);
    }

    return SPP_USER_ERR_SEND_FAIL;
}

int online_spp_send_data_check(u16 len)
{
    if (spp_api) {
        if (spp_api->busy_state()) {
            return 0;
        }
    }

    return 1;
}

static void online_spp_state_cbk(u8 state)
{
    spp_state = state;

    switch (state) {
    case SPP_USER_ST_CONNECT:
        log_info("SPP_USER_ST_CONNECT ~~~\n");
        db_api->init(DB_COM_TYPE_SPP);
        db_api->register_send_data(online_spp_send_data);
        break;

    case SPP_USER_ST_DISCONN:
        log_info("SPP_USER_ST_DISCONN ~~~\n");
        db_api->exit();
        break;

    default:
        break;
    }
}

static void online_spp_send_wakeup(void)
{
    /* putchar('W'); */
    db_api->send_wake_up();
}

static void online_spp_recieve_cbk(void *priv, u8 *buf, u16 len)
{
    log_debug("spp_api_rx(%d) \n", len);
    /* log_info_hexdump(buf, len); */

    db_api->packet_handle(buf, len);
}

void online_spp_init(void)
{
    spp_state = 0;
    spp_get_operation_table(&spp_api);
    spp_api->regist_recieve_cbk(0, online_spp_recieve_cbk);
    spp_api->regist_state_cbk(0, online_spp_state_cbk);
    spp_api->regist_wakeup_send(NULL, online_spp_send_wakeup);
    db_api = app_online_get_api_table();
}

#endif

