#ifndef __TVS_DNS_H__
#define __TVS_DNS_H__

int tvs_dns_init();

void tvs_dns_fetch_all(tvs_http_client_callback_exit_loop should_exit_func,
                       tvs_http_client_callback_should_cancel should_cancel_func, void *exit_param);

void tvs_dns_retry_dns(tvs_http_client_callback_exit_loop should_exit_func,
                       tvs_http_client_callback_should_cancel should_cancel_func, void *exit_param);

void tvs_dns_on_network_changed(bool connected);

bool tvs_http_dns_check_start();

int tvs_http_dns_get_count();

int tvs_http_dns_get_next_ip(int index);

bool tvs_system_dns_check_start();

int tvs_system_dns_get_count();

int tvs_system_dns_get_next_ip(int index);

bool tvs_system_dns_remove_ip(int ip_addr);

bool tvs_http_dns_remove_ip(int ip_addr);

void tvs_dns_get_first_ip();

#endif