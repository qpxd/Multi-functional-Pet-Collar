/*****************************************************************************
* Copyright:
* File name: key_board.h
* Description: 键盘模块 公开头文件
* Author: 许少
* Version: V1.3
* Date: 2022/04/08
* log:  V1.0  2021/12/29
*       发布：
*
*       V1.1  2022/01/30
*       修复：377行KEY_PRESS_IS_ONE 写成KEY_PRESS_IS_ZERO 导致无法进入按下为1的处理。
*
*       V1.2  2022/03/08
*       新增：get_hw 通过硬件状态 获取按键状态
*
*       V1.3  2022/04/06
*       修复：get_hw 函数指针未赋值
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_KEY_BOARD_ENABLED

#define MODULE_NAME       "key board"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define KEY_BOARD_MESG_MAX_LEN   20

static int m_int_handle(uint32_t pin,void* param); 
static void  m_timer_call_back( TimerHandle_t xTimer );
static void m_key_board_task(void* pdata);
static int m_get(const c_key_board* this,const char* key_name,u8* state);
static int m_get_hw(const c_key_board* this,const char* key_name,u8* state);
static int m_set_callback(c_key_board* this,int (*callback)(const char*,u8));


typedef struct _M_KEY_BOARD m_key_board;


/*按键消息*/
typedef struct _KEY_MESG
{
    const char* m_name ;
    u8          m_state;
}key_mesg;

/*按键对象*/
typedef struct _KEY
{
    const char*   m_name    ;    /*按键名称*/
    GPIO_TypeDef* m_gpio    ;    /*按键所在GPIO分组*/
    uint32_t      m_pin     ;    /*按键所在PIN脚*/
    u8            m_polarity;    /*按键极性*/
    u8            m_state   ;    /*按键状态*/
    TimerHandle_t m_timer   ;    /*软件定时器*/
    m_key_board*  m_father  ;    /*留下个父指针 让按键能够找到所属键盘*/
}c_key;

/*键盘 私有成员*/
typedef struct _M_KEY_BOARD
{
    c_key*         m_key_list      ;  /*按键列表*/
    u8             m_key_amount    ;  /*按键列表长度*/
    TaskHandle_t   m_task_handle   ;  /*键盘任务句柄*/
    QueueHandle_t  m_queue_handle  ;  /*键盘消息队列*/
    int (*m_callback)(const char*,u8);
}m_key_board;

