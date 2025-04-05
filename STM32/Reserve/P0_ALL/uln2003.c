/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : uln2003.h
* Description: uln2003步进电机驱动模块 函数实现
* Author: xzh
* Version: v1.0
* Date: 2022/02/04
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_ULN2003_ENABLED

#define MODULE_NAME       "uln2003"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define STEP_ANGLE         5.625f  /*步进角*/
#define REDUCTION_RATIO   64.000f  /*减速比*/

#define DIR_POSITIVE       0       /*正向*/
#define DIR_REVERSE        1       /*反向*/

typedef struct __M_ULN2003
{
    c_switch      m_a          ;  /*a相*/
    c_switch      m_b          ;  /*b相*/
    c_switch      m_c          ;  /*c相*/
    c_switch      m_d          ;  /*d相*/
    TimerHandle_t m_timer      ;  /*软件定时器*/
    u32           m_pul        ;  /*剩余脉冲*/
    u32           m_speed      ;  /*速度*/
    u32           m_type       ;  /*励磁类型*/ 
    u32           m_dir        ;  /*方向*/
    u32           m_beat       ;  /*当前节拍*/
    u32           m_beat_count ;  /*节拍数*/
    const u8*     m_beat_list  ;  /*节拍列表*/  
    bool          m_speed_mode ;  /*速度模式使能*/    
}m_uln2003;

static const u8 g_phase_1_cw   [4] = {0x08, 0x04, 0x02, 0x01};  /*顺时针：D-C-B-A*/
static const u8 g_phase_1_ccw  [4] = {0x01, 0x02, 0x04, 0x08};  /*逆时针：A-B-C-D*/
                     
static const u8 g_phase_2_cw   [4] = {0x0c, 0x06, 0x03, 0x09};  /*顺时针：DC-CB-BA-AD*/
static const u8 g_phase_2_ccw  [4] = {0x0c, 0x06, 0x03, 0x09};  /*逆时针：AB-BC-CD-DA*/
                      
static const u8 g_phase_1_2_cw [8] = {0x08, 0x0c, 0x04, 0x06, 0x02, 0x03, 0x01, 0x09};  /*顺时针：D-DC-C-CB-B-BA-A-AD*/
static const u8 g_phase_1_2_ccw[8] = {0x01, 0x03, 0x02, 0x06, 0x04, 0x0c, 0x08, 0x89};  /*逆时针：A-AB-B-BC-C-CD-D-DA*/


static int   m_set           (const c_uln2003* this,u8 type                 );
static int   m_rotate        (const c_uln2003* this,float angle,float speed );
static int   m_speed         (const c_uln2003* this,float speed             );
static int   m_stop          (const c_uln2003* this                         );
static int   m_state         (const c_uln2003* this,u8* state               );
static int   m_match_table   (const c_uln2003* this                         );
static void  timer_call_back (TimerHandle_t    xTimer                       );


c_uln2003 uln2003_creat(u8 type,GPIO_TypeDef* a_gpio,uint32_t a_pin,GPIO_TypeDef* b_gpio,uint32_t b_pin,
                           GPIO_TypeDef* c_gpio,uint32_t c_pin,GPIO_TypeDef* d_gpio,uint32_t d_pin)
{
    int ret = 0;
    c_uln2003  new = {0};
    m_uln2003* m_this = NULL;  

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_uln2003));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_uln2003));
    m_this = new.this;

    /*创建定时器*/
    m_this->m_timer = xTimerCreate("switch timer",      /*定时器名称*/
                         1             ,      /*默认1ms*/
                         pdTRUE        ,      /*循环定时器 */
                         new.this      ,      /*把私有成员传到定时器回调里头*/
                         timer_call_back);
    if(NULL == m_this->m_timer)
    {
        log_error("Timer creat failed.");
        goto error_handle;
    }
    
    /*创建四个开关*/
    m_this->m_a = switch_create(a_gpio,a_pin);
    if(NULL == m_this->m_a.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }
    m_this->m_b = switch_create(b_gpio,b_pin);
    if(NULL == m_this->m_b.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }
    m_this->m_c = switch_create(c_gpio,c_pin);
    if(NULL == m_this->m_c.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }
    m_this->m_d = switch_create(d_gpio,d_pin);
    if(NULL == m_this->m_d.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }

    /*保存相关变量*/
    new.set    = m_set;
    new.rotate = m_rotate;
    new.speed  = m_speed;
    new.stop   = m_stop;
    new.state  = m_state;

    /*设置励磁类型*/
    ret = m_set(&new,type);
    if(E_OK != ret)
    {
        log_error("Set failed.");
        goto error_handle;
    }

    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}


