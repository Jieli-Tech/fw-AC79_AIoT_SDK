#ifndef __FIG_ASR_API_H__
#define __FIG_ASR_API_H__

#include "fig_asr_type.h"

typedef void *FIG_INST;

#ifdef __cplusplus
extern "C" {
#endif

int FigCreateInst(FIG_INST *inst, const char *szNnet, const char *szGraph);
int FigDestroyInst(FIG_INST inst);
int FigSetParameter(FIG_INST inst, AsrParamType eType, const char *szValue);
int FigStartProcess(FIG_INST inst);
int FigStopProcess(FIG_INST inst);
int FigWriteAudio(FIG_INST inst, char *pData, int nLen, int bFinish, PAsrResult *ppResult);

#ifdef __cplusplus
}
#endif

#endif // __FIG_ASR_API_H__
