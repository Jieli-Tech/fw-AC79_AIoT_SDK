#ifndef __MATH_H
#define __MATH_H

#include "generic/typedef.h"

/*******************************************************************************



注：
   此页主要说明，定点(Fix)和浮点(Float)的数学接口函数的调用方法及注意事项！
目前仅支持sin、cos、arctan(y/x)、sqrt(x^2+y^2)、sinh、cosh、exp、arctanh(y/x)、sqrt(x^2-y^2)、ln(x)、sqrt(x)共11个常用数学函数调用

特别说明：1、各数学接口函数调用时的输入数据，应严格遵循表1中的限制；
          2、由于受int型数据位宽限制，exp函数的输入数据范围应限制在[-10，4.852]；
          3、数据的精度损失详见Reference relative error。


----------------------------------------------------------------------------------

void sin_fix(int a, int aQ, int rQ, int *r)

计算定点数a的正弦值r，
其中：aQ为数a的实际值移位数；
      rQ为计算sin输出r的放大倍数位宽，可自定义。
注意：计算sin、cos的输入a值均是不包含pi！详见表1


 void arctan_fix(int x, int xQ, int y, int yQ, int rQ, int *r)

计算定点数y/x的反正切值，
其中：xQ为输入定点数x的实际放大倍数位宽；
      yQ为输入定点数y的实际放大倍数位宽；
      rQ为计算arctan输出r的放大倍数位宽，可自定义。
注意：计算arctan时的输出r值是乘以了pi！详见表1



----------------------------------------------------------------------------------

in_float(float a, float *r)

计算浮点数a的正弦值r，

注意：计算sin、cos的输入a值均是不包含pi！详见表1


  void arctan_float(float x, float y, float *r)

计算浮点数y/x的反正切值r，

注意：计算arctan时的输出r值是乘以了pi！详见表1




***********************************************************************************/


void sin_float(float a, float *r);
void cos_float(float a, float *r);
void arctan_float(float x, float y, float *r);
void arctanh_float(float x, float y, float *r);
void sinh_float(float a, float *r);
void cosh_float(float a, float *r);
void exp_float(float a, float *r);
void log_float(float a, float *r);
void sqrt_float(float a, float *r);
void SRSS_float(float x, float y, float *r);
void SDS_float(float x, float y, float *r);

void sin_fix(int a, int aQ, int rQ, int *r);
void cos_fix(int a, int aQ, int rQ, int *r);
void arctan_fix(int x, int xQ, int y, int yQ, int rQ, int *r);
void arctanh_fix(int x, int xQ, int y, int yQ, int rQ, int *r);
void sinh_fix(int x, int xQ, int rQ, int *r);
void cosh_fix(int x, int xQ, int rQ, int *r);
void exp_fix(int a, int aQ, int rQ, int *r);
void log_fix(int a, int aQ, int rQ, int *r);
void sqrt_fix(int a, int aQ, int rQ, int *r);
void SRSS_fix(int x, int xQ, int y, int yQ, int rQ, int *r);
void SDS_fix(int x, int xQ, int y, int yQ, int rQ, int *r);


#endif /*__MATCH_H*/

