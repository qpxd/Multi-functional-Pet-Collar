/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : esp8266.h
* Description: esp8266驱动模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/25
* Refrence

// AT+CWMODE=1
// AT+CWJAP="gwen9","12345678"

// /*STEP1: 配置连接参数*/
// AT+MQTTUSERCFG=0,1,"NULL","CAR_IN&a1twzbwXv54","3ce2c81dd54d1c3516d8fb7792c50767fbc04e49045ecf9be99e2915ef953b08",0,0,""

// /*STEP2: 配置连接ID */
// //clientId第二个参数注意每个逗号后加分隔符“\”，已踩过坑，
// //例如：t|securemode=3\,signmethod=hmacsha1\,
// AT+MQTTCLIENTID=0,"a1twzbwXv54.CAR_IN|securemode=2\,signmethod=hmacsha256\,timestamp=1677941302390|"

// /*STEP3: 连接阿里云 成功后会在线 */
// AT+MQTTCONN=0,"a1twzbwXv54.iot-as-mqtt.cn-shanghai.aliyuncs.com",1883,1

// /*STEP4: 订阅主题 */
// AT+MQTTSUB=0,"/sys/a1twzbwXv54/CAR_IN/thing/event/property/post",1		//订阅的主题可在云端设备的“自定义Topic列表”复制进去
// AT+MQTTSUB=0,"/sys/a1twzbwXv54/CAR_IN/thing/service/property/set",1

// /*STEP4: 发布消息 */
// AT+MQTTPUB=0,"/sys/a1twzbwXv54/CAR_IN/thing/event/property/post","{\"method\":\"thing.event.property.post\"\,\"id\":\"1234\"\,\"params\":{\"temp\":24}\,\"version\":\"1.0.0\"}",1,0
/*****************************************************************************/
//i9ovVM4Uo4A.door|securemode=2,signmethod=hmacsha256,timestamp=1678181400782|

#ifndef __ESP8266_H__
#define __ESP8266_H__

#define	OK_ACK			"OK\r\n"

#define ALI_CLOUD_POST	"/sys/i9ovVM4Uo4A/WATER/thing/event/property/post"		//注意这里要改产品id
#define ALI_CLOUD_SET	"/sys/i9ovVM4Uo4A/WATER/thing/service/property/set"

#define ESP_CMD_RST     "AT+RST\r\n"      	//复位
#define ESP_CMD_MODE  	"AT+CWMODE=1\r\n"

// AT+CWJAP="gwen9","12345678"
//sta 
#define ESP_CMD_QAP 		"AT+CWQAP\r\n"
#define ESP_CMD_JAP  		"AT+CWJAP=\"gwen9\",\"12345678\"\r\n"
//MQTT a11oQdNV919.WATER|securemode=2,signmethod=hmacsha256,timestamp=1678008992524|
#define ESP_CMD_MQTT_PARAM 		"AT+MQTTUSERCFG=0,1,\"NULL\",\"%s&%s\",\"%s\",0,0,\"\"\r\n"
#define ESP_CMD_MQTT_CLIENTID 	"AT+MQTTCLIENTID=0,\"%s.%s|securemode=2\\\,signmethod=hmacsha256\\\,timestamp=1678181400782|\"\r\n"	 //注意这里要改时间戳 在MQTT连接参数里面
#define ESP_CMD_MQTT_CON 		"AT+MQTTCONN=0,\"%s.iot-as-mqtt.cn-shanghai.aliyuncs.com\",1883,1\r\n"
//								 AT+MQTTCONN=0,"a1twzbwXv54.iot-as-mqtt.cn-shanghai.aliyuncs.com",1883,1
#define ESP_CMD_MQTT_DISCON 	"AT+MQTTCLEAN=0\r\n"
//订阅主题
#define ESP_CMD_MQTT_SUB 		"AT+MQTTSUB=0,\"%s\",1\r\n"
//发布消息
#define ESP_CMD_MQTT_PUB 		"AT+MQTTPUB=0,\"%s\",\"{\\\"method\\\":\\\"thing.event.property.post\\\"\\\,\\\"id\\\":\\\"1234\\\"\\\,\\\"params\\\":{%s}\\\,\\\"version\\\":\\\"1.0.0\\\"}\",1,0\r\n"


//ALI CLOUD

/*状态机*/
typedef enum
{
    ESP_STA_UNINITED = 0,        //未初始化状态
    ESP_STA_RDY,                 //上电状态 
    ESP_STA_CSQ,                 //信号强度正常
    ESP_STA_CREG,                //数据网络注册成功
    ESP_STA_MQTT_CONNECT = 11,        //Mqtt 连接成功
}esp_sta_t;

typedef struct __c_esp c_esp;
typedef struct __c_esp
{
    void* this;
    int(*state)  (const c_esp* this);

	int (*sta_start)(const c_esp* this, const char *key, const char *dev_name, const char *dev_secret);

	/************************************************* 
	* Function: mqtt_start
	* Description: 启动mqtt服务 (暂未启用)
	* Demo  :
	*         ret = ec600.mqtt_start(&ec600, "i7ysw32S3D3",                         //ProductKey
    *                                        "EC600N",                              //DeviceName
    *                                        "e9526940bf209a19d5a44748b8b4408d");   //DeviceSecret
	*         if(ret != E_OK)    log_error("ec600 mqtt start failed.");
	*************************************************/
    int (*mqtt_start)(const c_esp* this, const char *key, const char *dev_name, const char *dev_secret);
    /************************************************* 
	* Function: ec600 mqtt post 上报数据
	* Description: 创建 ec600 对象
	* Demo  :
		// if(esp8266.state(&esp8266) == ESP_STA_MQTT_CONNECT)
		// {
		// 	ret = esp8266.mqtt_send(&ec600, "\"temp\":15");  
		// 	if(ret != E_OK)    log_error("ec600 mqtt send failed, queue full.");
		// }
	*************************************************/
    int (*mqtt_send)(const c_esp* this, const char *data);
    /************************************************* 
	* Function: ec600 设置 mqtt 接收回调函数 (暂未启用)
	* Description: 创建 ec600 对象
	* Demo  :
	*         ret = ec600.set_callback(&ec600, "temp", "21.5");  
	*         if(ret != E_OK)    log_error("ec600 mqtt start failed.");
	*************************************************/
    int (*set_callback)     (const c_esp* this, void* param, int (*callback)(void* param, const u8* data, u16 data_len));
}c_esp;

/************************************************* 
* Function: ec600_creat 
* Description: 创建 ec600 对象  (创建了，获取状态后 正确后可以直接发送)
* Demo  :
	// c_esp esp8266 = esp8266_creat(MY_UART_1,
	// 							"i9ovVM4Uo4A",		//产品ID
// 								"door",				//设备名
	// 							"ad70500f9b9630c8c61674013a201d926e3b91b9affb8c09c12dc2753069661c");	//密码 需要在MQTT连接参数里面找
	// if(NULL == esp8266.this)	log_error("esp8266 creat failed.");
*************************************************/
c_esp esp8266_creat(u8 uart_id, const char *key, const char *dev_name, const char *dev_secret);

#endif

/*

单条指令上报
{"method":"thing.event.property.post","id":"1234","params":{"temp":20},"version":"1.0.0"}
多条指令上报
{"method":"thing.event.property.post","id":"1234","params":{"temp":20},"version":"1.0.0"}

*/

