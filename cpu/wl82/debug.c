/**
 * @file debug.c
 * @brief DebugModule
 * @author chenrixin@zh-jieli.net
 * @version 0.1.00
 * @date 2016-10-27
 */
#include "system/includes.h"
#include "asm/debug.h"
#include "app_config.h"


extern void exception_irq_handler();
extern void sdtap_init(u32 ch);
extern void wdt_clear(void);
static u8 debug_index;

static const char *const debug_msg[32] = {
    /*0---7*/
    "wdt_timout_err",
    "prp_rd_mmu_err",
    "prp_wr_limit_err",
    "prp_wr_mmu_err",
    "axi_rd_inv",
    "axi_wr_inv",
    "c1_rd_mmu_expt",
    "c0_rd_mmu_expt",
    /*8---15*/
    "c1_wr_mmu_expt",
    "c0_wr_mmu_expt",
    "c1_pc_limit_err_r",
    "c1_wr_limit_err_r",
    "c0_pc_limit_err_r",
    "c0_wr_limit_err_r",
    "dmc_pnd",
    "dbg_pnd",
    /*16---23*/
    "c1_if_bus_inv",
    "c1_rd_bus_inv",
    "c1_wr_bus_inv",
    "c0_if_bus_inv",
    "c0_rd_bus_inv",
    "c0_wr_bus_inv",
    "reserved",
    "reserved",
    /*24---31*/
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
    "reserved",
};

static const char *const emu_msg[32] = {
    "reserved",     //31
    "reserved",     //30
    "reserved",     //29
    "reserved",     //28

    "reserved",     //27
    "reserved",     //26
    "reserved",     //25
    "reserved",     //24

    "reserved",     //23
    "reserved",     //22
    "reserved",     //21
    "fpu_ine_err",      //20

    "fpu_huge_err",     //19
    "fpu_tiny",     //18
    "fpu_inf_err",      //17
    "fpu_inv_err",      //16

    "reserved",     //15
    "reserved",     //14
    "reserved",     //13
    "reserved",     //12

    "reserved",     //11
    "reserved",     //10
    "reserved",     //9
    "etm_wp0_err",     //8

    "reserved",     //7
    "reserved",     //6
    "reserved",     //5
    "reserved",     //4

    "stackoverflow",     //3
    "div0_err",     //2
    "illeg_err",        //1
    "misalign_err",     //0
};

static void trace_call_stack()
{
    for (int c = 0; c < CPU_CORE_NUM; c++) {
        printf("CPU%d %x --> %x --> %x --> %x\n", c,
               q32DSP(c)->ETM_PC3, q32DSP(c)->ETM_PC2,
               q32DSP(c)->ETM_PC1, q32DSP(c)->ETM_PC0);
    }
}


