#ifndef VT_CAMERA_H
#define VT_CAMERA_H

#include "vt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

const camera_ops_t *get_linux_camera(void);

#ifdef __cplusplus
}
#endif
#endif // VT_CAMERA_H

