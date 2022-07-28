#ifndef _ETHMAC_H_
#define _ETHMAC_H_
#include "generic/typedef.h"
#include "eth/eth_phy.h"
//#include "usart.h"
/*
CHIP_PIN    3SL150	3SL340	    MII_MODE		 RMII_MODE

PE2---------P82---------PE2-------------MII_RX2----------NC
PE3---------P83---------PE3-------------MII_RX3----------NC
PH0---------PE0---------PM0-------------NC/MII_TXERR-----NC/RMII_TXERR
PH1---------PE1---------PM1-------------MII_RXCLK--------NC
PH2---------PE2---------PM2-------------NC/MII_COL-------NC/RMII_COL
PH3---------PE3---------PM3-------------MII_RXDV---------NC/RMII_RXDV
PH4---------PE4---------PM4-------------MII_TX2----------NC
PH5---------PE5---------PM5-------------MII_TX3----------NC
PH6---------PE6---------PM6-------------MDC--------------MDC
PH7---------PE7---------PM7-------------MDIO-------------MDIO
PH8---------PF0---------PN0-------------MII_TX0----------RMII_TX0
PH9---------PF1---------PN1-------------MII_TX1----------RMII_TX1
PH10--------PF2---------PN2-------------MII_TXEN---------RMII_TXEN
PH11--------PF3---------PN3-------------NC/MII_RXERR-----NC/RMII_RXER
PH12--------PF4---------PN4-------------MII_TXCLK--------RMII_REFCLK
PH13--------PF5---------PN5-------------MII_CRS----------RMII_CRSDV
PH14--------PF6---------PN6-------------MII_RX0----------RMII_RX0
PH15--------PF7---------PN7-------------MII_RX1----------RMII_RX1
*/

/* Ethernet configuration registers */
typedef struct _oeth_regs {
    volatile u32    moder;          /* Mode Register */
    volatile   u32    int_src;        /* Interrupt Source Register */
    volatile   u32    int_mask;       /* Interrupt Mask Register */
    volatile u32    ipgt;           /* Back to Bak Inter Packet Gap Register */
    volatile  u32   ipgr1;          /* Non Back to Back Inter Packet Gap Register 1 */
    volatile  u32    ipgr2;          /* Non Back to Back Inter Packet Gap Register 2 */
    volatile  u32    packet_len;     /* Packet Length Register (min. and max.) */
    volatile u32    collconf;       /* Collision and Retry Configuration Register */
    volatile u32    tx_bd_num;      /* Transmit Buffer Descriptor Number Register */
    volatile u32    ctrlmoder;      /* Control Module Mode Register */
    volatile u32    miimoder;       /* MII Mode Register */
    volatile    u32    miicommand;     /* MII Command Register */
    volatile    u32    miiaddress;     /* MII Address Register */
    volatile    u32    miitx_data;     /* MII Transmit Data Register */
    volatile u32    miirx_data;     /* MII Receive Data Register */
    volatile u32    miistatus;      /* MII Status Register */
    volatile u32    mac_addr0;      /* MAC Individual Address Register 0 */
    volatile u32    mac_addr1;      /* MAC Individual Address Register 1 */
    volatile u32    hash_addr0;     /* Hash Register 0 */
    volatile u32    hash_addr1;     /* Hash Register 1 */
    volatile u32    txctrl;         /* Transmit control frame Register */
    volatile u32    rxctrl;         /* Rx control frame Register */
    volatile u32    wbdbg;          /* Wishbone state machine debug information */

} oeth_regs;

/* Ethernet buffer descriptor */
typedef struct _oeth_bd {
    volatile     u32    len_status;
    volatile    u32    addr;           /* Buffer address */
} oeth_bd;

struct oeth_data {
    u8 *data;
    u32   data_len;
};




/* Tx BD */
#define OETH_TX_BD_READY        0x8000 /* Tx BD Ready */
#define OETH_TX_BD_IRQ          0x4000 /* Tx BD IRQ Enable */
#define OETH_TX_BD_WRAP         0x2000 /* Tx BD Wrap (last BD) */
#define OETH_TX_BD_PAD          0x1000 /* Tx BD Pad Enable */
#define OETH_TX_BD_CRC          0x0800 /* Tx BD CRC Enable */