void exception_analyze(int *sp)
{
    /* log_output_release_deadlock(); */ //TODO
    printf("\r\n\r\n---------------------exception error ------------------------\r\n\r\n");

    u32 cpu_id = current_cpu_id();
    if (cpu_id == 0) {
        q32DSP(1)->CMD_PAUSE = -1;
        corex2(0)->C1_CON &= ~BIT(3) ;
        corex2(0)->C1_CON |= BIT(1) ;
        printf("CPU1 run addr = 0x%x \n", q32DSP(1)->PCRS);
        printf("cpu 0 %s DEBUG_MSG = 0x%x, EMU_MSG = 0x%x C0_CON=%x\n",
               __func__, corex2(0)->DBG_MSG, q32DSP(0)->EMU_MSG, corex2(0)->C0_CON);
    } else {
        q32DSP(0)->CMD_PAUSE = -1;
        corex2(0)->C0_CON &= ~BIT(3) ;
        corex2(0)->C0_CON |= BIT(1) ;
        printf("CPU0 run addr = 0x%x \n", q32DSP(0)->PCRS);
        printf("cpu 1 %s DEBUG_MSG = 0x%x, EMU_MSG = 0x%x C1_CON=%x\n",
               __func__, corex2(0)->DBG_MSG, q32DSP(1)->EMU_MSG, corex2(0)->C1_CON);
    }

    trace_call_stack();

    unsigned int reti = sp[16];
    unsigned int rete = sp[17];
    unsigned int retx = sp[18];
    unsigned int rets = sp[19];
    unsigned int psr  = sp[20];
    unsigned int icfg = sp[21];
    unsigned int usp  = sp[22];
    unsigned int ssp  = sp[23];
    unsigned int _sp  = sp[24];

    for (int r = 0; r < 16; r++) {
        printf("R%d: %x\n", r, sp[r]);
    }

    printf("icfg: %x \n", icfg);
    printf("psr:  %x \n", psr);
    printf("rets: 0x%x \n", rets);
    printf("reti: 0x%x \n", reti);
    printf("usp : %x, ssp : %x sp: %x\r\n\r\n", usp, ssp, _sp);

    for (int i = 0; i < 32; i++) {
        if (q32DSP(cpu_id)->EMU_MSG & BIT(i)) {
            printf("%s\r\n", emu_msg[31 - i]);
            printf("current_task : %s\r\n", os_current_task());
            if (i == 3) {
                printf("usp limit %x  %x\r\n",
                       q32DSP(cpu_id)->EMU_USP_L, q32DSP(cpu_id)->EMU_USP_H);
                printf("ssp limit %x  %x\r\n",
                       q32DSP(cpu_id)->EMU_SSP_L, q32DSP(cpu_id)->EMU_SSP_H);
            }
        }
    }

    printf("\nAXI_WR_INV_ID : 0x%x, AXI_RD_INV_ID : 0x%x, PRP_WR_LIMIT_ID : 0x%x, PRP_MMU_ERR_RID : 0x%x, PRP_MMU_ERR_WID : 0x%x\n",
           corex2(0)->AXI_WR_INV_ID,
           corex2(0)->AXI_RD_INV_ID,
           corex2(0)->PRP_WR_LIMIT_ID,
           corex2(0)->PRP_MMU_ERR_RID,
           corex2(0)->PRP_MMU_ERR_WID);

    int i = 0;
    int j = 0;
    int num = 0;

    for (i = 0; i < 32; i++) {
        if (BIT(i) & corex2(0)->DBG_MSG) {
            j = 0;
            if (i == 2) {
                printf("PRP_MMU_ERR_WID %x %x\n",
                       corex2(0)->PRP_MMU_ERR_WID, corex2(0)->PRP_WR_LIMIT_ERR_NUM);
                num = corex2(0)->PRP_WR_LIMIT_ERR_NUM;
                while (!(num & BIT(j)) && ++j < 8);
                printf("\nWR_LIMITH : 0x%x, WR_LIMITL : 0x%x\n",
                       ((volatile u32 *)(&corex2(0)->WR_LIMIT0_H))[j],
                       ((volatile u32 *)(&corex2(0)->WR_LIMIT0_L))[j]);
            } else if (i == 11) {
                num = corex2(0)->C1_WR_LIMIT_ERR_NUM;
                while (!(num & BIT(j)) && ++j < 8);
                printf("\nWR_LIMITH : 0x%x, WR_LIMITL : 0x%x\n",
                       ((volatile u32 *)(&corex2(0)->WR_LIMIT0_H))[j],
                       ((volatile u32 *)(&corex2(0)->WR_LIMIT0_L))[j]);
            } else if (i == 13) {
                num = corex2(0)->C0_WR_LIMIT_ERR_NUM;
                while (!(num & BIT(j)) && ++j < 8);
                printf("\nWR_LIMITH : 0x%x, WR_LIMITL : 0x%x\n",
                       ((volatile u32 *)(&corex2(0)->WR_LIMIT0_H))[j],
                       ((volatile u32 *)(&corex2(0)->WR_LIMIT0_L))[j]);
            } else if (i == 10) {
                printf("\nPC_LIMIT0_H : 0x%x, PC_LIMIT0_L : 0x%x\n",
                       corex2(0)->PC_LIMIT0_H, corex2(0)->PC_LIMIT0_L);
                printf("\nPC_LIMIT1_H : 0x%x, PC_LIMIT1_L : 0x%x\n",
                       corex2(0)->PC_LIMIT1_H, corex2(0)->PC_LIMIT1_L);
            } else if (i == 12) {
                printf("\nPC_LIMIT0_H : 0x%x, PC_LIMIT0_L : 0x%x\n",
                       corex2(0)->PC_LIMIT0_H, corex2(0)->PC_LIMIT0_L);
                printf("\nPC_LIMIT1_H : 0x%x, PC_LIMIT1_L : 0x%x\n",
                       corex2(0)->PC_LIMIT1_H, corex2(0)->PC_LIMIT1_L);
            } else if (i == 14) {
                printf("JL_SDR->DBG_INFO : 0x%x ,err in JL_SDR->DBG_ADR = 0x%x, JL_SDR->DBG_START : 0x%x JL_SDR->DBG_END : 0x%x\n",
                       JL_SDR->DBG_INFO, JL_SDR->DBG_ADR, JL_SDR->DBG_START, JL_SDR->DBG_END);
            } else if (i == 15) {
                printf("\nAXI_RAM_LIMIT0_H : 0x%x, AXI_RAM_LIMIT0_L : 0x%x\n",
                       JL_DBG->LIMIT_H, JL_DBG->LIMIT_L);
                printf("\nJL_DBG->WR_ALLOW_ID0 : 0x%x, JL_DBG->WR_ALLOW_ID1 : 0x%x, JL_DBG->WR_LIMIT_ID : 0x%x\n",
                       JL_DBG->WR_ALLOW_ID0, JL_DBG->WR_ALLOW_ID1, JL_DBG->WR_LIMIT_ID);
            }
            printf("exception reason : %s \n", debug_msg[i]);
        }
    }
    printf("system_reset...\r\n\r\n\r\n");
    log_flush();    //只能在异常中断里调用
    wdt_clear();
#ifdef SDTAP_DEBUG
    sdtap_init(2);//防止IO被占据,重新初始化SDTAP
    while (1);
#endif
#ifndef CONFIG_FPGA_ENABLE
    cpu_reset();
#endif
}
___interrupt
static void osa_irq_handler(void)
{
    JL_OSA->CON |= BIT(6);
    puts("----------lsb clk lost-----------\n");
}


