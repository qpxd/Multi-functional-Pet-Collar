/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : jq8400.h
* Description: jq8400语音播报模块 公开头文件
* Author: xzh
* Version: v1.2
* Date: 2022/02/18
* log:  V1.0  2022/02/05
*       发布：
*
*       V1.1  2022/02/06
*       修复：m_state 状态获取导致的HardFault_Handler。
*             获取状态失败后，等待一会，程序会进入状态获取导致的HardFault_Handler。
*             堆栈观察变现为空指针解引。
*             初步观察，m_state中等待状态回复时间写成了0 修改后错误消失。
*             进一步的，就算时间写0，最多事超时，不应该进入错误。怀疑事回调掉
*             用空指针，断点查看，回调只调用了一次，且调用成功，死是死在回调出
*             来之后。通过PSP指针定位，没有找到发生错误的代码块，问题陷入僵局。
*             在my_uart中禁用回调后，发现错误消失，说明问题还是出在回调。在jq8400
*             的回调函数中按每行注释的方法排查，最终定位在memcpy上。检查memcpy
*             进入的参数，没有空地址。但是反推地址来源，目标地址为m_state的一个
*             局部变量。因为接收是异步的，所以如果时间写0，接收还没有完成，局部
*             变量就已经被释放了，再次拷贝虽然不是空指针，但会写入到后入栈函数
*             的调用栈，导致未知的错误，且位置不定。
*             将变量修改为堆内存，问题解决。注：异步写入 不要使用局部变量
*
*       V1.2  2022/02/18
*       修复：jq8400_creat参数保存位置在send后，导致send使用的时候需要用到的参数
*             还没有保存到位。之前没有暴露该问题的原因在于，uart_id默认为0，默认
*             值0对应串口1也是可以使用的，但换成其它串口，则会导致回调错乱。
*
*       V1.3  2022/02/24
*       同步：串口更新后，取消了clean和缓存机制，同步更新jq8400的接收机制
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_JQ8400_ENABLED

#define MODULE_NAME       "jq8400"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

const u8 g_cmd_disk   [] = {0xAA , 0x0B , 0x01 , 0x02 , 0xB8};   /*指定盘符*/
const u8 g_cmd_state  [] = {0xAA , 0x01 , 0x00 , 0xAB};          /*获取状态*/
const u8 g_cmd_play   [] = {0xAA , 0x02 , 0x00 , 0xAC};          /*播放*/
const u8 g_cmd_pause  [] = {0xAA , 0x03 , 0x00 , 0xAD};          /*暂停播放*/
const u8 g_cmd_stop   [] = {0xAA , 0x04 , 0x00 , 0xAE};          /*停止播放*/
const u8 g_cmd_once   [] = {0xAA , 0x18 , 0x01 , 0x02 , 0xC5};   /*单曲停止*/
const u8 g_cmd_loop   [] = {0xAA , 0x18 , 0x01 , 0x01 , 0xC4};   /*单曲循环*/
const u8 g_cmd_volume [] = {0xAA , 0x13 , 0x01 , 0xFF , 0xFF};   /*音量设置*/
const u8 g_cmd_specify[] = {0xAA , 0x07 , 0x02 , 0xFF , 0xFF , 0xFF};   /*指定播放*/

typedef struct __M_JQ8400
{
    u8  m_uart_id;
    u8  recv_data[5];
    u8  recv_index;
    u8  recv_need;
    SemaphoreHandle_t m_sem;
}m_jq8400;


static int m_play  (const c_jq8400* this,bool loop,u16 index);
static int m_stop  (const c_jq8400* this);
static int m_state (const c_jq8400* this,u8* state);
static int m_volume(const c_jq8400* this,u8 volume);
static int m_send(const c_jq8400* this,const u8* send_data,u8 send_len,u8 recv_len,TickType_t time);
static int m_callback(void* param,const u8* data,u16 data_len);