c_key_board key_board_create(key_list* list,u8 list_len)
{
    c_key_board new = {0};
    m_key_board* m_this = NULL;
    GPIO_InitTypeDef gpio_init = {0};
    u8 i = 0;
    int ret = 0;
    BaseType_t        os_ret   = 0 ;

    /*参数检查*/
    if(NULL == list)
    {
        log_error("Null pointer.");
        return new;
    }
    if(list_len > KEY_BOARD_MAX_KEY_AMOUNT)
    {
        log_error("Param error.");
        return new;
    }
    for(i = 0;i < list_len;++i)
    {
        if(KEY_PRESS_IS_ZERO != list[i].polarity && KEY_PRESS_IS_ONE != list[i].polarity)
        {
            log_error("Param error.");
            return new;
        }
    }

    /*为新对象 申请内存*/
    new.this = pvPortMalloc(sizeof(m_key_board));
    if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_key_board));
	m_this = new.this;

    /*为按键列表开辟内存*/
    m_this->m_key_list = pvPortMalloc(sizeof(c_key) * list_len);  
    if(NULL == m_this->m_key_list)
    {
        vPortFree(new.this);
        new.this = NULL;
        log_error("Out of memory");
		return new;
    }
    memset(m_this->m_key_list,0,sizeof(c_key) * list_len);
    
    m_this->m_key_amount = list_len;  /*保存按键长度*/

    /*初始化相应的GPIO 和按键定时器*/
    gpio_init.Mode  = GPIO_MODE_INPUT   ;  //输入模式
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH  ;  //高速
    for(i = 0;i < list_len;++i)
    {
        /*先创建定时器，因为 定时器创建 可能失败*/
        m_this->m_key_list[i].m_timer = xTimerCreate("key timer"             ,      /*定时器名称*/
                                                      10                     ,      /*默认10ms 既为按键消抖时间*/
                                                      pdFALSE                ,      /*单次定时器 */
                                                      &m_this->m_key_list[i] ,      /*把按键对象传到定时器回调里头*/
                                                      m_timer_call_back);
        /*创建情况*/
        if(NULL == m_this->m_key_list[i].m_timer)
        {
            log_error("Timer creat failed.");
            goto error_handle;
        }
    
        /*如果按下为零，就配置为上拉*/
        if(KEY_PRESS_IS_ZERO == list[i].polarity)
        {
            gpio_init.Pull  = GPIO_PULLUP           ;  //上拉
            exti_cfg(list[i].gpio,list[i].pin,GPIO_MODE_IT_FALLING);  /*默认配置为下降沿*/
        }

        /*如果按下为一，就配置为下拉*/
        else 
        {
            gpio_init.Pull  = GPIO_PULLDOWN           ;  //下拉
            exti_cfg(list[i].gpio,list[i].pin,GPIO_MODE_IT_RISING);  /*默认配置为上升沿*/
        }
        gpio_init.Pin = list[i].pin;
        HAL_GPIO_Init(list[i].gpio,&gpio_init);  /*io 初始化*/
        EXTI_IT_ENABLE(list[i].pin);             /*使能中断线*/ 

        /*保存按键的相关参数*/
        m_this->m_key_list[i].m_name     = list[i].name     ;
        m_this->m_key_list[i].m_gpio     = list[i].gpio     ;
        m_this->m_key_list[i].m_pin      = list[i].pin      ;
        m_this->m_key_list[i].m_polarity = list[i].polarity ;
        m_this->m_key_list[i].m_father   = m_this           ;

        /*注册中断回调 中断触发时 会把按键对象传进去*/
        ret = int_manage.login_exti(list[i].pin,m_int_handle,&m_this->m_key_list[i]);
        if(E_OK != ret)
        {
            log_error("Int func login failed.");
            goto error_handle;
        }
    }

    /*初始化消息队列*/
    m_this->m_queue_handle = xQueueCreate(KEY_BOARD_MESG_MAX_LEN,sizeof(key_mesg));
    if(NULL == m_this->m_queue_handle)
    {
        log_error("Queue creat filed.");
        goto error_handle;
    }

    /*初始化键盘任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_key_board_task    ,
                         (const char*    )"key board task"    ,
                         (uint16_t       )KEY_BOARD_TASK_STK  ,
                         (void*          )m_this              ,
                         (UBaseType_t    )KEY_BOARD_TASK_PRO  ,
                         (TaskHandle_t*  )&m_this->m_task_handle);
                                 
    if(pdPASS != os_ret)
    {
        log_error("Key board task creat filed,ret=%d",(int)os_ret);
        goto error_handle;
    }

    /*保存函数指针*/
    new.get = m_get;
		new.get_hw = m_get_hw;
    new.set_callback = m_set_callback;

    return new;

    error_handle:

    /*如果创建了定时器 就把已经创建的定时器删除*/
    for(;i != 0;--i)
    {
        os_ret = xTimerDelete(m_this->m_key_list[i].m_timer,100);
        if(pdPASS != os_ret)
        {
            log_warning("Timer delete failed.");
        }
    }

    /*释放内存*/
    vPortFree(m_this->m_key_list);
    m_this->m_key_list = NULL;
    vPortFree(m_this);
    m_this = NULL;
    new.this = NULL;
    
    return new;
}

static int m_int_handle(uint32_t pin,void* param)
{
    c_key*         this = param;
    BaseType_t  os_ret  = 0 ;
    
    if(NULL == param)
    {
        log_error("Null pointer");
        return E_NULL;
    }

    /*不管是什么状态 只要进了中断处理 一定是按键状态发生变化*/
    /*进来第一步 关中断 停定时器 并复位 */

    /*关中断*/
    EXTI_IT_DISABLE(this->m_pin);

    /*设置定时器 消抖*/
    xTimerChangePeriodFromISR(this->m_timer,KEY_ELIMINATE_JITTER_MS,&os_ret);

    /*手动进行一次任务切换*/
    if(pdTRUE == os_ret)
    {
        taskYIELD();
    }

    return E_OK;
}



