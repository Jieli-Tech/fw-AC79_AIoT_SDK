#ifndef SPECTRUMSHOW_API_H
#define SPECTRUMSHOW_API_H

#ifdef WIN32
#define AT_SPECTRUMSHOW(x)
#define AT_SPECTRUMSHOW_CODE
#define AT_SPECTRUMSHOW_CONST
#define AT_SPECTRUMSHOW_SPARSE_CODE
#define AT_SPECTRUMSHOW_SPARSE_CONST
#else
#define AT_SPECTRUMSHOW(x)           __attribute((section(#x)))
#define AT_SPECTRUMSHOW_CODE         AT_SPECTRUMSHOW(.specshow_code)
#define AT_SPECTRUMSHOW_CONST        AT_SPECTRUMSHOW(.specshow_const)
#define AT_SPECTRUMSHOW_SPARSE_CODE  AT_SPECTRUMSHOW(.specshow_sparse_code)
#define AT_SPECTRUMSHOW_SPARSE_CONST AT_SPECTRUMSHOW(.specshow_sparse_const)
#endif

int getSpectrumShowBuf(void);

void SpectrumShowInit(void *workBuf, float attackFactor, float releaseFactor, int fs, int channel, int mode, unsigned int fft_addr);

int SpectrumShowRun(void *workBuf, short *in, int len);

int getSpectrumNum(void *workBuf);

short *getSpectrumValue(void *workBuf);

#endif
