/**
  ******************************************************************************
  * @file           : moudle_test.h
  * @brief          : 
*******************************************************************************/
#ifndef __MOUDLE_TEST_H
#define __MOUDLE_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Macros ------------------------------------------------------------*/
/* Typedef ------------------------------------------------------------*/
typedef enum
{
    LOG_ITM = 0,
    LOG_UART,
    LOG_OLED,
    LOG_TFT18,
}log_type_t;

//typedef struct  xMOUDLE_TEST
//{
//    /*片上外设 ------------------------------------------------------------*/
//    struct 
//    {
//        c_my_iic    iic;        //iic测试对象


//    }onchip;
//    /*P0-通用IO模块 -------------------------------------------------------*/
//    struct 
//    {
//        c_ds18b20   ds18b20;
//        c_dht11     dht11;    
//        c_key_board key_board;
//        c_hx711     hx711;
//        c_uln2003   uln2003;
//        c_hcsr04    hcsr04;
//        c_mlx90614  mlx90614;
//        //DO  声音，红外对管，火焰，土壤湿度，雨水, MQ-X, 循迹灯
//        //LED & BEEP & FANS
//        //
//    }p0_all;
//    /*P1-PWM/ADC模块 -------------------------------------------------------*/
//    struct 
//    {
//        c_servo servo[4];   //舵机测试对象
//        c_gp2y  gp2y;
//        c_ph    ph;
//        //TS-300B
//        //MOTOR
//        //A0 MQ-X
//    }p1_pwm_adc;
//    /*P2-串口通信模块 -------------------------------------------------------*/
//    struct 
//    {
//        c_esp8266   esp8266;
//        c_jq8400    jq8400;
//        c_gps       gps;
//        c_sim800    sim800;
//        //su_o3t    //语音识别
//        //esp_can
//        //as608     指纹识别
//    }p2_uart;
//    /*P3-IIC通信模块---------------------------------------------------------*/
//    struct 
//    {
//        c_oled      oled;         //oled测试对象
//        c_bh1750    bh1750;          
//        c_bmp180    bmp180;
//        c_sgp30     sgp30;       //
//        c_tcs34725  tcs34725;    //RGB模块
//        c_sht30     sht30;
////        c_vl53l0x   vl53l0x;
//        c_ina219    ina219;  
//        c_ds3231    ds3231;
//        //PAJ7620
//        //MAX30102
//        //MPU6050
//    }p3_iic;
//    /*P4-SPI通信模块------------------------------------------------------------*/
//    struct 
//    {
//        c_tft18     tft18;
//        //rc522
//    }p4_spi;
//}moudle_test_t;


#pragma pack(1)

#pragma pack()


/* Exported constants --------------------------------------------------------*/
/* Exported functions prototypes ---------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
#ifdef __cplusplus
}
#endif
#endif /* __MOUDLE_TEST_H */