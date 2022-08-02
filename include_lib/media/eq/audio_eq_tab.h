#ifndef _AUDIO_EQ_TAB_H_
#define _AUDIO_EQ_TAB_H_

// #if (defined(TCFG_EQ_ENABLE) && (TCFG_EQ_ENABLE != 0))
#include "system/includes.h"
// #include "app_config.h"
#include "audio_eq.h"

/* const struct eq_seg_info eq_tab_normal[] = { */
// {0, EQ_IIR_TYPE_BAND_PASS, 31,    0 << 20, (int)(0.7f * (1 << 24))},
// {1, EQ_IIR_TYPE_BAND_PASS, 62,    0 << 20, (int)(0.7f * (1 << 24))},
// {2, EQ_IIR_TYPE_BAND_PASS, 125,   0 << 20, (int)(0.7f * (1 << 24))},
// {3, EQ_IIR_TYPE_BAND_PASS, 250,   0 << 20, (int)(0.7f * (1 << 24))},
// {4, EQ_IIR_TYPE_BAND_PASS, 500,   0 << 20, (int)(0.7f * (1 << 24))},
// {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0 << 20, (int)(0.7f * (1 << 24))},
// {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0 << 20, (int)(0.7f * (1 << 24))},
// {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0 << 20, (int)(0.7f * (1 << 24))},
// {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0 << 20, (int)(0.7f * (1 << 24))},
// {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0 << 20, (int)(0.7f * (1 << 24))},


// };

// const struct eq_seg_info eq_tab_rock[] = {
// {0, EQ_IIR_TYPE_BAND_PASS, 31,    -2 << 20, (int)(0.7f * (1 << 24))},
// {1, EQ_IIR_TYPE_BAND_PASS, 62,     0 << 20, (int)(0.7f * (1 << 24))},
// {2, EQ_IIR_TYPE_BAND_PASS, 125,    2 << 20, (int)(0.7f * (1 << 24))},
// {3, EQ_IIR_TYPE_BAND_PASS, 250,    4 << 20, (int)(0.7f * (1 << 24))},
// {4, EQ_IIR_TYPE_BAND_PASS, 500,   -2 << 20, (int)(0.7f * (1 << 24))},
// {5, EQ_IIR_TYPE_BAND_PASS, 1000,  -2 << 20, (int)(0.7f * (1 << 24))},
// {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0 << 20, (int)(0.7f * (1 << 24))},
// {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0 << 20, (int)(0.7f * (1 << 24))},
// {8, EQ_IIR_TYPE_BAND_PASS, 8000,   4 << 20, (int)(0.7f * (1 << 24))},
// {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4 << 20, (int)(0.7f * (1 << 24))},


// };

// const struct eq_seg_info eq_tab_pop[] = {
// {0, EQ_IIR_TYPE_BAND_PASS, 31,     3 << 20, (int)(0.7f * (1 << 24))},
// {1, EQ_IIR_TYPE_BAND_PASS, 62,     1 << 20, (int)(0.7f * (1 << 24))},
// {2, EQ_IIR_TYPE_BAND_PASS, 125,    0 << 20, (int)(0.7f * (1 << 24))},
// {3, EQ_IIR_TYPE_BAND_PASS, 250,   -2 << 20, (int)(0.7f * (1 << 24))},
// {4, EQ_IIR_TYPE_BAND_PASS, 500,   -4 << 20, (int)(0.7f * (1 << 24))},
// {5, EQ_IIR_TYPE_BAND_PASS, 1000,  -4 << 20, (int)(0.7f * (1 << 24))},
// {6, EQ_IIR_TYPE_BAND_PASS, 2000,  -2 << 20, (int)(0.7f * (1 << 24))},
// {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0 << 20, (int)(0.7f * (1 << 24))},
// {8, EQ_IIR_TYPE_BAND_PASS, 8000,   1 << 20, (int)(0.7f * (1 << 24))},
// {9, EQ_IIR_TYPE_BAND_PASS, 16000,  2 << 20, (int)(0.7f * (1 << 24))},


// };

// const struct eq_seg_info eq_tab_classic[] = {
// {0, EQ_IIR_TYPE_BAND_PASS, 31,     0 << 20, (int)(0.7f * (1 << 24))},
// {1, EQ_IIR_TYPE_BAND_PASS, 62,     8 << 20, (int)(0.7f * (1 << 24))},
// {2, EQ_IIR_TYPE_BAND_PASS, 125,    8 << 20, (int)(0.7f * (1 << 24))},
// {3, EQ_IIR_TYPE_BAND_PASS, 250,    4 << 20, (int)(0.7f * (1 << 24))},
// {4, EQ_IIR_TYPE_BAND_PASS, 500,    0 << 20, (int)(0.7f * (1 << 24))},
// {5, EQ_IIR_TYPE_BAND_PASS, 1000,   0 << 20, (int)(0.7f * (1 << 24))},
// {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0 << 20, (int)(0.7f * (1 << 24))},
// {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0 << 20, (int)(0.7f * (1 << 24))},
// {8, EQ_IIR_TYPE_BAND_PASS, 8000,   2 << 20, (int)(0.7f * (1 << 24))},
// {9, EQ_IIR_TYPE_BAND_PASS, 16000,  2 << 20, (int)(0.7f * (1 << 24))},

