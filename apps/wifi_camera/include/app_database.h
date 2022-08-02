#ifndef APP_DATABASE_H
#define APP_DATABASE_H



/*
 * 视频分辨率
 */
#define     VIDEO_RES_1080P         0
#define     VIDEO_RES_720P          1
#define     VIDEO_RES_VGA           2


/*
 * 重力感应灵敏度
 */
#define     GRA_SEN_OFF             0
#define     GRA_SEN_LO              1
#define     GRA_SEN_MD              2
#define     GRA_SEN_HI              3


/*
 * 电视制式
 */
#define     TVM_PAL                 0
#define     TVM_NTSC                1


/*
 * 拍照分辨率
 */
#define     PHOTO_RES_VGA           0       /* 640*480 */
#define     PHOTO_RES_1M            1       /* 1280*720 */
#define     PHOTO_RES_2M            2       /* 1920*1080 */
#define     PHOTO_RES_3M            3       /* 2048*1536 */
#define     PHOTO_RES_5M            5       /* 2592*1944 */
#define     PHOTO_RES_8M            8       /* 3264*2448 */
#define     PHOTO_RES_10M           10      /* 3648*2736 */
#define     PHOTO_RES_12M           12      /* 4032*3024 */


/*
 * 图片质量
 */
#define     PHOTO_QUA_LO            0
#define     PHOTO_QUA_MD            1
#define     PHOTO_QUA_HI            2

/*
 * 图像锐度
 */
#define     PHOTO_ACU_LO            0
#define     PHOTO_ACU_MD            1
#define     PHOTO_ACU_HI            2

/*
 * 图像白平衡
 */
#define     PHOTO_WBL_AUTO          0       /* 自动 */
#define     PHOTO_WBL_SUN           1       /* 日光 */
#define     PHOTO_WBL_CLDY          2       /* 阴天 */
#define     PHOTO_WBL_TSL           3       /* 钨丝灯 */
#define     PHOTO_WBL_FLUL          4       /* 荧光灯 */

/*
 * 图像色彩
 */
#define     PHOTO_COLOR_NORMAL      0
#define     PHOTO_COLOR_WB          1
#define     PHOTO_COLOR_OLD         2



struct app_cfg {
    const char *table;
    int (*set)(u32 exdata);
};




int app_set_config(struct intent *it, const struct app_cfg *cfg, int size);
int db_select(const char *name);
int db_update(const char *name, u32 value);










#endif
