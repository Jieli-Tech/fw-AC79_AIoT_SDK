//*********************************************************************************//
// Module name : csfr.h                                                            //
// Description : q32DSP core sfr define                                            //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#ifndef __Q32DSP_CSFR__
#define __Q32DSP_CSFR__

#undef ___RW
#undef ___RO
#undef ___WO

#undef ___u8
#undef ___u16
#undef ___u32

#undef map_adr

#define ___RW      volatile       // read write
#define ___RO      volatile const // only read
#define ___WO      volatile       // only write


#define ___u8(x)   unsigned char(x); char(reserved_1_##x); char(reserved_2_##x); char(reserved_3_##x)
#define ___u16(x)  unsigned short(x); short(reserved_1_##x)
#define ___u32(x)  unsigned int(x)

#define ___s8(x)   char(x); char(reserved_1_##x); char(reserved_2_##x); char(reserved_3_##x)
#define ___s16(x)  short(x); short(reserved_1_##x)
#define ___s32(x)  int(x)
#define map_adr(grp, adr)  ((64 * grp + adr) * 4)   // grp(0x0-0xff), adr(0x0-0x3f)

//*********************************************************************************
//
// hcore control sfr
//
//*********************************************************************************

#define csfr_base    0x1ee0000

//............. 0x0000 - 0x00ff............
typedef struct {
    ___RO ___u32(SOFT_INT);
    ___WO ___u32(SOFT_SET);
    ___WO ___u32(SOFT_CLR);
    ___RW ___u32(CON);
} JL_CMNG_TypeDef;

#define JL_CMNG_BASE                   (csfr_base + map_adr(0x00, 0x00))
#define JL_CMNG                        ((JL_CMNG_TypeDef *)JL_CMNG_BASE)

//............. 0x0100 - 0x01ff............
typedef struct {
    ___RW ___u32(CON);
    ___RW ___u32(KEY);
} JL_SDTAP_TypeDef;

#define JL_SDTAP_BASE                  (csfr_base + map_adr(0x01, 0x00))
#define JL_SDTAP                       ((JL_SDTAP_TypeDef *)JL_SDTAP_BASE)


//............. 0x0200 - 0x02ff............
typedef struct {
    ___RW ___u32(MBISTCTL);
    ___RO ___u32(MBISTSOGO);
} JL_MBIS_TypeDef;

#define JL_MBIS_BASE                   (csfr_base + map_adr(0x02, 0x00))
#define JL_MBIS                        ((JL_MBIS_TypeDef *)JL_MBIS_BASE)

//............. 0x0300 - 0x03ff............
typedef struct {
    ___RW ___u32(CON);
    ___RW ___u32(PAGE_END);     //page number - 1
} JL_HMEM_TypeDef;

#define JL_HMEM_BASE                    (csfr_base + map_adr(0x03, 0x00))
#define JL_HMEM                         ((JL_HMEM_TypeDef *)JL_HMEM_BASE)

//............. 0x0400 - 0x04ff............
typedef struct {
    ___RW ___u32(CON);
    ___RW ___u32(CADR);
    ___RW ___u32(ACC0L);
    ___RW ___u32(ACC0H);
    ___RW ___u32(ACC1L);
    ___RW ___u32(ACC1H);
    ___RW ___u32(CONST);
    ___RW ___u32(TEST1);
} JL_FFT_TypeDef;

#define JL_FFT_BASE                    (csfr_base + map_adr(0x04, 0x00))
#define JL_FFT                         ((JL_FFT_TypeDef *)JL_FFT_BASE)




//*********************************************************************************
//
// corex2 sfr
//
//*********************************************************************************

//---------------------------------------------//
// corex2 define
//---------------------------------------------//

#define corex2_sfr_offset   0x400
#define corex2_sfr_base    (csfr_base + 0xe000)
#define corex2(n)          ((JL_TypeDef_corex2     *)(corex2_sfr_base + corex2_sfr_offset*n))

