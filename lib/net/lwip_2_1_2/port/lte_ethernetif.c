/* Includes ------------------------------------------------------------------*/

#include "lwip/err.h"
#include "etharp.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip.h"
#include "lte_module.h"
#include <string.h>
#include "printf.h"
/* #include "os/os_compat.h" */
#include "os/os_api.h"
/* Define those to better describe your network interface. */
#define IFNAME0 'L'
#define IFNAME1 'T'

extern void lte_ethernetif_input(struct netif *netif);
/* void lte_ethernetif_input(void *param, void *data, int len); */
//6字节的目的MAC＋6字节的源MAC＋2字节的帧类型＋最大数据包(1500)
//unsigned char Rx_Data_Buf[1514];
//unsigned char Tx_Data_Buf[1514];
void *__attribute__((weak)) lte_module_get_txaddr(void)
{
    return NULL;
}


int __attribute__((weak)) lte_module_get_rxpkt_addr_len(struct lte_module_pkg *_oeth_data)
{
    return -1;
}


void __attribute__((weak)) lte_module_tx_packet(u16 length)
{
}


void __attribute__((weak)) lte_module_release_current_rxpkt(void)
{

}


u8 *__attribute__((weak)) lte_module_get_mac_addr(void)
{

}


static void lte_rx_task(void *p_arg)
{
    while (1) {
        /* Read a received packet from the Ethernet buffers and send it to the lwIP for handling */
        lte_ethernetif_input((struct netif *)p_arg);

    }
}

static int create_lte_rx_task(void *p_arg, u8 prio)
{
    return os_task_create(lte_rx_task, (void *)p_arg, prio, 0x2000, 0, "lte_rx_task");
}

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
//struct ethernetif {
//  struct eth_addr *ethaddr;
//  /* Add whatever per-interface state that is needed here. */
//};
/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */

static void
wired_low_level_init(struct netif *netif)
{
    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* set MAC hardware address */
    memcpy(netif->hwaddr, lte_module_get_mac_addr(), 6);
    printf("lte mac addr -> %X:%X:%X:%X:%X:%X\n\n"
           , netif->hwaddr[0]
           , netif->hwaddr[1]
           , netif->hwaddr[2]
           , netif->hwaddr[3]
           , netif->hwaddr[4]
           , netif->hwaddr[5]);

    /* maximum transfer unit */
    netif->mtu = 1500;

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP;

    /* Do whatever else is needed to initialize interface. */

    /* Initialise the EMAC.  This routine contains code that polls status bits.
    If the Ethernet cable is not plugged in then this can take a considerable
    time.  To prevent this starving lower priority tasks of processing time we
    lower our priority prior to the call, then raise it back again once the
    initialisation is complete. */


    create_lte_rx_task(netif, 2);
    /* netdev_rx_register((void (*)(void *, void *, unsigned int))lte_ethernetif_input, netif); */
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
static err_t
wired_low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    unsigned int i;
    u8 *ptxpkt;

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    /* Parameter not used. */

    //获取txbd指向的内存地址
    ptxpkt = lte_module_get_txaddr();

    for (i = 0, q = p; q != NULL; q = q->next) {
        /* dma1_cpy(&ptxpkt[i], (u8_t*)q->payload, q->len); */
        memcpy(&ptxpkt[i], (u8_t *)q->payload, q->len);
        i = i + q->len;
    }
    /* printf("low_level_output len = %d \r\n",i); */
    /* printf("+++++++++++++++++++++send_AAA+++++++++++++++++++\n"); */
    /* printf("%s, len = %d\n", __FUNCTION__, i); */
    /* put_buf(ptxpkt,i); */
    /* printf("+++++++++++++++++++++send_BBB+++++++++++++++++++\n"); */
    lte_module_tx_packet(i);

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
static struct pbuf *
wired_low_level_input(struct netif *netif)
{
    struct pbuf *q, *p = NULL;
    u16 i;
    int ret = 0;
    struct lte_module_pkg lte_pkg = {0};
    /* Obtain the size of the packet and put it into the "len" variable. 并且调整下一个接收oeth_rxbd */
    ret = lte_module_get_rxpkt_addr_len(&lte_pkg);
    /* printf("+++++++++++++++++++++recv_AAA+++++++++++++++++++\n"); */
    /* printf("%s, len = %d\n", __FUNCTION__, lte_pkg.data_len); */
    /* put_buf(lte_pkg.data, lte_pkg.data_len); */
    /* printf("+++++++++++++++++++++recv_BBB+++++++++++++++++++\n"); */

    if (ret < 0) {
        return NULL;
    }
    /* printf("low_level_input len = %d \r\n",len); */
    /* put_buf(prxpkt,len); */
#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

    /* We allocate a pbuf chain of pbufs from the pool. */
    p = pbuf_alloc(PBUF_RAW, lte_pkg.data_len, PBUF_POOL);

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

//             dma0_cpy((void *)q->payload, (void *)&prxpkt[i], q->len);
            memcpy((void *)q->payload, (void *)&lte_pkg.data[i], q->len);
            i += q->len;
        }

        lte_module_release_current_rxpkt();

#if ETH_PAD_SIZE
        pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

        LINK_STATS_INC(link.recv);
    } else {
        puts("pbuf_alloc fail!\n");
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
void lte_ethernetif_input(struct netif *netif)
/* void lte_ethernetif_input(void *param, void *data, int len) */
{
    /* printf("@@@%s\n", __FUNCTION__); */
    int err;
    struct pbuf *p;
    /* struct netif *netif = (struct netif *)param; */
    /* move received packet into a new pbuf */

    p = wired_low_level_input(netif);

    /* no packet could be read, silently ignore this */
    if (p == NULL) {
        return;
    }

    /* full packet send to tcpip_thread to process */
    err = netif->input(p, netif);

    if (err != ERR_OK) {
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
        pbuf_free(p);
        p = NULL;
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
err_t
lte_ethernetif_init(struct netif *netif)
{
//  struct ethernetif *ethernetif;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

//  ethernetif = malloc(sizeof(struct ethernetif));
//  if (ethernetif == NULL) {
//    LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
//    return ERR_MEM;
//  }

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
#define LINK_SPEED_OF_YOUR_NETIF_IN_BPS (100*1000000)
    NETIF_INIT_SNMP(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

//  netif->state = ethernetif;
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;
    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
    netif->output = etharp_output;
    netif->linkoutput = wired_low_level_output;

//  ethernetif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);

    /* initialize the hardware */
    wired_low_level_init(netif);
    return ERR_OK;
}

