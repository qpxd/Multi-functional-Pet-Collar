/*****************************************************************************
* Copyright:
* File name: int_manage.c
* Description: 中断管理模块 函数实现
* Author: 许少
* Version: V1.1
* Date: 2022/02/16
* log:  V1.0  2022/01/20
*       发布：
*
*       V1.1  2022/02/16
*       优化：增加DMA的中断管理。
*****************************************************************************/

#include "main.h"

#define MODULE_NAME       "int manage"

#ifdef  LOG_TAGE
#undef  LOG_TAGE
#endif
#define LOG_TAGE          MODULE_NAME


static int m_login_exti(uint32_t int_line,int_handle_func int_handle,void* param);
static int m_login_dma(uint32_t dma_cha,int_handle_func int_handle,void* param);
static int m_login_timer (uint32_t timer_cha,TIM_HandleTypeDef* timer,int_handle_func int_handle,void* param);
static void m_dma_exti_call_back(u8 dma_channal);

typedef struct __M_INT_FUNC_PARAM
{
    int_handle_func    m_func ;
    void*              m_param;
    TIM_HandleTypeDef* m_timer;
}m_func_param;

const c_int_manage int_manage = {m_login_exti,m_login_dma,m_login_timer};
static m_func_param g_exti_func_list[16];   /*外部中断回调函数列表*/
static m_func_param g_dma_func_list[7]  ;   /*外部中断回调函数列表*/
static m_func_param g_timer_func_list[4];   /*外部中断回调函数列表*/

void SystemIsr_PriorityInit(void)
{
    /*配置外部中断优先级*/
    HAL_NVIC_SetPriority (EXTI0_IRQn    ,EXTI_0_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI1_IRQn    ,EXTI_1_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI2_IRQn    ,EXTI_2_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI3_IRQn    ,EXTI_3_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI4_IRQn    ,EXTI_4_PRO    ,0); 
    HAL_NVIC_SetPriority (EXTI9_5_IRQn  ,EXTI_5_9_PRO  ,0); 
    HAL_NVIC_SetPriority (EXTI15_10_IRQn,EXTI_10_15_PRO,0); 

    /*使能所有的中断线*/
    HAL_NVIC_EnableIRQ  (EXTI0_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI1_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI2_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI3_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI4_IRQn     );
    HAL_NVIC_EnableIRQ  (EXTI9_5_IRQn   );
    HAL_NVIC_EnableIRQ  (EXTI15_10_IRQn );

	/*配置DMA中断优先等级*/
	HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, DMA_1_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, DMA_2_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, DMA_3_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, DMA_4_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, DMA_5_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, DMA_6_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, DMA_7_PRO, 0);	

	/*使能DMA所有的中断通道*/
	HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

    /*配置TIM中断优先等级*/
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, DMA_1_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, DMA_2_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, DMA_3_PRO, 0);
	HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, DMA_4_PRO, 0);

    /*配置TIM中断优先等级*/
    HAL_NVIC_EnableIRQ(TIM1_UP_IRQn);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
	HAL_NVIC_EnableIRQ(TIM3_IRQn);
	HAL_NVIC_EnableIRQ(TIM4_IRQn);
}

int m_login_exti(uint32_t int_line,int_handle_func int_handle,void* param)
{
u8 pin_num = 0;

    /*检查参数*/
    if(NULL == int_handle)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*检查是否已经被注册*/
    for(pin_num = 0;1 != int_line;int_line >>= 1)
    {
        ++pin_num;
    }
    if(NULL != g_exti_func_list[pin_num].m_func)
    {
        log_error("Registered bus.");
        return E_ERROR;
    }

    /*注册*/
    g_exti_func_list[pin_num].m_func = int_handle;
    g_exti_func_list[pin_num].m_param = param;

    return E_OK;
}

