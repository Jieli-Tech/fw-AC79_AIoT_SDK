#ifndef _DUI_NET_H_
#define _DUI_NET_H_
#include "iot.h"

extern int dui_net_thread_run(void *priv);
extern void dui_net_thread_kill(void);
extern u8 get_reopen_record();
extern void set_reopen_record(u8 value);
extern int get_play_list_match(void);
extern void set_play_list_match(int value);
extern int get_play_mode(void);
extern int get_play_list_max(void);
extern void dui_play_playlist_for_match(media_item_t *info, int  match);

#endif
