#include "fm_rw.h"
#include "syscfg/syscfg_id.h"

#if CONFIG_FM_DEV_ENABLE


/*----------------------------------------------------------------------------*/
/*@brief   获取一个byte中有几个位被置一
  @param   byte ：所传进去的byte
  @return  被置一位数
  @note    u8 get_total_mem_channel(void)
 */
/*----------------------------------------------------------------------------*/
static inline u8 __my_get_one_count(u8 byte)
{
    u8 count = 0;
    while (byte) {
        ++count;
        byte &= (byte - 1);
    }
    return count;
}

/*----------------------------------------------------------------------------*/
/**@brief 获取全部记录的频道
  @param 	无
  @return  频道总数
  @note  u8 get_total_mem_channel(void)
 */
/*----------------------------------------------------------------------------*/
u16 get_total_mem_channel(void)
{
    FM_VM_INFO fm_info = {0};
    fm_read_info(&fm_info);
    return fm_info.total_chanel;
}

/*----------------------------------------------------------------------------*/
/**@brief 根据频点偏移量获取频道
  @param 	channel：频道
  @return  频道
  @note  u8 get_channel_via_fre(u8 fre)

 */
/*----------------------------------------------------------------------------*/
u16 get_channel_via_fre(u16 fre)
{
    FM_VM_INFO fm_info = {0};
    u16 i, k;
    u16 total = 0;
    fre = VIRTUAL_FREQ(fre);//fre - 874;
    fm_read_info(&fm_info);
    for (i = 0; i < (MEM_FM_LEN); i++) {
        for (k = 0; k < 8; k++) {
            if (fm_info.dat[i] & (BIT(k))) {
                total++;
                if (fre == (i * 8 + k)) {
                    return total;		 //return fre index
                }
            }
        }
    }
    return 0x00;//fm_mode_var->wFreChannel;						    //find none
}

/*----------------------------------------------------------------------------*/
/**@brief 通过频道获取频点
  @param 	channel：频道
  @return  有效的频点偏移量
  @note  u8 get_fre_via_channle(u8 channel)
 */
/*----------------------------------------------------------------------------*/
u16 get_fre_via_channel(u16 channel)
{
    FM_VM_INFO fm_info = {0};
    u16 i, k;
    u16 total = 0;

    fm_read_info(&fm_info);

    for (i = 0; i < (MEM_FM_LEN); i++) {
        for (k = 0; k < 8; k++) {
            if (fm_info.dat[i] & (BIT(k))) {
                total++;
                if (total == channel) {
                    return i * 8 + k;		 //fre = MIN_FRE + return val
                }
            }
        }
    }

    return 0;							//find none
}

/*----------------------------------------------------------------------------*/
/**@brief 从vm清除所有频点信息
  @param 	无
  @return  无
  @note  void clear_all_fm_point(void)
 */
/*----------------------------------------------------------------------------*/
void clear_all_fm_point(void)
{
    FM_VM_INFO fm_info = {0};
    fm_info.mask = FM_VM_MASK;
    fm_info.curFreq = 1;
    fm_info.curChanel = 0;
    fm_info.total_chanel = 0;
    syscfg_write(VM_FM_INFO_INDEX, (char *)&fm_info, sizeof(FM_VM_INFO));
}

/*----------------------------------------------------------------------------*/
/**@brief 根据频点偏移量保存到相应的频点位变量到vm
  @param 	fre：频点偏移量
  @return  无
  @note  void save_fm_point(u8 fre)
 */
/*----------------------------------------------------------------------------*/
void save_fm_point(u16 fre)//1-206
{
    FM_VM_INFO fm_info = {0};
    u16 i, k;
    u16 total = 0;
    fm_read_info(&fm_info);
    fre = VIRTUAL_FREQ(fre);//fre - 874;
    /* fre = fre - 874; */
    i = fre / 8;
    k = fre % 8;

    fm_info.dat[i] |= BIT(k);
    fm_info.curFreq = fre;

    for (i = 0; i < (MEM_FM_LEN); i++) {
        total += __my_get_one_count(fm_info.dat[i]);
    }

    total = total > MAX_CHANNEL ? MAX_CHANNEL : total;
    fm_info.total_chanel = total;

    fm_save_info(&fm_info);
}

/*----------------------------------------------------------------------------*/
/**@brief 删除频道
  @param 	无
  @return  无
  @note  void delete_fm_point(u8 fre)
 */
/*----------------------------------------------------------------------------*/
void delete_fm_point(u16 fre)
{
    u16 i, k;
    FM_VM_INFO fm_info = {0};
    fm_read_info(&fm_info);
    i = fre / 8;
    k = fre % 8;
    if (fm_info.dat[i] & BIT(k)) {
        fm_info.dat[i] &= (~BIT(k));
        fm_info.total_chanel--;
        fm_save_info(&fm_info);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief 保存频道
  @param 	无
  @return  无
  @note  u8 ch_save(void)
 */
/*----------------------------------------------------------------------------*/
void fm_last_ch_save(u16 channel)
{
    FM_VM_INFO fm_info = {0};
    fm_read_info(&fm_info);
    fm_info.curFreq = 0;
    fm_info.curChanel = channel;
    syscfg_write(VM_FM_INFO_INDEX, (char *)&fm_info, sizeof(FM_VM_INFO));
}

void fm_last_freq_save(u16 freq)
{
    FM_VM_INFO fm_info = {0};
    fm_read_info(&fm_info);
    fm_info.curChanel = 0;
    freq = VIRTUAL_FREQ(freq);//fre - 874;
    fm_info.curFreq = freq;// - 874;
    syscfg_write(VM_FM_INFO_INDEX, (char *)&fm_info, sizeof(FM_VM_INFO));
}

/*----------------------------------------------------------------------------*/
/**@brief  保存信息到fm_buf
  @param  无
  @return 无
  @note  void fm_save_info()
 */
/*----------------------------------------------------------------------------*/
void fm_save_info(FM_VM_INFO *info)
{
    syscfg_write(VM_FM_INFO_INDEX, (char *)info, sizeof(FM_VM_INFO));
}

/*----------------------------------------------------------------------------*/
/**@brief  从vm读取信息
  @param  无
  @return 无
  @note  void fm_read_info()
 */
/*----------------------------------------------------------------------------*/
void fm_read_info(FM_VM_INFO *info)
{
    if (sizeof(FM_VM_INFO) != syscfg_read(VM_FM_INFO_INDEX, (char *)info, sizeof(FM_VM_INFO))) {
        printf("fm_info is null \n");
        memset(info, 0x00, sizeof(FM_VM_INFO));
    }
}

/*----------------------------------------------------------------------------*/
/**@brief  检查vm信息
  @param  无
  @return 无
  @note  void fm_read_info()
 */
/*----------------------------------------------------------------------------*/
void fm_vm_check(void)
{
    FM_VM_INFO fm_info = {0};
    syscfg_read(VM_FM_INFO_INDEX, (char *)&fm_info, sizeof(FM_VM_INFO));
    if (fm_info.mask != FM_VM_MASK) {
        memset(&fm_info, 0, sizeof(FM_VM_INFO));
        fm_info.mask = FM_VM_MASK;
        fm_info.curFreq = 1;
        fm_info.curChanel = 0;
        fm_info.total_chanel = 0;
        syscfg_write(VM_FM_INFO_INDEX, (char *)&fm_info, sizeof(FM_VM_INFO));
    }
}

#endif

