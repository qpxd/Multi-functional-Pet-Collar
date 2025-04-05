/*****************************************************************************
* Copyright:
* File name: data_cut.c
* Description: 数据裁剪模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/02/23
* log:  V1.0  2022/02/23
*       发布：
*****************************************************************************/

#include "help_conf.h"

#ifdef HELP_DATACUT_ENABLED

#define MODULE_NAME       "data cut"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define BUF_MAGNIFICATION   3  /*缓存倍率*/
#define QUEUE_LEN          10  /*队列长度*/

#define MSG_TYPE_DATA   0
#define MSG_TYPE_CLEAN  1

static void m_task(void* pdata);

typedef struct __CUT_MSG
{
    u8  o_type;
    u16 o_data_len;
    u8* o_data;
}cut_msg;

typedef struct __M_DATA_CUT
{
    data_cut_cfg   m_cfg       ;  /*裁剪配置*/
    u8*            m_buf       ;  /*数据缓存*/
    u16            m_buf_index ;  /*数据量*/
    u8             m_type      ;  /*裁剪类型*/
    bool           m_cut_switch;  /*裁剪开关*/
    QueueHandle_t  m_cut_queue ;  /*输入数据消息队列*/
    QueueHandle_t  m_dat_queue ;  /*裁剪完成的消息队列*/
    TaskHandle_t   m_task      ;  /*裁剪消息队列*/

    int (*m_cust_cut)  (const u8* buf,u16 buf_len,u8** head,u8** end);
}m_data_cut;

static int m_cut_fb(m_data_cut* this,cut_msg* mesg);
static int m_cut_b (m_data_cut* this,cut_msg* mesg);
static int m_cut_custom(m_data_cut* this,cut_msg* mesg);

static int m_close (const c_data_cut* this);
static int m_open  (const c_data_cut* this);
static int m_clear (const c_data_cut* this);
static int m_add   (const c_data_cut* this,const u8* data,u16 data_len);
static int m_get   (const c_data_cut* this,const u8** data,u16* data_len,TickType_t xTicksToWait);
static int m_back  (const c_data_cut* this,u8* data);
static int m_get_static(const c_data_cut* this,u8* data,u16* data_len,TickType_t xTicksToWait);
static int m_set_custom_cut(const c_data_cut* this,int (*cust_cut)  (const u8* buf,u16 buf_len,u8** head,u8** end));

