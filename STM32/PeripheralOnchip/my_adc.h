/*****************************************************************************
* Copyright:
* File name: my_adc.h
* Description: adc模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/01/19
*****************************************************************************/


#ifndef __MY_ADC_H__
#define __MY_ADC_H__



#define MY_ADC_0  0
#define MY_ADC_1  1
#define MY_ADC_2  2
#define MY_ADC_3  3
#define MY_ADC_4  4
#define MY_ADC_5  5
#define MY_ADC_6  6
#define MY_ADC_7  7
#define MY_ADC_8  8
#define MY_ADC_9  9



typedef struct __ADC
{
    int (*init)(u8 ch);
    int (*get)(u8 ch,float* v);
}c_adc;

extern const c_adc  my_adc;

#endif 
