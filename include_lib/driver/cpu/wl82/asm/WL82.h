
#ifndef __WL82__
#define __WL82__

#include "csfr.h"

//===============================================================================//
//
//      sfr define
//
//===============================================================================//
#define psfr_base          0x50000                // port
#define hsfr_base          0x40000                // hsb
#define bsfr_base          0x20000                // bt; wire less
#define lsfr_base          0x10000                // lsb
#define map_adr(grp, adr)  ((64 * grp + adr) * 4) // grp(0x0-0xff), adr(0x0-0x3f)

#define ___RW      volatile       // read write
#define ___RO      volatile const // only read
#define ___WO      volatile       // only write

#define ___u8(x)   unsigned char(x); char(reserved_1_##x); char(reserved_2_##x); char(reserved_3_##x)
#define ___u16(x)  unsigned short(x); short(reserved_1_##x)
#define ___u32(x)  unsigned int(x)

#define ___s8(x)   char(x); char(reserved_1_##x); char(reserved_2_##x); char(reserved_3_##x)
#define ___s16(x)  short(x); short(reserved_1_##x)
#define ___s32(x)  int(x)

//  #include "wl82_private.h"


//===============================================================================//
//
//      high speed sfr address define
//
//===============================================================================//
//............. 0x0200 - 0x02ff............
typedef struct {
    ___RW ___u32(CON);
    ___WO ___u32(BAUD);
    ___WO ___u32(CODE);
    ___WO ___u32(BASE_ADR);
} JL_SFC_TypeDef;

#define JL_SFC_BASE                     (hsfr_base + map_adr(0x02, 0x00))
#define JL_SFC                          ((JL_SFC_TypeDef    *)JL_SFC_BASE)

//............. 0x0300 - 0x03ff............ for sfc encrypt
typedef struct {
    ___RW ___u8(CON);
    ___RW ___u16(KEY);
    ___WO ___u32(UNENC_ADRH);
    ___WO ___u32(UNENC_ADRL);
    ___WO ___u32(LENC_ADRH);
    ___WO ___u32(LENC_ADRL);
} JL_SFCENC_TypeDef;

#define JL_SFCENC_BASE                  (hsfr_base + map_adr(0x03, 0x00))
#define JL_SFCENC                       ((JL_SFCENC_TypeDef *)JL_SFCENC_BASE)

//............. 0x0400 - 0x04ff............
typedef struct {
    ___RW ___u32(CON0);
    ___WO ___u32(CON1);
    ___RW ___u32(CON2);
    ___RW ___u32(CON3);
    ___RW ___u32(CON4);
    ___RW ___u32(CON5);
    ___RW ___u32(CON6);
    ___RW ___u32(CON7);
    ___RW ___u32(CON8);
    ___RW ___u32(CON9);
    ___RW ___u32(CON10);
    ___RW ___u32(CON11);
    ___RW ___u32(CON12);
    ___RW ___u32(CON13);
    ___RW ___u32(DBG_INFO);
    ___RW ___u32(DBG_START);
    ___RW ___u32(DBG_END);
    ___RW ___u32(DBG_ADR);
} JL_SDR_TypeDef;

#define JL_SDR_BASE                     (hsfr_base + map_adr(0x04, 0x00))
#define JL_SDR                          ((JL_SDR_TypeDef    *)JL_SDR_BASE)

//............. 0x0500 - 0x05ff............
typedef struct {
    ___RW ___u32(CON);
    ___WO ___u32(BAUD);
    ___WO ___u32(QUCNT);
} JL_PSRAM_TypeDef;

#define JL_PSRAM_BASE                   (hsfr_base + map_adr(0x05, 0x00))
#define JL_PSRAM                        ((JL_PSRAM_TypeDef  *)JL_PSRAM_BASE)

//............. 0x1000 - 0x10ff............

//............. 0x1100 - 0x11ff............
typedef struct {
    ___RW ___u32(MBISTCTLH);
    ___RO ___u32(MBISTSOGOH);
} JL_MBISTH_TypeDef;

#define JL_MBISTH_BASE                  (hsfr_base + map_adr(0x11, 0x00))
#define JL_MBISTH                       ((JL_MBISTH_TypeDef    *)JL_MBISTH_BASE)
//
//............. 0x1200 - 0x12ff............
typedef struct {
    ___RW ___u32(CON);
    ___RW ___u32(DATIN);
    ___WO ___u32(KEY);
    ___RW ___u32(ENCRES0);
    ___RW ___u32(ENCRES1);
    ___RW ___u32(ENCRES2);
    ___RW ___u32(ENCRES3);
    ___RW ___u32(DECRES0);
    ___RW ___u32(DECRES1);
    ___RW ___u32(DECRES2);
    ___RW ___u32(DECRES3);
} JL_AES_TypeDef;

#define JL_AES_BASE                     (hsfr_base + map_adr(0x12, 0x00))
#define JL_AES                          ((JL_AES_TypeDef    *)JL_AES_BASE)

//............. 0x1300 - 0x13ff............
typedef struct {
    ___RW ___u32(CON);
    ___RW ___u32(ADR);
} JL_DCP_TypeDef;

#define JL_DCP_BASE                     (hsfr_base + map_adr(0x13, 0x00))
#define JL_DCP                          ((JL_DCP_TypeDef      *)JL_DCP_BASE)

//............. 0x1400 - 0x14ff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
    ___RW ___u32(ADR);
    ___RW ___u32(CNT);
} JL_EMI_TypeDef;

#define JL_EMI_BASE                     (hsfr_base + map_adr(0x14, 0x00))
#define JL_EMI                          ((JL_EMI_TypeDef       *)JL_EMI_BASE)

//............. 0x1900 - 0x19ff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
    ___RW ___u32(YUVLINE);
    ___RW ___u32(YDCVAL);
    ___RW ___u32(UDCVAL);
    ___RW ___u32(VDCVAL);
    ___RW ___u32(YPTR);
    ___RW ___u32(UPTR);
    ___RW ___u32(VPTR);
    ___RW ___u32(BADDR);
    ___RW ___u32(BCNT);
    ___RW ___u32(MCUCNT);
    ___RW ___u32(PRECNT);
    ___RW ___u32(HQTBLADR);
    ___RW ___u32(HQTBLDAT);
    ___RW ___u32(PTR_NUM);
    ___RW ___u32(HAS_PTR);
    ___RW ___u32(MCU_BUF);

} JL_JPG_TypeDef;

#define JL_JPG_BASE                   (hsfr_base + map_adr(0x19, 0x00))
#define JL_JPG                        ((JL_JPG_TypeDef  *)JL_JPG_BASE)

//............. 0x1a00 - 0x1bff............
typedef struct {
    ___RW ___u32(COM_CON);
    ___RW ___u32(PND_CON);
    ___RW ___u32(PND_CLR);
    ___RW ___u32(RCV_CON);
    ___RW ___u32(RCV_VTT);
    ___RW ___u32(RCV_VBLK);
    ___RW ___u32(RCV_VACT);
    ___RW ___u32(RCV_HTT);
    ___RW ___u32(RCV_HBLK);
    ___RW ___u32(RCV_HACT);
    ___RW ___u32(FRC_CON);
    ___RW ___u32(OSD_CON);
    ___RW ___u32(OSD_BASE);
    ___RW ___u32(OSD_H_CFG);
    ___RW ___u32(OSD_V_CFG);
    ___RW ___u32(OSD_COLOR);
    ___RW ___u32(DMA_CON);
    ___RW ___u32(DMA_CNT);
    ___RW ___u32(DMA_BASE0_Y);
    ___RW ___u32(DMA_BASE0_U);
    ___RW ___u32(DMA_BASE0_V);
    ___RW ___u32(DMA_BASE1_Y);
    ___RW ___u32(DMA_BASE1_U);
    ___RW ___u32(DMA_BASE1_V);
} JL_ISC_TypeDef;

