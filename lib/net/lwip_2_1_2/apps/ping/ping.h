#ifndef __PING_H__
#define __PING_H__

/**
 * PING_USE_SOCKETS: Set to 1 to use sockets, otherwise the raw api is used
 */
#ifndef PING_USE_SOCKETS
int ping_init(const char *ip_addr_str, u32 delayms, u32 ping_total_cnt, void (*cb)(void *, u32), void *priv);
#else
int ping_init(const char *ip_addr_str);
#endif


#endif /* __PING_H__ */

