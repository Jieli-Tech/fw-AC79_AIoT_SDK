#ifndef CODE_MOVABLE_H
#define CODE_MOVABLE_H

#include "generic/typedef.h"

void code_movable_init();

void code_movable_load(u8 *code_store_addr, u32 code_size, u8 *code_load_addr,
                       u32 *code_slot_addr, u32 *code_slot_end, u32 **start_of_region);

void code_movable_unload(u8 *code_store_addr, u32 *code_slot_addr, u32 *code_slot_end, u32 **start_of_region);

#endif


