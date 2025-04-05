/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : vl53l0x.h
* Description: vl53l0x tof测距模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/10
*****************************************************************************/

#ifndef __VL53L0X_H__
#define __VL53L0X_H__

#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"

#define VL53L0X_ADDR             0x29  //VL53L0X传感器上电默认IIC地址为0X29
#define VL53L0X_REACQUIR_TIMES      5  /*从新获取次数 vl53l0x 容易被外接环境干扰 从新获取是在被干扰时 最大从新获取次数 超出将返回失败*/
                                       /*最小为1 不可写0*/

//使能2.8V IO电平模式
#define USE_I2C_2V8  1

#define VL53L0X_MEASUREMENT_SINGLE      VL53L0X_DEVICEMODE_SINGLE_RANGING       /*单次测量*/
#define VL53L0X_MEASUREMENT_CONTINUOUS  VL53L0X_DEVICEMODE_CONTINUOUS_RANGING   /*循环测量*/

#define VL53L0X_MODE_DEFAULT   0 //默认    测量时间 30ms  1.2m 标准
#define VL53L0X_MODE_ACCURACY  1 //高精度  测量时间200ms  1.2m 精确测量 
#define VL53L0X_MODE_LONG      2 //长距离  测量时间 33ms  2.0m 长距离，但只适用于黑暗环境
#define VL53L0X_MODE_HIGH      3 //高速    测量时间 20ms  1.2m 高速，但精度较差

typedef struct __C_VL53L0X c_vl53l0x;

typedef struct __C_VL53L0X
{
    void* this;   
    
    /************************************************* 
    * Function: start 
    * Description: 启动距离转换
    * Input : <this>              vl53l0x对象
    *         <measurement_mode>  测量模式
    *                             (VL53L0X_MEASUREMENT_SINGLE)     单次测量，每次获取前会启动一次测量，因此单次获取时间较长 优点省电
    *                             (VL53L0X_MEASUREMENT_CONTINUOUS) 连续测量，启动后传感器会一直不停的转换，但首次启动会需要一定时间 单次获取快，但费电
    *         <data_mode>         数据模式
    *                             (VL53L0X_MODE_DEFAULT ) 默认    测量时间 30ms  1.2m 标准
    *                             (VL53L0X_MODE_ACCURACY) 高精度  测量时间200ms  1.2m 精确测量 
    *                             (VL53L0X_MODE_LONG    ) 长距离  测量时间 33ms  2.0m 长距离，但只适用于黑暗环境
    *                             (VL53L0X_MODE_HIGH    ) 高速    测量时间 20ms  1.2m 高速，但精度较差
    * Output: 无
    * Return: <E_OK>     启动成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         ret = vl53l0x.start(&vl53l0x,VL53L0X_MEASUREMENT_CONTINUOUS,VL53L0X_MODE_DEFAULT);
    *         if(E_OK != ret)
    *         {
    *             log_error("vl53l0 start failed.");
    *         }
    *************************************************/      
    int (*start)(const c_vl53l0x* this,u8 measurement_mode, u8 data_mode);
    
    /************************************************* 
    * Function: set_addr 
    * Description: 设置vl53l0x的新地址
    * Input : <this>  vl53l0x对象
    * Output: <addr>  新地址
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 该接口设置成功后 立即生效
    * Demo  :
    *         ret = vl53l0x.set_addr(&vl53l0x,0x11);
    *         if(E_OK != ret)
    *         {
    *             log_error("vl53l0 set addr failed.");
    *         }
    *************************************************/      
    int (*set_addr)(const c_vl53l0x* this,u8 addr);
    
    /************************************************* 
    * Function: reset 
    * Description: 复位vl53l0x
    * Input : <this>  vl53l0x对象
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 复位后，vl53l0x的地址会恢复成默认地址
    * Demo  :
    *         ret = vl53l0x.reset(&vl53l0x);
    *         if(E_OK != ret)
    *         {
    *             log_error("vl53l0 reset failed.");
    *         }
    *************************************************/  
    int (*reset)(const c_vl53l0x* this);
    
    /************************************************* 
    * Function: get 
    * Description: 获取当前距离
    * Input : <this>  vl53l0x对象
    * Output: <dis>   当前距离 单位mm
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         u16 dis = 0; 
    *
    *         ret = vl53l0x.get(&vl53l0x,&dis);
    *         if(E_OK != ret)
    *         {
    *             log_error("vl53l0 get failed.");
    *         }
    *         log_inform("DIS:%dmm.",dis);
    *************************************************/  
    int (*get)(const c_vl53l0x* this,u16* dis);
}c_vl53l0x;

/************************************************* 
* Function: vl53l0x_create 
* Description: 创建一个vl53l0x对象
* Input : <iic>         vl53l0x所在的iic总线
*         <addr>        vl53l0x的器件地址
*         <xshut_gpio>  电源脚GPIO分组
*         <xshut_pin>   电源脚PIN
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 地址可填写默认地址也可随意填写，函数将自行修改
* Demo  :
*         c_vl53l0x vl53l0x = {0}; 
*         
*	      vl53l0x = vl53l0x_create(&iic,0x29,GPIOA,GPIO_PIN_4);
*         if(NULL == vl53l0x.this)
*         {
*             log_error("vl53l0x creat failed.");
*         }
*************************************************/
c_vl53l0x vl53l0x_create(const c_my_iic* iic,u8 addr,GPIO_TypeDef* xshut_gpio,u32 xshut_pin);

#endif
