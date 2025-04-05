/*****************************************************************************
* Copyright:
* File name: my_pwm.c
* Description: my_pwm模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/04/29
* log:  V1.0  2022/04/29
*       发布：
*****************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_PWM_ENABLED

#define MODULE_NAME       "my_pwm"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

/*编码器 配置信息*/
typedef struct __TIMER_CH_CFG
{
    TIM_TypeDef*      m_timer_addr;
    GPIO_TypeDef*     m_ch_gpio[SYS_TIME_CH_NUM] ; 
    u32               m_ch_pin[SYS_TIME_CH_NUM]  ;
}timer_ch_cfg;


/*my_pwm 对象*/
typedef struct __M_MY_PWM
{
    TIM_HandleTypeDef        m_timer       ; 
    bool                     m_timer_init  ;
    TIM_OC_InitTypeDef       m_ch[SYS_TIME_CH_NUM]         ;
    bool                     m_ch_init[SYS_TIME_CH_NUM]    ;
}m_my_pwm;

static const timer_ch_cfg g_cfg[4] = {
{TIM1  , GPIOA , GPIOA , GPIOA , GPIOA , GPIO_PIN_8  , GPIO_PIN_9  , GPIO_PIN_10 , GPIO_PIN_11},
//{TIM1  , GPIOB , GPIOB , GPIOB , GPIOB , GPIO_PIN_12 , GPIO_PIN_13 , GPIO_PIN_14 , GPIO_PIN_15},
{TIM2  , GPIOA , GPIOA , GPIOA , GPIOA , GPIO_PIN_0  , GPIO_PIN_1  , GPIO_PIN_2  , GPIO_PIN_3 },
{TIM3  , GPIOA , GPIOA , GPIOB , GPIOB , GPIO_PIN_6  , GPIO_PIN_7  , GPIO_PIN_0  , GPIO_PIN_1 },
{TIM4  , GPIOB , GPIOB , GPIOB , GPIOB , GPIO_PIN_6  , GPIO_PIN_7  , GPIO_PIN_8  , GPIO_PIN_9 }
//{TIM1  , GPIOB , GPIO_PIN_13 , GPIOB , GPIO_PIN_14 , GPIOB , GPIO_PIN_15 , GPIOA , GPIO_PIN_11},
//{TIM2  , GPIOA , GPIO_PIN_0  , GPIOA , GPIO_PIN_1  , GPIOA , GPIO_PIN_2  , GPIOA , GPIO_PIN_3 },
//{TIM3  , GPIOA , GPIO_PIN_6  , GPIOA , GPIO_PIN_7  , GPIOB , GPIO_PIN_0  , GPIOB , GPIO_PIN_1 },
//{TIM4  , GPIOB , GPIO_PIN_6  , GPIOB , GPIO_PIN_7  , GPIOB , GPIO_PIN_8  , GPIOB , GPIO_PIN_9 }
};

static  int m_time_init (sys_timer timer,u16 psc,u16 arr);
static  int m_ch_init       (sys_timer timer,sys_timer_ch ch);
static  int m_ch_set        (sys_timer timer,sys_timer_ch ch,u16 duty_cycle);

const c_my_pwm my_pwm = {m_time_init,m_ch_init,m_ch_set};
static const u32  g_ch_value[SYS_TIME_CH_NUM] = {TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_3,TIM_CHANNEL_4};
static m_my_pwm   g_my_pwm[SYS_TIME_NUM] = {0};


static  int m_time_init (sys_timer timer,u16 psc,u16 arr)
{
    /*检查参数*/
    if(timer >= SYS_TIME_NUM)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*初始化定时器*/
    g_my_pwm[timer].m_timer.Instance           =  g_cfg[timer].m_timer_addr ; //定时器
    g_my_pwm[timer].m_timer.Init.Prescaler     =  psc                       ; //定时器分频
    g_my_pwm[timer].m_timer.Init.CounterMode   =  TIM_COUNTERMODE_UP        ; //向上计数模式
    g_my_pwm[timer].m_timer.Init.Period        =  arr                       ; //自动重装载值
    g_my_pwm[timer].m_timer.Init.ClockDivision =  TIM_CLOCKDIVISION_DIV1    ;
    HAL_TIM_PWM_Init(&g_my_pwm[timer].m_timer); 

    /*标记为定时器已经初始化*/
    g_my_pwm[timer].m_timer_init = true;

    return E_OK;
}

static  int m_ch_init       (sys_timer timer,sys_timer_ch ch)
{
    GPIO_InitTypeDef gpio = {0};

    /*检查参数*/
    if(timer >= SYS_TIME_NUM || ch >= SYS_TIME_CH_NUM)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查定时器初始化状态*/
    if(true != g_my_pwm[timer].m_timer_init)
    {
        log_error("Need init timer first.");
        return E_ERROR;
    }

    /*初始化gpio*/
    gpio.Pin   = g_cfg[timer].m_ch_pin[ch];
    gpio.Mode  = GPIO_MODE_AF_PP       ;   //复用推挽输出
    gpio.Pull  = GPIO_PULLUP           ;   //上拉
    gpio.Speed = GPIO_SPEED_FREQ_HIGH  ;   //高速
    HAL_GPIO_Init(g_cfg[timer].m_ch_gpio[ch],&gpio); 

    /*初始化定时器通道*/
    g_my_pwm[timer].m_ch[ch].OCMode     =  TIM_OCMODE_PWM1        ; //模式选择PWM1
    g_my_pwm[timer].m_ch[ch].Pulse      =  0                      ; //设置比较值 默认为0
    g_my_pwm[timer].m_ch[ch].OCPolarity =  TIM_OCPOLARITY_HIGH    ; //输出比较极性为低
	
//	g_my_pwm[timer].m_ch[ch].OCNIdleState  = TIM_OCNIDLESTATE_RESET;
//	g_my_pwm[timer].m_ch[ch].OCNPolarity  = TIM_OCPOLARITY_LOW;		//互补通道输出比较极性
	
    HAL_TIM_PWM_ConfigChannel(&g_my_pwm[timer].m_timer,&g_my_pwm[timer].m_ch[ch],g_ch_value[ch]);
    HAL_TIM_PWM_Start(&g_my_pwm[timer].m_timer,g_ch_value[ch]);
    
	
//	/* 启动定时器互补通道 PWM 输出 */
//	HAL_TIMEx_PWMN_Start(&g_my_pwm[timer].m_timer, g_ch_value[ch]);
	
    /*通道标记为初始化*/
    g_my_pwm[timer].m_ch_init[ch] = true;
    
    return E_OK;
}

static  int m_ch_set        (sys_timer timer,sys_timer_ch ch,u16 duty_cycle)
{
    /*检查参数*/
    if(timer >= SYS_TIME_NUM || ch >= SYS_TIME_CH_NUM)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查定时器初始化状态*/
    if(true != g_my_pwm[timer].m_timer_init)
    {
        log_error("Need init timer first.");
        return E_ERROR;
    }

    /*检查通道初始化状态*/
    if(true != g_my_pwm[timer].m_ch_init[ch])
    {
        log_error("Need init timer ch first.");
        return E_ERROR;
    }

    /*比较值不可比重转载值大*/
    if(duty_cycle > g_my_pwm[timer].m_timer.Init.Period)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*设置重装载值*/
    __HAL_TIM_SET_COMPARE(&g_my_pwm[timer].m_timer,g_ch_value[ch],duty_cycle);

    return E_OK;
}
#endif





