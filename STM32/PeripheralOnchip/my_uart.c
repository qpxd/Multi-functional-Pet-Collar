/*****************************************************************************
* Copyright:
* File name: my_uart.c
* Description: 串口模块 函数实现
* Author: 许少
* Version: V1.1
* Date: 2022/01/03
* log:  V1.0  2022/01/13
*       发布：
*
*       V1.1  2022/02/16
*       优化：将DMA的中断处理移动到中断管理模块中统一处理。
*****************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_UART_ENABLED


#define MODULE_NAME       "uart"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define UART_SEND_QUEUE_LEN    10
#define UART_SEND_MAX_TIME   1000   

typedef struct __UART_CFG
{  
    USART_TypeDef*       m_uart_addr       ;   /*串口地址*/
    DMA_Channel_TypeDef* m_send_dma_channel;   /*发送DMA通道*/
    DMA_Channel_TypeDef* m_recv_dma_channel;   /*接收DMA通道*/
    u32                  m_send_dma_cha_int;   /*发送dma中断通道*/
    u32                  m_recv_dma_cha_int;   /*接收dma中断通道*/
    UBaseType_t  m_send_task_pro;              /*发送任务优先级*/
    UBaseType_t  m_recv_task_pro;              /*接收任务优先级*/
    uint16_t     m_send_task_stk;              /*发送任务堆栈大小*/
    uint16_t     m_recv_task_stk;              /*接收任务堆栈大小*/
    const char*  m_send_task_name;             /*发送任务名字*/
    const char*  m_recv_task_name;             /*接收任务名字*/
}uart_cfg;

/*发送消息*/
typedef struct _UART_SEND_MSG
{
    void*  m_data     ;  /*数据地址*/
    u16    m_data_len ;  /*数据长度*/
}uart_send_msg;

typedef struct _M_UART
{
    u8                 m_my_uart_id                                    ;   /*串口id*/
    bool               m_init_state                                    ;  /*初始化状态*/
    uart_mode_t        m_uart_mode                                     ;  /*串口工作模式*/
    volatile uint16_t  m_recv_cnt;                                     ;  /*串口接收计数*/
    u8*                m_recv_buf                                      ;  /*接收缓存*/
    u8*                m_buf_head                                      ;  /*数据头*/
    u8*                m_buf_end                                       ;  /*数据尾*/
    void*              m_callback_param                                ;  /*回调函数参数*/
    int                (*m_callback)(void* param,const u8* data,const u16 data_len);  /*接收回调函数*/
    UART_HandleTypeDef m_uart_handle                                   ;  /*串口句柄        */
    DMA_HandleTypeDef  m_tx_dma_handle                                 ;  /*发送dma 句柄*/
    DMA_HandleTypeDef  m_rx_dma_handle                                 ;  /*接收dma 句柄*/
    TaskHandle_t       m_recv_task_handle                              ;  /*接收任务句柄*/
    TaskHandle_t       m_send_task_handle                              ;  /*发送任务句柄*/
    QueueHandle_t      m_send_queue_handle                             ;  /*发送消息队列*/
}m_uart;

static int m_uart_init           (u8  uart,u32 baud_rate,u16 buf_size,uart_mode_t mode);
static int m_uart_send           (u8  uart,const u8* data     ,u16 data_len);
static int m_uart_set_callback   (u8     uart,void* param,int (*callback)(void* param,const u8* data,u16 data_len));
static int m_my_dma_tx_stop(m_uart* handle);
static void m_send_task(void* pdata);
static void m_recv_task(void* pdata);
static int m_send_int_handle(u32 dma_cha,void* param);
static int m_recv_int_handle(u32 dma_cha,void* param);

const c_uart my_uart = {m_uart_init,m_uart_send,m_uart_set_callback};

/*串口配置*/
const uart_cfg  g_my_uart_cfg[3] = {{ USART1 , DMA1_Channel4 , DMA1_Channel5 , DMA1_CHA_4 , DMA1_CHA_5 , UART_1_SEND_TASK_PRO , UART_1_RECV_TASK_PRO , UART_1_SEND_TASK_STK , UART_1_RECV_TASK_STK , "uart 1 send" , "uart 1 recv"} ,
                                    { USART2 , DMA1_Channel7 , DMA1_Channel6 , DMA1_CHA_7 , DMA1_CHA_6 , UART_2_SEND_TASK_PRO , UART_1_RECV_TASK_PRO , UART_1_SEND_TASK_STK , UART_2_RECV_TASK_STK , "uart 2 send" , "uart 2 recv"} ,
                                    { USART3 , DMA1_Channel2 , DMA1_Channel3 , DMA1_CHA_2 , DMA1_CHA_3 , UART_3_SEND_TASK_PRO , UART_1_RECV_TASK_PRO , UART_1_SEND_TASK_STK , UART_3_RECV_TASK_STK , "uart 3 send" , "uart 3 recv"}};

