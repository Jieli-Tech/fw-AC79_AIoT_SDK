/**
 * @file tuya_os_adapt_storge.c
 * @brief flash操作接口
 *
 * @copyright Copyright(C),2018-2020, 涂鸦科技 www.tuya.com
 *
 */

#include "system/includes.h"
#include "fs/fs.h"
#include "asm/sfc_norflash_api.h"

#include "tuya_os_adapt_storge.h"
#include "tuya_os_adapter_error_code.h"
#include "tuya_os_adapter.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
/*
********************************************************************************
*
*                               涂鸦数据分区说明
*
*********************************************************************************
*               *               *               *               *               *
*               *     大小      *   起始地址    *    结束地址   *     备注      *
*               *               *               *               *               *
*********************************************************************************
*               *               *               *               *               *
*    TUYA_UF    * 128K(0X3E000) *               *               *  UF_FILE区    *
*               *               *               *               *               *
*********************************************************************************
*               *               *               *               * 涂鸦应用自定义*
*    保留扇区   *  20K(0X5000)  *               *               * 数据          *
*               *               *               *               *               *
*********************************************************************************
*               *               *               *               *  tuya_sdk加密 *
*    TUYA_KEY   *  4K(0x1000)   *               *               *  存储区       *
*               *               *               *               *               *
*****************************************************************               *
*               *               *               *               *               *
*    TUYA_KV    *  64K(0x10000) *               *               *               *
*               *               *               *               *               *
*****************************************************************               *
*               *               *               *               *               *
*    TUYA_SWAP  *  32K(0x8000)  *               *               *               *
*               *               *               *               *               *
*********************************************************************************
*/

#define PARTITION_SIZE         (1 << 12) /* 4KB */
#define FLH_BLOCK_SZ            PARTITION_SIZE

/** tuya 分区划分*/
// |<-                     248K                           ->|
// | UF (128k) | 保留（20k）| KEY(4K) | KV(64K) | SWAP(32K) |
#define UF_PARTITION_SIZE 0x20000 //128k
#define TUYA_RESERVED_AREA 0x5000 //20k
#define TUYA_KEY_SIZE 0x1000 //4k
#define TUYA_KV_SIZE 0x10000 //64k
#define SIMPLE_FLASH_SWAP_SIZE 0x8000 //32K
#define TUYA_DATA_TOTAL_SIZE (UF_PARTITION_SIZE + TUYA_RESERVED_AREA + TUYA_KEY_SIZE + TUYA_KV_SIZE + SIMPLE_FLASH_SWAP_SIZE)

/** 涂鸦分区路径*/
#define TUYA_DATA_AREA_PATH "mnt/sdfile/app/exif"
/***********************************************************
*************************variable define********************
***********************************************************/
static const TUYA_OS_STORAGE_INTF m_tuya_os_storage_intfs = {
    .read  = tuya_os_adapt_flash_read,
    .write = tuya_os_adapt_flash_write,
    .erase = tuya_os_adapt_flash_erase,
    .get_storage_desc = tuya_os_adapt_storage_get_desc,
    .get_uf_desc = tuya_os_adapt_uf_get_desc,
};

static UNI_STORAGE_DESC_S storage = {0};

static UF_PARTITION_TABLE_S uf_file = {0};

/***********************************************************
*************************variable define********************
***********************************************************/

/***********************************************************
*************************function define********************
***********************************************************/
/**
 * @brief read data from flash
 *
 * @param[in]       addr        flash address
 * @param[out]      dst         pointer of buffer
 * @param[in]       size        size of buffer
 * @return int 0=成功，非0=失败
 */
int tuya_os_adapt_flash_read(const unsigned int addr, unsigned char *dst, const unsigned int size)
{
    if (NULL == dst) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    norflash_origin_read(dst, addr, size);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief write data to flash
 *
 * @param[in]       addr        flash address
 * @param[in]       src         pointer of buffer
 * @param[in]       size        size of buffer
 * @return int 0=成功，非0=失败
 */
int tuya_os_adapt_flash_write(const unsigned int addr, const unsigned char *src, const unsigned int size)
{
    if (NULL == src) {
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    norflash_write(NULL, src, size, addr);

    return OPRT_OS_ADAPTER_OK;
}

/**
 * @brief erase flash block
 *
 * @param[in]       addr        flash block address
 * @param[in]       size        size of flash block
 * @return int 0=成功，非0=失败
 */
int tuya_os_adapt_flash_erase(const unsigned int addr, const unsigned int size)
{
#define FLASH_BLOCK 4096
    norflash_open(NULL, NULL, NULL);

    int beginBlock = (addr / FLASH_BLOCK);
    int endBlock = (((addr + size) / FLASH_BLOCK));

    int writeLength = 0;
    int length = FLASH_BLOCK - (addr % FLASH_BLOCK);

    norflash_ioctl(NULL, IOCTL_SET_WRITE_PROTECT, 0);

    if (length >= size) {
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, beginBlock * FLASH_BLOCK);//每个块，写之前擦除对应块
        return OPRT_OS_ADAPTER_OK;
    } else {
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, beginBlock * FLASH_BLOCK);//每个块，写之前擦除对应块
        writeLength = length;
    }

    for (int i = beginBlock + 1; i < endBlock; i++) {
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, i * FLASH_BLOCK);
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, i * FLASH_BLOCK);
        writeLength += FLASH_BLOCK;
    }

    if (beginBlock != endBlock && (size > writeLength)) {
        norflash_ioctl(NULL, IOCTL_ERASE_SECTOR, endBlock * FLASH_BLOCK);
    }

    return OPRT_OS_ADAPTER_OK;
}