#define JL_ISC0_BASE                  (hsfr_base + map_adr(0x1a, 0x00))
#define JL_ISC0                       ((JL_ISC_TypeDef  *)JL_ISC0_BASE)

#define JL_ISC1_BASE                  (hsfr_base + map_adr(0x1b, 0x00))
#define JL_ISC1                       ((JL_ISC_TypeDef  *)JL_ISC1_BASE)

//............. 0x1c00 - 0x1cff............
typedef struct {
    ___RW ___u32(CON);
    ___RW ___u32(WR_ALLOW_ID0);
    ___RW ___u32(WR_ALLOW_ID1);
    ___RW ___u32(LIMIT_H);
    ___RW ___u32(LIMIT_L);
    ___RW ___u32(WR_LIMIT_ID);
} JL_DBG_TypeDef;

#define JL_DBG_BASE                   (hsfr_base + map_adr(0x1c, 0x00))
#define JL_DBG                        ((JL_DBG_TypeDef        *)JL_DBG_BASE)


//............. 0x1d00 - 0x1dff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
    ___RW ___u32(CON3);
    ___RW ___u32(LD_START_ADR);
    ___RW ___u32(DEBUG_PIXEL);
    ___RW ___u32(SPI_CON0);
    ___RW ___u16(SPI_BAUD);
    ___RW ___u32(SPI_BUF);
    ___RW ___u32(PAP_CON0);
    ___RW ___u32(PAP_PRD);
    ___RW ___u32(PAP_BUF);
    ___RW ___u32(RGB_CON0);
    ___RW ___u32(RGB_HPRD);
    ___RW ___u32(RGB_VPRD);
} JL_IMD_TypeDef;

#define JL_IMD_BASE                   (hsfr_base + map_adr(0x1d, 0x00))
#define JL_IMD                        ((JL_IMD_TypeDef  *)JL_IMD_BASE)


//===============================================================================//
//
//      port low speed sfr address define
//
//===============================================================================//
//............. 0x0000 - 0x00ff............
typedef struct {
    ___RW ___u32(OUT);
    ___RO ___u32(IN);
    ___RW ___u32(DIR);
    ___RW ___u32(DIE);
    ___RW ___u32(PU);
    ___RW ___u32(PD);
    ___RW ___u32(HD0);
    ___RW ___u32(HD);
    ___RW ___u32(DIEH);
} JL_PORT_FLASH_TypeDef;

#define JL_PORTA_BASE                   (psfr_base + map_adr(0x00, 0x00))
#define JL_PORTA                        ((JL_PORT_FLASH_TypeDef *)JL_PORTA_BASE)

#define JL_PORTB_BASE                   (psfr_base + map_adr(0x00, 0x10))
#define JL_PORTB                        ((JL_PORT_FLASH_TypeDef *)JL_PORTB_BASE)

#define JL_PORTC_BASE                   (psfr_base + map_adr(0x00, 0x20))
#define JL_PORTC                        ((JL_PORT_FLASH_TypeDef *)JL_PORTC_BASE)

#define JL_PORTD_BASE                   (psfr_base + map_adr(0x00, 0x30))
#define JL_PORTD                        ((JL_PORT_FLASH_TypeDef *)JL_PORTD_BASE)

#define JL_PORTE_BASE                   (psfr_base + map_adr(0x00, 0x40))
#define JL_PORTE                        ((JL_PORT_FLASH_TypeDef *)JL_PORTE_BASE)

#define JL_PORTF_BASE                   (psfr_base + map_adr(0x00, 0x50))
#define JL_PORTF                        ((JL_PORT_FLASH_TypeDef *)JL_PORTF_BASE)

#define JL_PORTG_BASE                   (psfr_base + map_adr(0x00, 0x60))
#define JL_PORTG                        ((JL_PORT_FLASH_TypeDef *)JL_PORTG_BASE)

#define JL_PORTH_BASE                   (psfr_base + map_adr(0x00, 0x70))
#define JL_PORTH                        ((JL_PORT_FLASH_TypeDef *)JL_PORTH_BASE)

//............. 0x1000 - 0x10ff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
} JL_USB_IO_TypeDef;

#define JL_USB_IO_BASE                  (psfr_base + map_adr(0x10, 0x00))
#define JL_USB_IO                       ((JL_USB_IO_TypeDef    *)JL_USB_IO_BASE)

typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
    ___RW ___u32(CON3);
} JL_WAKEUP_TypeDef;

#define JL_WAKEUP_BASE                  (psfr_base + map_adr(0x10, 0x03))
#define JL_WAKEUP                       ((JL_WAKEUP_TypeDef    *)JL_WAKEUP_BASE)

typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
    ___RW ___u32(CON3);
    ___RW ___u32(CON4);
    ___RW ___u32(CON5);
    ___RW ___u32(CON6);
    ___RW ___u32(CON7);
    ___RW ___u32(CON8);
} JL_IOMAP_TypeDef;

#define JL_IOMAP_BASE                   (psfr_base + map_adr(0x10, 0x07))
#define JL_IOMAP                        ((JL_IOMAP_TypeDef      *)JL_IOMAP_BASE)

//............. 0x1100 - 0x11ff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW   ___u32(RESERVED)[1];
    ___RW ___u32(SEG_IOEN0);
    ___RW ___u32(SEG_IOEN1);
} JL_LCDC_TypeDef;

#define JL_LCDC_BASE                    (psfr_base + map_adr(0x11, 0x00))
#define JL_LCDC                         ((JL_LCDC_TypeDef     *)JL_LCDC_BASE)


//===============================================================================//
//
//      lsb_top low speed sfr address define
//
//===============================================================================//
//............. 0x0000 - 0x00ff............
typedef struct {
    ___RW ___u32(CON);
} JL_POWER_TypeDef;

#define JL_POWER_BASE                   (lsfr_base + map_adr(0x00, 0x00))
#define JL_POWER                        ((JL_POWER_TypeDef      *)JL_POWER_BASE)

typedef struct {
    ___RW ___u32(PWR_CON);          //same with JL_POWER->CON
    ___RW ___u32(RESERVED0);
    ___RW ___u32(SYS_DIV);
    ___RW ___u32(CLK_CON0);
    ___RW ___u32(CLK_CON1);
    ___RW ___u32(CLK_CON2);
    ___RW ___u32(CLK_CON3);
} JL_CLOCK_TypeDef;

#define JL_CLOCK_BASE                   (lsfr_base + map_adr(0x00, 0x00))
#define JL_CLOCK                        ((JL_CLOCK_TypeDef      *)JL_CLOCK_BASE)

typedef struct {
    ___RW ___u32(RST_SRC);
} JL_SYSTEM_TypeDef;

#define JL_SYSTEM_BASE                  (lsfr_base + map_adr(0x00, 0x30))
#define JL_SYSTEM                       ((JL_SYSTEM_TypeDef     *)JL_SYSTEM_BASE)

//............. 0x0100 - 0x01ff............ for mode det
typedef struct {
    ___RW ___u32(CON);
} JL_MODE_TypeDef;

#define JL_MODE_BASE                    (lsfr_base + map_adr(0x01, 0x00))
#define JL_MODE                         ((JL_MODE_TypeDef       *)JL_MODE_BASE)

//............. 0x0200 - 0x02ff............ for intest
typedef struct {
    ___RW ___u32(CHIP_ID);
} JL_INTEST_TypeDef;

#define JL_INTEST_BASE                  (lsfr_base + map_adr(0x02, 0x00))
#define JL_INTEST                       ((JL_INTEST_TypeDef     *)JL_INTEST_BASE)

