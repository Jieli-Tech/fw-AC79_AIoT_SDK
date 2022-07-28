#include "system/includes.h"
#include "app_database.h"
#include "app_config.h"
#include "generic/typedef.h"
#include "syscfg/syscfg_id.h"

#define CN_PA   ((0xA9BE << 16) | ('A' << 8)  | ('B' << 0))
#define CN_PB   (('C'    << 24) | ('D' << 16) | ('E' << 8) | ('F' << 0))

/*
 * app配置项表
 * 参数1: 配置项名字
 * 参数2: 配置项需要多少个字节存储
 * 参数3: 配置项INDEX
 * 参数4: 配置项的默认值
 */

struct db_cfg {
    const char *name;
    u8 len;
    int index;
    int value;
};

static const struct db_cfg config_table[] = {
    {"res",     1,   VM_RES_INDEX,   	VIDEO_RES_VGA},           	// 录像分辨率
    {"cyc",     1,   VM_CYC_INDEX,   	3 },                         // 循环录像时间，单位分钟
    {"mic",     1,   VM_MIC_INDEX,   	1 },                         // 录音开关
    {"dat",     1,   VM_DAT_INDEX,   	1 },                         // 时间标签开关
};

int db_select(const char *name)
{
    int err;
    int value = 0;

    if (name == NULL) {
        return -EINVAL;
    }

    for (int i = 0; i < ARRAY_SIZE(config_table); i++) {
        if (!strcmp(name, config_table[i].name)) {
            err = syscfg_read(config_table[i].index, &value, config_table[i].len);
            if (err <= 0) {
                puts(">>>syscfg_read err");
                return -EINVAL;
            }
            return value;
        }
    }
    return -EINVAL;
}

int db_update(const char *name, u32 value)
{
    int err;

    if (name == NULL) {
        return -EINVAL;
    }

    for (int i = 0; i < ARRAY_SIZE(config_table); i++) {
        if (!strcmp(name, config_table[i].name)) {
            err = syscfg_write(config_table[i].index, &value, config_table[i].len);
            if (err <= 0) {
                puts(">>>syscfg_write err");
                return -EINVAL;
            }
            return 0;
        }
    }
    return -EINVAL;
}

static int app_config_init()
{
    int err;
    int value;

    for (int i = 0; i < ARRAY_SIZE(config_table); i++) {
        value = 0;
        err = syscfg_read(config_table[i].index, &value, config_table[i].len);
        if (err < 0) {
            err = syscfg_write(config_table[i].index, &config_table[i].value, config_table[i].len);
            if (err <= 0) {
                puts(">>>app_config_init err");
                return -EINVAL;
            }
        }
    }

    return 0;
}

__initcall(app_config_init);
