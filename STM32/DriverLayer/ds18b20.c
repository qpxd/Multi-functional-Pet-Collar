/*****************************************************************************
* Copyright:
* File name: ds18b20.c
* Description: ds18b20温度传感器 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/01/17
*****************************************************************************/
#include "driver_conf.h"

#ifdef DRIVER_DS18B20_ENABLED

#define MODULE_NAME       "ds18b20"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define OFFLINE  false
#define ONLINE    true

typedef struct _M_DS18B20
{
    GPIO_TypeDef* m_gpio     ;  /*gpio*/
    uint32_t      m_pin      ;  /*pin*/
    uint32_t      m_old_tick ;  /*上一次采集的时间*/
    float         m_temp_buf ;  /*温度缓存*/
    bool          m_state    ;  /*传感器状态*/
}m_ds18b20;

static int ds18b20_start    (const c_ds18b20* this);
static int ds18b20_read_byte(const c_ds18b20* this,u8* data);
static int ds18b20_send_byte(const c_ds18b20* this,u8 data);
static int m_get            (const c_ds18b20* this,float* temp);



c_ds18b20 ds18b20_create(GPIO_TypeDef* gpio,uint32_t pin)
{
    c_ds18b20  new = {0};
    m_ds18b20* m_this = NULL;
    GPIO_InitTypeDef GPIO_Initure = {0};
    
    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_ds18b20));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_ds18b20));
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

static int ds18b20_start(const c_ds18b20* this)
{
    m_ds18b20* m_this = NULL;
    u8 retry=0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return 0.0f;
    }
    m_this = this->this;

    /*发送信号的过程中，如果被打断，会导致信号发送的时序不稳定
	所以此处要进入临界态*/
	taskENTER_CRITICAL();   //进入临界区
    
    be_out(m_this->m_gpio,m_this->m_pin);  /*输出模式*/
    GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);    /*先保证总线为空闲状态*/
    delay_us(20);
    GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_RESET);  /*拉低至少480us 参数复位脉冲*/
    delay_us(750);
    GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);    /*释放总线  等待15~60us后 接受ds18b20的应答信号*/
    delay_us(15);

    be_in(m_this->m_gpio,m_this->m_pin);  /*输入模式*/

    /*等待ds18b20拉低*/
    while (GPIO_PIN_SET == GPIO_GET(m_this->m_gpio,m_this->m_pin) && retry<200)
    {
        retry++;
        delay_us(1);
    };   

    /*如果超时了 返回错误*/
    if(retry>=200)
    {
        log_error("ds18b20 nack.Not pulled low.");
        goto error_handle;
    }
    else 
    {
        retry=0;
    }

    /*等待ds18b20 拉高*/
    while (GPIO_PIN_RESET == GPIO_GET(m_this->m_gpio,m_this->m_pin) && retry<240)
    {
        retry++;
        delay_us(1);
    };
    
    if(retry>=240)
    {
        log_error("ds18b20 nack.Not pulled up.");
        goto error_handle;
    }

    taskEXIT_CRITICAL();            //退出临界区	
    
    return E_OK;

    error_handle:

    taskEXIT_CRITICAL();            //退出临界区	
    be_out(m_this->m_gpio,m_this->m_pin);  /*输出模式*/
    GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);
    return E_ERROR;
}

static int ds18b20_read_byte(const c_ds18b20* this,u8* data)
{
    m_ds18b20* m_this = NULL;
    u8  i = 0,recv_data = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == data)
    {
        log_error("Null pointer.");
        return 0.0f;
    }
    m_this = this->this;

    /*发送信号的过程中，如果被打断，会导致信号发送的时序不稳定
	所以此处要进入临界态*/
	taskENTER_CRITICAL();   //进入临界区
    
    *data = 0;
    for(i = 0;i < 8;++i)
    {
        be_out(m_this->m_gpio,m_this->m_pin);                  /*输出模式*/
        GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_RESET); /*进入读模式*/
        delay_us(2);
        GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);
        be_in(m_this->m_gpio,m_this->m_pin);  /*输入模式*/
        delay_us(12);
        if(GPIO_PIN_SET == GPIO_GET(m_this->m_gpio,m_this->m_pin))
        {
            recv_data |= 0x01 << i;
        }
        
        delay_us(50);       
    }
    taskEXIT_CRITICAL();            //退出临界区	
    
    *data = recv_data;

    return E_OK;
}
static int ds18b20_send_byte(const c_ds18b20* this,u8 data)
{
    m_ds18b20* m_this = NULL;
    u8  i = 0,bit = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return 0.0f;
    }
    m_this = this->this;
    
    /*发送信号的过程中，如果被打断，会导致信号发送的时序不稳定
	所以此处要进入临界态*/
	taskENTER_CRITICAL();   //进入临界区

    be_out(m_this->m_gpio,m_this->m_pin);  /*输出模式*/
    for(i = 0;i < 8;++i)
    {
        bit = data & 0x01;
        data >>= 1;

        /*写1*/
        if(bit)
        {
            GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_RESET);    // Write 1
            delay_us(2);                            
            GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);
            delay_us(60);   
        }

        /*写0*/
        else
        {
            GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_RESET);    
            delay_us(60);                            
            GPIO_SET(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);
            delay_us(2);  

        }
    }
    taskEXIT_CRITICAL();            //退出临界区	

    return E_OK;
}

static int m_get(const c_ds18b20* this,float* temp)
{
    m_ds18b20* m_this = NULL;
    int ret = 0;
    u8 tl = 0;
    u8 th = 0;
    short temp_16 = 0;
    float temp_float = 0.0f;
    float temp_polarity = 0.0f;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == temp)
    {
        log_error("Null pointer.");
        return 0.0f;
    }
    m_this = this->this;

    /*启动通讯*/
    ret = ds18b20_start(this);
    if(E_OK != ret)
    {
        log_error("Start failed.");
        goto error_handle;
    }

    /*跳过 rom*/
    ret = ds18b20_send_byte(this,0xcc);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        goto error_handle;
    }

    /*发送启动转换命令*/
    ret = ds18b20_send_byte(this,0x44);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        goto error_handle;
    }

    /*等待转换完成*/
    //vTaskDelay(750);

    /*启动通讯*/
    ret = ds18b20_start(this);
    if(E_OK != ret)
    {
        log_error("Start failed.");
        goto error_handle;
    }

    /*跳过 rom*/
    ret = ds18b20_send_byte(this,0xcc);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        goto error_handle;
    }

    /*发送读命令*/
    ret = ds18b20_send_byte(this,0xbe);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        goto error_handle;
    }

    /*读取两个字节温度数据*/
    ret = ds18b20_read_byte(this,&tl);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        goto error_handle;
    }
    ret = ds18b20_read_byte(this,&th);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        goto error_handle;
    }

    /*判断温度正负号*/
    if(th>7)
    {
        th=~th;
        tl=~tl; 
        temp_polarity = -1.0f;      //温度为负  
    }
    else 
    {
        temp_polarity =  1.0f;      //温度为正    
    }

    /*合并高低位*/
    temp_16=th;                     //获得高八位
    temp_16<<=8;    
    temp_16+=tl;                    //获得底八位

    /*转换成浮点*/
    temp_float = (float)temp_16*0.625f / 10.0f;
    *temp = temp_float;

    return E_OK;

    error_handle:

    return E_ERROR;
}
#endif

 
