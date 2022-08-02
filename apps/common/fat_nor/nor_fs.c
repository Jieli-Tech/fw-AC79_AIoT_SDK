/***********************************Jieli tech************************************************
  File : nor_fs.c
  By   : Huxi
  Email: xi_hu@zh-jieli.com
  date : 2016-11-30 14:30
********************************************************************************************/
#include "spi/nor_fs.h"
#include "os/os_api.h"
#include "asm/crc16.h"
/* #include <string.h> */
#include "stdlib.h"
#include "fs/fs.h"
/* #include "jlfat/ff_opr.h" */
#include "system/includes.h"




#if (VFS_ENABLE == 1)
//文件头
/* typedef struct __RECFILEHEAD { */
/*     u16 crc; */
/*     u8 head_len; */
/*     u8 reserv[5]; */
/*     u32 file_len; */
/*     u32 index; */
/* } RECFILEHEAD, *PRECFILEHEAD __attribute__((aligned(4))); */


#define RECFILEHEAD  SDFILE_FILE_HEAD

static int offset_addr = 0;
/* int max_rec_len = 0; */


#define NO_DEBUG_EN

#ifdef NO_DEBUG_EN
#undef r_printf
#undef y_printf
#define r_printf(...)
#define y_printf(...)
#endif

u8 norfs_mutex_dis = 0;
OS_MUTEX norfs_mutex;             ///<互斥信号量
void norfs_mutex_init(void)       ///<信号量初始化
{
    if (norfs_mutex_dis) {
        return;
    }
    static u8 init = 0;
    if (init) {
        return;
    }
    init = 1;
    os_mutex_create(&norfs_mutex);
}
void norfs_mutex_enter(void)      ///<申请信号量
{
    if (norfs_mutex_dis) {
        return;
    }
    norfs_mutex_init();
    os_mutex_pend(&norfs_mutex, 0);
}
void norfs_mutex_exit(void)       ///<释放信号量
{
    if (norfs_mutex_dis) {
        return;
    }
    os_mutex_post(&norfs_mutex);
}


/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理读函数
   @param   addr:读取的地址，字节单位。
   @param   len:读取的长度，字节单位。
   @param   buf:读取的目标BUFF。
   @return  u16:读取的长度
   @author  liujie
   @note    u8 recfs_read(u32 addr,u8 *buf,u16 len)
*/
/*----------------------------------------------------------------------------*/
u16 recfs_read(RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len)
{
    putchar('r');
    norfs_mutex_enter();
    pfs->read(buf, addr, len);
    norfs_mutex_exit();
    return len;
}

/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理写函数
   @param   addr:要写入设备的地址，字节单位。
   @param   len:需要写入的长度，字节单位。
   @param   buf:数据来源BUFF。
   @return  u16:写入的长度
   @author  liujie
   @note    u16 recfs_wirte(u32 addr,u8 *buf,u16 len)
*/
/*----------------------------------------------------------------------------*/
u16 recfs_wirte(RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len)
{
    putchar('w');
    norfs_mutex_enter();
    pfs->write(buf, addr, len);
    norfs_mutex_exit();
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
   @note    u16 recfs_wirte_align(RECFILESYSTEM *pfs, u32 addr,u8 *buf,u16 len)
*/
/*----------------------------------------------------------------------------*/
u16 recfs_wirte_align(RECFILESYSTEM *pfs, u32 addr, u8 *buf, u16 len)
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
        recfs_wirte(pfs, addr, tmpbuf, FLASH_PAGE_SIZE);
        memset(tmpbuf, 0, FLASH_PAGE_SIZE);
        len -= rlen;
        buf += rlen;
        addr += FLASH_PAGE_SIZE;
    }

    while (len >= FLASH_PAGE_SIZE) {
        putchar('A');
        memset(tmpbuf, 0xff, FLASH_PAGE_SIZE);
        memcpy(tmpbuf, buf, FLASH_PAGE_SIZE);
        recfs_wirte(pfs, addr, tmpbuf, FLASH_PAGE_SIZE);
        len -= FLASH_PAGE_SIZE;
        buf += FLASH_PAGE_SIZE;
        addr += FLASH_PAGE_SIZE;
    }

    if (len) {
        putchar('B');
        memset(tmpbuf, 0xff, FLASH_PAGE_SIZE);
        memcpy(tmpbuf, buf, len);
        /* recfs_wirte(pfs, addr, tmpbuf, FLASH_PAGE_SIZE); */
        /* put_buf(tmpbuf,len); */
        recfs_wirte(pfs, addr, tmpbuf, len);
    }

    /* void *tmpbuf2 = malloc(512); */
    /* if (!tmpbuf2){ */
    /*     r_printf(">>>[test]:no 512 cache\n"); */
    /* } */
    /*     memset(tmpbuf2, 0, 512); */
    /*     recfs_read(pfs, addr2, tmpbuf2, 512); */
    /*     put_buf(tmpbuf2,512); */
    /*     free(tmpbuf2); */
    return total_len;
}

