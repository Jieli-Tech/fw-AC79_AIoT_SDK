#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "tvs_core.h"
#include "tvs_api/tvs_api.h"
#include "tvs_log.h"

extern int tvs_core_authorize_set_callback(tvs_authorize_callback auth_callback);

extern int tvs_authorizer_load_auth_info(const char *authorize_info, int len);

extern char *tvs_authorizer_generate_client_id(const char *tvs_product_id, const char *tvs_dsn);

extern int tvs_authorizer_set_manuf_current_client_id(const char *client_id, const char *auth_resp_info);

extern int tvs_authorize_build_auth_req(char *authReqInfo, int len);

int tvs_authorize_manager_initalize(const char *product_id, const char *dsn, const char *authorize_info, int authorize_info_size, tvs_authorize_callback auth_callback)
{
    return tvs_core_authorize_initalize(product_id, dsn, authorize_info, authorize_info_size, auth_callback);
}

int tvs_authorize_manager_guest_login()
{
    return tvs_core_authorize_guest_login();
}

int tvs_authorize_manager_set_client_id(const char *client_id)
{
    return tvs_core_authorize_set_client_id(client_id);
}

int tvs_authorize_manager_build_auth_req(char *authReqInfo, int len)
{
    return tvs_authorize_build_auth_req(authReqInfo, len);
}

int tvs_authorize_manager_set_manuf_client_id(const char *client_id, const char *auth_resp_info)
{
    return tvs_authorizer_set_manuf_current_client_id(client_id, auth_resp_info);
}


int tvs_authorize_manager_login()
{
    return tvs_core_authorize_login();
}

int tvs_authorize_manager_logout()
{
    return tvs_core_authorize_logout();
}

int tvs_authorize_init(tvs_authorize_callback callback)
{
    tvs_core_authorize_set_callback(callback);
    return 0;
}

int tvs_authorize_set_auth_info(char *auth_info, int auth_info_len)
{
    return tvs_authorizer_load_auth_info(auth_info, auth_info_len);
}

int tvs_authorize_set_client_id(const char *client_id)
{
    return tvs_core_authorize_set_client_id(client_id);
}

int tvs_authorize_clear_auth_info()
{
    return tvs_core_authorize_logout();
}

char *tvs_authorize_generate_client_id(const char *product_id, const char *dsn)
{
    return tvs_authorizer_generate_client_id(product_id, dsn);
}

int tvs_authorize_start()
{
    return tvs_core_authorize_login();
}

int tvs_authorize_restart(const char *client_id)
{
    TVS_LOG_PRINTF("call tvs_authorize_restart\n");

    tvs_authorize_clear_auth_info();

    int ret = tvs_authorize_set_client_id(client_id);

    if (ret == 0) {
        tvs_authorize_start();
    }

    return ret;
}