static void  m_timer_call_back( TimerHandle_t xTimer )
{
    c_key*  this = NULL;
    GPIO_PinState   io_state = GPIO_PIN_RESET;
    u8         new_state = 0;
    TickType_t time_period = 0;
    key_mesg   new_mesg  = {0};
    uint32_t   follow_state = 0;
    BaseType_t     os_ret  = 0;
    int ret= 0;

    /*进入定时器回调 有两种情况 一种是按下 一种是长按*/
    /*这两种情况 我们需要根据按键当前状态来分辨*/

    /*先拿到按键对象*/
    this = pvTimerGetTimerID(xTimer);
    if(NULL == this)
    {
        log_error("Key item lost.");
        return;
    }
    if(NULL == this->m_father)
    {
        log_error("Key board item lost.");
        return;
    }
    new_state = this->m_state;
    time_period = xTimerGetPeriod(this->m_timer);  /*获取当前定时器进来是延时了多久时间*/
    follow_state = exti_get_follow(this->m_pin);

    /*获取IO口状态*/
    io_state = HAL_GPIO_ReadPin(this->m_gpio,this->m_pin);

    /*如果按键极性为 按下为零*/
    if(KEY_PRESS_IS_ZERO == this->m_polarity)
    {
        /*如果按键状态为 弹起*/
        if(KEY_UP == this->m_state)
        {
            /*下降沿 IO为0 且定时器是处于消抖状态*/
            if(GPIO_MODE_IT_FALLING == follow_state && GPIO_PIN_RESET == io_state && KEY_ELIMINATE_JITTER_MS == time_period)
            {
                new_state = KEY_PRESS;  /*确认按键按下*/

                /*中断修改为上升沿 监测按键弹起*/
                exti_set_follow(this->m_pin,GPIO_MODE_IT_RISING);
                EXTI_IT_ENABLE(this->m_pin);
                
                /*修改定时器 进行长按检测*/
                os_ret = xTimerChangePeriod(xTimer,KEY_LONG_PRESS_TIME_MS,100);
                if(pdPASS != os_ret)
                {
                    log_error("Timer set failed.");
                    return;
                }
            }

            /*否者就是个抖动 重新检测新的按键按下动作*/
            else
            {
                exti_set_follow(this->m_pin,GPIO_MODE_IT_FALLING);
                EXTI_IT_ENABLE(this->m_pin);
            }
        }

        /*如果按键状态为 按下*/
        else if(KEY_PRESS == this->m_state)
        {
            /*IO为0 且定时器处于长按检查状态*/
            if(GPIO_PIN_RESET == io_state && KEY_LONG_PRESS_TIME_MS == time_period)
            {
                new_state = KEY_LONG_PRESS;  /*确认按键长按*/
            }

            /*上升沿 IO为1 且按键处于消抖检查状态*/
            else if(GPIO_MODE_IT_RISING == follow_state && GPIO_PIN_SET == io_state && KEY_ELIMINATE_JITTER_MS == time_period)
            {
                new_state = KEY_UP;  /*确认按键弹起*/

                /*修改为下降沿检测 等待下一次按键按下*/
                exti_set_follow(this->m_pin,GPIO_MODE_IT_FALLING);
                EXTI_IT_ENABLE(this->m_pin);
            }

            /*这里的情况剩余情况 只可能是手不稳抖了下*/
            else
            {
                /*还是检查上升沿 防止长按一般弹起*/
                exti_set_follow(this->m_pin,GPIO_MODE_IT_RISING);
                EXTI_IT_ENABLE(this->m_pin);
                
                /*修改定时器 重新进行长按检测*/
                os_ret = xTimerChangePeriod(xTimer,KEY_LONG_PRESS_TIME_MS,100);
                if(pdPASS != os_ret)
                {
                    log_error("Timer set failed.");
                    return;
                }
            }
        }

        /*如果按键状态为 长按*/
        else if(KEY_LONG_PRESS == this->m_state)
        {
            /*长按触发定时器 只有可能是弹起了*/

            /*上升沿 IO为1 且按键处于消抖检查状态*/
            if(GPIO_MODE_IT_RISING == follow_state && GPIO_PIN_SET == io_state && KEY_ELIMINATE_JITTER_MS == time_period)
            {
                new_state = KEY_UP;  /*确认按键弹起*/

                /*修改为下降沿检测 等待下一次按键按下*/
                exti_set_follow(this->m_pin,GPIO_MODE_IT_FALLING);
                EXTI_IT_ENABLE(this->m_pin);
            }

            /*否者为抖动 从新等待上升沿触发*/
            else
            {
                exti_set_follow(this->m_pin,GPIO_MODE_IT_RISING);
                EXTI_IT_ENABLE(this->m_pin);
            }
        }
    }

    /*如果按键极性为 按下为一*/
    else if(KEY_PRESS_IS_ONE == this->m_polarity)
    {
        /*如果按键状态为 弹起*/
        if(KEY_UP == this->m_state)
        {
            /*上升沿 IO为1 且定时器是处于消抖状态*/
            if(GPIO_MODE_IT_RISING == follow_state && GPIO_PIN_SET == io_state && KEY_ELIMINATE_JITTER_MS == time_period)
            {
                new_state = KEY_PRESS;  /*确认按键按下*/

                /*中断修改为下降沿 监测按键弹起*/
                exti_set_follow(this->m_pin,GPIO_MODE_IT_FALLING);
                EXTI_IT_ENABLE(this->m_pin);
                
                /*修改定时器 进行长按检测*/
                os_ret = xTimerChangePeriod(xTimer,KEY_LONG_PRESS_TIME_MS,100);
                if(pdPASS != os_ret)
                {
                    log_error("Timer set failed.");
                    return;
                }
            }

            /*否者就是个抖动 重新检测新的按键按下动作*/
            else
            {
                exti_set_follow(this->m_pin,GPIO_MODE_IT_RISING);
                EXTI_IT_ENABLE(this->m_pin);
            }
        }

        /*如果按键状态为 按下*/
        else if(KEY_PRESS == this->m_state)
        {
            /*IO为1 且定时器处于长按检查状态*/
            if(GPIO_PIN_SET == io_state && KEY_LONG_PRESS_TIME_MS == time_period)
            {
                new_state = KEY_LONG_PRESS;  /*确认按键长按*/
            }

            /*下降沿 IO为0 且按键处于消抖检查状态*/
            else if(GPIO_MODE_IT_FALLING == follow_state && GPIO_PIN_RESET == io_state && KEY_ELIMINATE_JITTER_MS == time_period)
            {
                new_state = KEY_UP;  /*确认按键弹起*/

                /*修改为上升沿检测 等待下一次按键按下*/
                exti_set_follow(this->m_pin,GPIO_MODE_IT_RISING);
                EXTI_IT_ENABLE(this->m_pin);
            }

            /*这里的情况剩余情况 只可能是手不稳抖了下*/
            else
            {
                /*还是检查下降沿 防止长按一般弹起*/
                exti_set_follow(this->m_pin,GPIO_MODE_IT_FALLING);
                EXTI_IT_ENABLE(this->m_pin);
                
                /*修改定时器 重新进行长按检测*/
                os_ret = xTimerChangePeriod(xTimer,KEY_LONG_PRESS_TIME_MS,100);
                if(pdPASS != os_ret)
                {
                    log_error("Timer set failed.");
                    return;
                }
            }
        }

        /*如果按键状态为 长按*/
        else if(KEY_LONG_PRESS == this->m_state)
        {
            /*长按触发定时器 只有可能是弹起了*/

            /*下降沿 IO为0 且按键处于消抖检查状态*/
            if(GPIO_MODE_IT_FALLING == follow_state && GPIO_PIN_RESET == io_state && KEY_ELIMINATE_JITTER_MS == time_period)
            {
                new_state = KEY_UP;  /*确认按键弹起*/

                /*修改为上升沿检测 等待下一次按键按下*/
                exti_set_follow(this->m_pin,GPIO_MODE_IT_RISING);
                EXTI_IT_ENABLE(this->m_pin);
            }

            /*否者为抖动 从新等待下降沿触发*/
            else
            {
                exti_set_follow(this->m_pin,GPIO_MODE_IT_FALLING);
                EXTI_IT_ENABLE(this->m_pin);
            }
        }
    }

    /*否则就是错误极性*/
    else
    {
        log_error("Error polarity.");
        return;
    }


    /*如果按键状态被更新 发送消息到键盘*/
    if(new_state != this->m_state)
    {
        this->m_state = new_state;
        new_mesg.m_state = new_state;
        new_mesg.m_name  = this->m_name;

        os_ret = xQueueSend(this->m_father->m_queue_handle,&new_mesg,0);
        if(pdTRUE != os_ret)
        {
        	log_error("Send message filed.");
        	return;
        }
    }

    return;
}

