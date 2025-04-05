/**
  ******************************************************************************
  * @file    onchip_conf.h
  * @brief   
  ******************************************************************************
  * @attention
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __ONCHIP_conf_H
#define __ONCHIP_conf_H

#ifdef __cplusplus
 extern "C" {
#endif
 
/* Dependency file ------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <math.h>
	 
#include "stm32f1xx.h"
#include "stm32_hal_legacy.h"
#include "stm32f1xx_hal.h"

#include "sys_cfg.h"
#include "sys.h"
#include "int_manage.h"
#include "delay.h"
   
#ifdef SYSTEM_FREERTOS_ENABLED
	#include <FreeRTOSConfig.h>
	#include "FreeRTOS.h"              
	#include "task.h"
	#include "queue.h"
	#include "semphr.h"
	#include "event_groups.h"
#endif

#include "help_conf.h"
/* Exported constants --------------------------------------------------------*/
// #ifdef ONCHIP_CHASSIS_ENABLED

#define ONCHIP_ADC_ENABLED

#define ONCHIP_IIC_ENABLED

#define ONCHIP_PWM_ENABLED

#define ONCHIP_SPI_ENABLED

// #define ONCHIP_TIMER_ENABLED

#define ONCHIP_UART_ENABLED

//#define ONCHIP_SMBUS_ENABLED

#define ONCHIP_FLASH_ENABLED

#define ONCHIP_FLASH_NVS_ENABLED
/* Typedef -------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/                                  
/**
  * @brief Include module's header file
  */
#ifdef ONCHIP_CHASSIS_ENABLED
  #include "chassis.h" 
#endif 

#ifdef ONCHIP_ADC_ENABLED
  #include "my_adc.h" 
#endif 
                                              
#ifdef ONCHIP_IIC_ENABLED
  #include "my_iic.h" 
#endif 

#ifdef ONCHIP_PWM_ENABLED
  #include "my_pwm.h" 
#endif 

#ifdef ONCHIP_SPI_ENABLED
  #include "my_spi.h" 
#endif 
  
#ifdef ONCHIP_TIMER_ENABLED
  #include "my_timer.h" 
#endif 

#ifdef ONCHIP_UART_ENABLED
  #include "my_uart.h" 
#endif 

#ifdef ONCHIP_SMBUS_ENABLED
  #include "smbus.h" 
#endif 

#ifdef ONCHIP_FLASH_ENABLED
  #include "my_flash.h" 
#endif 
/* Macros --------------------------------------------------------------------*/
                               
#ifdef __cplusplus
}
#endif

#endif /* __ONCHIP_conf_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