c_data_cut data_cut_creat(u8 cut_type,const data_cut_cfg* cfg)
{
    int ret = 0;
    c_data_cut  new = {0};
    m_data_cut* m_this = NULL;
    BaseType_t  os_ret = pdFALSE;

    /*检查配置参数*/
    if(NULL == cfg || 0 == cfg->o_max_len)
    {
        log_inform("Param error.");
        return new;
    }
    if(CUT_TYPE_FB != cut_type && CUT_TYPE_B != cut_type)
    {
        log_inform("Param error.");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_data_cut));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_data_cut));
    m_this = new.this;

    /*为buf开辟缓存*/
    m_this->m_buf = pvPortMalloc(cfg->o_max_len * BUF_MAGNIFICATION);
    if(NULL == new.this)
    {
        log_error("Out of memory");
        goto error_handle;
    }
    memset(m_this->m_buf,0,cfg->o_max_len * BUF_MAGNIFICATION);

    /*初始化输入数据消息队列*/
    m_this->m_cut_queue = xQueueCreate(QUEUE_LEN,sizeof(cut_msg));
    if(NULL == m_this->m_cut_queue)
    {
        log_error("Queue creat filed.");
        goto error_handle;
    }

    /*初始化裁剪完成消息队列*/
    m_this->m_dat_queue = xQueueCreate(QUEUE_LEN,sizeof(cut_msg));
    if(NULL == m_this->m_dat_queue)
    {
        log_error("Queue creat filed.");
        goto error_handle;
    }

    /*保存相关变量*/
    m_this->m_type = cut_type;
    m_this->m_cut_switch = true; /*默认打开裁剪*/
    memcpy(&m_this->m_cfg,cfg,sizeof(data_cut_cfg));  /*保存cfg*/
    new.clear  = m_clear ;
    new.add  = m_add     ;
    new.get  = m_get     ;
    new.back = m_back    ;
    new.close= m_close   ;
    new.open = m_open    ;
    new.get_static = m_get_static;
    new.set_custom_cut = m_set_custom_cut;

    /*创建裁剪任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_task        ,
                         (const char*    )"cut task"    ,
                         (uint16_t       )CUT_TASK_STK  ,
                         (void*          )m_this       ,
                         (UBaseType_t    )CUT_TASK_PRO  ,
                         (TaskHandle_t*  )&m_this->m_task);
                             
    if(pdPASS != os_ret)
    {
        log_error("Task creat failed");
        goto error_handle;
    }

    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_cut_fb(m_data_cut* this,cut_msg* mesg)
{
    return E_OK;
}
static int m_cut_b (m_data_cut* this,cut_msg* mesg)
{
    u8*         cut_data = NULL;
    BaseType_t  os_ret  = pdFALSE;
    u8*         param_end = NULL;
    u16 i = 0;
    u8  j = 0;

    u16  msg_len = 0;

    /*参数检查*/
    if(NULL == this || NULL == mesg)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    mesg->o_data = NULL;
    mesg->o_data_len = 0;
    
    /*如果长度还没有尾巴长 肯定不存在命令主体*/
    if(this->m_buf_index <= this->m_cfg.o_end_len)
    {
        return E_NULL;
    }
    
    /*查找命令尾*/
    for(i = 0;this->m_buf + i <= this->m_buf + this->m_cfg.o_max_len * BUF_MAGNIFICATION       &&
              this->m_buf + i <= this->m_buf + this->m_cfg.o_max_len - this->m_cfg.o_end_len &&
              NULL == param_end;++i)
    {
        /*不可比尾部长 总长也不可超过buf_index*/
        for(j = 0;j < this->m_cfg.o_end_len && i + j < this->m_buf_index;++j)
        {
            if(this->m_buf[i + j] != this->m_cfg.o_end[j])
            {
                param_end = NULL;
                break;
            }

            /*对比完成*/
            else if(j == this->m_cfg.o_end_len - 1)
            {
                param_end = this->m_buf + i;
                msg_len = i + this->m_cfg.o_end_len;
                break;
            }
        }
    }

    i -= 1;

    /*无尾*/
    if(NULL == param_end)
    {
        /*无尾超长*/
        i = this->m_cfg.o_max_len - this->m_cfg.o_end_len;
        if(this->m_buf_index > i)
        {
            memcpy(this->m_buf,this->m_buf + i,this->m_buf_index - i);
            memset(this->m_buf + this->m_buf_index - i,0,i);
            this->m_buf_index -= i;
            
            //goto cut_again;
            return E_OK;  /*需要再一次裁剪*/
        }
        
        return E_NULL;    /*无法再裁剪一次*/
    }

    /*有尾*/

    /*为消息开辟缓存 必须大于尾长 否则认为是一条空消息*/
    if(msg_len > this->m_cfg.o_end_len)
    {
        mesg->o_data = pvPortMalloc(msg_len);
        mesg->o_data_len = msg_len;
        if(NULL == mesg->o_data)
        {
            log_error("Out of memory");
            return E_NULL;
        }
        memset(mesg->o_data,0,msg_len);

        /*拷贝数据到缓存*/
        memcpy(mesg->o_data,this->m_buf,msg_len);
    }
    
    
    /*移动数据到头部*/
    memcpy(this->m_buf,this->m_buf + msg_len,this->m_buf_index - msg_len);
    memset(this->m_buf + this->m_buf_index - msg_len,0,msg_len);
    this->m_buf_index -= msg_len;
    
    /*如果还有数据 继续裁剪*/
    if(this->m_cfg.o_end_len < this->m_buf_index)
    {
        param_end = NULL;
        return E_OK;
    }

    return E_NULL;
}

static int m_cut_custom(m_data_cut* this,cut_msg* mesg)
{
    u8*         cut_data = NULL;
    BaseType_t  os_ret  = pdFALSE;
    u8*         param_end = NULL;
    u8* cust_head = NULL;
    u8* cust_end = NULL;
    int ret = 0;
    u16  end_len = 0; /*尾部后 剩余数据长度*/
    u16  msg_len = 0;

    /*参数检查*/
    if(NULL == this || NULL == mesg)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    mesg->o_type = MSG_TYPE_DATA;
    mesg->o_data = NULL;
    mesg->o_data_len = 0;

    /*检查自定义裁剪函数*/
    if(NULL == this->m_cust_cut)
    {
        return E_NULL;
    }
    cust_head = NULL;
    cust_end  = NULL;

    /*自定义裁剪*/
    ret = this->m_cust_cut(this->m_buf,this->m_buf_index,&cust_head,&cust_end);
    if(E_AGAIN == ret)
    {
        return ret;
    }
    else if(E_OK != ret)
    {
        return E_ERROR;
    }
    
    /*检查自定义裁剪指针的有效性*/
    if(cust_end > cust_head &&
       this->m_buf <= cust_head && this->m_buf + this->m_buf_index >= cust_head &&
       this->m_buf <= cust_end  && this->m_buf + this->m_buf_index >= cust_end)
    {
        msg_len = cust_end - cust_head;
        mesg->o_data = pvPortMalloc(msg_len);
        mesg->o_data_len = msg_len;
        if(NULL == mesg->o_data)
        {
            log_error("Out of memory");
            return E_NULL;
        }
        memset(mesg->o_data,0,msg_len);

        /*拷贝数据到缓存*/
        memcpy(mesg->o_data,cust_head,msg_len);
    }
    else
    {
        log_error("Wrong custom cropping.");
    }

    /*抽走 裁剪的数据 将剩下的数据接在一起*/
    end_len  = cust_head - this->m_buf;
    end_len += msg_len;
    end_len  = this->m_buf_index - end_len;
    memset(cust_head,0,msg_len);
    memcpy(cust_head,cust_end,end_len);
    this->m_buf_index -= msg_len;

    return E_OK;
}

