#ifndef _RDEC_H_
#define _RDEC_H_

#include "generic/typedef.h"

enum rdec_index {
    RDEC0,
    RDEC1,
    RDEC2,
};

struct rdec_device {
    enum rdec_index index;
    u8 sin_port0; 	//采样信号端口0
    u8 sin_port1; 	//采样信号端口1
    u8 key_value0; 	//键值1
    u8 key_value1; 	//键值2
};

struct rdec_platform_data {
    u8 enable;
    u8 num; 	//rdec数量
    const struct rdec_device *rdec;
};

/*********************** rdec 初始化 ******************************/
int rdec_init(const struct rdec_platform_data *user_data);

extern s8 get_rdec_rdat(int i);

#endif