//............. 0x0300 - 0x03ff............

//............. 0x0400 - 0x07ff............
typedef struct {
    ___RW ___u32(CON);
    ___RW ___u32(CNT);
    ___RW ___u32(PRD);
    ___RW ___u32(PWM);
} JL_TIMER_TypeDef;

#define JL_TIMER0_BASE                  (lsfr_base + map_adr(0x04, 0x00))
#define JL_TIMER0                       ((JL_TIMER_TypeDef     *)JL_TIMER0_BASE)

#define JL_TIMER1_BASE                  (lsfr_base + map_adr(0x05, 0x00))
#define JL_TIMER1                       ((JL_TIMER_TypeDef     *)JL_TIMER1_BASE)

#define JL_TIMER2_BASE                  (lsfr_base + map_adr(0x06, 0x00))
#define JL_TIMER2                       ((JL_TIMER_TypeDef     *)JL_TIMER2_BASE)

#define JL_TIMER3_BASE                  (lsfr_base + map_adr(0x07, 0x00))
#define JL_TIMER3                       ((JL_TIMER_TypeDef     *)JL_TIMER3_BASE)

#define JL_TIMER4_BASE                  (lsfr_base + map_adr(0x08, 0x00))
#define JL_TIMER4                       ((JL_TIMER_TypeDef     *)JL_TIMER4_BASE)

#define JL_TIMER5_BASE                  (lsfr_base + map_adr(0x09, 0x00))
#define JL_TIMER5                       ((JL_TIMER_TypeDef     *)JL_TIMER5_BASE)

//............. 0x1000 - 0x10ff............
typedef struct {
    ___RW ___u8(CON);
    ___RW ___u32(VAL);
} JL_PCNT_TypeDef;

#define JL_PCNT_BASE                    (lsfr_base + map_adr(0x10, 0x00))
#define JL_PCNT                         ((JL_PCNT_TypeDef       *)JL_PCNT_BASE)

//............. 0x1100 - 0x11ff............
typedef struct {
    ___RW ___u32(CON);
    ___RW ___u32(NUM);
} JL_GPCNT_TypeDef;

#define JL_GPCNT_BASE                   (lsfr_base + map_adr(0x11, 0x00))
#define JL_GPCNT                        ((JL_GPCNT_TypeDef     *)JL_GPCNT_BASE)

//............. 0x1400 - 0x16ff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
    ___WO ___u32(CPTR);
    ___WO ___u32(DPTR);
    ___RW ___u32(CTU_CON);
    ___WO ___u32(CTU_CNT);
} JL_SD_TypeDef;

#define JL_SD0_BASE                     (lsfr_base + map_adr(0x14, 0x00))
#define JL_SD0                          ((JL_SD_TypeDef        *)JL_SD0_BASE)

#define JL_SD1_BASE                     (lsfr_base + map_adr(0x15, 0x00))
#define JL_SD1                          ((JL_SD_TypeDef        *)JL_SD1_BASE)


//............. 0x1700 - 0x17ff............
typedef struct {
    ___RW ___u32(CON);
    ___WO ___u32(ADR);
    ___RW ___u32(RESULT0);
    ___RW ___u32(RESULT1);
    ___RW ___u32(RESULT2);
    ___RW ___u32(RESULT3);
    ___RW ___u32(RESULT4);
    ___RW ___u32(RESULT5);
    ___RW ___u32(RESULT6);
    ___RW ___u32(RESULT7);
} JL_SHA_TypeDef;

#define JL_SHA_BASE                     (lsfr_base + map_adr(0x17, 0x00))
#define JL_SHA                          ((JL_SHA_TypeDef        *)JL_SHA_BASE)

//............. 0x1800 - 0x18ff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___WO ___u32(EP0_CNT);
    ___WO ___u32(EP1_CNT);
    ___WO ___u32(EP2_CNT);
    ___WO ___u32(EP3_CNT);
    ___WO ___u32(EP0_ADR);
    ___WO ___u32(EP1_TADR);
    ___WO ___u32(EP1_RADR);
    ___WO ___u32(EP2_TADR);
    ___WO ___u32(EP2_RADR);
    ___WO ___u32(EP3_TADR);
    ___WO ___u32(EP3_RADR);
    ___WO ___u32(EP4_CNT);
    ___WO ___u32(EP4_TADR);
    ___WO ___u32(EP4_RADR);
} JL_USB_TypeDef;

#define JL_USB_BASE                     (lsfr_base + map_adr(0x18, 0x00))
#define JL_USB                          ((JL_USB_TypeDef       *)JL_USB_BASE)

//............. 0x1900 - 0x19ff............
typedef struct {
    ___RW ___u32(WLA_CON0) ;
    ___RW ___u32(WLA_CON1) ;
    ___RW ___u32(WLA_CON2) ;
    ___RW ___u32(WLA_CON3) ;
    ___RW ___u32(WLA_CON4) ;
    ___RW ___u32(WLA_CON5) ;
    ___RW ___u32(WLA_CON6) ;
    ___RW ___u32(WLA_CON7) ;
    ___RW ___u32(WLA_CON8) ;
    ___RW ___u32(WLA_CON9) ;
    ___RW ___u32(WLA_CON10);
    ___RW ___u32(WLA_CON11);
    ___RW ___u32(WLA_CON12);
    ___RW ___u32(WLA_CON13);
    ___RW ___u32(WLA_CON14);
    ___RW ___u32(WLA_CON15);
    ___RW ___u32(WLA_CON16);
    ___RW ___u32(WLA_CON17);
    ___RW ___u32(WLA_CON18);
    ___RW ___u32(WLA_CON19);
    ___RW ___u32(WLA_CON20);
    ___RW ___u32(WLA_CON21);
    ___RW ___u32(WLA_CON22);
    ___RW ___u32(WLA_CON23);
    ___RW ___u32(WLA_CON24);
    ___RW ___u32(WLA_CON25);
    ___RW ___u32(WLA_CON26);
    ___RW ___u32(WLA_CON27);
    ___RW ___u32(WLA_CON28);
    ___RW ___u32(WLA_CON29);
    ___RW ___u32(WLA_CON30);
    ___RO ___u32(WLA_CON31);
    ___RO ___u32(WLA_CON32);
    ___RO ___u32(WLA_CON33);
    ___RO ___u32(WLA_CON34);
    ___RO ___u32(WLA_CON35);
    ___RO ___u32(WLA_CON36);
    ___RO ___u32(WLA_CON37);
    ___RO ___u32(WLA_CON38);
    ___RO ___u32(WLA_CON39);
    ___RW ___u32(PLL_CON0);
    ___RW ___u32(PLL_CON1);
    ___RW ___u32(PLL2_CON0);
    ___RW ___u32(PLL2_CON1);
    ___RO   ___u32(RESERVED0)[0x30 - 0x2b - 1];
    ___RW ___u32(DAA_CON0);
    ___RW ___u32(DAA_CON1);
    ___RW ___u32(DAA_CON2);
    ___RW ___u32(DAA_CON3);
    ___RO   ___u32(RESERVED1)[0x37 - 0x33 - 1];
    ___RW ___u32(DAA_CON7);
    ___RW ___u32(ADA_CON0);
    ___RW ___u32(ADA_CON1);
    ___RW ___u32(ADA_CON2);
} JL_ANA_TypeDef;

#define JL_ANA_BASE                     (lsfr_base + map_adr(0x19, 0x00))
#define JL_ANA                          ((JL_ANA_TypeDef       *)JL_ANA_BASE)

//............. 0x1c00 - 0x1eff............
typedef struct {
    ___RW ___u32(CON);
    ___WO ___u32(BAUD);
    ___RW ___u32(BUF);
    ___WO ___u32(ADR);
    ___RW ___u32(CNT);
} JL_SPI_TypeDef;

