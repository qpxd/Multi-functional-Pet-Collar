/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : sim800.h
* Description: sim800驱动模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/25
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_SIM800_ENABLED

#define MODULE_NAME       "sim800"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define MESG_QUEUE_MAX_LEN   5
#define BUF_QUEUE_MAX_LEN    5
#define AT_RECV_MAX_LEN     (128)

/*短信消息*/
typedef struct __MESG
{
    const char o_num[SIM800_NUM_LEN + 1]        ;   /*号码*/
    const char o_content[SIM800_MSG_MAX_LEN];   /*内容*/
}mesg;


typedef struct __M_SIM800
{
    u8  m_uart_id;
    u32 m_signal ;
    TaskHandle_t    m_task  ;
    QueueHandle_t   m_queue ;  /*短信消息队列*/
    c_data_cut      m_cut   ;  /*数据裁剪*/
}m_sim800;


static int m_callback(void* param,const u8* data,u16 data_len);
static void m_task(void* pdata);
static int m_message(const c_sim800* this,const char* num,const char* content);
static int m_message_cnt(const c_sim800* this, unsigned char *count);
static int m_message_del(const c_sim800* this);
static int m_message_content(const c_sim800* this, unsigned char index, char *num, char *content);

static int m_get(const m_sim800* this,const u8* data,u8* recv,TickType_t time);
static int m_get_content(const m_sim800* this,const u8* data,u8* recv,TickType_t time);
static int m_set(const m_sim800* this,const u8* data,TickType_t time);
static int m_match(const m_sim800* this);
static int m_send_message(const m_sim800* this,const char* num,const char* content);
static int m_recv_find(const m_sim800* this,const char* str,TickType_t time);

const char* g_cmd_mode   = "AT+CMGF=1\r\n";         /*短信模式 0表示PDU模式(可以发中文) 1表示文本模式(可以发英文) */
const char* g_cmd_text   = "AT+CSMP=17,11,0,0\r\n"; /*短信文本模式*/
const char* g_cmd_code   = "AT+CSCS=\"IRA\"\r\n";   /*国际参考字符集编码*/
const char* g_cmd_card   = "AT+CPIN?\r\n";
const char* g_cmd_signal = "AT+CSQ\r\n";

const char* g_cmd_del    = "AT+CMGD=1,4\r\n";	/*删除所有短信*/
const char* g_cmd_count  = "AT+CPMS?\r\n";		/*获取短信数量*/
const char* g_cmd_content= "AT+CMGR=%d\r\n";    /*获取短信内容*/

