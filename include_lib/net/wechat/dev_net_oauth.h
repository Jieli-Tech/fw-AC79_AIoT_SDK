
#ifndef __DEV_NET_OAUTH_H
#define __DEV_NET_OAUTH_H

#include "typedef.h"



// int app_chat_send_voice(char *filename);
int app_chat_send_voice_buffer(char *buffer, u32 len);

char *dev_net_get_access_token(void);

// if type==NULL
//     no voice
typedef void (*jl_chat_voice)(char *type, char *buf);
int app_chat_voice_set_read(void);
int app_chat_get_new_voice(jl_chat_voice cb);
int app_chat_get_one_voice(jl_chat_voice cb);
int app_chat_get_next_voice(jl_chat_voice cb);
int app_chat_get_prev_voice(jl_chat_voice cb);

int app_get_music_url(const char *str, u32 album_id, u32 meta_sn, u8 *data, u32 data_len);

//mqtt登录上，才能使用
int ota_update(const char *code, const char *platform, u32 major, u32 minjor, u32 patch);

#endif

