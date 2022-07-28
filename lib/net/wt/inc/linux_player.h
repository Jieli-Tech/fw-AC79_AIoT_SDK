#ifndef VT_PLAYER_H
#define VT_PLAYER_H

#include "vt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

const media_player_ops_t *get_linux_player(void);

#ifdef __cplusplus
}
#endif
#endif // VT_PLAYER_H

