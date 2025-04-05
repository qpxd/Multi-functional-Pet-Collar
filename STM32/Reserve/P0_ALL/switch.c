/*****************************************************************************
* Copyright:
* File name: switch.c
* Description: 开关模块函数实现
* Author: 许少
* Version: V1.2
* Date: 2022/04/08
* log:  V1.0  2022/04/08
*       发布：
*
*       V1.1  2022/04/02
*       新增：power接口中 相同功率退出 没有先给this指针赋值再判断。
*
*       V1.2  2022/04/08
*       新增：SWITHC_POWER_INTERVAL 开关频率控制宏
*****************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_SWITCH_ENABLED

#define MODULE_NAME       "switch"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define MODE_SWITCH  0
#define MODE_TIMER   1
#define MODE_POWER   2

typedef struct __M_SWITCH
{
	GPIO_TypeDef* m_gpio     ;  /*gpio*/
	uint32_t      m_pin      ;  /*pin*/
	TimerHandle_t m_timer    ;  /*软件定时器*/
    uint16_t      m_intervals;  /*间隔时间*/
    u8            m_mode     ;  /*模式*/
    u8            m_power    ;  /*功率*/
    u8            m_count    ;
}m_switch;

static int   set     (const c_switch* this,GPIO_PinState  state);  
static int   get     (const c_switch* this,GPIO_PinState* state);
static int   flicker (const c_switch* this,u16 time);
static int   m_power (const c_switch* this,u8 power);
static void  timer_call_back( TimerHandle_t xTimer );

c_switch switch_create(GPIO_TypeDef* gpio,uint32_t pin)
{
	c_switch new = {0};
	GPIO_InitTypeDef GPIO_Initure;
    TimerHandle_t  timer = NULL;
    
	/*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_switch));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_switch));

    /*创建软件定时器*/
    timer = xTimerCreate("switch timer",      /*定时器名称*/
                         1000          ,      /*默认一秒*/
                         pdTRUE        ,      /*循环定时器 */
                         new.this      ,      /*把私有成员传到定时器回调里头*/
                         timer_call_back);
    /*创建情况*/
    if(NULL == timer)
    {
        vPortFree(new.this);
        new.this = NULL;
        log_error("Timer creat failed.");
        return new;
    }
	
    //初始化对应的GPIO
    GPIO_Initure.Pin   = pin                 ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP ;  //输出
    GPIO_Initure.Pull  = GPIO_PULLUP         ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;  //高速
    HAL_GPIO_Init(gpio,&GPIO_Initure);  

	/*保存相关变量*/
	((m_switch* )new.this)->m_gpio     = gpio    ;
	((m_switch* )new.this)->m_pin      = pin     ;
	((m_switch* )new.this)->m_timer    = timer   ;
	new.set = set;
	new.get = get;
	new.flicker = flicker;
	new.power = m_power;
    
	return new;
}

static int   set     (const c_switch* this,GPIO_PinState  state)
{
    m_switch* m_this = NULL;
    BaseType_t      ret_os = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(GPIO_PIN_SET != state && GPIO_PIN_RESET != state)
    {
        log_error("Param error");
        return E_PARAM;
    }
    m_this = this->this;

    /*把定时器停掉*/
    if( xTimerIsTimerActive(m_this->m_timer) != pdFALSE )
    {
        ret_os = xTimerStop(m_this->m_timer,100);
        if(pdPASS != ret_os)
        {
            log_error("Timer stop failed.");
            return E_ERROR;
        }
    }
    
    m_this->m_mode = MODE_SWITCH;
    HAL_GPIO_WritePin(m_this->m_gpio,m_this->m_pin,state);

    return  E_OK;
}

static int   get     (const c_switch* this,GPIO_PinState* state)
{
    const m_switch* m_this = NULL;
    GPIO_PinState   io_state = GPIO_PIN_RESET;

    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == state)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    io_state = HAL_GPIO_ReadPin(m_this->m_gpio,m_this->m_pin);

    *state = io_state;

    return  E_OK;
}

static int   flicker (const c_switch* this,u16 time)
{
    m_switch* m_this = NULL;
    BaseType_t      ret_os = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    m_this->m_mode = MODE_TIMER;  /*定时模式*/
    
    /*设置新的定时器周期*/
    ret_os = xTimerChangePeriod(m_this->m_timer,time,100);
    if(pdPASS != ret_os)
    {
        log_error("Timer set failed.");
        return E_ERROR;
    }

    /*复位定时器*/
    ret_os = xTimerReset(m_this->m_timer,100);
    if(pdPASS != ret_os)
    {
        log_error("Timer reset failed.");
        return E_ERROR;
    }

    /*启动定时器*/
//    ret_os = xTimerStart(m_this->m_timer,100);
//    if(pdPASS != ret_os)
//    {
//        log_error("Timer start failed.");
//        return E_ERROR;
//    }

    return E_OK;
}

int  m_power(const c_switch* this,u8 power)
{
    m_switch* m_this = NULL;
    BaseType_t      ret_os = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
		m_this = this->this;
    if(power >= 10)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    if(MODE_POWER == m_this->m_mode && m_this->m_power == 10 - power)
    {
        return E_OK;
    }
    
    m_this->m_mode = MODE_POWER;  /*功率模式*/ 
    m_this->m_power = 10 - power;
    m_this->m_count = 0;
    
    /*设置新的定时器周期*/
    ret_os = xTimerChangePeriod(m_this->m_timer,SWITHC_POWER_INTERVAL,100);
    if(pdPASS != ret_os)
    {
        log_error("Timer set failed.");
        return E_ERROR;
    }

    /*复位定时器*/
    ret_os = xTimerReset(m_this->m_timer,100);
    if(pdPASS != ret_os)
    {
        log_error("Timer reset failed.");
        return E_ERROR;
    }    
    
    return E_OK;
}

static void  timer_call_back( TimerHandle_t xTimer )
{
    m_switch*  m_this = NULL;

    /*获取定时器ID*/
    m_this = pvTimerGetTimerID(xTimer);
    if(NULL == m_this)
    {
        log_error("Timer id get failed.");
        return;
    }

    /*如果是定时模式*/
    if(MODE_TIMER == m_this->m_mode)
    {
        
        /*开关电平反转*/
        HAL_GPIO_TogglePin(m_this->m_gpio,m_this->m_pin);
    }
    
    /*功率模式*/
    if(MODE_POWER == m_this->m_mode)
    {
        if(0 == m_this->m_count % m_this->m_power)
        {
            HAL_GPIO_WritePin(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);
        }
        else
        {
            HAL_GPIO_WritePin(m_this->m_gpio,m_this->m_pin,GPIO_PIN_RESET);
        }
    }
    
    ++m_this->m_count;
    
    if(m_this->m_count > 100)
    {
        m_this->m_count = 0;
    }

    return;
}
#endif 

