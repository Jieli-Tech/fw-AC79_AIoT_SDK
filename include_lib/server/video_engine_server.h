#ifndef __VIDEO_ENGINE_SERVER_H__
#define __VIDEO_ENGINE_SERVER_H__

#include "typedef.h"
#include "list.h"


typedef enum VE_MODULE_TYPE {
    VE_MODULE_MOTION_DETECT,
    VE_MODULE_LANE_DETECT,
    VE_MODULE_CUSTOM,
    VE_MODULE_MAX,
} ve_module_type_t;

#define VE_MODULE_MOTION_DETECT_MASK    (1<<VE_MODULE_MOTION_DETECT)
#define VE_MODULE_LANE_DETECT_MASK      (1<<VE_MODULE_LANE_DETECT)


typedef enum {
    VE_REQ_MODULE_OPEN,
    VE_REQ_MODULE_CLOSE,
    VE_REQ_MODULE_START,
    VE_REQ_MODULE_STOP,
    VE_REQ_MODULE_SET_PARAM,
    VE_REQ_MODULE_GET_PARAM,
    VE_REQ_MODULE_IOCTL,
    VE_REQ_MODULE_GET_STATUS,
    VE_REQ_GET_STATUS,
    VE_REQ_GET_CAPABILITY,
    VE_REQ_SET_HINT,
} ve_req_type_t;

typedef enum {
    VE_MSG_MOTION_DETECT_STILL = 1,
    VE_MSG_MOTION_DETECT_MOVING,
    VE_MSG_LANE_DETECT_WARNING,
    VE_MSG_VEHICLE_DETECT_WARNING,
    VE_MSG_LANE_DETCET_LEFT,
    VE_MSG_LANE_DETCET_RIGHT,
    VE_MSG_CUSTOM_MODULE,
} ve_msg_type_t;


typedef enum {
    VE_STATUS_IDLE,
    VE_STATUS_STOP,
    VE_STATUS_RUNNING,
    VE_STATUS_ERROR,
} ve_status_t;

typedef enum {
    VE_MOTION_DETECT_MODE_NORMAL,
    VE_MOTION_DETECT_MODE_ISP,
} ve_motion_detect_mode_t;

typedef struct {
    u32 hint;
    u32 mode_hint0;
    u32 mode_hint1;
} ve_hint_info_t;

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} ve_roi_t;

typedef struct {
    u8          level;          //0-4
    u32         move_delay_ms;  //ms
    u32         still_delay_ms; //ms
    ve_roi_t    roi;            //Range Of Interest.
    //defalut:x=0,y=0,w=640,h=480, must 8 bytes aligned
} ve_motion_detect_params_t;

typedef struct {
    u16 car_head_y;
    u16 vanish_y;
    u16 len_factor;
} ve_lane_detect_params_t;

typedef struct video_engine_req {
    u8 module;
    u32 cmd;
    union {
        u8 param[0];
        ve_motion_detect_mode_t md_mode;
        ve_motion_detect_params_t md_params;
        ve_lane_detect_params_t lane_detect_params;
        ve_hint_info_t   hint_info;
        ve_status_t status;
        u32 capabilities;
    };
} video_engine_req_t;

//vehicle detect open or not
//flag = 1: vehicle detect open
//flag = 0: vehicle detect close
extern void vehicle_detect_open_close(int flag);

extern void set_lane_params(int ANGLE_LOW, int ANGLE_HIGH, int LANE_DISTANCE, int RATIO_HW);

/*custom module api*/
typedef struct ve_custom_module_api {
    void *(*init)(int w, int h);
    int (*free)(void *h);
    int (*process)(void *h, unsigned char *inputFrame, int width, int height);
} ve_custom_module_api_t;


int ve_custom_module_register(ve_custom_module_api_t *api);

#endif

