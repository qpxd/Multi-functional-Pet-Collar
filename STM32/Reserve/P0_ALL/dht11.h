/*****************************************************************************
* Copyright:
* File name: dht11.h
* Description: 温湿度传感器公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/03/18
* log:  V1.0  2022/01/13
*       发布：
*
*       V1.1  2022/02/05
*       修复：m_get中没有临界态保护，会导致接收发生错误。
*
*       V1.2  2022/03/18
*       优化：m_start中，不同厂家的dht11检测不到启动电平，故启动低电平从20增加
*             到25，提示代码兼容性。
*****************************************************************************/

#ifndef __DHT11_H__
#define __DHT11_H__



typedef struct __DHT11 c_dht11;

typedef struct __DHT11
{
    void* this;
    
    /************************************************* 
    * Function: get 
    * Description: 获取温湿度
    * Input : <this>  dht11对象
    * Output: <temp>   当前温度 单位度
    *         <humi>   当前湿度 百分比
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         u8 temp = 0,humi = 0;
    *
    *         ret = dht11.get(&dht11,&temp,&humi);
    *         if(E_OK != ret)
    *         {
    *             log_error("dht11 get failed.");
    *         }
    *************************************************/  
    int (*get)(const c_dht11* this,u8* temp,u8* humi);
}c_dht11;

/************************************************* 
* Function: dht11_create 
* Description: 创建一个dht11对象
* Input : <gpio>  数据线的gpio分组
*         <pin>   数据线的pin分组
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_dht11  dht11  = {0};
*         
*         dht11 = dht11_create(GPIOA,GPIO_PIN_11);
*         if(NULL == dht11.this)
*         {
*             log_error("dht11 creat failed.");
*         }
*************************************************/
c_dht11 dht11_create(GPIO_TypeDef* gpio,uint32_t pin);



#endif





