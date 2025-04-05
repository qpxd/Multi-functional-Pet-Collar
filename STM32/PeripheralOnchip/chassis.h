/*****************************************************************************
* Copyright:
* File name: chassis.c
* Description: 底盘硬件 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2021/12/19
*****************************************************************************/
#ifndef __CHASSIS_H__
#define __CHASSIS_H__




#define LEFT_WHEEL_REVERSE  0  /*左轮反向*/
#define RIGHT_WHEEL_RIVERSE 1  /*右轮反向*/
#define PWM_REVIERSE        0  /*PWM波反向*/

#define CHASSIS_STOP        0  
#define CHASSIS_GO          1
#define CHASSIS_BACK        2
#define CHASSIS_LEFT        3
#define CHASSIS_RIGHT       4

/*************************************************
* Function: 底盘初始化
* Description: chassis_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int chassis_init(void);

/*************************************************
* Function: 底盘工作
* Description: chassis_work
* Input : <type>  工作类型
*         <speed> 速度  0~100
* Output: 无
* Return:
* Others: 无
*************************************************/
int chassis_work(u8 type,u8 speed);

#endif