static void debug_enter_critical(void)
{
    if (corex2(0)->DBG_WR_EN & BIT(0)) {
        return;
    }
    corex2(0)->DBG_WR_EN = 0xe7;//set debug reg write enable
}

static void debug_exit_critical(void)
{
    if (corex2(0)->DBG_WR_EN & BIT(0)) {
        corex2(0)->DBG_WR_EN = 0xe7;//set debug reg write disable
    }
}

static inline int get_debug_index(void)
{
    for (u8 i = 0; i < 8 ; i++) {
        if (!(debug_index & BIT(i))) {
            return i;
        }
    }

    return -1;
}

static void pc0_rang_limit(void *low_addr, void *high_addr)
{
    printf("pc0 :%x---%x\n", (u32)low_addr, (u32)high_addr);

    corex2(0)->DBG_MSG_CLR = 0xffffffff;
    corex2(0)->PC_LIMIT0_H = (u32)high_addr;
    corex2(0)->PC_LIMIT0_L = (u32)low_addr;
}

static void pc1_rang_limit(void *low_addr, void *high_addr)
{
    printf("pc1 :%x---%x\n", (u32)low_addr, (u32)high_addr);

    corex2(0)->DBG_MSG_CLR = 0xffffffff;
    corex2(0)->PC_LIMIT1_H = (u32)high_addr;
    corex2(0)->PC_LIMIT1_L = (u32)low_addr;
}

