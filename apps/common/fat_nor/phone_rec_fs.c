/***********************************Jieli tech************************************************
  File : phone_rec_fs.c
  By   : phewlee
  Email: lifuyong@zh-jieli.com
  date : 2020-6-4
********************************************************************************************/
#include "spi/nor_fs.h"
#include "os/os_api.h"
#include "asm/crc16.h"
/* #include <string.h> */
#include "stdlib.h"
#include "fs/fs.h"
/* #include "jlfat/ff_opr.h" */
#include "system/includes.h"
#include "fs/sdfile.h"
#include "boot.h"


//文件头
/* typedef struct __PHONE_RECFILEHEAD { */
/*     u16 crc; */
/*     u8 head_len; */
/*     u8 reserv[5]; */
/*     u8 priv_data[16]; */
/*     u32 file_len; */
/*     u32 index; */
/* } PHONE_RECFILEHEAD, *PPHONE_RECFILEHEAD __attribute__((aligned(4))); */

#define PHONE_RECFILEHEAD  SDFILE_FILE_HEAD

int phone_offset_addr = 0;
int max_rec_len = 0;
static int sdfile_rec_index = 0;
static u8 flash_page_mode_flag = 0;



#define NO_DEBUG_EN

#ifdef NO_DEBUG_EN
#undef r_printf
#undef y_printf
#define r_printf(...)
#define y_printf(...)
#endif

typedef struct U16BIT_IN_PHONEFS {
    u8 b0 : 1;
    u8 b1 : 1;
    u8 b2 : 1;
    u8 b3 : 1;
    u8 b4 : 1;
    u8 b5 : 1;
    u8 b6 : 1;
    u8 b7 : 1;
    u8 b8 : 1;
    u8 b9 : 1;
    u8 b10 : 1;
    u8 b11 : 1;
    u8 b12 : 1;
    u8 b13 : 1;
    u8 b14 : 1;
    u8 b15 : 1;
} U16BIT_IN_PHONEFS;

typedef union UBIT16_IN_PHONEFS {
    U16BIT_IN_PHONEFS bits;
    u16 val;
} UBIT16_IN_PHONEFS;

static UBIT16_IN_PHONEFS cd_phone;

static u16 do16_in_phonefs(void)
{
    UBIT16_IN_PHONEFS ufcc, urp;
    u8 f_xdat;
    u8 f_datin;
    ufcc.bits = urp.bits = cd_phone.bits;
    f_datin = 0x10;
    f_xdat = f_datin ^ urp.bits.b15;

    ufcc.val = urp.val << 1;
    ufcc.bits.b12 = urp.bits.b11 ^ f_xdat;
    ufcc.bits.b5 = urp.bits.b4 ^ f_xdat;
    ufcc.bits.b0 = f_xdat;

    urp.bits = ufcc.bits;
    cd_phone.bits = ufcc.bits;
    return ufcc.val & 0x00ff;
}

static void doe_in_phonefs(u16 k, void *pBuf, u32 lenIn, u32 addr)
{
    u8 *p = (u8 *)pBuf;

    if (addr) {
        k = (addr >> 2) ^ k;
    }
    cd_phone.val = k;
    *p++ ^= k;
    while (--lenIn) {
        *p++ ^= do16_in_phonefs();
    }
}



u8 phonefs_mutex_dis = 0;
OS_MUTEX phonefs_mutex;             ///<互斥信号量
void phonefs_mutex_init(void)       ///<信号量初始化
{
    if (phonefs_mutex_dis) {
        return;
    }
    static u8 init = 0;
    if (init) {
        return;
    }
    init = 1;
    os_mutex_create(&phonefs_mutex);
}
void phonefs_mutex_enter(void)      ///<申请信号量
{
    if (phonefs_mutex_dis) {
        return;
    }
    phonefs_mutex_init();
    os_mutex_pend(&phonefs_mutex, 0);
}
void phonefs_mutex_exit(void)       ///<释放信号量
{
    if (phonefs_mutex_dis) {
        return;
    }
    os_mutex_post(&phonefs_mutex);
}


/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理读函数
   @param   addr:读取的地址，字节单位。
   @param   len:读取的长度，字节单位。
   @param   buf:读取的目标BUFF。
   @return  u16:读取的长度
   @author  liujie
   @note    u8 phone_recfs_read(u32 addr,u8 *buf,u16 len)
*/
/*----------------------------------------------------------------------------*/
u16 phone_recfs_read(RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len)
{
    putchar('r');
    phonefs_mutex_enter();
    pfs->read(buf, addr, len);
    phonefs_mutex_exit();
    return len;
}

/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理写函数
   @param   addr:要写入设备的地址，字节单位。
   @param   len:需要写入的长度，字节单位。
   @param   buf:数据来源BUFF。
   @return  u16:写入的长度
   @author  liujie
   @note    u16 phone_recfs_wirte(u32 addr,u8 *buf,u16 len)
*/
/*----------------------------------------------------------------------------*/
u16 phone_recfs_wirte(RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len)
{
    putchar('w');
    phonefs_mutex_enter();
    pfs->write(buf, addr, len);
    phonefs_mutex_exit();
    return len;
}


/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理写函数(对齐)
   @param   pfs:文件系统句柄
   @param   addr:要写入设备的地址，字节单位。
   @param   len:需要写入的长度，字节单位。
   @param   buf:数据来源BUFF。
   @return  u16:写入的长度
   @author  liujie
   @note    u16 phone_recfs_wirte_align(RECFILESYSTEM *pfs, u32 addr,u8 *buf,u16 len)
