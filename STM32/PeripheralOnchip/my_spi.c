/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : spi.c
* Description: spi通讯模块 函数实现
* Author: xzh
* Version: v1.1
* Date: 2022/04/08
* log:  V1.0  2022/02/06
*       发布：
*
*       V1.1  2022/04/02
*       变更：分配系数从SPI_BAUDRATEPRESCALER_2 更改为SPI_BAUDRATEPRESCALER_4。
*****************************************************************************/
#include "onchip_conf.h"

#ifdef ONCHIP_SPI_ENABLED

#define MODULE_NAME       "spi"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

typedef struct __SPI_CFG
{  
    SPI_TypeDef*         m_spi_addr       ;   /*spi地址*/
    DMA_Channel_TypeDef* m_send_dma_channel;   /*发送DMA通道*/
    DMA_Channel_TypeDef* m_recv_dma_channel;   /*接收DMA通道*/
    u32                  m_send_dma_cha_int;   /*发送dma中断通道*/
    u32                  m_recv_dma_cha_int;   /*接收dma中断通道*/
    GPIO_TypeDef*        m_spi_gpio        ;
    u32                  m_pin_sck         ;
    u32                  m_pin_miso        ;
    u32                  m_pin_mosi        ;
}spi_cfg;

typedef struct _M_SPI
{
    u8                m_id;
    bool              m_init_state;
    SPI_HandleTypeDef m_spi_handle;
    DMA_HandleTypeDef m_tx_dma_handle;
    DMA_HandleTypeDef m_rx_dma_handle;
    SemaphoreHandle_t m_semaphore    ;  /*二值信号量 */
}m_spi;

static int m_transmission(u8 spi,const u8* send,u8* recv,u16 len,bool incremental,TickType_t time);
static int m_set_datasize(u8 spi,u8 datasize);
static int m_init  (u8 spi);
static int m_send_int_handle(u32 dma_cha,void* param);
static int m_recv_int_handle(u32 dma_cha,void* param);
static int m_enable(u8 spi,bool en);
static int m_set_speed(u8 spi,u8 speed);

const c_spi my_spi = {m_init,m_enable,m_set_speed,m_set_datasize,m_transmission};

/*spi配置*/
static const spi_cfg g_cfg[2] = {{SPI1,DMA1_Channel3,DMA1_Channel2,DMA1_CHA_3,DMA1_CHA_2,
                               GPIOA,GPIO_PIN_5,GPIO_PIN_6,GPIO_PIN_7},
                               {SPI2,DMA1_Channel5,DMA1_Channel4,DMA1_CHA_5,DMA1_CHA_4,
                               GPIOB,GPIO_PIN_13,GPIO_PIN_14,GPIO_PIN_15},
                                };
static m_spi g_my_spi[2];
                                
