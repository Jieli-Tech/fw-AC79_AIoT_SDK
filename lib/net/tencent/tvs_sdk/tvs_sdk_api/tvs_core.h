#ifndef __TVS_CORE_H__
#define __TVS_CORE_H__
#include <stdbool.h>
#include "tvs_media_player_interface.h"
#include "tvs_common_def.h"

int tvs_core_initialize(tvs_api_callback *callback, tvs_default_config *default_config, tvs_product_qua *qua);

int tvs_core_new_session_id();

void tvs_core_set_current_session_id(int session_id);

int tvs_core_get_current_session_id();

/**
 * @brief 判断一个session id，是否和当前的全局session id相同
 *
 * @param  session_id
 * @return true为相同，false为不同
 */
bool tvs_core_check_session_id(int session_id);

int tvs_core_start_recognize(int session_id,
                             tvs_api_recognizer_type type, const char *wakeword, int startIndexInSamples, int endIndexInSamples);

int tvs_core_play_control_next();

int tvs_core_play_control_prev();

int tvs_core_send_semantic(const char *semantic);

int tvs_core_send_push_text(const char *text);

int tvs_core_start_text_to_speech(tvs_api_tts_param *tts_param);

void tvs_core_notify_net_state_changed(bool connected);

void tvs_core_notify_media_player_on_play_finished(const char *token);

void tvs_core_notify_media_player_on_play_stopped(int error_code, const char *token);

void tvs_core_notify_media_player_on_play_started(const char *token);

void tvs_core_notify_media_player_on_play_paused(const char *token);

void tvs_core_notify_volume_changed(int volume);

void tvs_core_notify_alert_start_ring(const char *token);

void tvs_core_notify_alert_stop_ring(const char *token);

void tvs_core_start_sdk();

void tvs_core_pause_sdk();

void tvs_core_enable_log(bool enable);

/**
 * @brief 授权模块初始化
 *
 * @param  product_id 产品ID
 * @param  dsn 设备唯一标识
 * @param  authorize_info 之前保存的授权信息，内容为json字符串；可以传NULL，代表之前从未授权
 * @param  authorize_info_size 授权信息的长度
 * @param  auth_callback 授权结果的监听器
 * @return 0为成功，其他值为失败
 */
int tvs_core_authorize_initalize(const char *product_id, const char *dsn, const char *authorize_info, int authorize_info_size, tvs_authorize_callback auth_callback);

/**
 * @brief 触发访客授权流程
 *
 * @param
 * @return 0为成功，其他值为失败；
 *         取值TVS_API_ERROR_NETWORK_INVALID代表网络未连接，在网络连接后会自动进行授权;
 */
int tvs_core_authorize_guest_login();

/**
 * @brief 在设备授权方案中，从手机APP端同步client id到设备端后，需要调用此函数设置client id;
 *
 * @param  client_id 从手机APP同步过来的client id
 * @return 0为成功，其他值为失败；
 */
int tvs_core_authorize_set_client_id(const char *client_id);

/**
 * @brief 触发设备授权流程
 *
 * @param
 * @return 0为成功，其他值为失败；
 *         取值TVS_API_ERROR_NETWORK_INVALID代表网络未连接，在网络连接后会自动进行授权;
 */
int tvs_core_authorize_login();

/**
 * @brief 注销，用于重新授权的流程，调用此函数清除之前授权的信息
 *
 * @param
 * @return 0为成功，其他值为失败；
 */
int tvs_core_authorize_logout();

#endif
