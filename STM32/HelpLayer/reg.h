/*****************************************************************************
* Copyright:
* File name: reg.c
* Description: 寄存器操作工具公开头文件
* Author:许少
* Version: V1.0
* Date: 2022/01/14
*****************************************************************************/

#ifndef __REG_H__
#define __REG_H__

void be_out(GPIO_TypeDef* gpio,uint32_t pin);
void be_in(GPIO_TypeDef* gpio,uint32_t pin);
void exti_cfg(GPIO_TypeDef* gpio,uint32_t pin,uint32_t mode) ;
void exti_set_follow(uint32_t pin,uint32_t mode);
uint32_t exti_get_follow(uint32_t pin);


#endif

