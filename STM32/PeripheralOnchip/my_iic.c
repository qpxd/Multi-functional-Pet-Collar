/*****************************************************************************
* Copyright:
* File name: my_iic.c
* Description: my_iic函数实现
* Author: 许少
* Version: V1.5
* Date: 2022/02/03
* log:  V1.0  2021/12/29
*       发布：
*
*       V1.1  2022/01/30
*       修复：m_begin中m_recv接收参数写死为2的bug。
*       
*       v1.2  2022/01/31
*       优化：m_send函数中新增ack的等待时间。在使用sgp30的过程中发现，sgp30长
*             时间没上电后初次上电，ack回应非常慢，易导致通讯失败，造成器件不
*             在线上的误解。
*
*       v1.3  2022/02/02
*       增加：clear接口，用于停止正在工作的器件。在调试bmp180时，发现bmp180的
*             初始化会影响到OLED初始化（发送地址无响应）。初探发现两个器件的地
*             址之间并没有冲突，所以偏向硬件问题。跟换新的bmp180、跟换bmp180电
*             源电压、去除bmp180的上拉电阻，现象均没有消失，看现象和硬件关联不
*             大。联想到mlx90614，看是否有相同现象。结果启动mlx90614，并将采集
*             值显示到oled上时，出现相同现象，mlx90614间歇性的地址无响应，若不
*             显显示在oled上，获取正常。继续联想1750、tcs34725,发现二者相对温度
*             问题更多的跟这器件类型走，并不是普视。在组合测试过程中，发现先启
*             用1750再启用mlx90614跟容易出现无响应，而先启用tcs34725，或先启用
*             1750再启用tcs34725后启用mlx90614相对稳定。目前情况而言，跟多的偏
*             向软件问题。倒退会最初现象，在不改动硬件的前提下，剔出bmp180的初
*             始化，结果现象消失，更多的印证软件对错误的影响。猜想为特殊的数据
*             字导致了本该静默器件的启动，增加该接口，可在发起iic通讯前，手动
*             停止一次器件的工作。使用后，问题消失。但注意：本问题的修复基于猜
*             想，因细排下去的工作量比较大，暂时用本方式来做黑盒解决。
*
*       v1.4  2022/02/02
*       修复：m_recv接口，接收最后一个字节时发送NACK的条件错误，导致NACK发不出
*             去，时序缺失。倒是bmp180在读了温度后，总是读不了气压，读了气压又
*             读不了温度。原因就在没错读取最后没有发送NACK，导致后续读取失效。
*
*       v1.5  2022/02/03
*       增加：iic启动接口增加速度参数。对于一些低速的iic器件，如：AM2320，只支持
*             到100Kb/s的iic速度，需要降速。但对于一些高速的iic器件，如OLED，过
*             慢的iic速度会导致屏幕刷新迟钝。因此，在对外的begin接口新增调速参数
*****************************************************************************/

#include "onchip_conf.h"

#ifdef ONCHIP_PWM_ENABLED

#define MODULE_NAME       "my_iic"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define CMD_TYPE_NONE         0
#define CMD_TYPE_START        1  /*起始信号    */
#define CMD_TYPE_STOP         2  /*停止信号    */
#define CMD_TYPE_SEND_BYTES   3  /*发送N个字节 */
#define CMD_TYPE_RECV_BYTES   4  /*接收N个字节 */

#define IIC_ACK               false  /*有ack回应*/
#define IIC_NACK              true   /*无ack回应*/

/*iic 命令节点*/
typedef struct __IIC_CMD_NODE
{
    my_list   m_list        ; /*链表节点     */
    u8        m_cmd_type    ; /*命令类型     */
    u8*       m_recv_addr   ; /*收数据地址   */
    const u8* m_send_addr   ; /*发数据地址   */
    u16       m_data_len    ; /*数据长度     */
}iic_cmd_node;

/*iic 对象 私有成员*/
typedef struct _M_MY_IIC
{
    GPIO_TypeDef*      m_sda_gpio     ;  /*sda gpio   */
    uint32_t           m_sda_pin      ;  /*sda  pin   */
    GPIO_TypeDef*      m_scl_gpio     ;  /*scl gpio   */
    uint32_t           m_scl_pin      ;  /*scl  pin   */
    SemaphoreHandle_t  m_semaphore    ;  /*互斥信号量 */
}m_my_iic;


