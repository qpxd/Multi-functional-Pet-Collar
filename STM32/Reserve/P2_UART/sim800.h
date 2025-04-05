/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : sim800.h
* Description: sim800驱动模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/25
*****************************************************************************/

#ifndef __SIM800_H__
#define __SIM800_H__



#define SIM800_BAUDRATE  9600

#define SIM800_NUM_LEN       11
#define SIM800_MSG_MAX_LEN   60

typedef struct __C_SIM800 c_sim800;

typedef struct __C_SIM800
{
    void* this;

    int(*state)  (const c_sim800* this);
	
	/************************************************* 
	* Function: sim800发送短信 
	* Description: 创建 sim800 对象
	* Demo  :
	*         c_sim800 sim800 = sim800_creat(MY_UART_2);
	*         if(NULL == sim800.this)
	*         {
	*             log_error("sim800 creat failed.");
	*         }
	*************************************************/
    int(*message)(const c_sim800* this,const char* num,const char* content);
	
	/************************************************* 
	* Function: message_del
	* Description: sim800删除所有短信
	* Demo  :
	*         ret = sim800.message_del(&sim800);
	*         if(NULL == sim800.this)
	*         {
	*             log_error("sim800 delete message failed.");
	*         }
	*************************************************/
	int(*message_del)(const c_sim800* this);				
	
	/************************************************* 
	* Function: message_cnt
	* Description: sim800获取当前短信数量
	* Demo  :
	*		  uint8_t message_cnt;
	*         ret = sim800.message_cnt(&sim800, &message_cnt);
	*         if(NULL == sim800.this)
	*         {
	*             log_error("sim800 message cnt failed.");
	*         }
	*************************************************/
	int(*message_cnt)(const c_sim800* this, unsigned char *count);			/*获取当前短信数目*/
	
	/************************************************* 
	* Function: 
	* Description: sim800获取指定短信的号码和内容
	* Input : <index> 	索引 第几条短信
	*         <num>   	电话号码 字符串带双引号，注意用于接收的buf必须大于16个字节
	*         <content> 开关所在pin脚
	* Demo  :
	*		  char num[20];
	*		  char content[32];
	*		  memset(num, 0, sizeof(num));  
	*		  memset(content, 0, sizeof(content));  
	*         ret = sim800.message_content(&sim800, 1, num, content);
	*         if(NULL == sim800.this)
	*         {
	*             log_error("message content get failed.");
	*         }
	*************************************************/
	int(*message_content)(const c_sim800* this, unsigned char index, char *num, char *content);	/*获取端信内容*/
}c_sim800;

/************************************************* 
* Function: sim800_creat 
* Description: 创建 sim800 对象
* Demo  :
*         c_sim800 sim800 = sim800_creat(MY_UART_2);
*         if(NULL == sim800.this)
*         {
*             log_error("sim800 creat failed.");
*         }
*************************************************/
c_sim800 sim800_creat(u8 uart_id);

#endif