/*串口 对象列表*/
m_uart g_my_uart[3];



static int m_uart_init           (u8 uart, u32 baud_rate, u16 buf_size, uart_mode_t mode)
{
    HAL_StatusTypeDef hal_ret  = HAL_ERROR ;
    BaseType_t        os_ret   = 0 ;
    int                   ret  = 0 ;

    /*检查参数*/
    if((MY_UART_1 != uart && MY_UART_2 != uart && MY_UART_3 != uart) || buf_size > UART_MAX_RECV_BUF_SIZE)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查初始化状态*/
    if(g_my_uart[uart].m_init_state)
    {
        return E_OK;
    }
    
    g_my_uart[uart].m_my_uart_id    = uart;
    g_my_uart[uart].m_uart_mode     = mode;
    g_my_uart[uart].m_recv_cnt      = 0;

     /*初始化串口*/
    g_my_uart[uart].m_uart_handle.Instance        = g_my_uart_cfg[uart].m_uart_addr  ;
    g_my_uart[uart].m_uart_handle.Init.BaudRate   = baud_rate                        ; //波特率
    g_my_uart[uart].m_uart_handle.Init.WordLength = UART_WORDLENGTH_8B               ; //字长为8位数据格式
    g_my_uart[uart].m_uart_handle.Init.StopBits   = UART_STOPBITS_1                  ; //一个停止位
    g_my_uart[uart].m_uart_handle.Init.Parity     = UART_PARITY_NONE                 ; //无奇偶校验位
    g_my_uart[uart].m_uart_handle.Init.HwFlowCtl  = UART_HWCONTROL_NONE              ; //无硬件流控
    g_my_uart[uart].m_uart_handle.Init.Mode       = UART_MODE_TX_RX                  ; //收发模式
    hal_ret = HAL_UART_Init(&g_my_uart[uart].m_uart_handle)             ; //HAL_UART_Init()会使能UART1
    if(HAL_OK != hal_ret)
    {
        log_error("HAL init uart failed.");
        return E_ERROR;
    }
    //开启中断接收
    if(mode == UART_MODE_ISR)
    {
        __HAL_UART_ENABLE_IT(&g_my_uart[uart].m_uart_handle, UART_IT_RXNE); 
    }
    else
    {
        /*初始化 发送DMA*/
        __HAL_LINKDMA(&g_my_uart[uart].m_uart_handle,hdmatx,g_my_uart[uart].m_tx_dma_handle);
        g_my_uart[uart].m_tx_dma_handle.Instance                 = g_my_uart_cfg[uart].m_send_dma_channel ;   //通道选择
        g_my_uart[uart].m_tx_dma_handle.Init.Direction           = DMA_MEMORY_TO_PERIPH ;   //存储器到外设
        g_my_uart[uart].m_tx_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE     ;   //外设非增量模式
        g_my_uart[uart].m_tx_dma_handle.Init.MemInc              = DMA_MINC_ENABLE      ;   //存储器增量模式
        g_my_uart[uart].m_tx_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE  ;   //外设数据长度:8位
        g_my_uart[uart].m_tx_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE  ;   //存储器数据长度:8位
        g_my_uart[uart].m_tx_dma_handle.Init.Mode                = DMA_NORMAL           ;   //外设普通模式
        g_my_uart[uart].m_tx_dma_handle.Init.Priority            = DMA_PRIORITY_MEDIUM  ;   //中等优先级
        HAL_DMA_DeInit(&g_my_uart[uart].m_tx_dma_handle);   
        HAL_DMA_Init  (&g_my_uart[uart].m_tx_dma_handle);  /*初始化DMA*/
        /*注册DMA发送回调函数*/
        ret = int_manage.login_dma(g_my_uart_cfg[uart].m_send_dma_cha_int,m_send_int_handle,&g_my_uart[uart]);
        if(E_OK != ret)
        {
            log_error("Int func login failed.");
            return E_ERROR;
        } 
        /*初始化 接收DMA*/
        __HAL_LINKDMA(&g_my_uart[uart].m_uart_handle,hdmarx,g_my_uart[uart].m_rx_dma_handle);
        g_my_uart[uart].m_rx_dma_handle.Instance                 = g_my_uart_cfg[uart].m_recv_dma_channel       ;   //通道选择
        g_my_uart[uart].m_rx_dma_handle.Init.Direction           = DMA_PERIPH_TO_MEMORY ;   //外设到存储器
        g_my_uart[uart].m_rx_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE     ;   //外设非增量模式
        g_my_uart[uart].m_rx_dma_handle.Init.MemInc              = DMA_MINC_ENABLE      ;   //存储器增量模式
        g_my_uart[uart].m_rx_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE  ;   //外设数据长度:8位
        g_my_uart[uart].m_rx_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE  ;   //存储器数据长度:8位
        g_my_uart[uart].m_rx_dma_handle.Init.Mode                = DMA_CIRCULAR         ;   //外设循环模式
        g_my_uart[uart].m_rx_dma_handle.Init.Priority            = DMA_PRIORITY_HIGH  ;   //中等优先级
        HAL_DMA_DeInit(&g_my_uart[uart].m_rx_dma_handle);   
        HAL_DMA_Init  (&g_my_uart[uart].m_rx_dma_handle);  /*初始化DMA*/
        /*注册DMA接收回调函数*/
        ret = int_manage.login_dma(g_my_uart_cfg[uart].m_recv_dma_cha_int,m_recv_int_handle,&g_my_uart[uart]);
        if(E_OK != ret)
        {
            log_error("Int func login failed.");
            return E_ERROR;
        }
    }

    /*申请接收缓存*/
    if(NULL == g_my_uart[uart].m_recv_buf)
    {
        g_my_uart[uart].m_recv_buf = pvPortMalloc(UART_MAX_RECV_BUF_SIZE);
        if(NULL == g_my_uart[uart].m_recv_buf)
        {
            log_error("Out of memory.");
            return E_OUT_OF_MEMORY;
        }
    }

    /*初始化 表示头尾在一起*/
     g_my_uart[uart].m_buf_head = g_my_uart[uart].m_buf_end =  g_my_uart[uart].m_recv_buf;

    /*创建消息队列*/
    if(NULL == g_my_uart[uart].m_send_queue_handle)
    {
        g_my_uart[uart].m_send_queue_handle = xQueueCreate(UART_SEND_QUEUE_LEN,sizeof(uart_send_msg));
        if(NULL == g_my_uart[uart].m_send_queue_handle)
        {
            log_error("Queue creat filed.");
            vPortFree(g_my_uart[uart].m_recv_buf);
            g_my_uart[uart].m_recv_buf = NULL;
            return E_ERROR;
        }
    }

    /*创建接收任务*/
    if(NULL == g_my_uart[uart].m_recv_task_handle)
    {
        os_ret = xTaskCreate((TaskFunction_t )m_recv_task    ,
                             (const char*    )g_my_uart_cfg[uart].m_recv_task_name ,
                             (uint16_t       )g_my_uart_cfg[uart].m_recv_task_stk  ,
                             (void*          )&g_my_uart[uart]                     ,
                             (UBaseType_t    )g_my_uart_cfg[uart].m_recv_task_pro  ,
                             (TaskHandle_t*  )&g_my_uart[uart].m_recv_task_handle);
                             
        if(pdPASS != os_ret)
        {
            log_error("UART recv task creat filed,ret=%d",(int)os_ret);
            vPortFree(g_my_uart[uart].m_recv_buf);
            g_my_uart[uart].m_recv_buf = NULL;
            return E_ERROR;
        }
    } 

    /*创建发送任务*/
    if(NULL == g_my_uart[uart].m_send_task_handle)
    {
        os_ret = xTaskCreate((TaskFunction_t )m_send_task    ,
                             (const char*    )g_my_uart_cfg[uart].m_send_task_name ,
                             (uint16_t       )g_my_uart_cfg[uart].m_send_task_stk  ,
                             (void*          )&g_my_uart[uart]                     ,
                             (UBaseType_t    )g_my_uart_cfg[uart].m_send_task_pro  ,
                             (TaskHandle_t*  )&g_my_uart[uart].m_send_task_handle);
                             
        if(pdPASS != os_ret)
        {
            log_error("UART send task creat filed,ret=%d",(int)os_ret);
            return E_ERROR;
        }
    } 

    g_my_uart[uart].m_init_state = true;

    return E_OK;
}

