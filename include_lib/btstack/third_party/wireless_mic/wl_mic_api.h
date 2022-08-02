#ifndef __WL_MIC_API_H__
#define __WL_MIC_API_H__

typedef struct {
    uint8_t  pair_name[28];
    uint8_t  spec_name[28];
    uint16_t param[32];
} wlm_param;

typedef struct {
    void (*wlm_connect_succ)(void);
    void (*wlm_disconnect)(void);
    void (*wlm_iso_rx)(const void *const buf, size_t length);
    void (*wlm_set_pair_trigger)(uint32_t, uint32_t);
} wlm_lib_callback;

typedef struct {
    void (*wlm_init)(const void *const param, const void *const cb);
    void (*wlm_open)(void);
    void (*wlm_close)(void);
    int (*wlm_iso_tx)(const void *const buf, size_t length);
    void (*wlm_set_pair)(uint32_t);
    void (*wlm_enter_pair)(void);
    void (*wlm_exit_pair)(void);
} wlm_lib_ops;

extern const wlm_lib_ops wlm_1t1_rx_op;
extern const wlm_lib_ops wlm_1t1_tx_op;
extern const wlm_lib_ops wlm_1tN_rx_op;
extern const wlm_lib_ops wlm_1tN_tx_op;

#endif /* __WL_MIC_API_H__ */
