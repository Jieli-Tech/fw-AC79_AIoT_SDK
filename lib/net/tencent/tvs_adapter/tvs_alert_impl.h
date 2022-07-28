#ifndef __TVS_ALERT_IMPL_H__
#define __TVS_ALERT_IMPL_H__

int tvs_init_alert_adater_impl(tvs_alert_adapter *ad);

// TO-DO 闹钟响铃时需要调用此函数通知SDK
void on_alert_trigger_start(const char *alert_token);

// TO-DO 闹钟响铃之后，超时或者手动停止，都需要调用此函数通知SDK
void on_alert_trigger_stop(const char *alert_token, tvs_alert_stop_reason reason);

#endif