static void m_key_board_task(void* pdata)
{
    m_key_board* this = NULL;
    BaseType_t  os_ret = 0 ;
    key_mesg    new_msg = {0};
    int ret = 0;

    /*参数检查*/
    if(NULL == pdata)
    {
        log_error("Param error.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    this = pdata;

    while(1)
    {
        /*死等  取消息*/
        os_ret = xQueueReceive(this->m_queue_handle,&new_msg,portMAX_DELAY);
        if(pdTRUE != os_ret)
        {
            log_error("Failed to accept message ");
            continue;
        }

        /*检查消息有效性*/
        if((KEY_UP != new_msg.m_state && KEY_PRESS != new_msg.m_state && KEY_LONG_PRESS != new_msg.m_state) || NULL == new_msg.m_name)
        {
            log_error("Failed to accept message ");
            continue;
        }

        /*调用回调函数*/
        if(NULL != this->m_callback)
        {
            ret = this->m_callback(new_msg.m_name,new_msg.m_state);
        }
    }

    return;
}

static int m_get(const c_key_board* this,const char* key_name,u8* state)
{
    const m_key_board*  m_this = NULL;
    u8 i = 0;

    if(NULL == this ||NULL == key_name ||NULL == state)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*确认键盘状态*/
    if(NULL == m_this->m_key_list)
    {
        log_error("Key list lost.");
        return E_ERROR;
    }

    /*找到对应按键*/
    for(i = 0;i < m_this->m_key_amount;++i)
    {
        if(0 == strcmp(m_this->m_key_list[i].m_name,key_name))
        {
            *state = m_this->m_key_list[i].m_state;
            return E_OK;
        }
    }

    /*如果跑这来了 就说明没有找到对应的按键*/
    log_error("Invalid key name.");
    
    return E_PARAM;
}

static int m_get_hw(const c_key_board* this,const char* key_name,u8* state)
{
    const m_key_board*  m_this = NULL;
    u8 i = 0;

    if(NULL == this ||NULL == key_name ||NULL == state)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*确认键盘状态*/
    if(NULL == m_this->m_key_list)
    {
        log_error("Key list lost.");
        return E_ERROR;
    }

    /*找到对应按键*/
    for(i = 0;i < m_this->m_key_amount;++i)
    {
        if(0 == strcmp(m_this->m_key_list[i].m_name,key_name))
        {
            /*如果按下为0*/
            if(KEY_PRESS_IS_ZERO == m_this->m_key_list[i].m_polarity)
            {
                if(GPIO_PIN_RESET == HAL_GPIO_ReadPin(m_this->m_key_list[i].m_gpio,m_this->m_key_list[i].m_pin))
                {
                    *state = KEY_PRESS;
                }
                else
                {
                    *state = KEY_UP;
                }
            }
            
            /*如果按下为1*/
            else if(KEY_PRESS_IS_ONE == m_this->m_key_list[i].m_polarity)
            {
                if(GPIO_PIN_SET == HAL_GPIO_ReadPin(m_this->m_key_list[i].m_gpio,m_this->m_key_list[i].m_pin))
                {
                    *state = KEY_PRESS;
                }
                else
                {
                    *state = KEY_UP;
                }
            }
            
            /*错误的极性*/
            else
            {
                log_error("Error polarity.");
                return E_ERROR;
            }
            
            return E_OK;
        }
    }

    /*如果跑这来了 就说明没有找到对应的按键*/
    log_error("Invalid key name.");
    
    return E_PARAM;
}

static int m_set_callback(c_key_board* this,int (*callback)(const char*,u8))
{
    m_key_board*  m_this = NULL;

    if(NULL == this || NULL == callback)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    m_this->m_callback = callback;
    

    return E_OK;
}

#endif