*/
/*----------------------------------------------------------------------------*/
u16 phone_recfs_wirte_align(RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len)
{
    u32 align_addr = addr % FLASH_PAGE_SIZE;
    u16 rlen = FLASH_PAGE_SIZE - align_addr;
    u16 total_len = len;
    u8 *tmpbuf = pfs->buf;

    if (len == 90) {
        align_addr = 0;
    }

    addr -= align_addr;
    /* u32 addr2 = addr; */

    if (align_addr) {
        memset(tmpbuf, 0xff, FLASH_PAGE_SIZE);
        if (rlen > len) {
            rlen = len;
        }
        memcpy(tmpbuf + align_addr, buf, rlen);
        putchar('C');
        phone_recfs_wirte(pfs, addr, tmpbuf, FLASH_PAGE_SIZE);
        memset(tmpbuf, 0, FLASH_PAGE_SIZE);
        len -= rlen;
        buf += rlen;
        addr += FLASH_PAGE_SIZE;
    }

    while (len >= FLASH_PAGE_SIZE) {
        putchar('A');
        memset(tmpbuf, 0xff, FLASH_PAGE_SIZE);
        memcpy(tmpbuf, buf, FLASH_PAGE_SIZE);
        phone_recfs_wirte(pfs, addr, tmpbuf, FLASH_PAGE_SIZE);
        len -= FLASH_PAGE_SIZE;
        buf += FLASH_PAGE_SIZE;
        addr += FLASH_PAGE_SIZE;
    }

    if (len) {
        putchar('B');
        memset(tmpbuf, 0xff, FLASH_PAGE_SIZE);
        memcpy(tmpbuf, buf, len);
        /* phone_recfs_wirte(pfs, addr, tmpbuf, FLASH_PAGE_SIZE); */
        /* put_buf(tmpbuf,len); */
        phone_recfs_wirte(pfs, addr, tmpbuf, len);
    }

    return total_len;
}

/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理擦除函数
   @param   pfs:要写入设备的地址，字节单位。
   @param   start_sector:需要擦除的第一个扇区。
   @param   end_sector:需要擦除的最后一个扇区。
   @return  u16:擦除的扇区数目
   @author  liujie
   @note    u16 phone_rec_erase_sector(RECFILESYSTEM *pfs,u16 start_sector,u16 end_sector)
*/
/*----------------------------------------------------------------------------*/
u16 phone_rec_erase_sector(RECFILESYSTEM *pfs, u16 start_sector, u16 end_sector)
{
//    u8 sr;

    if ((start_sector < pfs->first_sector) || (start_sector > pfs->last_sector)) {
        return 0 ;
    }
    if ((end_sector < pfs->first_sector) || (end_sector > pfs->last_sector)) {
        return 0 ;
    }
    u16 sector_cnt = 0;
    /* r_printf(">>>[test]:start_sector = %d, end_sector =%d\n", start_sector , end_sector); */
    while (1) {

        phonefs_mutex_enter();
        pfs->eraser(((u32)start_sector << pfs->sector_size) + phone_offset_addr);
        phonefs_mutex_exit();
        sector_cnt++;
        if (start_sector == end_sector) {
            break;
        }
        start_sector++;
        if (start_sector > pfs->last_sector) {
            start_sector = pfs->first_sector;
        }
    }
    //return len;//返回擦除的sector数目
    return sector_cnt;
}