void pc_rang_limit(void *low_addr0, void *high_addr0, void *low_addr1, void *high_addr1)
{
    debug_enter_critical();

    if (low_addr0 && high_addr0) {
        pc0_rang_limit(low_addr0, high_addr0);
    }

    if (low_addr1 && high_addr1) {
        pc1_rang_limit(low_addr1, high_addr1);
    }

    debug_exit_critical();
}

#define wr_limit_h    ((volatile u32 *)&corex2(0)->WR_LIMIT0_H)
#define wr_limit_l    ((volatile u32 *)&corex2(0)->WR_LIMIT0_L)
#define wr_allow_num  ((volatile u32 *)&corex2(0)->WR_ALLOW_ID0)

int dev_write_range_limit(int limit_index, void *low_addr, void *high_addr, u32 is_allow_write_out, int dev_id)
{
    if (limit_index == -1) {
        limit_index = get_debug_index();
    }
    /*
    	else if (debug_index & BIT(limit_index)) {
            return debug_index;
        }
    */

    if (limit_index == -1) {
        return -1;
    }

    debug_enter_critical();

    debug_index |= BIT(limit_index);

#if 1
    printf("dev %x write %s low_addr:%x--high_addr:%x\n",
           dev_id,
           is_allow_write_out ? ("allow") : ("limit"),
           (u32)low_addr,
           (u32)high_addr);
#endif

    corex2(0)->DBG_MSG_CLR = 0xffffffff;

    wr_limit_h[limit_index] = (u32)high_addr;
    wr_limit_l[limit_index] = (u32)low_addr;

    if (dev_id != -1) {
        corex2(0)->PRP_WR_ALLOW_EN |= BIT(limit_index);
        wr_allow_num[limit_index] = dev_id;
    }
    if (!is_allow_write_out) {
        corex2(0)->PRP_OUTLIM_ERR_EN |= BIT(limit_index);
    }
    corex2(0)->PRP_WR_LIMIT_EN |= BIT(limit_index);

    debug_exit_critical();

#if 1
    printf("WR_LIM%dL:%x--WR_LIM%dH:%x--corex2(0)->DBG_CON:%x--PRP_WR_LIMIT_EN:0x%x\n",
           limit_index,
           wr_limit_l[limit_index],
           limit_index,
           wr_limit_h[limit_index],
           corex2(0)->DBG_CON,
           corex2(0)->PRP_WR_LIMIT_EN);
#endif

    return 0;
}

int cpu_write_range_limit(void *low_addr, u32 win_size)
{
    void *high_addr = low_addr + win_size - 1;

    int i = get_debug_index();
    if (i == -1) {
        return -1;
    }

    debug_enter_critical();

    debug_index |= BIT(i);

    corex2(0)->DBG_MSG_CLR = 0xffffffff;

    wr_limit_h[i] = (u32)high_addr;
    wr_limit_l[i] = (u32)low_addr;

    corex2(0)->C0_WR_LIMIT_EN |= BIT(i);
    corex2(0)->C1_WR_LIMIT_EN |= BIT(i);

    debug_exit_critical();

    return 0;
}

void cpu_write_range_unlimit(void *low_addr)
{
    int i;

    debug_enter_critical();

    corex2(0)->DBG_MSG_CLR = 0xffffffff;

    for (i = 0; i < 8; i++) {
        if (wr_limit_l[i] == (u32)low_addr) {
            wr_limit_h[i] = 0;
            wr_limit_l[i] = 0;
            debug_index &= ~BIT(i);
            corex2(0)->C0_WR_LIMIT_EN &= ~BIT(i);
            corex2(0)->C1_WR_LIMIT_EN &= ~BIT(i);
        }
    }

    debug_exit_critical();
}