typedef struct {
    /* 00 */  ___RW ___u32(C0_CON);
    /* 01 */  ___RW ___u32(C1_CON);
    /* 02 */  ___RW ___u32(CACHE_CON);
    /* 03 */  ___RW ___u32(DCACHE_WAY);
    /* 04 */  ___RW ___u32(ICACHE_WAY);
    /* 05 */  ___RW ___u32(RING_OSC);
    /*    */  ___RO ___u32(REV_80_3[0x80 - 0x5 - 1]);

    /* 80 */  ___RW ___u32(C0_IF_UACNTL);
    /* 81 */  ___RW ___u32(C0_IF_UACNTH);
    /* 82 */  ___RW ___u32(C0_RD_UACNTL);
    /* 83 */  ___RW ___u32(C0_RD_UACNTH);
    /* 84 */  ___RW ___u32(C0_WR_UACNTL);
    /* 85 */  ___RW ___u32(C0_WR_UACNTH);
    /* 86 */  ___RW ___u32(C0_TL_CKCNTL);
    /* 87 */  ___RW ___u32(C0_TL_CKCNTH);
    /* 88 */  ___RW ___u32(C1_IF_UACNTL);
    /* 89 */  ___RW ___u32(C1_IF_UACNTH);
    /* 8a */  ___RW ___u32(C1_RD_UACNTL);
    /* 8b */  ___RW ___u32(C1_RD_UACNTH);
    /* 8c */  ___RW ___u32(C1_WR_UACNTL);
    /* 8d */  ___RW ___u32(C1_WR_UACNTH);
    /* 8e */  ___RW ___u32(C1_TL_CKCNTL);
    /* 8f */  ___RW ___u32(C1_TL_CKCNTH);

    /* 90 */  ___RW ___u32(DBG_WR_EN);
    /* 91 */  ___RO ___u32(DBG_MSG);
    /* 92 */  ___RW ___u32(DBG_MSG_CLR);
    /*    */  ___RO ___u32(REV_9c_92[0xa0 - 0x92 - 1]);

    /* a0 */  ___RW ___u32(WR_LIMIT0_H);
    /* a1 */  ___RW ___u32(WR_LIMIT1_H);
    /* a2 */  ___RW ___u32(WR_LIMIT2_H);
    /* a3 */  ___RW ___u32(WR_LIMIT3_H);
    /* a4 */  ___RW ___u32(WR_LIMIT4_H);
    /* a5 */  ___RW ___u32(WR_LIMIT5_H);
    /* a6 */  ___RW ___u32(WR_LIMIT6_H);
    /* a7 */  ___RW ___u32(WR_LIMIT7_H);
    /*    */  ___RO ___u32(REV_b0_a7[0xb0 - 0xa7 - 1]);

    /* b0 */  ___RW ___u32(WR_LIMIT0_L);
    /* b1 */  ___RW ___u32(WR_LIMIT1_L);
    /* b2 */  ___RW ___u32(WR_LIMIT2_L);
    /* b3 */  ___RW ___u32(WR_LIMIT3_L);
    /* b4 */  ___RW ___u32(WR_LIMIT4_L);
    /* b5 */  ___RW ___u32(WR_LIMIT5_L);
    /* b6 */  ___RW ___u32(WR_LIMIT6_L);
    /* b7 */  ___RW ___u32(WR_LIMIT7_L);
    /*    */  ___RO ___u32(REV_c0_b7[0xc0 - 0xb7 - 1]);

    /* c0 */  ___RW ___u32(WR_ALLOW_ID0);
    /* c1 */  ___RW ___u32(WR_ALLOW_ID1);
    /* c2 */  ___RW ___u32(WR_ALLOW_ID2);
    /* c3 */  ___RW ___u32(WR_ALLOW_ID3);
    /* c4 */  ___RW ___u32(WR_ALLOW_ID4);
    /* c5 */  ___RW ___u32(WR_ALLOW_ID5);
    /* c6 */  ___RW ___u32(WR_ALLOW_ID6);
    /* c7 */  ___RW ___u32(WR_ALLOW_ID7);
    /*    */  ___RO ___u32(REV_d0_c7[0xd0 - 0xc7 - 1]);

    /* d0 */  ___RW ___u32(DBG_EN);
    /* d1 */  ___RW ___u32(DBG_CON);
    /* d2 */  ___RW ___u32(C0_WR_LIMIT_EN);
    /* d3 */  ___RW ___u32(C1_WR_LIMIT_EN);
    /* d4 */  ___RW ___u32(PRP_WR_LIMIT_EN);
    /* d5 */  ___RW ___u32(PRP_WR_ALLOW_EN);
    /* d6 */  ___RW ___u32(PRP_OUTLIM_ERR_EN);
    /*    */  ___RO ___u32(REV_d8_d5[0xd8 - 0xd6 - 1]);

    /* d8 */  ___RO ___u32(C0_WR_LIMIT_ERR_NUM);
    /* d9 */  ___RO ___u32(C1_WR_LIMIT_ERR_NUM);
    /* da */  ___RO ___u32(PRP_WR_LIMIT_ERR_NUM);
    /* db */  ___RO ___u32(PRP_WR_LIMIT_ID);
    /* dc */  ___RO ___u32(AXI_RD_INV_ID);
    /* dd */  ___RO ___u32(AXI_WR_INV_ID);
    /* de */  ___RO ___u32(PRP_MMU_ERR_WID);
    /* df */  ___RO ___u32(PRP_MMU_ERR_RID);
    /*    */  ___RO ___u32(REV_e0_de[0xe0 - 0xdf - 1]);

    /* e0 */  ___RW ___u32(PC_LIMIT0_H);
    /* e1 */  ___RW ___u32(PC_LIMIT0_L);
    /* e2 */  ___RW ___u32(PC_LIMIT1_H);
    /* e3 */  ___RW ___u32(PC_LIMIT1_L);

} JL_TypeDef_corex2;