c_sim800 sim800_creat(u8 uart_id)
{
    int ret = 0;
    c_sim800    new = {0};
    m_sim800*   m_this = NULL;  
    BaseType_t  os_ret   = 0 ;
    data_cut_cfg cfg = {0};

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_sim800));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_sim800));
    m_this = new.this; 

    /*保存相关变量*/
    m_this->m_uart_id = uart_id;
    new.message 		= m_message;
	new.message_del		= m_message_del;
	new.message_cnt 	= m_message_cnt;
	new.message_content	= m_message_content;
	
    /*初始化串口*/
    ret = my_uart.init(uart_id,SIM800_BAUDRATE,15, UART_MODE_DMA);
    if(E_OK != ret)
    {
        log_error("Uart init failed.");
        goto error_handle;
    }

    /*注册串口回调*/
    ret = my_uart.set_callback(m_this->m_uart_id,(void*)m_this,m_callback);
    if(E_OK != ret)
    {
        log_error("Callback set failed.");
        goto error_handle;
    }

    /*初始化消息队列*/
    m_this->m_queue = xQueueCreate(MESG_QUEUE_MAX_LEN,sizeof(mesg));
    if(NULL == m_this->m_queue)
    {
        log_error("Queue creat filed.");
        goto error_handle;
    }

    /*初始化数据裁剪器*/
    cfg.o_end     = (u8*)"\r\n"     ;
    cfg.o_end_len = 2               ;
    cfg.o_max_len = AT_RECV_MAX_LEN ;
    m_this->m_cut = data_cut_creat(CUT_TYPE_B,&cfg);
    if(NULL == m_this->m_cut.this)
    {
        log_error("Cut creat filed.");
        goto error_handle;
    }

    /*创建任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_task       ,
                     (const char*    )"sim800 task"    ,
                     (uint16_t       )SIM800_TASK_STK  ,
                     (void*          )m_this           ,
                     (UBaseType_t    )SIM800_TASK_PRO  ,
                     (TaskHandle_t*  )&m_this->m_task);
                                 
    if(pdPASS != os_ret)
    {
        log_error("Sim800 task creat filed,ret=%d",(int)os_ret);
        goto error_handle;
    }

    return new;
    
    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;

}

static void m_task(void* pdata)
{
    int ret = 0;
    m_sim800* this = NULL;
    BaseType_t  os_ret = 0 ;
    u8 recv_buf[AT_RECV_MAX_LEN] = {0};
    mesg new_msg = {0};

    /*参数检查*/
    if(NULL == pdata)
    {
        log_error("Param error.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    this = pdata;
    
    /*等待启动*/
    vTaskDelay(1000);
    
    /*匹配波特率*/
    ret = m_match(this);
    if(E_OK != ret)
    {
        log_error("Match failed.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    
    while(1)
    {     
        /*检查卡状态*/
        memset(recv_buf,0,AT_RECV_MAX_LEN);
        ret = m_get(this,(const u8*)g_cmd_card,recv_buf,1000);
        if(E_OK != ret)
        {
            log_warning("Get failed.");
            goto again;
        }
        if(NULL == strstr((char*)recv_buf,"+CPIN: READY"))
        {
            log_warning("No card.");
            goto again;
        }
        
        /*检查信号状态*/
        memset(recv_buf,0,AT_RECV_MAX_LEN);
        ret = m_get(this,(const u8*)g_cmd_signal,recv_buf,1000);
        if(E_OK != ret)
        {
            log_warning("Get failed.");
            goto again;
        }
        ret = sscanf((char*)recv_buf,"+CSQ: %d",&this->m_signal);
        if(ret < 1)
        {
            log_warning("Get signal failed.");
            goto again;
        }
        if(this->m_signal < 5)
        {
            log_warning("signal bottom,%d.",this->m_signal);
        }
        
        /*设置短信模式*/
        ret = m_set(this,(const u8*)g_cmd_mode,1000);
        if(E_OK != ret)
        {
            log_warning("Set failed.");
            goto again;
        }
        
        /*设置文本模式参数*/
        ret = m_set(this,(const u8*)g_cmd_text,1000);
        if(E_OK != ret)
        {
            log_warning("Set failed.");
            goto again;
        }
        
        /*设置字符集*/
        ret = m_set(this,(const u8*)g_cmd_code,1000);
        if(E_OK != ret)
        {
            log_warning("Set failed.");
            goto again;
        }
        
        break;
        
        again:
        
        vTaskDelay(5000);
    }

    //m_send_message(this,"18030864931","123456");

    while(1)
    {
        /*死等  取消息*/
        os_ret = xQueueReceive(this->m_queue,&new_msg,portMAX_DELAY);
        if(pdTRUE != os_ret)
        {
            log_error("Failed to accept message ");
            continue;
        }

        /*发送短信*/
        ret = m_send_message(this,new_msg.o_num,new_msg.o_content);
        if(E_OK != ret)
        {
            log_warning("Send failde.");
        }
    
        vTaskDelay(100);
    }

}

static int m_message(const c_sim800* this,const char* num,const char* content)
{
    m_sim800* m_this = NULL;
    mesg new_msg = {0};
    BaseType_t     os_ret  = 0;

    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(strlen(num) > SIM800_NUM_LEN || strlen(num) > SIM800_MSG_MAX_LEN)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    strcpy((char*)new_msg.o_num,num);
    strcpy((char*)new_msg.o_content,content);

    /*压入消息队列*/
    os_ret = xQueueSend(m_this->m_queue,&new_msg,0);
    if(pdTRUE != os_ret)
    {
    	log_error("Send message filed.");
    	return E_ERROR;
    }

    return E_OK;
}

static int m_message_cnt(const c_sim800* this, unsigned char *count)
{
    m_sim800* m_this = NULL;
    mesg new_msg = {0};
    BaseType_t     os_ret  = 0;
	char recv_buf[AT_RECV_MAX_LEN] = {0};
	
    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == count)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
	
	/*设置字符集*/
	int m_cnt = 0;
	int m_total = 0;
	memset(recv_buf,0,AT_RECV_MAX_LEN);
	int ret = m_get(this->this, (const u8*)g_cmd_count, recv_buf, 2000);
	if(E_OK != ret)
	{
		log_warning("Get failed.");
		return E_ERROR;
	}
	else
	{
		char *pxIndex = NULL;
		pxIndex = strtok(recv_buf, ",");
		if(pxIndex == NULL) return E_ERROR;
		while(pxIndex != NULL)
		{
			if(strstr(pxIndex, "\"SM_P\"") != NULL)
			{
				pxIndex = strtok(NULL, ","); 
				if(pxIndex == NULL) return E_ERROR;
				m_cnt = atoi(pxIndex);
				pxIndex = strtok(NULL, ","); 
				if(pxIndex == NULL) return E_ERROR;
				m_total = atoi(pxIndex);
				break;
			}
			pxIndex = strtok(NULL, ",");
			if(pxIndex == NULL) return E_ERROR;
		}
		log_warning("Current message: %d/%d", m_cnt, m_total);
		*count = m_cnt;
	}
	
    return E_OK;
}


static int m_message_del(const c_sim800* this)
{
    m_sim800* m_this = NULL;
    mesg new_msg = {0};
    BaseType_t     os_ret  = 0;
	int ret;
	
    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

	/*删除所有短信*/
	ret = m_set(this->this, (const u8*)g_cmd_del, 1000);
	if(E_OK != ret){
		log_warning("Del failed.");
	}else{
		log_warning("All message del.");
	}
	
    return E_OK;
}

static int m_message_content(const c_sim800* this, unsigned char index, char *num, char *content)
{
    m_sim800* m_this = NULL;
    mesg new_msg = {0};
    BaseType_t     os_ret  = 0;
	int ret;
	char senf_buf[16];
	char recv_buf[AT_RECV_MAX_LEN] = {0};
	
    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == num || NULL == content)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

	/*删除所有短信*/
	memset(senf_buf, 0, sizeof(senf_buf));
	memset(recv_buf, 0, AT_RECV_MAX_LEN);
	sprintf(senf_buf, g_cmd_content, index);

	ret = m_get_content(this->this, (const u8*)senf_buf, recv_buf, 1000);
	if(E_OK != ret)
	{
		log_warning("Get failed.");
		return E_ERROR;
	}
	else
	{
		char *pxIndex = NULL;
		pxIndex = strtok(recv_buf, ",");
		if(pxIndex == NULL) return E_ERROR;

		pxIndex = strtok(NULL, ",");
		if(pxIndex == NULL) return E_ERROR;
		strcpy(num, pxIndex);
		
		pxIndex = strtok(NULL, "\r\n");
		if(pxIndex == NULL) return E_ERROR;
		
		pxIndex = strtok(NULL, "\r\n");
		if(pxIndex == NULL) return E_ERROR;
		strcpy(content, pxIndex);
	}

	log_inform("Number: %s", num);
	log_inform("Text  : %s", content);
    return E_OK;
}

static int m_match(const m_sim800* this)
{
    int ret = 0;
    BaseType_t  os_ret = 0 ;
    u8* new_msg = 0;
    u16 recv_len = 0;
    u8 i = 0;

    /*参数检测*/
    if(NULL == this )
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    for(i = 0;i < 5;++i)
    {
    
        /*发送命令*/
        ret = my_uart.send(this->m_uart_id,(u8*)"AT\r\n",4);
        if(E_OK != ret)
        {
            log_error("Uart send failed.");
            return E_ERROR;
        }

        /*AT回复*/
        ret = m_recv_find(this,"AT\r\r\n",500);
        if(E_OK != ret)
        {
            continue;
        }

        /*等待ok*/
        ret = m_recv_find(this,"OK\r\n",500);
        if(E_OK != ret)
        {
            continue;
        }
        break;
    }
    
    /*检查波特率是否匹配成功*/
    if(i >= 5)
    {
        log_error("Match failed.");
        return E_ERROR;
    }
    
    return E_OK;
}


static int m_callback(void* param,const u8* data,u16 data_len)
{
    int ret = 0;
    m_sim800*  m_this = NULL;

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

    /*数据压入给数据裁剪对象*/
    ret = m_this->m_cut.add(&m_this->m_cut,data,data_len);
    if(E_OK != ret)
    {
        log_error("Cut add failed.");
        return E_ERROR;
    }

    /*数据裁剪*/
//    ret = m_this->m_cut.cut_b(&m_this->m_cut,data,data_len);
//    if(E_OK != ret)
//    {
//        log_error("Cut data failed.");
//        return E_ERROR;
//    }

    return E_OK;
}

static int m_get_content(const m_sim800* this,const u8* data,u8* recv,TickType_t time)
{
    int ret = 0;
    BaseType_t  os_ret = 0 ;
    u8* new_msg = 0;
    u16 recv_len = 0;
	u16 recv_len2 = 0;
    u8  cmd_recv[AT_RECV_MAX_LEN] = {0};

    /*参数检测*/
    if(NULL == this || NULL == data || NULL == recv)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*发送命令*/
    ret = my_uart.send(this->m_uart_id,data,strlen((char*)data));
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
        return E_ERROR;
    }

    /*收命令 会收到一条和发送的一样的命令*/
    memset(cmd_recv,0,AT_RECV_MAX_LEN);
    memcpy(cmd_recv,data,strlen((char*)data) - 2);
    ret = m_recv_find(this,(char*)cmd_recv,time);
    if(E_OK != ret)
    {
        log_error("Find failed.");
        return E_ERROR;
    }

    /*取出接收的数据*/
    ret = this->m_cut.get(&this->m_cut,&new_msg,&recv_len,time);
    if(E_OK != ret)
    {
        log_error("Recv data failed.");
        return E_ERROR;
    }
    memcpy(recv,new_msg,recv_len);
	
	ret = this->m_cut.get(&this->m_cut,&new_msg,&recv_len2,time);
    if(E_OK != ret)
    {
        log_error("Recv data failed.");
        return E_ERROR;
    }
	memcpy(recv + recv_len,new_msg,recv_len2);
	
	
    ret = this->m_cut.back(&this->m_cut,new_msg);
    if(E_OK != ret)
    {
        log_error("Back data failed.");
        return E_ERROR;
    }
    new_msg = NULL;

    /*最后还有一个ok*/
    ret = m_recv_find(this,"OK\r\n",1000);
    if(E_OK != ret)
    {
        log_error("Recv failed.");
        return E_ERROR;
    }   

    return E_OK;
}


