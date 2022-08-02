#ifndef  __LTE_MODULE_H__
#define  __LTE_MODULE_H__

#include "generic/typedef.h"
#include "device/device.h"


enum {
    LTE_NETWORK_START,
    LTE_NETWORK_STOP,
    LTE_DEV_SET_CB,
    LTE_GET_MAC_ADDR,
};


struct lte_module_data {
    u8 *name;
    u8 mac_addr[6];
};

struct lte_module_pkg {
    u8 *data;
    u32   data_len;
};


#define LTE_MODULE_DATA_BEGIN(data) \
	static struct lte_module_data data = {


#define LTE_MODULE_DATA_END() \
};


u8 *lte_module_get_mac_addr(void);
void *lte_module_get_txaddr(void);
void lte_module_tx_packet(u16 length);
s32 lte_module_get_rxpkt_addr_len(struct lte_module_pkg *lte_module_pkg);
void lte_module_release_current_rxpkt(void);
u32 lte_get_upload_rate(void);
u32 lte_get_download_rate(void);

#endif  /*PHY_STATE_H*/
