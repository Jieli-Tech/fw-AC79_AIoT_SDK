#ifndef __JIELI_OTA_H
#define __JIELI_OTA_H

#include "typedef.h"

int is_need_ota_update(const char *code, const char *platform, u32 major, u32 minor, u32 patch, const char *access_token);

int get_update_url(char *buffer, u32 len, const char *code, const char *platform, const char *access_token);

int get_update_data(const char *url);

int ota_update(const char *code, const char *platform, u32 major, u32 minjor, u32 patch);
/* ota_update("wifi_story","device",0,0,0); */

#endif /* __JIELI_OTA_H */