static int m_set(const c_uln2003* this,u8 type)
{
    m_uln2003* m_this = NULL;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(TYPE_1_PHASE != type && TYPE_2_PHASE != type && TYPE_1_2_PHASE != type)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*状态检查*/

    /*要求电机在停止的状态下进行更改*/
    if(0 != m_this->m_pul || true == m_this->m_speed_mode)
    {
        log_error("Need stop motor first.");
        return E_ERROR;
    }

    /*更改励磁*/
    m_this->m_type = type;
    if(TYPE_1_2_PHASE == m_this->m_type)
    {
        m_this->m_beat_count = 8;
    }
    else 
    {
        m_this->m_beat_count = 4;
    }
    
    return E_OK;
}

static int m_rotate(const c_uln2003* this,float angle,float speed)
{
    int ret = 0;
    m_uln2003* m_this = NULL;
    BaseType_t ret_os = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(angle > ANGLE_MAX)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    if(((speed >= 0) && (speed < SPEED_MIN || speed > SPEED_MAX)) || ((speed < 0) && (-speed < SPEED_MIN || -speed > SPEED_MAX)))
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*先停止电机*/
    ret = m_stop(this);
    if(E_OK != ret)
    {
        log_error("Motor stop failed.");
        return E_ERROR;
    }

    /*换算参数*/
    
    /*电机方向*/
    if(speed < 0)
    {
        m_this->m_dir = DIR_REVERSE;  /*反向*/
        speed *= -1.0f;
    }
    else
    {
        m_this->m_dir = DIR_POSITIVE; /*正向*/
    }

    /*脉冲*/
    m_this->m_pul = angle * REDUCTION_RATIO / STEP_ANGLE * m_this->m_beat_count / 8.0f;

    /*速度*/
    m_this->m_speed = 1000.0f / (speed * 6.0f * REDUCTION_RATIO / STEP_ANGLE * m_this->m_beat_count / 8.0f) + 0.5f;   

    /*根据励磁类型和方向 选择对应的励磁列表*/
    ret = m_match_table(this);
    if(E_OK != ret)
    {
        log_error("Match table failed.");
        return E_ERROR;
    }
    
    /*设置定时器*/
    ret_os = xTimerChangePeriod(m_this->m_timer,m_this->m_speed,100);
    if(pdPASS != ret_os)
    {
        log_error("Timer set failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_speed(const c_uln2003* this,float speed)
{
    int ret = 0;
    m_uln2003* m_this = NULL;
    BaseType_t ret_os = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(((speed >= 0) && (speed < SPEED_MIN || speed > SPEED_MAX)) || ((speed < 0) && (-speed < SPEED_MIN || -speed > SPEED_MAX)))
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*先停止电机*/
    ret = m_stop(this);
    if(E_OK != ret)
    {
        log_error("Motor stop failed.");
        return E_ERROR;
    }

    /*换算参数*/
    
    /*电机方向*/
    if(speed < 0)
    {
        m_this->m_dir = DIR_REVERSE;  /*反向*/
        speed *= -1.0f;
    }
    else
    {
        m_this->m_dir = DIR_POSITIVE; /*正向*/
    }

    /*速度写此 内存会被篡改 原因未知*/
    m_this->m_speed = 1000.0f / (speed * 6.0f * REDUCTION_RATIO / STEP_ANGLE * m_this->m_beat_count / 8.0f) + 0.5f;
    
    /*模式*/
    m_this->m_speed_mode = true;

    /*根据励磁类型和方向 选择对应的励磁列表*/
    ret = m_match_table(this);
    if(E_OK != ret)
    {
        log_error("Match table failed.");
        return E_ERROR;
    }
    
    /*设置定时器*/
    ret_os = xTimerChangePeriod(m_this->m_timer,m_this->m_speed,100);
    if(pdPASS != ret_os)
    {
        log_error("Timer set failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_stop(const c_uln2003* this)
{
    int ret = 0;
    m_uln2003* m_this = NULL;
    BaseType_t ret_os = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*把定时器停掉*/
    ret_os = xTimerStop(m_this->m_timer,0);
    if(pdPASS != ret_os)
    {
        log_error("Timer stop failed.");
        return E_ERROR;
    }

    /*关闭所有的开关*/
    ret = m_this->m_a.set(&m_this->m_a,GPIO_PIN_RESET);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        return E_ERROR;
    }
    ret = m_this->m_b.set(&m_this->m_b,GPIO_PIN_RESET);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        return E_ERROR;
    }
    ret = m_this->m_c.set(&m_this->m_c,GPIO_PIN_RESET);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        return E_ERROR;
    }
    ret = m_this->m_d.set(&m_this->m_d,GPIO_PIN_RESET);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        return E_ERROR;
    }

    /*清除参数*/
    m_this->m_pul = 0;
    m_this->m_speed_mode = false;
    m_this->m_speed = 0;

    return E_OK;
}

static int m_state(const c_uln2003* this,u8* state)
{
    m_uln2003* m_this = NULL;
    
    /*参数检测*/
    if(NULL == this || NULL == this->this|| NULL == state)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    if(0 != m_this->m_pul || true == m_this->m_speed_mode)
    {
        *state = ULN2003_BUSY;
    }
    else
    {
        *state = ULN2003_IDLE;
    }
    
    return E_OK;
}

static void  timer_call_back( TimerHandle_t xTimer )
{
    m_uln2003*  m_this = NULL;
    BaseType_t ret_os = 0;
    int ret = 0;
    u8 data = 0;

    /*获取定时器ID*/
    m_this = pvTimerGetTimerID(xTimer);
    if(NULL == m_this)
    {
        log_error("Timer id get failed.");
        goto error_handle;
    }

    /*检查是否转完了*/
    if(0 != m_this->m_pul && false == m_this->m_speed_mode)
    {
        --m_this->m_pul;
    }
    else if(0 == m_this->m_pul && false == m_this->m_speed_mode)
    {
        /*把定时器停掉*/
        ret_os = xTimerStop(m_this->m_timer,0);
        if(pdPASS != ret_os)
        {
            log_error("Timer stop failed.");
            goto error_handle;
        }

        /*关闭所有的开关*/
        ret = m_this->m_a.set(&m_this->m_a,GPIO_PIN_RESET);
        if(E_OK != ret)
        {
            log_error("Switch set failed.");
            goto error_handle;
        }
        ret = m_this->m_b.set(&m_this->m_b,GPIO_PIN_RESET);
        if(E_OK != ret)
        {
            log_error("Switch set failed.");
            goto error_handle;
        }
        ret = m_this->m_c.set(&m_this->m_c,GPIO_PIN_RESET);
        if(E_OK != ret)
        {
            log_error("Switch set failed.");
            goto error_handle;
        }
        ret = m_this->m_d.set(&m_this->m_d,GPIO_PIN_RESET);
        if(E_OK != ret)
        {
            log_error("Switch set failed.");
            goto error_handle;
        }
    }
    
    /*根据节拍 设置相应的电平组合*/
    if(m_this->m_beat_count <= m_this->m_beat)
    {
        m_this->m_beat = 0;
    }
    data = m_this->m_beat_list[m_this->m_beat];

    /*设置电平*/
    ret = m_this->m_a.set(&m_this->m_a,data & 0x01);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }
    ret = m_this->m_b.set(&m_this->m_b,(data >> 1) & 0x01);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }
    ret = m_this->m_c.set(&m_this->m_c,(data >> 2) & 0x01);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }
    ret = m_this->m_d.set(&m_this->m_d,(data >> 3) & 0x01);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }
    
    ++m_this->m_beat;

    return;

    error_handle:

    m_this->m_pul = 0;
    m_this->m_speed_mode = false;
    m_this->m_speed = 0;
    m_this->m_a.set(&m_this->m_a,GPIO_PIN_RESET);
    m_this->m_b.set(&m_this->m_b,GPIO_PIN_RESET);
    m_this->m_c.set(&m_this->m_c,GPIO_PIN_RESET);
    m_this->m_d.set(&m_this->m_d,GPIO_PIN_RESET);
    xTimerStop(m_this->m_timer,0);
    return;
}

