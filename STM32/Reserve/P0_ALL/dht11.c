/*****************************************************************************
* Copyright:
* File name: dht11.c
* Description: 温湿度传感器实现
* Author: 许少
* Version: V1.2
* Date: 2022/03/18
* log:  V1.0  2022/01/13
*       发布：
*
*       V1.1  2022/02/05
*       修复：m_get中没有临界态保护，会导致接收发生错误。
*
*       V1.2  2022/03/18
*       优化：m_start中，不同厂家的dht11检测不到启动电平，故启动低电平从20增加
*             到25，提示代码兼容性。
*****************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_DHT11_ENABLED

#define MODULE_NAME       "dht11"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG      MODULE_NAME

#define DHT11_READ_INTERVAL_MS   1100  /*dht11读取间隔*/
#define DHT11_OFFLINE           false
#define DHT11_ONLINE             true


typedef struct _M_DHT11
{
    GPIO_TypeDef* m_gpio     ;  /*gpio*/
    uint32_t      m_pin      ;  /*pin*/
    uint32_t      m_old_tick ;  /*上一次采集的时间*/
    u8            m_temp_buf ;  /*温度缓存*/
    u8            m_humi_buf ;  /*湿度缓存*/
    bool          m_state    ;  /*传感器状态*/
}m_dht11;

static int m_get(const c_dht11* this,u8* temp,u8* humi);
static int m_start(const c_dht11* this);


c_dht11 dht11_create(GPIO_TypeDef* gpio,uint32_t pin)
{
    c_dht11  new = {0};
    m_dht11* m_this = NULL;
    GPIO_InitTypeDef GPIO_Initure = {0};
    
    /*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_dht11));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_dht11));
	m_this = new.this;
	
	/*初始化相应的GPIO*/
	
	/*初始化 trig*/
	GPIO_Initure.Pin   = pin              ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP   ;  //推挽输出
    GPIO_Initure.Pull  = GPIO_PULLUP           ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH  ;  //高速
    HAL_GPIO_Init(gpio,&GPIO_Initure)     ;  
	HAL_GPIO_WritePin(gpio,pin,GPIO_PIN_SET);  /*默认输出高电平*/

    /*保存相关变量*/
    m_this->m_gpio = gpio;
    m_this->m_pin  = pin;
    new.get        = m_get;

    /*返回新建的对象*/
    return new;
}

static int m_start(const c_dht11* this)
{
    m_dht11* m_this = NULL;
    u8 retry=0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    be_out(m_this->m_gpio,m_this->m_pin);                   /*输出*/
    GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_RESET);  /*拉低*/
    vTaskDelay(25);                                         /*至少18ms*/
    GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);    /*拉高*/
    delay_us(30);                                           /*主机拉高 20~40us 等待从机响应*/
    
    be_in (m_this->m_gpio,m_this->m_pin);                   /*输入*/
    
    /*DHT11会拉低40~80us*/
    while (GPIO_PIN_SET == GPIO_GET(m_this->m_gpio,m_this->m_pin) && retry<100)
    {
        retry++;
        delay_us(1);
    }; 

    /*如果超时 表示dht11无响应*/
    if(retry>=100)
    {
        log_error("dht11 nack.");
        be_out(m_this->m_gpio,m_this->m_pin);                   /*输出*/
        GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);    /*恢复空闲状态*/
        return E_ERROR;
    }
    else 
    {
        retry=0;
    }

    /*dht11 会再次拉高 40~80us*/
    while (GPIO_PIN_RESET == GPIO_GET(m_this->m_gpio,m_this->m_pin) && retry<100)
    {
        retry++;
        delay_us(1);
    };

    
    /*如果超时 表示dht11无响应*/
    if(retry>=100)
    {
        log_error("dht11 nack.");
        be_out(m_this->m_gpio,m_this->m_pin);                   /*输出*/
        GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);    /*恢复空闲状态*/
        return E_ERROR;
    }
    
    return E_OK;
}

