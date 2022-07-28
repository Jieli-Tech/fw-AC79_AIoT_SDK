#include "system/includes.h"
#include "os/os_api.h"
#include "database.h"
#include "asm/crc16.h"
#include "http/http_cli.h"
#include "fs/fs.h"
#include "app_config.h"
#include "system/init.h"
#include "device/device.h"

extern void early_puts(char *s);

void ram0_wr_test(void)
{
    while (1) {
        for (int i = 0; i < 0x70000 / 4; i++) {
            *((volatile u32 *)(0x1c70000 - 4 * i)) = 0x12345678;
            if (*((volatile u32 *)(0x1c70000 - 4 * i)) != 0x12345678) {
                early_puts("N");
            }
        }
    }
}

static u8 dma_buf_cacheram[27 * 1024] ALIGNE(32) sec(.memp_memory_x);

static void crc16_cacheram_test(void)
{
    u16 ret;

    while (1) {
        for (int i = 0; i < sizeof(dma_buf_cacheram) - 2; i++) {
            dma_buf_cacheram[i] = (u8)JL_RAND->R64H;
        }
        ret = CRC16(dma_buf_cacheram, sizeof(dma_buf_cacheram) - 2);
        /* printf("ret1 = 0x%x\n", ret); */
        dma_buf_cacheram[sizeof(dma_buf_cacheram) - 2] = (ret >> 8 & 0x00ff);
        /* printf("ret2 = 0x%x\n", dma_buf_cacheram[sizeof(dma_buf_cacheram) - 2]); */
        dma_buf_cacheram[sizeof(dma_buf_cacheram) - 1] = (ret & 0x00ff);
        /* printf("ret3 = 0x%x\n", dma_buf_cacheram[sizeof(dma_buf_cacheram) - 1]); */
        ret = CRC16(dma_buf_cacheram, sizeof(dma_buf_cacheram));
        /* printf("ret = 0x%x\n", ret); */
        if (ret) {
            put_buf(dma_buf_cacheram, sizeof(dma_buf_cacheram));
            ASSERT(0);
        }
    }
}

___interrupt
static void aes_isr(void)
{
    if (JL_AES->CON & BIT(5)) {
        JL_AES->CON |= BIT(13);
        puts("111\n");
    }
    if (JL_AES->CON & BIT(2)) {
        JL_AES->CON |= BIT(12);
        puts("222\n");
    }
    if (JL_AES->CON & BIT(3)) {
        JL_AES->CON |= BIT(11);
        puts("333\n");
    }
}

___interrupt
static void sha_isr(void)
{
    if (JL_SHA->CON & BIT(15)) {
        JL_SHA->CON |= BIT(14);
        puts("000\n");
    }
}

void aes_sha_128_256_test(void)
{
#if 0
    JL_SHA->CON |= BIT(2);
    request_irq(IRQ_AES_IDX, 2, aes_isr, 0);
    JL_AES->CON |= (BIT(8) | BIT(9) | BIT(10));
    request_irq(IRQ_SHA_IDX, 2, sha_isr, 0);
#endif

    int mbedtls_aes_self_test(int verbose);
    int mbedtls_sha1_self_test(int verbose);
    int mbedtls_sha256_self_test(int verbose);
    ASSERT(mbedtls_aes_self_test(0) == 0);
    ASSERT(mbedtls_sha1_self_test(0) == 0);
    ASSERT(mbedtls_sha256_self_test(0) == 0);
}

void vm_write_read_test(void)
{
    static char vm_buf[252];
    u16 crc1 = 0;
    u16 crc2 = 0;

    while (1) {
        for (int i = 0; i < sizeof(vm_buf); ++i) {
            vm_buf[i] = JL_RAND->R64L;
        }
        crc1 = CRC16(vm_buf, sizeof(vm_buf));
        if (db_update_buffer(9, vm_buf, sizeof(vm_buf)) < 0) {
            puts("buffer write fail !\n");
        }
        memset(vm_buf, 0, sizeof(vm_buf));
        if (db_select_buffer(9, vm_buf, sizeof(vm_buf)) < 0) {
            puts("buffer read fail !\n");
        }
        crc2 = CRC16(vm_buf, sizeof(vm_buf));
        if (crc1 != crc2) {
            printf("crc1 : 0x%x  != crc2 : 0x%x\n", crc1, crc2);
        }
        os_time_dly(10);
    }
}


static void fs_write_test()
{
#define FILE_SIZE   20 * 1024 * 1024

    static u8 write_buf[32 * 1024 + 500];

    for (int step = 0; step < 10; step++) {

        int wlen = 1;

        FILE *f = fopen(CONFIG_ROOT_PATH"wt_001.m", "w+");

        if (!f) {
            return;
        }

        memset(write_buf, (0x5A + step) & 0xff, sizeof(write_buf));

        for (int i = 0; i < FILE_SIZE;) {
            int len = fwrite(f, write_buf, wlen);
            if (len != wlen) {
                printf("fwrite: %x, %x, %x\n", i, len, wlen);
                fclose(f);
                while (1);
                return;
            }
            i += wlen;
            if (i == FILE_SIZE) {
                break;
            }
            wlen +=  jiffies % sizeof(write_buf);
            if (wlen > sizeof(write_buf)) {
                wlen = 2;
            }
            if (i + wlen > FILE_SIZE) {
                wlen = FILE_SIZE - i;
            }
        }
        printf("write_ok: %x, %x\n", wlen, fpos(f));
        fseek(f, 0, SEEK_SET);

        int rlen = 1024;

        for (int i = 0; i < FILE_SIZE; i += rlen) {
            memset(write_buf, 0, rlen);
            int len = fread(f, write_buf, rlen);
            if (len != rlen) {
                printf("fread: %x, %x, %x\n", len, rlen, fpos(f));
                fclose(f);
                while (1);
                return;
            }
            for (int i = 0; i < rlen; i++) {
                if (write_buf[i] != ((0x5A + step) & 0xff)) {
                    printf("not eq: %x, %x, %x\n", write_buf[i], fpos(f), i);
                    fclose(f);
                    while (1);
                    return;
                }
            }
            if (i + rlen >= FILE_SIZE) {
                rlen = FILE_SIZE - i;
            }
        }

        printf("---------------------fs_write_suss: %d\n", step);

        fdelete(f);
    }
}