static void m_send_task(void* pdata)
{
    m_uart*        uart_handle = NULL;
    uart_send_msg      new_msg = {0} ;
    BaseType_t          os_ret = 0   ;
    HAL_StatusTypeDef  hal_ret = HAL_ERROR   ;
    int ret = 0;

    /*参数检查*/
    if(NULL == pdata)
    {
        log_error("Param error.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    uart_handle = pdata;

    while(1)
    {
        memset(&new_msg,0,sizeof(uart_send_msg));  /*清空一次内存*/

        /*死等  取消息*/
        os_ret = xQueueReceive(uart_handle->m_send_queue_handle,&new_msg,portMAX_DELAY);
        if(pdTRUE != os_ret)
        {
            log_error("Failed to accept message ");
            continue;
        }
        

        /*检查消息有效性*/
        if(0 == new_msg.m_data_len || UART_MAX_SEND_DATA_LEN < new_msg.m_data_len || NULL == new_msg.m_data)
        {
            log_error("Invalid message.");
            if(NULL != new_msg.m_data)
            {
                vPortFree(new_msg.m_data);  /*释放消息内存*/
            }
            continue;
        }

        if(uart_handle->m_uart_mode == UART_MODE_ISR)
        {
            uint32_t cnt;
            for(cnt = 0; cnt < new_msg.m_data_len; cnt++)
            {
                while((uart_handle->m_uart_handle.Instance->SR & 0x40) == 0);	
				uart_handle->m_uart_handle.Instance->DR = *((uint8_t*)new_msg.m_data + cnt);
                while((uart_handle->m_uart_handle.Instance->SR & 0x40) == 0);	
            }
        }
        else
        {
            /*使用DMA发送数据*/
            hal_ret = HAL_UART_Transmit_DMA(&uart_handle->m_uart_handle,new_msg.m_data,new_msg.m_data_len);
            if(HAL_OK != hal_ret)
            {
                log_error("Uart send failed.");
            }
            
            /*等待任务通知*/
            ret = ulTaskNotifyTake(pdTRUE,UART_SEND_MAX_TIME);
            if(0 == ret)
            {
                log_error("Uart send timeout.");  /*本条消息 发送超时*/
                
                /*停止下DMA*/
                ret = m_my_dma_tx_stop(uart_handle);
            }
        }
        vPortFree(new_msg.m_data);  /*释放消息内存*/
    }
}

static void m_recv_task(void* pdata)
{
    m_uart* uart_handle = NULL;
    HAL_StatusTypeDef  hal_ret = HAL_ERROR;
    int ret = 0;
    u16 recv_count = 0;
    u16 recv_count_old = 0;
    u16 data_len = 0,temp = 0;
    u8* recv_data = 0;

    /*参数检查*/
    if(NULL == pdata)
    {
        log_error("Param error.");
        vTaskDelete(NULL);   /*删除本任务*/
    }

    /*打开接收*/
    uart_handle = pdata;

    /*打开接收*/
	if(uart_handle->m_uart_mode != UART_MODE_ISR)
	{
		hal_ret = HAL_UART_Receive_DMA(&uart_handle->m_uart_handle,uart_handle->m_recv_buf,UART_MAX_RECV_BUF_SIZE);
		if(HAL_OK != hal_ret)
		{
			log_error("Start recv failed.");
			vTaskDelete(NULL);   /*删除本任务*/
		}
	}
	
    while(1)
    {
        /*等待任务通知*/
        ret = ulTaskNotifyTake(pdTRUE,100);
        if(0 == ret)
        {

        }
    
		if(uart_handle->m_uart_mode != UART_MODE_ISR)
		{
			recv_count = __HAL_DMA_GET_COUNTER(&uart_handle->m_rx_dma_handle);//得到当前还剩余多少个数据
			recv_count = UART_MAX_RECV_BUF_SIZE - recv_count;  /*得到DMA本次循环收到了多少个数据*/
			uart_handle->m_buf_end = uart_handle->m_recv_buf + recv_count;  /*得到新的接收指针所指向的位置 指向为下一个要写入的位置*/
		}
		else
		{
			recv_count = uart_handle->m_recv_cnt;
			uart_handle->m_buf_end = uart_handle->m_recv_buf + recv_count;  /*得到新的接收指针所指向的位置 指向为下一个要写入的位置*/
		}

        
        //log_inform("recv:%d",recv_count);

        /*计算有效数据长度*/

        /*如果头尾相等 表示没有数据*/
        if(uart_handle->m_buf_head == uart_handle->m_buf_end)
        {
            continue;
        }

        /*如果 尾在头后面*/
        else if(uart_handle->m_buf_head < uart_handle->m_buf_end)
        {
            data_len = uart_handle->m_buf_end - uart_handle->m_buf_head;

            /*为新消息开辟内存*/
            recv_data = pvPortMalloc(data_len);
            if(NULL == recv_data)
            {
                log_error("Out of memory.");
                //return E_OUT_OF_MEMORY;
            }
            memset(recv_data,0,data_len);

            /*拷贝数据*/
            memcpy(recv_data,uart_handle->m_buf_head,data_len);
        }

        /*如果 尾在头前面*/
        else if(uart_handle->m_buf_head > uart_handle->m_buf_end)
        {
            data_len = UART_MAX_RECV_BUF_SIZE - (uart_handle->m_buf_head - uart_handle->m_buf_end);

            /*为新消息开辟内存*/
            recv_data = pvPortMalloc(data_len);
            if(NULL == recv_data)
            {
                log_error("Out of memory.");
                //return E_OUT_OF_MEMORY;
            }
            memset(recv_data,0,data_len);

            /*拷贝数据*/
            temp = UART_MAX_RECV_BUF_SIZE - (uart_handle->m_buf_head - uart_handle->m_recv_buf);

            /*拷贝前段*/
            memcpy(recv_data,uart_handle->m_buf_head,temp);

            /*拷贝后段*/
            memcpy(recv_data + temp,uart_handle->m_recv_buf,uart_handle->m_buf_end - uart_handle->m_recv_buf);
        }
        else
        {
            log_error("Unknown situation.");
        }

        /*更新头部指针位置*/
        uart_handle->m_buf_head = uart_handle->m_buf_end;

        /*调用回调函数*/
        if(NULL != uart_handle->m_callback)
        {
            ret = uart_handle->m_callback(uart_handle->m_callback_param,recv_data,data_len);
        }
        
        /*释放内存*/
        vPortFree(recv_data);
        recv_data = NULL;
        data_len = 0;
        
        vTaskDelay(100);
    }
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_Initure;
    
    if(huart->Instance==USART1)//如果是串口1，进行串口1 MSP初始化
    {
        __HAL_RCC_USART1_CLK_ENABLE();          //使能USART1时钟
        __HAL_RCC_AFIO_CLK_ENABLE();
    
        GPIO_Initure.Pin    = GPIO_PIN_9           ; //PA9 TX
        GPIO_Initure.Mode   = GPIO_MODE_AF_PP      ; //复用推挽输出
        GPIO_Initure.Pull   = GPIO_PULLUP          ; //上拉
        GPIO_Initure.Speed  = GPIO_SPEED_FREQ_HIGH ; //高速
        HAL_GPIO_Init(GPIOA,&GPIO_Initure);          //初始化PA9

        GPIO_Initure.Pin    = GPIO_PIN_10;           //PA10 RX
        GPIO_Initure.Mode   = GPIO_MODE_AF_INPUT;    //模式要设置为复用输入模式！   
        HAL_GPIO_Init(GPIOA,&GPIO_Initure);          //初始化PA10      
		
		if(g_my_uart[MY_UART_1].m_uart_mode == UART_MODE_ISR)
		{
			HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);                                
			HAL_NVIC_EnableIRQ(USART1_IRQn);   
		}
    }
    else if(huart->Instance==USART2)//如果是串口2，进行串口2 MSP初始化
    {
        __HAL_RCC_USART2_CLK_ENABLE();          //使能USART1时钟
        __HAL_RCC_AFIO_CLK_ENABLE();
    
        GPIO_Initure.Pin    = GPIO_PIN_2           ; //PA2 TX
        GPIO_Initure.Mode   = GPIO_MODE_AF_PP      ; //复用推挽输出
        GPIO_Initure.Pull   = GPIO_PULLUP          ; //上拉
        GPIO_Initure.Speed  = GPIO_SPEED_FREQ_HIGH ; //高速
        HAL_GPIO_Init(GPIOA,&GPIO_Initure);          //初始化PA9

        GPIO_Initure.Pin    = GPIO_PIN_3;           //PA3 RX
        GPIO_Initure.Mode   = GPIO_MODE_AF_INPUT;    //模式要设置为复用输入模式！   
        HAL_GPIO_Init(GPIOA,&GPIO_Initure);          //初始化PA10      

		if(g_my_uart[MY_UART_2].m_uart_mode == UART_MODE_ISR)
		{
			HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);                                
			HAL_NVIC_EnableIRQ(USART2_IRQn);   
		} 		
    }
    else if(huart->Instance==USART3)//如果是串口3，进行串口3 MSP初始化
    {
        __HAL_RCC_USART3_CLK_ENABLE();          //使能USART1时钟
        __HAL_RCC_AFIO_CLK_ENABLE();
    
        GPIO_Initure.Pin    = GPIO_PIN_10           ; //PB10 TX
        GPIO_Initure.Mode   = GPIO_MODE_AF_PP      ; //复用推挽输出
        GPIO_Initure.Pull   = GPIO_PULLUP          ; //上拉
        GPIO_Initure.Speed  = GPIO_SPEED_FREQ_HIGH ; //高速
        HAL_GPIO_Init(GPIOB,&GPIO_Initure);          //初始化PA9

        GPIO_Initure.Pin    = GPIO_PIN_11;           //PB11 RX
        GPIO_Initure.Mode   = GPIO_MODE_AF_INPUT;    //模式要设置为复用输入模式！   
        HAL_GPIO_Init(GPIOB,&GPIO_Initure);          //初始化PA10      

		if(g_my_uart[MY_UART_3].m_uart_mode == UART_MODE_ISR)
		{
			HAL_NVIC_SetPriority(USART3_IRQn, 0, 0);                                
			HAL_NVIC_EnableIRQ(USART3_IRQn);   
		}  		
    }
}

