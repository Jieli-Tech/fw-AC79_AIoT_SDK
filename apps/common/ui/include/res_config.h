#ifndef __RES_CONFIG_H__
#define __RES_CONFIG_H__

#include "app_config.h"

#define EXTERN_PATH "storage/virfat_flash/C/"
#define INTERN_PATH "mnt/sdfile/res/ui_res/"

#if TCFG_USE_SD_ADD_UI_FILE
#define RES_PATH   CONFIG_ROOT_PATH"ui_res/"///EXTERN_PATH
#else     //USE_FLASH_ADD_UI_FILE
#define RES_PATH   INTERN_PATH//EXTERN_PATH
#endif

#define FONT_PATH   RES_PATH

#define UPGRADE_PATH   INTERN_PATH"ui_upgrade/"



// #define UI_STY_CHECK_PATH     \
//     RES_PATH"JL/JL.sty",      \
//     RES_PATH"watch/watch.sty",\
//     RES_PATH"watch1/watch1.sty",\
//     RES_PATH"watch2/watch2.sty",\
//     RES_PATH"watch3/watch3.sty",\
//     RES_PATH"watch4/watch4.sty",\
//     RES_PATH"watch5/watch5.sty",
//
// #define UI_RES_CHECK_PATH  \
//     RES_PATH"JL/JL.res",   \
//     RES_PATH"watch/watch.res",\
//     RES_PATH"watch1/watch1.res",\
//     RES_PATH"watch2/watch2.res",\
//     RES_PATH"watch3/watch3.res",\
//     RES_PATH"watch4/watch4.res",\
//     RES_PATH"watch5/watch5.res",\
//
// #define UI_STR_CHECK_PATH      \
//     RES_PATH"JL/JL.str",       \
//     RES_PATH"watch/watch.str", \
//     RES_PATH"watch1/watch1.str",\
//     RES_PATH"watch2/watch2.str",\
//     RES_PATH"watch3/watch3.str",\
//     RES_PATH"watch4/watch4.str",\
//     RES_PATH"watch5/watch5.str",
//
//
// #define UI_STY_WATCH_PATH     \
//     RES_PATH"watch/watch.sty",\
//     RES_PATH"watch1/watch1.sty",\
//     RES_PATH"watch2/watch2.sty",\
//     RES_PATH"watch3/watch3.sty",\
//     RES_PATH"watch4/watch4.sty",\
//     RES_PATH"watch5/watch5.sty",


// #define UI_USED_DOUBLE_BUFFER   1//使用双buf推屏
// #define UI_WATCH_RES_ENABLE     1//表盘功能
// #define UI_UPGRADE_RES_ENABLE   1//升级界面功能



#endif