static void fs_write_test2(void)
{
    int ret1, ret2;
    static u8 buf[10240];

    for (int i = 0; i < 100; i++) {

        printf("\nfs_write_test num : %d    addr : 0x%lx\n", i, i * sizeof(buf));

        for (int j = 0; j < sizeof(buf); j++) {
            buf[j] = (u8)j;
        }

        FILE *f = fopen(CONFIG_ROOT_PATH"test.txt", "w+");

        fseek(f, i * sizeof(buf), SEEK_SET);

        puts("---------fwrite\n");

        ret1 = fwrite(f, buf, sizeof(buf));
#if 1
        fclose(f);

        f = fopen(CONFIG_ROOT_PATH"test.txt", "r");
#endif
        fseek(f, i * sizeof(buf), SEEK_SET);

        memset(buf, 0, sizeof(buf));

        ret2 = fread(f, buf, sizeof(buf));

        for (int j = 0; j < sizeof(buf); j++) {
            if (buf[j] != (u8)j) {
                printf("fs_write_err: 0x%lx, 0x%x, 0x%x, ret1 : %d | ret2 : %d\n", i * sizeof(buf), j, buf[j], ret1, ret2);
                fclose(f);
                put_buf(buf, sizeof(buf));
                return;
            }
        }
        fclose(f);
    }
}

static void exfat_read_test(void)
{
    u8 name[128];
    struct vfscan *fs;

    fs = fscan(CONFIG_ROOT_PATH, "-tTXT -sn");
    if (fs == NULL) {
        log_e("fscan_faild\n");
        return;
    }

    printf("fscan: file_number = %d\n", fs->file_number);;

    FILE *file = fselect(fs, FSEL_FIRST_FILE, 0);
    if (file) {
        fget_name(file, name, sizeof(name));
        printf("file_name: %s\n", name);
    }

    fclose(file);
    fscan_release(fs);
}

extern int storage_device_ready();
static void fs_test_task(void *p)
{
    OS_SEM sem;

    os_sem_create(&sem, 0);

    while (1) {
        if (!storage_device_ready()) {
            os_time_dly(10);
            continue;
        }
        fs_write_test();
        fs_write_test2();
        exfat_read_test();
        os_sem_pend(&sem, 0);
    }
}

static int fs_test_init()
{
    task_create(fs_test_task, NULL, "fs_test");
    return 0;
}
/* late_initcall(fs_test_init); */



#include "device/iic.h"

static void *iic1_dev_handel;

#define _24CXX_HARDADDR_DEAL(addr) (addr/MEM_SIZE+0)

//芯片基本信息
#define MEM_SIZE		0x0080

//芯片常量
#define DEV_24CXX_MAIN_ADDR	0xA0
#define DEV_24CXX_READ	0x01
#define DEV_24CXX_WRITE	0x00

#define _24_ADDR 0

static void _24cxx_writebyte(u32 addr, u8 data)
{
    int err = 0;
    err = dev_ioctl(iic1_dev_handel, IIC_IOCTL_TX_WITH_START_BIT, DEV_24CXX_MAIN_ADDR | (_24CXX_HARDADDR_DEAL(addr) << 1) | DEV_24CXX_WRITE);
    if (err) {
        printf("IIC_IOCTL_TX_WITH_START_BIT FIAL\n");
        return;
    }
    err = dev_ioctl(iic1_dev_handel, IIC_IOCTL_TX, addr & (MEM_SIZE - 1));
    if (err) {
        printf("IIC_IOCTL_TX FIAL\n");
        return;
    }
    err = dev_ioctl(iic1_dev_handel, IIC_IOCTL_TX_WITH_STOP_BIT, data);
    if (err) {
        printf("IIC_IOCTL_TX_WITH_STOP_BIT FIAL\n");
        return;
    }
}

static u8 _24cxx_readbyte(u32 addr)
{
    int err = 0;
    unsigned char temp;
    err = dev_ioctl(iic1_dev_handel, IIC_IOCTL_TX_WITH_START_BIT, DEV_24CXX_MAIN_ADDR | (_24CXX_HARDADDR_DEAL(addr) << 1) | DEV_24CXX_WRITE);
    if (err) {
        printf("IIC_IOCTL_TX_WITH_START_BIT FIAL\n");
        return -1;
    }
    err = dev_ioctl(iic1_dev_handel, IIC_IOCTL_TX, addr & (MEM_SIZE - 1));
    /* err = dev_ioctl(iic1_dev_handel, IIC_IOCTL_TX_WITH_STOP_BIT, addr & (MEM_SIZE - 1)); */
    if (err) {
        printf("IIC_IOCTL_TX FIAL\n");
        return -1;
    }
    err = dev_ioctl(iic1_dev_handel, IIC_IOCTL_TX_WITH_START_BIT, DEV_24CXX_MAIN_ADDR | (_24CXX_HARDADDR_DEAL(addr) << 1) | DEV_24CXX_READ);
    if (err) {
        printf("IIC_IOCTL_TX_WITH_START_BIT FIAL\n");
        return -1;
    }
    err = dev_ioctl(iic1_dev_handel, IIC_IOCTL_RX_WITH_STOP_BIT, (int)&temp);
    if (err) {
        printf("IIC_IOCTL_RX_WITH_STOP_BIT FIAL\n");
        return -1;
    }
    return temp;
}

___interrupt
static void iic_isr(void)
{
    if (JL_IIC->CON0 & BIT(13)) {
        JL_IIC->CON0 |= BIT(12);
        puts("111\n");
    }
    if (JL_IIC->CON0 & BIT(15)) {
        JL_IIC->CON0 |= BIT(14);
        puts("222\n");
    }
}

static void iic_24cxx_test(void)
{
    u8 test_buf[0x80];
    u8 res = 0;

    iic1_dev_handel = dev_open("iic1", NULL);
    if (!iic1_dev_handel) {
        printf("iic0 dev open err\n");
        ASSERT(0);
    }

#if 0
    request_irq(IRQ_IIC_IDX, 2, iic_isr, 0);
    JL_IIC->CON0 |= BIT(8);
#endif

    /* printf(" iic open ok!\n"); */

#if 0
    for (u32 i = 0; i < sizeof(test_buf); i++) {
        /* test_buf[i] = 0x20 + i; */
        test_buf[i] = JL_RAND->R64L;
        _24cxx_writebyte(i + _24_ADDR, test_buf[i]);
        delay(48000);
        res = _24cxx_readbyte(i + _24_ADDR);
        if (res != test_buf[i]) {
            /* gpio_direction_output(IO_PORTH_02, 1); */
            printf("\n_24cxx_writebyte[%d] = 0x%x \n", i, test_buf[i]);
            printf("_24CXXX_ReadByte[%d] = 0x%x\n", i, res);
            printf("test fail!\n");
            /* gpio_direction_output(IO_PORTH_02, 0); */
            /* while (1); */
        }
        delay(48000);
    }
#else
    static u8 i = 0;
    test_buf[i] = JL_RAND->R64L;
    dev_ioctl(iic1_dev_handel, IIC_IOCTL_START, 0);
    _24cxx_writebyte(i + _24_ADDR, test_buf[i]);
    dev_ioctl(iic1_dev_handel, IIC_IOCTL_STOP, 0);
    os_time_dly(1);
    dev_ioctl(iic1_dev_handel, IIC_IOCTL_START, 0);
    res = _24cxx_readbyte(i + _24_ADDR);
    dev_ioctl(iic1_dev_handel, IIC_IOCTL_STOP, 0);
    if (res != test_buf[i]) {
        /* gpio_direction_output(IO_PORTH_02, 1); */
        printf("\n_24cxx_writebyte[%d] = 0x%x \n", i, test_buf[i]);
        printf("_24CXXX_ReadByte[%d] = 0x%x\n", i, res);
        printf("test fail!\n");
        /* gpio_direction_output(IO_PORTH_02, 0); */
        while (1);
    }
    if (++i >= 128) {
        i = 0;
    }
#endif

    dev_close(iic1_dev_handel);

    iic1_dev_handel = NULL;

    /* printf("iic test end!\n"); */
}