static int m_uart_send(u8 uart,const u8* data     ,u16 data_len)
{
    uart_send_msg  new_msg = {0};
    BaseType_t     os_ret  = 0;

    /*检查参数*/
    if(MY_UART_1 != uart && MY_UART_2 != uart && MY_UART_3 != uart)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*参数检查*/
    if(NULL == data || 0 == data_len || UART_MAX_SEND_DATA_LEN < data_len)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查初始化状态*/
    if(!g_my_uart[uart].m_init_state)
    {
        log_error("Need init first.");
        return E_PARAM;
    }

    /*为数据申请新内存*/
    new_msg.m_data = pvPortMalloc(data_len+1);
    if(NULL == new_msg.m_data)
    {
        log_error("Out of memory.");
        return E_OUT_OF_MEMORY;
    }
    memset(new_msg.m_data,0,sizeof(data_len+1));

    /*拷贝数据*/
    memcpy(new_msg.m_data,data,data_len);
    new_msg.m_data_len = data_len;

    /*新消息 压到队列*/
    os_ret = xQueueSend(g_my_uart[uart].m_send_queue_handle,&new_msg,0);
    if(pdTRUE != os_ret)
    {
        log_error("Send message filed.");
        vPortFree(new_msg.m_data);
        new_msg.m_data = NULL;
        return E_ERROR;
    }

    return E_OK;
}

