/**
******************************************************************************
* @file           : main.c
* @brief          : Main program body
******************************************************************************
*/

#include "main.h"

#ifdef SYSTEM_FREERTOS_ENABLED
static TaskHandle_t      my_app_init_handle ;  /*任务句柄*/
void vAppFreeRTOSStartUp(void)
{

#if configSUPPORT_STATIC_ALLOCATION
    AppTaskCreate_Handle = xTaskCreateStatic((TaskFunction_t )AppTaskCreate,
                                            (const char* )"AppTaskCreate",      
                                            (uint32_t )128,                     
                                            (void* )NULL,                      
                                            (UBaseType_t )3,                   
                                            (StackType_t* )AppTaskCreate_Stack,
                                            (StaticTask_t* )&AppTaskCreate_TCB);
    if(NULL != AppTaskCreate_Handle)   vTaskStartScheduler(); 
#else
    BaseType_t xReturn = pdPASS;
    xReturn = xTaskCreate((TaskFunction_t )my_app_init,          
                          (const char* )"my_app_init",           
                          (uint16_t )128,                          
                          (void* )NULL,                             
                          (UBaseType_t )1,                          
                          (TaskHandle_t* )&my_app_init_handle);  
    
    if(pdPASS == xReturn)               vTaskStartScheduler(); 
#endif

}
#endif

int main()
{
	HAL_Init();                    	 	//初始化HAL库   
	Stm32_Clock_Init(RCC_PLL_MUL9);   	//设置时钟,72M
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_TIM1_CLK_ENABLE();
	__HAL_RCC_TIM2_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();
	__HAL_RCC_TIM4_CLK_ENABLE();
	__HAL_RCC_DMA1_CLK_ENABLE();
	__HAL_RCC_AFIO_CLK_ENABLE();		//使能io复用时钟	
	
	delay_init(72);               		//初始化延时函数
	log_init();
	SystemIsr_PriorityInit();
	
#ifdef SYSTEM_FREERTOS_ENABLED
	vAppFreeRTOSStartUp();
#endif
	
	return(1);
}




