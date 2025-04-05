/*****************************************************************************
* Copyright:
* File name: encoder.c
* Description: 编码器模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/04/26
* log:  V1.0  2022/04/26
*       发布：
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_ENCODER_ENABLED


#define MODULE_NAME       "encoder"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define REINSTAL  65535

/*编码器 配置信息*/
typedef struct __ENCODER_CFG
{
    TIM_TypeDef*      m_timer_addr;
    GPIO_TypeDef*     m_gpio_a ;
    u32               m_pin_a  ;
    GPIO_TypeDef*     m_gpio_b ;
    u32               m_pin_b  ;
}encoder_cfg;

/*编码器 对象*/
typedef struct __M_ENCODER
{
    u8                         m_id              ;
    bool                       m_init_state      ;
    int                        m_add_up          ;
    TIM_HandleTypeDef          m_timer           ;
    TIM_Encoder_InitTypeDef    m_timer_encoder   ;
}m_encoder;

static const encoder_cfg g_cfg[4] = {
{TIM1  , GPIOB , GPIO_PIN_13 , GPIOB , GPIO_PIN_14},
{TIM2  , GPIOA , GPIO_PIN_0  , GPIOA , GPIO_PIN_1 },
{TIM3  , GPIOA , GPIO_PIN_6  , GPIOA , GPIO_PIN_7 },
{TIM4  , GPIOB , GPIO_PIN_6  , GPIOB , GPIO_PIN_7 }
};

static int m_init (u8 timer);
static int m_get_count (u8 timer,int* count,bool clear);
static int m_clear     (u8 timer);
static int m_timer_int_handle(u32 timer_id,void* param);

const c_encoder encoder = {m_init,m_get_count,m_clear};

static m_encoder g_encoder[3];

TIM_HandleTypeDef TIM_EncoderHandle;

