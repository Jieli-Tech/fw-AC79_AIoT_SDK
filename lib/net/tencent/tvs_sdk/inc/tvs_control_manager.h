#ifndef __TVS_CONTROL_MANAGER_H__
#define __TVS_CONTROL_MANAGER_H__


int tvs_control_manager_start(int control_type, const char *payload, int session_id,
                              tvs_http_client_callback_exit_loop should_exit_func,
                              tvs_http_client_callback_should_cancel should_cancel,
                              void *exit_param,
                              bool *expect_speech,
                              bool *has_media,
                              bool *connected);

char *tvs_control_manager_create_payload(int cmd, int iparam, void *pparam, void *other_param);


#endif