static int m_add   (const c_data_cut* this,const u8* data,u16 data_len)
{
    m_data_cut* m_this = NULL;
    cut_msg     new_mesg = {0};
    BaseType_t  os_ret       ;

    /*参数检查*/
    if(NULL == this || NULL == this->this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*如果裁剪被关闭了 直接退出*/
    if(false == m_this->m_cut_switch)
    {
        return E_OK;
    }
    
    if(0 == data_len || CUT_DATA_ADD_MAX_LEN < data_len)
    {
        log_error("Param error.");
        return E_ERROR;
    }

    /*创建新的消息*/
    new_mesg.o_data = pvPortMalloc(data_len);
    if(NULL == new_mesg.o_data)
    {
        log_error("Out of memory");
        return E_ERROR;
    }
    memset(new_mesg.o_data,0,data_len);

    /*数据 拷贝到新的消息中*/
    memcpy(new_mesg.o_data,data,data_len);
    new_mesg.o_type = MSG_TYPE_DATA;       /*数据类型*/
    new_mesg.o_data_len = data_len;

    /*发送消息*/
    os_ret = xQueueSend(m_this->m_cut_queue,&new_mesg,0);
    if(pdTRUE != os_ret)
	{
		log_error("Send message filed.");
        vPortFree(new_mesg.o_data);
        new_mesg.o_data = NULL;
		return E_ERROR;
	}

    return E_OK;
}

static int m_get   (const c_data_cut* this,const u8** data,u16* data_len,TickType_t xTicksToWait)
{
    m_data_cut* m_this = NULL;
    cut_msg     new_mesg = {0};
    BaseType_t  os_ret       ;

    /*参数检查*/
    if(NULL == this || NULL == this->this || NULL == data || NULL == data_len)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*接收消息*/
    os_ret = xQueueReceive(m_this->m_dat_queue,&new_mesg,xTicksToWait);
    if(pdTRUE != os_ret)
	{
		log_error("Recv message filed.");
		return E_ERROR;
	}

    /*消息输出 出去*/
    *data_len = new_mesg.o_data_len;
    *data     = new_mesg.o_data;

    return E_OK;
}

static int m_get_static(const c_data_cut* this,u8* data,u16* data_len,TickType_t xTicksToWait)
{
    m_data_cut* m_this = NULL;
    cut_msg     new_mesg = {0};
    BaseType_t  os_ret       ;
    int ret = 0;

    /*参数检查*/
    if(NULL == this || NULL == this->this || NULL == data || NULL == data_len)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    /*接收消息*/
    os_ret = xQueueReceive(m_this->m_dat_queue,&new_mesg,xTicksToWait);
    if(pdTRUE != os_ret)
	{
		log_error("Recv message filed.");
		return E_ERROR;
	}
    
    /*如果buf不够*/
    if(*data_len < new_mesg.o_data_len)
    {
        log_error("Out of memory.");
        vPortFree(new_mesg.o_data);
        new_mesg.o_data = NULL;
        return E_ERROR;
    }

    /*消息输出 出去*/
    memset(data,0,*data_len);
    *data_len = new_mesg.o_data_len;
    memcpy(data,new_mesg.o_data,new_mesg.o_data_len);

    /*释放消息*/
    vPortFree(new_mesg.o_data);
    new_mesg.o_data = NULL;

    return E_OK;
}

static int m_back  (const c_data_cut* this,u8* data)
{
    /*参数检查*/
    if(NULL == data)
    {
        log_error("Null pointer");
        return E_NULL;
    }

    /*释放掉内存*/
    vPortFree(data);

    return E_OK;
}


static int m_clear (const c_data_cut* this)
{
    m_data_cut* m_this = NULL;
    BaseType_t  os_ret  = pdFALSE;
    cut_msg     new_mesg = {0};

    /*参数检查*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    new_mesg.o_type = MSG_TYPE_CLEAN;   /*清除消息*/

    /*发送消息*/
    os_ret = xQueueSend(m_this->m_cut_queue,&new_mesg,0);
    if(pdTRUE != os_ret)
	{
		log_error("Send message filed.");
        vPortFree(new_mesg.o_data);
        new_mesg.o_data = NULL;
		return E_ERROR;
	}

    return E_OK;
}

static void m_task(void* pdata)
{
    m_data_cut* m_this = NULL;
    cut_msg     new_mesg = {0};
    BaseType_t  os_ret       ;
    int ret = 0;

    /*参数检查*/
    if(NULL == pdata)
    {
        log_error("Param error.");
        vTaskDelete(NULL);        /*删除本任务*/
    }
    m_this = pdata;
    
    while(1)
    {
        /*获取裁剪消息*/
        os_ret = xQueueReceive(m_this->m_cut_queue,&new_mesg,portMAX_DELAY);
        if(pdTRUE != os_ret)
        {
            log_error("Recv message filed.");
            continue;
        }

        /*如果是清除消息*/
        if(MSG_TYPE_CLEAN == new_mesg.o_type)
        {
            /*清空buf*/
            m_this->m_buf_index = 0;
            memset(m_this->m_buf,0,m_this->m_cfg.o_max_len * BUF_MAGNIFICATION);

            /*清空数据队列*/
            do
            {
                os_ret = xQueueReceive(m_this->m_dat_queue,&new_mesg,0);
                if(NULL != new_mesg.o_data)
                {
                    vPortFree(new_mesg.o_data);
                    new_mesg.o_data = NULL;
                }
            }
            while (pdTRUE == os_ret);
            
        }

        /*如果是数据 压入buf 并进行裁剪*/
        else if(MSG_TYPE_DATA == new_mesg.o_type)
        {
            /*先将新收入的数据*/
            if(m_this->m_buf_index + new_mesg.o_data_len > m_this->m_cfg.o_max_len * BUF_MAGNIFICATION)
            {
                log_error("Buf full.");
                vPortFree(new_mesg.o_data);
                new_mesg.o_data = NULL;
                continue;
            }
            memcpy(m_this->m_buf + m_this->m_buf_index,new_mesg.o_data,new_mesg.o_data_len);
            m_this->m_buf_index += new_mesg.o_data_len;
            
            /*释放消息*/
            vPortFree(new_mesg.o_data);
            new_mesg.o_data = NULL;
            new_mesg.o_data_len = 0;

            /*数据裁剪*/
            do
            {
                /*自定义裁剪*/
                ret = m_cut_custom(m_this,&new_mesg);
                if(E_AGAIN == ret)
                {
                    break;
                }
                else if(E_OK != ret)
                {
                    /*尾部裁剪*/
                    if(CUT_TYPE_B == m_this->m_type)
                    {
                        ret = m_cut_b(m_this,&new_mesg);
                    }

                    /*头尾裁剪*/
                    else if(CUT_TYPE_FB == m_this->m_type)
                    {
                        ret = m_cut_fb(m_this,&new_mesg);
                    }

                    /*错误的裁剪类型*/
                    else
                    {
                        log_error("Error cut type.");
                    }
                }
                

                /*发送消息到数据队列*/
                if(NULL != new_mesg.o_data)
                {
                    #if SHOW_CUT_DATA
                    log_inform("Len:%d,Cut:%.*s.",new_mesg.o_data_len,new_mesg.o_data_len,new_mesg.o_data);
                    #endif
                    os_ret = xQueueSend(m_this->m_dat_queue,&new_mesg,0);
                    if(pdTRUE != os_ret)
                    {
                        log_error("Send message filed.");
                        vPortFree(new_mesg.o_data);
                        new_mesg.o_data = NULL;
                    }
                }
                
            }
            while (E_OK == ret);
            
            //log_inform("SIZE:%u",uxTaskGetStackHighWaterMark(NULL));
        }
    }

    error_handle:

    log_error("Task be delete.");
    vTaskDelete(NULL);        /*删除本任务*/
}

static int m_close (const c_data_cut* this)
{
    m_data_cut* m_this = NULL;
    cut_msg     new_mesg = {0};
    BaseType_t  os_ret       ;

    /*参数检查*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    m_this->m_cut_switch = false;
    
    return E_OK;
}

static int m_open  (const c_data_cut* this)
{
    m_data_cut* m_this = NULL;

    /*参数检查*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    m_this->m_cut_switch = true;
    
    return E_OK;
}

static int m_set_custom_cut(const c_data_cut* this,int (*cust_cut)  (const u8* buf,u16 buf_len,u8** head,u8** end))
{
    m_data_cut* m_this = NULL;

    /*参数检查*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    m_this->m_cust_cut = cust_cut;

    return E_OK;
}
#endif