static int  m_start    (const c_my_iic* this,u8 speed);
static int  m_stop     (const c_my_iic* this,u8 speed);
static int  m_send     (const c_my_iic* this,const u8* data,u16 len,bool ack_check,u16 ack_wait_time,u8 speed);
static int  m_recv     (const c_my_iic* this,u8* data,u16 len,u8 speed);
static int  m_begin    (const c_my_iic* this,iic_cmd_handle_t cmd_object,u8 speed,TickType_t time);
static void m_send_bit (const c_my_iic* this,u8 bit,u8 speed);
static u8   m_recv_bit (const c_my_iic* this,u8 speed);
static int  m_clear    (const c_my_iic* this,u8 addr,u16 ack_wait_time,u8 speed);


c_my_iic iic_creat(GPIO_TypeDef* sda_gpio,uint32_t sda_pin,GPIO_TypeDef* scl_gpio,uint32_t scl_pin)
{
    c_my_iic        new           = {0}  ;
    GPIO_InitTypeDef GPIO_Initure = {0}  ;  
    m_my_iic*       m_this        = NULL ;

    /*初始化SDA*/
    GPIO_Initure.Pin   = sda_pin               ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP   ;  //推挽输出/*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_my_iic));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_my_iic));
    m_this = new.this;

    /*为新对象 创建互斥信号量*/
    m_this->m_semaphore = xSemaphoreCreateMutex();
    if(NULL == m_this->m_semaphore)
    {
        log_error("Mutex semaphore creat failed.");
        vPortFree(new.this);
        new.this = NULL;
        return new;
    }
    GPIO_Initure.Pull  = GPIO_PULLUP           ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH  ;  //高速
    HAL_GPIO_Init(sda_gpio,&GPIO_Initure)      ;  
    HAL_GPIO_WritePin(sda_gpio,sda_pin,GPIO_PIN_SET);  /*默认输出高电平*/

    /*初始化SCL*/
    GPIO_Initure.Pin   = scl_pin               ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP   ;  //推挽输出
    GPIO_Initure.Pull  = GPIO_PULLUP           ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH  ;  //高速
    HAL_GPIO_Init(scl_gpio,&GPIO_Initure)      ;  
    HAL_GPIO_WritePin(scl_gpio,scl_pin,GPIO_PIN_SET);  /*默认输出高电平*/

    /*保存相关变量*/
    m_this->m_sda_gpio = sda_gpio;
    m_this->m_sda_pin  = sda_pin ;
    m_this->m_scl_gpio = scl_gpio;
    m_this->m_scl_pin  = scl_pin ;

    /*保存函数指针*/
    new.begin = m_begin;
    new.clear = m_clear;

    /*返回新建的对象*/
    return new; 
}

iic_cmd_handle_t iic_cmd_link_create(void)
{
    iic_cmd_handle_t new_handle = NULL;

    /*创建 命令连接 首节点*/
    new_handle = pvPortMalloc(sizeof(my_list));
    if(NULL == new_handle)
    {
        log_error("Cmd link create failed.");       
    }

    /*初始化首节点*/
    dl_list_init(new_handle);

    return new_handle;
}

int iic_cmd_link_delete(iic_cmd_handle_t object)
{
    iic_cmd_node* cur = NULL;
    iic_cmd_node* next = NULL;  

    if(NULL == object)
    {
        log_error("Null pointer.");
        return E_PARAM;
    }

    /**/
    dl_list_for_each_safe(cur,next,(my_list*)object,iic_cmd_node,m_list)
    {
        
        dl_list_del(&cur->m_list); /*删除当前节点*/
        vPortFree(cur);            /*释放当前节点*/
        cur = NULL;                /*防野指针*/
    }

    /*删除首节点*/
    vPortFree(object);

    return E_OK;
}
int iic_master_start(iic_cmd_handle_t object)
{
    iic_cmd_node* new_cmd = NULL;

    if(NULL == object)
    {
        log_error("Null pointer.");
        return E_PARAM;
    }

    /*创建一个新的节点*/
    new_cmd = pvPortMalloc(sizeof(iic_cmd_node));
    if(NULL == new_cmd)
    {
        log_error("Memory is full.");
        return E_ERROR;
    }
    memset(new_cmd,0,sizeof(iic_cmd_node));

    /*新节点设置为启动信号*/
    new_cmd->m_cmd_type = CMD_TYPE_START;

    /*新节点添加到链表末尾*/
    dl_list_add_tail((my_list*)object,&new_cmd->m_list);
    

    return E_OK;
}

