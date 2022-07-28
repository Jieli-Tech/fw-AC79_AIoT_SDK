#if !defined(IVPOLOS__2019_10_20__H)
#define IVPOLOS__2019_10_20__H
//#include "ivPolos.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Create POLOS INSTANCE */
int POLOS_INIT(void *mPMEM);

/* POLOS WRITE AUDIO*/
int POLOS_AUDIOWRITE(const void *input_data, int in_size, void *output_data, int *out_size);

int GetAECVersion();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !defined(IVPOLOS__2019_10_20__H) */

