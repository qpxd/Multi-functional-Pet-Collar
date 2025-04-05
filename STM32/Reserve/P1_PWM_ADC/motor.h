/*****************************************************************************
* Copyright:
* File name: motor.h
* Description: motor模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/05/05
* log:  V1.0  2022/05/05
*       发布：
*****************************************************************************/

#ifndef __MOTOR_H__
#define __MOTOR_H__



#define MOTOR_MIN_RPM   5.0f  /*电机最小转速*/
#define MOTOR_MAX_RPM  40.0f  /*电机最大转速*/

typedef struct __MOTOR c_motor;

typedef struct __MOTOR_INIT_STRUCT
{
    sys_timer      o_encoder_timer    ;  /*编码器定时器*/
    sys_timer      o_pwm_timer        ;  /*pwm波定时器*/
    sys_timer_ch   o_pwm_ch           ;  /*pwm波定时器通道*/
    float          o_reduction_ratio  ;  /*减速比*/
    GPIO_TypeDef*  o_in1_gpio         ;  /*方向引脚1 gpio*/
    GPIO_TypeDef*  o_in2_gpio         ;  /*方向引脚1 pin */
    uint32_t       o_in1_pin          ;  /*方向引脚2 gpio*/
    uint32_t       o_in2_pin          ;  /*方向引脚2 pin */
}motor_init_struct;

typedef struct __MOTOR
{
    void*         this;

    /************************************************* 
    * Function: rotate 
    * Description: 电机定速旋转
    * Input : <c_motor>  电机对象
    *       ：<rpm>      转速 单位 转/min 
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *
    *         ret = motor.rotate(&motor,30.0f);
    *         if(E_OK != ret)
    *         {
    *             log_error("Motor set failed.");
    *         }
    *************************************************/     
    int       (*rotate)    (const c_motor* this,float rpm);
    
    /************************************************* 
    * Function: rotate_fix 
    * Description: 电机定速旋转指定角度
    * Input : <c_motor>  电机对象
    *       ：<rpm>      转速 单位 转/min 
    *       ：<angle>    需要旋转的角度 单位 度
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *
    *         ret = motor.rotate_fix(&motor,30.0f,360.0f);
    *         if(E_OK != ret)
    *         {
    *             log_error("Motor set failed.");
    *         }
    *************************************************/
    int       (*rotate_fix)(const c_motor* this,float rpm,float angle);
    
    /************************************************* 
    * Function: stop 
    * Description: 立即停止电机
    * Input : <c_motor>  电机对象
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *
    *         ret = motor.stop(&motor);
    *         if(E_OK != ret)
    *         {
    *             log_error("Motor stop failed.");
    *         }
    *************************************************/
    int       (*stop)      (const c_motor* this);
    
    /************************************************* 
    * Function: speed_cur 
    * Description: 获取电机当前转速
    * Input : <c_motor>  电机对象
    * Output: <rpm>      转速 单位 转/min 
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float rpm = 0.0f;
    *
    *         ret = motor.stop(&motor,&rpm);
    *         if(E_OK != ret)
    *         {
    *             log_error("Motor stop failed.");
    *         }
    *         log_inform("RPM:%.2f",rpm);
    *************************************************/
    int       (*speed_cur) (const c_motor* this,float* rpm);
}c_motor;

/************************************************* 
* Function: motor_create 
* Description: 创建一个新的电机对象
* Input : <cfg>  电机配置
* Output: 无
* Return: <E_OK>     操作成功
*         <E_NULL>   空指针
*         <E_ERROR>  操作失败
* Others: 无
* Demo  :
*         c_motor motor = {0}; 
*         motor_init_struct motor_cfg = {0};         
*
*         motor_cfg.o_encoder_timer   = SYS_TIME_2;
*         motor_cfg.o_pwm_timer       = SYS_TIME_3;
*         motor_cfg.o_pwm_ch          = SYS_TIME_CH3;
*         motor_cfg.o_reduction_ratio = 120.0f;
*         motor_cfg.o_in1_gpio        = GPIOB;
*         motor_cfg.o_in1_pin         = GPIO_PIN_12;
*         motor_cfg.o_in2_gpio        = GPIOB;
*         motor_cfg.o_in2_pin         = GPIO_PIN_13;
*
*         motor = motor_create(&motor_cfg);
*         if(NULL == motor.this)
*         {
*             log_error("Motor create failed.");
*         }
*************************************************/
c_motor motor_create(motor_init_struct* cfg);

#endif