#if 0

#include "../cpu/WL80/dvp_lcd_driver/lcd_st7735s.h"

#define LCD_SHOW_INTERVAL_TIME	1000

static void pap_lcd_test(void)
{
    volatile u16 i = 0;
    u32 cnt = 0;
    u16 color[] = {WHITE, BLACK, BLUE, BRED, GRED, GBLUE, RED, MAGENTA, GREEN, CYAN, YELLOW, BROWN, BRRED, GRAY};

    ST7735S_init();
    while (1) {
#if 1
        for (i = 0; i < sizeof(color) / sizeof(color[0]); i++) {
            printf("lcd test cnt : %d \n", ++cnt);
            ST7735S_clear_screen(color[i]);
            msleep(LCD_SHOW_INTERVAL_TIME);
        }
#endif
    }
}

#endif


/********************DMA COPY TEST*****************************/
#if defined CONFIG_FPGA_TEST_ENABLE && !defined CONFIG_VIDEO_ENABLE
#define DMA_COPY_TEST 1
#else
#define DMA_COPY_TEST 0
#endif

#if DMA_COPY_TEST && !defined CONFIG_NO_SDRAM_ENABLE

#define DMA_TEST_BUF_SIZE	(100 * 1024)

static const u8 dma_tx_buf_rom[DMA_TEST_BUF_SIZE] ALIGNE(4) = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,	31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46,	47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,	63,
    [1024 * 10] = 1, [1024 * 20] = 2, [1024 * 30] = 3, [1024 * 40] = 4, [1024 * 50] = 5,
    [1024 * 60] = 6, [1024 * 70] = 7, [1024 * 80] = 8, [1024 * 90] = 9, [1024 * 100 - 1] = 10,
};

static u8 dma_tx_buf_sdram[DMA_TEST_BUF_SIZE] ALIGNE(32);
static u8 dma_rx_buf_sdram[DMA_TEST_BUF_SIZE] ALIGNE(32);
static u8 dma_tx_buf_ram[DMA_TEST_BUF_SIZE] sec(.sram) ALIGNE(32);
static u8 dma_rx_buf_ram[DMA_TEST_BUF_SIZE] sec(.sram) ALIGNE(32);

static void dma_rom_to_sdram(void)
{
    int ret = 0;

    dma_copy(dma_rx_buf_sdram, (void *)dma_tx_buf_rom, sizeof(dma_tx_buf_rom));
    for (int i = 0; i < sizeof(dma_tx_buf_rom); ++i) {
        if (dma_rx_buf_sdram[i] != dma_tx_buf_rom[i]) {
            printf("dma_rom_to_sdram\n");
            printf("dma_tx_buf_rom = 0x%x, dma_rx_buf_sdram = 0x%x\n", dma_tx_buf_rom[i], dma_rx_buf_sdram[i]);
            ASSERT(0);
        }
    }
}

static void dma_rom_to_ram0(void)
{
    int ret = 0;

    dma_copy(dma_rx_buf_ram, (void *)dma_tx_buf_rom, sizeof(dma_tx_buf_rom));
    for (int i = 0; i < sizeof(dma_tx_buf_rom); ++i) {
        if (dma_rx_buf_ram[i] != dma_tx_buf_rom[i]) {
            printf("dma_rom_to_ram0\n");
            printf("dma_tx_buf_rom = 0x%x, dma_rx_buf_ram = 0x%x\n", dma_tx_buf_rom[i], dma_rx_buf_ram[i]);
            ASSERT(0);
        }
    }
}

static void dma_sdram_to_sdram(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(dma_tx_buf_sdram) - 2; i++) {
        dma_tx_buf_sdram[i] = (u8)JL_RAND->R64H;
    }
    ret = CRC16(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram) - 2);
    dma_tx_buf_sdram[sizeof(dma_tx_buf_sdram) - 2] = (ret >> 8 & 0x00ff);
    dma_tx_buf_sdram[sizeof(dma_tx_buf_sdram) - 1] = (ret & 0x00ff);

    memset(dma_rx_buf_sdram, 0, sizeof(dma_rx_buf_sdram));

    /* put_buf(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram)); */
    /* printf("sizeof:%d\n", sizeof(dma_tx_buf_sdram)); */
    /* puts("\n\n\n\n!!!\n\n\n"); */
    dma_copy(dma_rx_buf_sdram, dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram));
    /* put_buf(dma_rx_buf[j], sizeof(dma_rx_buf)); */
    ret = CRC16(dma_rx_buf_sdram, sizeof(dma_tx_buf_sdram));
    if (ret) {
        printf("dma_sdram_to_sdram\n");
        printf("dma_tx_buf_sdram = 0x%x, ret = 0x%x\n", (u32)dma_tx_buf_sdram, ret);
        printf("dma_rx_buf_sdram = 0x%x, ret = 0x%x\n", (u32)dma_rx_buf_sdram, ret);
        ASSERT(0);
    }
}

static void dma_ram0_to_ram0(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(dma_tx_buf_ram) - 2; i++) {
        dma_tx_buf_ram[i] = (u8)JL_RAND->R64L;
    }
    ret = CRC16(dma_tx_buf_ram, sizeof(dma_tx_buf_ram) - 2);
    dma_tx_buf_ram[sizeof(dma_tx_buf_ram) - 2] = (ret >> 8 & 0x00ff);
    dma_tx_buf_ram[sizeof(dma_tx_buf_ram) - 1] = (ret & 0x00ff);

    memset(dma_rx_buf_ram, 0, sizeof(dma_rx_buf_ram));

    /* put_buf(dma_tx_buf_ram[j], sizeof(dma_tx_buf_ram[j])); */
    /* printf("sizeof:%d\n", sizeof(dma_tx_buf_ram[0])); */
    /* puts("\n\n\n\n!!!\n\n\n"); */
    dma_copy(dma_rx_buf_ram, dma_tx_buf_ram, sizeof(dma_tx_buf_ram));
    /* put_buf(dma_rx_buf_ram[j], sizeof(dma_rx_buf_ram[j])); */
    ret = CRC16(dma_rx_buf_ram, sizeof(dma_tx_buf_ram));
    if (ret) {
        printf("dma_ram0_to_ram0 ret = 0x%x  \n", ret);
        ASSERT(0);
    }
}

