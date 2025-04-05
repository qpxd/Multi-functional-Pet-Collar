/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : my_timer.c
* Description: 定时器模块 函数实现
* Author: xzh
* Version: v1.0
* Date: 2022/03/02
*****************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_TIMER_ENABLED

#define MODULE_NAME       "my_timer"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

typedef struct __TIMER_CFG
{  
    TIM_TypeDef*       m_timer_addr       ;   /*定时器地址*/
    GPIO_TypeDef*      m_gpio_ch_1   ;
    u32                m_gpio_ch_1   ;
    GPIO_TypeDef*      m_gpio_ch_2   ;
    u32                m_gpio_ch_2   ;
    GPIO_TypeDef*      m_gpio_ch_3   ;
    u32                m_gpio_ch_3   ;
    GPIO_TypeDef*      m_gpio_ch_4   ;
    u32                m_gpio_ch_4   ;
}timer_cfg;

typedef struct _M_TIMER
{
    bool               m_timer_init_state ;
    TIM_HandleTypeDef  m_timer_handle    ;
    TIM_OC_InitTypeDef m_timer_ch_handle ;
}m_timer;

const uart_cfg  g_my_timer_cfg[4] = {{TIM1,NULL ,0         ,NULL          ,0, NULL,          0,NULL,0,},
                                     {TIM2,GPIOA,GPIO_PIN_0,GPIOA,GPIO_PIN_1,GPIOA,GPIO_PIN_2,GPIOA,GPIO_PIN_3},
                                     {TIM3,GPIOA,GPIO_PIN_6,GPIOA,GPIO_PIN_7,GPIOA,GPIO_PIN_0,GPIOA,GPIO_PIN_1},
                                     {TIM4,GPIOA,GPIO_PIN_6,GPIOA,GPIO_PIN_7,GPIOA,GPIO_PIN_0,GPIOA,GPIO_PIN_1}};
m_timer g_my_timer[4];

static int m_init_ch(u8 timer,u32 ch);
static int m_init(u8 timer,u16 arr,u16 psc);

int m_init(u8 timer,u16 arr,u16 psc)
{
    /*参数检查*/
    if(timer > 4)
    {
        log_error("Param error.");
        return E_PARAM;
    }
	
	/*初始化定时器*/
	g_my_timer[timer].m_timer_handle.Instance           =  g_my_timer_cfg[timer]  ; //定时器
    g_my_timer[timer].m_timer_handle.Init.Prescaler     =  psc                    ; //定时器分频
    g_my_timer[timer].m_timer_handle.Init.CounterMode   =  TIM_COUNTERMODE_UP     ; //向上计数模式
    g_my_timer[timer].m_timer_handle.Init.Period        =  arr                    ; //自动重装载值
    g_my_timer[timer].m_timer_handle.Init.ClockDivision =  TIM_CLOCKDIVISION_DIV1 ;
	HAL_TIM_PWM_Init(&g_my_timer[timer].m_timer_handle);       	

    g_my_timer[timer].m_timer_init_state = true;

    return E_OK;
}

static int m_init_ch(u8 timer,u32 ch)
{
    /*参数检查*/
    if(timer > 4)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    else if(MY_TIMER_CH_1 != ch && MY_TIMER_CH_2 != ch && MY_TIMER_CH_3 != ch && MY_TIMER_CH_4 != ch)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查时钟初始化状态*/
    if(false == g_my_timer[timer].m_timer_init_state)
    {
        log_error("Need init timer first.");
        return E_ERROR;
    }

    /*配置定时器比较通道*/
}
#endif 

