

#include "help_conf.h"

#ifdef HELP_PID_ENABLED

#define MODULE_NAME       "my pid"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME


/*定义PID结构体*/
typedef struct __M_MY_PID
{
    float   m_error      ;   /*实际值与目标值的偏差*/
    float   m_last_error ;   /*上一次的偏差值 */
    float   m_kp         ;   /*比例控制系数*/
    float   m_ki         ;   /*积分控制系数*/
    float   m_kd         ;   /*微分控制系数*/
    u32     m_kt         ;   /*时间常数 */
    float   m_i_max      ;   /*积分贡献上线*/
    float   m_i_min      ;   /*积分贡献下线*/
    float   m_intergral  ;   /*积分值*/
}m_my_pid;

static int m_realize(c_my_pid* this,float des_v, float real_v,float* out);


c_my_pid my_pid_create(float kp,float ki,float kd,float i_max,float i_min)
{
    c_my_pid new = {0};
    
    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_my_pid));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_my_pid));

    new.realize = m_realize;
    ((m_my_pid* )new.this)->m_kp    = kp    ;
    ((m_my_pid* )new.this)->m_ki    = ki    ;
    ((m_my_pid* )new.this)->m_kd    = kd    ;
    ((m_my_pid* )new.this)->m_i_max = i_max ;
    ((m_my_pid* )new.this)->m_i_min = i_min ;

    return new;
}

static int m_realize(c_my_pid* this,float des_v, float real_v,float* out)
{
    m_my_pid* m_this = NULL;
    float output = 0;
    float P = 0;
    float I = 0;
    float D = 0;

    /*参数检查*/
    if(NULL == this || NULL == this->this || NULL == out)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    m_this->m_error = des_v - real_v;
    m_this->m_intergral += m_this->m_error;
    
    P  = m_this->m_kp * m_this->m_error;
    I  = m_this->m_ki * m_this->m_intergral;
    D  = m_this->m_kd * -(m_this->m_error - m_this->m_last_error);

    if(!FLOAT_IS_ZERO(m_this->m_i_max) && I > m_this->m_i_max)
    {
        I = m_this->m_i_max;
        m_this->m_intergral = I / m_this->m_ki;
    }
    else if(!FLOAT_IS_ZERO(m_this->m_i_min) && I < m_this->m_i_min)
    {
        I = m_this->m_i_min;
        m_this->m_intergral = I / m_this->m_ki;
    }
    
    output = P + I + D;

    #if SHOW_PID_EFFECT
    log_debug("Expected  value:%.2f,Real-time value:%.2f,P = %.2f,I = %f,D = %.2f,output = %.2f",des_v,real_v,P,I,D,output);
    #endif
    
    m_this->m_last_error = m_this->m_error;
    
    *out = output;
    
    return E_OK;
}

#endif