static void dma_ram0_to_sdram(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(dma_tx_buf_ram) - 2; i++) {
        dma_tx_buf_ram[i] = (u8)JL_RAND->R64L;
    }
    ret = CRC16(dma_tx_buf_ram, sizeof(dma_tx_buf_ram) - 2);
    dma_tx_buf_ram[sizeof(dma_tx_buf_ram) - 2] = (ret >> 8 & 0x00ff);
    dma_tx_buf_ram[sizeof(dma_tx_buf_ram) - 1] = (ret & 0x00ff);

    memset(dma_rx_buf_sdram, 0, sizeof(dma_rx_buf_sdram));
    /* put_buf(dma_tx_buf_ram[j], sizeof(dma_tx_buf_ram[j])); */
    /* printf("sizeof:%d\n", sizeof(dma_tx_buf_ram[0])); */
    /* puts("\n\n\n\n!!!\n\n\n"); */
    dma_copy(dma_rx_buf_sdram, dma_tx_buf_ram, sizeof(dma_tx_buf_ram));
    /* put_buf(dma_rx_buf_sdram[j], sizeof(dma_rx_buf_sdram[j])); */
    ret = CRC16(dma_rx_buf_sdram, sizeof(dma_tx_buf_ram));
    if (ret) {
        printf("dma_ram0_to_sdram ret = 0x%x  \n", ret);
    }
}

static void dma_sdram_to_ram0(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(dma_tx_buf_sdram) - 2; i++) {
        dma_tx_buf_sdram[i] = (u8)JL_RAND->R64H;
    }
    ret = CRC16(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram) - 2);
    dma_tx_buf_sdram[sizeof(dma_tx_buf_sdram) - 2] = (ret >> 8 & 0x00ff);
    dma_tx_buf_sdram[sizeof(dma_tx_buf_sdram) - 1] = (ret & 0x00ff);

    memset(dma_rx_buf_ram, 0, sizeof(dma_rx_buf_ram));
    /* put_buf(dma_tx_buf_sdram[j], sizeof(dma_tx_buf_sdram[j])); */
    /* printf("sizeof:%d\n", sizeof(dma_tx_buf_sdram[0])); */
    /* puts("\n\n\n\n!!!\n\n\n"); */
    dma_copy(dma_rx_buf_ram, dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram));
    /* put_buf(dma_rx_buf_ram[j], sizeof(dma_rx_buf_ram[j])); */
    ret = CRC16(dma_rx_buf_ram, sizeof(dma_tx_buf_sdram));
    if (ret) {
        printf("dma_sdram_to_ram0 ret = 0x%x  \n", ret);
        ASSERT(0);
    }
}

static void dma_sdram_to_sdram_nocache(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(dma_tx_buf_sdram) - 2; i++) {
        dma_tx_buf_sdram[i] = (u8)JL_RAND->R64H;
    }
    ret = CRC16(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram) - 2);
    dma_tx_buf_sdram[sizeof(dma_tx_buf_sdram) - 2] = (ret >> 8 & 0x00ff);
    dma_tx_buf_sdram[sizeof(dma_tx_buf_sdram) - 1] = (ret & 0x00ff);

    memset(dma_rx_buf_sdram, 0, sizeof(dma_rx_buf_sdram));

    flush_dcache(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram));
    flushinv_dcache(dma_rx_buf_sdram, sizeof(dma_rx_buf_sdram));
    dma_copy((void *)NO_CACHE_ADDR(dma_rx_buf_sdram), (void *)NO_CACHE_ADDR(dma_tx_buf_sdram), sizeof(dma_tx_buf_sdram));

    ret = CRC16(dma_rx_buf_sdram, sizeof(dma_rx_buf_sdram));
    if (ret) {
        printf("dma_sdram_to_sdram_nocache\n");
        printf("dma_tx_buf_sdram = 0x%x, ret = 0x%x\n", (u32)dma_tx_buf_sdram, ret);
        printf("dma_rx_buf_sdram = 0x%x, ret = 0x%x\n", (u32)dma_rx_buf_sdram, ret);
        put_buf(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram));
        put_buf(dma_rx_buf_sdram, sizeof(dma_rx_buf_sdram));
        ASSERT(0);
    }
}

static void dma_ram0_to_sdram_nocache(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(dma_tx_buf_ram) - 2; i++) {
        dma_tx_buf_ram[i] = (u8)JL_RAND->R64L;
    }
    ret = CRC16(dma_tx_buf_ram, sizeof(dma_tx_buf_ram) - 2);
    dma_tx_buf_ram[sizeof(dma_tx_buf_ram) - 2] = (ret >> 8 & 0x00ff);
    dma_tx_buf_ram[sizeof(dma_tx_buf_ram) - 1] = (ret & 0x00ff);

    memset(dma_rx_buf_sdram, 0, sizeof(dma_rx_buf_sdram));

    flushinv_dcache(dma_rx_buf_sdram, sizeof(dma_rx_buf_sdram));
    dma_copy((void *)NO_CACHE_ADDR(dma_rx_buf_sdram), dma_tx_buf_ram, sizeof(dma_tx_buf_ram));

    ret = CRC16(dma_rx_buf_sdram, sizeof(dma_rx_buf_sdram));
    if (ret) {
        printf("dma_ram0_to_sdram_nocache ret = 0x%x  \n", ret);
    }
}

static void dma_sdram_to_ram0_nocache(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(dma_tx_buf_sdram) - 2; i++) {
        dma_tx_buf_sdram[i] = (u8)JL_RAND->R64H;
    }
    ret = CRC16(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram) - 2);
    dma_tx_buf_sdram[sizeof(dma_tx_buf_sdram) - 2] = (ret >> 8 & 0x00ff);
    dma_tx_buf_sdram[sizeof(dma_tx_buf_sdram) - 1] = (ret & 0x00ff);

    memset(dma_rx_buf_ram, 0, sizeof(dma_rx_buf_ram));

    flush_dcache(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram));
    dma_copy(dma_rx_buf_ram, (void *)NO_CACHE_ADDR(dma_tx_buf_sdram), sizeof(dma_tx_buf_sdram));

    ret = CRC16(dma_rx_buf_ram, sizeof(dma_rx_buf_ram));
    if (ret) {
        printf("dma_sdram_to_ram0_nocache ret = 0x%x  \n", ret);
        ASSERT(0);
    }
}

#if 1

static u8 large_dma_tx_buf_sdram[6 * 1024 * 1024] ALIGNE(32);
static u8 large_dma_rx_buf_sdram[6 * 1024 * 1024] ALIGNE(32);