//使用前一定要先完全flush
int sdr_read_write_range_limit(u32 low_addr, u32 high_addr,
                               int id0, int id1,
                               u8 rd0_permit, u8 wr0_permit,
                               u8 rd1_permit, u8 wr1_permit,
                               u8 wr0_over_area_permit, u8 wr1_over_area_permit)
{
    if (id0 == -1 && id1 == -1) {
        return -1;
    }

    JL_SDR->DBG_INFO = 0;
    JL_SDR->DBG_INFO |= BIT(13);
    if (id0 != -1) {
        JL_SDR->DBG_INFO |= ((id0 >> 6) & 0xf);
        JL_SDR->DBG_INFO &= ~BIT(4);
        JL_SDR->DBG_INFO &= ~BIT(5);
        JL_SDR->DBG_INFO &= ~BIT(14);
        if (rd0_permit) {
            JL_SDR->DBG_INFO |= BIT(5);
        }
        if (wr0_permit) {
            JL_SDR->DBG_INFO |= BIT(4);
        }
        if (wr0_over_area_permit) {
            JL_SDR->DBG_INFO |= BIT(14);
        }
    }
    if (id1 != -1) {
        JL_SDR->DBG_INFO |= ((id1 >> 6) & 0xf) << 6;
        JL_SDR->DBG_INFO &= ~BIT(10);
        JL_SDR->DBG_INFO &= ~BIT(11);
        JL_SDR->DBG_INFO &= ~BIT(15);
        if (rd1_permit) {
            JL_SDR->DBG_INFO |= BIT(10);
        }
        if (wr1_permit) {
            JL_SDR->DBG_INFO |= BIT(11);
        }
        if (wr1_over_area_permit) {
            JL_SDR->DBG_INFO |= BIT(15);
        }
    }
    JL_SDR->DBG_START 	= low_addr;
    JL_SDR->DBG_END 	= high_addr;

    JL_SDR->DBG_INFO |= BIT(12);
    /* matrt id
    ISC0:0
    ISC1:1
    LSB:2
    JPG:3
    CPI0:4
    CPU1:5
    CACH:6
    DMA_COPY:7
    EMI:8
    IMD:9
     */
    return 0;
}
int sdr_read_write_range_unlimit(void)
{
    JL_SDR->DBG_INFO &= ~BIT(12);
    __asm_csync();
    return 0;
}

int axi_ram_write_range_limit(u32 low_addr, u32 high_addr, u32 is_allow_write_out, int id0, int id1)
{
#if 0
    printf("axi_ram id0 %x id1 %x write_out %s low_addr:%x--high_addr:%x\n",
           id0, id1,
           is_allow_write_out ? ("allow") : ("limit"),
           low_addr,
           high_addr);
#endif

    if (low_addr < 0x1bfe000 || low_addr >= 0x1c00000) {
        return -1;
    }

    if (high_addr < 0x1bfe000 || high_addr >= 0x1c00000 || high_addr < low_addr) {
        return -1;
    }

    JL_DBG->LIMIT_H = high_addr;
    JL_DBG->LIMIT_L = low_addr;

    if (id0 == -1 && id1 == -1) {
        JL_DBG->CON &= ~BIT(1);
    } else {
        if (id0 != -1) {
            JL_DBG->WR_ALLOW_ID0 = id0;
            JL_DBG->CON |= BIT(1);
        }
        if (id1 != -1) {
            JL_DBG->WR_ALLOW_ID1 = id1;
            JL_DBG->CON |= BIT(1);
        }
        if (!is_allow_write_out) {
            JL_DBG->CON |= BIT(2);
        }
    }

    JL_DBG->CON |= (BIT(6) | BIT(3) | BIT(0));

#if 0
    printf("WR_LIML:0x%x--WR_LIMH:0x%x-- JL_DBG->CON:0x%x \n",
           JL_DBG->LIMIT_L,
           JL_DBG->LIMIT_H,
           JL_DBG->CON);
#endif

    return 0;
}

void debug_clear(void)
{
    debug_enter_critical();

    JL_SDR->DBG_INFO = 0;
    JL_DBG->CON = 0;

    corex2(0)->DBG_CON = 0;
    corex2(0)->DBG_EN = 0;

    corex2(0)->C0_WR_LIMIT_EN = 0;
    corex2(0)->C1_WR_LIMIT_EN = 0;

    corex2(0)->PRP_WR_LIMIT_EN = 0;
    corex2(0)->PRP_WR_ALLOW_EN = 0;
    corex2(0)->PRP_OUTLIM_ERR_EN = 0;

    corex2(0)->DBG_MSG_CLR = 0xffffffff;

    q32DSP(0)->EMU_MSG = -1;
    q32DSP(1)->EMU_MSG = -1;

    debug_index = 0;

    debug_exit_critical();
}

