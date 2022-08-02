#ifndef __RF_FCC_MAIN_H_
#define __RF_FCC_MAIN_H_


#define TRIGGER_IO      IO_PORTC_00  //触发IO

/**
 * @brief rf_fcc_test_init，RF_FCC测试初始化
 *
 * @param
 *
 * @note
 */
u8 rf_fcc_test_init(void);


/**
 * @brief wifi_tx_data_test，WIFI发送数据测试
 *
 * @param
 *          u8 channel     ：信道(1-13)
 *          u8 power       ：数字增益(0-128)
 *          u8 rate        ：速率
 *          u8 *packet     ：需发送的数据
 *          u32 packet_len ：需发送的数据长度(1-1513)
 *          u32 npackets   ：发送次数；设为0，则一直发送
 *          u32 tx_interval：发送间隔
 *
 * @note
 */
void wifi_tx_data_test(u8 channel, u8 power, u8 rate, u8 *packet, u32 packet_len, u32 npackets, u32 tx_interval);


/**
 * @brief rf_fcc_get_uart，获取RF_FCC测试的上位机通信串口号
 *
 * @param
 *
 * @note 可在外部定义同名函数，修改通信串口号
 */
__attribute__((weak)) const char *rf_fcc_get_uart(void);


/**
 * @brief rf_fcc_test_init，开机检测是否进入RF_FCC测试
 *
 * @param
 *
 * @note 可在外部定义同名函数，修改触发方式
 */
__attribute__((weak))u8 fcc_mode_enter_check(void *priv);


/**
 * @brief fcc_res_handler，RF_FCC测试结果处理
 *
 * @param res == true, 测试PASS
 *        res == false, 测试FAIL
 *
 * @note 可在外部定义同名函数，根据测试结果添加自定义操作
 */
__attribute__((weak))void fcc_res_handler(u8 res);

#endif


