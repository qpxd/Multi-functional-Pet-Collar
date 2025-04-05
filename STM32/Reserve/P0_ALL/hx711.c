/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : hx711.h
* Description: hx711称重模块 函数实现
* Author: xzh
* Version: v1.0
* Date: 2022/02/08
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_HX711_ENABLED

#define MODULE_NAME       "hx711"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

typedef struct _M_HX711
{
    GPIO_TypeDef* m_gpio_sck     ;
    u32           m_pin_sck      ;
    GPIO_TypeDef* m_gpio_dio     ;
    u32           m_pin_dio      ;
    u32           m_zero         ;  /*零点*/
}m_hx711;


static int m_zero(const c_hx711* this);
static int m_get (const c_hx711* this,float* weight);
static int m_read(const c_hx711* this,u32* data);


c_hx711 hx711_create(GPIO_TypeDef* sck_gpio,u32 sck_pin,GPIO_TypeDef* dio_gpio,u32 dio_pin)
{
    c_hx711  new = {0};
    m_hx711* m_this = NULL;
    GPIO_InitTypeDef GPIO_Initure;

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_hx711));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_hx711));
    m_this = new.this;

    /*引脚配置*/
    GPIO_Initure.Pin   = sck_pin             ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP ;  //输出
    GPIO_Initure.Pull  = GPIO_PULLUP         ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;  //高速
    HAL_GPIO_Init(sck_gpio,&GPIO_Initure);  

    GPIO_Initure.Pin   = dio_pin             ;
    GPIO_Initure.Mode  = GPIO_MODE_AF_INPUT  ;  //输入
    GPIO_Initure.Pull  = GPIO_PULLUP         ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;  //高速
    HAL_GPIO_Init(dio_gpio,&GPIO_Initure);  
    

    /*保存相关变量*/
    new.zero = m_zero  ;
    new.get  = m_get   ;
    m_this->m_gpio_sck = sck_gpio;
    m_this->m_pin_sck  = sck_pin;
    m_this->m_gpio_dio = dio_gpio;
    m_this->m_pin_dio  = dio_pin;
    
    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_zero(const c_hx711* this)
{
    m_hx711* m_this = NULL;
    u32   o_data = 0;
    u32   n_data = 0;
    int ret = 0;
    u8 i = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    do
    {
        vTaskDelay(500);
        ret = m_read(this,&o_data);
        if(E_OK != ret)
        {
            log_error("Read failed.");
            return E_ERROR;
        }
        vTaskDelay(500);
        ret = m_read(this,&n_data);
        if(E_OK != ret)
        {
            log_error("Read failed.");
            return E_ERROR;
        }
        ++i;
        if(i > 20)
        {
            log_error("HX711 get zero timeout.");
            return E_ERROR;
        }
        log_inform("HX711 get zero,time:%ds.",i);
    }while(abs(o_data  - n_data) > 5);

    /*保存零点*/
    m_this->m_zero = n_data;

    return E_OK;
}
static int m_get (const c_hx711* this,float* weight)
{
    m_hx711* m_this = NULL;
    float f_data = 0;
    u32   u_data = 0;
    int ret = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == weight)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    if(0 == m_this->m_zero)
    {
        log_error("Please zero first.");
        return E_ERROR;
    }

    /*获取读数*/
    ret = m_read(this,&u_data);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        return E_ERROR;
    }

    /*换算读数*/
    if(u_data > m_this->m_zero)
    {
        u_data -= m_this->m_zero;
        f_data  = (float)u_data / GAPVALUE;
    }
    else
    {
        f_data = 0.0f;
    }

    *weight = f_data;

    return E_OK;
}

static int m_read(const c_hx711* this,u32* data)
{
    m_hx711* m_this = NULL;
    u8 i = 0;
    u32 recv_data = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    HAL_GPIO_WritePin(m_this->m_gpio_sck,m_this->m_pin_sck,GPIO_PIN_RESET);  //使能AD（SCK 置低）

    /*等待AD转换结束*/
    while(GPIO_PIN_SET == HAL_GPIO_ReadPin(m_this->m_gpio_dio,m_this->m_pin_dio))
    {
        vTaskDelay(5);
        ++i;
        if(i > 20)
        {
            log_error("Conversion timeout.");
            return E_ERROR;
        }
    }
    
    taskENTER_CRITICAL();   //进入临界区

    for (i=0;i<24;i++)
    {
        HAL_GPIO_WritePin(m_this->m_gpio_sck,m_this->m_pin_sck,GPIO_PIN_SET);   //SCK 置高（发送脉冲）
        recv_data = recv_data << 1; //下降沿来时变量data左移一位，右侧补零
        HAL_GPIO_WritePin(m_this->m_gpio_sck,m_this->m_pin_sck,GPIO_PIN_RESET); //SCK 置低
        if(GPIO_PIN_SET == HAL_GPIO_ReadPin(m_this->m_gpio_dio,m_this->m_pin_dio))
        {
            recv_data++;
        }
    }
    HAL_GPIO_WritePin(m_this->m_gpio_sck,m_this->m_pin_sck,GPIO_PIN_SET);
    recv_data = recv_data^0x800000;//第25个脉冲下降沿来时，转换数据
    HAL_GPIO_WritePin(m_this->m_gpio_sck,m_this->m_pin_sck,GPIO_PIN_RESET);
    
    taskEXIT_CRITICAL();  /*退出临界态*/

    *data = recv_data;

    return E_OK;
}
#endif


