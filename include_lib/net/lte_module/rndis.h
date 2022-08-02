#ifndef  __RNDIS_H__
#define  __RNDIS_H__


typedef struct {
    s32(*init)(struct lte_module_data *);
    void (*dev_in)(void);
    void (*dev_out)(void);
    void (*get_mac)(u8 *mac);
    void *(*get_txaddr)(void);
    void (*tx_packet)(u16 length);
    s32(*get_rxpkt_addr_len)(struct lte_module_pkg *lte_module_pkg);
    void (*release_current_rxpkt)(void);
} rndis_interface_type;


void get_rndis_ops(rndis_interface_type **ops);

#endif  /*PHY_STATE_H*/