static int m_uart_set_callback   (u8 uart,void* param,int (*callback)(void* param,const u8* data,u16 data_len))
{
    /*检查参数*/
    if((MY_UART_1 != uart && MY_UART_2 != uart && MY_UART_3 != uart))
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*检查初始化状态*/
    if(!g_my_uart[uart].m_init_state)
    {
        log_error("Need init first.");
        return E_PARAM;
    }
    
    /*保存参数*/
    g_my_uart[uart].m_callback = callback;
    g_my_uart[uart].m_callback_param = param;
    
    return E_OK;
}

static int m_send_int_handle(u32 dma_cha,void* param)
{
    m_uart* m_this = NULL;
    BaseType_t ret = 0;

    /*检查参数*/
    if(NULL == param)
    {
        log_error("NULL pointer.");
        return E_NULL;
    }
    m_this = param;

    /*如果触发了发送完成DMA中断*/
    if(__HAL_DMA_GET_FLAG(&m_this->m_tx_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&m_this->m_tx_dma_handle)) != RESET)
    {       
      m_my_dma_tx_stop(m_this);
      
      /*发送任务通知 发送下一条串口消息*/
      vTaskNotifyGiveFromISR(m_this->m_send_task_handle,&ret);
      
    }
    HAL_DMA_IRQHandler(&m_this->m_tx_dma_handle);
      
      /*手动进行一次任务切换*/
    if(pdTRUE == ret)
    {
        taskYIELD();
    }
    
    return E_OK;
}

