/*****************************************************************************
* Copyright:
* File name: servo.c
* Description: 舵机模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/01/18
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_SERVO_ENABLED

/* Macros -------------------------------------------------------------*/
#define MODULE_NAME       "servo"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

/* Private declare ---------------------------------------------------*/
static int m_set(const c_servo* this, float angle);

/* Typedef -----------------------------------------------------------*/
typedef struct __M_SERVO
{
	sys_timer 		timer;
	sys_timer_ch	ch;
}m_servo;

/* Public function prototypes------------------------------------------*/
c_servo servo_create(sys_timer timer, sys_timer_ch ch)
{
    c_servo  new = {0};
    m_servo* m_this = NULL;
    /*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_servo));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_servo));
	/*公有成员初始化*/
	new.set = m_set;
	/*私有成员初始化*/
	m_this 			= new.this;
	m_this->timer 	= timer;
	m_this->ch 		= ch;
	/*PWM初始化*/
	my_pwm.time_init(timer, 7199, 199);
	my_pwm.ch_init(timer, ch);
	my_pwm.ch_set(timer, ch, 0);
    /*返回新建的对象*/
    return new;
}

/* Private function prototypes------------------------------------------*/
static int m_set(const c_servo* this,float angle)
{
    const m_servo* m_this = NULL;
    u16 pul = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(0.0f > angle || 180.0f < angle)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

	/*
	周期 20ms    = 200
	0.5ms = 0°   = 5 
	1.5ms = 90°  = 15
	2.5ms = 180° = 25 
	*/
    pul = angle / 180.0f * 20.0f + 5.0f;

	my_pwm.ch_set(m_this->timer, m_this->ch, pul);

    return E_OK;
}

#endif