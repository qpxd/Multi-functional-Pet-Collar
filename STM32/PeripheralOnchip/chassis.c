/*****************************************************************************
* Copyright:
* File name: chassis.c
* Description: 底盘硬件 函数实现
* Author: 许少
* Version: V1.0
* Date: 2021/12/19
*****************************************************************************/
#ifdef ONCHIP_CHASSIS_ENABLED

#include "onchip_conf.h"

#define MODULE_NAME       "chassis"
#define MAX_SPEED         100         /*最大速度*/

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME



TIM_HandleTypeDef 	TIM4_Handler   ;    //定时器句柄 
TIM_OC_InitTypeDef 	TIM4_CHHandler;	//定时器4通道句柄

static void m_time1_init(u16 arr,u16 psc);

/*************************************************
* Function: 底盘初始化
* Description: chassis_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int chassis_init(void)
{
	m_time1_init(MAX_SPEED,71);
	
	chassis_work(CHASSIS_STOP,0);  /*默认停车*/
	
	return E_OK;
}

/*************************************************
* Function: 底盘工作
* Description: chassis_work
* Input : <type>  工作类型
*         <speed> 速度  0~100
* Output: 无
* Return:
* Others: 无
*************************************************/
int chassis_work(u8 type,u8 speed)
{
	if(speed > MAX_SPEED)
	{
		log_error("Speed too fast.");
		return E_PARAM;
	}
	
	switch(type)
	{
		/*停车*/
		case CHASSIS_STOP:
		{
			TIM4->CCR1 = 0;
			TIM4->CCR2 = 0;
			TIM4->CCR3 = 0;
			TIM4->CCR4 = 0;
			break;
		}
		
		/*倒车*/
		case CHASSIS_BACK:
		{
			/*左轮*/
		    #if LEFT_WHEEL_REVERSE
			TIM4->CCR1 = speed ;
			TIM4->CCR2 = 0     ;
			#else              
			TIM4->CCR1 = 0     ;
			TIM4->CCR2 = speed ;
			#endif
			
			/*右轮*/
			#if RIGHT_WHEEL_RIVERSE
			TIM4->CCR3 = speed ;
			TIM4->CCR4 = 0     ;
			#else
			TIM4->CCR3 = 0     ;
			TIM4->CCR4 = speed ;
			#endif
			break;
		}
		
		/*直走*/
		case CHASSIS_GO:
		{
			/*左轮*/
		    #if LEFT_WHEEL_REVERSE
			TIM4->CCR1 = 0     ;
			TIM4->CCR2 = speed ;
			#else              
			TIM4->CCR1 = speed ;
			TIM4->CCR2 = 0     ;
			#endif
			
			/*右轮*/
			#if RIGHT_WHEEL_RIVERSE
			TIM4->CCR3 = 0     ;
			TIM4->CCR4 = speed ;
			#else
			TIM4->CCR3 = speed ;
			TIM4->CCR4 = 0     ;
			#endif
			break;
		}
		
		/*左转*/
		case CHASSIS_LEFT:
		{
			/*左轮*/
		    #if LEFT_WHEEL_REVERSE
			TIM4->CCR1 = 0     ;
			TIM4->CCR2 = speed ;
			#else              
			TIM4->CCR1 = speed ;
			TIM4->CCR2 = 0     ;
			#endif
			
			/*右轮*/
			#if RIGHT_WHEEL_RIVERSE
			TIM4->CCR3 = speed ;
			TIM4->CCR4 = 0     ;
			#else
			TIM4->CCR3 = 0     ;
			TIM4->CCR4 = speed ;
			#endif
			break;
		}
		
		/*右转*/
		case CHASSIS_RIGHT:
		{
			/*左轮*/
		    #if LEFT_WHEEL_REVERSE
			TIM4->CCR1 = speed ;
			TIM4->CCR2 = 0     ;
			#else              
			TIM4->CCR1 = 0     ;
			TIM4->CCR2 = speed ;
			#endif
			
			/*右轮*/
			#if RIGHT_WHEEL_RIVERSE
			TIM4->CCR3 = 0     ;
			TIM4->CCR4 = speed ;
			#else
			TIM4->CCR3 = speed ;
			TIM4->CCR4 = 0     ;
			#endif
			break;
		}
		
		default:
		{
			TIM4->CCR1 = 0;
			TIM4->CCR2 = 0;
			TIM4->CCR3 = 0;
			TIM4->CCR4 = 0;
			log_error("Param error.");
			return E_PARAM;
		}
	}
	
	
	return E_OK;
}