__attribute__((noinline))
void sp_ovf_unen(void)
{
    q32DSP(OS_CPU_ID)->EMU_CON &= ~ BIT(3);
}

__attribute__((noinline))
void sp_ovf_en(void)
{
#ifndef SDTAP_DEBUG
    q32DSP(OS_CPU_ID)->EMU_CON |= BIT(3); //如果用户使用setjmp longjmp, 或者使用sdtap gdb调试 务必要删掉这句话
#endif
}

void debug_msg_clear(void)
{
    debug_enter_critical();

    JL_DBG->CON |= BIT(6);
    JL_SDR->DBG_INFO |= BIT(11);
    corex2(0)->DBG_CON |= (BIT(17) | BIT(18));
    corex2(0)->DBG_MSG_CLR = 0xffffffff;

    debug_exit_critical();
}

extern u32 cpu0_sstack_begin[];
extern u32 cpu0_sstack_end[];
extern u32 cpu1_sstack_begin[];
extern u32 cpu1_sstack_end[];

void debug_init(void)
{
    u32 cpu_id = current_cpu_id();

    request_irq(IRQ_EXCEPTION_IDX, 7, exception_irq_handler, 0xff);

    JL_OSA->CON = 0x6D;	//监控lsb_clk
    request_irq(IRQ_OSA_IDX, 7, osa_irq_handler, 0);

#ifdef SDTAP_DEBUG
    sdtap_init(2);
#endif


    q32DSP(cpu_id)->EMU_CON = 0;

    if (cpu_id == 0) {
        debug_clear();

        debug_enter_critical();
        corex2(0)->DBG_EN |= (0x3f << 16);
        corex2(0)->DBG_EN |= (0x3  << 4);
        debug_exit_critical();

        q32DSP(0)->EMU_CON &= ~ BIT(3);
        q32DSP(0)->EMU_SSP_L = (int)cpu0_sstack_begin;
        q32DSP(0)->EMU_SSP_H = (int)cpu0_sstack_end;
        q32DSP(0)->EMU_CON |= BIT(3);
        /* q32DSP(0)->EMU_USP_L = 0;//(int)cpu0_sstack_begin; */
        /* q32DSP(0)->EMU_USP_H = -1;//(int)cpu0_sstack_end;  */


        printf("cpu 0 usp limit %x  %x\r\n", q32DSP(0)->EMU_USP_L, q32DSP(0)->EMU_USP_H);
        printf("cpu 0 ssp limit %x  %x\r\n", q32DSP(0)->EMU_SSP_L, q32DSP(0)->EMU_SSP_H);
    } else {
#if !defined CONFIG_VIDEO_ENABLE && !defined CONFIG_UI_ENABLE && !defined CONFIG_RTOS_AND_MM_LIB_CODE_SECTION_IN_SDRAM
        pc_rang_limit(&text_rodata_begin, &text_rodata_end,
                      &ram_text_rodata_begin, &ram_text_rodata_end);
#endif

        q32DSP(1)->EMU_CON &= ~BIT(3);
        q32DSP(1)->EMU_SSP_L = (int)cpu1_sstack_begin;
        q32DSP(1)->EMU_SSP_H = (int)cpu1_sstack_end;
        q32DSP(1)->EMU_CON |= BIT(3);

        /* q32DSP(1)->EMU_USP_L = 0;//(int)cpu0_sstack_begin; */
        /* q32DSP(1)->EMU_USP_H = -1;//(int)cpu0_sstack_end; */

        printf("cpu 1 usp limit %x  %x\r\n", q32DSP(1)->EMU_USP_L, q32DSP(1)->EMU_USP_H);
        printf("cpu 1 ssp limit %x  %x\r\n", q32DSP(1)->EMU_SSP_L, q32DSP(1)->EMU_SSP_H);
    }

    /* q32DSP(cpu_id)->EMU_CON |= (0xf << 0) | (0x1 << 8) | (0x1f << 16); */
    /* q32DSP(cpu_id)->EMU_CON |= (0xf << 0) | (0x1 << 8) | (0x06 << 16); */

    q32DSP(cpu_id)->EMU_CON |= (BIT(2) | BIT(8));
    q32DSP(cpu_id)->ETM_CON |= BIT(0);
}
#if 0
static u64 cpu_cnt[CPU_CORE_NUM][4] ;