int iic_master_stop(iic_cmd_handle_t object)
{
    iic_cmd_node* new_cmd = NULL;

    if(NULL == object)
    {
        log_error("Null pointer.");
        return E_PARAM;
    }

    /*创建一个新的节点*/
    new_cmd = pvPortMalloc(sizeof(iic_cmd_node));
    if(NULL == new_cmd)
    {
        log_error("Memory is full.");
        return E_ERROR;
    }
    memset(new_cmd,0,sizeof(iic_cmd_node));

    /*新节点设置为停止信号*/
    new_cmd->m_cmd_type = CMD_TYPE_STOP;

    /*新节点添加到链表末尾*/
    dl_list_add_tail((my_list*)object,&new_cmd->m_list);
    

    return E_OK;

}

int iic_write_bytes(iic_cmd_handle_t object,const u8* data,u16 data_len)
{
    iic_cmd_node* new_cmd = NULL;

    if(NULL == object || NULL == data)
    {
        log_error("Null pointer.");
        return E_PARAM;
    }
    if(0 == data_len)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*创建一个新的节点*/
    new_cmd = pvPortMalloc(sizeof(iic_cmd_node));
    if(NULL == new_cmd)
    {
        log_error("Memory is full.");
        return E_ERROR;
    }
    memset(new_cmd,0,sizeof(iic_cmd_node));
    

    /*新节点设置为写多字节信号*/
    new_cmd->m_cmd_type  = CMD_TYPE_SEND_BYTES;
    new_cmd->m_send_addr = data;
    new_cmd->m_data_len  = data_len;

    /*新节点添加到链表末尾*/
    dl_list_add_tail((my_list*)object,&new_cmd->m_list);
    
    return E_OK;
}

int iic_read_bytes(iic_cmd_handle_t object,u8* data,u16 data_len)
{
    iic_cmd_node* new_cmd = NULL;

    if(NULL == object || NULL == data)
    {
        log_error("Null pointer.");
        return E_PARAM;
    }
    if(0 == data_len)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*创建一个新的节点*/
    new_cmd = pvPortMalloc(sizeof(iic_cmd_node));
    if(NULL == new_cmd)
    {
        log_error("Memory is full.");
        return E_ERROR;
    }
    memset(new_cmd,0,sizeof(iic_cmd_node));
    

    /*新节点设置为读多字节信号*/
    new_cmd->m_cmd_type  = CMD_TYPE_RECV_BYTES;
    new_cmd->m_recv_addr = data;
    new_cmd->m_data_len  = data_len;

    /*新节点添加到链表末尾*/
    dl_list_add_tail((my_list*)object,&new_cmd->m_list);
    
    return E_OK;
}

static int m_start(const c_my_iic* this,u8 speed)
{
    const m_my_iic* m_this = NULL;
    
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*SDA设置为输出模式*/
    be_out(m_this->m_sda_gpio,m_this->m_sda_pin);
    delay_us(speed);
    
    GPIO_SET(m_this->m_sda_gpio,m_this->m_sda_pin,GPIO_PIN_SET);  /*SDA = 1*/
    delay_us(speed);
    GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_SET);  /*SCL = 1*/
    delay_us(speed);

    /*当SCL=1时，SDA由1到0表示通信开始*/
    GPIO_SET(m_this->m_sda_gpio,m_this->m_sda_pin,GPIO_PIN_RESET);
    delay_us(speed);

    /*钳住时钟线 准备发送或接收数据*/
    GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_RESET);
    delay_us(speed);
    
    return E_OK;
}