#define JL_SPI0_BASE                    (lsfr_base + map_adr(0x1c, 0x00))
#define JL_SPI0                         ((JL_SPI_TypeDef      *)JL_SPI0_BASE)

#define JL_SPI1_BASE                    (lsfr_base + map_adr(0x1d, 0x00))
#define JL_SPI1                         ((JL_SPI_TypeDef      *)JL_SPI1_BASE)

#define JL_SPI2_BASE                    (lsfr_base + map_adr(0x1e, 0x00))
#define JL_SPI2                         ((JL_SPI_TypeDef      *)JL_SPI2_BASE)

#define JL_SPI3_BASE                    (lsfr_base + map_adr(0x1f, 0x00))
#define JL_SPI3                         ((JL_SPI_TypeDef      *)JL_SPI3_BASE)

//............. 0x2000 - 0x22ff............
typedef struct {
    ___RW ___u16(CON0);
    ___RW ___u16(CON1);
    ___WO ___u16(BAUD);
    ___RW ___u8(BUF);
    ___RW ___u32(OTCNT);
    ___RW ___u32(TXADR);
    ___WO ___u16(TXCNT);
    ___RW ___u32(RXSADR);
    ___RW ___u32(RXEADR);
    ___RW ___u32(RXCNT);
    ___RO ___u16(HRXCNT);
} JL_UART_TypeDef;

#define JL_UART0_BASE                   (lsfr_base + map_adr(0x20, 0x00))
#define JL_UART0                        ((JL_UART_TypeDef       *)JL_UART0_BASE)

#define JL_UART1_BASE                   (lsfr_base + map_adr(0x21, 0x00))
#define JL_UART1                        ((JL_UART_TypeDef       *)JL_UART1_BASE)

#define JL_UART2_BASE                   (lsfr_base + map_adr(0x22, 0x00))
#define JL_UART2                        ((JL_UART_TypeDef      *)JL_UART2_BASE)

//............. 0x2400 - 0x24ff............
typedef struct {
    ___RW ___u16(CON0);
    ___RW ___u8(BUF);
    ___WO ___u8(BAUD);
    ___RW ___u16(CON1);
} JL_IIC_TypeDef;

#define JL_IIC_BASE                     (lsfr_base + map_adr(0x24, 0x00))
#define JL_IIC                          ((JL_IIC_TypeDef       *)JL_IIC_BASE)

//............. 0x2700 - 0x27ff............
#define JL_USB1_BASE                    (lsfr_base + map_adr(0x27, 0x00))
#define JL_USB1                         ((JL_USB_TypeDef       *)JL_USB1_BASE)

//............. 0x2800 - 0x28ff............
typedef struct {
    ___RW ___u32(CON);
    ___WO ___u32(DAT0);
    ___WO ___u32(DAT1);
    ___RW ___u32(BUF);
    ___WO ___u32(ADR);
    ___WO ___u32(CNT);
} JL_PAP_TypeDef;

#define JL_PAP_BASE                     (lsfr_base + map_adr(0x28, 0x00))
#define JL_PAP                          ((JL_PAP_TypeDef       *)JL_PAP_BASE)

//............. 0x2b00 - 0x2bff............
typedef struct {
    ___RW ___u32(CON);
    ___RO ___u32(SR_CNT);
    ___RW ___u32(IO_CON);
    ___RW ___u32(DMA_CON);
    ___RW ___u32(DMA_LEN);
    ___WO ___u32(DAT_ADR);
    ___WO ___u32(INF_ADR);
    ___RO ___u32(CSB0);
    ___RO ___u32(CSB1);
    ___RO ___u32(CSB2);
    ___RO ___u32(CSB3);
    ___RO ___u32(CSB4);
    ___RO ___u32(CSB5);
} JL_SS_TypeDef;

#define JL_SS_BASE                      (lsfr_base + map_adr(0x2b, 0x00))
#define JL_SS                           ((JL_SS_TypeDef        *)JL_SS_BASE)

//............. 0x2c00 - 0x2cff............
typedef struct {
    ___RW ___u8(CON);
    ___RW ___u8(DAT);
} JL_RDEC_TypeDef;

#define JL_RDEC_BASE                    (lsfr_base + map_adr(0x2c, 0x00))
#define JL_RDEC                         ((JL_RDEC_TypeDef       *)JL_RDEC_BASE)

//............. 0x2d00 - 0x2dff............
typedef struct {
    ___RW ___u16(CON);
    ___RW ___u8(SMR);
    ___RW ___u32(ADR);
    ___RW ___u32(LEN);
} JL_PLNK_TypeDef;

#define JL_PLNK0_BASE                   (lsfr_base + map_adr(0x2d, 0x00))
#define JL_PLNK0                        ((JL_PLNK_TypeDef     *)JL_PLNK0_BASE)

#define JL_PLNK1_BASE                   (lsfr_base + map_adr(0x52, 0x00))
#define JL_PLNK1                        ((JL_PLNK_TypeDef     *)JL_PLNK1_BASE)

//............. 0x2e00 - 0x2eff............
typedef struct {
    ___RW ___u16(CON0);
    ___RW ___u16(CON1);
    ___RW ___u8(CON2);
    ___RW ___u8(CON3);
    ___WO ___u32(ADR0);
    ___WO ___u32(ADR1);
    ___WO ___u32(ADR2);
    ___WO ___u32(ADR3);
    ___WO ___u16(LEN);
} JL_ALNK_TypeDef;

#define JL_ALNK0_BASE                   (lsfr_base + map_adr(0x2e, 0x00))
#define JL_ALNK0                        ((JL_ALNK_TypeDef    *)JL_ALNK0_BASE)

#define JL_ALNK1_BASE                   (lsfr_base + map_adr(0x51, 0x00))
#define JL_ALNK1                        ((JL_ALNK_TypeDef    *)JL_ALNK1_BASE)

//............. 0x2f00 - 0x2fff............
typedef struct {
    ___RW ___u32(DAC_CON);
    ___RW ___u32(DAC_ADR);
    ___RW ___u16(DAC_LEN);
    ___RW ___u16(DAC_PNS);
    ___RW ___u16(DAC_HRP);
    ___RW ___u16(DAC_SWP);
    ___RW ___u16(DAC_SWN);
    ___RO ___u32(RESERVED7);
    ___RW ___u32(DAC_VL0);
    ___RW ___u32(DAC_VL1);
    ___RW ___u32(DAC_TM0);
    ___RW ___u32(DAC_TM1);
    ___RW ___u16(DAC_DTV);
    ___RW ___u16(DAC_DTB);
    ___RO ___u32(RESERVEDe);
    ___WO ___u16(DAC_COP);
    ___RW ___u32(ADC_CON);
    ___RW ___u32(ADC_ADR);
    ___RW ___u16(ADC_LEN);
    ___RW ___u16(ADC_PNS);
    ___RW ___u16(ADC_HWP);
    ___RW ___u16(ADC_SRP);
    ___RW ___u16(ADC_SRN);
} JL_AUDIO_TypeDef;

#define JL_AUDIO_BASE                   (lsfr_base + map_adr(0x2f, 0x00))
#define JL_AUDIO                        ((JL_AUDIO_TypeDef   *)JL_AUDIO_BASE)

//............. 0x3000 - 0x30ff............
typedef struct {
    ___RW ___u32(CON);
    ___WO ___u32(KEY);
    ___WO ___u32(ADR);
} JL_ENC_TypeDef;

#define JL_ENC_BASE                     (lsfr_base + map_adr(0x30, 0x00))
#define JL_ENC                          ((JL_ENC_TypeDef    *)JL_ENC_BASE)