static void m_time1_init(u16 arr,u16 psc)
{
	GPIO_InitTypeDef GPIO_Initure;
	
	__HAL_RCC_TIM4_CLK_ENABLE();			//使能定时器4
	__HAL_RCC_GPIOB_CLK_ENABLE();			//开启GPIOB时钟
	
	/*初始化要使用的GPIO*/
	GPIO_Initure.Pin   = GPIO_PIN_6 | 
	                     GPIO_PIN_7 | 
	                     GPIO_PIN_8 |
	                     GPIO_PIN_9            ;   //PB6 B7 B8 B9
	GPIO_Initure.Mode  = GPIO_MODE_AF_PP       ;   //复用推挽输出
	GPIO_Initure.Pull  = GPIO_PULLUP           ;   //上拉
	GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH  ;   //高速
	HAL_GPIO_Init(GPIOA,&GPIO_Initure); 	
	
	/*初始化定时器*/
	TIM4_Handler.Instance           =  TIM4                   ; //定时器1
  TIM4_Handler.Init.Prescaler     =  psc                    ; //定时器分频
  TIM4_Handler.Init.CounterMode   =  TIM_COUNTERMODE_UP     ; //向上计数模式
  TIM4_Handler.Init.Period        =  arr                    ; //自动重装载值
  TIM4_Handler.Init.ClockDivision =  TIM_CLOCKDIVISION_DIV1 ;
	HAL_TIM_PWM_Init(&TIM4_Handler);       	
    
  /*初始化PWM比较通道*/
  TIM4_CHHandler.OCMode          =  TIM_OCMODE_PWM1        ; //模式选择PWM1
  TIM4_CHHandler.Pulse           =  0                      ; //设置比较值,此值用来确定占空比，默认比较值为自动重装载值的一半,即占空比为50%
	#if PWM_REVIERSE
    TIM2_CHHandler.OCPolarity      =  TIM_OCPOLARITY_LOW     ; //输出比较极性为低 
	#else
	TIM4_CHHandler.OCPolarity      =  TIM_OCPOLARITY_HIGH    ; //输出比较极性为低 
	#endif
	
  HAL_TIM_PWM_ConfigChannel(&TIM4_Handler,&TIM4_CHHandler,TIM_CHANNEL_1);//配置TIM2通道1
	HAL_TIM_PWM_ConfigChannel(&TIM4_Handler,&TIM4_CHHandler,TIM_CHANNEL_2);//配置TIM2通道2
	HAL_TIM_PWM_ConfigChannel(&TIM4_Handler,&TIM4_CHHandler,TIM_CHANNEL_3);//配置TIM2通道3
	HAL_TIM_PWM_ConfigChannel(&TIM4_Handler,&TIM4_CHHandler,TIM_CHANNEL_4);//配置TIM2通道4
	
  HAL_TIM_PWM_Start(&TIM4_Handler,TIM_CHANNEL_1);//开启PWM通道1	
	HAL_TIM_PWM_Start(&TIM4_Handler,TIM_CHANNEL_2);//开启PWM通道2	
	HAL_TIM_PWM_Start(&TIM4_Handler,TIM_CHANNEL_3);//开启PWM通道3	
	HAL_TIM_PWM_Start(&TIM4_Handler,TIM_CHANNEL_4);//开启PWM通道4	
}
#endif