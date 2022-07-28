/* Includes ------------------------------------------------------------------*/
#include "typedef.h"
#include "lwip/err.h"
#include "etharp.h"
#include "ethip6.h"
#include "lwip/dhcp6.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip.h"
#include "lwip/sys.h"
/* Define those to better describe your network interface. */
#define IFNAME0 'B'
#define IFNAME1 'T'

static struct netif *record_netif = NULL;

void bt_ethernetif_input(void *data, int len);

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
#if 0
struct ethernetif {
    struct eth_addr *ethaddr;
    /* Add whatever per-interface state that is needed here. */
};
#endif

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void low_level_init(struct netif *netif)
{
    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    /*netif->state = netdev_open();*/
    /* netdev_rx_register((void (*)(void *, void *, unsigned int))ethernetif_input, netif); */

    /* pan_rx_register(bt_ethernetif_input,netif); */
    record_netif = netif ;


    /* netdev_get_mac_addr((char *)(netif->hwaddr)); */

    /*set virtual address ,to test*/

    netif->hwaddr[0] = 0x33;
    netif->hwaddr[1] = 0x33;
    netif->hwaddr[2] = 0x0;
    netif->hwaddr[3] = 0x0;
    netif->hwaddr[4] = 0x0;
    netif->hwaddr[5] = 0x16;



    /* maximum transfer unit */
    netif->mtu = NETIF_MTU;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP;
#if LWIP_IPV6
    static  struct dhcp6 dhcp6;
    dhcp6_set_struct(netif, &dhcp6);
    dhcp6_enable_stateless(netif);
    netif->output_ip6 = ethip6_output;
    netif->ip6_autoconfig_enabled = 1;
    netif_create_ip6_linklocal_address(netif, 1);
    netif->flags |= NETIF_FLAG_MLD6;
#endif

    /* Do whatever else is needed to initialize interface. */

    /* Initialise the EMAC.  This routine contains code that polls status bits.
       If the Ethernet cable is not plugged in then this can take a considerable
       time.  To prevent this starving lower priority tasks of processing time we
       lower our priority prior to the call, then raise it back again once the
       initialisation is complete. */
}

/***********************************************************************************************
 ****函数名:lbs_sendpbuf
 ****描述:实现lwip与驱动层的接口
 ****参数:priv:全局驱动变量
p:lwip数据报缓冲区pbuf
 ****返回:0:正常 其他:错误

 ***********************************************************************************************/
static int send_packet(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    char *buffer, *pos;
    int ret = 0;
    int i = 0;
    buffer = malloc(p->tot_len);
    if (!buffer) {
        return -1;
    }
    for (q = p; q != NULL; q = q->next) {
        memcpy(buffer + i, q->payload, q->len);
        i += (int)q->len;
    }
    /* printf("low_level_output len = %d \r\n",p->tot_len); */
    /* put_buf(buffer,p->tot_len); */
#ifdef LIWP_USE_BT
    extern int user_pan_send_cmd(u8 * addr, u32 cmd, u32 value, u8 * data);
    ret = user_pan_send_cmd(NULL, 0xff, p->tot_len, buffer);
#endif
    free(buffer);

    return ret;


    /* ret = netdev_send_data(netif->state, buffer); */

//printf("%s ::: %d \n",__func__,__LINE__);
    /* return ret; */
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become availale since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    lwip_netflow(1, 0);
#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    if (send_packet(netif, p)) {
        return ERR_IF;
    }

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.xmit);

    return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *low_level_input(void *rx_pkt, int len)
{
    struct pbuf *q, *p;
    u16 i;

    /* Obtain the size of the packet and put it into the "len" variable. */

    if (len == 0) {
        return 0;
    }

    /* printf("low_level_input len = %d",len); */

    /* put_buf(rx_pkt,len); */

#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

    if (p != NULL) {
#if ETH_PAD_SIZE
        pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

        /* We iterate over the pbuf chain until we have read the entire packet into the pbuf. */
        for (i = 0, q = p; q != NULL; q = q->next) {
            /* Read enough bytes to fill this pbuf in the chain. The
             * available data in the pbuf is given by the q->len
             * variable.
             * This does not necessarily have to be a memcpy, you can also preallocate
             * pbufs for a DMA-enabled MAC and after receiving truncate it to the
             * actually received size. In this case, ensure the tot_len member of the
             * pbuf is the sum of the chained pbuf len members.
             */
            memcpy((u8_t *)q->payload, (u8_t *)rx_pkt + i, q->len);
            i += q->len;
        }

#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

        LINK_STATS_INC(link.recv);

    } else {
        /* puts("pbuf_alloc fail!\n"); */

        LINK_STATS_INC(link.memerr);
        LINK_STATS_INC(link.drop);
    }

    return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
void bt_ethernetif_input(void *data, int len)
{
    struct pbuf *p;
    struct netif *netif = (struct netif *)record_netif;

    if (!netif) {
        return;
    }


    /* move received packet into a new pbuf */
    p = low_level_input(data, len);
    /* no packet could be read, silently ignore this */
    if (p == NULL) {
        return;
    }

//	puts("ether-input\n");
    if (netif->input(p, netif) != ERR_OK) {
        pbuf_free(p);
    }
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t bt_ethernetif_init(struct netif *netif)
{
    /*struct ethernetif *ethernetif;


    ethernetif = mem_malloc(sizeof(struct ethernetif));
    if (ethernetif == NULL) {
    	return ERR_MEM;
    }*/

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip-bt";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
#define LINK_SPEED_OF_YOUR_NETIF_IN_BPS (100*1000000)
    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    //netif->state = ethernetif;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    //ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

    /* initialize the hardware */
    low_level_init(netif);

    return ERR_OK;
}
