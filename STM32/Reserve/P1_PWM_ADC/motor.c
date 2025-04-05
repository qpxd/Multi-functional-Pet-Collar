/*****************************************************************************
* Copyright:
* File name: motor.c
* Description: motor模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/05/05
* log:  V1.0  2022/05/05
*       发布：
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_MOTOR_ENABLED

#define MODULE_NAME       "motor"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define MOTOR_ENCODER_PULSES_NUM  44   /*电机转一圈 脉冲数*/

#define MOTOR_STOP        0
#define MOTOR_CONST_SPEED 1
#define MOTOR_CONST_DIS   2

#define DF_KP   16.0f
#define DF_KI   1.0f
#define DF_KD   12.0f
#define DF_KT   30

#define TIMER_ARR   1000  /*重装载值*/
#define TIMER_PSC      4  /*分频系数*/

typedef struct __M_MOTOR
{
    TimerHandle_t  m_timer            ;  /*软件定时器*/
    sys_timer      m_encoder_timer    ;  /*编码器定时器*/
    sys_timer      m_pwm_timer        ;  /*pwm波定时器*/
    sys_timer_ch   m_pwm_ch           ;  /*pwm波定时器通道*/
    float          m_reduction_ratio  ;  /*减速比*/
    c_switch       m_in1              ;
    c_switch       m_in2              ;
    c_my_pid       m_pid              ;

    u8             m_mode             ;  /*工作模式*/
    int            m_old_encoder_count;
    float          m_set_speed        ;
    float          m_cur_speed        ;
    float          m_rem_dis          ;
}m_motor;

static int  m_rotate    (const c_motor* this,float rpm);
static int  m_rotate_fix(const c_motor* this,float rpm,float angle);
static int  m_stop      (const c_motor* this);
static int  m_speed_cur (const c_motor* this,float* rpm);
static void  timer_call_back( TimerHandle_t xTimer );


c_motor motor_create(motor_init_struct* cfg)
{
    c_motor  new = {0};
    m_motor* m_this = NULL; 
    int ret = 0;
    BaseType_t ret_os = pdFALSE;

    /*参数检测*/
    if(NULL == cfg)
    {
        log_error("Null pointer.");
        return new;
    }

    /*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_motor));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_motor));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_encoder_timer   = cfg->o_encoder_timer   ;
    m_this->m_pwm_timer       = cfg->o_pwm_timer       ;
    m_this->m_pwm_ch          = cfg->o_pwm_ch          ;
    m_this->m_reduction_ratio = cfg->o_reduction_ratio ;
    m_this->m_old_encoder_count = 1;
    
    new.rotate     = m_rotate     ;
    new.rotate_fix = m_rotate_fix ;
    new.speed_cur  = m_speed_cur  ;
    new.stop       = m_stop       ;
    
    /*配置编码器*/
    ret = encoder.init(cfg->o_encoder_timer);
    if(E_OK != ret)
    {
        log_error("Encoder timer init failed.");
        goto error_handle;
    }

    /*配置定时器*/
    ret = my_pwm.time_init(cfg->o_pwm_timer,TIMER_PSC - 1,TIMER_ARR - 1);
    if(E_OK != ret)
    {
        log_error("Pwm timer init failed.");
        goto error_handle;
    }

    /*配置定时器通道*/
    ret = my_pwm.ch_init(cfg->o_pwm_timer,cfg->o_pwm_ch);
    if(E_OK != ret)
    {
        log_error("Pwm ch init failed.");
        goto error_handle;
    }

    /*创建pid 控制*/
    m_this->m_pid = my_pid_create(DF_KP,DF_KI,DF_KD,TIMER_ARR,700);
    if(NULL == m_this->m_pid.this)
    {
        log_error("Pid creat failed.");
        goto error_handle;
    }

    /*创建switch*/
    m_this->m_in1 = switch_create(cfg->o_in1_gpio,cfg->o_in1_pin);
    if(NULL == m_this->m_in1.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }
    m_this->m_in2 = switch_create(cfg->o_in2_gpio,cfg->o_in2_pin);
    if(NULL == m_this->m_in2.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }

    /*默认停止电机*/
    ret = m_this->m_in1.set(&m_this->m_in1,SWITCH_LOW);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }
    ret = m_this->m_in2.set(&m_this->m_in2,SWITCH_LOW);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }
  
    /*创建软件定时器*/
    m_this->m_timer = xTimerCreate("motor timer" ,      /*定时器名称*/
                         DF_KT         ,      /*默认时间*/
                         pdTRUE        ,      /*循环定时器 */
                         new.this      ,      /*把私有成员传到定时器回调里头*/
                         timer_call_back);
    /*创建情况*/
    if(NULL == m_this->m_timer)
    {
        log_error("Timer creat failed.");
        goto error_handle;
    }
    
    /*启动定时器*/
    ret_os = xTimerStart(m_this->m_timer,100);
    if(pdPASS != ret_os)
    {
        log_error("Timer start failed.");
        goto error_handle;
    }

    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int  m_rotate    (const c_motor* this,float rpm)
{
    m_motor*    m_this = NULL;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    if(rpm < MOTOR_MIN_RPM || MOTOR_MAX_RPM > rpm)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    m_this->m_mode = MOTOR_CONST_SPEED;
    m_this->m_set_speed = rpm ;
    m_this->m_rem_dis   = 0.0f;

    return E_OK;
}

