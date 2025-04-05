/*****************************************************************************
* Copyright:
* File name: servo.h
* Description: 舵机模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/01/18
*****************************************************************************/

#ifndef __SERVO_H__
#define __SERVO_H__



typedef struct __SERVO c_servo;

typedef struct __SERVO
{
    void*  this;
    
    /************************************************* 
    * Function: set 
    * Description: 设置舵机角度
    * Input : <this>  舵机对象
    *         <angle>  目标角度 角度范围为0.0f~180.0f
    * Output: <无>
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         ret = servo.set(&servo,55.0f);
    *         if(E_OK != ret)
    *         {
    *             log_error("servo set failed.");
    *         }
    *************************************************/  
    int (*set)(const c_servo* this,float angle);
}c_servo;

/************************************************* 
* Function: servo_create 
* Description: 创建一个舵机对象
* Input : <timer>  	定时器
		  <ch>  	定时器通道
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 引脚定义见 pwm 模块
* Demo  :
*         c_servo  servo  = {0};
*         servo = servo_create(SYS_TIME_2, SYS_TIME_CH1);
*         if(NULL == servo.this)
*         {
*             log_error("servo creat failed.");
*         }
*************************************************/
c_servo servo_create(sys_timer timer, sys_timer_ch ch);

#endif