static int m_init  (u8 spi)
{
    GPIO_InitTypeDef gpio = {0};
    int ret = 0;

	if(g_my_spi[spi].m_init_state != false)  return E_OK;
//    __HAL_RCC_SPI1_CLK_ENABLE();  //开启SPI 时钟后会影响 TIME3 后TIME4的使用
    __HAL_RCC_SPI2_CLK_ENABLE();
    
    /*初始化IO*/
    gpio.Pin   = g_cfg[spi].m_pin_sck|g_cfg[spi].m_pin_miso|g_cfg[spi].m_pin_mosi;
    gpio.Mode  = GPIO_MODE_AF_PP;             //复用推挽输出
    gpio.Pull  = GPIO_PULLUP    ;             //上拉
    gpio.Speed = GPIO_SPEED_HIGH;             //快速            
    HAL_GPIO_Init(g_cfg[spi].m_spi_gpio,&gpio);

    /*初始化spi*/
    g_my_spi[spi].m_spi_handle.Instance              = g_cfg[spi].m_spi_addr      ; //spi寄存器基地址
    g_my_spi[spi].m_spi_handle.Init.Mode             = SPI_MODE_MASTER            ; //设置SPI工作模式，设置为主模式
    g_my_spi[spi].m_spi_handle.Init.Direction        = SPI_DIRECTION_2LINES       ; //设置SPI单向或者双向的数据模式:SPI设置为双线模式
    g_my_spi[spi].m_spi_handle.Init.DataSize         = SPI_DATASIZE_8BIT          ; //设置SPI的数据大小:SPI发送接收8位帧结构
    g_my_spi[spi].m_spi_handle.Init.CLKPolarity      = SPI_POLARITY_HIGH          ; //串行同步时钟的空闲状态为高电平
    g_my_spi[spi].m_spi_handle.Init.CLKPhase         = SPI_PHASE_2EDGE            ; //串行同步时钟的第二个跳变沿（上升或下降）数据被采样
    g_my_spi[spi].m_spi_handle.Init.NSS              = SPI_NSS_SOFT               ; //NSS信号由硬件（NSS管脚）还是软件（使用SSI位）管理:内部NSS信号有SSI位控制
    g_my_spi[spi].m_spi_handle.Init.BaudRatePrescaler= SPI_BAUDRATEPRESCALER_64    ; //定义波特率预分频的值:波特率预分频值为2
    g_my_spi[spi].m_spi_handle.Init.FirstBit         = SPI_FIRSTBIT_MSB           ; //指定数据传输从MSB位还是LSB位开始:数据传输从MSB位开始
    g_my_spi[spi].m_spi_handle.Init.TIMode           = SPI_TIMODE_DISABLE         ; //关闭TI模式
    g_my_spi[spi].m_spi_handle.Init.CRCCalculation   = SPI_CRCCALCULATION_DISABLE ; //关闭硬件CRC校验
    g_my_spi[spi].m_spi_handle.Init.CRCPolynomial    = 7                          ; //CRC值计算的多项式
    HAL_SPI_Init(&g_my_spi[spi].m_spi_handle);//初始化
    
    __HAL_SPI_ENABLE(&g_my_spi[spi].m_spi_handle);                    //使能SPI
    
    /*配置发送DMA*/
    __HAL_LINKDMA(&g_my_spi[spi].m_spi_handle,hdmatx,g_my_spi[spi].m_tx_dma_handle);
    g_my_spi[spi].m_tx_dma_handle.Instance                 = g_cfg[spi].m_send_dma_channel ;   //通道选择
    g_my_spi[spi].m_tx_dma_handle.Init.Direction           = DMA_MEMORY_TO_PERIPH          ;   //存储器到外设
    g_my_spi[spi].m_tx_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE              ;   //外设非增量模式
    g_my_spi[spi].m_tx_dma_handle.Init.MemInc              = DMA_MINC_ENABLE               ;   //存储器增量模式
    g_my_spi[spi].m_tx_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE           ;   //外设数据长度:8位
    g_my_spi[spi].m_tx_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE           ;   //存储器数据长度:8位
    g_my_spi[spi].m_tx_dma_handle.Init.Mode                = DMA_NORMAL                    ;   //外设普通模式
    g_my_spi[spi].m_tx_dma_handle.Init.Priority            = DMA_PRIORITY_MEDIUM           ;   //中等优先级
    HAL_DMA_DeInit(&g_my_spi[spi].m_tx_dma_handle);   
    HAL_DMA_Init  (&g_my_spi[spi].m_tx_dma_handle);  /*初始化DMA*/
    
    /*注册DMA发送回调函数*/
    ret = int_manage.login_dma(g_cfg[spi].m_send_dma_cha_int,m_send_int_handle,&g_my_spi[spi]);
	if(E_OK != ret)
	{
		log_error("Int func login failed.");
		return E_ERROR;
	}    
    //HAL_NVIC_SetPriority(g_cfg[spi].m_send_dma_irqn, 9, 0);
    //HAL_NVIC_EnableIRQ(g_cfg[spi].m_send_dma_irqn);  
    
    /*配置接收DMA*/
    __HAL_LINKDMA(&g_my_spi[spi].m_spi_handle,hdmarx,g_my_spi[spi].m_rx_dma_handle);
    g_my_spi[spi].m_rx_dma_handle.Instance                 = g_cfg[spi].m_recv_dma_channel ;   //通道选择
    g_my_spi[spi].m_rx_dma_handle.Init.Direction           = DMA_PERIPH_TO_MEMORY          ;   //外设到存储器
    g_my_spi[spi].m_rx_dma_handle.Init.PeriphInc           = DMA_PINC_DISABLE              ;   //外设非增量模式
    g_my_spi[spi].m_rx_dma_handle.Init.MemInc              = DMA_MINC_ENABLE               ;   //存储器增量模式
    g_my_spi[spi].m_rx_dma_handle.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE           ;   //外设数据长度:8位
    g_my_spi[spi].m_rx_dma_handle.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE           ;   //存储器数据长度:8位
    g_my_spi[spi].m_rx_dma_handle.Init.Mode                = DMA_NORMAL                    ;   //外设普通模式
    g_my_spi[spi].m_rx_dma_handle.Init.Priority            = DMA_PRIORITY_MEDIUM           ;   //中等优先级
    HAL_DMA_DeInit(&g_my_spi[spi].m_rx_dma_handle);   
    HAL_DMA_Init  (&g_my_spi[spi].m_rx_dma_handle);  /*初始化DMA*/
    //HAL_NVIC_SetPriority(g_cfg[spi].m_recv_dma_irqn, 8, 0);
    //HAL_NVIC_EnableIRQ(g_cfg[spi].m_recv_dma_irqn);  
   	
	    /*注册DMA接收回调函数*/
    ret = int_manage.login_dma(g_cfg[spi].m_recv_dma_cha_int,m_recv_int_handle,&g_my_spi[spi]);
	if(E_OK != ret)
	{
		log_error("Int func login failed.");
		return E_ERROR;
	}   
	
	
    /*创建二值信号量*/
    g_my_spi[spi].m_semaphore = xSemaphoreCreateBinary();
    if(NULL == g_my_spi[spi].m_semaphore)
    {
        log_error("Mutex semaphore creat failed.");
        return E_ERROR;
    }

	g_my_spi[spi].m_init_state = true;
    return E_OK;
}

