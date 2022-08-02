#ifndef __TVS_PREFERENCE_INTERFACE_H__
#define __TVS_PREFERENCE_INTERFACE_H__

/**
 * @brief 实现此接口，向TVS SDK提供读取持久化配置的能力。
 * 一般在设备重启的时候，会回调此函数加载持久化配置的各项参数
 *
 * @param preference_size 出参，代表当前持久化配置数据占用内存空间的总字节数
 * @return
 *		当前系统的持久化配置数据，由此函数内部通过malloc在堆上申请内存，SDK调完此接口，将会调用free释放内存
 */
const char *tvs_preference_load_data(int *preference_size);

/**
 * @brief 实现此接口，向TVS SDK提供保存持久化配置的能力。
 * SDK在各种配置参数的值改变的时候，会回调此函数，保存新的持久化配置
 *
 * @param preference_buffer 带保存的持久化配置数据
 * @param preference_size 当前持久化配置数据占用内存空间的总字节数
 * @return
 *		0为保存成功，其他值为保存失败
 */
int tvs_preference_save_data(const char *preference_buffer, int preference_size);

#endif