/**
 * @brief get description of storage
 *
 * @return  pointer to storage description structure
 */
UNI_STORAGE_DESC_S *tuya_os_adapt_storage_get_desc(VOID)
{
    return &storage;
}

/**
 * @brief get UF file description
 *
 * @return  pointer to descrtion of UF file
 */
UF_PARTITION_TABLE_S *tuya_os_adapt_uf_get_desc(VOID)
{
    return &uf_file;
}

/**
 * @brief get swap description
 *
 * @return  pointer to descrtion of swap file
 */
int tuya_os_adapt_legacy_swap_get_desc(unsigned int *addr, unsigned int *size)
{
    int len;
    FILE *fp = fopen(TUYA_DATA_AREA_PATH, "r");
    if (!fp) {
        printf("fopen err \n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    //获取涂鸦可用flash地址
    struct vfs_attr file_attr;
    fget_attrs(fp, &file_attr);
    unsigned int uf_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    //printf("the addr :0x%x\n", uf_addr);

    //获取涂鸦可用flash空间大小
    len = flen(fp);
    fclose(fp);

    //判断实际可用空间是否小于预设空间
    if (len < TUYA_DATA_TOTAL_SIZE) {
        printf("tuya_os_adapt_uf_get_desc : NOT ENOUGH SPACE!\n");
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    *addr = uf_addr + uf_addr + UF_PARTITION_SIZE + TUYA_RESERVED_AREA + TUYA_KEY_SIZE + TUYA_KV_SIZE;
    *size = SIMPLE_FLASH_SWAP_SIZE;

    return 0;
}

int tuya_data_area_init(void)
{
    int len;
    FILE *fp = fopen(TUYA_DATA_AREA_PATH, "r");
    if (!fp) {
        printf("fopen err \n");
        return -1;
    }

    //获取涂鸦可用flash地址
    struct vfs_attr file_attr;
    fget_attrs(fp, &file_attr);
    unsigned int uf_addr = sdfile_cpu_addr2flash_addr(file_attr.sclust);
    //printf("the addr :0x%x\n", uf_addr);

    //获取涂鸦可用flash空间大小
    len = flen(fp);
    fclose(fp);

    //判断实际可用空间是否小于预设空间
    if (len < TUYA_DATA_TOTAL_SIZE) {
        printf("tuya_os_adapt_uf_get_desc : NOT ENOUGH SPACE!\n");
        return -1;
    }

    /** UF分区初始化*/
    uf_file.sector_size = PARTITION_SIZE;
    uf_file.uf_partition_num = 1;
    uf_file.uf_partition[0].uf_partition_start_addr = uf_addr;
    uf_file.uf_partition[0].uf_partiton_flash_sz = UF_PARTITION_SIZE;  //128K

    /** storage分区初始化*/
    storage.start_addr = uf_addr + UF_PARTITION_SIZE + TUYA_RESERVED_AREA + TUYA_KEY_SIZE;
    storage.flash_sz = TUYA_KV_SIZE;
    storage.block_sz = FLH_BLOCK_SZ;
    storage.swap_start_addr = uf_addr + UF_PARTITION_SIZE + TUYA_RESERVED_AREA + TUYA_KEY_SIZE + TUYA_KV_SIZE;
    storage.swap_flash_sz = SIMPLE_FLASH_SWAP_SIZE;
    storage.key_restore_addr = uf_addr + UF_PARTITION_SIZE + TUYA_RESERVED_AREA;

    /** 调试信息*/
    printf("UF addr : 0x%x, size : 0x%x\n", uf_file.uf_partition[0].uf_partition_start_addr, uf_file.uf_partition[0].uf_partiton_flash_sz);
    printf("KV addr : 0x%x, size : 0x%x\n", storage.start_addr, storage.flash_sz);
    printf("SWAP addr : 0x%x, size : 0x%x\n", storage.swap_start_addr, storage.swap_flash_sz);
    printf("key addr : 0x%x\n", storage.key_restore_addr);

    return 0;
}

/**
 * @brief tuya_os_adapt_reg_storage_intf 接口注册
 * @return int
 */
int tuya_os_adapt_reg_storage_intf(void)
{
    if (tuya_data_area_init()) {
        printf("tuya_os_adapt_reg_storage_intf : reg error!\n");
        return OPRT_INVALID_PARM;
    }

    return tuya_os_adapt_reg_intf(INTF_STORAGE, (void *)&m_tuya_os_storage_intfs);
}