static int m_recv_int_handle(u32 dma_cha,void* param)
{
    m_uart* m_this = NULL;
    BaseType_t ret = pdFAIL;

    /*检查参数*/
    if(NULL == param)
    {
        log_error("NULL pointer.");
        return E_NULL;
    }
    m_this = param;
    
    /*如果触发了接收完成DMA中断*/
    if(__HAL_DMA_GET_FLAG(&g_my_uart[MY_UART_1].m_rx_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&g_my_uart[MY_UART_1].m_rx_dma_handle)) != RESET)
    {
        /*发送任务通知 立即查看DMA接收内容*/
        vTaskNotifyGiveFromISR(m_this->m_recv_task_handle,&ret);
    
    
        /*表示缓存已经满了 */
        //log_inform("Buf full.");
    }
    
    HAL_DMA_IRQHandler(&m_this->m_rx_dma_handle);

     /*手动进行一次任务切换*/
    if(pdTRUE == ret)
    {
        taskYIELD();
    }
    
}

int m_my_dma_tx_stop(m_uart* handle)
{
    /*检查参数*/
    if(NULL == handle)
    {
        log_error("NULL pointer.");
        return E_NULL;
    }
    
    /* Disable the UART Tx DMA requests   停止发送DMA请求 并关闭发送DMA*/
    CLEAR_BIT(handle->m_uart_handle.Instance->CR3, USART_CR3_DMAT);
    HAL_DMA_Abort(&handle->m_tx_dma_handle);
    
    /*因为上文中 发送停止了，所以 清楚串口状态中的 发送忙状态*/
    if(HAL_UART_STATE_BUSY_TX == handle->m_uart_handle.State)
    {
        handle->m_uart_handle.State = HAL_UART_STATE_READY;
    }
    else if(HAL_UART_STATE_BUSY_TX_RX == handle->m_uart_handle.State)
    {
        handle->m_uart_handle.State = HAL_UART_STATE_BUSY_RX;
    }
    
    return E_OK;
}


