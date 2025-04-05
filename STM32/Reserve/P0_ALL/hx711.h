/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : hx711.h
* Description: hx711称重模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/08
*****************************************************************************/

#ifndef __HX711_H__
#define __HX711_H__

//校准参数
//因为不同的传感器特性曲线不是很一致，因此，每一个传感器需要矫正这里这个参数才能使测量值很准确。
//当发现测试出来的重量偏大时，增加该数值。
//如果测试出来的重量偏小时，减小改数值。
//该值可以为小数
#define GAPVALUE 106.5f


typedef struct _C_HX711 c_hx711;

typedef struct _C_HX711
{
    void* this;

    /************************************************* 
    * Function: zero 
    * Description: hx711校零
    * Input : <this>  hx711对象
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 新创建的hx711 必须进行一次校零 否则无法获取重量信息
    *         获取零点的时间在1s~40s之间 不等
    * Demo  :  
    *         ret = hx711.zero(&hx711);
    *         if(E_OK != ret)
    *         {
    *             log_error("hx711 get zero failed.");
    *         }
    *************************************************/  
    int (*zero)(const c_hx711* this);
    
        /************************************************* 
    * Function: get 
    * Description: 获取当前重量
    * Input : <this>   tft18对象
    * Output: <weight> 当前重量 单位克
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float weight = 0.0f;
    *
    *         ret = hx711.get(&hx711,&weight);
    *         if(E_OK != ret)
    *         {
    *             log_error("hx711 get failed.");
    *         }
    *************************************************/  
    int (*get) (const c_hx711* this,float* weight);
}c_hx711;

/************************************************* 
* Function: hx711_create 
* Description: 创建一个hx711对象
* Input : <sck_gpio>  sck脚gpio分组
*         <sck_pin>   sck脚pin     
*         <dio_gpio>  dio脚gpio分组
*         <dio_pin>   dio脚pin
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_hx711 hx711 = {0};
*         
*	      hx711 = hx711_create(GPIOB,GPIO_PIN_6,GPIOB,GPIO_PIN_7);
*         if(NULL == hx711.this)
*         {
*             log_error("hx711 creat failed.");
*         }
*************************************************/
c_hx711 hx711_create(GPIO_TypeDef* sck_gpio,u32 sck_pin,GPIO_TypeDef* dio_gpio,u32 dio_pin);


#endif


