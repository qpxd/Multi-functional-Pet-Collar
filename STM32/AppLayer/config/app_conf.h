/**
  ******************************************************************************
  * @file    onchip_conf.h
  * @brief   
  ******************************************************************************
  * @attention
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __app_conf_H
#define __app_conf_H

#ifdef __cplusplus
 extern "C" {
#endif
 
/* Dependency file ------------------------------------------------------------*/
#include "help_conf.h"
#include "onchip_conf.h"
#include "driver_conf.h"

/* Exported constants --------------------------------------------------------*/
//#define APP_DIS_ENABLED

//#define APP_MOUDLE_ENABLED

#define APP_MY_APP_ENABLED

//#define APP_MY_INTERFACE_ENABLED

//#define APP_TRACK_CAR_ENABLED
/* Typedef -------------------------------------------------------------------*/

/* Includes ------------------------------------------------------------------*/                                  
/**
  * @brief Include module's header file
  */
#ifdef APP_DIS_ENABLED
  #include "dis_detection.h" 
#endif 
                                              
#ifdef APP_MOUDLE_ENABLED
  #include "moudle_test.h" 
#endif 

#ifdef APP_MY_APP_ENABLED
  #include "my_app.h" 
#endif 

#ifdef APP_MY_INTERFACE_ENABLED
  #include "my_interface.h" 
#endif 

#ifdef APP_TRACK_CAR_ENABLED
  #include "tracking_car.h" 
#endif 
   
/* Macros --------------------------------------------------------------------*/
                               
#ifdef __cplusplus
}
#endif

#endif /* __APP_conf_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
