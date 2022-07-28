#ifndef _SDIO_HOST_H_
#define _SDIO_HOST_H_

#include "os/os_api.h"
#include "asm/irq.h"
#include "sdio/card.h"

#define SDIO_DBG(fmt,...) do{printf("%s-%d"#fmt"\r\n",__FILE__,__LINE__,##__VA_ARGS__);}while(0)
#define SDIO_WARN(fmt,...) do{printf("%s-%d"#fmt"\r\n",__FILE__,__LINE__,##__VA_ARGS__);}while(0)
#define SDIO_ERR(fmt,...) do{printf("%s-%d"#fmt"\r\n",__FILE__,__LINE__,##__VA_ARGS__);}while(0)

#define SDIO_BYTE_ORDER_BIG_ENDIAN        1234
#define SDIO_BYTE_ORDER_LITTLE_ENDIAN     4321
#define SDIO_BYTE_ORDER 				  SDIO_BYTE_ORDER_LITTLE_ENDIAN

#if SDIO_BYTE_ORDER == SDIO_BYTE_ORDER_LITTLE_ENDIAN
#define le16_to_cpup(x) *(x)
#define cpu_to_le16(x) x
#define le32_to_cpup(x) *(x)
#define cpu_to_le32(x) x
#define be32_to_cpu(x) ((((u32)(x) & (u32)0x000000ffUL) << 24) |		\
                        (((u32)(x) & (u32)0x0000ff00UL) <<  8) |		\
                        (((u32)(x) & (u32)0x00ff0000UL) >>  8) |		\
                        (((u32)(x) & (u32)0xff000000UL) >> 24))
#else
#define le16_to_cpup(x) ((((u16)(*(x)) & (u16)0x00ffU) << 8) |	(((u16)(*(x)) & (u16)0xff00U) >> 8))))
#define cpu_to_le16(x)  ((((u16)(x) & (u16)0x00ffU) << 8) |	(((u16)(x) & (u16)0xff00U) >> 8))))
#define le32_to_cpup(x) ((((u32)(*(x)) & (u32)0x000000ffUL) << 24) |		\
                        (((u32)(*(x))) & (u32)0x0000ff00UL) <<  8) |		\
                        (((u32)(*(x)) & (u32)0x00ff0000UL) >>  8) |		\
                        (((u32)(*(x)) & (u32)0xff000000UL) >> 24)))
#define cpu_to_le32(x)  ((((u32)(x) & (u32)0x000000ffUL) << 24) |		\
                        (((u32)(x) & (u32)0x0000ff00UL) <<  8) |		\
                        (((u32)(x) & (u32)0x00ff0000UL) >>  8) |		\
                        (((u32)(x) & (u32)0xff000000UL) >> 24))

#define be32_to_cpu(x) (u32(x))
#endif


/*Command & Response List*/
#define RSP6BUSY	 0x1
#define RSP17BUSY	 0x2
#define NORSPBUSY	 0x3
#define RSP6NOBUSY   0x5
#define RSP17NOBUSY	 0x6
#define NORSPNOBUSY	 0x7


#define NORSP       NORSPNOBUSY
#define R1          RSP6NOBUSY
#define R1B         RSP6BUSY
#define R2          RSP17NOBUSY
#define R3          RSP6NOBUSY
#define R4          RSP6NOBUSY
#define R5          RSP6NOBUSY
#define R6          RSP6NOBUSY
#define R7          RSP6NOBUSY

#define  CMD_STR_HOST 0x40L

#define  CARD_CMD0   ((0 | CMD_STR_HOST)<<8)
#define  CARD_CMD1   ((1 | CMD_STR_HOST)<<8)
#define  CARD_CMD2   ((2 | CMD_STR_HOST)<<8)
#define  CARD_CMD3   ((3 | CMD_STR_HOST)<<8)

#define  CARD_CMD5   ((5 | CMD_STR_HOST)<<8)

#define  CARD_CMD6   ((6 | CMD_STR_HOST)<<8)
#define  CARD_CMD7   ((7 | CMD_STR_HOST)<<8)
#define  CARD_CMD8   ((8 | CMD_STR_HOST)<<8)
#define  CARD_CMD9   ((9 | CMD_STR_HOST)<<8)
#define  CARD_CMD10  ((10 | CMD_STR_HOST)<<8)
#define  CARD_CMD11  ((11 | CMD_STR_HOST)<<8)
#define  CARD_CMD12	 ((12 | CMD_STR_HOST)<<8)
#define  CARD_CMD13	 ((13 | CMD_STR_HOST)<<8)
#define  CARD_CMD15	 ((15 | CMD_STR_HOST)<<8)
#define  CARD_CMD16  ((16 | CMD_STR_HOST)<<8)
#define  CARD_CMD17  ((17 | CMD_STR_HOST)<<8)
#define  CARD_CMD18  ((18 | CMD_STR_HOST)<<8)
#define  CARD_CMD25  ((25 | CMD_STR_HOST)<<8)
#define  CARD_CMD41  ((41 | CMD_STR_HOST)<<8)
#define  CARD_CMD55  ((55 | CMD_STR_HOST)<<8)
#define  CARD_CMD51  ((51 | CMD_STR_HOST)<<8)
#define  CARD_CMD52  ((52 | CMD_STR_HOST)<<8)
#define  CARD_CMD53  ((53 | CMD_STR_HOST)<<8)
#define  CARD_CMD59  ((59 | CMD_STR_HOST)<<8)

