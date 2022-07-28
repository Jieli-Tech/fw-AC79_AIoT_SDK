#ifndef __TVS_PRESET_IP_LIST_H__
#define __TVS_PRESET_IP_LIST_H__

int tvs_iplist_preset_get_next_ip(int index);

bool tvs_iplist_preset_remove_ip(int ip_addr);

bool tvs_iplist_preset_check_start();

int tvs_iplist_preset_get_count();

int tvs_iplist_preset_init();

void tvs_iplist_preset_on_network_changed(bool connected);

#endif
