#ifndef __TVS_IP_PROVIDER_H__
#define __TVS_IP_PROVIDER_H__

int tvs_ip_provider_on_ip_invalid(int ipaddr);

int tvs_ip_provider_get_valid_ip();

void tvs_ip_provider_set_valid_ip(int ip, bool dnsIp);

void tvs_ip_provider_set_valid_ip_ex(const char *ip_str, bool dnsIp);

int tvs_ip_provider_init();

void tvs_ip_provider_on_network_changed(bool connected);

int tvs_ip_provider_convert_ip_str(const char *ip_str);

void tvs_ip_provider_get_first_ip();

#endif