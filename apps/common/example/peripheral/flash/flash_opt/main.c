#include "app_config.h"
#include "system/includes.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"

#ifdef USE_FLASH_OPT_TEST_DEMO

static int c_main(void)
{
    printf("norflash_otp test \n");

    int norflash_eraser_otp(void);
    int norflash_write_otp(u8 * buf, int len);
    int norflash_read_otp(u8 * buf, int len);

    u8 buf[32];
    for (int i = 0; i < 32; i++) {
        buf[i] = i;
    }
    norflash_eraser_otp();//擦除
    norflash_write_otp(buf, 32);//写，读写最大字节：768
    memset(buf, 0, 32);
    norflash_read_otp(buf, 32);//读，读写最大字节：768
    put_buf(buf, 32);
    return 0;
}
late_initcall(c_main);
#endif