void cpu_effic_init(void)
{
    debug_enter_critical();
    for (u8 j = 0 ; j < CPU_CORE_NUM ; ++j) {
        if (j == 0) {
            corex2(0)->DBG_CON |= (0x7);	///< enable if_inv_en of_inv_en ex_inv_en
            ((u32 *)(&cpu_cnt[0][0]))[0] = corex2(0)->C0_IF_UACNTL ;
            ((u32 *)(&cpu_cnt[0][0]))[1] = corex2(0)->C0_IF_UACNTH ;
            ((u32 *)(&cpu_cnt[0][1]))[0] = corex2(0)->C0_RD_UACNTL ;
            ((u32 *)(&cpu_cnt[0][1]))[1] = corex2(0)->C0_RD_UACNTH ;
            ((u32 *)(&cpu_cnt[0][2]))[0] = corex2(0)->C0_WR_UACNTL ;
            ((u32 *)(&cpu_cnt[0][2]))[1] = corex2(0)->C0_WR_UACNTH ;
            ((u32 *)(&cpu_cnt[0][3]))[0] = corex2(0)->C0_TL_CKCNTL ;
            ((u32 *)(&cpu_cnt[0][3]))[1] = corex2(0)->C0_TL_CKCNTH ;
        } else {
            corex2(0)->DBG_CON |= (0x7 << 8);	///< enable if_inv_en of_inv_en ex_inv_en
            ((u32 *)(&cpu_cnt[1][0]))[0] = corex2(0)->C1_IF_UACNTL ;
            ((u32 *)(&cpu_cnt[1][0]))[1] = corex2(0)->C1_IF_UACNTH ;
            ((u32 *)(&cpu_cnt[1][1]))[0] = corex2(0)->C1_RD_UACNTL ;
            ((u32 *)(&cpu_cnt[1][1]))[1] = corex2(0)->C1_RD_UACNTH ;
            ((u32 *)(&cpu_cnt[1][2]))[0] = corex2(0)->C1_WR_UACNTL ;
            ((u32 *)(&cpu_cnt[1][2]))[1] = corex2(0)->C1_WR_UACNTH ;
            ((u32 *)(&cpu_cnt[1][3]))[0] = corex2(0)->C1_TL_CKCNTL ;
            ((u32 *)(&cpu_cnt[1][3]))[1] = corex2(0)->C1_TL_CKCNTH ;
        }
    }
    debug_exit_critical();
}

