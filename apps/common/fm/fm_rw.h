#ifndef _FM_RW_H_
#define _FM_RW_H_

#include "fm_manage.h"

#define MEM_FM_LEN  ((MAX_CHANNEL+7)/8)

#define FM_VM_MASK  (0x1234+MAX_CHANNEL)//自动mask

#pragma pack(1)//不平台对齐编译
typedef struct _FM_VM_INFO_ {
    u16 mask;
    u16 curFreq;//x-874
    u16 curChanel;//1~206
    u16 total_chanel;
    u8 dat[MEM_FM_LEN];
} FM_VM_INFO;
#pragma pack()


u16 get_total_mem_channel(void);//获取记忆有效台数
u16 get_channel_via_fre(u16 fre);//输入真实频率
u16 get_fre_via_channel(u16 channel);
void clear_all_fm_point(void);
void save_fm_point(u16 fre);//输入真实频率
void delete_fm_point(u16 fre);//输入虚拟频率
void fm_read_info(FM_VM_INFO *info);
void fm_save_info(FM_VM_INFO *info);
void fm_last_ch_save(u16 channel);
void fm_last_freq_save(u16 freq);//输入真实频率
void fm_vm_check(void);

#endif

