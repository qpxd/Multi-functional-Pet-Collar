/*****************************************************************************
* Copyright:
* File name: tracking_light.h
* Description: 循迹灯 公开头文集
* Author: 许少
* Version: V1.0
* Date: 2021/12/20
*****************************************************************************/
#ifndef __TRACKING_LIGHT_H__
#define __TRACKING_LIGHT_H__



typedef struct __TRACKING_LIGHT
{
	void*           this;
	GPIO_PinState (*get_state)(void* this);  /*获取循迹灯状态*/
}c_tracking_light;

/*************************************************
* Function: 创建一个循迹灯
* Description: tracking_light_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
c_tracking_light tracking_light_init(GPIO_TypeDef* gpio,uint32_t pin,u8 reverse);

#endif

