/*****************************************************************************
* Copyright:
* File name: my_app.c
* Description: 我的应用 函数实现
* Author: 许少
* Version: V1.0
* Date: 2021/12/20
*****************************************************************************/
#include "app_conf.h"

#ifdef APP_DIS_ENABLED

#define MODULE_NAME       "dis detection"
#define SCAN_TIME_MS      300

#ifdef  LOG_TAGE
#undef  LOG_TAGE
#endif
#define LOG_TAGE          MODULE_NAME

static bool              g_init_state  ;
static TaskHandle_t      g_task_handle ;  /*任务句柄*/
static float             g_dis         ;

static void m_task(void* param)
{
	
	while(1)
	{
		/*重复执行逻辑 从此处开始*/
		//g_dis = Hcsr04GetLength();
		
		vTaskDelay(SCAN_TIME_MS);
	}
}

/*************************************************
* Function: 我的app初始化
* Description: my_app_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int dis_detection_init(void)
{
	BaseType_t os_ret;
	
	if(g_init_state)
	{
		return E_OK;
	}
	
	/*创建app任务*/
	if(NULL == g_task_handle)
	{
		os_ret = xTaskCreate((TaskFunction_t )m_task,     	
                             (const char*    )MODULE_NAME,   	
                             (uint16_t       )DIS_DETECTION_STK, 
                             (void*          )NULL,				
                             (UBaseType_t    )DIS_DETECTION_PRO,	
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

/*************************************************
* Function: 获取当前距离
* Description: dis_detection_get
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int dis_detection_get(float* dis)
{
	if(!g_init_state)
	{
		log_error("Need init first.");
		return E_OK;
	}
	
	if(NULL == dis)
	{
		log_error("Param error.");
		return E_ERROR;
	}
	
	*dis = g_dis;
	
	return E_OK;
}

#endif
