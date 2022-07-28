#ifndef __PLATFORMCOMMON_H
#define __PLATFORMCOMMON_H

extern void ReadDDNSSettingsFromConfFile(mDNS *const m, const char *const filename, domainname *const hostname, domainname *const domain, mDNSBool *DomainDiscoveryDisabled);

#endif