static int m_transmission(u8 spi,const u8* send,u8* recv,u16 len,bool incremental,TickType_t time)
{
    HAL_StatusTypeDef hal_ret = 0;
    BaseType_t os_ret = 0;

    /*参数检查*/
    if(MY_SPI_1 != spi && MY_SPI_2 != spi)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    if(0 == len || (NULL == send && NULL == recv))
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*确认spi状态*/
    if(g_my_spi[spi].m_spi_handle.State != HAL_SPI_STATE_READY)
    {
        log_error("Spi is busy.");
        return E_ERROR;
    }

    /*确认DMA状态*/
    if(g_my_spi[spi].m_tx_dma_handle.State != HAL_DMA_STATE_READY ||
       g_my_spi[spi].m_rx_dma_handle.State != HAL_DMA_STATE_READY)
    {
        log_error("Spi dma is busy.");
        return E_ERROR;
    }

    /*必须先关闭DMA 才可以修改DMA配置 和SPI一样*/
    __HAL_DMA_DISABLE(&g_my_spi[spi].m_tx_dma_handle);
		HAL_SPI_DMAStop(&g_my_spi[spi].m_spi_handle);
    
    /*确认增量模式*/
    if(incremental)
    {
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_CCR_MINC_Msk;  /*存储器增量模式*/
    }
    else
    {
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_MINC_Msk;  /*不执行存储器增量模式*/
    }
    
    __HAL_DMA_ENABLE(&g_my_spi[spi].m_tx_dma_handle);

    /*请求一次信号量 保证开始的时候是空的*/
    xSemaphoreTake(g_my_spi[spi].m_semaphore,0);

    /*根据参数类型 启动相应的spi收发模式*/
    if(NULL != send && NULL == recv)
    {
        /*单发送*/
        hal_ret = HAL_SPI_Transmit_DMA(&g_my_spi[spi].m_spi_handle,(void*)send,len);
    }
    else if(NULL == send && NULL != recv)
    {
        /*单接收*/
        hal_ret = HAL_SPI_Receive_DMA(&g_my_spi[spi].m_spi_handle,recv,len);
    }
    else
    {
        /*收发*/
        hal_ret = HAL_SPI_TransmitReceive_DMA(&g_my_spi[spi].m_spi_handle,(void*)send,recv,len);
    }
    if(HAL_OK != hal_ret)
    {
        log_error("Spi dma start failed.");
        return E_ERROR;
    }

    /*等待传输完成*/
    os_ret = xSemaphoreTake(g_my_spi[spi].m_semaphore,time);
    if(pdTRUE != os_ret)
    {
        log_error("Get semaphore timout.");
        return E_ERROR;
    }
	HAL_SPI_DMAStop(&g_my_spi[spi].m_spi_handle);	
    return E_OK;
}