static void large_dma_sdram_to_sdram(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(large_dma_tx_buf_sdram) - 2; i++) {
        large_dma_tx_buf_sdram[i] = (u8)JL_RAND->R64L;
    }
    ret = CRC16(large_dma_tx_buf_sdram, sizeof(large_dma_tx_buf_sdram) - 2);
    large_dma_tx_buf_sdram[sizeof(large_dma_tx_buf_sdram) - 2] = (ret >> 8 & 0x00ff);
    large_dma_tx_buf_sdram[sizeof(large_dma_tx_buf_sdram) - 1] = (ret & 0x00ff);

    memset(large_dma_rx_buf_sdram, 0, sizeof(large_dma_rx_buf_sdram));

    /* put_buf(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram)); */
    /* printf("sizeof:%d\n", sizeof(dma_tx_buf_sdram)); */
    /* puts("\n\n\n\n!!!\n\n\n"); */
    dma_copy(large_dma_rx_buf_sdram, large_dma_tx_buf_sdram, sizeof(large_dma_tx_buf_sdram));
    /* put_buf(dma_rx_buf[j], sizeof(dma_rx_buf)); */
    ret = CRC16(large_dma_rx_buf_sdram, sizeof(large_dma_tx_buf_sdram));
    if (ret) {
        printf("large_dma_sdram_to_sdram\n");
        printf("dma_tx_buf_sdram = 0x%x, ret = 0x%x\n", (u32)large_dma_tx_buf_sdram, ret);
        printf("dma_rx_buf_sdram = 0x%x, ret = 0x%x\n", (u32)large_dma_rx_buf_sdram, ret);
        ASSERT(0);
    }
}

static void large_dma_sdram_to_sdram_nocache(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(large_dma_tx_buf_sdram) - 2; i++) {
        large_dma_tx_buf_sdram[i] = (u8)JL_RAND->R64H;
    }
    ret = CRC16(large_dma_tx_buf_sdram, sizeof(large_dma_tx_buf_sdram) - 2);
    large_dma_tx_buf_sdram[sizeof(large_dma_tx_buf_sdram) - 2] = (ret >> 8 & 0x00ff);
    large_dma_tx_buf_sdram[sizeof(large_dma_tx_buf_sdram) - 1] = (ret & 0x00ff);

    memset(large_dma_rx_buf_sdram, 0, sizeof(large_dma_rx_buf_sdram));

    flush_dcache(large_dma_tx_buf_sdram, sizeof(large_dma_tx_buf_sdram));
    flushinv_dcache(large_dma_rx_buf_sdram, sizeof(large_dma_rx_buf_sdram));
    dma_copy((void *)NO_CACHE_ADDR(large_dma_rx_buf_sdram), (void *)NO_CACHE_ADDR(large_dma_tx_buf_sdram), sizeof(large_dma_tx_buf_sdram));

    ret = CRC16(large_dma_rx_buf_sdram, sizeof(large_dma_rx_buf_sdram));
    if (ret) {
        printf("large_dma_sdram_to_sdram_nocache\n");
        printf("dma_tx_buf_sdram = 0x%x, ret = 0x%x\n", (u32)large_dma_tx_buf_sdram, ret);
        printf("dma_rx_buf_sdram = 0x%x, ret = 0x%x\n", (u32)large_dma_rx_buf_sdram, ret);
        ASSERT(0);
    }
}

#endif


#endif

#if 0
static u8 dma_buf_cacheram[27 * 1024] ALIGNE(32) sec(.memp_memory_x);
static u8 dma_buf_ram[27 * 1024] ALIGNE(32) sec(.sram);

static void dma_cacheram_to_ram(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(dma_buf_cacheram) - 2; i++) {
        dma_buf_cacheram[i] = (u8)JL_RAND->R64L;
    }
    ret = CRC16(dma_buf_cacheram, sizeof(dma_buf_cacheram) - 2);
    dma_buf_cacheram[sizeof(dma_buf_cacheram) - 2] = (ret >> 8 & 0x00ff);
    dma_buf_cacheram[sizeof(dma_buf_cacheram) - 1] = (ret & 0x00ff);

    memset(dma_buf_ram, 0, sizeof(dma_buf_ram));

    /* put_buf(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram)); */
    /* printf("sizeof:%d\n", sizeof(dma_tx_buf_sdram)); */
    /* puts("\n\n\n\n!!!\n\n\n"); */
    dma_copy(dma_buf_ram, dma_buf_cacheram, sizeof(dma_buf_cacheram));
    /* put_buf(dma_rx_buf[j], sizeof(dma_rx_buf)); */
    ret = CRC16(dma_buf_ram, sizeof(dma_buf_cacheram));
    if (ret) {
        printf("dma_cacheram_to_ram\n");
        printf("dma_buf_cacheram = 0x%x, ret = 0x%x\n", (u32)dma_buf_cacheram, ret);
        printf("dma_buf_ram = 0x%x, ret = 0x%x\n", (u32)dma_buf_ram, ret);
        ASSERT(0);
    }
}

static void dma_ram_to_cacheram(void)
{
    int ret = 0;

    for (int i = 0; i < sizeof(dma_buf_ram) - 2; i++) {
        dma_buf_ram[i] = (u8)JL_RAND->R64H;
    }
    ret = CRC16(dma_buf_ram, sizeof(dma_buf_ram) - 2);
    dma_buf_ram[sizeof(dma_buf_ram) - 2] = (ret >> 8 & 0x00ff);
    dma_buf_ram[sizeof(dma_buf_ram) - 1] = (ret & 0x00ff);

    memset(dma_buf_cacheram, 0, sizeof(dma_buf_cacheram));

    /* put_buf(dma_tx_buf_sdram, sizeof(dma_tx_buf_sdram)); */
    /* printf("sizeof:%d\n", sizeof(dma_tx_buf_sdram)); */
    /* puts("\n\n\n\n!!!\n\n\n"); */
    dma_copy(dma_buf_cacheram, dma_buf_ram, sizeof(dma_buf_ram));
    /* put_buf(dma_rx_buf[j], sizeof(dma_rx_buf)); */
    ret = CRC16(dma_buf_cacheram, sizeof(dma_buf_ram));
    if (ret) {
        printf("dma_cacheram_to_ram\n");
        printf("dma_buf_ram = 0x%x, ret = 0x%x\n", (u32)dma_buf_ram, ret);
        printf("dma_buf_cacheram = 0x%x, ret = 0x%x\n", (u32)dma_buf_cacheram, ret);
        ASSERT(0);
    }
}

#endif


/***********************************************************/

sec(.vm) volatile u32 debug_error_cnt = 0;
sec(.vm) volatile u32 debug_wait_flag = 0;

