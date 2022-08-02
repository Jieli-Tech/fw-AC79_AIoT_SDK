#ifndef LIMITER_API_H
#define LIMITER_API_H

#ifndef WIN32
#define AT_LIMITER(x)			__attribute((section(#x)))
#define AT_LIMITER_CODE			AT_LIMITER(.limiter_code)
#define AT_LIMITER_CONST		AT_LIMITER(.limiter_const)
#define AT_LIMITER_SPARSE_CODE	AT_LIMITER(.limiter_sparse_code)
#define AT_LIMITER_SPARSE_CONST	AT_LIMITER(.limiter_sparse_const)
#endif

int need_limiter_buf();
void limiter_init(void *work_buf, int *attackTime, int *releaseTime, int *threshold, int sample_rate, int channel);
void limiter_update_para(void *work_buf, int *attackTime, int *releaseTime, int *threshold, int sample_rate, int channel);
int limiter_run_16(void *work_buf, short *in_buf, short *out_buf, int per_channel_npoint);
int limiter_run_32(void *work_buf, int *in_buf, int *out_buf, int per_channel_npoint);

#endif // !LIMITER_API_H
