/*****************************************************************************
* Copyright:
* File name: hc_sr04.h
* Description: 超声模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2021/12/29
*****************************************************************************/

#ifndef __HC_SR04_h__
#define __HC_SR04_h__



#define  NO_COMPENSATION  -99.9f   /*不需要温度补偿*/
#define  MAX_DIS_CM       100.0f   /*最大检测距离*/

typedef struct __HCSR04 c_hcsr04;

typedef struct __HCSR04
{
	void* this                                  ;  /*this 指针*/
    
    /************************************************* 
    * Function: get_dis 
    * Description: 获取传感器对象 当前距离
    * Input : <this>  超声对象
    *         <temp>  当前温度 可进行温度补偿，若不需要补偿 填入NO_COMPENSATION
    * Output: 无
    * Return: 当前距离 单位cm
    * Others: 无
    * Demo  :
    *         float dis = 0.0f;
    *
    *         dis = hcsr04.get_dis(&hcsr04,25.0f);
	*         log_inform("Dis:%.2fcm.",dis);
    *************************************************/    
	float (*get_dis)(c_hcsr04* this,float temp) ;  
}c_hcsr04;	


/************************************************* 
* Function: hcsr04_creat 
* Description: 创建一个超声对象
* Input : <trig_gpio>  超声trig脚  gpio分组
*         <trig_pin>   超声trig脚  pin 值
*         <echo_gpio>  超声echo脚  gpio分组
*         <echo_pin>   超声echo脚  pin 值  注：echo pin值只能是10~15
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_hcsr04 hcsr04 = {0};
*         
*	      hcsr04 = hcsr04_creat(GPIOB,GPIO_PIN_5,GPIOB,GPIO_PIN_12);
*	      if(NULL == hcsr04.this)
*	      {
*	      	 log_error("hcsr04 creat failed.");
*	      }
*************************************************/
c_hcsr04 hcsr04_creat(GPIO_TypeDef* trig_gpio,uint32_t trig_pin,GPIO_TypeDef* echo_gpio,uint32_t echo_pin);


#endif