/*----------------------------------------------------------------------------*/
/**@brief   录音文件seek函数
   @param   pfile:文件句柄
   @param   type:seek的格式
   @param   offsize:seek的偏移量。
   @return  u8:返回值
   @author  liujie
   @note    u8 phone_recf_seek (REC_FILE *pfile, u8 type, u32 offsize)
*/
/*----------------------------------------------------------------------------*/
u8 phone_recf_seek(REC_FILE *pfile, u8 type, int offsize)
{
    u32 file_len;
    if (NULL == pfile) {
        return 0;
    }
    y_printf(">>>[test]:pfile->len = %d,type = %d, offsize = %d\n", pfile->len, type, offsize);
    file_len = pfile->len;//(u32)pfile->index.sector << pfile->pfs->sector_size;
    if (NOR_FS_SEEK_SET == type) {
        if (abs(offsize) >= file_len) {
            return REC_FILE_END;
        }
        pfile->rw_p = offsize + sizeof(PHONE_RECFILEHEAD);
    } else { // if(NOR_FS_SEEK_CUR == type)
        offsize += pfile->rw_p;
        if (offsize >= file_len) {
            return REC_FILE_END;
        }
        pfile->rw_p += offsize;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   录音文件读函数
   @param   pfile:文件句柄
   @param   buff:数据BUFF
   @param   btr:需要读取的数据长度，字节单位
   @return  u16:读取回来的长度
   @author  liujie
   @note    u16 phone_recf_read(REC_FILE _xdata *pfile ,u8 _xdata *buff , u16 btr)
*/
/*----------------------------------------------------------------------------*/
u16 phone_recf_read(REC_FILE  *pfile, u8 *buff, u16 btr)
{
    u32 file_len, addr;
    u16 read_len = btr;
    u16 len, len0;
    u32 max_addr, min_addr;
    max_addr = phone_offset_addr + ((u32)(pfile->pfs->last_sector + 1) << pfile->pfs->sector_size) ;
    min_addr = phone_offset_addr + ((u32)(pfile->pfs->first_sector) << pfile->pfs->sector_size) ;
    file_len = (u32)pfile->len;
    if ((u32)(btr + pfile->rw_p) >= file_len) {
        if (pfile->rw_p >= file_len) {
            return 0;
        }
        read_len = file_len - pfile->rw_p;
    }
    /* if (pfile->rw_p){ */
    /*         addr = pfile->rw_p + ((u32)pfile->index.sector << pfile->pfs->sector_size) + 16; */
    /* }else */
    addr = pfile->rw_p + ((u32)pfile->index.sector << pfile->pfs->sector_size);
    addr += phone_offset_addr;


    len0 = read_len;
    /* y_printf(">>>[test]r_addr = %d,max_addr =%d, min_addr=%d, pfile->rw_p =%d\n",addr, max_addr, min_addr, pfile->rw_p); */
    /* y_printf(">>>[test]:r_len0 = %d,btr = %d, file_len = %d\n",len0, btr, file_len); */
    while (len0) {
        if (addr >= max_addr) {
            addr = addr - max_addr + min_addr;
        }
        if ((addr + len0) > max_addr) {
            len = max_addr - addr;
        } else {
            len = len0;
        }
        /* y_printf(">>>[test]2222r_addr = %d,max_addr =%d, min_addr=%d, pfile->rw_p =%d\n",addr, max_addr, min_addr, pfile->rw_p); */
        phone_recfs_read(pfile->pfs, addr, buff, len);
        /* y_printf(">>>[test]:2222r_len0 = %d\n",len0); */
        pfile->rw_p += len;
        len0 -= len;
        buff += len;
        addr += len;
    }
    return read_len;
}

/*----------------------------------------------------------------------------*/
/**@brief   录音文件写函数
   @param   pfile:文件句柄
   @param   buff:数据BUFF
   @param   btr:需要写入的数据长度，字节单位
   @return  u16:写入的长度
   @author  liujie
   @note    u16 phone_recf_write(REC_FILE *pfile,u8 *buff,u16 btw)
*/
/*----------------------------------------------------------------------------*/
u16 phone_recf_write(REC_FILE *pfile, u8 *buff, u16 btw)
{
    u32 addr;
    u32 max_addr, min_addr;
    u16 sector;
    u16 len0 = 0;
    u16 len = 0;;
    max_addr = phone_offset_addr + ((u32)(pfile->pfs->last_sector + 1) << pfile->pfs->sector_size) ;
    min_addr = phone_offset_addr + ((u32)(pfile->pfs->first_sector) << pfile->pfs->sector_size)  ;
    addr = pfile->rw_p + ((u32)pfile->index.sector << pfile->pfs->sector_size);
    addr += phone_offset_addr;
    /* y_printf(">>>[test]:addr = %d,max_addr =%d, min_addr=%d, pfile->rw_p =%d\n",addr, max_addr, min_addr, pfile->rw_p); */
    //pfile->len
    while (1) {
        if ((pfile->rw_p + btw) > pfile->len) {
            //获取更多的空间；
            sector = (pfile->len + (1L << pfile->pfs->sector_size) - 1) >> pfile->pfs->sector_size;
            sector += pfile->index.sector;
            if (sector > pfile->pfs->last_sector) {
                sector = sector - pfile->pfs->last_sector + pfile->pfs->first_sector - 1;
            }
            if (pfile->index.sector != sector) {
                /* r_printf(">>>[test]:goto erase\n"); */
                phone_rec_erase_sector(pfile->pfs, sector, sector);
                /* r_printf(">>>[test]:exit erase\n"); */
                pfile->len = pfile->len + (1L << pfile->pfs->sector_size);
            } else {
                /* y_printf(">>>[test]:not erase, sector = %d\n", sector); */
                break;
            }

        } else {
            break;
        }
    }
    if ((pfile->rw_p + btw) > pfile->len) {
        len = pfile->len - pfile->rw_p ;
        y_printf(">>>[test]:len = %d, pfile->len = %d,pfile->rw_p =%d \n", len, pfile->len, pfile->rw_p);
    } else {
        len = btw;
    }
    len0 = len;

    while (len0) {
        if (addr >= max_addr) {
            addr = addr - max_addr + min_addr;
            /* r_printf(">>>[test]:write the end, goto first addr write\n"); */
        }

        if ((addr + len0) > max_addr) {
            btw = max_addr - addr;
        } else {
            btw = len0;
        }
        /* y_printf(">>>[test]:btw = %d\n", btw); */
        phone_recfs_wirte_align(pfile->pfs, addr, buff, btw);
        /* y_printf(">>>[test]:len0 = %d\n",len0); */
        addr += btw;
        buff += btw;
        len0 -= btw;

    }

    pfile->rw_p += len;
    if (pfile->rw_p > pfile->w_len) {
        pfile->w_len = pfile->rw_p;
    }
    return len;
}

/*----------------------------------------------------------------------------*/
/**@brief   清除索引
   @param   pfs:文件系统句柄
   @return  void
   @author  liujie
   @note    void phone_recfs_idx_clear(RECFILESYSTEM *pfs)
*/
/*----------------------------------------------------------------------------*/
void phone_recfs_idx_clear(RECFILESYSTEM *pfs)
{
    PHONE_RECFILEHEAD file_h;
    u32 addr;
    u16 curr_sector;

    curr_sector = pfs->first_sector;
    memset(&file_h, 0, sizeof(PHONE_RECFILEHEAD));

    while (curr_sector <= pfs->last_sector) {
        addr = (u32)curr_sector << pfs->sector_size;
        addr += phone_offset_addr;
        phone_recfs_wirte_align(pfs, addr, (u8 *)(&file_h), sizeof(PHONE_RECFILEHEAD));

        curr_sector++;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   delete a rec_file
   @param   pfs:文件系统句柄
   @return  void
   @author  phew
*/
/*----------------------------------------------------------------------------*/
void phone_recfs_clear_file(REC_FILE *pfile)
{
    PHONE_RECFILEHEAD file_h;
    u32 addr;
    u16 curr_sector;

    curr_sector = pfile->index.sector;
    memset(&file_h, 0, sizeof(PHONE_RECFILEHEAD));
    addr = (u32)curr_sector << pfile->pfs->sector_size;
    addr += phone_offset_addr;
    phone_recfs_wirte_align(pfile->pfs, addr, (u8 *)(&file_h), sizeof(PHONE_RECFILEHEAD));

}

/*----------------------------------------------------------------------------*/
/**@brief   扫描文件系统
   @param   pfs:文件系统句柄
   @return  u32：总文件数
   @author  liujie
   @note    u32 phone_recfs_scan(RECFILESYSTEM *pfs)
*/
/*----------------------------------------------------------------------------*/
/*扫描出最少的文件*/
u32 phone_recfs_scan(RECFILESYSTEM *pfs)
{
    PHONE_RECFILEHEAD file_h __attribute__((aligned(4)));
    RECF_INDEX_INFO max_index;
    //RECF_INDEX_INFO min_index;
    u32 addr;
    u32 total_len;
    u16 crc;
    u16 file_cnt = 0;
    u16 curr_sector = pfs->first_sector;
    pfs->total_file = 0;
    max_index.index = 0;
    max_index.sector = pfs->first_sector;
    total_len = (u32)((u32)pfs->last_sector + 1 - (u32)pfs->first_sector) << pfs->sector_size;
    while (1) {
        addr = (u32)curr_sector << pfs->sector_size;
        addr += phone_offset_addr;
        /* r_printf(">>>[test]:addr=0x%x\n", addr); */
        phone_recfs_read(pfs, addr, (u8 *)(&file_h), sizeof(PHONE_RECFILEHEAD));
        /* put_buf(&file_h , 16); */
        crc = CRC16(&file_h.data_crc, sizeof(PHONE_RECFILEHEAD) - 2);
        /* y_printf(">>>[test]:crc = 0x%x, file_h.len = %d, file_h.index = %d\n", crc, file_h.len, file_h.index); */
        if ((0 != crc) && (crc == file_h.head_crc) && (file_h.len <= total_len)) {
            if (file_h.index >= max_index.index) {
                max_index.index = file_h.index;
                max_index.sector = curr_sector;
            }

            /*
            if(file_h.index <= pfs->index.index)
            {
                min_index.index = file_h.index;
                min_index.sector = curr_sector;
            }
            */
            r_printf(">>>[test]:addr=%d\n", addr);
            file_cnt++;
            if (0 != file_h.len) {
                curr_sector += file_h.len >> pfs->sector_size;
                if (file_h.len % (1L << pfs->sector_size)) {
                    curr_sector++;
                }
            } else {
                curr_sector++;
            }

        } else {
            curr_sector++;
        }
        if (curr_sector > pfs->last_sector) {
            //curr_sector = pfs->first_sector;
            break;
        }
    }

    if (max_index.index < 0) {
        //清空整个MEM()
        y_printf(">>>[test]:goto clear  index.sector\n");
        phone_recfs_idx_clear(pfs);
        pfs->total_file = 0;
        pfs->index.index = 0;
        pfs->index.sector = pfs->first_sector;
    } else {
        pfs->index.index = max_index.index;
        pfs->index.sector = max_index.sector;
        /* y_printf(">>>[test]:recfs_scan   pfs->index.sector = %d, pfs->index.index = %d\n", pfs->index.sector, pfs->index.index); */
        pfs->total_file = file_cnt;
        /* y_printf(">>>[test]:recfs_scan   file_cnt = %d\n", file_cnt); */
    }

    return file_cnt;
}
/*----------------------------------------------------------------------------*/
/**@brief   检查指定扇区是否存爱文件
   @param   pfs:文件系统的句柄
   @param   pfile_h:文件头指针
   @param   sector:需要分析的扇区
   @return  u16:下一个有效扇区
   @author  liujie
   @note    u16 phone_check_head(RECFILESYSTEM *pfs,PHONE_RECFILEHEAD *pfile_h,u16 sector)
*/
/*----------------------------------------------------------------------------*/
u16 phone_check_head(RECFILESYSTEM *pfs, PHONE_RECFILEHEAD *pfile_h, u16 sector)
{
    //RECFILEHEAD file_h;
    u32 addr;
    u32 total_len;
    addr = sector;
    addr = addr << pfs->sector_size;
    addr += phone_offset_addr;

    total_len = (u32)((u32)pfs->last_sector + 1 - (u32)pfs->first_sector) << pfs->sector_size;
    y_printf(">>>[test]:  check head  addr = %d\n", addr);
    phone_recfs_read(pfs, addr, (u8 *)pfile_h, sizeof(PHONE_RECFILEHEAD));
    if ((CRC16(&pfile_h->data_crc, sizeof(PHONE_RECFILEHEAD) - 2) == pfile_h->head_crc)
        && (pfile_h->len <= total_len)
       ) {
        r_printf(">>>[test]:AAAAAAAAAAA\n");
        sector = sector + (pfile_h->len >> pfs->sector_size);
        r_printf(">>>[test]:sector = %d\n", sector);
        if (pfile_h->len % (1L << pfs->sector_size)) {
            r_printf(">>>[test]:BBBBBBBBBBB\n");
            sector++;
        } else {
        }
        /*
        if(sector > pfs->last_sector)
        {
            sector -= pfs->last_sector;
        }
        */
    } else {
    }
    return sector;
}



/*----------------------------------------------------------------------------*/
/**@brief   建立一个文件
   @param   pfs:文件系统的句柄
   @param   pfile:文件的句柄
   @return  u32:建立文件索引
   @author  liujie
   @note    u32 phone_create_recfile(RECFILESYSTEM *pfs,REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 phone_create_recfile(RECFILESYSTEM *pfs, REC_FILE *pfile)
{
    PHONE_RECFILEHEAD file_h;
    u16 sector, sec0;
    u16 sec_len;

    memset((u8 *)pfile, 0, sizeof(REC_FILE));
    pfile->pfs = pfs;
    sector = pfs->index.sector;
    /* y_printf(">>>[test]:create   pfs->index.sector = %d\n", pfs->index.sector); */
    sec0 = phone_check_head(pfs, &file_h, sector);
    if (sector != sec0) {
        if (sec0 > pfs->last_sector) {
            sec0 = sec0 - pfs->last_sector + pfs->first_sector - 1;
        }
        sector = sec0;
    }
    pfile->index.index = pfs->index.index + 1;
    pfile->index.sector = sec0;
    sec_len = phone_rec_erase_sector(pfs, sector, sec0);
    /* y_printf(">>>[test]:sec_len = %d\n",sec_len); */
    pfile->len = (u32)sec_len << pfs->sector_size;
    pfile->w_len = sizeof(PHONE_RECFILEHEAD);
    pfile->rw_p = sizeof(PHONE_RECFILEHEAD);
    pfile->addr = pfile->rw_p + (pfile->index.sector << pfile->pfs->sector_size) + phone_offset_addr;
    /* y_printf(">>>[test]:rw_p = %d\n", pfile->rw_p); */
    return pfile->index.index;
}


/*----------------------------------------------------------------------------*/
/**@brief   关闭一个文件
   @param   pfile:文件的句柄
   @return  u32:建立文件索引
   @author  liujie
   @note    u32 phone_close_recfile(REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 phone_close_recfile(REC_FILE *pfile)
{
    PHONE_RECFILEHEAD file_h;

    memset(&file_h, 0xff, sizeof(PHONE_RECFILEHEAD));
    //file_h->crc;
    pfile->len = pfile->w_len;
    pfile->rw_p = 0;//sizeof(PHONE_RECFILEHEAD);
    file_h.len = pfile->w_len;
    file_h.data_crc = sizeof(PHONE_RECFILEHEAD);
    file_h.index = pfile->index.index;
    file_h.addr = pfile->addr;
    memcpy(file_h.name, pfile->priv_data, NORFS_DATA_LEN);

    file_h.head_crc = CRC16(&file_h.data_crc, sizeof(PHONE_RECFILEHEAD) - 2);
    r_printf(">>>[test]:pfile->index.sector = %d\n", pfile->index.sector);

    //memset(&file_h,0,sizeof(RECFILEHEAD));
    //recfs_read((u32)pfile->index.sector<<pfile->pfs->sector_size,(u8 *)&file_h,sizeof(RECFILEHEAD));

    phone_recf_write(pfile, (u8 *)&file_h, sizeof(PHONE_RECFILEHEAD));
    /* put_buf(&file_h , 16); */
    pfile->pfs->index.index = pfile->index.index;
    pfile->pfs->index.sector = pfile->index.sector;

    //memset(&file_h,0,sizeof(RECFILEHEAD));
    //recfs_read((u32)pfile->index.sector<<pfile->pfs->sector_size,(u8 *)&file_h,sizeof(RECFILEHEAD));

    /* r_printf(">>>[test]:file_h.len = %d\n", file_h.len); */
    return file_h.len;
}

void phone_recf_save_sr(REC_FILE *pfile, u16 sr)
{
    if (pfile) {
        pfile->sr = sr;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   按照索引号，搜索一个文件
   @param   index:索引号
   @param   pfile:文件的句柄
   @return  u32:找到文件的索引
   @author  liujie
   @note    u32 phone_revf_index_scan(u32 index,REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 phone_revf_index_scan(u32 index, REC_FILE *pfile)
{
    PHONE_RECFILEHEAD file_h;
    RECFILESYSTEM *pfs;
    u32 addr;
    u32 total_len;
    u16 curr_sector, crc;

    pfs = pfile->pfs;
    curr_sector = pfs->index.sector;
    total_len = (u32)((u32)pfs->last_sector + 1 - (u32)pfs->first_sector) << pfs->sector_size;
    r_printf(">>>[test]:pfile->pfs->index.sector = %d, total_len = %d\n", pfile->pfs->index.sector, total_len);

    while (1) {
        addr = (u32)curr_sector << pfs->sector_size;
        addr += phone_offset_addr;
        r_printf(">>>[test]:offset_addr = 0x%x, addr = 0x%x\n", phone_offset_addr, addr);

        phone_recfs_read(pfs, addr, (u8 *)(&file_h), sizeof(PHONE_RECFILEHEAD));
        /* put_buf(&file_h , 16); */
        crc = CRC16(&file_h.data_crc, sizeof(PHONE_RECFILEHEAD) - 2);
        y_printf(">>>[test]:crc = 0x%x, file_h.len = %d, file_h.index = %d\n", crc, file_h.len, file_h.index);
        if ((0 != crc) && (crc == file_h.head_crc) && (file_h.len <= total_len)) {
            if (file_h.index == index) {
                r_printf(">>>[test]:go in AAAAAA\n");
                pfile->index.index = index;
                pfile->index.sector = curr_sector;
                pfile->len = file_h.len;
                pfile->w_len = file_h.len;
                pfile->rw_p = sizeof(PHONE_RECFILEHEAD);
                pfile->addr = file_h.addr;
                /* pfile->sr = (u16)(((u16)file_h.reserv[1] << 8) | file_h.reserv[0]);//利用了保留区 */
                memcpy(pfile->priv_data, file_h.name, NORFS_DATA_LEN);
                return index;
            } else {
                if (0 == file_h.len) {
                    curr_sector++;
                }
                curr_sector = curr_sector + (file_h.len >> pfs->sector_size);
                if (file_h.len % (1L << pfs->sector_size)) {
                    curr_sector++;
                }
            }
        } else {
            curr_sector++;
        }

        if (curr_sector > pfs->last_sector) {
            curr_sector = pfs->first_sector;
        }
        /* r_printf(">>>[test]:pfile->index.sector = %d, curr_sector = %d\n", pfile->index.sector, curr_sector); */
        if (pfs->index.sector == curr_sector) {
            break;
        }

    }
    return index - 1;
}

/*----------------------------------------------------------------------------*/
/**@brief   按照索引号打开一个文件
   @param   index:索引号
   @param   pfs:文件系统的句柄
   @param   pfile:文件的句柄
   @return  u32:找到文件的索引
   @author  liujie
   @note    u32 phone_open_recfile(u32 index,RECFILESYSTEM *pfs,REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 phone_open_recfile(u32 index, RECFILESYSTEM *pfs, REC_FILE *pfile)
{
    memset((u8 *)pfile, 0, sizeof(REC_FILE));
    pfile->pfs = pfs;
    pfile->rw_p = sizeof(PHONE_RECFILEHEAD);
    return phone_revf_index_scan(index, pfile);
}


u32 decode_data_by_user_key_in_sdfile(u16 key, u8 *buff, u16 size, u32 dec_addr, u8 dec_len)
{
    u16 key_addr;
    u16 r_len;

    while (size) {
        r_len = (size > dec_len) ? dec_len : size;
        key_addr = (dec_addr >> 2)^key;
        doe_in_phonefs(key_addr, buff, r_len, 0);
        buff += r_len;
        dec_addr += r_len;
        size -= r_len;
    }

    return dec_addr;
}

static u16 get_chip_id_in_phonefs()
{
    return boot_info.chip_id;
}

static void head_addr_get(char *name)
{
    extern u32 boot_info_get_sfc_base_addr(void);
    u32 start_addr = boot_info_get_sfc_base_addr();
    u32 offset = 0;
    struct sdfile_file_head head = {0};
    u16 local_flash_key = get_chip_id_in_phonefs();
    while (1) {
        extern int norflash_origin_read(u8 * buf, u32 offset, u32 len);
        norflash_origin_read((u8 *)&head, start_addr, sizeof(head));
        decode_data_by_user_key_in_sdfile(local_flash_key, (u8 *)&head, sizeof(head), offset, sizeof(head));
        if (head.head_crc != CRC16((u8 *)&head.data_crc, sizeof(head) - sizeof(head.head_crc))) {
            y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);
            return;
        }
        if (0 == memcmp(head.name, name, strlen(name))) {
            break;
        }
        offset += sizeof(head);
        start_addr += sizeof(head);
    }

    {
        extern int phone_offset_addr;
        extern int max_rec_len;
        phone_offset_addr = head.addr;
        max_rec_len = head.len;
    }
    r_printf(">>>[test]:#########  start_addr = 0x%x, len = 0x%x\n", head.addr, head.len);
}



/*----------------------------------------------------------------------------*/
/**@brief   初始化文件系统信息
   @param   pfs:文件系统的句柄
   @param   sector_start:起始扇区
   @param   sector_end:结束扇区
   @param   sector_size:扇区大小（1L<<sector_size）
   @return  u32:找到文件的索引
   @author  liujie
   @note    void phone_init_nor_fs(RECFILESYSTEM *pfs,u16 sector_start,u16 sector_end,u8 sector_size)
*/
/*----------------------------------------------------------------------------*/
void phone_init_nor_fs(RECFILESYSTEM *pfs, u16 sector_start, u16 sector_end, u8 sector_size)
{
//    memset((u8 *)pfs,0,sizeof(RECFILESYSTEM));
    pfs->first_sector = sector_start;
    pfs->last_sector = sector_end;
    pfs->sector_size = sector_size;
}


RECFILESYSTEM phone_recfs;
REC_FILE phone_recfile;
int phone_max_rec_capacity = 256 * 1024;
int set_rec_capacity(int capacity) //需要先设置容量。
{

    r_printf(">>>[test]:goto set_rec_capacity:%d\n", capacity);
    head_addr_get("REC");

    ASSERT(capacity <= max_rec_len, "capacity must smaller than max_rec_len!!!!!!! ");
    phone_max_rec_capacity = capacity;
    return phone_max_rec_capacity;
}
#define FS_SECTOR_S     (4*1024L)
#define FS_S_SECTOR     (1)
#define FS_E_SECTOR     (phone_max_rec_capacity/FS_SECTOR_S-1)

/////////////////////////////SDFILE_REC_FUNCTION////////////////////////////////////////
/* extern u32 hook_sdfile_rec_read(u8 *buf, u32 addr, u32 len); */
/* extern u32 hook_sdfile_rec_write(u8 *buf, u32 addr, u32 len); */
/* extern u32 hook_sdfile_rec_erase(u32 addr); */
extern int norflash_read(struct device *device, void *buf, u32 len, u32 offset);
extern int norflash_write(struct device *device, void *buf, u32 len, u32 offset);
extern int norflash_erase(u32 cmd, u32 addr);

s32 hook_sdfile_rec_read(u8 *buf, u32 addr, u32 len)
{
    return norflash_read(NULL, (void *)buf, len, addr);
}

s32 hook_sdfile_rec_write(u8 *buf, u32 addr, u32 len)
{
    return norflash_write(NULL, (void *)buf, len, addr);
}

void hook_sdfile_rec_erase(u32 addr)
{
    if (flash_page_mode_flag) {
        norflash_erase(IOCTL_ERASE_PAGE, addr);
        return;
    }
    norflash_erase(IOCTL_ERASE_SECTOR, addr);
}



u32 phone_init_recfs(void)
{
    memset(&phone_recfs, 0, sizeof(RECFILESYSTEM));
    phone_recfs.eraser = hook_sdfile_rec_erase;
    phone_recfs.read  = hook_sdfile_rec_read;
    phone_recfs.write = hook_sdfile_rec_write;
    return true;
}

u32 sdfile_rec_init(void)
{
    if (boot_info.vm.align == 1) {
        flash_page_mode_flag = 1;
    }
    phone_init_recfs();
    if (flash_page_mode_flag) {
        phone_init_nor_fs(&phone_recfs, FS_S_SECTOR, (phone_max_rec_capacity / 256 - 1), 8);
    } else {
        phone_init_nor_fs(&phone_recfs, FS_S_SECTOR, FS_E_SECTOR, 12);
    }
    phone_recfs_scan(&phone_recfs);
#if (VFS_ENABLE == 1)
    if (mount(NULL, "mnt/rec_sdfile", "rec_sdfile", 0, NULL)) {
        r_printf("sdfile_rec mount succ");
    } else {
        r_printf("sdfile_rec mount failed!!!");
    }
#endif /* #if (VFS_ENABLE == 1) */
    y_printf(">>>[test]:sdfile_rec_init\n");
    sdfile_rec_index = phone_recfs.index.index;
    return  phone_recfs.index.index;
}

u32 sdfile_rec_createfile(void)
{
    int index;
    y_printf(">>>[test]:goto create\n");
    phone_recfs_idx_clear(&phone_recfs);
    index = phone_create_recfile(&phone_recfs, &phone_recfile);
    return index;

}

u32 sdfile_rec_write(void *buff, u32 len)
{
    u16 w_len;
    w_len = phone_recf_write(&phone_recfile, buff, len);
    /* put_buf(buff, len); */
    return w_len;
}

u32 sdfile_rec_read(void *buff, u32 len)
{
    u16 r_len;
    if (len > 512) {
        log_e("len > 512!!!!!!!!!!");
        return -1;
    }
    /* r_printf(">>>[test]:goto nor_fs read\n"); */
    if (((u32)buff % 4) != 0) {
        void *tmp_buf = malloc(512);
        r_len = phone_recf_read(&phone_recfile, tmp_buf, len);
        memcpy(buff, tmp_buf, len);
        free(tmp_buf);
        /* put_buf(buff, len); */
        return r_len;
    }
    r_len = phone_recf_read(&phone_recfile, buff, len);
    /* put_buf(buff, len); */
    return r_len;
}

u32 sdfile_rec_open(u32 index)
{
    int reg;
    reg = phone_open_recfile(index, &phone_recfs, &phone_recfile);
    return reg;
}

u32 sdfile_rec_close(void)
{
    int reg;
    r_printf(">>>[test]:goto nor_fs close\n");
    reg =  phone_close_recfile(&phone_recfile);
    return reg;
}

u32 sdfile_rec_seek(int offsize, int type)
{
    int reg;
    /* r_printf(">>>[test]:goto nor_fs seek\n"); */
    reg = phone_recf_seek(&phone_recfile, (u8) type, offsize);
    return reg;
}

u32 sdfile_rec_flen(void)
{
    return phone_recfile.len;
}

void sdfile_rec_get_priv_info(u8 *info)
{
    memcpy(info, &phone_recfile.priv_data, 16);
}

void  sdfile_rec_set_priv_info(u8 *info) //用于设置数据格式和电话信息.
{
    memcpy(&phone_recfile.priv_data, info, 16);
}

u16 sdfile_rec_get_sr(void)
{
    return phone_recfile.sr;
}

void sdfile_rec_set_sr(u16 sr)
{
    phone_recfile.sr = sr;
}

void rec_clear_norfs_fileindex(void)
{
    phone_recfs_idx_clear(&phone_recfs);
}

u32 rec_delete_rec_file(u32 index)
{
    int num;
    num = phone_open_recfile(index, &phone_recfs, &phone_recfile);
    if (num != index) {
        log_e("open error");
        return -1;
    }
    phone_recfs_clear_file(&phone_recfile);
    return 0;
}

u32 flashinsize_rec_get_capacity(void)
{
    return phone_max_rec_capacity;
}

u32 flashinsize_rec_get_index(void)
{
    return sdfile_rec_index;
}

#if (VFS_ENABLE == 1)
int sdfile_rec_scan_ex()
{
    phone_recfs_scan(&phone_recfs);
    return  phone_recfs.index.index;
}

int recfile_insizeflash_set_index(u8 file_sel, u32 index)
{
    y_printf(">>>[test]:goto recfile_insizeflash_set_index\n");
    phone_recfs_scan(&phone_recfs);
    switch (file_sel) {
    case  FSEL_NEXT_FILE:
        r_printf(">>>[test]:flash FSEL_NEXT_FILE\n");
        sdfile_rec_index++;
        if (sdfile_rec_index > phone_recfs.index.index) {
            log_w("play on the end, goto first play!!!!!!!!!");
            sdfile_rec_index = 1;
            /* music_flash_file_set_index(FSEL_CURR_FILE, 1); */
        }
        break;
    case  FSEL_PREV_FILE:
        r_printf(">>>[test]:flash FSEL_PREV_FILE\n");
        sdfile_rec_index--;
        if (sdfile_rec_index <= 0) {
            log_e("index <= 0 err!!!, play first");
            sdfile_rec_index = 1;
            /* music_flash_file_set_index(FSEL_CURR_FILE, 1); */
        }
        break;
    case  FSEL_CURR_FILE:
        r_printf(">>>[test]:flash FSEL_CURR_FILE\n");
        if (index <= (phone_recfs.index.index - phone_recfs.total_file)) {
            sdfile_rec_index = (phone_recfs.index.index - phone_recfs.total_file) + 1;
        } else if (index && (index <= phone_recfs.index.index)) {
            sdfile_rec_index = index;
        } else {
            log_e("error!!!!!!! no file");
            return 1;
        }
        break;
    }
    if (sdfile_rec_index <= (phone_recfs.index.index - phone_recfs.total_file)) {
        sdfile_rec_index = (phone_recfs.index.index - phone_recfs.total_file) + 1;
    }
    return 0;
}


u32 _sdfile_rec_init(void)
{
    return  sdfile_rec_init();
}

static int get_ext_name(char *path, char *ext)
{
    int len = strlen(path);
    char *name = (char *)path;
    while (len--) {
        if (*name++ == '.') {
            break;
        }
    }
    if (len <= 0) {
        name = path + (strlen(path) - 3);
    }
    memcpy(ext, name, 3);
    ext[3] = 0;
    return 1;
}

static int get_filename_by_path(char *filename, char *path)
{
    int len = strlen(path);
    char *name = (char *)path;
    name = path + len;
    for (int i = len; i >= 0; i--) {
        if (*name-- == '/') {
            break;
        }
        if ((len - i) > 15) {
            r_printf(">>>[test]:i= %d,len = %d\n", i, len);
            log_w("filename is too long!!!!!!!!");
            break;
        }
    }

    memcpy(filename, name + 2, 15);
    filename[15] = 0;
    return 1;

}

static int sdfile_first_flag = 1;
static int _sdfile_rec_open(FILE *_file, const char *path, const char *mode)
{
    int index;
    int reg;
    y_printf(">>>[test]:nor_fs_open\n");

    if (mode[0] == 'w') {
        y_printf(">>>[test]:goto create\n");
        char ext[4] = {0};
        char filename[16] = {0};
        get_ext_name((char *)path, (char *)ext);
        if (memcmp(ext, "WAV", 3) == 0 || memcmp(ext, "wav", 3) == 0) {
            sdfile_first_flag = 1;
        } else {
            sdfile_first_flag = 0;
        }

        index = sdfile_rec_createfile();
        get_filename_by_path((char *)filename, (char *) path);
        sdfile_rec_set_priv_info((u8 *)filename);
        r_printf(">>>[test]:path = %s, ext = %s, filename = %s\n", path, ext, filename);

        sdfile_rec_index = index;
        return 0;
    } else if (mode[0] == 'r') {
        y_printf(">>>[test]: open_recfile  nor_fs_hdl->index= %d\n", sdfile_rec_index);
        if (sdfile_rec_index) {
            index = sdfile_rec_index;
        } else {
            log_e("error!!!!!!!!!");
            index = 0;
            return 1;
        }
        reg = sdfile_rec_open(index);
        return 0;
    }
    return 0;

}

static int _sdfile_rec_write(FILE *_file, void *buff, u32 len)
{
    if (sdfile_first_flag == 1) {
        memset(buff, 0xff, 90);
        sdfile_first_flag = 0;
    }
    return sdfile_rec_write(buff, len);
}
static int _sdfile_rec_read(FILE *_file, void *buff, u32 len)
{
    return sdfile_rec_read(buff, len);
}
static int _sdfile_rec_close(FILE *_file)
{
    return sdfile_rec_close();
}
static int _sdfile_rec_seek(FILE *_file, int offsize, int type)
{
    return sdfile_rec_seek(offsize, type);
}
static int _sdfile_rec_flen(FILE *_file)
{
    return sdfile_rec_flen();
}
static int _sdfile_rec_fpos(FILE *_file)
{
    return phone_recfile.rw_p;
}
static int _sdfile_rec_mount(struct imount *mt, int cache_num)
{
    y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);
    struct vfs_partition *app_part = &mt->part;

    app_part->offset = 0 + phone_offset_addr;
    app_part->dir[0] = 'C';
    app_part->dir[1] = '\0';

    mt->part_num = 1;

    return 0;

}

REGISTER_VFS_OPERATIONS(inside_nor_fs_vfs_ops) = {
    .fs_type         = "rec_sdfile",
    .mount           = _sdfile_rec_mount,
    .unmount	     = NULL,
    .format          = NULL,
    .fopen	         = _sdfile_rec_open,
    .fwrite          = _sdfile_rec_write,
    .fread        	 = _sdfile_rec_read,
    .fseek 		     = _sdfile_rec_seek,
    .flen 		     = _sdfile_rec_flen,
    .fpos 		     = _sdfile_rec_fpos,
    .fclose          = _sdfile_rec_close,
    .fscan  	     = NULL,
    .fscan_release 	 = NULL,
    .fsel	         = NULL,
    .fdelete	     = NULL,
    .fget_name       = NULL,
    .frename         = NULL,
    .fget_free_space = NULL,
    .fget_attr       = NULL,
    .fset_attr       = NULL,
    .fget_attrs      = NULL,
    .fset_vol        = NULL,
    .fmove           = NULL,
    .ioctl           = NULL,
};

int sdfile_rec_ops_init(void)
{
    return 0;
}

#endif