void cpu_effic_calc(void)
{
    u64 cpu_cnt_tmp[CPU_CORE_NUM][4] ;
    u64 all_cnt ;
    u64 effic ;
    int i = 0 ;
    int j ;
    static u8 init_flag = 0;

    if (!init_flag) {
        cpu_effic_init();
        init_flag = 1;
        return;		//初始化后需要隔一段时间
    }

    for (j = 0 ; j < CPU_CORE_NUM ; j++) {
        if (j == 0) {
            ((u32 *)(&cpu_cnt_tmp[0][0]))[0] = corex2(0)->C0_IF_UACNTL ;
            ((u32 *)(&cpu_cnt_tmp[0][0]))[1] = corex2(0)->C0_IF_UACNTH ;
            ((u32 *)(&cpu_cnt_tmp[0][1]))[0] = corex2(0)->C0_RD_UACNTL ;
            ((u32 *)(&cpu_cnt_tmp[0][1]))[1] = corex2(0)->C0_RD_UACNTH ;
            ((u32 *)(&cpu_cnt_tmp[0][2]))[0] = corex2(0)->C0_WR_UACNTL ;
            ((u32 *)(&cpu_cnt_tmp[0][2]))[1] = corex2(0)->C0_WR_UACNTH ;
            ((u32 *)(&cpu_cnt_tmp[0][3]))[0] = corex2(0)->C0_TL_CKCNTL ;
            ((u32 *)(&cpu_cnt_tmp[0][3]))[1] = corex2(0)->C0_TL_CKCNTH ;
        } else if (j == 1) {
            ((u32 *)(&cpu_cnt_tmp[1][0]))[0] = corex2(0)->C1_IF_UACNTL ;
            ((u32 *)(&cpu_cnt_tmp[1][0]))[1] = corex2(0)->C1_IF_UACNTH ;
            ((u32 *)(&cpu_cnt_tmp[1][1]))[0] = corex2(0)->C1_RD_UACNTL ;
            ((u32 *)(&cpu_cnt_tmp[1][1]))[1] = corex2(0)->C1_RD_UACNTH ;
            ((u32 *)(&cpu_cnt_tmp[1][2]))[0] = corex2(0)->C1_WR_UACNTL ;
            ((u32 *)(&cpu_cnt_tmp[1][2]))[1] = corex2(0)->C1_WR_UACNTH ;
            ((u32 *)(&cpu_cnt_tmp[1][3]))[0] = corex2(0)->C1_TL_CKCNTL ;
            ((u32 *)(&cpu_cnt_tmp[1][3]))[1] = corex2(0)->C1_TL_CKCNTH ;
        }
    }

    const char *CNTLX[] = {"IF_UACNT", "RD_UACNT", "WR_UACNT", "TL_CKCNT"};

    for (j = 0 ; j < CPU_CORE_NUM ; j++) {
        all_cnt = 0 ;
        u64 ddd = cpu_cnt_tmp[j][3] - cpu_cnt[j][3] ;
        /*printf("cpu[%d] TL_CKCNT = %d \r\n", j, ddd);*/

        for (i = 0 ; i < 3 ; i++) {
            /*printf("0x%08x%08x , 0x%08x%08x \r\n" ,((u32*) &cpu_cnt[j][i])[1],((u32*) &cpu_cnt[j][i])[0]  ,((u32*)&cpu_cnt_tmp[j][i])[1], ((u32*)&cpu_cnt_tmp[j][i])[0]);*/
            effic = cpu_cnt_tmp[j][i] - cpu_cnt[j][i] ;
            /*printf("sub = %08x%08x \r\n" ,((u32*)&effic)[1] ,((u32*)&effic)[0]  );*/
            all_cnt += effic ;
            /*printf("cpu[%d] %s = %d \r\n", j, CNTLX[i], effic);*/
            printf("cpu[%d] %s effic = %llu%% \r\n", j, CNTLX[i], effic * 100 / ddd) ;
        }

        /*printf("0x%08x%08x , 0x%08x%08x \r\n" ,((u32*) &cpu_cnt[j][i])[1],((u32*) &cpu_cnt[j][i])[0]  ,((u32*)&cpu_cnt_tmp[j][i])[1], ((u32*)&cpu_cnt_tmp[j][i])[0]);*/

        effic =  cpu_cnt_tmp[j][3] - cpu_cnt[j][3] ;

        /*printf("all cnt  = %08x%08x \r\n" ,((u32*)&effic)[1] ,((u32*)&effic)[0]);*/
        effic = (all_cnt * 100)   / effic   ;
        printf("cpu %d TL_CKCNT effic %d%% \r\n", j, (u32)effic) ;
        memcpy((void *)&cpu_cnt[j], (void *)&cpu_cnt_tmp[j], sizeof(u64) * 4) ;
    }
}
#endif