#define  CMD0_NR    (CARD_CMD0 | NORSP)
#define  CMD1_R1    (CARD_CMD1 | R1)
#define  CMD1_R3    (CARD_CMD1 | R3)
#define  CMD1_R1B   (CARD_CMD1 | R1B)
#define	 CMD2_R2    (CARD_CMD2 | R2)
#define	 CMD3_R1	(CARD_CMD3 | R1)
#define	 CMD3_R6	(CARD_CMD3 | R6)
#define  CMD5_R4    (CARD_CMD5 | R4)
#define	 CMD6_R1	(CARD_CMD6 | R1)
#define	 CMD6_R1B	(CARD_CMD6 | R1B)
#define	 CMD7_R1B   (CARD_CMD7 | R1B)
#define	 CMD8_R1    (CARD_CMD8 | R1)
#define	 CMD8_R7    (CARD_CMD8 | R7)
#define  CMD9_R2	(CARD_CMD9 | R2)
#define  CMD10_R2	(CARD_CMD10 | R2)
#define  CMD11_R1	(CARD_CMD11 | R1)
#define  CMD12_R1B  (CARD_CMD12 | R1B)
#define  CMD13_R1   (CARD_CMD13 | R1)
#define  CMD15_NR   (CARD_CMD15 | NORSP)
#define  CMD16_NR   (CARD_CMD16 | R1)
#define	 CMD18_R1	(CARD_CMD18 | R1)
#define	 CMD25_R1	(CARD_CMD25 | R1)
#define	 CMD41_R3	(CARD_CMD41 | R3)
#define  CMD55_R1   (CARD_CMD55 | R1)
#define  CMD51_R1   (CARD_CMD51 | R1)
#define  CMD52_R5   (CARD_CMD52 | R5)
#define  CMD53_R5   (CARD_CMD53 | R5)
#define  CMD59_R1   (CARD_CMD59 | R1)

#define GO_IDLE_ATATE               CMD0_NR
#define MMC_ALL_SEND_CID            CMD2_R2
#define SD_SEND_IF_COND             CMD8_R7
#define SD_SWITCH_VOLTAGE           CMD11_R1
#define SD_SEND_RELATIVE_ADDR       CMD3_R6
#define MMC_SELECT_CARD             CMD7_R1B
#define SD_APP_OP_COND              CMD41_R3
#define MMC_SEND_CSD                CMD9_R2
#define MMC_SPI_CRC_ON_OFF          CMD59_R1
#define SD_APP_SET_BUS_WIDTH        CMD6_R1
#define MMC_APP_CMD                 CMD55_R1
#define MMC_DATA_READ               CMD51_R1
#define SD_IO_SEND_OP_COND          CMD5_R4
#define SD_IO_RW_EXTENDED           CMD53_R5
#define SD_APP_SD_STATUS            CMD13_R1
#define SD_APP_SEND_SCR             CMD51_R1
#define MMC_SEND_CID                CMD10_R2
#define SD_SWITCH                   CMD6_R1
#define SD_IO_RW_DIRECT             CMD52_R5


#define CMD_RESP &sdio_host.cmd_buf[6+1]
#define CMD_RESP_0 (unsigned int)((sdio_host.cmd_buf[6+1] << 24) | (sdio_host.cmd_buf[6+2] << 16) | (sdio_host.cmd_buf[6+3] << 8) | (sdio_host.cmd_buf[6+4]))
#define CMD_RESP_1 (unsigned int)((sdio_host.cmd_buf[6+5] << 24) | (sdio_host.cmd_buf[6+6] << 16) | (sdio_host.cmd_buf[6+7] << 8) | (sdio_host.cmd_buf[6+8]))


#define typecheck(type,x) \
    ({      type __dummy; \
        typeof(x) __dummy2; \
        (void)(&__dummy == &__dummy2); \
        1; \
    })