static int m_init (u8 timer)
{
    GPIO_InitTypeDef gpio = {0};
    int ret = 0;

    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_TIM4_CLK_ENABLE();

    /*初始化 a相*/
    gpio.Pin   = g_cfg[timer].m_pin_a;
    gpio.Mode  = GPIO_MODE_AF_INPUT;             //复用推挽输出
    gpio.Pull  = GPIO_NOPULL    ;             //上拉
    gpio.Speed = GPIO_SPEED_HIGH;             //快速            
    HAL_GPIO_Init(g_cfg[timer].m_gpio_a,&gpio);

    /*初始化 b相*/
    gpio.Pin   = g_cfg[timer].m_pin_b;
    gpio.Mode  = GPIO_MODE_AF_INPUT;             //复用推挽输出
    gpio.Pull  = GPIO_NOPULL    ;             //上拉
    gpio.Speed = GPIO_SPEED_HIGH;             //快速
    HAL_GPIO_Init(g_cfg[timer].m_gpio_b,&gpio);

    /*定时器配置*/
    g_encoder[timer].m_timer.Instance               =  g_cfg[timer].m_timer_addr      ; //定时器x
    g_encoder[timer].m_timer.Init.Prescaler         =  0                              ; //定时器分频
    g_encoder[timer].m_timer.Init.CounterMode       =  TIM_COUNTERMODE_UP             ; //向上计数模式
    g_encoder[timer].m_timer.Init.Period            =  REINSTAL                       ; //自动重装载值
    g_encoder[timer].m_timer.Init.ClockDivision     =  TIM_CLOCKDIVISION_DIV1         ; /*不分频*/

    /*a相通道*/
    g_encoder[timer].m_timer_encoder.EncoderMode  = TIM_ENCODERMODE_TI12    ;   /*模式为 两个都采集*/
    g_encoder[timer].m_timer_encoder.IC1Polarity  = TIM_ICPOLARITY_RISING ;   /*极性 双边沿*/
    g_encoder[timer].m_timer_encoder.IC1Selection = TIM_ICSELECTION_DIRECTTI;   /*TIM输入1、2、3或4被选择为分别连接到IC1，IC2，IC3或IC4*/
    g_encoder[timer].m_timer_encoder.IC1Prescaler = TIM_ICPSC_DIV1          ;   /*每次在捕获输入上检测到边缘时执行的捕获*/
    g_encoder[timer].m_timer_encoder.IC1Filter    = 10                      ;   /*滤波器*/

    /*b相通道*/
    g_encoder[timer].m_timer_encoder.IC2Polarity  = TIM_ICPOLARITY_RISING ;   /*极性 双边沿*/
    g_encoder[timer].m_timer_encoder.IC2Selection = TIM_ICSELECTION_DIRECTTI;   /*TIM输入1、2、3或4被选择为分别连接到IC1，IC2，IC3或IC4*/
    g_encoder[timer].m_timer_encoder.IC2Prescaler = TIM_ICPSC_DIV1          ;   /*每次在捕获输入上检测到边缘时执行的捕获*/
    g_encoder[timer].m_timer_encoder.IC2Filter    = 10                      ;   /*滤波器*/

    /*初始化   编码器接口*/
    HAL_TIM_Encoder_Init(&g_encoder[timer].m_timer,&g_encoder[timer].m_timer_encoder);

    /* 清零计数器 */
    __HAL_TIM_SET_COUNTER(&g_encoder[timer].m_timer, 1);

    /* 清零中断标志*/
    __HAL_TIM_CLEAR_IT(&g_encoder[timer].m_timer,TIM_IT_UPDATE);
    /* 使能定时器更新中断 */
    __HAL_TIM_ENABLE_IT(&g_encoder[timer].m_timer,TIM_IT_UPDATE);
    /* 设置更新事件请求：计数器溢出 */
    __HAL_TIM_URS_ENABLE(&g_encoder[timer].m_timer);
    
    /* 使能编码器接口 */
    HAL_TIM_Encoder_Start(&g_encoder[timer].m_timer, TIM_CHANNEL_1);
    HAL_TIM_Encoder_Start(&g_encoder[timer].m_timer, TIM_CHANNEL_2);
    
    /*注册中断回调*/
    ret = int_manage.login_timer(timer,&g_encoder[timer].m_timer,m_timer_int_handle,&g_encoder[timer]);
    if(E_OK != ret)
    {
        log_error("Int func login failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_timer_int_handle(u32 timer_id,void* param)
{
    m_encoder* m_this = NULL;

    /*检查参数*/
    if(NULL == param)
    {
        log_error("NULL pointer.");
        return E_NULL;
    }
    m_this = param;

    /* 判断当前计数器的方向*/

    /* 下溢 */
    if(__HAL_TIM_IS_TIM_COUNTING_DOWN(&m_this->m_timer))
    {
        --m_this->m_add_up;
        //log_inform("Down");
    }
    
    /* 上溢 */
    else
    {
        ++m_this->m_add_up;
        //log_inform("Up");
    }
}


//void TIM2_IRQHandler(void)
//{
//    /* 判断当前计数器的方向*/
//    if(__HAL_TIM_IS_TIM_COUNTING_DOWN(&g_encoder[1].m_timer))
//    {
//        log_inform("Down");
//    }
//    /* 下溢 */
//    //Encoder_Overflow_Count--;
//    else
//    {
//        log_inform("Up");
//    }
//    /* 上溢 */
//    //Encoder_Overflow_Count++;
//    
//    HAL_TIM_IRQHandler(&g_encoder[1].m_timer);
//}


static int m_get_count (u8 timer,int* count,bool clear)
{
    u32 timer_count;
    
    timer_count = __HAL_TIM_GetCounter(&g_encoder[timer].m_timer);
    *count = g_encoder[timer].m_add_up * REINSTAL + timer_count;

    if(clear)
    {
        __HAL_TIM_SetCounter(&g_encoder[timer].m_timer,1);
        g_encoder[timer].m_add_up = 0;
    }
    
    return E_OK;
}

static int m_clear     (u8 timer)
{
    return E_OK;
}
#endif

