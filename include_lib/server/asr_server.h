#ifndef ASR_SERVER_H
#define ASR_SERVER_H

enum {
    STATE_OFF,
    STATE_ON,
};

enum {
    ASR_REQ_WAKE_ON_VOICE_ON,
    ASR_REQ_WAKE_ON_VOICE_OFF,
};

enum {
    ASR_SER_EVENT_WAKE,
};

struct asr_wake {
    void *enc_buf;
    int enc_buf_len;
    const char *data_file_path;
    u32 idle_gap_16ms;
    u32 run_gap_16ms;
    int best_sc;
};

union asr_req {
    struct asr_wake wake;
};


#endif