//#define time_after(a,b)  (typecheck(u32, a) && \
typecheck(u32, b) &&\
((s32)(b) - (s32)(a) < 0))

#define time_before(a,b) time_after(b,a)

/*失败列表*/
typedef enum {
    SDIO_SUCC = 0x0,
    SDIO_ERR_CMD_TIMEOUT,
    SDIO_ERR_DATA_TIMEOUT,
    SDIO_ERR_CRC_COMMAND,
    SDIO_ERR_CRC_READ,
    SDIO_ERR_CRC_WRITE,
} sdio_err;

struct mmc_ios {
//	unsigned int	clock;			/* clock rate */
#define SDIO_HOST_MAX_CLK    (80*1000000)

//	unsigned short	vdd;

    /* vdd stores the bit number of the selected voltage range from below. */

//	unsigned char	bus_mode;		/* command output mode */

#define MMC_BUSMODE_OPENDRAIN	1
#define MMC_BUSMODE_PUSHPULL	2

//	unsigned char	chip_select;		/* SPI chip select */

#define MMC_CS_DONTCARE		0
#define MMC_CS_HIGH		1
#define MMC_CS_LOW		2

//	unsigned char	power_mode;		/* power supply mode */

#define MMC_POWER_OFF		0
#define MMC_POWER_UP		1
#define MMC_POWER_ON		2

//	unsigned char	bus_width;		/* data bus width */

#define MMC_BUS_WIDTH_1		0
#define MMC_BUS_WIDTH_4		2
#define MMC_BUS_WIDTH_8		3

    unsigned char	timing;			/* timing specification used */

#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2
#define MMC_TIMING_UHS_SDR12	3
#define MMC_TIMING_UHS_SDR25	4
#define MMC_TIMING_UHS_SDR50	5
#define MMC_TIMING_UHS_SDR104	6
#define MMC_TIMING_UHS_DDR50	7
#define MMC_TIMING_MMC_DDR52	8
#define MMC_TIMING_MMC_HS200	9
#define MMC_TIMING_MMC_HS400	10

//	unsigned char	signal_voltage;		/* signalling voltage (1.8V or 3.3V) */

#define MMC_SIGNAL_VOLTAGE_330	0
#define MMC_SIGNAL_VOLTAGE_180	1
#define MMC_SIGNAL_VOLTAGE_120	2

//	unsigned char	drv_type;		/* driver type (A, B, C, D) */

#define MMC_SET_DRIVER_TYPE_B	0
#define MMC_SET_DRIVER_TYPE_A	1
#define MMC_SET_DRIVER_TYPE_C	2
#define MMC_SET_DRIVER_TYPE_D	3
};

struct mmc_host {
    u8 cmd_buf[6 + 18 + 8];         //<SD 命令 & 反馈缓冲区,+8 for align 32
    char *name;
    u32			caps;		/* Host capabilities */
#define MMC_CAP_4_BIT_DATA	(1 << 0)	/* Can the host do 4 bit transfers */
#define MMC_CAP_MMC_HIGHSPEED	(1 << 1)	/* Can do MMC high-speed timing */
#define MMC_CAP_SD_HIGHSPEED	(1 << 2)	/* Can do SD high-speed timing */
#define MMC_CAP_SDIO_IRQ	(1 << 3)	/* Can signal pending SDIO IRQs */
#define MMC_CAP_SPI		(1 << 4)	/* Talks only SPI protocols */
#define MMC_CAP_NEEDS_POLL	(1 << 5)	/* Needs polling for card-detection */
#define MMC_CAP_8_BIT_DATA	(1 << 6)	/* Can the host do 8 bit transfers */
#define MMC_CAP_AGGRESSIVE_PM	(1 << 7)	/* Suspend (e)MMC/SD at idle  */
#define MMC_CAP_NONREMOVABLE	(1 << 8)	/* Nonremovable e.g. eMMC */
#define MMC_CAP_WAIT_WHILE_BUSY	(1 << 9)	/* Waits while card is busy */
#define MMC_CAP_ERASE		(1 << 10)	/* Allow erase/trim commands */
#define MMC_CAP_1_8V_DDR	(1 << 11)	/* can support */
    /* DDR mode at 1.8V */
#define MMC_CAP_1_2V_DDR	(1 << 12)	/* can support */
    /* DDR mode at 1.2V */
#define MMC_CAP_POWER_OFF_CARD	(1 << 13)	/* Can power off after boot */
#define MMC_CAP_BUS_WIDTH_TEST	(1 << 14)	/* CMD14/CMD19 bus width ok */
#define MMC_CAP_UHS_SDR12	(1 << 15)	/* Host supports UHS SDR12 mode */
#define MMC_CAP_UHS_SDR25	(1 << 16)	/* Host supports UHS SDR25 mode */
#define MMC_CAP_UHS_SDR50	(1 << 17)	/* Host supports UHS SDR50 mode */
#define MMC_CAP_UHS_SDR104	(1 << 18)	/* Host supports UHS SDR104 mode */
#define MMC_CAP_UHS_DDR50	(1 << 19)	/* Host supports UHS DDR50 mode */
#define MMC_CAP_RUNTIME_RESUME	(1 << 20)	/* Resume at runtime_resume. */
#define MMC_CAP_DRIVER_TYPE_A	(1 << 23)	/* Host supports Driver Type A */
#define MMC_CAP_DRIVER_TYPE_C	(1 << 24)	/* Host supports Driver Type C */
#define MMC_CAP_DRIVER_TYPE_D	(1 << 25)	/* Host supports Driver Type D */
#define MMC_CAP_CMD23		(1 << 30)	/* CMD23 supported. */
#define MMC_CAP_HW_RESET	(1 << 31)	/* Hardware reset */
#define mmc_host_is_spi(host)	((host)->caps & MMC_CAP_SPI)
#define USE_SPI_CRC                 1
    u32		ocr_avail;/*sdio主机电压支持范围*/
#define MMC_VDD_165_195		0x00000080	/* VDD voltage 1.65 - 1.95 */
#define MMC_VDD_20_21		0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22		0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23		0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24		0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25		0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26		0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27		0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28		0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29		0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30		0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31		0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32		0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33		0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34		0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35		0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36		0x00800000	/* VDD voltage 3.5 ~ 3.6 */

