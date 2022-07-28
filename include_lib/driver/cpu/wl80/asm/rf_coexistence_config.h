#ifndef __RF_COEXISTENCE_CONFIG_H__
#define __RF_COEXISTENCE_CONFIG_H__

/*********************************************************************************************************
0 - 均衡模式1：适用于wifi连接路由器后且edr配对前
1 - 均衡模式2：适用于edr配对后(接收模式)，适用于蓝牙接收多，发送少的场景，wifi保持连接
2 - 蓝牙绝对优先模式：wifi只能在蓝牙空闲的片段里工作，对wifi的性能有较大影响
3 - wifi绝对优先模式：一般用于开机时wifi快速连上路由器，不受蓝牙回连干扰，但不抢BLE，对edr的性能有较大影响
4 - wifi相对优先模式：优先保证wifi收发，不保证蓝牙播歌通话不卡，不抢BLE
5 - 均衡模式3：适用于edr配对后(发送模式)，适用于蓝牙发送多，接收少的场景
7 - 均衡模式5：一般用于设备edr主动回连时释放部分带宽给wifi
*********************************************************************************************************/

void switch_rf_coexistence_config_table(u8 index);

void switch_rf_coexistence_config_lock(void);

void switch_rf_coexistence_config_unlock(void);

#endif
