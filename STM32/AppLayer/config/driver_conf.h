/**
  ******************************************************************************
  * @file    onchip_conf.h
  * @brief   
  ******************************************************************************
  * @attention
  ******************************************************************************
  */ 

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DRIVER_conf_H
#define __DRIVER_conf_H

#ifdef __cplusplus
 extern "C" {
#endif
 
/* Dependency file ------------------------------------------------------------*/
#include "help_conf.h"
#include "onchip_conf.h"

/* Exported constants --------------------------------------------------------*/
/* P0 - ALL ----------------------*/    
#define DRIVER_SWITCH_ENABLED
//#define DRIVER_DHT11_ENABLED
#define DRIVER_DS18B20_ENABLED
#define DRIVER_HCSR04_ENABLED
//#define DRIVER_HX711_ENABLED
#define DRIVER_KEY_BOARD_ENABLED
//#define DRIVER_MLX90614_ENABLED
//#define DRIVER_TRACK_ENABLED
//#define DRIVER_ULN2003_ENABLED
//#define DRIVER_DS1302_ENABLED
//#define DRIVER_MATRIX_KEYBOARD_ENABLED
/* P1 - PWM|ADC ----------------------*/   
//#define DRIVER_ENCODER_ENABLED
//#define DRIVER_GP2Y101_ENABLED
//#define DRIVER_MOTOR_ENABLED
//#define DRIVER_PH_ENABLED
//#define DRIVER_SERVO_ENABLED
/* P2 - UART ----------------------*/   
#define DRIVER_ESP8266_ENABLED
#define DRIVER_GPS_ENABLED
//#define DRIVER_JQ8400_ENABLED
//#define DRIVER_SIM800_ENABLED
//#define DRIVER_HLKV40_ENABLED
//#define DRIVER_AT_ENABLED
//#define DRIVER_AS608_ENABLED
//#define DRIVER_EC600_ENABLED
/* P3 - IIC ----------------------*/   
//#define DRIVER_AM2320_ENABLED
//#define DRIVER_BH1750_ENABLED
//#define DRIVER_BMP180_ENABLED
//#define DRIVER_DS3231_ENABLED
//#define DRIVER_SGP30_ENABLED
//#define DRIVER_INA219_ENABLED
//#define DRIVER_MAX30102_ENABLED
#define DRIVER_OLED_ENABLED
//#define DRIVER_SHT30_ENABLED
//#define DRIVER_TCS34725_ENABLED
#define DRIVER_MPU6050_ENABLED
//#define DRIVER_VL53L0X_ENABLED
/* P4 - SPI ----------------------*/ 
//#define DRIVER_RC522_ENABLED
//#define DRIVER_TFT18_ENABLED
//#define DRIVER_TOUCH_ENABLED
//#define DRIVER_SD_CARD_ENABLED

/*Third part*/ 
//#define DRIVER_FATFS_ENABLED
/* Typedef -------------------------------------------------------------------*/
typedef struct xSYS_TIME_T
{
    uint16_t uxYear;
    uint8_t ucMon;
    uint8_t ucDate;
    uint8_t ucHour;
    uint8_t ucMin;
    uint8_t ucSec;
    uint8_t ucReserve;
}sys_time_t;

/* Includes ------------------------------------------------------------------*/   
#ifdef DRIVER_FATFS_ENABLED
	#include "ff.h"	
	#include "exfuns/exfuns.h" 
#endif 

/* P0 - ALL ----------------------*/            
#ifdef DRIVER_SWITCH_ENABLED
  #include "switch.h" 
#endif 

#ifdef DRIVER_DHT11_ENABLED
  #include "dht11.h" 
#endif 

#ifdef DRIVER_DS18B20_ENABLED
  #include "ds18b20.h" 
#endif 

#ifdef DRIVER_HCSR04_ENABLED
  #include "hc_sr04.h" 
#endif 

#ifdef DRIVER_HX711_ENABLED
  #include "hx711.h" 
#endif 
   
#ifdef DRIVER_KEY_BOARD_ENABLED
  #include "key_board.h" 
#endif 

#ifdef DRIVER_MLX90614_ENABLED
  #include "mlx90614.h" 
#endif 

#ifdef DRIVER_TRACK_ENABLED
  #include "tracking_light.h" 
#endif 

#ifdef DRIVER_ULN2003_ENABLED
  #include "uln2003.h" 
#endif 

#ifdef DRIVER_DS1302_ENABLED
 #include "ds1302.h"
#endif

#ifdef DRIVER_MATRIX_KEYBOARD_ENABLED
 #include "matrix_keyboard.h"
#endif
/* P1 - PWM|ADC ----------------------*/   
#ifdef DRIVER_ENCODER_ENABLED
  #include "encoder.h" 
#endif 

#ifdef DRIVER_GP2Y101_ENABLED
  #include "gp2y101.h" 
#endif 

#ifdef DRIVER_MOTOR_ENABLED
  #include "motor.h" 
#endif 

#ifdef DRIVER_HX711_ENABLED
  #include "hx711.h" 
#endif 
   
#ifdef DRIVER_PH_ENABLED
  #include "ph.h" 
#endif 

#ifdef DRIVER_SERVO_ENABLED
  #include "servo.h" 
#endif 

/* P2 - UART ----------------------*/   
#ifdef DRIVER_ESP8266_ENABLED
  #include "esp8266.h" 
#endif 

#ifdef DRIVER_GPS_ENABLED
  #include "gps.h" 
#endif 

#ifdef DRIVER_JQ8400_ENABLED
  #include "jq8400.h" 
#endif 
   
#ifdef DRIVER_SIM800_ENABLED
  #include "sim800.h" 
#endif 

#ifdef DRIVER_HLKV40_ENABLED
	#include "hlk_v40.h" 
#endif 

#ifdef DRIVER_AT_ENABLED
	#include "at.h" 
#endif 

#ifdef DRIVER_AS608_ENABLED
	#include "as608.h" 
#endif 

#ifdef DRIVER_EC600_ENABLED
	#include "ec600.h" 
#endif 
/* P3 - IIC ----------------------*/   
#ifdef DRIVER_AM2320_ENABLED
  #include "am2320.h" 
#endif 

#ifdef DRIVER_BH1750_ENABLED
  #include "bh1750.h" 
#endif 

#ifdef DRIVER_BMP180_ENABLED
  #include "bmp180.h" 
#endif 
   
#ifdef DRIVER_DS3231_ENABLED
  #include "ds3231.h" 
#endif 

#ifdef DRIVER_SGP30_ENABLED
  #include "gy_sgp30.h" 
#endif 

#ifdef DRIVER_INA219_ENABLED
  #include "ina219.h" 
#endif 

#ifdef DRIVER_MAX30102_ENABLED
  #include "max30102.h" 
#endif 
   
#ifdef DRIVER_OLED_ENABLED
  #include "oled.h" 
#endif 

#ifdef DRIVER_SHT30_ENABLED
  #include "sht30.h" 
#endif 

#ifdef DRIVER_TCS34725_ENABLED
  #include "tcs34725.h" 
#endif 

#ifdef DRIVER_MPU6050_ENABLED
  #include "mpu6050.h" 
#endif 

#ifdef DRIVER_VL53L0X_ENABLED
  #include "vl53l0x.h" 
#endif 
/* P4 - SPI ----------------------*/ 
#ifdef DRIVER_RC522_ENABLED
  #include "rc522.h" 
#endif 
   
#ifdef DRIVER_TFT18_ENABLED
  #include "tft18.h" 
#endif 

#ifdef DRIVER_TOUCH_ENABLED
  #include "touch.h" 
#endif 

#ifdef DRIVER_SD_CARD_ENABLED
  #include "my_sd_card.h"
#endif 
/* Macros --------------------------------------------------------------------*/
                               
#ifdef __cplusplus
}
#endif

#endif /* __DRIVER_conf_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
