#ifndef __TVS_EVENT_MANAGER_H__
#define __TVS_EVENT_MANAGER_H__
#include "tvs_http_client.h"

char *tvs_event_manager_create_payload(int cmd, int iparam, void *pparam, void *pparam2);

int tvs_event_manager_start(const char *payload,
                            tvs_http_client_callback_exit_loop should_exit_func,
                            tvs_http_client_callback_should_cancel should_cancel,
                            void *exit_param, bool *is_force_break);
#endif