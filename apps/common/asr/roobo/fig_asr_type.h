#ifndef __FIG_ASR_TYPE_H__
#define __FIG_ASR_TYPE_H__

typedef enum {
    NNET_MODEL = 0,
    WFST_COMMAND,
    WFST_FILLER,
} AsrResType;

typedef enum {
    REC_CM_THRES,
    REC_RESET_CM_THRES,
    REC_DECODER_BEAM,
    REC_FILLER_NUM,
} AsrParamType;

typedef struct tagResult {
    int nBegin;
    int nEnd;
    char szText[32];
    short nWordId;
    int nCmScore;
} AsrResult, *PAsrResult;

#endif // __FIG_ASR_TYPE_H__
