/*****************************************************************************
* Copyright:
* File name: gps.h
* Description: gps模块 函数实现
* Author: 许少
* Version: V1.1
* Date: 2022/04/08
* log:  V1.0  2022/02/17
*       发布：
*       V1.1  2022/04/08
*       修复：gnvta获取 地址m_this->m_gga 少写了o_gnvtg成员问题
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_GPS_ENABLED

#define MODULE_NAME       "GPS"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME


#define GPS_MSG_SIZE       512
#define GPS_MSG_QUEUE_LEN   10
#define GPS_MSG_MAX_LEN    220

static void m_task(void* pdata);
static int m_get(const c_gps* this,nmea_gga* msg);
static int m_call_back(void* param,const u8* data,u16 data_len);
static u8* m_cut_out_data(u8* buf,u16* buf_len,const u8* head ,u8 head_len,const char* end,u8 end_len,u16 max_len);

typedef struct __M_GPS
{
    u8            m_uart_id               ;  /*串口id*/
    u8            m_msg_buf[GPS_MSG_SIZE] ;
    u16           m_msg_intex             ;  /*当前缓存位置编号*/
    nmea_gga      m_gga                   ;  /*gga数据*/
    TaskHandle_t  m_task_handle           ;  /*消息处理任务句柄*/
    QueueHandle_t m_queue_handle          ;  /*消息队列*/
}m_gps;


c_gps gps_creat(u8 uart_id)
{
    int ret = 0;
    BaseType_t os_ret = pdTRUE; 
    c_gps  new = {0};
    m_gps* m_this = NULL;  

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_gps));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_gps));
    m_this = new.this;

    /*初始化匹配的串口*/
    ret = my_uart.init(uart_id,GPS_BAUDRATE,256, UART_MODE_DMA);
    if(E_OK != ret)
    {
        log_error("Uart init failed.");
        goto error_handle;
    }

    /*注册回调函数*/
    ret = my_uart.set_callback(uart_id,m_this,m_call_back);
    if(E_OK != ret)
    {
        log_error("Set call back failed.");
        goto error_handle;
    }

    /*创建消息队列*/
    m_this->m_queue_handle = xQueueCreate(GPS_MSG_QUEUE_LEN,sizeof(u8*));
    if(NULL == m_this->m_queue_handle)
    {
        log_error("Queue creat filed.");
        goto error_handle;
    }

    /*创建消息解析任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_task        ,
                         (const char*    )"gps task"    ,
                         (uint16_t       )GPS_TASK_STK  ,
                         (void*          )m_this        ,
                         (UBaseType_t    )GPS_TASK_PRO  ,
                         (TaskHandle_t*  )&m_this->m_task_handle);
                         
    if(pdPASS != os_ret)
    {
        log_error("Gps task creat filed,ret=%d",(int)os_ret);
        goto error_handle;
    }

    /*保存相关变量*/
    new.get = m_get;
    return new;
    
    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;

}

