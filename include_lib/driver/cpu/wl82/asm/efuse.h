#ifndef  __EFUSE_H__
#define  __EFUSE_H__


u16 get_chip_id();
u16 get_vbat_trim();
u8 get_vbg_trim();
/* struct lrc_config_t {                */
/*     u16 lrc_ws_inc;			//from uboot   */
/*     u16 lrc_ws_init;		//from uboot   */
/*     u16 btosc_ws_inc;		//from uboot  */
/*     u16 btosc_ws_init;		//from uboot */
/*     u8 lrc_change_mode;	//from uboot */
/* };                                   */
u16 get_lrc_ws_inc();			//from uboot
u16 get_lrc_ws_init();		//from uboot
u16 get_btosc_ws_inc();		//from uboot
u16 get_btosc_ws_init();		//from uboot
u8 get_lrc_change_mode();	//from uboot

u32 get_boot_flag();
void set_boot_flag(u32 flag);
#endif  /*EFUSE_H*/