void USART1_IRQHandler(void)
{
    if(__HAL_UART_GET_FLAG(&g_my_uart[MY_UART_1].m_uart_handle, UART_FLAG_RXNE) != RESET)
    {		
        uint8_t nRec = (uint16_t)READ_REG(g_my_uart[MY_UART_1].m_uart_handle.Instance->DR);
        if(++g_my_uart[MY_UART_1].m_recv_cnt >= UART_MAX_RECV_BUF_SIZE) g_my_uart[MY_UART_1].m_recv_cnt = 0;
        g_my_uart[MY_UART_1].m_recv_buf[g_my_uart[MY_UART_1].m_recv_cnt] = nRec;
    }
}   

void USART2_IRQHandler(void)
{
    if(__HAL_UART_GET_FLAG(&g_my_uart[MY_UART_2].m_uart_handle, UART_FLAG_RXNE) != RESET)
    {		
        uint8_t nRec = (uint16_t)READ_REG(g_my_uart[MY_UART_2].m_uart_handle.Instance->DR);
        if(++g_my_uart[MY_UART_2].m_recv_cnt >= UART_MAX_RECV_BUF_SIZE) g_my_uart[MY_UART_2].m_recv_cnt = 0;
        g_my_uart[MY_UART_2].m_recv_buf[g_my_uart[MY_UART_2].m_recv_cnt] = nRec;
    }
}

void USART3_IRQHandler(void)
{
    if(__HAL_UART_GET_FLAG(&g_my_uart[MY_UART_3].m_uart_handle, UART_FLAG_RXNE) != RESET)
    {		
        uint8_t nRec = (uint16_t)READ_REG(g_my_uart[MY_UART_3].m_uart_handle.Instance->DR);
        if(++g_my_uart[MY_UART_3].m_recv_cnt >= UART_MAX_RECV_BUF_SIZE) g_my_uart[MY_UART_3].m_recv_cnt = 0;
        g_my_uart[MY_UART_3].m_recv_buf[g_my_uart[MY_UART_3].m_recv_cnt] = nRec;
    }
}
#endif 