//............. 0x3100 - 0x31ff............
typedef struct {
    ___RW ___u32(CON);
    ___RO ___u32(RES);
} JL_ADC_TypeDef;

#define JL_ADC_BASE                     (lsfr_base + map_adr(0x31, 0x00))
#define JL_ADC                          ((JL_ADC_TypeDef       *)JL_ADC_BASE)

//............. 0x3200 - 0x32ff............
typedef struct {
    ___RW ___u32(RFLT_CON);
} JL_IR_TypeDef;

#define JL_IR_BASE                      (lsfr_base + map_adr(0x32, 0x00))
#define JL_IR                           ((JL_IR_TypeDef         *)JL_IR_BASE)

//............. 0x3300 - 0x33ff............

//............. 0x3400 - 0x34ff............
typedef struct {
    ___RW ___u32(CON);
} JL_OSA_TypeDef;

#define JL_OSA_BASE                     (lsfr_base + map_adr(0x34, 0x00))
#define JL_OSA                          ((JL_OSA_TypeDef          *)JL_OSA_BASE)

//............. 0x3500 - 0x35ff............
typedef struct {
    ___WO ___u32(FIFO);
    ___RW ___u32(REG);
} JL_CRC_TypeDef;

#define JL_CRC_BASE                     (lsfr_base + map_adr(0x35, 0x00))
#define JL_CRC                          ((JL_CRC_TypeDef       *)JL_CRC_BASE)

//............. 0x3600 - 0x36ff............
typedef struct {
    ___WO ___u32(CON);
    ___RW ___u32(NUM);
} JL_LRCT_TypeDef;

#define JL_LRCT_BASE                    (lsfr_base + map_adr(0x36, 0x00))
#define JL_LRCT                         ((JL_LRCT_TypeDef     *)JL_LRCT_BASE)

//............. 0x3700 - 0x37ff............
typedef struct {
    ___WO ___u32(CON);
    ___RO   ___u32(RESERVED)[8 - 0 - 1];
    ___WO ___u32(ME);
} JL_EFUSE_TypeDef;

#define JL_EFUSE_BASE                   (lsfr_base + map_adr(0x37, 0x00))
#define JL_EFUSE                        ((JL_EFUSE_TypeDef     *)JL_EFUSE_BASE)

//............. 0x3b00 - 0x3bff............
typedef struct {
    ___RO ___u32(R64L);
    ___RO ___u32(R64H);
} JL_RAND_TypeDef;

#define JL_RAND_BASE                    (lsfr_base + map_adr(0x3b, 0x00))
#define JL_RAND                         ((JL_RAND_TypeDef   *)JL_RAND_BASE)

//............. 0x3c00 - 0x3cff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RO ___u32(RES);
} JL_CTM_TypeDef;

#define JL_CTM_BASE                     (lsfr_base + map_adr(0x3c, 0x00))
#define JL_CTM                          ((JL_CTM_TypeDef    *)JL_CTM_BASE)

//............. 0x3d00 - 0x3dff............ reserved
typedef struct {
    ___RW ___u32(CON0)         ;
    ___RW ___u32(DEC_SRC_ADR)  ;
    ___RW ___u32(DEC_DST_ADR)  ;
    ___RW ___u32(DEC_PCM_WCNT) ;
    ___WO ___u32(DEC_INBUF_LEN);
    ___WO ___u32(ENC_SRC_ADR)  ;
    ___WO ___u32(ENC_DST_ADR)  ;
    ___RO ___u32(DEC_DST_BASE) ;
} JL_SBC_TypeDef;

#define JL_SBC_BASE                  	(lsfr_base + map_adr(0x3d, 0x00))
#define JL_SBC                       	((JL_SBC_TypeDef    *)JL_SBC_BASE)

//............. 0x3e00 - 0x3eff............ for p33
typedef struct {
    ___RW ___u32(PMU_CON);
    ___RW ___u32(RTC_CON);
    ___RW ___u32(SPI_CON);
    ___RW ___u32(SPI_DAT);
} JL_P33_TypeDef;

#define JL_P33_BASE                 (lsfr_base + map_adr(0x3e, 0x00))
#define JL_P33                      ((JL_P33_TypeDef        *)JL_P33_BASE)

//............. 0x3f00 - 0x3fff............ for dma
typedef struct {
    ___RW ___u32(PRI0);
    ___RW ___u32(PRI1);
    ___RW ___u32(PRI2);
    ___RW ___u32(PRI3);
    ___RW ___u32(PRI4);
    ___u32(RESERVED0[0x08 - 0x04 - 1]);
    ___RW ___u32(MSG);
    ___RO ___u32(MSG_CH);
    ___RW ___u32(RDH);
    ___RW ___u32(WRL);
    ___RW ___u32(WRH);
} JL_DMA_TypeDef;

#define JL_DMA_BASE                 (lsfr_base + map_adr(0x3f, 0x00))
#define JL_DMA                      ((JL_DMA_TypeDef        *)JL_DMA_BASE)

//............. 0x4000 - 0x40ff............
typedef struct {
    ___RW ___u32(WL_CON0);
    ___RW ___u32(WL_CON1);
    ___RW ___u32(WL_CON2);
    ___RW ___u32(WL_CON3);
    ___RW ___u32(WL_CON4);
    ___RW ___u32(WL_CON5);
    ___RW ___u32(WL_CON6);
    ___RW ___u32(WL_CON7);
    ___RW ___u32(LOFC_CON);
    ___RW ___u32(LOFC_RES);
    ___RW ___u32(WL_ANL_CON0);
    ___RW ___u32(WL_ANL_CON1);
    ___RW ___u32(WL_ANL_CON2);
    ___RW ___u32(WL_ANL_CON3);
    ___u32(RESERVED0[0x10 - 0x0d - 1]);
    ___RW ___u32(WF_CON0);
    ___RW ___u32(WF_CON1);
    ___RW ___u32(WF_CON2);
    ___RW ___u32(WF_CON3);
    ___RW ___u32(WF_CON4);
    ___RW ___u32(WF_CON5);
    ___RW ___u32(WF_CON6);
    ___RW ___u32(WF_CON7);
    ___RW ___u32(WF_CON8);
    ___RW ___u32(WF_CON9);
    ___u32(RESERVED1[0x20 - 0x19 - 1]);
    ___RW ___u32(SD_CON0);
    ___RW ___u32(SD_CON1);
    ___RW ___u32(SD_CON2);
    ___RW ___u32(SD_CTU_CON);
    ___WO ___u32(SD_CTU_CNT);
    ___WO ___u32(SD_CPTR);
    ___WO ___u32(SD_DPTR);
} JL_WL_TypeDef;

#define JL_WL_BASE                      (lsfr_base + map_adr(0x40, 0x00))
#define JL_WL                           ((JL_WL_TypeDef     *)JL_WL_BASE)

//............. 0x4200 - 0x42ff............

//............. 0x4300 - 0x43ff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
    ___RW ___u32(CON3);
    ___RW ___u32(IDAT_ADR);
    ___RW ___u32(IDAT_LEN);
    ___RW ___u32(ODAT_ADR);
    ___RW ___u32(ODAT_LEN);
    ___RW ___u32(FLTB_ADR);
} JL_SRC_TypeDef;

#define JL_SRC_BASE                    (lsfr_base + map_adr(0x43, 0x00))
#define JL_SRC                         ((JL_SRC_TypeDef  *)JL_SRC_BASE)