static void m_task(void* pdata)
{
    int ret = 0;
    m_gps* m_this = NULL;  
    BaseType_t   os_ret = pdTRUE ;
    u8* new_msg = NULL;
    nmea_gngga  gga = {0};
    nmea_gnvtg  vtg = {0};
    
    /*参数检查*/
    if(NULL == pdata)
    {
        log_error("Param error.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    m_this = pdata;

    while(1)
    {
        /*死等消息字符串*/
        os_ret = xQueueReceive(m_this->m_queue_handle,&new_msg,portMAX_DELAY);
        if(pdTRUE != os_ret)
        {
            log_error("Failed to accept message ");
            continue;
        }

        if(strstr((char*)new_msg,"GNGGA"))
        {
            ret = sscanf((char*)new_msg,"$GNGGA,%f,%f,%c,%f,%c,%u,%u,%f,%f,",&gga.o_utc,&gga.o_latitude,&gga.o_latitude_hemisphere,
                                                                             &gga.o_longitude,&gga.o_longitude_hemisphere,&gga.o_state,
                                                                             &gga.o_num,&gga.o_hdop,&gga.o_altitude);
            if(ret >= 9)
            {
                memcpy(&m_this->m_gga.o_gngga,&gga,sizeof(nmea_gngga));
                //log_inform("error.");
            }                        
        }
        
        if(strstr((char*)new_msg,"GNVTG"))
        {
            /*地面速度信息 有磁力计*/
            ret = sscanf((char*)new_msg,"$GNVTG,%f,T,%f,M,%f,N,%f,K,%c*",&vtg.o_heading_due_north,&vtg.o_heading_magnetic_north,&vtg.o_ground_speed_knots,
                                                                             &vtg.o_ground_speed_kmh,&vtg.o_mode);
            if(ret >= 5)
            {
                memcpy(&m_this->m_gga.o_gnvtg,&vtg,sizeof(nmea_gnvtg));
                //log_inform("error.");
            }
            
            /*地面速度信息 无磁力计*/
            ret = sscanf((char*)new_msg,"$GNVTG,%f,T,,M,%f,N,%f,K,%c*",&vtg.o_heading_due_north,&vtg.o_ground_speed_knots,
                                                                             &vtg.o_ground_speed_kmh,&vtg.o_mode);
            if(ret >= 4)
            {
                memcpy(&m_this->m_gga.o_gnvtg,&vtg,sizeof(nmea_gnvtg));
                //log_inform("error.");
            }
        }

        else
        {
            //log_warning("Invalid message.");
        }

        vPortFree(new_msg);
        new_msg = NULL;
    }


    return;
}

static int m_call_back(void* param,const u8* data,u16 data_len)
{
    m_gps* m_this = NULL;
    u8*    cut_data = NULL;
    BaseType_t     os_ret  = pdFALSE;

    /*参数检查*/
    if(NULL == data || NULL == param)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = param;

    /*数据拷贝到缓存*/
    if(m_this->m_msg_intex + data_len > GPS_MSG_SIZE)
    {
        log_error("Buf full.");
        return E_ERROR;
    }
    memcpy(m_this->m_msg_buf + m_this->m_msg_intex,data,data_len);
    m_this->m_msg_intex += data_len;

    /*按头尾要求 裁剪数据*/
    do
    {
        cut_data = m_cut_out_data(m_this->m_msg_buf,&m_this->m_msg_intex,"$",strlen("$"),"\r\n",strlen("\r\n"),GPS_MSG_MAX_LEN);
        if(NULL != cut_data)
        {
            /*新消息 压到队列*/
            os_ret = xQueueSend(m_this->m_queue_handle,&cut_data,0);
            if(pdTRUE != os_ret)
            {
                log_error("Send message filed.");
                vPortFree(cut_data);
                cut_data = NULL;
                return E_ERROR;
            };
        }
    }while(cut_data);

    return E_OK;
}

static u8* m_cut_out_data(u8* buf,u16* buf_len,const u8* head ,u8 head_len,const char* end,u8 end_len,u16 max_len)
{
    u8* param_head = NULL;
    u8* param_end  = NULL;
    u16 i = 0;
    u8  j = 0;
    u16 data_len = 0;
    u8* cut_data = NULL;

    /*参数检查*/
    if(NULL == buf || NULL == buf_len || NULL == head || NULL == end)
    {
        log_error("Null pointer.");
        return NULL;
    }
    if(0 == *buf_len || 0 == max_len || 0 == head_len || 0 == end_len)
    {
        //log_error("Param error.");
        return NULL;
    }

    cut_again:

    /*查找命令头*/                                                           
    for(i = 0,param_head = NULL;buf + i + head_len + end_len <= buf + *buf_len &&      /*不超过总长*/  
                                buf + i + head_len + end_len <= buf +  max_len &&      /*不超过命令的最大长度*/
                                NULL == param_head;++i)                               /*还没找到头*/
    {
        /*不可比头部长 总长也不可超过buf_len的长度*/
        for(j = 0;j < head_len && i + j < *buf_len;++j)
        {
            if(buf[i + j] != head[j])
            {
                param_head = NULL;
                break;
            }

            /*对比完成*/
            else if(j == head_len - 1)
            {
                param_head = buf + i;
                
                break;
            }
        }
    }

    i -= 1;
    
    /*无头*/
    if(NULL == param_head)
    {
        /*如果超出了最大数据长度 清除已超出的长度*/
        if(buf + i + head_len  + end_len >= buf + max_len && buf + i + head_len  + end_len <= buf + *buf_len)
        {
            memcpy(buf,buf + i,*buf_len - i);  /*移动buf 覆盖超出长度得数据 */
            memset(buf + *buf_len - i,0,i);    /*清除因移动 而产生得空数据位*/
            *buf_len -= i;                     /*更新长度*/
            log_warning("The target string is too long,cut again.");
            
            goto cut_again;
        }
    
        return NULL;
    }

    /*头不在开头位置 覆盖掉前面无用得数据*/
    if(buf != param_head)
    {
         memcpy(buf,buf + i,*buf_len - i);  /*移动buf 覆盖超出长度得数据*/
         memset(buf + *buf_len - i,0,i);    /*清除因移动 而产生得空数据位*/
         *buf_len -= i;                     /*更新长度*/
         param_head = buf;
    }

    /*查找命令尾*/                                                         
    for(i = head_len,param_end = NULL;buf + i  + end_len <= buf + *buf_len &&    /*不超过总长*/
                                      buf + i + end_len <= buf + max_len &&      /*不超过命令的最大长度*/  
                                      NULL == param_end;++i)                           /*还没找到尾*/
    {
        /*查找尾部之前 需要再查一遍头部 防止再出现一个新的头*/
        for(j = 0;j < head_len && i + j < *buf_len;++j)
        {
            /*无头*/
            if(buf[i + j] != head[j])
            {
                break;
            }

            /*找到一个新头*/
            else if(j == head_len - 1)
            {
                memcpy(buf,buf + i,*buf_len - i);  /*移动buf 覆盖超出长度得数据 */
                memset(buf + *buf_len - i,0,i);    /*清除因移动 而产生得空数据位*/
                *buf_len -= i;                     /*更新长度*/
                param_head = param_end = NULL;
                log_warning("A new head.");
                goto cut_again;
            }
        }
    
        /*不可比尾部长 总长也不可超过buf_len的长度*/
        for(j = 0;j < end_len && buf + i + j < buf + *buf_len;++j)
        {
            if(buf[i + j] != end[j])
            {
                param_end = NULL;
                break;
            }

            /*对比完成*/
            else if(j == end_len - 1)
            {
                param_end = buf + i;                
                break;
            }
        }
    }
    
    i -= 1;

    /*有头无尾 */
    if(NULL == param_end)
    {
        /*如果超出了最大数据长度 清除已超出的长度*/
        if(param_head + i + end_len >= buf + max_len && param_head + i + end_len <= buf + *buf_len)
        {
            /*移动buf 覆盖超出长度得数据 但保留一个头部长度 防止末尾有不完全头部*/
            memcpy(buf,buf + max_len - head_len,*buf_len - max_len + head_len);  

            /*清除因移动 而产生得空数据位*/
            memset(buf + *buf_len - max_len + head_len,0,max_len - head_len);
            *buf_len -= max_len - head_len;                     /*更新长度*/
            log_warning("The target string is too long,cut again.");
            
            goto cut_again;
        }
    
        return NULL;
    }

    /*有头 有尾 且头部贴在buf头*/

    /*字符串有效长度*/
    data_len  = param_end - buf;
    data_len += end_len;

    cut_data = pvPortMalloc(data_len);
    if(NULL == cut_data)
    {
        log_error("Out of memory");
        return NULL;
    }

    /*拷贝数据到缓存*/
    memcpy(cut_data,buf,data_len);
    
    log_inform("cmd:%.*s\n",data_len,buf);

    /*如果后头还有数据 就把数据移动到前面来*/
    if(*buf_len > data_len)
    {
        memcpy(buf,buf + data_len,*buf_len - data_len);  /*移动buf 覆盖超出长度得数据*/
        memset(buf + *buf_len - data_len,0,data_len);    /*清除因移动 而产生得空数据位*/
        *buf_len -= data_len;                     /*更新长度*/
    }
    else
    {
        memset(buf,0,data_len);
        *buf_len = 0;
    }

    return cut_data;
}


static int m_get(const c_gps* this,nmea_gga* msg)
{
    m_gps* m_this = NULL;

    /*检查参数*/
    if(NULL == this || NULL == msg)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*信息拷贝出去*/
    memcpy(msg,&m_this->m_gga,sizeof(nmea_gga));
    
    return E_OK;
}
#endif

