#ifndef _BT_EMITTER_H
#define _BT_EMITTER_H

extern void emitter_or_receiver_switch(u8 flag);
extern void bt_search_device(void);
extern void emitter_search_noname(u8 status, u8 *addr, char *name);
extern u8 bt_emitter_pp(u8 pp);
extern u8 bt_emitter_stu_set(u8 on);
extern u8 bt_emitter_stu_sw(void);
extern u8 bt_emitter_stu_get(void);
extern void emitter_media_source(u8 source, u8 en);
extern u8 emitter_search_result(char *name, u8 name_len, u8 *addr, u32 dev_class, char rssi);
extern void emitter_search_stop(u8 result);
extern int bt_emitter_enc_gain_set(int step);

#endif