/********************DEBUG MSG TEST*****************************/
sec(.vm) static void debug_test_write(u32 addr, u32 len)
{
    printf("test write addr 0x%x ---- 0x%x   len : 0x%x  \n", addr, addr + len, len);
    if (addr == 0x4000000) {
        corex2(0)->CACHE_CON &= ~(1 << 9); //disable sdram cache
    }
    for (u32 i = 0; i < len; ++i) {
        *(volatile u8 *)(addr + i) = 0x5e;
        while (!debug_wait_flag);
        debug_wait_flag = 0;
    }
    if (addr == 0x4000000) {
        corex2(0)->CACHE_CON |= (1 << 9); //enable sdram cache
    }
    printf("error cnt = 0x%x\n", debug_error_cnt);
    debug_error_cnt = 0;
}

sec(.vm) static void debug_test_read(u32 addr, u32 len)
{
    printf("test read addr 0x%x ---- 0x%x   len : 0x%x  \n", addr, addr + len, len);
    if (addr == 0x4000000) {
        corex2(0)->CACHE_CON &= ~(1 << 9); //disable rw cache
    }
    if (addr == 0x2000000) {
        corex2(0)->CACHE_CON &= ~(1 << 8); //disable ro cache
    }
    for (u32 i = 0; i < len; ++i) {
        volatile u8 data = *(volatile u8 *)(addr + i);
        while (!debug_wait_flag);
        debug_wait_flag = 0;
    }
    if (addr == 0x4000000) {
        corex2(0)->CACHE_CON |= (1 << 9); //enable rw cache
    }
    if (addr == 0x2000000) {
        corex2(0)->CACHE_CON |= (1 << 8); //enable ro cache
    }
    printf("error cnt = 0x%x\n", debug_error_cnt);
    debug_error_cnt = 0;
}

sec(.vm) static void debug_test_write_limit(u32 addr, u32 len)
{
    u8 volatile data;
    printf("test write addr 0x%x ---- 0x%x   len : 0x%x  \n", addr, addr + len, len);
    for (u32 i = 0; i < len; ++i) {
        data = *(volatile u8 *)(addr + i);
        *(volatile u8 *)(addr + i) = data;
        while (!debug_wait_flag);
        debug_wait_flag = 0;
    }
    printf("error cnt = 0x%x\n", debug_error_cnt);
    debug_error_cnt = 0;
}

sec(.vm) static void debug_test_axi_wr_inv(u32 addr, u32 len)
{
    printf("test write addr 0x%x ---- 0x%x   len : 0x%x  \n", addr, addr + len, len / 4);
    JL_PLNK0->LEN = 2;
    if (addr == 0x4000000) {
        while (!(corex2(0)->CACHE_CON & BIT(14)));
        corex2(0)->CACHE_CON &= ~(1 << 9); //disable sdram cache
        while (!(corex2(0)->CACHE_CON & BIT(14)));
    }
    for (u32 i = 0; i < len; i += 4) {
        JL_PLNK0->ADR = addr + i;
        JL_PLNK0->CON = BIT(14) | BIT(0);
        while (!debug_wait_flag);
        debug_wait_flag = 0;
    }
    if (addr == 0x4000000) {
        corex2(0)->CACHE_CON |= (1 << 9); //enable sdram cache
    }
    printf("error cnt = 0x%x\n", debug_error_cnt);
    debug_error_cnt = 0;
}

sec(.vm) static void debug_test_axi_rd_inv(u32 addr, u32 len)
{
    printf("test read addr 0x%x ---- 0x%x   len : 0x%x  \n", addr, addr + len, len / 4);
    if (addr == 0x4000000) {
        while (!(corex2(0)->CACHE_CON & BIT(14)));
        corex2(0)->CACHE_CON &= ~(1 << 9); //disable rw cache
    }
    if (addr == 0x2000000) {
        while (!(corex2(0)->CACHE_CON & BIT(14)));
        corex2(0)->CACHE_CON &= ~(1 << 8); //disable ro cache
    }
    JL_ALNK0->LEN = 2;
    JL_ALNK0->CON1 = BIT(0);
    JL_ALNK0->CON3 = 0x23;
    for (u32 i = 0; i < len; i += 4) {
        JL_ALNK0->ADR0 = addr + i;
        JL_ALNK0->CON0 = BIT(11);
        while (!debug_wait_flag);
        JL_ALNK0->CON2 = 0xf;
        debug_wait_flag = 0;
    }
    if (addr == 0x4000000) {
        corex2(0)->CACHE_CON |= (1 << 9); //enable rw cache
    }
    if (addr == 0x2000000) {
        corex2(0)->CACHE_CON |= (1 << 8); //enable ro cache
    }
    printf("error cnt = 0x%x\n", debug_error_cnt);
    debug_error_cnt = 0;
}

