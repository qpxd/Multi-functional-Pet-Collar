/*****************************************************************************
* Copyright:
* File name: int_manage.h
* Description: 中断管理模块 公开头文件
* Author: 许少
* Version: V1.1
* Date: 2022/02/16
* log:  V1.0  2022/01/20
*       发布：
*
*       V1.1  2022/02/16
*       优化：增加DMA的中断管理。
*****************************************************************************/

#ifndef __INT_MANAGE_H__
#define __INT_MANAGE_H__

#define DMA1_CHA_1  0
#define DMA1_CHA_2  1
#define DMA1_CHA_3  2
#define DMA1_CHA_4  3
#define DMA1_CHA_5  4
#define DMA1_CHA_6  5
#define DMA1_CHA_7  6

#define TIM1_UPDATA 0
#define TIM2_UPDATA 1
#define TIM3_UPDATA 2
#define TIM4_UPDATA 3

typedef int (*int_handle_func)(uint32_t,void*) ;

typedef struct __INT_MANAGE
{
    int (*login_exti)  (uint32_t int_line,int_handle_func int_handle,void* param);
    int (*login_dma)   (uint32_t dma_cha,int_handle_func int_handle,void* param);
    int (*login_timer) (uint32_t timer_cha,TIM_HandleTypeDef* timer,int_handle_func int_handle,void* param);
}c_int_manage;

extern const c_int_manage int_manage;
void SystemIsr_PriorityInit(void);

#endif


