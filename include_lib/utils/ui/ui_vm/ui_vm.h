#ifndef __UI_F_VM_H__
#define __UI_F_VM_H__


extern int flash_message_init(const char *name, int max);

extern void flash_message_release();

extern int flash_message_write(u8 *data, u16 len);

extern int flash_message_read_by_index(u8 index, u8 *data, u16 len);

extern int flash_message_delete_by_mask(u32 mask);

extern int flash_message_count();

extern int flash_weather_init(const char *name);

extern int flash_weather_read(u8 *data, u16 len);

extern int flash_weather_write(u8 *data, u16 len);



















#endif
