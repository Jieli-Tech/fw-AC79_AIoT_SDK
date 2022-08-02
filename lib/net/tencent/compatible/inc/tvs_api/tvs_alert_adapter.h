
/**
* @file  tvs_alert_adapter.h
* @brief TVS 闹钟适配接口层
* @date  2019-5-10
*/

#ifndef __TVS_ALERT_ADAPTER_H__
#define __TVS_ALERT_ADAPTER_H__

/**
* @brief 闹钟的详细信息
*/
typedef struct {
    char *alert_detail;         /*!< 闹钟的详细信息,json格式 */
} tvs_alert_infos;

#define TVS_ALERTS_TYPE_MAX   32
#define TVS_ALERTS_TIME_MAX   64
#define TVS_ALERTS_TOKEN_MAX   255

/**
* @brief 闹钟概要信息
*/
typedef struct {
    char alert_type[TVS_ALERTS_TYPE_MAX + 1];  			/*!< 闹钟类型 */
    char time_iso_8601[TVS_ALERTS_TIME_MAX + 1];        /*!< 闹钟触发时间，按ISO 8601格式定义 */
    char alert_token[TVS_ALERTS_TOKEN_MAX + 1];         /*!< 闹钟唯一标识 */
} tvs_alert_summary;

/**
* @brief 闹钟触发振铃后，停止的原因
*/
typedef enum {
    TVS_ALART_STOP_REASON_NEW_RECO,  /*!< 点击录音按钮导致闹钟振铃停止 */
    TVS_ALART_STOP_REASON_TIMEOUT,   /*!< 闹钟振铃超时停止 */
    TVS_ALART_STOP_REASON_MANNUAL,   /*!< 用户手动停止 */
} tvs_alert_stop_reason;

/**
 * @brief 实现此接口，赋予SDK新增闹钟的能力
 *
 * @param alerts 闹钟详情
 * @param alert_count 闹钟的个数
 * @return 0为添加闹钟成功，其他值为失败
 */
typedef int (*tvs_alert_adapter_new)(tvs_alert_infos *alerts, int alert_count);

/**
 * @brief 实现此接口，赋予SDK删除闹钟的能力
 *
 * @param alerts 待删除闹钟概要，可以根据alert_token删除指定闹钟
 * @param alert_count 待删除闹钟的个数
 * @return 0为删除闹钟成功，其他值为失败
 */
typedef int (*tvs_alert_adapter_delete)(tvs_alert_summary *alerts, int alert_count);

/**
 * @brief 实现此接口，赋予SDK获取闹钟概要信息的能力
 *
 * @param count 出参，闹钟的个数
 * @return 当前系统闹钟概要信息的集合
 */
typedef tvs_alert_summary *(*tvs_alert_adapter_get_all_alerts)(int *count);

/**
 * @brief 实现此接口，赋予SDK获取闹钟概要信息的能力,扩展接口。
 * 如果实现了本接口，tvs_alert_adapter_get_all_alerts将不会被调用
 *
 * @return 当前系统闹钟概要信息的集合，是一个cJSON*
 */
typedef void *(*tvs_alert_adater_get_all_alerts_ex)();

typedef struct {
    tvs_alert_adapter_new do_new_alert;
    tvs_alert_adapter_delete do_delete;
    tvs_alert_adapter_get_all_alerts get_alerts;
    tvs_alert_adater_get_all_alerts_ex get_alerts_ex;
} tvs_alert_adapter;

/**
 * @brief 初始化
 *
 * @param
 * @return 0为成功，其他值为失败
 */
int tvs_alert_adapter_init(tvs_alert_adapter *adapter);

/**
 * @brief 在闹钟到时开始振铃的时候，调用此函数通知SDK
 *
 * @param alert_token 当前触发的闹钟token
 * @return 0为成功，其他值为失败
 */
int tvs_alert_adapter_on_trigger(const char *alert_token);

/**
 * @brief 在闹钟到时触发振铃之后，在结束的时候，调用此函数通知SDK
 *
 * @param alert_token 当前触发停止的闹钟token
 * @param reason 停止原因
 * @return 0为成功，其他值为失败
 */
int tvs_alert_adapter_on_trigger_stop(const char *alert_token, tvs_alert_stop_reason reason);

#endif