static int m_get(const m_sim800* this,const u8* data,u8* recv,TickType_t time)
{
    int ret = 0;
    BaseType_t  os_ret = 0 ;
    u8* new_msg = 0;
    u16 recv_len = 0;
    u8  cmd_recv[AT_RECV_MAX_LEN] = {0};

    /*参数检测*/
    if(NULL == this || NULL == data || NULL == recv)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*发送命令*/
    ret = my_uart.send(this->m_uart_id,data,strlen((char*)data));
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
        return E_ERROR;
    }

    /*收命令 会收到一条和发送的一样的命令*/
    memset(cmd_recv,0,AT_RECV_MAX_LEN);
    memcpy(cmd_recv,data,strlen((char*)data) - 2);
    ret = m_recv_find(this,(char*)cmd_recv,time);
    if(E_OK != ret)
    {
        log_error("Find failed.");
        return E_ERROR;
    }

    /*取出接收的数据*/
    ret = this->m_cut.get(&this->m_cut,&new_msg,&recv_len,time);
    if(E_OK != ret)
    {
        log_error("Recv data failed.");
        return E_ERROR;
    }
    memcpy(recv,new_msg,recv_len);
    ret = this->m_cut.back(&this->m_cut,new_msg);
    if(E_OK != ret)
    {
        log_error("Back data failed.");
        return E_ERROR;
    }
    new_msg = NULL;
