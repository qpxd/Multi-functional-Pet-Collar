/*****************************************************************************
* Copyright:
* File name: gp2y101.h
* Description: pm2.5传感器函数实现
* Author: 许少
* Version: V1.1
* Date: 2022/01/28
* log:  V1.0  2022/01/28
*       发布：
*       V1.1  2022/04/08
*       修复：m_get中 target 和 adc获取判断叠在一起的问题
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_GP2Y101_ENABLED

#define MODULE_NAME       "gp2y101"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

typedef struct _M_GP2Y
{
    uint32_t      m_old_tick ;  /*上一次采集的时间*/
    bool          m_state    ;  /*传感器状态*/
    float         m_pm2_5_buf;  /*pm2.5缓存*/
    c_switch      m_trig     ;  /*触发开关*/
    u8            m_adc_ch   ;  /*采集adc通道*/
}m_gp2y;

static int m_get(const c_gp2y* this,float* pm2_5);

c_gp2y gp2y_create(u8 adc_ch,GPIO_TypeDef* trig_gpio,uint32_t trig_pin)
{
    c_gp2y new = {0};
    m_gp2y* m_this  = NULL;
    c_switch trig  = {0};
    int ret = 0;

    /*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_gp2y));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_gp2y));
	m_this = new.this;

    /*初始化对应的adc采集通道*/
    ret = my_adc.init(adc_ch);
    if(E_OK != ret)
    {
        log_error("ADC init failed.");
        goto error_handle;
    }

    /*初始化触发开关*/
    trig = switch_create(trig_gpio,trig_pin);
    if(NULL == trig.this)
    {
        log_error("Trig switch creat failed.");
        goto error_handle;
    }
    ret = trig.set(&trig,GPIO_PIN_SET);  /*默认为高电平*/
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }

    /*保存相关变量*/
    m_this->m_adc_ch = adc_ch;
    m_this->m_trig   = trig;
    m_this->m_state  = true;
    new.get          = m_get;

    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}


static int m_get(const c_gp2y* this,float* pm2_5)
{
    m_gp2y* m_this  = NULL;
    float v = 0.0f;
    int ret= 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == pm2_5)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    *pm2_5 = 0.0f;

    /*如果上一次启动 距离现在启动的时间 小于10ms 就把上一次的数据返回出去*/
    if(HAL_GetTick() - m_this->m_old_tick < 10 && 0 != m_this->m_old_tick)
    {
        /*如果传感器状态错误 就报错*/
        if(false == m_this->m_state)
        {
            *pm2_5 = 0.0f;
            log_error("gp2y101 state error.");
            return E_ERROR;
        }

        /*如果上一次采集成功了 就返回缓存*/
        else 
        {
            *pm2_5 = m_this->m_pm2_5_buf;
            return E_OK;
        }
    }
    
    taskENTER_CRITICAL();   //进入临界区

    /*一次新的采集*/
    ret = m_this->m_trig.set(&m_this->m_trig,GPIO_PIN_RESET);
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }
    delay_us(280);
    
    delay_us(19);
    ret = m_this->m_trig.set(&m_this->m_trig,GPIO_PIN_SET);
    if(E_OK != ret)
		{
        log_error("Switch set failed.");
        goto error_handle;
    }
		ret = my_adc.get(m_this->m_adc_ch,&v);
    if(E_OK != ret)
    {
        log_error("adc get failed.");
        goto error_handle;
    }
    
    
    taskEXIT_CRITICAL();            //退出临界区	

    /*换算pm2.5*/
    v *= 2.0f;
    v = (0.17f * v - 0.1f) * 1000.0f;

    if(v < 0)
    {
        v = 0.0f;
    }
    else if(v > 500.0f)
    {
        v = 500.0f;
    }

    *pm2_5 = v;
    return E_OK;

    error_handle:

    ret = m_this->m_trig.set(&m_this->m_trig,GPIO_PIN_SET);  /*默认恢复为高*/
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
    }
    *pm2_5 = 0.0f;
    m_this->m_state = false;  /*状态标记为错误*/
    return E_ERROR;
}

#endif
