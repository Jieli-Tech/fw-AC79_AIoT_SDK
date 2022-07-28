#include "device/device.h"
#include "asm/crc16.h"
#include "asm/sfc_norflash_api.h"

//==========================================用户flash API函数说明=====================================================//
/*norflash_open函数是初始化flash硬件，在用户使用flash的API接口首先需要打开才能使用flash API(多次打开没有影响)
参数：name、device、arg为NULL即可。
*/
int norflash_open(const char *name, struct device **device, void *arg);

/*norflash_origin_read函数是直接SPI读取flash函数，用户应该使用的API接口。
当通过norflash_write函数写，则norflash_origin_read读取与写的相同，需要加解密则应用层用户自行处理。
参数：buf:数据地址，offsetf:flash地址，len:数据长度
*/
int norflash_origin_read(u8 *buf, u32 offset, u32 len);

/*norflash_read函数是CPU读取flash，用户应使用该API接口时需要注意：读写地址小于VM区域时会做加解密处理。
参数：device为NULL，buf:数据地址，offsetf:flash地址，len:数据长度
*/
int norflash_read(struct device *device, void *buf, u32 len, u32 offset);

/*norflash_write函数是直接SPI写flash函数，用户应该使用的API接口。
当通过norflash_write函数写，则norflash_origin_read读取与写的相同，需要加解密则应用层用户自行处理。
参数：device为NULL，buf:数据地址，offsetf:flash地址，len:数据长度
*/
int norflash_write(struct device *device, void *buf, u32 len, u32 offset);

/*norflash_ioctl，flash操作命令接口
参数：device为NULL
cmd：
IOCTL_GET_CAPACITY，获取flash容量（flash实际物理容量大小），arg为容量缓冲区的地址
IOCTL_ERASE_SECTOR，擦除扇区命令，arg为擦除扇区地址
IOCTL_GET_ID，获取flash ID命令，arg为ID缓冲区的地址
IOCTL_GET_UNIQUE_ID，获取flash的UUID，arg为UUID缓冲区的地址，地址可用大小为16字节
IOCTL_SET_WRITE_PROTECT，解除flash的写保护，arg=0（默认0）
*/
int norflash_ioctl(struct device *device, u32 cmd, u32 arg);



//==================flash写地址小于MV区，norflash_read函数读取的数据需要进行先加密写进=========================//
void doe(u16 k, void *pBuf, u32 lenIn, u32 addr);//系统的加密函数
u32 get_system_enc_key(void);//系统加密秘钥
u32 get_norflash_vm_addr(void);//获取VM地址，VM长度为isd_config.ini文件配置的VM_LEN

//写地址小于MV区，数据进行加密，norflash_read读取才不出错
//当写flash不加密时，读取只能用norflash_origin_read
void flash_data_doe(u8 *buf, int len)
{
    doe(get_system_enc_key(), buf, len, 0);
}



//=======================================flash测试函数==========================================================//
void flash_test(void)
{
    void *dev = NULL;

    u8 *mac_addr_data = zalloc(4096);
    if (!mac_addr_data) {
        printf("----> err !!!!!!! malloc \n\n");
        return ;
    }
#define USE_CRC32	1 //使用CRC校验，不使用则默认和校验
    u32 cap_size = 0;
    u32 id = 0;
    u8 uuid[16] = {0};

    norflash_open(NULL, NULL, NULL);
    norflash_ioctl(NULL, IOCTL_GET_ID, (u32)&id);
    norflash_ioctl(NULL, IOCTL_GET_UNIQUE_ID, (u32)&uuid);
    norflash_ioctl(NULL, IOCTL_GET_CAPACITY, (u32)&cap_size);
    printf("--->flash id = 0x%x \n", id);
    printf("--->flash uuid \n");
    put_buf(uuid, 16);
    printf("--->flash cap_size = %dMB \n", cap_size / 1024 / 1024);

    u32 timer_get_sec(void);
    u32 start_time = timer_get_sec();
    u32 start_addr = 2 * 1024 * 1024;//起始地址
    u32 end_addr = start_addr + 2 * 1024 * 1024;//结束地址

    for (u32 addr = start_addr; addr < end_addr; addr += 4096) { //测试:start_addr - end_addr
        if (addr % (1024 * 1024) == 0) {
            printf("--->flash test addr = 0x%x \n", addr);
        }
        u32 mac_flash_addr = addr;
        u32 i, sum;
        for (i = 0, sum = 0; i < (4096 - 4); i++) {//随机数 + 和校验生成
            mac_addr_data[i] = JL_RAND->R64L & 0xFF;
            sum += mac_addr_data[i];
        }
        u32 *check = (u32 *)&mac_addr_data[4096 - 4];
#if USE_CRC32
        u32 crc32 = CRC32(mac_addr_data, (4096 - 4));//CRC校验码生成
        *check = crc32;//CRC校验
#else
        *check = sum;//和校验
#endif
        norflash_ioctl(NULL, IOCTL_SET_WRITE_PROTECT, 0);
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, mac_flash_addr);
        norflash_write(NULL, mac_addr_data, 4096, mac_flash_addr);
        memset(mac_addr_data, 0, 4096);
        norflash_origin_read(mac_addr_data, mac_flash_addr, 4096);

#if USE_CRC32
        u32 check_crc32 = CRC32(mac_addr_data, (4096 - 4));//读取数据后对数据进行CRC校验
        if (*check != crc32 || check_crc32 != crc32) {//双重校验：读校验地址的值和校验数据
            printf("---> flash check err : read crc = 0x%x , write crc = 0x%x , check_crc = 0x%x,addr=0x%x\n", *check, crc32, check_crc32, addr);
        }
#else
        u32 check_sum = 0;
        for (i = 0; i < (4096 - 4); i++) {//读取数据后对数据进行和校验
            check_sum += mac_addr_data[i];
        }
        if (*check != sum || check_sum != sum) {//双重校验：读校验地址的值和校验数据
            printf("---> flash check err : read sum = 0x%x , write sum = 0x%x , check_sum = 0x%x,addr=0x%x\n", *check, sum, check_sum, addr);
        }
#endif
    }
    u32 end_time = timer_get_sec();
    printf("--->flash test susscess , use time = %ds\n", end_time - start_time);
    free(mac_addr_data);
}

