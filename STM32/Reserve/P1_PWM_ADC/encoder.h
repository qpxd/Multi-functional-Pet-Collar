/*****************************************************************************
* Copyright:
* File name: encoder.h
* Description: encoder模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/04/26
* log:  V1.0  2022/04/26
*       发布：
*****************************************************************************/

#ifndef __ENCODER_H__
#define __ENCODER_H__



#define ENCODER_TIM_1  0
#define ENCODER_TIM_2  1
#define ENCODER_TIM_3  2
#define ENCODER_TIM_4  3

typedef struct __C_ENCODER
{
    int (*init)  (u8 timer);

    int (*get_count) (u8 timer,int* count,bool clear);
    int (*clear)     (u8 timer);
}c_encoder;

extern const c_encoder encoder;

#endif

