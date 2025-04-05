/*****************************************************************************
* Copyright:
* File name: hc_sr04.c
* Description: 超声模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2021/12/29
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_HCSR04_ENABLED

#define MODULE_NAME       "hc_sr04"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define HC_SR04_TIME      TIM1         /*使用定时器1           */
#define HC_SRO4_TIME_IRQ  TIM1_UP_IRQn /*使用定时器1的更新中断 */
#define TIME_PSC          (  72 - 1)   /*时钟分频系数*/
#define TIME_ARR          (60000 - 1)   /*时钟重装载值*/
                                       /*72000000 / 72 = 1000000 / 1000 = 1000  1/60000秒*/
#define SPEED_OF_SOUND_CM 34000.0f     /*声速*/
#define MAX_TIME_MS       500          /*最大测距时间*/
#define FOLLOW_UP         true         /*上升沿标志*/
#define FOLLOW_DOWN       false        /*下降沿标志*/
#define LOCKING           true         /*锁定*/
#define UNLOCKING         false        /*解锁*/

typedef struct _M_HC_SR04
{
	GPIO_TypeDef* m_trig_gpio     ;  /*trig gpio*/
	uint32_t      m_trig_pin      ;  /*trig  pin*/
	GPIO_TypeDef* m_echo_gpio     ;  /*echo gpio*/
	uint32_t      m_echo_pin      ;  /*echo  pin*/
}m_hc_sr04;

static SemaphoreHandle_t g_semaphore    ;   /*二值信号量句柄*/
static TIM_HandleTypeDef g_tim1_handler ;   /*定时器句柄*/
static GPIO_TypeDef*     g_gpio_waiting ;   /*正在等待中断的GPIO*/
static GPIO_PinState     g_echo_state   ;   /*echo的状态*/
static bool              g_overrange    ;   /*超量程*/
static bool              g_base_init    ;   /*基础初始化*/
static bool              g_follow       ;   /*沿状态*/
static bool              g_lock         ;   /*锁状态*/

static void  m_tim1_init(u16 arr,u16 psc);
static float hc_sr04_get_dis(c_hcsr04* this,float temp);
static int   hcsr04_init(void);

/************************************************* 
* Function: sys_boot 
* Description: 系统启动函数
* Input : 无
* Output: 无
* Return: 无
* Others: 无
*************************************************/
static int	hcsr04_init(void)
{
    float max_time = 0;
    u16   time_arr = 0;

	/*检查初始化状态*/
	if(g_base_init)
	{
		return E_OK;
	}

    /*计算最大量程 所需要的时间*/
    max_time = MAX_DIS_CM / SPEED_OF_SOUND_CM;
    time_arr = max_time * (72000000.0f / (TIME_PSC + 1)) * 2.0f;

	/*初始化时钟*/
	m_tim1_init         (time_arr,TIME_PSC ); /*初始化时钟*/
	__HAL_TIM_DISABLE   (&g_tim1_handler   ); /*默认先关闭时钟 需要用的时候才打开*/
	__HAL_TIM_SetCounter(&g_tim1_handler,0 ); /*默认讲计数值清零*/
	
	/*初始化外部中断*/
	HAL_NVIC_SetPriority (EXTI15_10_IRQn,EXTI_10_15_PRO,0); 	
	HAL_NVIC_EnableIRQ  (EXTI15_10_IRQn);      //先禁用中断线15-10
	
	/*初始化 二值信号量*/
	if(NULL == g_semaphore)
	{
		g_semaphore = xSemaphoreCreateBinary();  /*创建二值信号量*/
		if(NULL == g_semaphore)
		{
			log_error("Signal creat filed.");
			return E_ERROR;
		}
	}
	
	g_base_init = true;       /*标记为初始化成功*/
    g_lock      = UNLOCKING;  /*解锁*/

	return E_OK;
}



c_hcsr04 hcsr04_creat(GPIO_TypeDef* trig_gpio,uint32_t trig_pin,GPIO_TypeDef* echo_gpio,uint32_t echo_pin)
{
	c_hcsr04         new          = {0}  ;
	GPIO_InitTypeDef GPIO_Initure = {0}  ;  
	m_hc_sr04*       m_this       = NULL ;
	
	/*检查初始化定时器1用于超声计时*/
	if(!g_base_init)
	{
        hcsr04_init();
	}
	
	/*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_hc_sr04));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_hc_sr04));
	m_this = new.this;
	
	/*初始化相应的GPIO*/
	
	/*初始化 trig*/
	GPIO_Initure.Pin   = trig_pin              ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP   ;  //推挽输出
    GPIO_Initure.Pull  = GPIO_PULLUP           ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH  ;  //高速
    HAL_GPIO_Init(trig_gpio,&GPIO_Initure)     ;  
	HAL_GPIO_WritePin(trig_gpio,trig_pin,GPIO_PIN_RESET);  /*默认输出低电平*/
	
	/*初始化 echo*/
	GPIO_Initure.Pin   = echo_pin                    ;
    GPIO_Initure.Mode  = GPIO_MODE_INPUT ;  //上下沿中断
    GPIO_Initure.Pull  = GPIO_PULLDOWN               ;  //浮空
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH        ;  //高速
    HAL_GPIO_Init(echo_gpio,&GPIO_Initure)           ;  
    
    /*初始化外部中断*/
    exti_cfg(echo_gpio,echo_pin,GPIO_MODE_IT_RISING);
    EXTI_IT_DISABLE(echo_pin);
	
	/*保存相关变量*/
	m_this->m_trig_gpio = trig_gpio;
	m_this->m_trig_pin  = trig_pin ;
	m_this->m_echo_gpio = echo_gpio;
	m_this->m_echo_pin  = echo_pin ;
    new.this = m_this;
    new.get_dis = hc_sr04_get_dis;
	
	/*返回新建的对象*/
	return new;
}