#define OETH_TX_BD_UNDERRUN     0x0100 /* Tx BD Underrun Status */
#define OETH_TX_BD_RETRY        0x00F0 /* Tx BD Retry Status */
#define OETH_TX_BD_RETLIM       0x0008 /* Tx BD Retransmission Limit Status */
#define OETH_TX_BD_LATECOL      0x0004 /* Tx BD Late Collision Status */
#define OETH_TX_BD_DEFER        0x0002 /* Tx BD Defer Status */
#define OETH_TX_BD_CARRIER      0x0001 /* Tx BD Carrier Sense Lost Status */
#define OETH_TX_BD_STATS        (OETH_TX_BD_UNDERRUN            | \
                                OETH_TX_BD_RETRY                | \
                                OETH_TX_BD_RETLIM               | \
                                OETH_TX_BD_LATECOL              | \
                                OETH_TX_BD_DEFER                | \
                                OETH_TX_BD_CARRIER)

#define OETH_TX_BD_ERR_STATS   (OETH_TX_BD_UNDERRUN             | \
                                OETH_TX_BD_RETRY                | \
                                OETH_TX_BD_RETLIM               | \
                                OETH_TX_BD_LATECOL              | \
                                OETH_TX_BD_DEFER                | \
                                OETH_TX_BD_CARRIER)

/* Rx BD */
#define OETH_RX_BD_EMPTY        0x8000 /* Rx BD Empty */
#define OETH_RX_BD_IRQ          0x4000 /* Rx BD IRQ Enable */
#define OETH_RX_BD_WRAP         0x2000 /* Rx BD Wrap (last BD) */

#define OETH_RX_BD_CF           0x0100 /* Rx BD Control Frame */
#define OETH_RX_BD_MISS         0x0080 /* Rx BD Miss Status */
#define OETH_RX_BD_OVERRUN      0x0040 /* Rx BD Overrun Status */
#define OETH_RX_BD_INVSIMB      0x0020 /* Rx BD Invalid Symbol Status */
#define OETH_RX_BD_DRIBBLE      0x0010 /* Rx BD Dribble Nibble Status */
#define OETH_RX_BD_TOOLONG      0x0008 /* Rx BD Too Long Status */
#define OETH_RX_BD_SHORT        0x0004 /* Rx BD Too Short Frame Status */
#define OETH_RX_BD_CRCERR       0x0002 /* Rx BD CRC Error Status */
#define OETH_RX_BD_LATECOL      0x0001 /* Rx BD Late Collision Status */
#define OETH_RX_BD_STATS       (OETH_RX_BD_CF                   | \
                                OETH_RX_BD_MISS                 | \
                                OETH_RX_BD_OVERRUN              | \
                                OETH_RX_BD_INVSIMB              | \
                                OETH_RX_BD_DRIBBLE              | \
                                OETH_RX_BD_TOOLONG              | \
                                OETH_RX_BD_SHORT                | \
                                OETH_RX_BD_CRCERR               | \
                                OETH_RX_BD_LATECOL)

#define OETH_RX_BD_ERR_STATS   (OETH_RX_BD_OVERRUN              | \
                                OETH_RX_BD_INVSIMB              | \
                                OETH_RX_BD_DRIBBLE              | \
                                OETH_RX_BD_TOOLONG              | \
                                OETH_RX_BD_SHORT                | \
                                OETH_RX_BD_CRCERR               | \
                                OETH_RX_BD_LATECOL)

/* MODER Register */
#define OETH_MODER_RXEN         0x00000001 /* Receive Enable  */
#define OETH_MODER_TXEN         0x00000002 /* Transmit Enable */
#define OETH_MODER_NOPRE        0x00000004 /* No Preamble  */
#define OETH_MODER_BRO          0x00000008 /* Reject Broadcast */
#define OETH_MODER_IAM          0x00000010 /* Use Individual Hash */
#define OETH_MODER_PRO          0x00000020 /* Promiscuous (receive all) */
#define OETH_MODER_IFG          0x00000040 /* Min. IFG not required */
#define OETH_MODER_LOOPBCK      0x00000080 /* Loop Back */
#define OETH_MODER_NOBCKOF      0x00000100 /* No Backoff */
#define OETH_MODER_EXDFREN      0x00000200 /* Excess Defer */
#define OETH_MODER_FULLD        0x00000400 /* Full Duplex */
//#define OETH_MODER_RST          0x00000800 /* Reset MAC */
#define OETH_MODER_MODE         0x00000800 /* MII/RMII mode */
#define OETH_MODER_DLYCRCEN     0x00001000 /* Delayed CRC Enable */
#define OETH_MODER_CRCEN        0x00002000 /* CRC Enable */
#define OETH_MODER_HUGEN        0x00004000 /* Huge Enable */
#define OETH_MODER_PAD          0x00008000 /* Pad Enable */
#define OETH_MODER_RECSMALL     0x00010000 /* Receive Small */

