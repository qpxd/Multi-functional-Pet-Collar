/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : ec600.h
* Description: ec600驱动模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/25
* Refrence: https://blog.csdn.net/m0_47722349/article/details/124303495
*           https://blog.csdn.net/Mark_md/article/details/113429139?spm=1001.2101.3001.6650.11&utm_medium=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-11-113429139-blog-120675587.pc_relevant_default&depth_1-utm_source=distribute.pc_relevant.none-task-blog-2%7Edefault%7ECTRLIST%7ERate-11-113429139-blog-120675587.pc_relevant_default&utm_relevant_index=13
*****************************************************************************/

#ifndef __EC600_H__
#define __EC600_H__

#define EC_CMD_ATE0     "ATE0\r\n"      //关闭回显
#define EC_CMD_AT_TEST  "AT\r\n"
#define EC_CMD_CPIN     "AT+CPIN?\r\n"
#define EC_CMD_CSQ      "AT+CSQ\r\n"
#define EC_CMD_REG      "AT+CREG?\r\n"

#define EC_CMD_QMTCFG           "AT+QMTCFG=\"recv/mode\",0,0,1\r\n"
#define EC_CMD_MQTT_PARAM       "AT+QMTCFG=\"aliauth\",0,\"%s\",\"%s\",\"%s\"\r\n"

#define EC_CMD_MQTT_OPEN        "AT+QMTOPEN=0,\"iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883\r\n"         //打开MQTT网络
#define EC_CMD_MQTT_CONNECT     "AT+QMTCONN=0,\"clientExample\"\r\n"             //连接MQTT网络

#define EC_CMD_MQTT_PUB_SET     "AT+QMTSUB=0,1,\"/sys/%s/%s/thing/service/property/set\",0\r\n"
#define EC_CMD_MQTT_PUB_POST    "AT+QMTSUB=0,1,\"/sys/%s/%s/thing/event/property/post\",0\r\n"

#define EC_CMD_MQTT_PUBEX		"AT+QMTPUBEX=0,0,0,0,\"/sys/%s/%s/thing/event/property/post\",%d\r\n"
#define EC_CMD_MQTT_PUBEX_JOSN	"{\"method\":\"thing.event.property.post\",\"id\":\"1234\",\"params\":{%s},\"version\":\"1.0.0\"}\r\n"
/*状态机*/
typedef enum
{
    EC_STA_UNINITED = 0,        //未初始化状态
    EC_STA_RDY,                 //上电状态 
    EC_STA_CSQ,                 //信号强度正常
    EC_STA_CREG,                //数据网络注册成功
    EC_STA_MQTT_CONNECT = 11,        //Mqtt 连接成功
}ec600_sta_t;

typedef struct __C_EC600 c_ec600;
typedef struct __C_EC600
{
    void* this;
    int(*state)  (const c_ec600* this);
	/************************************************* 
	* Function: mqtt_start
	* Description: 启动mqtt服务 (暂未启用)
	* Demo  :
	*         ret = ec600.mqtt_start(&ec600, "i7ysw32S3D3",                         //ProductKey
    *                                        "EC600N",                              //DeviceName
    *                                        "e9526940bf209a19d5a44748b8b4408d");   //DeviceSecret
	*         if(ret != E_OK)    log_error("ec600 mqtt start failed.");
	*************************************************/
    int (*mqtt_start)(const c_ec600* this, const char *key, const char *dev_name, const char *dev_secret);
    /************************************************* 
	* Function: ec600 mqtt post 上报数据
	* Description: 创建 ec600 对象
	* Demo  :
		// if(ec600.state(&ec600) == EC_STA_MQTT_CONNECT)
		// {
		// 	ret = ec600.mqtt_send(&ec600, "\"temp\":15");  
		// 	if(ret != E_OK)    log_error("ec600 mqtt send failed, queue full.");
		// 	ret = ec600.mqtt_send(&ec600, "\"humi\":80");  
		// 	if(ret != E_OK)    log_error("ec600 mqtt send failed, queue full.");
		// }
	*************************************************/
    int (*mqtt_send)(const c_ec600* this, const char *data);
    /************************************************* 
	* Function: ec600 设置 mqtt 接收回调函数 (暂未启用)
	* Description: 创建 ec600 对象
	* Demo  :
	*         ret = ec600.set_callback(&ec600, "temp", "21.5");  
	*         if(ret != E_OK)    log_error("ec600 mqtt start failed.");
	*************************************************/
    int (*set_callback)     (const c_ec600* this, void* param, int (*callback)(void* param, const u8* data, u16 data_len));
}c_ec600;

/************************************************* 
* Function: ec600_creat 
* Description: 创建 ec600 对象  (创建了，获取状态后 正确后可以直接发送)
* Demo  :
	// c_ec600 ec600 = ec600_creat(MY_UART_1,
	// 							"i7ysw32S3D3",
	// 							"EC600N",
	// 							"e9526940bf209a19d5a44748b8b4408d");
	// if(NULL == ec600.this)	log_error("ec600 creat failed.");
*************************************************/
c_ec600 ec600_creat(u8 uart_id, const char *key, const char *dev_name, const char *dev_secret);

#endif

/*

单条指令上报
{"method":"thing.event.property.post","id":"1234","params":{"temp":20},"version":"1.0.0"}
多条指令上报
{"method":"thing.event.property.post","id":"1234","params":{"temp":20},"version":"1.0.0"}

*/