/************************************************* 
* Function: m_tim1_init 
* Description: 定时器1初始化
* Input : <arr>  自动重装值    0~65535
*         <psc>  时钟预分频数  0~65535
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 定时器溢出时间计算方法:Tout=((arr+1)*(psc+1))/Ft us.
*************************************************/
static void m_tim1_init(u16 arr,u16 psc)
{  	
    g_tim1_handler.Instance           = TIM1           ;  //定时器
    g_tim1_handler.Init.Prescaler     = psc                    ;  //分频系数
    g_tim1_handler.Init.CounterMode   = TIM_COUNTERMODE_UP     ;  //向上计数器
    g_tim1_handler.Init.Period        = arr                    ;  //自动装载值
    g_tim1_handler.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1 ;  //时钟分频因子
    HAL_TIM_Base_Init(&g_tim1_handler);
    __HAL_TIM_DISABLE   (&g_tim1_handler   ); /*默认先关闭时钟 需要用的时候才打开*/
	
	HAL_NVIC_SetPriority  (TIM1_UP_IRQn,TIME_1_UP_PRO,0); //设置中断优先级
	HAL_NVIC_EnableIRQ    (TIM1_UP_IRQn    ); //开启ITM1中断 
    //   
    //HAL_TIM_Base_Start_IT (&g_tim1_handler ); //使能定时器1和定时器1更新中断：TIM_IT_UPDATE   
}