//=======================================用户flash API读写函，用户可直接使用===================================================//
//应用层用户始化flash硬件接口
void flash_read_write_init(void)
{
    u32 cap_size = 0;
    u32 flash_id = 0;
    u8 uuid[16] = {0};

    norflash_open(NULL, NULL, NULL);
    norflash_ioctl(NULL, IOCTL_GET_ID, (u32)&flash_id);//获取ID
    norflash_ioctl(NULL, IOCTL_GET_UNIQUE_ID, (u32)&uuid);//获取UUID
    norflash_ioctl(NULL, IOCTL_GET_CAPACITY, (u32)&cap_size);//获取容量
    printf("--->flash id = 0x%x , cap_size = %dMB \n", flash_id, cap_size / 1024 / 1024);
    printf("--->flash uuid \n");
    put_buf(uuid, 16);
}

//应用层用户读写API，使用该函数前，先调用norflash_open函数初始化flash硬件接口
int flash_write_buff(u8 *buff, u32 addr, u32 len)
{
#define FLASH_BLOCK 4096
    u32 beginBlock = (addr / FLASH_BLOCK);
    u32 endBlock = (((addr + len) / FLASH_BLOCK));

    u8 *flashBuffer = malloc(FLASH_BLOCK);
    if (!flashBuffer) {
        return -1;
    }
    u32 writeLength = 0;
    u32 length = FLASH_BLOCK - (addr % FLASH_BLOCK);
    norflash_ioctl(NULL, IOCTL_SET_WRITE_PROTECT, 0);
    norflash_origin_read(flashBuffer, beginBlock * FLASH_BLOCK, FLASH_BLOCK);
    if (length >= len) {
        memcpy(flashBuffer + (addr % FLASH_BLOCK), buff, len);
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, beginBlock * FLASH_BLOCK);//每个块，写之前擦除对应块
        norflash_write(NULL, flashBuffer, FLASH_BLOCK, beginBlock * FLASH_BLOCK);
        free(flashBuffer);
        return 0;
    } else {
        memcpy(flashBuffer + (addr % FLASH_BLOCK), buff, length);
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, beginBlock * FLASH_BLOCK);//每个块，写之前擦除对应块
        norflash_write(NULL, flashBuffer, FLASH_BLOCK, beginBlock * FLASH_BLOCK);
        writeLength = length;
    }
    for (int i = beginBlock + 1; i < endBlock; i++) {
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, i * FLASH_BLOCK);
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, i * FLASH_BLOCK);//每个块，写之前擦除对应块
        norflash_write(NULL, buff + writeLength, FLASH_BLOCK, i * FLASH_BLOCK);
        writeLength += FLASH_BLOCK;
    }
    if (beginBlock != endBlock && (len > writeLength)) {
        //最后一个块，剩余数据没有满一个块，不能丢失其他没用到地址的数据
        //因此最后一块先读数据，更改本次写对应地址的数据，再将没有用到地址的原有数据写回去，否则造成数据丢失
        norflash_origin_read(flashBuffer, endBlock * FLASH_BLOCK, FLASH_BLOCK);
        memcpy(flashBuffer, buff + writeLength, len - writeLength);
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, endBlock * FLASH_BLOCK);//每个块，写之前擦除对应块
        norflash_write(NULL, flashBuffer, FLASH_BLOCK, endBlock * FLASH_BLOCK);
    }
    free(flashBuffer);
    return 0;
}

//应用层用户读写API，使用该函数前，先调用norflash_open函数初始化flash硬件接口
int flash_read_buff(u8 *buff, u32 addr, u32 len)
{
    norflash_origin_read(buff, addr, len);
    return 0;
}
//=======================================用户flash API读写函，用户可直接使用===================================================//