static int m_get(const c_dht11* this,u8* temp,u8* humi)
{
    int ret = 0;
    m_dht11* m_this = NULL;
    u8 retry=0;
    u8 buf_data[5] = {0};
    u8 data = 0;
    u8 i = 0,j = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == temp || NULL == humi)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    *temp = 0;
    *humi = 0;

    /*如果上一次启动 距离现在启动的时间 小于时间间隔 就把上一次的数据返回出去*/
    if(HAL_GetTick() - m_this->m_old_tick < DHT11_READ_INTERVAL_MS && 0 != m_this->m_old_tick)
    {
        /*如果上一次采集 dht11离线了 就报错*/
        if(DHT11_OFFLINE == m_this->m_state)
        {
            *temp = 0;
            *humi = 0;
            log_error("dht11 is offline.");
            return E_ERROR;
        }

        /*如果上一次采集成功了 就返回缓存*/
        else 
        {
            *temp = m_this->m_temp_buf;
            *humi = m_this->m_humi_buf;
            return E_OK;
        }
    }

    /*可以启动一次新的转换*/
    ret = m_start(this);
    m_this->m_old_tick = HAL_GetTick();
    if(E_OK != ret)
    {
        m_this->m_state = DHT11_OFFLINE;
        log_error("dht11 start failed.");
        return E_ERROR;
    }
    else
    {
        m_this->m_state = DHT11_ONLINE;
    }

    taskENTER_CRITICAL();   //进入临界区
    
    /*接收5个字节*/
    be_in (m_this->m_gpio,m_this->m_pin);  /*输入*/
    for(i = 0;i < 5;++i)
    {
        data = 0;

        /*接收一个字节的8个位*/
        for(j = 0;j < 8;++j)
        {
            retry = 0;

            /*一个bit开始 DHT11会先拉低12~14us*/
            while (GPIO_PIN_SET == GPIO_GET(m_this->m_gpio,m_this->m_pin) && retry<100)
            {
                retry++;
                delay_us(1);
            }; 

            /*如果超时 表示dht11无响应*/
            if(retry>=100)
            {
                log_error("Not pulled low.");
                goto error_handle;
            }
            else 
            {
                retry=0;
            }

            /*总线拉低后，等待总线拉高*/
            while (GPIO_PIN_RESET == GPIO_GET(m_this->m_gpio,m_this->m_pin) && retry<100)
            {
                retry++;
                delay_us(1);
            };
            
            /*如果超时 表示dht11无响应*/
            if(retry>=100)
            {
                log_error("Not pulled up.");
                goto error_handle;
            }

            /*dht11 拉高26~28us表示0 拉高116~118us 表示1
              所以我们延时40us后 来看下总线电平 就知道
              该bit是0 还是1了*/
            delay_us(40);

            data <<= 1;
            if(GPIO_PIN_SET == GPIO_GET(m_this->m_gpio,m_this->m_pin))
            {
                data |= 0x01;
            }
              
        }

        buf_data[i] = data;
    }
    taskEXIT_CRITICAL();  /*退出临界态*/
    be_out(m_this->m_gpio,m_this->m_pin);                   /*输出*/
    GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);    /*恢复空闲状态*/

    /*接收完毕后 进行数值校验*/
    data = buf_data[0] + buf_data[1] + buf_data[2] + buf_data[3];
    if(data != buf_data[4])
    {
        log_error("Incorrect data received.");
        return E_ERROR;
    }

    /*转换数据*/
    m_this->m_humi_buf = buf_data[0];
    m_this->m_temp_buf = buf_data[2];
    *temp = m_this->m_temp_buf;
    *humi = m_this->m_humi_buf;

    return E_OK;
    

    error_handle:
    
    taskEXIT_CRITICAL();  /*退出临界态*/
    be_out(m_this->m_gpio,m_this->m_pin);                   /*输出*/
    GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);    /*恢复空闲状态*/
    return E_ERROR;
}

#endif