//............. 0x5000 - 0x50ff............
typedef struct {
    ___RW ___u32(TMR0_CON);
    ___RW ___u32(TMR0_CNT);
    ___RW ___u32(TMR0_PR);
    ___RW ___u32(TMR1_CON);
    ___RW ___u32(TMR1_CNT);
    ___RW ___u32(TMR1_PR);
    ___RW ___u32(TMR2_CON);
    ___RW ___u32(TMR2_CNT);
    ___RW ___u32(TMR2_PR);
    ___RW ___u32(TMR3_CON);
    ___RW ___u32(TMR3_CNT);
    ___RW ___u32(TMR3_PR);
    ___RW ___u32(TMR4_CON);
    ___RW ___u32(TMR4_CNT);
    ___RW ___u32(TMR4_PR);
    ___RW ___u32(TMR5_CON);
    ___RW ___u32(TMR5_CNT);
    ___RW ___u32(TMR5_PR);
    ___RW ___u32(TMR6_CON);
    ___RW ___u32(TMR6_CNT);
    ___RW ___u32(TMR6_PR);
    ___RW ___u32(TMR7_CON);
    ___RW ___u32(TMR7_CNT);
    ___RW ___u32(TMR7_PR);
    ___RW ___u32(FPIN_CON);
    ___RW ___u32(CH0_CON0);
    ___RW ___u32(CH0_CON1);
    ___RW ___u32(CH0_CMPH);
    ___RW ___u32(CH0_CMPL);
    ___RW ___u32(CH1_CON0);
    ___RW ___u32(CH1_CON1);
    ___RW ___u32(CH1_CMPH);
    ___RW ___u32(CH1_CMPL);
    ___RW ___u32(CH2_CON0);
    ___RW ___u32(CH2_CON1);
    ___RW ___u32(CH2_CMPH);
    ___RW ___u32(CH2_CMPL);
    ___RW ___u32(CH3_CON0);
    ___RW ___u32(CH3_CON1);
    ___RW ___u32(CH3_CMPH);
    ___RW ___u32(CH3_CMPL);
    ___RW ___u32(CH4_CON0);
    ___RW ___u32(CH4_CON1);
    ___RW ___u32(CH4_CMPH);
    ___RW ___u32(CH4_CMPL);
    ___RW ___u32(CH5_CON0);
    ___RW ___u32(CH5_CON1);
    ___RW ___u32(CH5_CMPH);
    ___RW ___u32(CH5_CMPL);
    ___RW ___u32(CH6_CON0);
    ___RW ___u32(CH6_CON1);
    ___RW ___u32(CH6_CMPH);
    ___RW ___u32(CH6_CMPL);
    ___RW ___u32(CH7_CON0);
    ___RW ___u32(CH7_CON1);
    ___RW ___u32(CH7_CMPH);
    ___RW ___u32(CH7_CMPL);
    ___RW ___u32(MCPWM_CON0);

} JL_MCPWM_TypeDef;

#define JL_MCPWM_BASE                   (lsfr_base + map_adr(0x50, 0x00))
#define JL_PWM                          ((JL_MCPWM_TypeDef  *)JL_MCPWM_BASE)

//............. 0x5100 - 0x51ff............
// Used by ALNK1

//............. 0x5200 - 0x52ff............
// Used by PLNK1

//............. 0x5300 - 0x53ff............
// Used by lsb_core mbist_ctrl
typedef struct {
    ___RW ___u32(MBISTCTLL);
    ___RO ___u32(MBISTSOGOL);
} JL_MBIST_TypeDef;
#define JL_MBIST_BASE                      (lsfr_base + map_adr(0x53, 0x00))
#define JL_MBIST                           ((JL_MBIST_TypeDef     *)JL_MBIST_BASE)


//............. 0x5400 - 0x54ff............
// Used by jl80211 mbist_ctrl
typedef struct {
    ___RW ___u32(MBISTCTLW);
    ___RO ___u32(MBISTSOGOW);
} JL_MBISTW_TypeDef;
#define JL_MBISTW_BASE                      (lsfr_base + map_adr(0x54, 0x00))
#define JL_MBISTW                           ((JL_MBISTW_TypeDef     *)JL_MBISTW_BASE)


//............. 0x6000 - 0x6fff............
#define husb0_sie_base (lsfr_base + map_adr(0x60, 0x00))
#define H0_FADDR         (*(volatile u8  *)(husb0_sie_base + 0x000))
#define H0_POWER         (*(volatile u8  *)(husb0_sie_base + 0x001))
#define H0_INTRTX        (*(volatile u16 *)(husb0_sie_base + 0x002))
#define H0_INTRRX        (*(volatile u16 *)(husb0_sie_base + 0x004))
#define H0_INTRTXE       (*(volatile u16 *)(husb0_sie_base + 0x006))
#define H0_INTRRXE       (*(volatile u16 *)(husb0_sie_base + 0x008))
#define H0_INTRUSB       (*(volatile u8  *)(husb0_sie_base + 0x00a))
#define H0_INTRUSBE      (*(volatile u8  *)(husb0_sie_base + 0x00b))
#define H0_FRAME         (*(volatile u16 *)(husb0_sie_base + 0x00c))
#define H0_INDEX         (*(volatile u8  *)(husb0_sie_base + 0x00e))
#define H0_TESTMODE      (*(volatile u8  *)(husb0_sie_base + 0x00f))
#define H0_FIFO0         (*(volatile u8  *)(husb0_sie_base + 0x020))
#define H0_FIFO1         (*(volatile u8  *)(husb0_sie_base + 0x024))
#define H0_FIFO2         (*(volatile u8  *)(husb0_sie_base + 0x028))
#define H0_FIFO3         (*(volatile u8  *)(husb0_sie_base + 0x02c))
#define H0_FIFO4         (*(volatile u8  *)(husb0_sie_base + 0x030))
#define H0_DEVCTL        (*(volatile u8  *)(husb0_sie_base + 0x060))
#define H0_CSR0          (*(volatile u16 *)(husb0_sie_base + 0x102))
#define H0_COUNT0        (*(volatile u16 *)(husb0_sie_base + 0x108))
#define H0_NAKLIMIT0     (*(volatile u8  *)(husb0_sie_base + 0x10b))
#define H0_CFGDATA       (*(volatile u8  *)(husb0_sie_base + 0x10f))
#define H0_EP1TXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x110))
#define H0_EP1TXCSR      (*(volatile u16 *)(husb0_sie_base + 0x112))
#define H0_EP1RXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x114))
#define H0_EP1RXCSR      (*(volatile u16 *)(husb0_sie_base + 0x116))
#define H0_EP1RXCOUNT    (*(volatile u16 *)(husb0_sie_base + 0x118))
#define H0_EP1TXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x11a))
#define H0_EP1TXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x11b))
#define H0_EP1RXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x11c))
#define H0_EP1RXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x11d))
#define H0_EP1FIFOSIZE   (*(volatile u8  *)(husb0_sie_base + 0x11f))
#define H0_EP2TXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x120))
#define H0_EP2TXCSR      (*(volatile u16 *)(husb0_sie_base + 0x122))
#define H0_EP2RXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x124))
#define H0_EP2RXCSR      (*(volatile u16 *)(husb0_sie_base + 0x126))
#define H0_EP2RXCOUNT    (*(volatile u16 *)(husb0_sie_base + 0x128))
#define H0_EP2TXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x12a))
#define H0_EP2TXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x12b))
#define H0_EP2RXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x12c))
#define H0_EP2RXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x12d))
#define H0_EP2FIFOSIZE   (*(volatile u8  *)(husb0_sie_base + 0x12f))
#define H0_EP3TXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x130))
#define H0_EP3TXCSR      (*(volatile u16 *)(husb0_sie_base + 0x132))
#define H0_EP3RXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x134))
#define H0_EP3RXCSR      (*(volatile u16 *)(husb0_sie_base + 0x136))
#define H0_EP3RXCOUNT    (*(volatile u16 *)(husb0_sie_base + 0x138))
#define H0_EP3TXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x13a))
#define H0_EP3TXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x13b))
#define H0_EP3RXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x13c))
#define H0_EP3RXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x13d))
#define H0_EP3FIFOSIZE   (*(volatile u8  *)(husb0_sie_base + 0x13f))
#define H0_EP4TXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x140))
#define H0_EP4TXCSR      (*(volatile u16 *)(husb0_sie_base + 0x142))
#define H0_EP4RXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x144))
#define H0_EP4RXCSR      (*(volatile u16 *)(husb0_sie_base + 0x146))
#define H0_EP4RXCOUNT    (*(volatile u16 *)(husb0_sie_base + 0x148))
#define H0_EP4TXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x14a))
#define H0_EP4TXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x14b))
#define H0_EP4RXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x14c))
#define H0_EP4RXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x14d))
#define H0_EP4FIFOSIZE   (*(volatile u8  *)(husb0_sie_base + 0x14f))
#define H0_C_T_UCH       (*(volatile u16 *)(husb0_sie_base + 0x344))

