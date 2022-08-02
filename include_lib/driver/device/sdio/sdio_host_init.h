#ifndef _SDIO_HOST_INIT_H_
#define _SDIO_HOST_INIT_H_

/*
						    CMD 		CLK 		D0 				D1 			D2 			D3
SDIO_GRP_0|SDIO_PORT_0: IO_PORTA_01, IO_PORTA_02, IO_PORTA_04, IO_PORTH_13, IO_PORTA_03, IO_PORTA_05
SDIO_GRP_0|SDIO_PORT_1: IO_PORTH_07, IO_PORTH_08, IO_PORTH_10, IO_PORTH_11, IO_PORTH_09, IO_PORTH_06
SDIO_GRP_0|SDIO_PORT_2: IO_PORTG_02, IO_PORTG_03, IO_PORTG_04, IO_PORTG_05, IO_PORTG_00, IO_PORTG_01
SDIO_GRP_0|SDIO_PORT_3: IO_PORTG_10, IO_PORTG_11, IO_PORTG_12, IO_PORTG_13, IO_PORTG_08, IO_PORTG_09
SDIO_GRP_1|SDIO_PORT_0: IO_PORTF_04, IO_PORTF_05, IO_PORTF_06, IO_PORTF_07, IO_PORTF_02, IO_PORTF_03
SDIO_GRP_1|SDIO_PORT_1: IO_PORTG_02, IO_PORTG_03, IO_PORTG_04, IO_PORTG_05, IO_PORTG_00, IO_PORTG_01
SDIO_GRP_1|SDIO_PORT_2: IO_PORTD_08, IO_PORTD_09, IO_PORTD_10, IO_PORTD_11, IO_PORTD_06, IO_PORTD_07
SDIO_GRP_1|SDIO_PORT_3: IO_PORTA_08, IO_PORTA_10, IO_PORTA_11, IO_PORTA_12, IO_PORTA_06, IO_PORTA_07
SDIO_GRP_2|SDIO_PORT_0: IO_PORTF_04, IO_PORTF_05, IO_PORTF_06, IO_PORTF_07, IO_PORTF_02, IO_PORTF_03
SDIO_GRP_2|SDIO_PORT_1: IO_PORTH_07, IO_PORTH_08, IO_PORTH_10, IO_PORTH_11, IO_PORTH_09, IO_PORTH_06

注意：ac521x的SD2-B不能用wekeup中断，只能用轮训
*/

//选择SDIO0? SDIO1? 作为输出,默认为SDIO1
#define SDIO_GRP_0  0
#define SDIO_GRP_1  (1 << 31)
#define SDIO_GRP_2  (1 << 30)
#define SDIO_GRP_MASK  (1 << 31|1 << 30)

//选择SDIOx的哪个出口作为输出,默认为出口0
#define SDIO_PORT_0  0
#define SDIO_PORT_1  (1 << 29)
#define SDIO_PORT_2  (1 << 28)
#define SDIO_PORT_3  (1 << 29|1 << 28)
#define SDIO_PORT_MASK  (1 << 29|1 << 28)

//是否使用四线模式, 默认为单线模式
#define SDIO_1_BIT_DATA  0
#define SDIO_4_BIT_DATA  (1 << 27)
#define SDIO_4_BIT_DATA_MASK  (1 << 27)

//是否使用硬件中断检测外设事件,例如接收到数据. , 默认使用轮询方式查询事件, //对接收速度不敏感使用SDIO_POLLING即可 ,使用SDIO_DATA1_IRQ如果接收数据量过大会导致CPU不足的问题.
#define SDIO_DATA1_IRQ  0
#define SDIO_POLLING  (1 << 26)
#define SDIO_DATA1_IRQ_MASK  (1 << 26)

//配置SDIO时钟(HZ), 默认为40MHZ
#define SDIO_MAX_CLK_MASK ((1 << 26)-1)

#define SDIO_CLOCK_80M  (0) //80M 写0
#define SDIO_CLOCK_40M  (40 * 1000000)
#define SDIO_CLOCK_26M  (26 * 1000000)
#define SDIO_CLOCK_20M  (20 * 1000000)
#define SDIO_CLOCK_16M  (16 * 1000000)
#define SDIO_CLOCK_8M  (8 * 1000000)
#define SDIO_CLOCK_4M  (4 * 1000000)
#define SDIO_CLOCK_2M (2 * 1000000)

/*
sdio_host_init(0);
sdio_host_init(SDIO_GRP_1|SDIO_4_BIT_DATA);
sdio_host_init(SDIO_GRP_1|SDIO_PORT_1|SDIO_DATA1_IRQ);
sdio_host_init(SDIO_GRP_0|SDIO_PORT_0|SDIO_4_BIT_DATA|SDIO_DATA1_IRQ|(10*1000000));
*/

extern  void sdio_host_init(unsigned int parm);
extern unsigned char *SDIO_GET_CMD_BUF(void);
extern void  SDIO_SET_GRP_PORT(u32 grp, u32 port);
extern void cpu_sdio_host_uninit(void);
extern void sdio_dat1_irq_uninit(void);
extern void sdio_dat1_irq_init(void);
extern void host_set_timing(void *host, unsigned int timing);
extern void mmc_set_bus_width(void *host, unsigned int width);
extern void mmc_set_clock(void *host,  unsigned int hz);
extern void  SDIO_CONTROLLER_RESET(void);
extern void  SDIO_IDLE_CLK_EN(u8 enable) ;
extern void  SDIO_CONTROLLER_START(void);
extern void SDIO_CONTROLLER_SET_IRQ(void);
extern  void  SDIO_SET_4WIRE_MODE(u8 enable);

#endif  //_SDIO_HOST_INIT_H_
