#ifndef _BT_EMITTER_H
#define _BT_EMITTER_H

void emitter_or_receiver_switch(u8 flag);
void bt_search_device(void);
void emitter_search_noname(u8 status, u8 *addr, char *name);
u8 bt_emitter_pp(u8 pp);
u8 bt_emitter_stu_set(u8 on);
u8 bt_emitter_stu_sw(void);
u8 bt_emitter_stu_get(void);
u8 bt_emitter_role_get(void);
void emitter_media_source(u8 source, u8 en);
u8 emitter_search_result(char *name, u8 name_len, u8 *addr, u32 dev_class, char rssi);
void emitter_search_stop(u8 result);
int bt_emitter_page_timeout(void);
int bt_emitter_disconnect(void);
void *get_bt_emitter_audio_server(void);

#endif