//    os_ret = xQueueReceive(this->m_buf,&new_msg,time);
//    if(pdTRUE != os_ret)
//    {
//        log_error("Recv cmd failed.");
//        return E_ERROR;
//    }
//    recv_len = *((u16*)new_msg);
//    memcpy(recv,new_msg + 2,recv_len);
//    vPortFree(new_msg);
//    new_msg = NULL;

    /*最后还有一个ok*/
    ret = m_recv_find(this,"OK\r\n",1000);
    if(E_OK != ret)
    {
        log_error("Recv failed.");
        return E_ERROR;
    }   

    return E_OK;
}

static int m_set(const m_sim800* this,const u8* data,TickType_t time)
{
    int ret = 0;
    BaseType_t  os_ret = 0 ;
    u8* new_msg = 0;
    u16 recv_len = 0;
    u8  cmd_recv[AT_RECV_MAX_LEN] = {0};

    /*参数检测*/
    if(NULL == this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*发送命令*/
    ret = my_uart.send(this->m_uart_id,data,strlen((char*)data));
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
        return E_ERROR;
    }

    /*收命令 会收到一条和发送的一样的命令*/
    memset(cmd_recv,0,AT_RECV_MAX_LEN);
    memcpy(cmd_recv,data,strlen((char*)data) - 2);
    ret = m_recv_find(this,(char*)cmd_recv,time);
    if(E_OK != ret)
    {
        log_error("Find failed.");
        return E_ERROR;
    }
    
    /*等待ok*/
    ret = m_recv_find(this,"OK\r\n",time);
    if(E_OK != ret)
    {
        log_error("Find failed.");
        return E_ERROR;
    }   

    return E_OK;

    error_handle:

    if(0 != new_msg)
    {
        vPortFree(new_msg);
        new_msg = NULL;
    }
    return E_ERROR;

}