static int m_stop (const c_my_iic* this,u8 speed)
{
    const m_my_iic* m_this = NULL;
    
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*SDA设置为输出模式*/
    be_out(m_this->m_sda_gpio,m_this->m_sda_pin);

    GPIO_SET(m_this->m_sda_gpio,m_this->m_sda_pin,GPIO_PIN_RESET);  /*SDA = 0*/
    delay_us(speed);
    GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_RESET);  /*SCL = 0*/
    delay_us(speed);
    GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_SET);    /*SCL = 1*/
    delay_us(speed);
    
    /*当SCL=1时，SDA由0到1表示通信结束*/
    GPIO_SET(m_this->m_sda_gpio,m_this->m_sda_pin,GPIO_PIN_RESET);
    delay_us(speed);

    return E_OK;
}
static int m_send(const c_my_iic* this,const u8* data,u16 len,bool ack_check,u16 ack_wait_time,u8 speed)
{
    u16 i = 0;
    u8 bit_count = 0;
    u8 data_cpy = 0;
    u16 ack_count = 0;
    bool ack = 0;
    const m_my_iic* m_this = NULL;

    if(NULL == this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }   
    if(0 == len || IIC_MAX_DATA_LEN < len)
    {
        log_error("Param error");
        return E_ERROR;
    }
    m_this = this->this;
    
    for(i = 0;i < len;++i)
    {
        be_out(m_this->m_sda_gpio,m_this->m_sda_pin);  /*SDA 输出模式*/
        data_cpy = data[i]; /*数据拷贝出来*/
        for(bit_count = 0;bit_count < 8;++bit_count)
        {
            if(data_cpy & 0x80)
            {
                m_send_bit(this,1,speed);
            }
            else
            {
                m_send_bit(this,0,speed);
            }
            data_cpy <<= 1;
        }

        /*准备接收ack*/
        be_in(m_this->m_sda_gpio,m_this->m_sda_pin);  /*SDA 输入模式*/
        
        GPIO_SET(m_this->m_sda_gpio,m_this->m_sda_pin,GPIO_PIN_SET);   /*SDA = 1*/
        delay_us(speed);
        GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_SET);   /*SCL = 1*/
        delay_us(speed);  
        do
        {
            ack = GPIO_GET(m_this->m_sda_gpio,m_this->m_sda_pin);
            ++ack_count;
            delay_us(1);
        }while(IIC_NACK == ack && ack_count < ack_wait_time);
        GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_RESET);/*SCL = 0*/
        delay_us(speed);
        
        //ack = m_recv_bit(this);  /*接收ack*/
        /*如果需要校验ack 且ack无效*/
        if(ack_check && IIC_NACK == ack)
        {
            log_error("IIC slave NACK,send:%#X",data[i]);
            return E_ERROR;
        }
        
    }

    return E_OK;
}
    
static int m_recv (const c_my_iic* this,u8* data,u16 len,u8 speed)
{
    u16 i = 0;
    u8 bit_count = 0;
    u8 data_cpy = 0;
    const m_my_iic* m_this = NULL;

    if(NULL == this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }   
    if(0 == len || IIC_MAX_DATA_LEN < len)
    {
        log_error("Param error");
        return E_ERROR;
    }
    m_this = this->this;
    
    for(i = 0;i < len;++i)
    {
        be_in(m_this->m_sda_gpio,m_this->m_sda_pin);  /*SDA 输入模式*/
        data_cpy = 0; /*清一下数据缓存*/
        for(bit_count = 0;bit_count < 8;++bit_count)
        {
            data_cpy <<= 1;
            /*接收一个bit*/
            if(m_recv_bit(this,speed))
            {
                data_cpy  |= 0x01;
            }
            else
            {
                data_cpy  &= 0xFE;
            }
            
        }
        data[i] = data_cpy;  /*保存数据到指定位置*/

        /*准备发送ack*/
        be_out(m_this->m_sda_gpio,m_this->m_sda_pin);  /*SDA 输出模式*/

        /*如果收到最后一个字节 就发送NACK 停止接收 否则发送ACK 继续接收*/
        if(len - 1 == i)              /*错误原  if(len == i)  导致最后一个字节NACK一直发不出来*/
        {
            m_send_bit(this,IIC_NACK,speed);
        }
        else
        {
            m_send_bit(this,IIC_ACK,speed);
        }       
    }

    return E_OK;
}

