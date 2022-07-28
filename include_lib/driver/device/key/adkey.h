#ifndef DEVICE_ADKEY_H
#define DEVICE_ADKEY_H

#include "generic/typedef.h"

///  \cond DO_NOT_DOCUMENT
#define ADKEY_MAX_NUM 10

#define ADKEY_MAX_NUM 10
/// \endcond

struct adkey_platform_data {
    u8 enable;					   /*!<  ad按键使能，使能为1，不使能为0*/
    u8 adkey_pin;				   /*!<  ad按键引脚*/
    u8 extern_up_en;               /*!<  是否用外部上拉，1：用外部上拉， 0：用内部上拉10K*/
    u32 ad_channel;				   /*!<  ad通道*/
    u16 ad_value[ADKEY_MAX_NUM];   /*!<  ad值*/
    u8  key_value[ADKEY_MAX_NUM];  /*!<  key值*/
};

///  \cond DO_NOT_DOCUMENT
struct adkey_rtcvdd_platform_data {
    u8 enable;
    u8 adkey_pin;
    u8  adkey_num;
    u32 ad_channel;
    u32 extern_up_res_value;        //是否用外部上拉，1：用外部上拉， 0：用内部上拉10K
    u16 res_value[ADKEY_MAX_NUM]; 	//电阻值, 从 [大 --> 小] 配置
    u8  key_value[ADKEY_MAX_NUM];
};
/// \endcond

/**
 * @brief adkey_init：ad按键初始化
 *
 * @param adkey_data ad按键句柄
 */
extern int adkey_init(const struct adkey_platform_data *adkey_data);

/**
 * @brief ad_get_key_value：获取ad按键值
 */
extern u8 ad_get_key_value(void);

///  \cond DO_NOT_DOCUMENT
//RTCVDD ADKEY API:
extern int adkey_rtcvdd_init(const struct adkey_rtcvdd_platform_data *rtcvdd_adkey_data);
extern u8 adkey_rtcvdd_get_key_value(void);
/// \endcond

#endif