static int m_set_datasize(u8 spi,u8 datasize)
{
    /*参数检查*/
    if(MY_SPI_1 != spi && MY_SPI_2 != spi)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    if(8 != datasize && 16 != datasize)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*确认spi状态*/
    if(g_my_spi[spi].m_spi_handle.State != HAL_SPI_STATE_READY)
    {
        log_error("Spi is busy.");
        return E_ERROR;
    }

    /*确认DMA状态*/
    if(g_my_spi[spi].m_tx_dma_handle.State != HAL_DMA_STATE_READY ||
       g_my_spi[spi].m_rx_dma_handle.State != HAL_DMA_STATE_READY)
    {
        log_error("Spi dma is busy.");
        return E_ERROR;
    }

    __HAL_SPI_DISABLE(&g_my_spi[spi].m_spi_handle);
    __HAL_DMA_DISABLE(&g_my_spi[spi].m_tx_dma_handle);
    __HAL_DMA_DISABLE(&g_my_spi[spi].m_rx_dma_handle);

    if(8 == datasize)
    {
        g_my_spi[spi].m_spi_handle.Init.DataSize = SPI_DATASIZE_8BIT; 
        
        /*DFF 清0 位8位宽*/
        g_cfg[spi].m_spi_addr->CR1 &= ~SPI_CR1_DFF;

        /*发送 修改为8位宽*/
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_PSIZE_Msk;
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_PDATAALIGN_BYTE;
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_MSIZE_Msk;
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_MDATAALIGN_BYTE;

        /*接收 修改位8位宽*/
        g_cfg[spi].m_recv_dma_channel->CCR &= ~DMA_CCR_PSIZE_Msk;
        g_cfg[spi].m_recv_dma_channel->CCR |= DMA_PDATAALIGN_BYTE;
        g_cfg[spi].m_recv_dma_channel->CCR &= ~DMA_CCR_MSIZE_Msk;
        g_cfg[spi].m_recv_dma_channel->CCR |= DMA_MDATAALIGN_BYTE;
    }
    else
    {
        g_my_spi[spi].m_spi_handle.Init.DataSize = SPI_DATASIZE_16BIT;
        
        /*DFF 置1 位16位宽*/
        g_cfg[spi].m_spi_addr->CR1 |= SPI_CR1_DFF;

        /*发送 修改位16位宽*/
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_PSIZE_Msk;
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_PDATAALIGN_HALFWORD;
        g_cfg[spi].m_send_dma_channel->CCR &= ~DMA_CCR_MSIZE_Msk;
        g_cfg[spi].m_send_dma_channel->CCR |= DMA_MDATAALIGN_HALFWORD;

        /*接收 修改位16位宽*/
        g_cfg[spi].m_recv_dma_channel->CCR &= ~DMA_CCR_PSIZE_Msk;
        g_cfg[spi].m_recv_dma_channel->CCR |= DMA_PDATAALIGN_HALFWORD;
        g_cfg[spi].m_recv_dma_channel->CCR &= ~DMA_CCR_MSIZE_Msk;
        g_cfg[spi].m_recv_dma_channel->CCR |= DMA_MDATAALIGN_HALFWORD;
    }

    __HAL_SPI_ENABLE(&g_my_spi[spi].m_spi_handle);
    __HAL_DMA_ENABLE(&g_my_spi[spi].m_tx_dma_handle);
    __HAL_DMA_ENABLE(&g_my_spi[spi].m_rx_dma_handle);

    return E_OK;
}

static int m_enable(u8 spi,bool en)
{
		/*参数检查*/
    if(MY_SPI_1 != spi && MY_SPI_2 != spi)
    {
        log_error("Param error.");
        return E_PARAM;
    }
		
		if(en)
		{
			  __HAL_SPI_ENABLE(&g_my_spi[spi].m_spi_handle);
		}
		else
		{
				__HAL_SPI_DISABLE(&g_my_spi[spi].m_spi_handle);
		}
		
		return E_OK;
}

