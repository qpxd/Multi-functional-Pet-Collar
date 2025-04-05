/**
  ******************************************************************************
  * @file           : moudle.c
  * @brief          : 毕设PCB模块测试组件
  * @note: 
  *******************************************************************************/
#include "app_conf.h"
  
#ifdef APP_MOUDLE_ENABLED

/* Private includes ----------------------------------------------------------*/
/* Private function declare -----------------------------------------------*/
void prvMTLogOut(log_type_t xType, bool xIfNewLine, char *format, ...);
void prvMTLogReset(log_type_t xType, uint8_t xLine);
void prvP0_ALL_TEST(void);
void prvP1_PWM_ADC_TEST(void);
void prvP2_UART_TEST(void);
void prvP3_IIC_TEST(void);
void prvP4_SPI_TEST(void);

/* Macros -----------------------------------------------------------------*/
#define LOG_TYPE                    LOG_OLED
#define POINTER_CHECK(n)    		do{\
                                        if(NULL == n)\
                                        {\
                                            log_error("null pointer !!!");\
                                            vTaskDelete(NULL);\
                                        }\
                                    }while(0)

#define POINTER_CHECK_LOG(n)    	do{\
                                        if(NULL == n){\
                                            prvMTLogOut(LOG_TYPE, true, "null pointer !!!");\
											prvMTLogOut(LOG_TYPE, true, "check failed !!!");\
                                            vTaskDelete(NULL);\
                                        }\
                                    }while(0)

#define RETURN_CHECK_LOG(n)    	do{\
                                        if(E_OK != n){\
											prvMTLogOut(LOG_TYPE, true, "check failed !!!");\
                                            vTaskDelete(NULL);\
                                        }\
                                    }while(0)

#define LOG_OUT(m, n, ...)		prvMTLogOut(LOG_TYPE, m, n, ##__VA_ARGS__)			
#define LOG_CLEAR()		        prvMTLogReset(LOG_TYPE, 0)				
/* Variables -----------------------------------------------------------------*/
static c_esp8266    esp8266 = {0};
float weight = 0.0f;
const int s_thePrice[4] = {16, 10, 27, 20};
static char esp_rec[64];
static char esp_send[64];


/* Public interface ----------------------------------------------------------*/
int esp8266_event(void* param,u32 event,const char* str)
{
    log_inform("esp8266_event:%s.",str);
	/*设置宠物进食时间*/
	
    return E_OK;
}

int esp8266_server_recv(void* param,const u8* data,u16 data_len)
{
    log_inform("Len:%d,data:%.*s.",data_len,data_len,data);
	
//	int ret = 0;
//	char *localptr = NULL;
//	
//	memset(esp_rec, 0, sizeof(esp_rec));
//	if(data_len < 64)	memcpy(esp_rec, data, data_len);
//	else				return E_ERROR;
//	localptr = strtok(esp_rec, "="); if(localptr == NULL) return E_ERROR;

//	if(strstr(data, "AT+TIME") != NULL){
////		localptr = strtok(NULL, ","); if(localptr == NULL) return E_ERROR;
////		feed_time.o_house =  atoi(localptr);
////		localptr = strtok(NULL, ","); if(localptr == NULL) return E_ERROR;
////		feed_time.o_min	 =  atoi(localptr);
////		localptr = strtok(NULL, ","); if(localptr == NULL) return E_ERROR;
////		feed_time.o_sec	 =  atoi(localptr);
//	}else if(strstr(data, "AT+MTIME") != NULL){

//	}
    return E_OK;
}

const static uint8_t rfid_sn[5][4] = 
{
	{0xd0, 0xc6, 0xae, 0x09},	//苹果
	{0x5c, 0xbc, 0x29, 0xe9},	//橘子
	{0x5c, 0xed, 0x2f, 0x88},	//栗子
	{0xf2, 0xdd, 0x45, 0x49},	//空白
    {0xb2, 0xe3, 0xa6, 0x49},	//空白
};

