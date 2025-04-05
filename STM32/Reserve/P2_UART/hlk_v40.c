/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : hlk_v40.h
* Description: hlk_v40 驱动模块
* Author: xzh
* Version: v1.0
* Date: 2022/12/07
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_HLKV40_ENABLED

typedef enum
{
    HLKV40_RESET_OK = 0,
    HLKV40_IDEL,
    HLKV40_BUSY,
    HLKV40_AT_MODE,
    HLKV40_ERROR = 0xF0,
}_HLKV40_STA;

typedef struct __M_HLKV40
{
    c_switch                    m_rst           ;  /*复位脚*/
    c_switch                    m_mode          ;  /*模式切换引脚*/
    unsigned char               m_uart_id;
    volatile _HLKV40_STA        m_state; 
}m_hlkv40;

static int m_set_volume(const c_hlkv40* this, unsigned char volume);
static int m_play_text(const c_hlkv40* this, char* format,...);
static int m_init(const c_hlkv40* this);


static int m_callback(void* param, const unsigned char* data, unsigned short data_len)
{
    int ret = 0;
    c_hlkv40    new = {0};
    m_hlkv40*   m_this = NULL;  

    /*参数检查*/
    if(NULL == param || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    if(0 == data_len)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    m_this = param;

    if(m_this->m_state   == HLKV40_RESET_OK && strstr(data, "a") != NULL)
    {
        taskENTER_CRITICAL();
        m_this->m_state = HLKV40_IDEL;
        taskEXIT_CRITICAL();
    }

    if(m_this->m_state   == HLKV40_IDEL && strstr(data, "OK") != NULL)
    {
        taskENTER_CRITICAL();
        m_this->m_state = HLKV40_AT_MODE;
        taskEXIT_CRITICAL();
    }

    if(m_this->m_state   == HLKV40_BUSY && strstr(data, "at+play") != NULL)
    {
        taskENTER_CRITICAL();
        m_this->m_state = HLKV40_AT_MODE;
        taskEXIT_CRITICAL();
    }

    return E_OK;
}

c_hlkv40 hlkv40_creat(unsigned char uart_id, GPIO_TypeDef* rst_gpio, uint32_t rst_pin)
{
    int ret = 0;
    c_hlkv40    new = {0};
    m_hlkv40*   m_this = NULL;  

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_hlkv40));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this, 0, sizeof(m_hlkv40));
    m_this = new.this; 

    /*保存相关变量*/
    new.set_volume  = m_set_volume;
    new.play_text   = m_play_text;
    new.init        = m_init;
    m_this->m_uart_id = uart_id;
    m_this->m_state   = HLKV40_ERROR;

    /*复位引脚配置*/
    m_this->m_rst = switch_create(rst_gpio, rst_pin);
    if(NULL == m_this->m_rst.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }

    /*初始化串口*/
    ret = my_uart.init(uart_id, HLKV40_BAUDRATE, 128, UART_MODE_DMA);
    if(E_OK != ret)
    {
        log_error("Uart init failed.");
        goto error_handle;
    }
    
    /*注册串口回调*/
    ret = my_uart.set_callback(uart_id, (void*)m_this, m_callback);
    if(E_OK != ret)
    {
        log_error("Callback set failed.");
        goto error_handle;
    }

    /*进入AT设置默认音量*/   
    new.init(&new);
    new.set_volume(&new, 100); 

    return new;
    
error_handle:
    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_init(const c_hlkv40* this)
{
	int ret = 0;
    m_hlkv40* m_this = NULL;
    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_ERROR;
    }
    m_this = this->this;

    /*复位*/
    m_this->m_rst.set(&m_this->m_rst, GPIO_PIN_RESET);  //拉低复位
    vTaskDelay(100);
    m_this->m_rst.set(&m_this->m_rst, GPIO_PIN_SET);    //拉高复位
    vTaskDelay(1000);    //等待复位完成
    m_this->m_state   = HLKV40_RESET_OK;

    /*进入AT指令模式*/     
    ret = my_uart.send(m_this->m_uart_id, HLK_CMD_ENTER_AT, strlen(HLK_CMD_ENTER_AT));
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
    }
    for(uint32_t i = 0; i <= HLKV40_VOER_TIME; i+=10)
    {
        if(m_this->m_state  == HLKV40_IDEL)  break;
        vTaskDelay(10);
        if(i == HLKV40_VOER_TIME)
        {
            log_error("Enter AT Mode Failed 1");
        }
    }
    ret = my_uart.send(m_this->m_uart_id, HLK_CMD_ENTER_AT2, strlen(HLK_CMD_ENTER_AT2));
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
    }
    for(uint32_t i = 0; i <= HLKV40_VOER_TIME; i+=10)
    {
        if(m_this->m_state  == HLKV40_AT_MODE)  break;
        vTaskDelay(10);
        if(i == HLKV40_VOER_TIME)
        {
            log_error("Enter AT Mode Failed 2");
        }
    }

    return E_OK;
}

static int m_play_text(const c_hlkv40* this, char* format,...)
{
	int ret = 0;
	va_list   argptr;
    m_hlkv40* m_this = NULL;
	static char str_buf[64] = {0};
    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_ERROR;
    }
    m_this = this->this;

    /*检查设备状态是否正确*/
    if(m_this->m_state != HLKV40_AT_MODE)
    {
        log_error("HLK State Error: %d.", m_this->m_state);
        return E_ERROR;
    }
    m_this->m_state   = HLKV40_BUSY;

	
	va_start( argptr, format );  // 初始化argptr	
	memset(str_buf, 0, sizeof(str_buf));
	strcat(str_buf, HLK_CMD_PALY_TEXT);
	vsnprintf(str_buf + strlen(HLK_CMD_PALY_TEXT), sizeof(str_buf), format, argptr);
	va_end(argptr);
	strcat(str_buf, "\r");
	
    /*发送播放语音命令，并等待返回*/
    ret = my_uart.send(m_this->m_uart_id, str_buf, strlen(str_buf));
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
    }

    for(uint32_t i = 0; i <= HLKV40_VOER_TIME; i+=10)
    {
        if(m_this->m_state == HLKV40_AT_MODE)  break;
        vTaskDelay(10);
        if(i == HLKV40_VOER_TIME)
        {
            log_error("Set volume Failed ");
            return E_ERROR;
        }
    }

    return E_OK;
}


static int m_set_volume(const c_hlkv40* this, unsigned char volume)
{
	int ret = 0;
    m_hlkv40* m_this = NULL;
    BaseType_t     os_ret  = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_ERROR;
    }
    m_this = this->this;

    /*检查状态是否正确*/
    if(m_this->m_state != HLKV40_AT_MODE)
    {
        log_error("HLK State Error: %d.", m_this->m_state);
        return E_ERROR;
    }
    m_this->m_state   = HLKV40_BUSY;

    /*发送设置音量命令，并等待返回*/
    char *pTemp = pvPortMalloc(40);
    if(NULL == pTemp)
    {
        log_error("Out of memory");
        return E_ERROR;
    }
    memset(pTemp, 0, 40);
    sprintf(pTemp, HLK_CMD_SET_VOL, volume);
    ret = my_uart.send(m_this->m_uart_id, pTemp, strlen(pTemp));
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
    }
    vPortFree(pTemp);

    for(uint32_t i = 0; i <= HLKV40_VOER_TIME; i+=10)
    {
        if(m_this->m_state  == HLKV40_AT_MODE)  break;
        vTaskDelay(10);
        if(i == HLKV40_VOER_TIME)
        {
            log_error("Set volume Failed ");
            return E_ERROR;
        }
    }

    return E_OK;
}

#endif
