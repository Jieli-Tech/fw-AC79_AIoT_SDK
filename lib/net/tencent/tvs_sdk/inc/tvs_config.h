#ifndef __TVS_CONFIG_H_34AF354ATR__
#define __TVS_CONFIG_H_34AF354ATR__
#include <stdbool.h>
#include "tvs_core.h"

void tvs_config_set_qua(tvs_product_qua *qua);

char *tvs_config_get_qua();

bool tvs_config_is_sandbox_open();

char *tvs_config_get_auth_url();

char *tvs_config_get_event_url();

char *tvs_config_get_down_channel_url();

char *tvs_config_get_echo_url();

char *tvs_config_get_ping_url();

void tvs_config_set_network_valid(bool valid);

bool tvs_config_is_network_valid();

char *tvs_config_get_current_host();

void tvs_config_init();

void tvs_config_set_current_env(int env);

int tvs_config_get_current_env();

void tvs_config_set_sdk_running(bool running);

bool tvs_config_is_sdk_running();

int tvs_config_get_recorder_bitrate();

int tvs_config_get_recorder_channels();

void tvs_config_enable_https(bool enable);

void tvs_config_print_asr_result(bool enable);

bool tvs_config_will_print_asr_result();

void tvs_config_set_sandbox_open(bool open);

int tvs_config_get_current_asr_bitrate();

void tvs_config_set_current_asr_bitrate(int bitrate);

void tvs_config_set_hearttick_interval(int interval_sec);

int tvs_config_get_hearttick_interval();

bool tvs_config_is_down_channel_enable();

void tvs_config_enable_down_channel(bool enable);

void tvs_config_enable_ip_provider(bool enable);

bool tvs_config_ip_provider_enabled();

int tvs_config_get_speex_compress();

void tvs_config_set_speex_compress(int compress);
#endif
