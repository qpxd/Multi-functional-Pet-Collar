/*****************************************************************************
* Copyright:
* File name: tracking_light.c
* Description: 循迹灯 函数实现
* Author: 许少
* Version: V1.0
* Date: 2021/12/20
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_TRACK_ENABLED

#define MODULE_NAME       "chassis"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

typedef struct __M_TRACKING_LIGHT
{
	GPIO_TypeDef* m_gpio     ;  /*gpio*/
	uint32_t      m_pin      ;  /*pin*/
	u8            m_reverse  ;  /*是否反向*/
}m_tracking_light;

/*************************************************
* Function: 获取循迹灯 状态
* Description: m_tracking_light_get
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
GPIO_PinState m_tracking_light_get(void* this);

/*************************************************
* Function: 创建一个循迹灯
* Description: tracking_light_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
c_tracking_light tracking_light_init(GPIO_TypeDef* gpio,uint32_t pin,u8 reverse)
{
	c_tracking_light new = {0};
	GPIO_InitTypeDef GPIO_Initure;    
    
	/*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_tracking_light));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_tracking_light));
	
    //初始化对应的GPIO
    GPIO_Initure.Pin   = pin                 ;
    GPIO_Initure.Mode  = GPIO_MODE_INPUT     ;  //输入
    GPIO_Initure.Pull  = GPIO_NOPULL         ;  //浮空
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;//高速
    HAL_GPIO_Init(gpio,&GPIO_Initure);  

	/*保存相关变量*/
	((m_tracking_light* )new.this)->m_gpio     = gpio    ;
	((m_tracking_light* )new.this)->m_pin      = pin     ;
	((m_tracking_light* )new.this)->m_reverse  = reverse ;
	new.get_state = m_tracking_light_get;
	
	
	return new;
}

/*************************************************
* Function: 获取循迹灯 状态
* Description: m_tracking_light_get
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
GPIO_PinState m_tracking_light_get(void* this)
{
	m_tracking_light* m_this = (m_tracking_light*)this;
	GPIO_PinState     state;
	
	/*检查this指针*/
	if(NULL == this)
	{
		log_error("Null ptr");
		return GPIO_PIN_RESET;
	}
	
	/*读取IO状态*/
	state = HAL_GPIO_ReadPin(m_this->m_gpio,m_this->m_pin);
	if(m_this->m_reverse)
	{
		state = !state;
	}
	
	return state;
}

#endif