    struct mmc_ios		ios;		/* current io bus settings */

    struct mmc_card		*card;		/* device attached to this host */

    /* host specific block data */
    u32		max_blk_size;	/*一个mmc请求块最大尺寸*/
    u32		max_blk_count;	/*最大的请求块数目*/

    OS_SEM sem;
    OS_MUTEX mutex;
    u32 timeout;
    void *port_wakeup_hdl;

    u32 max_clock;
    u32 grp_sel;
    u32 port_sel;
    u8 crc_err_flag;

} __attribute__((aligned(32)));
extern struct mmc_host sdio_host;


static inline int mmc_card_hs(struct mmc_card *card)
{
    return card->host->ios.timing == MMC_TIMING_SD_HS ||
           card->host->ios.timing == MMC_TIMING_MMC_HS;
}

static inline int mmc_card_uhs(struct mmc_card *card)
{
    return card->host->ios.timing >= MMC_TIMING_UHS_SDR12 &&
           card->host->ios.timing <= MMC_TIMING_UHS_DDR50;
}

static inline bool mmc_card_hs200(struct mmc_card *card)
{
    return card->host->ios.timing == MMC_TIMING_MMC_HS200;
}

static inline bool mmc_card_ddr52(struct mmc_card *card)
{
    return card->host->ios.timing == MMC_TIMING_MMC_DDR52;
}

static inline bool mmc_card_hs400(struct mmc_card *card)
{
    return card->host->ios.timing == MMC_TIMING_MMC_HS400;
}

static inline int mmc_host_uhs(struct mmc_host *host)
{
    return host->caps &
           (MMC_CAP_UHS_SDR12 | MMC_CAP_UHS_SDR25 |
            MMC_CAP_UHS_SDR50 | MMC_CAP_UHS_SDR104 |
            MMC_CAP_UHS_DDR50);
}

void sdio_host_init(u32 parm);
void sdio_host_uninit(void);
sdio_err sdio_host_send_command(u32 arg, u16 cmd);
sdio_err  sdio_host_req(u8 write, u32 cmd_arg, u16 cmd_opcode, u8 *buf, u16 blocks, u16 blksz);
char *mmc_hostname(struct mmc_host *host);
void mmc_delay(u32 cnt);
void mmc_set_clock(void *host, unsigned int hz);
void mmc_set_bus_width(void *host, unsigned int width);
void mmc_set_timing(struct mmc_host *host, unsigned int timing);
int sdio_read_common_cis(struct mmc_card *card);
int sdio_read_func_cis(struct sdio_func *func);
void sdio_remove_func(struct sdio_func *func);
int sdio_add_func(struct sdio_func *func);
u32 sd_get_host_max_current(struct mmc_host *host);
int sdio_get_ro(struct mmc_host *host);
u32 sdio_host_get_jiffies(void);
int host_get_cd(struct mmc_host *host);
int host_set_signal_voltage_switch(struct mmc_host *host, int signal_voltage);
    struct sdio_func *sdio_get_func(unsigned int fn);
void sdio_dat1_irq_uninit(void);
void sdio_dat1_irq_init(void);

#endif
