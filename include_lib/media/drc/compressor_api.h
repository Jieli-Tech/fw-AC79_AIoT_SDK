#ifndef COMPRESSOR_API
#define COMPRESSOR_API

#ifndef WIN32
#define AT_COMPRESSOR(x)			__attribute((section(#x)))
#define AT_COMPRESSOR_CODE			AT_COMPRESSOR(.compressor_code)
#define AT_COMPRESSOR_CONST			AT_COMPRESSOR(.compressor_const)
#define AT_COMPRESSOR_SPARSE_CODE	AT_COMPRESSOR(.compressor_sparse_code)
#define AT_COMPRESSOR_SPARSE_CONST	AT_COMPRESSOR(.compressor_sparse_const)
#endif

int need_compressor_buf();
void compressor_init(void *work_buf, int *attackTime, int *releaseTime, int (*threshold)[3], int (*ratio)[3], int sample_rate, int channel);
void compressor_update_para(void *work_buf, int *attackTime, int *releaseTime, int (*threshold)[3], int (*ratio)[3], int sample_rate, int channel);
int compressor_run_16(void *work_buf, short *in_buf, short *out_buf, int point_per_channel);
int compressor_run_32(void *work_buf, int *in_buf, int *out_buf, int point_per_channel);

#endif // !COMPRESSOR_API

