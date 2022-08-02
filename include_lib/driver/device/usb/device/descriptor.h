#ifndef  __DESCRIPTOR_H__
#define  __DESCRIPTOR_H__

#include "asm/usb.h"

void get_device_descriptor(u8 *ptr);
void get_language_str(u8 *ptr);
void get_manufacture_str(u8 *ptr);
void get_product_str(u8 *ptr);
void get_iserialnumber_str(u8 *ptr);
void get_string_ee(u8 *ptr);

u32 set_descriptor(const usb_dev usb_id, u32 class_config, u8 *p, u32 max_len);


#endif  /*DESCRIPTOR_H*/