#define husb0_bridge_base (lsfr_base + map_adr(0x68, 0x00))
#define H0_SIE_CON       (*(volatile u32 *)(husb0_bridge_base + 0x00*4))
#define H0_EP0_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x01*4))
#define H0_EP1_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x02*4))
#define H0_EP2_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x03*4))
#define H0_EP3_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x04*4))
#define H0_EP4_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x05*4))
#define H0_EP5_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x06*4))
#define H0_EP6_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x07*4))
#define H0_EP1_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x08*4))
#define H0_EP1_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x09*4))
#define H0_EP2_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x0a*4))
#define H0_EP2_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x0b*4))
#define H0_EP3_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x0c*4))
#define H0_EP3_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x0d*4))
#define H0_EP4_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x0e*4))
#define H0_EP4_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x0f*4))
#define H0_EP5_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x10*4))
#define H0_EP5_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x11*4))
#define H0_EP6_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x12*4))
#define H0_EP6_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x13*4))
#define H0_EP3_ISO_CON   (*(volatile u32 *)(husb0_bridge_base + 0x14*4))
#define H0_EP4_ISO_CON   (*(volatile u32 *)(husb0_bridge_base + 0x15*4))

#define husb_phy_base (lsfr_base + map_adr(0x6a, 0x00))
#define HUSB_COM_CON0    (*(volatile u32 *)(husb_phy_base + 0x00*4))
#define HUSB_COM_CON1    (*(volatile u32 *)(husb_phy_base + 0x01*4))
#define HUSB_COM_CON2    (*(volatile u32 *)(husb_phy_base + 0x02*4))
#define HUSB_PHY0_CON0   (*(volatile u32 *)(husb_phy_base + 0x03*4))
#define HUSB_PHY0_CON1   (*(volatile u32 *)(husb_phy_base + 0x04*4))
#define HUSB_PHY0_CON2   (*(volatile u32 *)(husb_phy_base + 0x05*4))


//............. 0xe000 - 0xefff............
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
    ___RW ___u32(CON3);
    ___RW ___u32(DATAI_ADR);
    ___RW ___u32(DATAO_ADR);
    ___RW ___u32(DATA_LEN);
    ___RW ___u32(FLT_ADR);
} JL_EQ_TypeDef;

#define JL_EQ_BASE                      (lsfr_base + map_adr(0xe0, 0x00))
#define JL_EQ                           ((JL_EQ_TypeDef     *)JL_EQ_BASE)

typedef struct {
    ___RW   ___u32(RAM[208]);

} JL_EQ_RAM_TypeDef;

#define JL_EQ_RAM_BASE                  (lsfr_base + map_adr(0xe8, 0x00))
#define JL_EQ_RAM                       ((JL_EQ_RAM_TypeDef   *)JL_EQ_RAM_BASE)


//............. 0xf000 - 0xffff............ for anc     //16x for ram inside access
typedef struct {
    ___RW ___u32(CON0);
    ___RW ___u32(CON1);
    ___RW ___u32(CON2);
    ___RW ___u32(CON3);
    ___RW ___u32(CON4);
    ___RW ___u32(CON5);
    ___RW ___u32(CON6);
    ___RW ___u32(CON7);
    ___RW ___u32(CON8);
    ___RW ___u32(CON9);
    ___RW ___u32(CON10);
    ___RW ___u32(CON11);
    ___RW ___u32(CON12);
    ___RW ___u32(CON13);
} JL_ANC_TypeDef;

#define JL_ANC_BASE                   (lsfr_base + map_adr(0xf0, 0x00))
#define JL_ANC                        ((JL_ANC_TypeDef   *)JL_ANC_BASE)

typedef struct {
    ___RW ___u8(RAM[512 * 4]);
} JL_ANC_RAM_TypeDef;

#define JL_ANC_RAM_BASE                   (lsfr_base + map_adr(0xf8, 0x00))
#define JL_ANC_RAM                        ((JL_ANC_RAM_TypeDef   *)JL_ANC_RAM_BASE)

//............. 0x6000 - 0x60ff............ for

//.............  Full Speed USB ...................
#define FADDR                   0x00
#define POWER                   0x01
#define INTRTX1                 0x02
#define INTRTX2                 0x03
#define INTRRX1                 0x04
#define INTRRX2                 0x05
#define INTRUSB                 0x06
#define INTRTX1E                0x07
#define INTRTX2E                0x08
#define INTRRX1E                0x09
#define INTRRX2E                0x0a
#define INTRUSBE                0x0b
#define FRAME1                  0x0c
#define FRAME2                  0x0d
#define INDEX                   0x0e
#define DEVCTL                  0x0f
#define TXMAXP                  0x10
#define CSR0                    0x11
#define TXCSR1                  0x11
#define TXCSR2                  0x12
#define RXMAXP                  0x13
#define RXCSR1                  0x14
#define RXCSR2                  0x15
#define COUNT0                  0x16
#define RXCOUNT1                0x16
#define RXCOUNT2                0x17
#define TXTYPE                  0x18
#define TXINTERVAL              0x19
#define RXTYPE                  0x1a
#define RXINTERVAL              0x1b

//............. 0x6000 - 0x6fff............
#define husb0_sie_base (lsfr_base + map_adr(0x60, 0x00))
#define H0_FADDR         (*(volatile u8  *)(husb0_sie_base + 0x000))
#define H0_POWER         (*(volatile u8  *)(husb0_sie_base + 0x001))
#define H0_INTRTX        (*(volatile u16 *)(husb0_sie_base + 0x002))
#define H0_INTRRX        (*(volatile u16 *)(husb0_sie_base + 0x004))
#define H0_INTRTXE       (*(volatile u16 *)(husb0_sie_base + 0x006))
#define H0_INTRRXE       (*(volatile u16 *)(husb0_sie_base + 0x008))
#define H0_INTRUSB       (*(volatile u8  *)(husb0_sie_base + 0x00a))
#define H0_INTRUSBE      (*(volatile u8  *)(husb0_sie_base + 0x00b))
#define H0_FRAME         (*(volatile u16 *)(husb0_sie_base + 0x00c))
#define H0_INDEX         (*(volatile u8  *)(husb0_sie_base + 0x00e))
#define H0_TESTMODE      (*(volatile u8  *)(husb0_sie_base + 0x00f))
#define H0_FIFO0         (*(volatile u8  *)(husb0_sie_base + 0x020))
#define H0_FIFO1         (*(volatile u8  *)(husb0_sie_base + 0x024))
#define H0_FIFO2         (*(volatile u8  *)(husb0_sie_base + 0x028))
#define H0_FIFO3         (*(volatile u8  *)(husb0_sie_base + 0x02c))
#define H0_FIFO4         (*(volatile u8  *)(husb0_sie_base + 0x030))
#define H0_DEVCTL        (*(volatile u8  *)(husb0_sie_base + 0x060))

