/*****************************************************************************
* Copyright:
* File name: my_interface.c
* Description: 界面应用 函数实现
* Author: 许少
* Version: V1.0
* Date: 2021/12/19
*****************************************************************************/
#include "app_conf.h"

#ifdef APP_MY_INTERFACE_ENABLED

#define MODULE_NAME       "my_interface"

#define MESSAGE_TYPE_TEXT  0   /*文本消息类型*/
#define MESSAGE_TYPE_CLEAN 1   /*清屏消息类型*/

#define LINE_STR_MAX_LEN  16   /*一行最大的字符串*/
#define STR_BUF_SIZE      20   /*最大字符串缓存*/
#define MESG_QUEUE_LEN     3   /*消息队列长度*/
#define SCAN_TIME_MS     250   /*屏幕刷新时间*/
#define LINE_MAX           3   /*最大行号*/
#define CHAR_SIZE         16   /*字符大小*/

#ifdef  LOG_TAGE
#undef  LOG_TAGE
#endif
#define LOG_TAGE          MODULE_NAME

/*消息结构*/
typedef struct __INTERFACE_MESSAGE_STRUCT
{
	u8   m_type                 ;  /*消息类型  */
	u8   m_line_no              ;  /*行号      */
	char m_str_buf[STR_BUF_SIZE];  /*字符缓存  */
}int_message_typedef;

static TaskHandle_t  g_task_handle          ;   /*任务句柄*/
static bool          g_init_state           ;   /*初始化状态*/
static QueueHandle_t g_interface_message    ;   /*界面更新消息队列*/
        int   my_interface_init(void)       ;
static void   m_task           (void* pdata);

/*************************************************
* Function: 界面应用初始化
* Description: my_interface_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int my_interface_init(void)
{
	BaseType_t os_ret;
	
	/*如果已经初始化 就直接返回初始化成功*/
	if(g_init_state)
	{
		return E_OK;
	}
	
	/*创建消息队列*/
	if(NULL == g_interface_message)
	{
		g_interface_message = xQueueCreate(MESG_QUEUE_LEN,sizeof(int_message_typedef));
		if(NULL == g_interface_message)
		{
			log_error("Queue creat filed.");
			return E_ERROR;
		}
	}
	
	
	/*创建界面任务*/
	if(NULL == g_task_handle)
	{
		os_ret = xTaskCreate((TaskFunction_t )m_task,     	
                             (const char*    )MODULE_NAME,   	
                             (uint16_t       )INTERFACE_TASK_STK, 
                             (void*          )NULL,				
                             (UBaseType_t    )INTERFACE_TASK_PRO,	
                             (TaskHandle_t*  )&g_task_handle); 
							 
		if(pdPASS != os_ret)
		{
			log_error("Task init filed,ret=%d",(int)os_ret);
			return E_ERROR;
		}
	} 
	
	g_init_state = true;
	
	return E_OK;
}

static void   m_task(void* pdata)
{
	BaseType_t          os_ret       ;
	int_message_typedef param   = {0};
	
	//OLED_Clear();  /*默认清除屏幕*/
	
	while(1)
	{
		memset(&param,0,sizeof(param));
		
		/*死等  取消息*/
		os_ret = xQueueReceive(g_interface_message,&param,portMAX_DELAY);
		if(pdTRUE != os_ret)
		{
			log_error("Failed to accept message ");
			continue;
		}
		
		/*取到消息后，根据消息类型 进行相应的操作*/
		
		switch(param.m_type)
		{
			/*文本消息*/
			case MESSAGE_TYPE_TEXT:
			{
				//OLED_ShowString(0,param.m_line_no * 2,(u8*)param.m_str_buf,CHAR_SIZE);
				break;
			}
			
			/*清屏消息*/
			case MESSAGE_TYPE_CLEAN:
			{
				//OLED_Clear();
				break;
			}
			
			default:
			{
				log_error("Error message type.type=%d.",param.m_type);
				continue;
			}
		}
		
		//vTaskDelay(SCAN_TIME_MS);
	}
}

/*************************************************
* Function: 界面应用初始化
* Description: my_interface_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int my_interface_display(u8 line,char* format,...)
{
	int                 str_len = 0  ;
	va_list             argptr       ;
	BaseType_t          os_ret       ;
	int_message_typedef param   = {0};
	
	/*检查初始化状态*/
	if(!g_init_state)
	{
		log_error("Need init first.");
		return E_ERROR;
	}
	
	/*检查行号*/
	if(line > LINE_MAX)
	{
		log_error("Line no is too big.");
		return E_ERROR;
	}
	
	va_start( argptr, format );  // 初始化argptr	
	memset(param.m_str_buf,0,STR_BUF_SIZE);
	vsnprintf(param.m_str_buf,STR_BUF_SIZE,format,argptr);
	va_end(argptr);
	
	/*校验字符串长度*/
	str_len = strlen(param.m_str_buf);
	if(str_len > LINE_STR_MAX_LEN)
	{
		log_error("String is too long.");
		return E_PARAM;
	}
	
	
	/*将字符串压入消息队列中 立即返回*/
	param.m_line_no = line             ;   /*传递行号*/
	param.m_type    = MESSAGE_TYPE_TEXT;   /*文本类型*/
	os_ret = xQueueSend(g_interface_message,&param,0);
	if(pdTRUE != os_ret)
	{
		//log_error("Send message filed.");
		return E_ERROR;
	}
	
	
	//OLED_ShowString(0,0,g_str_buf,16);
	
	return E_OK;
}
#endif
