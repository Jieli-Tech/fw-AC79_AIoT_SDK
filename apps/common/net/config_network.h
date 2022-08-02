#ifndef __CONFIG_NETWORK_H__
#define __CONFIG_NETWORK_H__

extern void config_network_start(void);
extern void config_network_stop(void);
extern void wifi_smp_set_ssid_pwd(void);
extern void config_network_connect(void);
extern void config_network_broadcast(void);
extern u8 is_in_config_network_state(void);

#endif // __CONFIG_NETWORK_H__