static int  m_rotate_fix(const c_motor* this,float rpm,float angle)
{
    m_motor*    m_this = NULL;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    if(rpm < MOTOR_MIN_RPM || MOTOR_MAX_RPM > rpm)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    m_this->m_mode = MOTOR_CONST_DIS;
    m_this->m_set_speed = rpm  ;
    m_this->m_rem_dis   = angle;

    return E_OK;

}


static int  m_stop      (const c_motor* this)
{
    m_motor*    m_this = NULL;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    m_this->m_mode = MOTOR_STOP;
    m_this->m_set_speed = 0.0f;
    m_this->m_rem_dis   = 0.0f;

    return E_OK;
}

static int  m_speed_cur (const c_motor* this,float* rpm)
{
    m_motor*    m_this = NULL;

    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == rpm)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    *rpm = m_this->m_cur_speed;
    
    return E_OK;
}

static void  timer_call_back( TimerHandle_t xTimer )
{
    m_motor*  m_this = NULL;
    int num_of_pulses = 0;
    int pulses_cur = 0;
    int ret = 0;
    float speed   = 0.0f;
    float angle   = 0.0f;
    float pid_out = 0.0f;
    u16   pid_set = 0;

    /*获取定时器ID*/
    m_this = pvTimerGetTimerID(xTimer);
    if(NULL == m_this)
    {
        log_error("Timer id get failed.");
        return;
    }

    /*如果是停止模式*/
    if(MOTOR_STOP == m_this->m_mode)
    {
        m_this->m_cur_speed = 0.0f;
    
        ret = my_pwm.ch_set(m_this->m_pwm_timer,m_this->m_pwm_ch,0);
        if(E_OK != ret)
        {
            log_error("Pwm set failed.");
            return;
        }
    }

    /*计算当前速度*/

    /*获取在指定时间间隔内 产生了多少个脉冲*/
    ret = encoder.get_count(m_this->m_encoder_timer,&pulses_cur,true);
    if(E_OK != ret)
    {
        log_error("Encoder count get failed.");
        return;
    }
    num_of_pulses = pulses_cur - 1;


    /*计算速度*/
    speed  = (float)num_of_pulses / ((float)DF_KT / 1000.0f);  /*得到一秒钟多少个脉冲*/
    speed *= 60.0f;                                            /*得到一分钟多少个脉冲*/
    speed /= (float)MOTOR_ENCODER_PULSES_NUM;                  /*得到一分钟电机转了多少圈*/
    speed /= m_this->m_reduction_ratio                      ;  /*得到外转子一分钟转了多少圈*/
    m_this->m_cur_speed = speed; /*保存当前速度*/
    
    /*如果是定距离模式*/
    if(MOTOR_CONST_DIS == m_this->m_mode)
    {
        angle  = (float)num_of_pulses / (float)MOTOR_ENCODER_PULSES_NUM / m_this->m_reduction_ratio;
        angle *= 360.0f;
        angle  = fabs(angle);
        if(angle <= m_this->m_rem_dis)
        {
            m_this->m_rem_dis -= angle;
        }
        else
        {
            m_this->m_rem_dis = 0.0f;
        }
        //log_inform("dis:%.2f.",m_this->m_rem_dis);
        
        /*如果转完了*/
        if(FLOAT_IS_ZERO(m_this->m_rem_dis))
        {
            m_this->m_mode = MOTOR_STOP;
            m_this->m_set_speed = 0.0f;
            m_this->m_rem_dis   = 0.0f;
            
            ret = my_pwm.ch_set(m_this->m_pwm_timer,m_this->m_pwm_ch,0);
            if(E_OK != ret)
            {
                log_error("Pwm set failed.");
                return;
            }
        }
    }

    /*pid 计算*/
    if(!FLOAT_IS_ZERO(m_this->m_set_speed))
    {
        ret = m_this->m_pid.realize(&m_this->m_pid,fabs(m_this->m_set_speed),fabs(m_this->m_cur_speed),&pid_out);
        if(E_OK != ret)
        {
            log_error("Pid realize failed.");
            return;
        }
    }

    /*设置方向*/
    if(m_this->m_set_speed >= 0.0f)
    {
        ret = m_this->m_in1.set(&m_this->m_in1,SWITCH_LOW);
        if(E_OK != ret)
        {
            log_error("Switch set failed.");
            return;
        }
        ret = m_this->m_in2.set(&m_this->m_in2,SWITCH_HIGHT);
        if(E_OK != ret)
        {
            log_error("Switch set failed.");
            return;
        }
    }
    else
    {
        ret = m_this->m_in2.set(&m_this->m_in2,SWITCH_LOW);
        if(E_OK != ret)
        {
            log_error("Switch set failed.");
            return;
        }
        ret = m_this->m_in1.set(&m_this->m_in1,SWITCH_HIGHT);
        if(E_OK != ret)
        {
            log_error("Switch set failed.");
            return;
        }
    }

    /*将pid计算结果设置下去*/
    pid_set = fabs(pid_out);
    if(pid_set >= TIMER_ARR)
    {
        pid_set = TIMER_ARR - 1;
    }
    ret = my_pwm.ch_set(m_this->m_pwm_timer,m_this->m_pwm_ch,pid_set);
    if(E_OK != ret)
    {
        log_error("Pwm set failed.");
        return;
    }

    return;
}

#endif