static int m_send_message(const m_sim800* this,const char* num,const char* content)
{
    int ret = 0;
    BaseType_t  os_ret = 0 ;
    u8 recv_buf[AT_RECV_MAX_LEN] = {0};
    u8* new_msg = 0;
    u8 over = 0x1a;

    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*拼接号码*/
    snprintf((char*)recv_buf,AT_RECV_MAX_LEN,"AT+CMGS=\"%s\"\r\n",num);
    
    /*发送命令*/
    ret = my_uart.send(this->m_uart_id,recv_buf,strlen((char*)recv_buf));
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
        return E_ERROR;
    }
    
    /*收命令 会收到一条和发送的一样的命令*/
    snprintf((char*)recv_buf,AT_RECV_MAX_LEN,"AT+CMGS=\"%s\"",num);
    ret = m_recv_find(this,(char*)recv_buf,1000);
    if(E_OK != ret)
    {
        log_error("Recv failed.");
        return E_ERROR;
    }


    /*发送内容*/
    ret = my_uart.send(this->m_uart_id,(u8*)content,strlen((char*)content));
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
        return E_ERROR;
    }


    /*发送结束*/
    ret = my_uart.send(this->m_uart_id,&over,1);
    if(E_OK != ret)
    {
        log_error("Uart send failed.");
        return E_ERROR;
    }

    /*等待回应*/
    ret = m_recv_find(this,"+CMGS",15000);
    if(E_OK != ret)
    {
        log_error("Recv failed.");
        return E_ERROR;
    }

    /*等待ok*/
    ret = m_recv_find(this,"OK\r\n",1000);
    if(E_OK != ret)
    {
        log_error("Recv failed.");
        return E_ERROR;
    }    

    return E_OK;

    error_handle:

    if(0 != new_msg)
    {
        vPortFree(new_msg);
        new_msg = NULL;
    }
    return E_ERROR;
}

static int m_recv_find(const m_sim800* this,const char* str,TickType_t time)
{
    int ret = 0;
    BaseType_t  os_ret = 0 ;
    u8 recv_buf[AT_RECV_MAX_LEN + 1] = {0};
    u8* new_msg = 0;
    u16 recv_len = 0;

    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    do
    {
        /*等待回应*/
        ret = this->m_cut.get(&this->m_cut,&new_msg,&recv_len,time);
        if(E_OK == ret)
        {
            /*拷贝接收到的数据*/
            memset(recv_buf,0,AT_RECV_MAX_LEN + 1);
            memcpy(recv_buf,new_msg,recv_len);
            ret = this->m_cut.back(&this->m_cut,new_msg);
            if(E_OK != ret)
            {
                log_error("Back data failed.");
                return E_ERROR;
            }
            new_msg = NULL;
        
            if(NULL != strstr((char*)recv_buf,str))
            {
                return E_OK;
            }

        }
//        os_ret = xQueueReceive(this->m_buf,&new_msg,time);
//        if(pdTRUE == os_ret)
//        {
//            if(NULL != strstr((char*)new_msg + 2,str))
//            {
//                vPortFree(new_msg);
//                new_msg = NULL;
//                return E_OK;
//            }
//            vPortFree(new_msg);
//            new_msg = NULL;
//        }
    }while(E_OK == ret);

    //log_error("Recv error.");
        return E_ERROR;
}

#endif