// };

// const struct eq_seg_info eq_tab_country[] = {
// {0, EQ_IIR_TYPE_BAND_PASS, 31,     -2 << 20, (int)(0.7f * (1 << 24))},
// {1, EQ_IIR_TYPE_BAND_PASS, 62,     0 << 20, (int)(0.7f * (1 << 24))},
// {2, EQ_IIR_TYPE_BAND_PASS, 125,    0 << 20, (int)(0.7f * (1 << 24))},
// {3, EQ_IIR_TYPE_BAND_PASS, 250,    2 << 20, (int)(0.7f * (1 << 24))},
// {4, EQ_IIR_TYPE_BAND_PASS, 500,    2 << 20, (int)(0.7f * (1 << 24))},
// {5, EQ_IIR_TYPE_BAND_PASS, 1000,   0 << 20, (int)(0.7f * (1 << 24))},
// {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0 << 20, (int)(0.7f * (1 << 24))},
// {7, EQ_IIR_TYPE_BAND_PASS, 4000,   0 << 20, (int)(0.7f * (1 << 24))},
// {8, EQ_IIR_TYPE_BAND_PASS, 8000,   4 << 20, (int)(0.7f * (1 << 24))},
// {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4 << 20, (int)(0.7f * (1 << 24))},

// };

// const struct eq_seg_info eq_tab_jazz[] = {
// {0, EQ_IIR_TYPE_BAND_PASS, 31,     0 << 20, (int)(0.7f * (1 << 24))},
// {1, EQ_IIR_TYPE_BAND_PASS, 62,     0 << 20, (int)(0.7f * (1 << 24))},
// {2, EQ_IIR_TYPE_BAND_PASS, 125,    0 << 20, (int)(0.7f * (1 << 24))},
// {3, EQ_IIR_TYPE_BAND_PASS, 250,    4 << 20, (int)(0.7f * (1 << 24))},
// {4, EQ_IIR_TYPE_BAND_PASS, 500,    4 << 20, (int)(0.7f * (1 << 24))},
// {5, EQ_IIR_TYPE_BAND_PASS, 1000,   4 << 20, (int)(0.7f * (1 << 24))},
// {6, EQ_IIR_TYPE_BAND_PASS, 2000,   0 << 20, (int)(0.7f * (1 << 24))},
// {7, EQ_IIR_TYPE_BAND_PASS, 4000,   2 << 20, (int)(0.7f * (1 << 24))},
// {8, EQ_IIR_TYPE_BAND_PASS, 8000,   3 << 20, (int)(0.7f * (1 << 24))},
// {9, EQ_IIR_TYPE_BAND_PASS, 16000,  4 << 20, (int)(0.7f * (1 << 24))},


// };


// struct eq_seg_info eq_tab_custom[] = {
// {0, EQ_IIR_TYPE_BAND_PASS, 31,    0<<20, (int)(0.7f * (1<<24))},
// {1, EQ_IIR_TYPE_BAND_PASS, 62,    0<<20, (int)(0.7f * (1<<24))},
// {2, EQ_IIR_TYPE_BAND_PASS, 125,   0<<20, (int)(0.7f * (1<<24))},
// {3, EQ_IIR_TYPE_BAND_PASS, 250,   0<<20, (int)(0.7f * (1<<24))},
// {4, EQ_IIR_TYPE_BAND_PASS, 500,   0<<20, (int)(0.7f * (1<<24))},
// {5, EQ_IIR_TYPE_BAND_PASS, 1000,  0<<20, (int)(0.7f * (1<<24))},
// {6, EQ_IIR_TYPE_BAND_PASS, 2000,  0<<20, (int)(0.7f * (1<<24))},
// {7, EQ_IIR_TYPE_BAND_PASS, 4000,  0<<20, (int)(0.7f * (1<<24))},
// {8, EQ_IIR_TYPE_BAND_PASS, 8000,  0<<20, (int)(0.7f * (1<<24))},
// {9, EQ_IIR_TYPE_BAND_PASS, 16000, 0<<20, (int)(0.7f * (1<<24))},

/* }; */


// #endif [> (TCFG_EQ_ENABLE == 1) <]

#endif
