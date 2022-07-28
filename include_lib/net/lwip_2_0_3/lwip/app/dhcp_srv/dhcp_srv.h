#ifndef __DHCP_SRV_H__
#define __DHCP_SRV_H__

extern void dhcps_init(void);
extern void dhcps_release_ipaddr(void *hwaddr);

extern void dhcps_uninit(void);
#endif  //__DHCP_SRV_H__
