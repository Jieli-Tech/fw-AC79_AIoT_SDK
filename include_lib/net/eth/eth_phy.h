#ifndef  __PHY_STATE_H__
#define  __PHY_STATE_H__

#include "generic/typedef.h"
#include "eth/mdio_bus.h"
enum phy_state {
    PHY_CHECK,
    PHY_RUNNING,
    PHY_UNLINK,
};
typedef enum _LINK_SPEED_MODE {
    PHY_FULLDUPLEX_100M,
    PHY_HALFDUPLEX_100M,
    PHY_FULLDUPLEX_10M,
    PHY_HALFDUPLEX_10M,
    PHY_AUTONEGOTINATION,
} LINK_SPEED_MODE;


/**************************************************/
struct eth_platform_data {
    u8 *name;
    LINK_SPEED_MODE speed;
    u8 mode;
    char irq;
    char mdio_port;
    char mdc_port;
    u32 check_link_time;
    u8 mac_addr[6];
    struct software_rmii rmii_bus;
};






#define NET_PLATFORM_DATA_BEGIN(data) \
	static const struct eth_platform_data data = {


#define NET_PLATFORM_DATA_END() \
};

/**************************************************/
struct eth_phy_device {
    char *name;
    int (*init)(struct eth_platform_data *);
    int (*is_connect)(struct eth_platform_data *);
    void (*get_link_speed)(struct eth_platform_data *);
    void (*set_link_speed)(struct eth_platform_data *);
};


#define REGISTER_NET_PHY_DEVICE(dev) \
	static struct eth_phy_device dev SEC_USED(.eth_phy_device)


extern struct eth_phy_device eth_phy_device_begin[];
extern struct eth_phy_device eth_phy_device_end[];

/**************************************************/
extern const struct device_operations eth_phy_dev_ops;
extern const struct device_operations ipc_dev_ops;

u8 *get_mac_address_for_platform();
void phy_state_machine(void *priv);
int set_phy_stats_cb(u8 id, void (*f)(enum phy_state));//最大只能设置4次，可以通过宏控制 id:0-3
struct eth_platform_data *get_platform_data();
#endif  /*PHY_STATE_H*/
