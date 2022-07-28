#ifndef __TVS_IPLIST_H__
#define __TVS_IPLIST_H__

typedef struct {
    int session_id;
} tvs_iplist_exe_cfg;

int tvs_iplist_get_next_ip(int index);

bool tvs_iplist_remove_ip(int ip_addr);

bool tvs_iplist_check_start();

bool tvs_iplist_check_stop();

int tvs_iplist_get_count();

void tvs_iplist_clear();

int tvs_iplist_get_time(bool last_success);

void tvs_iplist_reset_timer(int time, void *func);

int tvs_iplist_init();

bool tvs_iplist_is_timeout();

void tvs_iplist_on_network_changed(bool connected);

int tvs_iplist_start(tvs_http_client_callback_exit_loop should_exit_func,
                     tvs_http_client_callback_should_cancel should_cancel_func,
                     void *exit_param, bool *force_break);

#endif
