

#ifndef _JPEG_UTILS_H_
#define _JPEG_UTILS_H_

#include "mediainfo.h"


int search_avih_code(media_entry *me);
int search_movi_code(void *fd);
int search_vframe_code(void *fd);
int search_aframe_code(void *fd);

#endif