c_jq8400 jq8400_creat(u8 uart_id)
{
    int ret = 0;
    c_jq8400  new = {0};
    m_jq8400* m_this = NULL;  

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_jq8400));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_jq8400));
    m_this = new.this;

    /*创建二值信号量*/
    m_this->m_sem = xSemaphoreCreateBinary();
    if(NULL == m_this->m_sem)
    {
        log_error("Semaphore create failed.");
        goto error_handle;
    }

    /*初始化匹配的串口*/
    ret = my_uart.init(uart_id,JQ8400_BAUDRATE,15, UART_MODE_DMA);
    if(E_OK != ret)
    {
        log_error("Uart init failed.");
        goto error_handle;
    }
    
    /*保存相关参数*/
    m_this->m_uart_id = uart_id;
    new.play   = m_play    ;
    new.stop   = m_stop    ;
    new.state  = m_state   ;
    new.volume = m_volume  ;

    vTaskDelay(1000);  /*等待模块 启动*/
    
    /*指定盘符为flash*/
    ret = m_send(&new,g_cmd_disk,sizeof(g_cmd_disk),0,0);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        goto error_handle;
    }
    
    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_play  (const c_jq8400* this,bool loop,u16 index)
{
    u8 data[6] = {0};
    m_jq8400* m_this = NULL;
    int ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*新的播放 都会先停止一次*/
    ret = m_stop(this);
    if(E_OK != ret)
    {
        log_error("Stop failed.");
        return E_ERROR;
    }

    /*检查是否需要循环播放*/
    if(true == loop)
    {
        ret = m_send(this,g_cmd_loop,sizeof(g_cmd_loop),0,0);
    }

    /*不循环 就播放一次*/
    else 
    {
        ret = m_send(this,g_cmd_once,sizeof(g_cmd_once),0,0);
    }
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    /*播放指定曲目*/
    memcpy(data,g_cmd_specify,3);
    data[3] = index >> 8;
    data[4] = index & 0x00FF;
    data[5] = data[0] + data[1] + data[2] + data[3] + data[4];
    ret = m_send(this,data,sizeof(data),0,0);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_stop  (const c_jq8400* this)
{
    int ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*发送停止播放命令*/
    ret = m_send(this,g_cmd_stop,sizeof(g_cmd_stop),0,0);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_state (const c_jq8400* this,u8* state)
{
    int ret = 0;
    m_jq8400* m_this = NULL;

    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == state)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*发送停止播放命令*/
    ret = m_send(this,g_cmd_state,sizeof(g_cmd_state),sizeof(m_this->recv_data),1000);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    /*命令校验*/
    if(m_this->recv_data[4] != m_this->recv_data[0] + m_this->recv_data[1] + m_this->recv_data[2] + m_this->recv_data[3])
    {
        log_error("Incorrect data received.");
        return E_ERROR;
    }

    /*状态校验*/
    if(m_this->recv_data[3] > 2)
    {
        log_error("Error state.");
        return E_ERROR;
    }

    /*输出状态*/
    *state = m_this->recv_data[3];

    return E_OK;
}

static int m_volume(const c_jq8400* this,u8 volume)
{
    u8 data[5] = {0};
    int ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(volume > JQ8400_VOLUME_MAX)
    {
        log_error("Param error.");
        return E_PARAM;
    }

    /*设置音量*/
    memcpy(data,g_cmd_volume,3);
    data[3] = volume;
    data[4] = data[0] + data[1] + data[2] + data[3];
    ret = m_send(this,data,sizeof(data),0,0);
    if(E_OK != ret)
    {
        log_error("Send failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_send(const c_jq8400* this,const u8* send_data,u8 send_len,u8 recv_len,TickType_t time)
{
    m_jq8400* m_this = NULL;
    BaseType_t os_ret = 0;
    int ret = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == send_data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*如果需要接收数据 就设置回调函数 否则清空回调*/
    if(0 != recv_len)
    {
        /*清空一下信号量*/
        xSemaphoreTake(m_this->m_sem,0);
        
        /*清空一下接收缓存*/
        memset(m_this->recv_data,0,sizeof(m_this->recv_data));
        m_this->recv_index = 0;
        m_this->recv_need = recv_len;
        
        ret = my_uart.set_callback(m_this->m_uart_id,(void*)this,m_callback);
    }
    else
    {
        ret = my_uart.set_callback(m_this->m_uart_id,NULL,NULL);
        m_this->recv_need = 0;
    }
    if(E_OK != ret)
    {
        log_error("Callback set failed.");
        return E_ERROR;
    }

    /*发送数据*/
    ret = my_uart.send(m_this->m_uart_id,send_data,send_len);
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
        return E_ERROR;
    }

    if(0 != recv_len)
    {
        /*等待回包*/
        os_ret = xSemaphoreTake(m_this->m_sem,1000);
        if(pdTRUE != os_ret)
        {
            log_error("Receive timeout.");
            return E_ERROR;
        }
    }

    /*如果等到了 数据就已经收到了*/
    return E_OK;
}

static int m_callback(void* param,const u8* data,u16 data_len)
{
    m_jq8400* m_this = NULL;
    BaseType_t os_ret = 0;

    /*检查参数*/
    if(NULL == param || NULL == data || 0 == data_len)
    {
        log_error("Uart data error.");
        return E_ERROR;
    }

    m_this = ((c_jq8400*)param)->this;
    
    /*如果没有接收需求*/
    if(0 == m_this->recv_need)
    {
        return E_OK;
    }
    
    /*是否已经收满了*/
    if(m_this->recv_index + data_len > sizeof(m_this->recv_data))
    {
        log_error("Buf full.");
        return E_ERROR;
    }

    /*数据拷贝过去*/
    memcpy(m_this->recv_data + m_this->recv_index,data,data_len);
    m_this->recv_index += data_len;

    /*如果达到需求 发送信号量*/
    if(m_this->recv_index >= m_this->recv_need)
    {
        os_ret = xSemaphoreGive(m_this->m_sem);
        if(pdTRUE != os_ret)
        {
            log_error("Semaphore give failed.");
            return E_ERROR;
        }
    }

    return E_OK; 
}


#endif