static int m_begin (const c_my_iic* this,iic_cmd_handle_t cmd_object,u8 speed,TickType_t time)
{
    const m_my_iic*      item      = NULL       ;
    BaseType_t     os_ret    = 0          ;
    int            ret       = 0          ;
    iic_cmd_node*  cmd_node  = NULL       ;
    my_list*       list      = cmd_object ;

    if(NULL == this || NULL == cmd_object)
    {
        log_error("Null pointer.");
        return E_NULL;
    }   

    /*请求互斥信号量*/
    item = this->this;
    os_ret = xSemaphoreTake(item->m_semaphore,time);
    if(pdTRUE != os_ret)
    {
        log_error("Get semaphore timout.");
        return E_ERROR;
    }

    /*发送IIC的过程中，如果被打断，会导致IIC发送的时序不稳定
    所以此处要进入临界态*/
    taskENTER_CRITICAL();   //进入临界区

    /*遍历命令链表*/
    dl_list_for_each(cmd_node,list,iic_cmd_node,m_list)
    {
        switch (cmd_node->m_cmd_type)
        {
            /*起始信号*/
            case CMD_TYPE_START :
            {
                m_start(this,speed);
                break;
            }
            /*停止信号*/
            case CMD_TYPE_STOP :
            {
                m_stop(this,speed);
                break;
            }
            /*发送N个字节 */
            case CMD_TYPE_SEND_BYTES :
            {
                ret = m_send(this,cmd_node->m_send_addr,cmd_node->m_data_len,true,I2C_ACK_WAIT_DEFAULT_TIME,speed);
                if(E_OK != ret)
                {
                    log_error("IIC send bytes failed.");
                    goto error_handle;
                }
                break;
            }
            /*接收N个字节 */
            case CMD_TYPE_RECV_BYTES :
            {
                ret = m_recv(this,cmd_node->m_recv_addr,cmd_node->m_data_len,speed);  /*20220130bug 原：ret = m_recv(this,cmd_node->m_recv_addr,2);*/
                if(E_OK != ret)                                                       /*gy_sgp 接收数据时 总是只收到两位数据 而且只有第一次成功，后面总是两次失败成功一次 失败时总是没有ACK*/
                {                                                                     /*单独取消读取后 现象消失，不会有NACK的情况，单独读取 问题任然存在。断定写入无问题 问题一定出在读取上 */
                    log_error("IIC recv bytes failed.");                              /*反复调整写入和读取顺序，问题都得不到解决 说明和顺序无关 所以恢复到文档要求的写入时序*/
                    goto error_handle;                                                /*调整iic的时间间隔 和等待ACK的时长 也得不到解决。回顾本模块使用的iic器件之多，问题应该不出在iic时序上*/
                }                                                                     /*回到读取六个字节只有前两个有数据的源头，结合读问题的定位，决定走读取有效性下手*/
                break;                                                                /*在设置读取六字节处下断点，再进入到iic m_recv中准备跟踪接收情况。最终发现m_recv投递参数中，数据长度值为2*/
            }                                                                         /*倒退到产生写入 传递 ，最终定位本行问题。*/
            default:                                                                  /*回顾之前问题没有触发原因在于 1750、tcs34725均为2字节读取 而oled又不需要读取 所以问题一直被隐藏*/
            {
                log_error("Error iic cmd type.");
                goto error_handle;
            }
        }
    }
    
    /*释放信号量*/
    os_ret = xSemaphoreGive(item->m_semaphore);
    if(pdTRUE != os_ret)
    {
        log_error("Give semaphore timout.");
        return E_ERROR;
    }
    
    taskEXIT_CRITICAL();            //退出临界区   
    return E_OK;

    error_handle:
    
    os_ret = xSemaphoreGive(item->m_semaphore);
    if(pdTRUE != os_ret)
    {
        log_error("Give semaphore timout.");
    }
    m_stop(this,speed);   /*异常退出 停止一次iic 保证总线处于空闲状态*/
    taskEXIT_CRITICAL();  /*退出临界态*/
    
    return E_ERROR;
}

static void m_send_bit(const c_my_iic* this,u8 bit,u8 speed)
{
    const m_my_iic* m_this = this->this;
      
    if(0 == bit)
    {
        GPIO_SET(m_this->m_sda_gpio,m_this->m_sda_pin,GPIO_PIN_RESET);  /*SDA = 0*/
    }
    else
    {
        GPIO_SET(m_this->m_sda_gpio,m_this->m_sda_pin,GPIO_PIN_SET);    /*SDA = 1*/
    }
    delay_us(speed);
    GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_SET);  /*SCL = 1*/
    delay_us(speed);
    GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_RESET);/*SCL = 0*/
    delay_us(speed);

    return;
}

static u8   m_recv_bit(const c_my_iic* this,u8 speed)
{
    u8 bit = 0;
    const m_my_iic* m_this = this->this;

    GPIO_SET(m_this->m_sda_gpio,m_this->m_sda_pin,GPIO_PIN_SET);   /*SDA = 1*/
    delay_us(speed);
    GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_SET);   /*SCL = 1*/
    delay_us(speed);
    
    if(GPIO_PIN_SET == GPIO_GET(m_this->m_sda_gpio,m_this->m_sda_pin))
    {
        bit = 0x01;
    }
    else
    {
        bit = 0x00;
    }

    GPIO_SET(m_this->m_scl_gpio,m_this->m_scl_pin,GPIO_PIN_RESET);/*SCL = 0*/
    delay_us(speed);

    return bit;
}

static int m_clear(const c_my_iic* this,u8 addr,u16 ack_wait_time,u8 speed)
{
    int ret = 0;

    ret = m_start(this,speed);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        return ret;
    }
    
    ret = m_send(this,&addr,1,false,ack_wait_time,speed);
    if(E_OK != ret)
    {
        log_error("iic send failed.");
        return ret;
    }
    
    ret = m_stop(this,speed);
    if(E_OK != ret)
    {
        log_error("iic stop failed.");
        return ret;
    }
    return E_OK;
}

#endif