#define H0_EP0_TXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x080))
#define H0_EP0_TXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x082))
#define H0_EP0_TXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x083))
#define H0_EP0_RXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x084))
#define H0_EP0_RXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x086))
#define H0_EP0_RXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x087))

#define H0_EP1_TXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x088))
#define H0_EP1_TXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x08a))
#define H0_EP1_TXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x08b))
#define H0_EP1_RXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x08c))
#define H0_EP1_RXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x08e))
#define H0_EP1_RXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x08f))

#define H0_EP2_TXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x090))
#define H0_EP2_TXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x092))
#define H0_EP2_TXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x093))
#define H0_EP2_RXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x094))
#define H0_EP2_RXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x096))
#define H0_EP2_RXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x097))

#define H0_EP3_TXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x098))
#define H0_EP3_TXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x09a))
#define H0_EP3_TXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x09b))
#define H0_EP3_RXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x09c))
#define H0_EP3_RXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x09e))
#define H0_EP3_RXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x09f))

#define H0_EP4_TXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x0a0))

#define H0_EP4_TXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x0a2))
#define H0_EP4_TXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x0a3))
#define H0_EP4_RXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x0a4))
#define H0_EP4_RXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x0a6))
#define H0_EP4_RXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x0a7))

#define H0_EP5_TXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x0a8))

#define H0_EP5_TXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x0aa))
#define H0_EP5_TXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x0ab))
#define H0_EP5_RXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x0ac))
#define H0_EP5_RXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x0ae))
#define H0_EP5_RXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x0af))

#define H0_EP6_TXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x0b0))

#define H0_EP6_TXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x0b2))
#define H0_EP6_TXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x0b3))
#define H0_EP6_RXFUNCADDR   (*(volatile u8  *)(husb0_sie_base + 0x0b4))
#define H0_EP6_RXHUBADDR    (*(volatile u8  *)(husb0_sie_base + 0x0b6))
#define H0_EP6_RXHUBPORT    (*(volatile u8  *)(husb0_sie_base + 0x0b7))

#define H0_CSR0          (*(volatile u16 *)(husb0_sie_base + 0x102))
#define H0_COUNT0        (*(volatile u16 *)(husb0_sie_base + 0x108))
#define H0_NAKLIMIT0     (*(volatile u8  *)(husb0_sie_base + 0x10b))
#define H0_CFGDATA       (*(volatile u8  *)(husb0_sie_base + 0x10f))
#define H0_EP1TXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x110))
#define H0_EP1TXCSR      (*(volatile u16 *)(husb0_sie_base + 0x112))
#define H0_EP1RXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x114))
#define H0_EP1RXCSR      (*(volatile u16 *)(husb0_sie_base + 0x116))
#define H0_EP1RXCOUNT    (*(volatile u16 *)(husb0_sie_base + 0x118))
#define H0_EP1TXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x11a))
#define H0_EP1TXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x11b))
#define H0_EP1RXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x11c))
#define H0_EP1RXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x11d))
#define H0_EP1FIFOSIZE   (*(volatile u8  *)(husb0_sie_base + 0x11f))
#define H0_EP2TXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x120))
#define H0_EP2TXCSR      (*(volatile u16 *)(husb0_sie_base + 0x122))
#define H0_EP2RXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x124))
#define H0_EP2RXCSR      (*(volatile u16 *)(husb0_sie_base + 0x126))
#define H0_EP2RXCOUNT    (*(volatile u16 *)(husb0_sie_base + 0x128))
#define H0_EP2TXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x12a))
#define H0_EP2TXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x12b))
#define H0_EP2RXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x12c))
#define H0_EP2RXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x12d))
#define H0_EP2FIFOSIZE   (*(volatile u8  *)(husb0_sie_base + 0x12f))
#define H0_EP3TXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x130))
#define H0_EP3TXCSR      (*(volatile u16 *)(husb0_sie_base + 0x132))
#define H0_EP3RXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x134))
#define H0_EP3RXCSR      (*(volatile u16 *)(husb0_sie_base + 0x136))
#define H0_EP3RXCOUNT    (*(volatile u16 *)(husb0_sie_base + 0x138))
#define H0_EP3TXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x13a))
#define H0_EP3TXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x13b))
#define H0_EP3RXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x13c))
#define H0_EP3RXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x13d))
#define H0_EP3FIFOSIZE   (*(volatile u8  *)(husb0_sie_base + 0x13f))
#define H0_EP4TXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x140))
#define H0_EP4TXCSR      (*(volatile u16 *)(husb0_sie_base + 0x142))
#define H0_EP4RXMAXP     (*(volatile u16 *)(husb0_sie_base + 0x144))
#define H0_EP4RXCSR      (*(volatile u16 *)(husb0_sie_base + 0x146))
#define H0_EP4RXCOUNT    (*(volatile u16 *)(husb0_sie_base + 0x148))
#define H0_EP4TXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x14a))
#define H0_EP4TXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x14b))
#define H0_EP4RXTYPE     (*(volatile u8  *)(husb0_sie_base + 0x14c))
#define H0_EP4RXINTERVAL (*(volatile u8  *)(husb0_sie_base + 0x14d))
#define H0_EP4FIFOSIZE   (*(volatile u8  *)(husb0_sie_base + 0x14f))

#define H0_RX_DPKTDIS    (*(volatile u16 *)(husb0_sie_base + 0x340))
#define H0_TX_DPKTDIS    (*(volatile u16 *)(husb0_sie_base + 0x342))
#define H0_C_T_UCH       (*(volatile u16 *)(husb0_sie_base + 0x344))

#define husb0_bridge_base (lsfr_base + map_adr(0x68, 0x00))
#define H0_SIE_CON       (*(volatile u32 *)(husb0_bridge_base + 0x00*4))
#define H0_EP0_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x01*4))
#define H0_EP1_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x02*4))
#define H0_EP2_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x03*4))
#define H0_EP3_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x04*4))
#define H0_EP4_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x05*4))
#define H0_EP5_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x06*4))
#define H0_EP6_CNT       (*(volatile u32 *)(husb0_bridge_base + 0x07*4))
#define H0_EP1_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x08*4))
#define H0_EP1_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x09*4))
#define H0_EP2_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x0a*4))
#define H0_EP2_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x0b*4))
#define H0_EP3_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x0c*4))
#define H0_EP3_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x0d*4))
#define H0_EP4_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x0e*4))
#define H0_EP4_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x0f*4))
#define H0_EP5_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x10*4))
#define H0_EP5_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x11*4))
#define H0_EP6_TADR      (*(volatile u32 *)(husb0_bridge_base + 0x12*4))
#define H0_EP6_RADR      (*(volatile u32 *)(husb0_bridge_base + 0x13*4))
#define H0_EP3_ISO_CON   (*(volatile u32 *)(husb0_bridge_base + 0x14*4))
#define H0_EP4_ISO_CON   (*(volatile u32 *)(husb0_bridge_base + 0x15*4))

#define husb_phy_base (lsfr_base + map_adr(0x6a, 0x00))
#define HUSB_COM_CON0    (*(volatile u32 *)(husb_phy_base + 0x00*4))
#define HUSB_COM_CON1    (*(volatile u32 *)(husb_phy_base + 0x01*4))
#define HUSB_COM_CON2    (*(volatile u32 *)(husb_phy_base + 0x02*4))
#define HUSB_PHY0_CON0   (*(volatile u32 *)(husb_phy_base + 0x03*4))
#define HUSB_PHY0_CON1   (*(volatile u32 *)(husb_phy_base + 0x04*4))
#define HUSB_PHY0_CON2   (*(volatile u32 *)(husb_phy_base + 0x05*4))

#endif


