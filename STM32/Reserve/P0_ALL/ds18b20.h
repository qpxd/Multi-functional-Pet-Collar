/*****************************************************************************
* Copyright:
* File name: ds18b20.h
* Description: ds18b20温度传感器 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/01/17
*****************************************************************************/

#ifndef __DS18B20_H
#define __DS18B20_H 

   

typedef struct __DS18B20 c_ds18b20;

typedef struct __DS18B20
{
    void* this;

    /************************************************* 
    * Function: get 
    * Description: 获取当前温度
    * Input : <this>  ds18b20对象
    * Output: <temp>  当前温度 单位度 
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float temp = 0.0f;
    *
    *         ret = ds18b20.get(&ds18b20,&temp);
    *         if(E_OK != ret)
    *         {
    *             log_error("ds18b20 get failed.");
    *         }
    *************************************************/  
    int (*get)(const c_ds18b20* this,float* temp);
}c_ds18b20;

/************************************************* 
* Function: ds18b20_create 
* Description: 创建一个1750对象
* Input : <gpio>  数据线的gpio分组
*         <pin>   数据线的pin分组
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_ds18b20 ds18b20 = {0};
*         
*	      ds18b20 = ds18b20_create(GPIOB,GPIO_PIN_6);
*         if(NULL == ds18b20.this)
*         {
*             log_error("ds18b20 creat failed.");
*         }
*************************************************/
c_ds18b20 ds18b20_create(GPIO_TypeDef* gpio,uint32_t pin);


#endif