static int m_match_table(const c_uln2003* this)
{
    m_uln2003* m_this = NULL;
    
    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*根据励磁类型和方向 选择对应的励磁列表*/
    if(TYPE_1_PHASE == m_this->m_type && DIR_POSITIVE == m_this->m_dir)
    {
        m_this->m_beat_list = g_phase_1_cw;
    }
    else if(TYPE_1_PHASE == m_this->m_type && DIR_REVERSE == m_this->m_dir)
    {
        m_this->m_beat_list = g_phase_1_ccw;
    }
    else if(TYPE_2_PHASE == m_this->m_type && DIR_POSITIVE == m_this->m_dir)
    {
        m_this->m_beat_list = g_phase_2_cw;
    }
    else if(TYPE_2_PHASE == m_this->m_type && DIR_REVERSE == m_this->m_dir)
    {
        m_this->m_beat_list = g_phase_2_ccw;
    }
    else if(TYPE_1_2_PHASE == m_this->m_type && DIR_POSITIVE == m_this->m_dir)
    {
        m_this->m_beat_list = g_phase_1_2_cw;
    }

    else if(TYPE_1_2_PHASE == m_this->m_type && DIR_REVERSE == m_this->m_dir)
    {
        m_this->m_beat_list = g_phase_1_2_ccw;
    }
    else
    {
        log_error("Error phase set.");
        return E_PARAM;
    }

    return E_OK;
}

#endif