static float hc_sr04_get_dis(c_hcsr04* this,float temp)
{
	m_hc_sr04* m_this = this->this ;
	BaseType_t ret = 0;
	float m_dis = 0.0f;
	u32   time_count = 0;
    u16  count = 0;

	/*检查参数*/
	if(NULL == this || NULL == this->this)
	{
		log_error("Null pointer.");
		return 0.0f;
	}
    
    /*检查锁状态*/
    if(LOCKING == g_lock)
    {
        log_error("Resources are locked.");
        return 0.0f;
    }
    
    g_lock = LOCKING; /*锁定*/

	/*清空一次信号量 保证开始前 信号量是空的 同时保证电平是低*/
	xSemaphoreTake(g_semaphore,0);
	HAL_GPIO_WritePin(m_this->m_trig_gpio, m_this->m_trig_pin, GPIO_PIN_RESET);
    
    g_follow = FOLLOW_UP;                     /*准备接收一个上升沿 */
    g_gpio_waiting = m_this->m_echo_gpio;     /*保存等待采集中断的GPIO分组*/
    g_overrange = false ;                     /*超量程清零*/
    __HAL_TIM_DISABLE(&g_tim1_handler);       /*定时器停止         */
    __HAL_TIM_SetCounter(&g_tim1_handler,0);                     /*清空定时器         */ 
    //__HAL_TIM_CLEAR_IT(&g_tim1_handler,TIM_IT_UPDATE);
	
    //__HAL_GPIO_EXTI_CLEAR_IT(m_this->m_echo_pin);
    //exti_set_follow(m_this->m_echo_pin,GPIO_MODE_IT_RISING);
    //EXTI_IT_ENABLE(m_this->m_echo_pin);
	
	
	/*发送一个高脉冲 启动发送超声波*/
	HAL_GPIO_WritePin(m_this->m_trig_gpio, m_this->m_trig_pin, GPIO_PIN_SET);
	delay_us(10);
	HAL_GPIO_WritePin(m_this->m_trig_gpio, m_this->m_trig_pin, GPIO_PIN_RESET);
    
    while(GPIO_PIN_RESET == HAL_GPIO_ReadPin(m_this->m_echo_gpio, m_this->m_echo_pin) && count < 2000)
    {
        delay_us(1);
        ++count;
    }
    if(count > 2000)
    {
        log_error("Echo get time out.");
		goto error_handle;
    }
    count = 0;
    
    __HAL_TIM_ENABLE(&g_tim1_handler);
    
    while(GPIO_PIN_SET == HAL_GPIO_ReadPin(m_this->m_echo_gpio, m_this->m_echo_pin) && count < 2000)
    {
        delay_us(1);
        ++count;
    }
    if(count > 2000)
    {
        log_error("Echo get time out.");
		goto error_handle;
    }
    
    /*计算距离*/
	time_count = __HAL_TIM_GetCounter(&g_tim1_handler);
	m_dis      = ((float)(time_count) / 58.823529f);
    //log_inform("Time:%d,count:%d.",time_count,g_time_overflow);

	/*清空计时器*/
    //__HAL_TIM_DISABLE_IT(&g_tim1_handler, TIM_IT_UPDATE); 
	__HAL_TIM_SetCounter(&g_tim1_handler,0);
    g_lock = UNLOCKING; /*解锁*/
    
    return m_dis;

	/*等待超声返回  超声返回后，会在中断中开启定时器一*/ 
//	ret = xSemaphoreTake(g_semaphore,MAX_TIME_MS);
//	if(pdPASS != ret)
//	{
//		log_error("Echo get time out.");
//		goto error_handle;
//	}

	/*确认声音是否返回*/
//	if(GPIO_PIN_SET != g_echo_state)
//	{
//		log_error("Echo state error");
//		goto error_handle;
//	}
	g_follow = FOLLOW_DOWN;  /*确认声音返回 准备接收一次下降沿*/

	/*等待脉冲结束*/
	ret = xSemaphoreTake(g_semaphore,MAX_TIME_MS);
	if(pdPASS != ret)
	{
		log_error("Echo get time out.");
		goto error_handle;
	}	

    /*如果没有超量程*/
    if(false == g_overrange)
    {
    	/*确认脉冲结束 中断中 会关闭定时器 和外部中断*/
//    	if(GPIO_PIN_RESET != g_echo_state)
//    	{
//    		log_error("Echo state error");
//    		goto error_handle;
//    	}
        time_count = __HAL_TIM_GetCounter(&g_tim1_handler);
    }

    /*如果超量程 手动关下中断线*/
    else
    {
        EXTI_IT_DISABLE(m_this->m_echo_pin);
        time_count = TIM1->ARR;
    }

	/*计算距离*/
	
	m_dis      = ((float)(time_count) / 58.823529f);
    //log_inform("Time:%d,count:%d.",time_count,g_time_overflow);

	/*清空计时器*/
    __HAL_TIM_DISABLE_IT(&g_tim1_handler, TIM_IT_UPDATE); 
	__HAL_TIM_SetCounter(&g_tim1_handler,0);
    g_lock = UNLOCKING; /*解锁*/
    
	return m_dis;

	error_handle:
    EXTI_IT_DISABLE(m_this->m_echo_pin);
    __HAL_TIM_DISABLE_IT(&g_tim1_handler, TIM_IT_UPDATE); 
	__HAL_TIM_DISABLE    (&g_tim1_handler);
	__HAL_TIM_SetCounter (&g_tim1_handler,0);
    g_lock = UNLOCKING; /*解锁*/	
	return 0.0f;
	
}

///*定时器1 句柄*/
//void TIM1_UP_IRQHandler(void)
//{
//    BaseType_t ret = pdFALSE;
//    
//    HAL_TIM_IRQHandler(&g_tim1_handler);

//    /*肯定是在收下降沿 出现了超限*/
//    if(FOLLOW_DOWN == g_follow)
//    {
//        __HAL_TIM_DISABLE(&g_tim1_handler);
//        __HAL_TIM_SetCounter (&g_tim1_handler,0);
//    	g_overrange = true;  /*标记为超量程*/
//        xSemaphoreGiveFromISR(g_semaphore,&ret);

//        /*手动进行一次任务切换*/
//    	if(pdTRUE == ret)
//    	{
//    		taskYIELD();
//    	}
//    }

//    /*如果是别的边沿 流程就紊乱了*/
//    else
//    {
//        log_error("Wrong clock interrupt.");
//    }
//}
 

 
//void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
//{
//    BaseType_t ret = pdFALSE;
//    
//    g_echo_state = HAL_GPIO_ReadPin(g_gpio_waiting, GPIO_Pin);   /*保存触发中断的GPIO状态*/
//    
//    if(GPIO_PIN_SET == g_echo_state)
//    {
//        __HAL_TIM_ENABLE_IT(&g_tim1_handler, TIM_IT_UPDATE); 
//        __HAL_TIM_ENABLE(&g_tim1_handler);
//        exti_set_follow(GPIO_Pin,GPIO_MODE_IT_FALLING);
//        
//        //log_error("Up.");
//    }
//    else
//    {
//        __HAL_TIM_DISABLE(&g_tim1_handler);
//        EXTI_IT_DISABLE(GPIO_Pin);
//        //log_error("down.");
//    }
//    
//    xSemaphoreGiveFromISR(g_semaphore,&ret);
//    
//    /*手动进行一次任务切换*/
//	if(pdTRUE == ret)
//	{
//		taskYIELD();
//	}
//}
#endif

