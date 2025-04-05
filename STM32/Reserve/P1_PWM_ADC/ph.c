/*****************************************************************************
* Copyright:
* File name: ph.h
* Description: ph传感器公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/02/28
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_PH_ENABLED

#define MODULE_NAME       "ph"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

static int m_get(const c_ph* this,float* ph);

typedef struct _M_PH
{
    u8            m_adc_ch   ;  /*采集adc通道*/
}m_ph;

c_ph ph_create(u8 adc_ch)
{
    c_ph new = {0};
    m_ph* m_this  = NULL;
    int ret = 0;

    /*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_ph));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_ph));
	m_this = new.this;

    /*初始化对应的adc采集通道*/
    ret = my_adc.init(adc_ch);
    if(E_OK != ret)
    {
        log_error("ADC init failed.");
        goto error_handle;
    }

    /*保存相关变量*/
    m_this->m_adc_ch = adc_ch;
    new.get          = m_get;

    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_get(const c_ph* this,float* ph)
{
    m_ph* m_this  = NULL;
    float v = 0.0f;
    int ret= 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == ph)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    *ph = 0.0f;

    /*获取电压*/
    ret = my_adc.get(m_this->m_adc_ch,&v);
    if(E_OK != ret)
    {
        log_error("adc get failed.");
        return E_OK;
    }

    /*转换为ph*/
    *ph = (4.2425 - v * 2.0f) / 0.1775f;

    return E_OK;
}
#endif