/* Interrupt Source Register */
#define OETH_INT_TXB            0x00000001 /* Transmit Buffer IRQ */
#define OETH_INT_TXE            0x00000002 /* Transmit Error IRQ */
#define OETH_INT_RXB            0x00000004 /* Receive Frame IRQ */
#define OETH_INT_RXE            0x00000008 /* Receive Error IRQ */
#define OETH_INT_BUSY           0x00000010 /* Busy IRQ */
#define OETH_INT_TXC            0x00000020 /* Transmit Control Frame IRQ */
#define OETH_INT_RXC            0x00000040 /* Received Control Frame IRQ */

/* Interrupt Mask Register */
#define OETH_INT_MASK_TXB       0x00000001 /* Transmit Buffer IRQ Mask */
#define OETH_INT_MASK_TXE       0x00000002 /* Transmit Error IRQ Mask */
#define OETH_INT_MASK_RXF       0x00000004 /* Receive Frame IRQ Mask */
#define OETH_INT_MASK_RXE       0x00000008 /* Receive Error IRQ Mask */
#define OETH_INT_MASK_BUSY      0x00000010 /* Busy IRQ Mask */
#define OETH_INT_MASK_TXC       0x00000020 /* Transmit Control Frame IRQ Mask */
#define OETH_INT_MASK_RXC       0x00000040 /* Received Control Frame IRQ Mask */

/* Control Module Mode Register */
#define OETH_CTRLMODER_PASSALL  0x00000001 /* Pass Control Frames */
#define OETH_CTRLMODER_RXFLOW   0x00000002 /* Receive Control Flow Enable */
#define OETH_CTRLMODER_TXFLOW   0x00000004 /* Transmit Control Flow Enable */

/* MII Mode Register */
#define OETH_MIIMODER_CLKDIV    0x000000FF /* Clock Divider */
#define OETH_MIIMODER_NOPRE     0x00000100 /* No Preamble */
#define OETH_MIIMODER_RST       0x00000200 /* MII Reset */

/* MII Command Register */
#define OETH_MIICOMMAND_SCANSTAT  0x00000001 /* Scan Status */
#define OETH_MIICOMMAND_RSTAT     0x00000002 /* Read Status */
#define OETH_MIICOMMAND_WCTRLDATA 0x00000004 /* Write Control Data */

/* MII Address Register */
#define OETH_MIIADDRESS_FIAD    0x0000001F /* PHY Address */
#define OETH_MIIADDRESS_RGAD    0x00001F00 /* RGAD Address */

/* MII Status Register */
#define OETH_MIISTATUS_LINKFAIL 0x00000001 /* Link Fail */
#define OETH_MIISTATUS_BUSY     0x00000002 /* MII Busy */
#define OETH_MIISTATUS_NVALID   0x00000004 /* Data in MII Status Register is invalid */

#define RMII_MODE   1
#define MII_MODE    0

#define oeth_puts     puts
#define oeth_printf   printf


void oeth_clean_current_bd(struct eth_platform_data *__data);

int ethmac_setup(struct eth_platform_data *__data);
void oeth_enable_oeth_txrx(void);
void oeth_disable_oeth_txrx(void);
/*针对lwip底层收发包机制定制的函数,加强耦合使得效率更高*/
u8 *oeth_get_txaddr(void);//发送一个数据包,最大buflen不能超过1514Byte,要求数据指针地址4字节对齐
void oeth_tx_packet(u16 length);
void oeth_tx_pkt_addr_word_align(void *pkt, u16 length);//发送一个数据包,最大buflen不能超过1514Byte,要求数据指针地址4字节对齐
//通用收发包函数
u8 oeth_tx_pkt(void *buf, u16 buf_len);
int oeth_rx_pkt(void *arg, u8 *buf, u16 buf_len);
void oeth_rx_pkt_test(void *arg);
u16 ethernet_checksum(void *dataptr, u32 len);
int oeth_get_rxpkt_addr_len(struct eth_platform_data *__data, struct oeth_data *_oeth_data);
u32 check_all_rx_bd();
void clear_all_rx_bd();
u32  check_all_tx_bd();
void tx_speed();
void rx_speed();
void check_oeth_stats();
void reset_mac(void);
#endif

