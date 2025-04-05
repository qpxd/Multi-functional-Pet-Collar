/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : esp8266.h
* Description: esp8266驱动模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/25
* Refrence
****************************************************************************/

#ifndef __ESP8266_H__
#define __ESP8266_H__

#define	OK_ACK			"OK\r\n"

#define ALI_CLOUD_POST	"/sys/i9ovVM4Uo4A/WATER/thing/event/property/post"		//注意这里要改产品id
#define ALI_CLOUD_SET	"/sys/i9ovVM4Uo4A/WATER/thing/service/property/set"

#define ESP_CMD_RST     	"AT+RST\r\n"      	//复位
#define ESP_CMD_MODE  		"AT+CWMODE=2\r\n"	//2 AP模式
#define ESP_CMD_CIPMODE  	"AT+CIPMODE=0\r\n"							//2
#define ESP_CMD_CIPMUX  	"AT+CIPMUX=1\r\n"	//设置为多连接模式
#define ESP_CMD_AP_PARAM  	"AT+CWSAP=\"gwen9\",\"12345678\",1,3,1,0\r\n"	//2 AP模式 WIFI账号，密码

#define ESP_CMD_AP_IP		 "AT+CIPAP=\"192.168.0.1\",\"192.168.0.1\",\"255.255.255.0\"\r\n"	
#define ESP_CMD_SERVER_START "AT+CIPSERVER=1,8080\r\n"//2 AP模式 IP地址

#define ESP_CMD_SERVER_SEND 	"AT+CIPSEND=0,%d\r\n"
#define	SEND_READY_OK			"OK\r\n\r\n>"


//ALI CLOUD

/*状态机*/
typedef enum
{
    ESP_STA_UNINITED = 0,        //未初始化状态
    ESP_STA_RDY,                 //上电状态 
    ESP_STA_CSQ,                 //信号强度正常
    ESP_STA_SERVER_DISCONN = 0xf0,                //数据网络注册成功
    ESP_STA_SERVER_CONNECT = 0xf1,        //Mqtt 连接成功
}esp_sta_t;

typedef struct __c_esp c_esp;
typedef struct __c_esp
{
    void* this;
    int(*state)  (const c_esp* this);

	int (*sta_start)(const c_esp* this, const char *key, const char *dev_name, const char *dev_secret);
    /************************************************* 
	* Function: ec600 mqtt post 上报数据
	* Description: 创建 ec600 对象
	* Demo  :
	
		ret = esp8266.server_send(&esp8266, "dsadsdasd");  
		if(ret != E_OK)    log_error("esp8266 mqtt send failed, queue full.");

	*************************************************/
    int (*server_send)(const c_esp* this, const char *data);
    /************************************************* 
	* Function: ec600 设置 mqtt 接收回调函数 (暂未启用)
	* Description: 创建 ec600 对象
	* Demo  :
			int esp8266_server_callback (const u8* data, u16 data_len)
			{
				log_inform("server recive %d: %s", data_len, data);
			}
			ret = esp8266.set_callback(&esp8266, callback);  
			if(ret != E_OK)    log_error("esp8266 set callback failed.");
	*************************************************/
    int (*set_callback)     (const c_esp* this, int (*callback)(const u8* data, u16 data_len));
}c_esp;

/************************************************* 
* Function: ec600_creat 
* Description: 创建 ec600 对象  (创建了，获取状态后 正确后可以直接发送)
* Demo  :
		c_esp esp8266_creat(MY_UART_1);
		if(NULL == esp8266.this)	log_error("esp8266 creat failed.");
*************************************************/
c_esp esp8266_creat(u8 uart_id);

#endif


