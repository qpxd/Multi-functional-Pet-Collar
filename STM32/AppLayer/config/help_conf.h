/**
  ******************************************************************************
  * @file    onchip_conf.h
  * @brief   
  ******************************************************************************
  * @attention
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __help_conf_H
#define __help_conf_H

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

/* Exported constants --------------------------------------------------------*/
#define HELP_ALG_ENABLED

#define HELP_DATACUT_ENABLED

#define HELP_LOG_ENABLED

#define HELP_PID_ENABLED

#define HELP_REG_ENABLED

#define HELP_MY_LIST_ENABLED

#define HELP_HASH_ENABLED
/* Typedef -------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/                                  
/**
  * @brief Include module's header file
  */
#ifdef HELP_ALG_ENABLED
  #include "algorithm.h" 
#endif 
                                              
#ifdef HELP_DATACUT_ENABLED
  #include "data_cut.h" 
#endif 

#ifdef HELP_LOG_ENABLED
  #include "log.h" 
#endif 

#ifdef HELP_PID_ENABLED
  #include "my_pid.h" 
#endif 

#ifdef HELP_REG_ENABLED
  #include "reg.h" 
#endif 

#ifdef HELP_MY_LIST_ENABLED
  #include "my_list.h" 
#endif  

#ifdef HELP_HASH_ENABLED
  #include "hash.h" 
#endif
/* Macros --------------------------------------------------------------------*/
                               
#ifdef __cplusplus
}
#endif

#endif /* __help_conf_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