static int m_set_speed(u8 spi,u8 speed)
{
			/*参数检查*/
    if(MY_SPI_1 != spi && MY_SPI_2 != spi)
    {
        log_error("Param error.");
        return E_PARAM;
    }
		
		__HAL_SPI_DISABLE(&g_my_spi[spi].m_spi_handle);            //关闭SPI
    g_my_spi[spi].m_spi_handle.Instance->CR1&=0XFFC7;          //位3-5清零，用来设置波特率
    g_my_spi[spi].m_spi_handle.Instance->CR1|=speed;//设置SPI速度
    __HAL_SPI_ENABLE(&g_my_spi[spi].m_spi_handle);             //使能SPI
		
	return E_OK;
}

static int m_send_int_handle(u32 dma_cha,void* param)
{
    m_spi* m_this = NULL;
    BaseType_t ret = 0;
    
    /*检查参数*/
    if(NULL == param)
    {
        log_error("NULL pointer.");
        return E_NULL;
    }
	m_this = param;
    
    /*如果触发接收完成DMA中断*/
    if(__HAL_DMA_GET_FLAG(&m_this->m_tx_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&m_this->m_tx_dma_handle)) != RESET)
    {
        /*这里发送完成不能马上关DMA有可能发接收还没完*/
//        HAL_SPI_DMAStop(&m_this->m_spi_handle);  /*停止SPI*/

        /*发送信号量 */
        xSemaphoreGiveFromISR(m_this->m_semaphore,&ret);
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
    m_spi* m_this = NULL;
    BaseType_t ret = 0;
    
    /*检查参数*/
    if(NULL == param)
    {
        log_error("NULL pointer.");
        return E_NULL;
    }
		m_this = param;
    
    /*如果触发接收完成DMA中断*/
    if(__HAL_DMA_GET_FLAG(&m_this->m_rx_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&m_this->m_rx_dma_handle)) != RESET)
    {
        /*这里接收完成不能马上关DMA有可能发送还没完*/
//        HAL_SPI_DMAStop(&m_this->m_spi_handle);  /*停止SPI*/

        /*发送信号量 */
        xSemaphoreGiveFromISR(m_this->m_semaphore,&ret);
    }
    
    HAL_DMA_IRQHandler(&m_this->m_rx_dma_handle);
    
    /*手动进行一次任务切换*/
	if(pdTRUE == ret)
	{
		taskYIELD();
	}
    
    return E_OK;
}

///*spi1 接收DMA中断*/
//void DMA1_Channel2_IRQHandler(void)
//{
//    BaseType_t ret = 0;
//    
//    /*如果触发接收完成DMA中断*/
//    if(__HAL_DMA_GET_FLAG(&g_my_spi[MY_SPI_1].m_rx_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&g_my_spi[MY_SPI_1].m_rx_dma_handle)) != RESET)
//    {
//        HAL_SPI_DMAStop(&g_my_spi[MY_SPI_1].m_spi_handle);  /*停止SPI*/

//        /*发送信号量 */
//        xSemaphoreGiveFromISR(g_my_spi[MY_SPI_1].m_semaphore,&ret);
//    }
//    
//    HAL_DMA_IRQHandler(&g_my_spi[MY_SPI_1].m_rx_dma_handle);
//    
//    /*手动进行一次任务切换*/
//	if(pdTRUE == ret)
//	{
//		taskYIELD();
//	}
//}

///*spi1 发送DMA中断*/
//void DMA1_Channel3_IRQHandler(void)
//{
//    BaseType_t ret = 0;
//    
//    /*如果触发发送完成DMA中断*/
//    if(__HAL_DMA_GET_FLAG(&g_my_spi[MY_SPI_1].m_tx_dma_handle, __HAL_DMA_GET_TC_FLAG_INDEX(&g_my_spi[MY_SPI_1].m_tx_dma_handle)) != RESET)
//    {
//        HAL_SPI_DMAStop(&g_my_spi[MY_SPI_1].m_spi_handle);  /*停止SPI*/ 

//        /*发送信号量 */
//        xSemaphoreGiveFromISR(g_my_spi[MY_SPI_1].m_semaphore,&ret);
//    }
//    
//    HAL_DMA_IRQHandler(&g_my_spi[MY_SPI_1].m_tx_dma_handle);
//    
//    /*手动进行一次任务切换*/
//	if(pdTRUE == ret)
//	{
//		taskYIELD();
//	}

//}

#endif 