void debug_msg_test(void)
{
    //测试CPU1需要将CPU0 HOLD住
#if 0
    //c0_core_cache_winv and c1_core_cache_winv
    debug_test_write(0x1c80000, 0x1d00000 - 0x1c80000);
    debug_test_write(0x1d80000, 0x1e00000 - 0x1d80000);
    debug_test_write(0x1e02800, 0x1edc000 - 0x1e02800);
    debug_test_write(0x1f04000, 0x1f08000 - 0x1f04000);
    debug_test_write(0x1f0a200, 0x1f0b000 - 0x1f0a200);
    debug_test_write(0x1f0b200, 0x1f20000 - 0x1f0b200);
    debug_test_write(0x1f20000, 0x1f28000 - 0x1f20000);
    debug_test_write(0x1f28000, 0x1f30000 - 0x1f28000);
    debug_test_write(0x1f30000, 0x2000000 - 0x1f30000);
    debug_test_write(0x2000000, 0x4000000 - 0x2000000);
    debug_test_write(0x4000000, 0x8000000 - 0x4000000);

    //c0_core_cache_rinv and c1_core_cache_rinv
    debug_test_read(0x1c80000, 0x1d00000 - 0x1c80000);
    debug_test_read(0x1d80000, 0x1e00000 - 0x1d80000);
    debug_test_read(0x1e02800, 0x1edc000 - 0x1e02800);
    debug_test_read(0x1f04000, 0x1f08000 - 0x1f04000);
    debug_test_read(0x1f0a200, 0x1f0b000 - 0x1f0a200);
    debug_test_read(0x1f0b200, 0x1f20000 - 0x1f0b200);
    debug_test_read(0x1f20000, 0x1f28000 - 0x1f20000);
    debug_test_read(0x1f28000, 0x1f30000 - 0x1f28000);
    debug_test_read(0x1f30000, 0x2000000 - 0x1f30000);
    debug_test_read(0x2000000, 0x4000000 - 0x2000000);
    debug_test_read(0x4000000, 0x8000000 - 0x4000000);

    //cpu axi_wr_inv
    debug_test_write(0, 		0x10000 - 0);
    debug_test_write(0x60000, 	0x400000 - 0x60000);
    debug_test_write(0x400000, 	0x1bf0000 - 0x400000);

    //cpu axi_rd_inv
    debug_test_read(0, 			0x10000 - 0);
    debug_test_read(0x60000, 	0x400000 - 0x60000);
    debug_test_read(0x400000, 	0x1bf0000 - 0x400000);
#endif

#if 0
    //测试时debug_index需要放在sec(.vm)
    cpu_write_range_limit((void *)0x1c00310, 0x1c7fe00 - 0x1c00310);
    cpu_write_range_limit((void *)0x4000000, 0x4800000 - 0x4000000);
    cpu_write_range_limit((void *)0x4800000, 0x5000000 - 0x4800000);
    cpu_write_range_limit((void *)0x5000000, 0x5800000 - 0x5000000);
    cpu_write_range_limit((void *)0x5800000, 0x6000000 - 0x5800000);
    cpu_write_range_limit((void *)0x6000000, 0x6800000 - 0x6000000);
    cpu_write_range_limit((void *)0x6800000, 0x7000000 - 0x6800000);
    cpu_write_range_limit((void *)0x7000000, 0x8000000 - 0x7000000);

    debug_test_write_limit(0x1c00310, 0x1c7fe00 - 0x1c00310);
    debug_test_write_limit(0x4000000, 0x4800000 - 0x4000000);
    debug_test_write_limit(0x4800000, 0x5000000 - 0x4800000);
    debug_test_write_limit(0x5000000, 0x5800000 - 0x5000000);
    debug_test_write_limit(0x5800000, 0x6000000 - 0x5800000);
    debug_test_write_limit(0x6000000, 0x6800000 - 0x6000000);
    debug_test_write_limit(0x6800000, 0x7000000 - 0x6800000);
    debug_test_write_limit(0x7000000, 0x8000000 - 0x7000000);
#endif

#if 0
    debug_test_axi_wr_inv(0, 		 0x10000   - 0);
    debug_test_axi_wr_inv(0x60000, 	 0x400000  - 0x60000);
    debug_test_axi_wr_inv(0x400000,	 0x1bf0000 - 0x400000);
    debug_test_axi_wr_inv(0x1c80000, 0x1d00000 - 0x1c80000);
    debug_test_axi_wr_inv(0x1d80000, 0x1e00000 - 0x1d80000);
    debug_test_axi_wr_inv(0x1e02800, 0x1edc000 - 0x1e02800);
    debug_test_axi_wr_inv(0x1f04000, 0x1f08000 - 0x1f04000);
    debug_test_axi_wr_inv(0x1f0a200, 0x1f0b000 - 0x1f0a200);
    debug_test_axi_wr_inv(0x1f0b200, 0x1f20000 - 0x1f0b200);
    debug_test_axi_wr_inv(0x1f20000, 0x1f28000 - 0x1f20000);
    debug_test_axi_wr_inv(0x1f28000, 0x1f30000 - 0x1f28000);
    debug_test_axi_wr_inv(0x1f30000, 0x2000000 - 0x1f30000);
    debug_test_axi_wr_inv(0x2000000, 0x4000000 - 0x2000000);
    debug_test_axi_wr_inv(0x4000000, 0x8000000 - 0x4000000);
#endif

#if 0
    debug_test_axi_rd_inv(0, 		 0x10000   - 0);
    debug_test_axi_rd_inv(0x60000, 	 0x400000  - 0x60000);
    debug_test_axi_rd_inv(0x400000,	 0x1bf0000 - 0x400000);
    debug_test_axi_rd_inv(0x1c80000, 0x1d00000 - 0x1c80000);
    debug_test_axi_rd_inv(0x1d80000, 0x1e00000 - 0x1d80000);
    debug_test_axi_rd_inv(0x1e02800, 0x1edc000 - 0x1e02800);
    debug_test_axi_rd_inv(0x1f04000, 0x1f08000 - 0x1f04000);
    debug_test_axi_rd_inv(0x1f0a200, 0x1f0b000 - 0x1f0a200);
    debug_test_axi_rd_inv(0x1f0b200, 0x1f20000 - 0x1f0b200);
    debug_test_axi_rd_inv(0x1f20000, 0x1f28000 - 0x1f20000);
    debug_test_axi_rd_inv(0x1f28000, 0x1f30000 - 0x1f28000);
    debug_test_axi_rd_inv(0x1f30000, 0x1fffff8 - 0x1f30000);
    debug_test_axi_rd_inv(0x2000000, 0x4000000 - 0x2000000);
    debug_test_axi_rd_inv(0x4000000, 0x8000000 - 0x4000000);
#endif

#if 0
    dev_write_range_limit(0, (void *)0x1c04b10, (void *)0x1c7fe00, 1, -1);
    dev_write_range_limit(1, (void *)0x4000000, (void *)0x4800000, 1, -1);
    dev_write_range_limit(2, (void *)0x4800000, (void *)0x5000000, 1, -1);
    dev_write_range_limit(3, (void *)0x5000000, (void *)0x5800000, 1, -1);
    dev_write_range_limit(4, (void *)0x5800000, (void *)0x6000000, 1, -1);
    dev_write_range_limit(5, (void *)0x6000000, (void *)0x6800000, 1, -1);
    dev_write_range_limit(6, (void *)0x6800000, (void *)0x7000000, 1, -1);
    dev_write_range_limit(7, (void *)0x7000000, (void *)0x8000000, 1, -1);

    //要小心外设改动stack和vm段
    debug_test_axi_wr_inv(0x1c04b10, 0x1c7fcf0 - 0x1c04b10);
    debug_test_axi_wr_inv(0x4000000, 0x4800000 - 0x4000000);
#if 0
    debug_test_axi_wr_inv(0x4800000, 0x5000000 - 0x4800000);
    debug_test_axi_wr_inv(0x5000000, 0x5800000 - 0x5000000);
    debug_test_axi_wr_inv(0x5800000, 0x6000000 - 0x5800000);
    debug_test_axi_wr_inv(0x6000000, 0x6800000 - 0x6000000);
    debug_test_axi_wr_inv(0x6800000, 0x7000000 - 0x6800000);
    debug_test_axi_wr_inv(0x7000000, 0x8000000 - 0x7000000);
#endif
#endif

#if 0
    dev_write_range_limit(0, (void *)0x1c04b10, (void *)0x1c7fe00, 1, 0x53);
    dev_write_range_limit(1, (void *)0x4000000, (void *)0x4800000, 1, 0x53);
    dev_write_range_limit(2, (void *)0x4800000, (void *)0x5000000, 1, 0x53);
    dev_write_range_limit(3, (void *)0x5000000, (void *)0x5800000, 1, 0x53);
    dev_write_range_limit(4, (void *)0x5800000, (void *)0x6000000, 1, 0x53);
    dev_write_range_limit(5, (void *)0x6000000, (void *)0x6800000, 1, 0x53);
    dev_write_range_limit(6, (void *)0x6800000, (void *)0x7000000, 1, 0x53);
    dev_write_range_limit(7, (void *)0x7000000, (void *)0x8000000, 1, 0x53);

    //要小心外设改动stack和vm段
    debug_test_axi_wr_inv(0x1c04b10, 0x1c7fcf0 - 0x1c04b10);
    debug_test_axi_wr_inv(0x4000000, 0x4800000 - 0x4000000);
#if 0
    debug_test_axi_wr_inv(0x4800000, 0x5000000 - 0x4800000);
    debug_test_axi_wr_inv(0x5000000, 0x5800000 - 0x5000000);
    debug_test_axi_wr_inv(0x5800000, 0x6000000 - 0x5800000);
    debug_test_axi_wr_inv(0x6000000, 0x6800000 - 0x6000000);
    debug_test_axi_wr_inv(0x6800000, 0x7000000 - 0x6800000);
    debug_test_axi_wr_inv(0x7000000, 0x8000000 - 0x7000000);
#endif
#endif

#if 0
    dev_write_range_limit(0, (void *)0x1c04b10, (void *)0x1c7fe00, 1, 0x51);
    dev_write_range_limit(1, (void *)0x4000000, (void *)0x4800000, 1, 0x51);
    dev_write_range_limit(2, (void *)0x4800000, (void *)0x5000000, 1, 0x51);
    dev_write_range_limit(3, (void *)0x5000000, (void *)0x5800000, 1, 0x51);
    dev_write_range_limit(4, (void *)0x5800000, (void *)0x6000000, 1, 0x51);
    dev_write_range_limit(5, (void *)0x6000000, (void *)0x6800000, 1, 0x51);
    dev_write_range_limit(6, (void *)0x6800000, (void *)0x7000000, 1, 0x51);
    dev_write_range_limit(7, (void *)0x7000000, (void *)0x8000000, 1, 0x51);

    //要小心外设改动stack和vm段
    debug_test_axi_wr_inv(0x1c04b10, 0x1c7fcf0 - 0x1c04b10);
    debug_test_axi_wr_inv(0x4000000, 0x4800000 - 0x4000000);
#if 0
    debug_test_axi_wr_inv(0x4800000, 0x5000000 - 0x4800000);
    debug_test_axi_wr_inv(0x5000000, 0x5800000 - 0x5000000);
    debug_test_axi_wr_inv(0x5800000, 0x6000000 - 0x5800000);
    debug_test_axi_wr_inv(0x6000000, 0x6800000 - 0x6000000);
    debug_test_axi_wr_inv(0x6800000, 0x7000000 - 0x6800000);
    debug_test_axi_wr_inv(0x7000000, 0x8000000 - 0x7000000);
#endif
#endif

#if 0
    dev_write_range_limit(0, (void *)0x1c04b10, (void *)0x1c7fe00, 0, 0x51);
    dev_write_range_limit(1, (void *)0x4000000, (void *)0x4800000, 0, 0x51);
    dev_write_range_limit(2, (void *)0x4800000, (void *)0x5000000, 0, 0x51);
    dev_write_range_limit(3, (void *)0x5000000, (void *)0x5800000, 0, 0x51);
    dev_write_range_limit(4, (void *)0x5800000, (void *)0x6000000, 0, 0x51);
    dev_write_range_limit(5, (void *)0x6000000, (void *)0x6800000, 0, 0x51);
    dev_write_range_limit(6, (void *)0x6800000, (void *)0x7000000, 0, 0x51);
    dev_write_range_limit(7, (void *)0x7000000, (void *)0x8000000, 0, 0x51);

    //要小心外设改动stack和vm段
    debug_test_axi_wr_inv(0x1c04b10, 0x1c7fcf0 - 0x1c04b10);
    debug_test_axi_wr_inv(0x4000000, 0x4800000 - 0x4000000);
#if 0
    debug_test_axi_wr_inv(0x4800000, 0x5000000 - 0x4800000);
    debug_test_axi_wr_inv(0x5000000, 0x5800000 - 0x5000000);
    debug_test_axi_wr_inv(0x5800000, 0x6000000 - 0x5800000);
    debug_test_axi_wr_inv(0x6000000, 0x6800000 - 0x6000000);
    debug_test_axi_wr_inv(0x6800000, 0x7000000 - 0x6800000);
    debug_test_axi_wr_inv(0x7000000, 0x8000000 - 0x7000000);
#endif
#endif
}
/***************************************************************/