static int m_login_dma(uint32_t dma_cha,int_handle_func int_handle,void* param)
{

	/*检查参数*/
	if(NULL == int_handle)
	{
		log_error("Null pointer.");
		return E_NULL;
	}
	
	/*检查是否已经被注册*/
	if(NULL != g_dma_func_list[dma_cha].m_func)
	{
		log_error("Registered bus.");
		return E_ERROR;
	}

	/*注册*/
	g_dma_func_list[dma_cha].m_func = int_handle;
	g_dma_func_list[dma_cha].m_param = param;

	return E_OK;
}

static int m_login_timer (uint32_t timer_cha,TIM_HandleTypeDef* timer,int_handle_func int_handle,void* param)
{

	/*检查参数*/
	if(NULL == int_handle || NULL == timer)
	{
		log_error("Null pointer.");
		return E_NULL;
	}
	
	/*检查是否已经被注册*/
	if(NULL != g_timer_func_list[timer_cha].m_func)
	{
		log_error("Registered bus.");
		return E_ERROR;
	}

	/*注册*/
	g_timer_func_list[timer_cha].m_func = int_handle;
	g_timer_func_list[timer_cha].m_param = param;
    g_timer_func_list[timer_cha].m_timer = timer;

	return E_OK;    
}


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    uint32_t int_pin = GPIO_Pin;
    u8 pin_num = 0;

    for(pin_num = 0;1 != int_pin;int_pin >>= 1)
	{
		++pin_num;
	}

    /*执行对应的回调函数*/
    if(NULL != g_exti_func_list[pin_num].m_func)
    {
        g_exti_func_list[pin_num].m_func(GPIO_Pin,g_exti_func_list[pin_num].m_param);
    }
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    u8 timer_id = 0;

    for(timer_id = 0;timer_id < 4;++timer_id)
    {
        if(htim == g_timer_func_list[timer_id].m_timer)
        {
            break;
        }
    }

    if(timer_id > 4)
    {
        log_error("Error timer handle.");
        return;
    }

    /*执行对应的回调函数*/
    if(NULL != g_timer_func_list[timer_id].m_func)
    {
        g_timer_func_list[timer_id].m_func(timer_id,g_timer_func_list[timer_id].m_param);
    }
}

static void m_dma_exti_call_back(u8 dma_channal)
{
	/*执行对应的回调函数*/
    if(NULL != g_dma_func_list[dma_channal].m_func)
    {
        g_dma_func_list[dma_channal].m_func(dma_channal,g_dma_func_list[dma_channal].m_param);
    }
}

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);
}

void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);
}

void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);
}

void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);
}

void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);
}

void EXTI9_5_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_5))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_6))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_7))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_7);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_8))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_8);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_9))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_9);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    }
}


void EXTI15_10_IRQHandler(void)
{
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_10);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_11))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_11);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_12))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_12);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_13))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_14))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_14);
    }
    if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15))
    {
        HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_15);
    }
}

void DMA1_Channel1_IRQHandler(void)
{
	m_dma_exti_call_back(DMA1_CHA_1);
}
void DMA1_Channel2_IRQHandler(void)
{
	m_dma_exti_call_back(DMA1_CHA_2);
}
void DMA1_Channel3_IRQHandler(void)
{
	m_dma_exti_call_back(DMA1_CHA_3);
}
void DMA1_Channel4_IRQHandler(void)
{
	m_dma_exti_call_back(DMA1_CHA_4);
}
void DMA1_Channel5_IRQHandler(void)
{
	m_dma_exti_call_back(DMA1_CHA_5);
}
void DMA1_Channel6_IRQHandler(void)
{
	m_dma_exti_call_back(DMA1_CHA_6);
}
void DMA1_Channel7_IRQHandler(void)
{
	m_dma_exti_call_back(DMA1_CHA_7);
}

void TIM1_UP_IRQHandler(void)
{
    HAL_TIM_IRQHandler(g_timer_func_list[0].m_timer);
}

void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(g_timer_func_list[1].m_timer);
}

void TIM3_IRQHandler(void)
{
    HAL_TIM_IRQHandler(g_timer_func_list[2].m_timer);
}

void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(g_timer_func_list[3].m_timer);
}









