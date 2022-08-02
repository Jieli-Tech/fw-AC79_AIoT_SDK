#ifndef _VM_H_
#define _VM_H_

#include "ioctl.h"
#ifdef CONFIG_CHRDEV
#include "device/device.h"
#endif
#include "syscfg/syscfg_id.h"

typedef u16 vm_hdl;

/*格式化VM_区域*/
extern CFG_ERR vm_eraser(void);

/*
每次上电初始化VM系统,
need_defrag_when_write_full: 当vm区域写满时是否需要整理碎片
*/
extern CFG_ERR vm_init(void *dev_hdl, u32 vm_addr, u32 vm_len, u8 vm_mode);

/******
**增加vm 索引务必要递增
**变长度读写,如果结构体成员顺序变换/成员增删等原因,有一定风险,请自行评估.
*/

//若vm数据区没有空间存储,则返回VM_WRITE_OVERFLOW, 其他情况返回VM_ERR_NONE
extern int vm_write(vm_hdl hdl, u8 *data_buf, u16 len);

//若vm内部存储数据长度大于 参数data_len, 则读出data_len长度的数据并 返回 data_len;
//若vm内部存储数据长度小于 参数data_len, 则读出 vm内部存储数据长度 的数据并 返回 vm内部存储数据长度;
//若vm内部存储数据长度与参数data_len长度相符, 则返回VM_ERR_NONE
//若vm内部从没有存储过 hdl代表的 索引, 则返回 VM_READ_NO_INDEX
extern int vm_read(vm_hdl hdl, u8 *data_buf, u16 len);

/*手动整理碎片*/
extern CFG_ERR vm_defrag(void);

/*侦查 vm 区域 使用情况, 返回使用的百分比*/
/*extern CFG_ERR vm_status(VM_STRUCT *vm_str_p);*/

extern const struct DEV_IO *dev_reg_vm(void *parm);

bool sfc_erase_zone(u32 addr, u32 len);

#endif  //_VM_H_