//*********************************************************************************
//
// q32DSP_sfr
//
//*********************************************************************************

//---------------------------------------------//
// q32DSP define
//---------------------------------------------//

#define q32DSP_sfr_offset   0x200
#define q32DSP_sfr_base    (csfr_base + 0xf000)

#define q32DSP_cpu_base    (q32DSP_sfr_base + 0x00)
#define q32DSP_mpu_base    (q32DSP_sfr_base + 0x80)

#define q32DSP(n)          ((JL_TypeDef_q32DSP     *)(q32DSP_sfr_base + q32DSP_sfr_offset*n))
#define q32DSP_mpu(n)      ((JL_TypeDef_q32DSP_MPU *)(q32DSP_mpu_base + q32DSP_sfr_offset*n))

//---------------------------------------------//
// q32DSP core sfr
//---------------------------------------------//

typedef struct {
    /* 00 */  ___RO ___u32(DR00);
    /* 01 */  ___RO ___u32(DR01);
    /* 02 */  ___RO ___u32(DR02);
    /* 03 */  ___RO ___u32(DR03);
    /* 04 */  ___RO ___u32(DR04);
    /* 05 */  ___RO ___u32(DR05);
    /* 06 */  ___RO ___u32(DR06);
    /* 07 */  ___RO ___u32(DR07);
    /* 08 */  ___RO ___u32(DR08);
    /* 09 */  ___RO ___u32(DR09);
    /* 0a */  ___RO ___u32(DR10);
    /* 0b */  ___RO ___u32(DR11);
    /* 0c */  ___RO ___u32(DR12);
    /* 0d */  ___RO ___u32(DR13);
    /* 0e */  ___RO ___u32(DR14);
    /* 0f */  ___RO ___u32(DR15);

    /* 10 */  ___RO ___u32(RETI);
    /* 11 */  ___RO ___u32(RETE);
    /* 12 */  ___RO ___u32(RETX);
    /* 13 */  ___RO ___u32(RETS);
    /* 14 */  ___RO ___u32(SR04);
    /* 15 */  ___RO ___u32(PSR);
    /* 16 */  ___RO ___u32(CNUM);
    /* 17 */  ___RO ___u32(SR07);
    /* 18 */  ___RO ___u32(SR08);
    /* 19 */  ___RO ___u32(SR09);
    /* 1a */  ___RO ___u32(SR10);
    /* 1b */  ___RO ___u32(ICFG);
    /* 1c */  ___RO ___u32(USP);
    /* 1d */  ___RO ___u32(SSP);
    /* 1e */  ___RO ___u32(SP);
    /* 1f */  ___RO ___u32(PCRS);

    /* 20 */  ___RW ___u32(BPCON);
    /* 21 */  ___RW ___u32(BSP);
    /* 22 */  ___RW ___u32(BP0);
    /* 23 */  ___RW ___u32(BP1);
    /* 24 */  ___RW ___u32(BP2);
    /* 25 */  ___RW ___u32(BP3);
    /* 26 */  ___WO ___u32(CMD_PAUSE);
    /*    */  ___RO ___u32(REV_30_26[0x30 - 0x26 - 1]);

    /* 30 */  ___RW ___u32(PMU_CON);
    /*    */  ___RO ___u32(REV_34_30[0x34 - 0x30 - 1]);
    /* 34 */  ___RW ___u32(EMU_CON);
    /* 35 */  ___RW ___u32(EMU_MSG);
    /* 36 */  ___RW ___u32(EMU_SSP_H);
    /* 37 */  ___RW ___u32(EMU_SSP_L);
    /* 38 */  ___RW ___u32(EMU_USP_H);
    /* 39 */  ___RW ___u32(EMU_USP_L);
    /*    */  ___RO ___u32(REV_3b_39[0x3b - 0x39 - 1]);
    /* 3b */  ___RW ___u8(TTMR_CON);
    /* 3c */  ___RW ___u32(TTMR_CNT);
    /* 3d */  ___RW ___u32(TTMR_PRD);
    /* 3e */  ___RW ___u32(BANK_CON);
    /* 3f */  ___RW ___u32(BANK_NUM);

    /* 40 */  ___RW ___u32(ICFG00);
    /* 41 */  ___RW ___u32(ICFG01);
    /* 42 */  ___RW ___u32(ICFG02);
    /* 43 */  ___RW ___u32(ICFG03);
    /* 44 */  ___RW ___u32(ICFG04);
    /* 45 */  ___RW ___u32(ICFG05);
    /* 46 */  ___RW ___u32(ICFG06);
    /* 47 */  ___RW ___u32(ICFG07);
    /* 48 */  ___RW ___u32(ICFG08);
    /* 49 */  ___RW ___u32(ICFG09);
    /* 4a */  ___RW ___u32(ICFG10);
    /* 4b */  ___RW ___u32(ICFG11);
    /* 4c */  ___RW ___u32(ICFG12);
    /* 4d */  ___RW ___u32(ICFG13);
    /* 4e */  ___RW ___u32(ICFG14);
    /* 4f */  ___RW ___u32(ICFG15);

    /* 50 */  ___RW ___u32(ICFG16);
    /* 51 */  ___RW ___u32(ICFG17);
    /* 52 */  ___RW ___u32(ICFG18);
    /* 53 */  ___RW ___u32(ICFG19);
    /* 54 */  ___RW ___u32(ICFG20);
    /* 55 */  ___RW ___u32(ICFG21);
    /* 56 */  ___RW ___u32(ICFG22);
    /* 57 */  ___RW ___u32(ICFG23);
    /* 58 */  ___RW ___u32(ICFG24);
    /* 59 */  ___RW ___u32(ICFG25);
    /* 5a */  ___RW ___u32(ICFG26);
    /* 5b */  ___RW ___u32(ICFG27);
    /* 5c */  ___RW ___u32(ICFG28);
    /* 5d */  ___RW ___u32(ICFG29);
    /* 5e */  ___RW ___u32(ICFG30);
    /* 5f */  ___RW ___u32(ICFG31);

    /* 60 */  ___RO ___u32(IPND0);
    /* 61 */  ___RO ___u32(IPND1);
    /* 62 */  ___RO ___u32(IPND2);
    /* 63 */  ___RO ___u32(IPND3);
    /* 64 */  ___RO ___u32(IPND4);
    /* 65 */  ___RO ___u32(IPND5);
    /* 66 */  ___RO ___u32(IPND6);
    /* 67 */  ___RO ___u32(IPND7);
    /* 68 */  ___WO ___u32(ILAT_SET);
    /* 69 */  ___WO ___u32(ILAT_CLR);
    /* 6a */  ___RW ___u32(IPMASK);
    /*    */  ___RO ___u32(REV_70_6a[0x70 - 0x6a - 1]);

    /* 70 */  ___RW ___u32(ETM_CON);
    /* 71 */  ___RO ___u32(ETM_PC0);
    /* 72 */  ___RO ___u32(ETM_PC1);
    /* 73 */  ___RO ___u32(ETM_PC2);
    /* 74 */  ___RO ___u32(ETM_PC3);
    /* 75 */  ___RW ___u32(WP0_ADRH);
    /* 76 */  ___RW ___u32(WP0_ADRL);
    /* 77 */  ___RW ___u32(WP0_DATH);
    /* 78 */  ___RW ___u32(WP0_DATL);
    /* 79 */  ___RW ___u32(WP0_PC);
} JL_TypeDef_q32DSP;

#undef ___RW
#undef ___RO
#undef ___WO

#undef ___u8
#undef ___u16
#undef ___u32

#undef map_adr

#endif

//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//