static const int uart_table[] = {
    1200, 2400, 4800, 9600, 14400, 19200, 38400, 43000, 57600, 76800, 115200, 128000, 230400, 256000, 460800, 921600, 1382400
};

static struct uart_platform_data uart_data = {
    .baudrate = 921600,
    .tx_pin = IO_PORTA_03,
    .rx_pin = IO_PORTA_04,
    .flags = UART_DEBUG,
    .irq = IRQ_UART0_IDX,
};

extern int uart_init(const struct uart_platform_data *data);
static void uart_baudrate_test(void)
{
    static u8 num = 0;
    u8 cnt = 0;

    uart_init((const struct uart_platform_data *)&uart_data);

    while (1) {
        if (cnt == 30) {
            cnt = 0;
            printf("The next baudrate is %d, remember change the baudrate of computer!!!\n", uart_table[num]);
            uart_data.baudrate = uart_table[num];
            uart_init((const struct uart_platform_data *)&uart_data);
            if (++num >= ARRAY_SIZE(uart_table)) {
                num = 0;
            }
            os_time_dly(500);
        }
        cnt++;
        printf("cnt:%d, cnt_addr:0x%x\n", cnt, (u32)&cnt);
    }
}

static void wl80_test_task(void *p)
{
    u32 cnt = 0;

    os_time_dly(300);

    while (1) {
        printf("cnt:%d\n", cnt++);

        os_time_dly(1);

        iic_24cxx_test();
#ifdef CONFIG_NET_ENABLE
        aes_sha_128_256_test();
#endif
        if (cnt % 30 == 0) {
            extern void pwm_led_test(void);
            pwm_led_test();
        }
#if DMA_COPY_TEST && !defined CONFIG_NO_SDRAM_ENABLE
        dma_rom_to_sdram();
        dma_rom_to_ram0();
        dma_sdram_to_sdram();
        dma_ram0_to_ram0();
        dma_ram0_to_sdram();
        dma_sdram_to_ram0();
        dma_sdram_to_ram0_nocache();
        dma_sdram_to_sdram_nocache();
        dma_ram0_to_sdram_nocache();
        large_dma_sdram_to_sdram_nocache();
        large_dma_sdram_to_sdram();
#else
        os_time_dly(100);
#endif
    }
}

static int wl80_test_init()
{
    return thread_fork("wl80_test", 0, 2048, 0, 0, wl80_test_task, NULL);
}
#ifdef CONFIG_FPGA_TEST_ENABLE
late_initcall(wl80_test_init);
#endif

