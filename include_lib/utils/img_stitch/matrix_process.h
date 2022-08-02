
#ifndef _MATRIX_PROCESS_H
#define _MATRIX_PROCESS_H
/*#include<stdio.h>
#include<string.h>
#include<stdint.h>*/
/*#include "fs.h"*/
//#define debug
typedef struct {
    unsigned int numRows;     /**< number of rows of the matrix.     */
    unsigned int numCols;     /**< number of columns of the matrix.  */
    float *pData;     /**< points to the data of the matrix. */
} pi32v2_matrix_instance_f32;

typedef enum {
    PI32V2_MATH_SUCCESS = 0,        /**< No error */
    PI32V2_MATH_ARGUMENT_ERROR = -1,        /**< One or more arguments are incorrect */
    PI32V2_MATH_LENGTH_ERROR = -2,        /**< Length of data buffer is incorrect */
    PI32V2_MATH_SIZE_MISMATCH = -3,        /**< Size of matrices is not compatible with the operation */
    PI32V2_MATH_NANINF = -4,        /**< Not-a-number (NaN) or infinity is generated */
    PI32V2_MATH_SINGULAR = -5,        /**< Input matrix is singular and cannot be inverted */
    PI32V2_MATH_TEST_FAILURE = -6         /**< Test Failed */
} pi32v2_status;

void pi32v2_mat_init_f32(
    pi32v2_matrix_instance_f32 *S,
    unsigned int nRows,
    unsigned int nColumns,
    float *pData);

pi32v2_status pi32v2_mat_mult_f32(
    const pi32v2_matrix_instance_f32 *pSrcA,
    const pi32v2_matrix_instance_f32 *pSrcB,
    pi32v2_matrix_instance_f32 *pDst);

pi32v2_status pi32v2_mat_trans_f32(
    const pi32v2_matrix_instance_f32 *pSrc,
    pi32v2_matrix_instance_f32 *pDst);

pi32v2_status pi32v2_mat_inverse_f32(
    const pi32v2_matrix_instance_f32 *pSrc,
    pi32v2_matrix_instance_f32 *pDst);

int least_square(unsigned int *src_point_x, unsigned int *src_point_y, unsigned int *dst_point_x, unsigned int *dst_point_y, int count, float *result, int th_x, int th_y);
#endif

