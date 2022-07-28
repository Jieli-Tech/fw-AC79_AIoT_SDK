#ifndef __TVS_AUTHORIZER_H_FAEFAFWEFA__
#define __TVS_AUTHORIZER_H_FAEFAFWEFA__
#include "tvs_http_client.h"

int tvs_authorizer_init();

char *tvs_authorizer_get_authtoken();

int tvs_authorizer_manager_start(const char *client_id, const char *refresh_token,
                                 tvs_http_client_callback_exit_loop should_exit_func,
                                 tvs_http_client_callback_should_cancel should_cancel,
                                 void *exit_param);

char *tvs_authorize_generate_client_id(const char *tvs_product_id, const char *tvs_dsn);

void tvs_authorizer_set_current_client_id(char *client_id);

int tvs_authorizer_set_manuf_current_client_id(const char *client_id, const char *auth_resp_info);

char *tvs_authorizer_dup_current_client_id();

char *tvs_authorizer_dup_refresh_token();

bool tvs_authorizer_is_authorized();

void tvs_authorizer_wait_authoried();

void tvs_authorizer_notify_authoried(bool valid);

bool tvs_authorizer_is_timeout();

int tvs_authorizer_start_inner();

#endif
