/**
 * @file lv_port_indev.c
 *
 */
#include "system/includes.h"
#include "lvgl.h"


static struct touch_hdl {
    u16 x;
    u16 y;
    u8 status;
};
static struct touch_hdl *hdl = NULL;

static bool touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);


extern void *get_touch_x_y(void);

void lv_port_indev_init(void)
{
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);
    hdl = get_touch_x_y();
}

extern u16 touch_lvgl_x;
extern u16 touch_lvgl_y;
extern u8 touch_lvgl_staus;


static bool touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
//		printf("<<<<<<<<<<<<touch_lvgl_staus = %d , x = %d, y = %d",touch_lvgl_staus , touch_lvgl_x , touch_lvgl_y);
    data->point.x = hdl->x;
    data->point.y = hdl->y;
    if (hdl->status) { //触摸按下了
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
    return false;
}