static char send_message[64] = {0}; 
int rfid_callback(void* param,u8 event,const u8* data)
{
	if(RC522_FIND == event)
	{
		log_inform("Get card:%#X,%#X,%#X,%#X.",data[0],data[1],data[2],data[3]);
		weight = weight < 0 ? 0 : weight;
		if((data[0] == rfid_sn[0][0]) && (data[1] == rfid_sn[0][1]) &&  
			(data[2] == rfid_sn[0][2]) && (data[3] == rfid_sn[0][3]) )
		{
			sprintf(esp_send, "AT+DATA=%d,%d,%d,OK\r\n", 1, (int)(weight/5), s_thePrice[0]);
		}else if((data[0] == rfid_sn[1][0]) && (data[1] == rfid_sn[1][1]) &&  
			(data[2] == rfid_sn[1][2]) && (data[3] == rfid_sn[1][3]) )
		{
			sprintf(esp_send, "AT+DATA=%d,%d,%d,OK\r\n", 2, (int)(weight/5), s_thePrice[1]);
		}else if((data[0] == rfid_sn[2][0]) && (data[1] == rfid_sn[2][1]) &&  
			(data[2] == rfid_sn[2][2]) && (data[3] == rfid_sn[2][3]) )
		{
			sprintf(esp_send, "AT+DATA=%d,%d,%d,OK\r\n", 3, (int)(weight/5), s_thePrice[2]);
		}else if((data[0] == rfid_sn[3][0]) && (data[1] == rfid_sn[3][1]) &&  
			(data[2] == rfid_sn[3][2]) && (data[3] == rfid_sn[3][3]) )
		{
			sprintf(esp_send, "AT+BLANK=%d,OK\r\n", (int)(weight/5));
		}else if((data[0] == rfid_sn[4][0]) && (data[1] == rfid_sn[4][1]) &&  
			(data[2] == rfid_sn[4][2]) && (data[3] == rfid_sn[4][3]) )
		{
			sprintf(esp_send, "AT+FIVE=0,OK\r\n");
		}
		esp8266.server_send(&esp8266, esp_send, strlen(esp_send));	
	}
	else if(RC522_LEAVE == event)
	{
		log_inform("Car leved.");
	}

	return E_OK;
}

void moudle_test_task(void* param)
{
	int ret;
	vTaskDelay(500);
	/*创建OLED对象*/
	c_my_iic iic  = iic_creat(GPIOB, GPIO_PIN_6, GPIOB, GPIO_PIN_7);
	c_oled oled   = oled_create(&iic, OLED_TYPE_96);
	if(NULL == oled.this)
	{
		log_error("oled creat failed.");
	}
	oled.display_clear(&oled);
	if(E_OK != ret)
	{
		log_error("Oled clear failed.");
	}
	oled.display_str(&oled, 0, 0, "Calibrating..."); 
	
	/*创建hx711对象*/
    c_hx711 hx711 = {0};
    hx711 = hx711_create(GPIOA, GPIO_PIN_0, GPIOA, GPIO_PIN_1);
	if(NULL == hx711.this)
	{
		log_error("hx711 creat failed.");
	}
	
	ret = hx711.zero(&hx711);
	if(E_OK != ret)
	{
		log_error("hx711 get zero failed.");
		oled.display_str(&oled, 0, 1, "Calibrate fail!"); 
		oled.display_str(&oled, 0, 2, "Please restart!"); 
		vTaskDelay(3000);
	}
	else
	{
		oled.display_str(&oled, 0, 1, "Calibrating OK !"); 
	}
	
	ret = hx711.get(&hx711,&weight);
	if(E_OK != ret)
	{
		log_error("hx711 get failed.");
	}

	
	/*创建ESP8266对象*/
	ap_inform    ap = {"2022-019","12345678","192.168.0.1","192.168.0.1","255.255.255.0"};
	esp8266 = esp8266_create(MY_UART_1);
	if(NULL == esp8266.this)
	{
		log_error("Esp8266 create failed.");
	}
	/*配置wifi*/
	ret = esp8266.wifi_cfg(&esp8266,ESP8266_WIFI_AP,NULL,&ap);
	if(E_OK != ret)
	{
		log_error("Wifi cfg failed.");
	}
	/*启动服务器*/
	ret = esp8266.server_start(&esp8266,8080);
	if(E_OK != ret)
	{
		log_error("Server start failed.");
	}
	/*设置服务器回调*/
	ret = esp8266.server_callback(&esp8266,NULL, esp8266_server_recv);
	if(E_OK != ret)
	{
		log_error("Callback set failed.");
	}
	/*设置事件回调*/
	ret = esp8266.event_callback(&esp8266,NULL, esp8266_event);
	if(E_OK != ret)
	{
		log_error("Callback set failed.");
	}
	
	/*创建RFID对象*/
	rc522_cfg cfg = {GPIOC,GPIO_PIN_13,	//RST
					 GPIOB,GPIO_PIN_14,	//MISO
					 GPIOB,GPIO_PIN_15,	//MOSI
					 GPIOB,GPIO_PIN_13,	//SCK
					 GPIOA,GPIO_PIN_8};	//CS
	ret = rc522.init(&cfg);
	if(E_OK != ret )
	{
		log_error("rc522 init failed.");
	}
    ret = rc522.set_callback(NULL, rfid_callback);
	if(E_OK != ret )
	{
		log_error("rc522 set callback failed.");
	}

	vTaskDelay(3000);
	oled.display_clear(&oled);
	oled.display_str(&oled, 0, 0, "IP:192.168.0.1");
    while (1)
    {    
		ret = hx711.get(&hx711,&weight);
		if(E_OK != ret)
		{
			log_error("hx711 get failed.");
		}
		oled.display_str(&oled, 0, 1, "Weight:%0.3fkg", weight/1000/5); 
        vTaskDelay(500);
    }
}

/* Private function define -----------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
#endif