/*----------------------------------------------------------------------------*/
/**@brief   提供给录音文件系统的物理擦除函数
   @param   pfs:要写入设备的地址，字节单位。
   @param   start_sector:需要擦除的第一个扇区。
   @param   end_sector:需要擦除的最后一个扇区。
   @return  u16:擦除的扇区数目
   @author  liujie
   @note    u16 rec_erase_sector(RECFILESYSTEM *pfs,u16 start_sector,u16 end_sector)
*/
/*----------------------------------------------------------------------------*/
u16 rec_erase_sector(RECFILESYSTEM *pfs, u16 start_sector, u16 end_sector)
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

        norfs_mutex_enter();
        pfs->eraser(((u32)start_sector << pfs->sector_size) + offset_addr);
        norfs_mutex_exit();
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
   @note    u8 recf_seek (REC_FILE *pfile, u8 type, u32 offsize)
*/
/*----------------------------------------------------------------------------*/
u8 recf_seek(REC_FILE *pfile, u8 type, int offsize)
{
    u32 file_len;
    if (NULL == pfile) {
        return 0;
    }
    /* y_printf(">>>[test]:pfile->len = %d,type = %d, offsize = %d\n",pfile->len, type, offsize); */
    file_len = pfile->len;//(u32)pfile->index.sector << pfile->pfs->sector_size;
    if (NOR_FS_SEEK_SET == type) {
        if (abs(offsize) >= file_len) {
            return REC_FILE_END;
        }
        pfile->rw_p = offsize + sizeof(RECFILEHEAD);
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
   @note    u16 recf_read(REC_FILE _xdata *pfile ,u8 _xdata *buff , u16 btr)
*/
/*----------------------------------------------------------------------------*/
u16 recf_read(REC_FILE  *pfile, u8 *buff, u16 btr)
{
    u32 file_len, addr;
    u16 read_len = btr;
    u16 len, len0;
    u32 max_addr, min_addr;
    max_addr = offset_addr + ((u32)(pfile->pfs->last_sector + 1) << pfile->pfs->sector_size) ;
    min_addr = offset_addr + ((u32)(pfile->pfs->first_sector) << pfile->pfs->sector_size) ;
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
    /* if (offset_addr){ */
    addr += offset_addr;
    /* } */


    len0 = read_len;
    /* y_printf(">>>[test]:r_addr = %d,max_addr =%d, min_addr=%d, pfile->rw_p =%d\n", addr, max_addr, min_addr, pfile->rw_p); */
    //y_printf(">>>[test]:r_len0 = %d,btr = %d, file_len = %d\n",len0, btr, file_len);
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
        recfs_read(pfile->pfs, addr, buff, len);
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
   @note    u16 recf_write(REC_FILE *pfile,u8 *buff,u16 btw)
*/
/*----------------------------------------------------------------------------*/
u16 recf_write(REC_FILE *pfile, u8 *buff, u16 btw)
{
    u32 addr;
    u32 max_addr, min_addr;
    u16 sector;
    u16 len0 = 0;
    u16 len = 0;;
    max_addr = offset_addr + ((u32)(pfile->pfs->last_sector + 1) << pfile->pfs->sector_size) ;
    min_addr = offset_addr + ((u32)(pfile->pfs->first_sector) << pfile->pfs->sector_size)  ;
    addr = pfile->rw_p + ((u32)pfile->index.sector << pfile->pfs->sector_size);
    /* if (offset_addr){ */
    addr += offset_addr;
    /* } */
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
                rec_erase_sector(pfile->pfs, sector, sector);
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
        recfs_wirte_align(pfile->pfs, addr, buff, btw);
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
   @note    void recfs_idx_clear(RECFILESYSTEM *pfs)
*/
/*----------------------------------------------------------------------------*/
void recfs_idx_clear(RECFILESYSTEM *pfs)
{
    RECFILEHEAD file_h;
    u32 addr;
    u16 curr_sector;

    curr_sector = pfs->first_sector;
    memset(&file_h, 0, sizeof(RECFILEHEAD));

    while (curr_sector <= pfs->last_sector) {
        addr = (u32)curr_sector << pfs->sector_size;
        /* if (offset_addr){ */
        addr += offset_addr;
        /* } */
        recfs_wirte_align(pfs, addr, (u8 *)(&file_h), sizeof(RECFILEHEAD));

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
void recfs_clear_file(REC_FILE *pfile)
{
    RECFILEHEAD file_h;
    u32 addr;
    u16 curr_sector;

    curr_sector = pfile->index.sector;
    memset(&file_h, 0, sizeof(RECFILEHEAD));
    addr = (u32)curr_sector << pfile->pfs->sector_size;
    addr += offset_addr;
    recfs_wirte_align(pfile->pfs, addr, (u8 *)(&file_h), sizeof(RECFILEHEAD));

}

/*----------------------------------------------------------------------------*/
/**@brief   扫描文件系统
   @param   pfs:文件系统句柄
   @return  u32：总文件数
   @author  liujie
   @note    u32 recfs_scan(RECFILESYSTEM *pfs)
*/
/*----------------------------------------------------------------------------*/
/*扫描出最少的文件*/
u32 recfs_scan(RECFILESYSTEM *pfs)
{
    RECFILEHEAD file_h __attribute__((aligned(4)));
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
        /* if (offset_addr){ */
        addr += offset_addr;
        /* } */
        /* r_printf(">>>[test]:addr=0x%x\n", addr); */
        recfs_read(pfs, addr, (u8 *)(&file_h), sizeof(RECFILEHEAD));
        /* put_buf(&file_h , 16); */
        crc = CRC16(&file_h.data_crc, sizeof(RECFILEHEAD) - 2);
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
        recfs_idx_clear(pfs);
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
   @note    u16 check_head(RECFILESYSTEM *pfs,RECFILEHEAD *pfile_h,u16 sector)
*/
/*----------------------------------------------------------------------------*/
u16 check_head(RECFILESYSTEM *pfs, RECFILEHEAD *pfile_h, u16 sector)
{
    //RECFILEHEAD file_h;
    u32 addr;
    u32 total_len;
    addr = sector;
    addr = addr << pfs->sector_size;
    /* if (offset_addr){ */
    addr += offset_addr;
    /* } */

    total_len = (u32)((u32)pfs->last_sector + 1 - (u32)pfs->first_sector) << pfs->sector_size;
    y_printf(">>>[test]:  check head  addr = %d\n", addr);
    recfs_read(pfs, addr, (u8 *)pfile_h, sizeof(RECFILEHEAD));
    if ((CRC16(&pfile_h->data_crc, sizeof(RECFILEHEAD) - 2) == pfile_h->head_crc)
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
   @note    u32 create_recfile(RECFILESYSTEM *pfs,REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 create_recfile(RECFILESYSTEM *pfs, REC_FILE *pfile)
{
    RECFILEHEAD file_h;
    u16 sector, sec0;
    u16 sec_len;

    memset((u8 *)pfile, 0, sizeof(REC_FILE));
    pfile->pfs = pfs;
    sector = pfs->index.sector;
    /* y_printf(">>>[test]:create   pfs->index.sector = %d\n", pfs->index.sector); */
    sec0 = check_head(pfs, &file_h, sector);
    if (sector != sec0) {
        if (sec0 > pfs->last_sector) {
            sec0 = sec0 - pfs->last_sector + pfs->first_sector - 1;
        }
        sector = sec0;
    }
    pfile->index.index = pfs->index.index + 1;
    pfile->index.sector = sec0;
    sec_len = rec_erase_sector(pfs, sector, sec0);
    /* y_printf(">>>[test]:sec_len = %d\n",sec_len); */
    pfile->len = (u32)sec_len << pfs->sector_size;
    pfile->w_len = sizeof(RECFILEHEAD);
    pfile->rw_p = sizeof(RECFILEHEAD);
    pfile->addr = pfile->rw_p + (pfile->index.sector << pfile->pfs->sector_size) + offset_addr;
    /* y_printf(">>>[test]:rw_p = %d\n", pfile->rw_p); */
    return pfile->index.index;
}


/*----------------------------------------------------------------------------*/
/**@brief   关闭一个文件
   @param   pfile:文件的句柄
   @return  u32:建立文件索引
   @author  liujie
   @note    u32 close_recfile(REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 close_recfile(REC_FILE *pfile)
{
    RECFILEHEAD file_h;

    memset(&file_h, 0xff, sizeof(RECFILEHEAD));
    //file_h->crc;
    pfile->w_len = pfile->rw_p;

    pfile->len = pfile->w_len;
    pfile->rw_p = 0;//sizeof(RECFILEHEAD);
    file_h.len = pfile->w_len;
    file_h.index = pfile->index.index;
    file_h.addr = pfile->addr;
    memcpy(file_h.name, pfile->priv_data, NORFS_DATA_LEN);
    r_printf(">>>[test]:file_h.priv_data = %s, len = %d\n", file_h.name, NORFS_DATA_LEN);

    file_h.head_crc = CRC16(&file_h.data_crc, sizeof(RECFILEHEAD) - 2);
    r_printf(">>>[test]:pfile->index.sector = %d\n", pfile->index.sector);
    /* recf_seek(pfile, NOR_FS_SEEK_SET, 0); */
    /* recf_seek(pfile, NOR_FS_SEEK_SET, 90); */

    //memset(&file_h,0,sizeof(RECFILEHEAD));
    //recfs_read((u32)pfile->index.sector<<pfile->pfs->sector_size,(u8 *)&file_h,sizeof(RECFILEHEAD));

    recf_write(pfile, (u8 *)&file_h, sizeof(RECFILEHEAD));
    /* put_buf(&file_h , 32); */
    pfile->pfs->index.index = pfile->index.index;
    pfile->pfs->index.sector = pfile->index.sector;

    //memset(&file_h,0,sizeof(RECFILEHEAD));
    //recfs_read((u32)pfile->index.sector<<pfile->pfs->sector_size,(u8 *)&file_h,sizeof(RECFILEHEAD));

    /* r_printf(">>>[test]:file_h.len = %d\n", file_h.len); */
    return file_h.len;
}

void recf_save_sr(REC_FILE *pfile, u16 sr)
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
   @note    u32 revf_index_scan(u32 index,REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 revf_index_scan(u32 index, REC_FILE *pfile)
{
    RECFILEHEAD file_h;
    RECFILESYSTEM *pfs;
    u32 addr;
    u32 total_len;
    u16 curr_sector, crc;

    pfs = pfile->pfs;
    curr_sector = pfs->index.sector;
    total_len = (u32)((u32)pfs->last_sector + 1 - (u32)pfs->first_sector) << pfs->sector_size;
    /* r_printf(">>>[test]:pfile->pfs->index.sector = %d, total_len = %d\n", pfile->pfs->index.sector, total_len); */

    while (1) {
        addr = (u32)curr_sector << pfs->sector_size;
        /* if (offset_addr){ */
        addr += offset_addr;
        /* r_printf(">>>[test]:offset_addr = 0x%x, addr = 0x%x\n", offset_addr, addr); */
        /* } */

        recfs_read(pfs, addr, (u8 *)(&file_h), sizeof(RECFILEHEAD));
        /* put_buf(&file_h , 32); */
        crc = CRC16(&file_h.data_crc, sizeof(RECFILEHEAD) - 2);
        /* y_printf(">>>[test]:crc = 0x%x, file_h.len = %d, file_h.index = %d\n", crc, file_h.len, file_h.index); */
        if ((0 != crc) && (crc == file_h.head_crc) && (file_h.len <= total_len)) {
            if (file_h.index == index) {
                r_printf(">>>[test]:go in AAAAAA\n");
                pfile->index.index = index;
                pfile->index.sector = curr_sector;
                pfile->len = file_h.len;
                pfile->w_len = file_h.len;
                pfile->rw_p = sizeof(RECFILEHEAD);
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
   @note    u32 open_recfile(u32 index,RECFILESYSTEM *pfs,REC_FILE *pfile)
*/
/*----------------------------------------------------------------------------*/
u32 open_recfile(u32 index, RECFILESYSTEM *pfs, REC_FILE *pfile)
{
    memset((u8 *)pfile, 0, sizeof(REC_FILE));
    pfile->pfs = pfs;
    pfile->rw_p = sizeof(RECFILEHEAD);
    return revf_index_scan(index, pfile);
}


/*----------------------------------------------------------------------------*/
/**@brief   初始化文件系统信息
   @param   pfs:文件系统的句柄
   @param   sector_start:起始扇区
   @param   sector_end:结束扇区
   @param   sector_size:扇区大小（1L<<sector_size）
   @return  u32:找到文件的索引
   @author  liujie
   @note    void init_nor_fs(RECFILESYSTEM *pfs,u16 sector_start,u16 sector_end,u8 sector_size)
*/
/*----------------------------------------------------------------------------*/
void init_nor_fs(RECFILESYSTEM *pfs, u16 sector_start, u16 sector_end, u8 sector_size)
{
//    memset((u8 *)pfs,0,sizeof(RECFILESYSTEM));
    pfs->first_sector = sector_start;
    pfs->last_sector = sector_end;
    pfs->sector_size = sector_size;
}


RECFILESYSTEM recfs;
REC_FILE recfile;
#if 1
/* NOR_FS_HDL *nor_fs_hdl; */
static int nor_fs_index = 0;
int max_rec_capacity = 256 * 1024;
int nor_fs_set_rec_capacity(int capacity) //需要先设置容量。
{
    /* ASSERT(capacity <= max_rec_len , "capacity must smaller than max_rec_len!!!!!!! "); */
    max_rec_capacity = capacity;
    return max_rec_capacity;
}
#define NOR_FS_SECTOR_S     (4*1024L)
#define NOR_FS_S_SECTOR     (1)
#define NOR_FS_E_SECTOR     (max_rec_capacity/NOR_FS_SECTOR_S-1)

static void *fd;
#if 0
extern int hook_norflash_open(void);
extern int hook_norflash_spirec_read(u8 *buf, u32 addr, u32 len);
extern int hook_norflash_spirec_write(u8 *buf, u32 addr, u32 len);
extern void hook_norflash_spirec_eraser(u32 addr);
#endif
#if 1
/* static int hook_norflash_open(void) */
/* { */
/*     fd = dev_open("norfs_rec", NULL); */
/*     if (fd == NULL) { */
/*         log_e("error!!!!!!,fd == NULL"); */
/*         return false; */
/*     } */
/*     return true; */
/* } */
static int hook_norflash_spirec_read(u8 *buf, u32 addr, u32 len)
{
    return dev_bulk_read(fd, buf, addr, len);
}

static int hook_norflash_spirec_write(u8 *buf, u32 addr, u32 len)
{
    return dev_bulk_write(fd, buf, addr, len);
}

static void hook_norflash_spirec_eraser(u32 addr)
{
    dev_ioctl(fd, IOCTL_ERASE_SECTOR, addr);
}

static int hook_norflash_get_capacity(void)
{
    int capacity;
    dev_ioctl(fd, IOCTL_GET_CAPACITY, (u32)&capacity);
    return capacity;
}
#endif


static u32 init_recfs(void)
{
    memset(&recfs, 0, sizeof(RECFILESYSTEM));
    recfs.eraser = hook_norflash_spirec_eraser;
    recfs.read  = hook_norflash_spirec_read;
    recfs.write = hook_norflash_spirec_write;

    return true;
}

/* u16 tmp_buf2[512];  ///test buff  */
__BANK_INIT
u32 nor_fs_init(void)
{
    init_recfs();
    /* u16 i = 0; */
    /* memset(tmp_buf2, 0, 256); */
    /* for (i = 0; i< 512; i++) */
    /* { */
    /*     tmp_buf2[i] = i; */
    /* } */
    /* put_buf(tmp_buf2, 64); */
    init_nor_fs(&recfs, NOR_FS_S_SECTOR, NOR_FS_E_SECTOR, 12);
    recfs_scan(&recfs);
    y_printf(">>>[test]:nor_fs_init\n");
    nor_fs_index = recfs.index.index;
    return  recfs.index.index;
}

void nor_rec_get_priv_info(u8 *info)
{
    memcpy(info, &recfile.priv_data, 16);
}

void  nor_rec_set_priv_info(u8 *info) //用于设置数据格式和电话信息.
{
    memcpy(&recfile.priv_data, info, 16);
}

u16 nor_rec_get_sr(void)
{
    return recfile.sr;
}

void nor_rec_set_sr(u16 sr)
{
    recfile.sr = sr;
}

int recfs_scan_ex()
{
    recfs_scan(&recfs);
    return  recfs.index.index;
}


void clear_norfs_fileindex(void)
{
    recfs_idx_clear(&recfs);
}

u32 delete_rec_file(u32 index)
{
    int num;
    num = open_recfile(index, &recfs, &recfile);
    if (num != index) {
        log_e("open error");
        return -1;
    }
    recfs_clear_file(&recfile);
    return 0;
}

u32 nor_get_absolute_addr(void)
{
    return recfile.addr;
}

u32 nor_get_start_addr(void)
{
    return offset_addr;
}

u32 nor_get_capacity(void)
{
    return max_rec_capacity;
}

u32 nor_get_index(void)
{
    return nor_fs_index;
}

int nor_set_offset_addr(int offset)
{
    offset_addr = offset;
    return offset_addr;
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
static int first_flag = 1;
static int nor_fs_open(FILE *_file, const char *path, const char *mode)
{
    int index;
    int reg;
    /* _file->private_data = nor_fs_hdl; */
    /* REC_FILE *recfile = recfile; */
    /* RECFILESYSTEM *recfs = recfs; */
    y_printf(">>>[test]:nor_fs_open\n");

    if (mode[0] == 'w') {
        y_printf(">>>[test]:goto create\n");
        char ext[4] = {0};
        char filename[16] = {0};
        get_ext_name((char *)path, (char *)ext);
        if (memcmp(ext, "WAV", 3) == 0 || memcmp(ext, "wav", 3) == 0) {
            first_flag = 1;
        } else {
            first_flag = 0;
        }

        index = create_recfile(&recfs, &recfile);
        get_filename_by_path((char *)filename, (char *) path);
        nor_rec_set_priv_info((u8 *)filename);
        r_printf(">>>[test]:path = %s, ext = %s, filename = %s\n", path, ext, filename);

        nor_fs_index = index;
        y_printf(">>>[test]: create_recfile  nor_fs_hdl->index= %d\n", nor_fs_index);
        return 0;
    } else if (mode[0] == 'r') {
        y_printf(">>>[test]: open_recfile  nor_fs_hdl->index= %d\n", nor_fs_index);
        if (nor_fs_index) {
            index = nor_fs_index;
        } else {
            log_e("error!!!!!!!!!");
            index = 0;
            return 1;
        }
        reg = open_recfile(index, &recfs, &recfile);
        return 0;
    }
    return 0;
}

int music_flash_file_set_index(u8 file_sel, u32 index)
{
    y_printf(">>>[test]:goto flash_file_play_set_index\n");
    recfs_scan(&recfs);
    switch (file_sel) {
    case  FSEL_NEXT_FILE:
        r_printf(">>>[test]:flash FSEL_NEXT_FILE\n");
        nor_fs_index++;
        if (nor_fs_index > recfs.index.index) {
            log_w("play on the end, goto first play!!!!!!!!!");
            nor_fs_index = 1;
            /* music_flash_file_set_index(FSEL_CURR_FILE, 1); */
        }
        break;
    case  FSEL_PREV_FILE:
        r_printf(">>>[test]:flash FSEL_PREV_FILE\n");
        nor_fs_index--;
        if (nor_fs_index <= 0) {
            log_e("index <= 0 err!!!, play first");
            nor_fs_index = 1;
            /* music_flash_file_set_index(FSEL_CURR_FILE, 1); */
        }
        break;
    case  FSEL_CURR_FILE:
        r_printf(">>>[test]:flash FSEL_CURR_FILE\n");
        if (index <= (recfs.index.index - recfs.total_file)) {
            nor_fs_index = (recfs.index.index - recfs.total_file) + 1;
        } else if (index && (index <= recfs.index.index)) {
            nor_fs_index = index;
        } else {
            log_e("error!!!!!!! no file");
            return 1;
        }
        break;
    }
    if (nor_fs_index <= (recfs.index.index - recfs.total_file)) {
        nor_fs_index = (recfs.index.index - recfs.total_file) + 1;
    }
    return 0;
}

static int nor_fs_write(FILE *_file, void *buff, u32 len)
{
    u16 w_len;
    /* REC_FILE *recfile = nor_fs_hdl->recfile; */
    /* r_printf(">>>[test]:goto nor_fs write\n"); */
    /* if (len != 90){ */
    /* void *tmp_buf = malloc(512); */
    /* memset(tmp_buf, 0xff, 512); */
    /* memcpy(tmp_buf, tmp_buf2, 512); */
    /* #<{(| memcpy(tmp_buf, buff, len); |)}># */
    /* w_len = recf_write(&recfile, tmp_buf, len); */
    /* put_buf(tmp_buf, len); */
    /* free(tmp_buf); */
    /* return w_len; */
    /* } */
    /* r_printf(">>>[test]:goto nor_fs write\n"); */
    if (first_flag == 1) {
        memset(buff, 0xff, 90);
        first_flag = 0;
    }
    w_len = recf_write(&recfile, buff, len);
    /* put_buf(buff, 32); */
    return w_len;
}

static int nor_fs_read(FILE *_file, void *buff, u32 len)
{
    u16 r_len;


    /* r_printf(">>>[test]:goto nor_fs read\n"); */
    if (((u32)buff % 4) != 0) {
        void *tmp_buf = malloc(512);
        r_len = recf_read(&recfile, tmp_buf, len);
        memcpy(buff, tmp_buf, len);
        free(tmp_buf);
        /* put_buf(buff, 32); */
        return r_len;
    }
    r_len = recf_read(&recfile, buff, len);
    /* put_buf(buff, 32); */
    return r_len;
}

static int nor_fs_close(FILE *_file)
{
    int reg;
    r_printf(">>>[test]:goto nor_fs close\n");
    reg =  close_recfile(&recfile);
    return reg;
}

static int nor_fs_seek(FILE *_file, int offsize, int type)
{
    int reg;
    /* y_printf(">>>[test]:goto nor_fs seek\n"); */
    reg = recf_seek(&recfile, (u8) type, offsize);
    return reg;
}

static int nor_fs_mount(struct imount *mt, int cache_num)
{
    y_printf("\n >>>[test]:func = %s,line= %d\n", __FUNCTION__, __LINE__);
    struct vfs_partition *app_part = &mt->part;
    fd = mt->dev.fd;
    /* hook_norflash_open(); */
    max_rec_capacity = hook_norflash_get_capacity();
    r_printf(">>>[test]:capacity = 0x%x , offset = %d\n", max_rec_capacity, offset_addr);
    nor_fs_init();

    app_part->offset = 0 + offset_addr;
    app_part->dir[0] = 'C';
    app_part->dir[1] = '\0';

    mt->part_num = 1;

    return 0;

}

static int nor_fs_flen(FILE *_file)
{
    RECFILEHEAD file_h;
    y_printf(">>>[test]:goto nor_fs_flen\n");
    r_printf(">>>[test]:pfile->len = %d\n", recfile.len);
    return recfile.len;
}

static int nor_fs_fpos(FILE *_file)
{
    RECFILEHEAD file_h;
    /* y_printf(">>>[test]:goto nor_fs_fpos\n"); */
    /* r_printf(">>>[test]:pfile->fpos(rw_p) = %d\n", recfile.rw_p); */
    return recfile.rw_p;
}

REGISTER_VFS_OPERATIONS(nor_fs_vfs_ops) = {
    .fs_type         = "nor_fs",
    .mount           = nor_fs_mount,
    .unmount	     = NULL,
    .format          = NULL,
    .fopen	         = nor_fs_open,
    .fwrite          = nor_fs_write,
    .fread        	 = nor_fs_read,
    .fseek 		     = nor_fs_seek,
    .flen 		     = nor_fs_flen,
    .fpos 		     = nor_fs_fpos,
    .fclose          = nor_fs_close,
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

int nor_fs_ops_init(void)
{
    return 0;
}

#endif
#endif


#if 0
/* RECFILESYSTEM recfs; */
/* REC_FILE recfile; */
/* extern  const struct norflash_dev_platform_data *norflash_dev_data; */
/* #define NOR_FS_SECTOR_S     (4*1024L) */
/* #define NOR_FS_S_SECTOR     (1) */
/* #define NOR_FS_E_SECTOR     (255) //(1*1024L*1024L/NOR_FS_SECTOR_S-1) */

/* extern int _norflash_init(struct norflash_dev_platform_data *pdata); */
/* extern int hook_norflash_open(void); */
/* extern int hook_norflash_spirec_read(u8 *buf, u32 addr, u32 len); */
/* extern int hook_norflash_spirec_write(u8 *buf, u32 addr, u32 len); */
/* extern void hook_norflash_spirec_eraser(u32 addr); */
/* static u32 init_recfs(void) */
/* { */
/*     memset(&recfs, 0, sizeof(RECFILESYSTEM)); */
/*     recfs.eraser = hook_norflash_spirec_eraser; */
/*     recfs.read  = hook_norflash_spirec_read; */
/*     recfs.write = hook_norflash_spirec_write; */
/*  */
/*     #<{(| init_nor_fs(&recfs, NOR_FS_S_SECTOR, NOR_FS_E_SECTOR, 12); |)}># */
/*     #<{(| recfs_scan(&recfs); |)}># */
/*     #<{(| return  recfs.index.index; |)}># */
/*     _norflash_init(&norflash_dev_data); */
/*     return true; */
/* } */

#define N 512
#define M 512
u8 test_buff[N];
void test_nor_fs(void)
{
    u16 i;
    u32 index;
    u32 filenum;
    r_printf(">>>[test]:gogogogoogogoogo\n");
    /* filenum = init_recfs(); */
    /* hook_norflash_open(); */
    /* r_printf(">>>[test]:index=filenum is ok? = %d\n", filenum); */
    /* init_nor_fs(&recfs, NOR_FS_S_SECTOR, NOR_FS_E_SECTOR, 12); */
    /* r_printf("recfs_scan\n"); */
    /* recfs_scan(&recfs); */
    r_printf("recfs.index.sector= %d\n", recfs.index.sector);
    r_printf("pfs.index.sector\n");
    r_printf("create_recfile\n");

    index = create_recfile(&recfs, &recfile);


    r_printf("index = %d\n", index);
    r_printf("index\n");
    r_printf("recfs.index.sector = %d\n", recfs.index.sector);
    r_printf("recfile.index.sector = %d\n", recfile.index.sector);
    r_printf("pfs.index.sector\n");
    /* for (i = 0; i < N; i++) { */
    /*     test_buff[i] = i; */
    /* } */

    void *test_buf = malloc(N);
    for (i = 1; i < 20; i++) {
        /* for (int j=0; j< N; j++) */
        /* { */
        /*     test_buf[j] = i; */
        /*     #<{(| test_buf += j; |)}># */
        /*     #<{(| test_buf = &i; |)}># */
        /* } */
        memset(test_buf, i, N);
        y_printf(">>>[test]:test_buf_len = %d\n", strlen(test_buf));
        recf_write(&recfile, test_buf, N);
    }
    y_printf("recf_write\n");
    /* recf_write(&recfile, test_buff, N); */

    /* y_printf(">>>[test]:1111recf_read\n"); */
    /* recf_read(&recfile, test_buff, N); */
    /* put_buf(test_buff, N); */

    y_printf("close_recfile\n");
    close_recfile(&recfile);


    y_printf("recfile.len = %d\n", recfile.len);
    y_printf("recfile.index.index =%d\n", recfile.index.index);
    y_printf("recfile.index.sector = %d\n", recfile.index.sector);
    y_printf("recfs.index.sector = %d\n", recfs.index.sector);
    /* y_printf("pfs.index.sector\n"); */

    index = open_recfile(index, &recfs, &recfile);
    /* memset(test_buff, 0, sizeof(test_buff)); */
    r_printf(">>>[test]:recf_read\n");

    void *test_buf2 = malloc(M);
    for (i = 1; i < 20; i++) {
        memset(test_buf, 0x0, N);
        memset(test_buf2, 0x0, M);
        recf_read(&recfile, test_buf2, M);
        put_buf(test_buf2, M);
        printf("\n\n");
    }
    /* recf_read(&recfile, test_buff, N); */
    /* put_buf(test_buff, N); */
    free(test_buf);
    free(test_buf2);

    while (1) {
        wdt_clear();
    };
}

#endif

