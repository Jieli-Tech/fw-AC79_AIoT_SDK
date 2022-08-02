/*****************************************************************
>file name : include_lib/server/upgrade_server.h
>author : lichao
>create time : Wed 19 Sep 2018 03:52:09 PM CST
*****************************************************************/
#ifndef _UPGRADE_SERVER_H_
#define _UPGRADE_SERVER_H_

#include "server/server_core.h"
#include "system/includes.h"
#include "fs/fs.h"

#define UPGRADE_TYPE_FILE       0x0
#define UPGRADE_TYPE_BUF        0x1

enum system_upgrade_err {
    SYS_UPGRADE_ERR_NONE = 0x0,
    SYS_UPGRADE_ERR_TYPE,
    SYS_UPGRADE_ERR_MODE,
    SYS_UPGRADE_ERR_FILE_ERR,
    SYS_UPGRADE_ERR_NO_FILE,
    SYS_UPGRADE_ERR_DATA_OFFSET,
    SYS_UPGRADE_ERR_CHECK_NO_MEM,
    SYS_UPGRADE_ERR_KEY,
    SYS_UPGRADE_ERR_FLASH_SPACE,
    SYS_UPGRADE_ERR_SYSTEM_FILE,
    SYS_UPGRADE_ERR_SAME,
    SYS_UPGRADE_ERR_ADDRESS,
    SYS_UPGRADE_ERR_BAK_FILE,
    SYS_UPGRADE_ERR_DATA_LARGE,
};

/*
 *请求列表
 */
enum {
    UPGRADE_REQ_CHECK_FILE,
    UPGRADE_REQ_LOAD_UI,
    UPGRADE_REQ_CHECK_SYSTEM,
    UPGRADE_REQ_CORE_START,
    UPGRADE_REQ_CORE_STOP,
};

enum upgrade_event {
    UPGRADE_EVENT_PERCENT,
    UPGRADE_EVENT_COMPLETE,
    UPGRADE_EVENT_FAILED,
};

struct upgrade_data {
    u8 *buf;
    u32 size;
};

struct upgrade_ui {
    u8 type;
    union {
        struct upgrade_data data;
        FILE   *file;
    } input;
    int (*show_progress)(int percent);
    int (*show_message)(int msg);
    const char *path;
};

struct upgrade_info {
    u8 type;
    union {
        struct upgrade_data data;
        FILE  *file;
    } input;
    u32 offset;//input数据或文件在整个升级数据的偏移
};

struct upgrade_core {
    u8 type;
    union {
        struct upgrade_data data;
        FILE *file;
    } input;
    u32 offset;
};

struct sys_upgrade_param {
    u8 *buf;//设置升级使用的buffer ***长度必须4K对齐方便升级和备份***
    u32 buf_size; //设置buffer长度
    const char *dev_name;//设置升级设备
};


union sys_upgrade_req {
    struct upgrade_ui   ui;
    struct upgrade_info info;
    struct upgrade_core core;
};

//同版本系统升级使能
//1:开启 0:关闭
void same_sys_upgrade_enable(u8 enable);
#endif
