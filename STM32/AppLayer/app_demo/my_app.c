/*****************************************************************************
* Copyright:
* File name: my_app.c
* Description: 我的应用 函数实现
* Author: 许少
* Version: V1.0
* Date: 2021/12/20
*****************************************************************************/
#include "app_conf.h"

#ifdef APP_MY_APP_ENABLED
#define MODULE_NAME       "my app"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

static TaskHandle_t      g_app1_handle ;  		 /*任务句柄*/
static TaskHandle_t      g_app2_handle ;  		 /*任务句柄*/
static TaskHandle_t      g_app3_handle ;  		 /*任务句柄*/
static TaskHandle_t      g_moudle_test_handle ;  /*任务句柄*/
int ret = 0;
/********************************************此处写入逻辑部分***************************************************/
/*OLED 显示*/
static uint8_t oled_show[32];
static uint8_t tx_buf[32];
static nmea_gga data = {0};
static float temp = 0.0f;	//温度
static float x, y, z;   
static float voice;	
static uint32_t step;
float dis = 0.0f;
static uint8_t time[3] = {22, 30, 30};
static uint8_t sys_cnt;
static uint8_t page;
static uint8_t led_flag;
static uint8_t step_flag;
static uint8_t xu_cong_flag;
static uint8_t unlock_flag;
static uint8_t cat_flag;
/*键盘创建*/
int key_board_call(const char* key_name,u8 state)
{
	if(strstr(key_name, "L1") != NULL &&  state == 0){
		xu_cong_flag = !xu_cong_flag;
	}
	
	if(strstr(key_name, "L2") != NULL &&  state == 0){
		page = !page;
		OLED_Clear();
	}	
	
	if(strstr(key_name, "L3") != NULL){
		if(state == 0)		cat_flag = 0;
		else				cat_flag = 1;
	}
		
	log_inform("KEY:%s,state:%d",key_name,state);
	return E_OK;
}

/*服务器接收回调函数*/
int esp8266_server_callback (const u8* data, u16 data_len)
{
	static char rec_buf[64];
	if(data_len < 64){
		memset(rec_buf, 0, sizeof(rec_buf));
		memcpy(rec_buf, data, data_len);
		
	}else{
		log_inform("Len:%d, Error", data_len);
	}	
	
	char *pxIndex;
	if(strstr(rec_buf, "\r\n") != NULL){
		if(strstr(rec_buf, "AT+TIME") != NULL){
			pxIndex = strtok(rec_buf, "=");
			if(pxIndex == NULL)	return E_OK;
			
			pxIndex = strtok(NULL, ":");
			if(pxIndex == NULL)	return E_OK;
			time[0] = atoi(pxIndex);
			
			pxIndex = strtok(NULL, ":");
			if(pxIndex == NULL)	return E_OK;
			time[1] = atoi(pxIndex);
			
			pxIndex = strtok(NULL, ",");
			if(pxIndex == NULL)	return E_OK;
			time[2] = atoi(pxIndex);
		}
		
		if(strstr(rec_buf, "AT+DATA1") != NULL){
			xu_cong_flag = 1;
		}
		
		if(strstr(rec_buf, "AT+DATA2") != NULL){
			xu_cong_flag = 0;
		}
		
		if(strstr(rec_buf, "AT+DATA3") != NULL){
			unlock_flag = 1;
		}
	}
    return E_OK;
	
	log_inform("server recive %d: %s", data_len, data);
}

#define APP_1_UPDATE_TIME_MS  1000
static void my_app_task_1(void* param)
{  
	/*DS18B20初始化*/
	c_ds18b20 ds18b20 = {0};
	ds18b20 = ds18b20_create(GPIOC,GPIO_PIN_13);
	if(NULL == ds18b20.this)	log_error("ds18b20 creat failed.");
	/*GPS*/
	c_gps gps = {0};
	gps = gps_creat(MY_UART_2);
	if(NULL == gps.this)	log_error("gps creat failed.");      
	/*mpu6050  初始化*/
	c_my_iic iic = {0};
	iic = iic_creat(GPIOB,GPIO_PIN_14, GPIOB,GPIO_PIN_13);
	if(NULL == iic.this)	log_error("iic creat failed.");
	ret = mpu6050.init(&iic);
	if(E_OK != ret )	log_error("mpu6050 init failed.");
	/*OLED 初始化*/
	OLED_Init();			//初始化OLED  
	OLED_Clear() ;
	/*键盘初始化*/
	c_key_board board = {0};
	key_list board_list[] = {
		{"L1"  ,GPIOA,GPIO_PIN_4,KEY_PRESS_IS_ZERO },
		{"L2"  ,GPIOA,GPIO_PIN_5,KEY_PRESS_IS_ZERO },
		{"L3"  ,GPIOA,GPIO_PIN_7,KEY_PRESS_IS_ZERO },		//红外
	};
	board = key_board_create(board_list,sizeof(board_list)/sizeof(key_list));
	if(NULL == board.this)	log_error("Board create failed.");
	ret = board.set_callback(&board,key_board_call);
	if(E_OK != ret)	log_error("Callback set failed.");
	/*蜂鸣器初始化*/
	c_switch beep = {0};
	beep = switch_create(GPIOC, GPIO_PIN_15);
	if(NULL == beep.this)	log_error("Swtich creat failed.");
	ret = beep.set(&beep,SWITCH_LOW);
	/*LED初始化*/
	c_switch led = {0};
	led = switch_create(GPIOB, GPIO_PIN_15);
	if(NULL == led.this)	log_error("Swtich creat failed.");
	ret = led.set(&led,SWITCH_HIGHT);
	/*震动马达初始化*/
	c_switch motor = {0};
	motor = switch_create(GPIOB, GPIO_PIN_0);
	if(NULL == motor.this)	log_error("Swtich creat failed.");
	ret = motor.set(&motor,SWITCH_LOW);
	/*声音检测adc初始化*/
	ret = my_adc.init(MY_ADC_0);
	if(E_OK != ret)	log_error("my_adc get failed.");
	/*wifi 初始化*/
	c_esp esp8266 = esp8266_creat(MY_UART_1);
	if(NULL == esp8266.this)	log_error("esp8266 creat failed.");
	ret = esp8266.set_callback(&esp8266, esp8266_server_callback);  
	if(ret != E_OK)    log_error("esp8266 set callback failed.");
	/*超声检测创建*/
	c_hcsr04 hcsr04 = {0};
	hcsr04 = hcsr04_creat(GPIOB, GPIO_PIN_11, GPIOB, GPIO_PIN_10);
	if(NULL == hcsr04.this)	log_error("hcsr04 creat failed.");
	
	while(1)
	{
		/*获取一次温度*/
		ret = ds18b20.get(&ds18b20,&temp);
		if(E_OK != ret)	log_error("ds18b20 get failed.");
		/*获取GPS定位信息*/
		ret = gps.get(&gps,&data);
		if(E_OK != ret)			log_error("gps get failed.");
		/*获取倾斜角度*/
		MPU_Calculate(&x,&y,&z);
		/*声音检测adc初始化*/
		ret = my_adc.get(MY_ADC_0, &voice);
		if(E_OK != ret)	log_error("my_adc get failed.");
		voice *= 30;
		
		/*时间计数*/
		if((sys_cnt%10) == 0){
			time[2]++;
			if(time[2] >= 60){
				time[1]++;
				time[2] = 0;
			}
			if(time[1] >= 60){
				time[0]++;
				time[1] = 0;
			}
			
			/*上传数据到app*/
			memset(tx_buf, 0, sizeof(tx_buf));
			sprintf(tx_buf, "AT+DATA=%.1f,%d!%.2f@%.2f#", 
								temp, step, data.o_gngga.o_longitude/100, data.o_gngga.o_latitude/100);
			ret = esp8266.server_send(&esp8266, tx_buf);  
		}
		
		/*蜂鸣器报警*/
		if(unlock_flag == 0 && cat_flag == 0 && sys_cnt > 50){
			ret = beep.set(&beep,SWITCH_HIGHT);
		}else{
			ret = beep.set(&beep,SWITCH_LOW);
		} 
		
		/*计步*/
		if(fabs(x) > 10 || fabs(y) > 10){
			if(step_flag <= 2)	step_flag++;
		}
		
		if(fabs(x) < 6 || fabs(y) < 6){
			if(step_flag > 2)	step_flag++;
		}
		
		if(step_flag > 4){
			step_flag = 0;
			step++;
		}
			
		/*大于10点且有声音*/
		if((time[0] >= 22 || time[0] <= 6) && voice > 20){
			led_flag = 100;
		}
		/*亮灯10s*/
		if(led_flag > 0){
			led_flag--;
			ret = led.set(&led,SWITCH_LOW);
		}else{
			ret = led.set(&led,SWITCH_HIGHT);
		}
		
		/*训宠止吠*/
		if(xu_cong_flag != 0){
			dis = hcsr04.get_dis(&hcsr04, 25.0f); //启动一次超声
			ret = motor.set(&motor,SWITCH_HIGHT);
		}else{
			ret = motor.set(&motor,SWITCH_LOW);
		}
		
		
		
		/*oled显示内容*/
		if((sys_cnt%5) == 0){

			
			if(page == 0){
				memset(oled_show, 0, sizeof(oled_show));
				sprintf(oled_show, "Temp:%.1f   ", temp);
				OLED_ShowString(0,0, oled_show, 16); 
				
				memset(oled_show, 0, sizeof(oled_show));
				sprintf(oled_show, "Voice:%.1f  ", voice);
				OLED_ShowString(0,2, oled_show, 16);
				
				memset(oled_show, 0, sizeof(oled_show));
				sprintf(oled_show, "Step:%d   ", step);
				OLED_ShowString(0,4, oled_show, 16);
				
				memset(oled_show, 0, sizeof(oled_show));
				sprintf(oled_show, "Time %02d:%02d:%02d", time[0], time[1], time[2]);
				OLED_ShowString(0,6, oled_show, 16);
			}else{
				memset(oled_show, 0, sizeof(oled_show));
				sprintf(oled_show, "Long:%.2f   ", data.o_gngga.o_longitude/100);
				OLED_ShowString(0,0, oled_show, 16); 
				
				memset(oled_show, 0, sizeof(oled_show));
				sprintf(oled_show, "Lati:%.2f  ",  data.o_gngga.o_latitude/100);
				OLED_ShowString(0,2, oled_show, 16);
			}
		}

		/*重复执行逻辑 从此处开始*/
		sys_cnt++;
		vTaskDelay(100);
	} 
}

#define APP_2_UPDATE_TIME_MS  100
static void my_app_task_2(void* param)
{
	while(1)
	{
		/*重复执行逻辑 从此处开始*/
		vTaskDelay(APP_2_UPDATE_TIME_MS);
	}
}


#define APP_3_UPDATE_TIME_MS  1000
static void my_app_task_3(void* param)
{
	while(1)
	{
		/*重复执行逻辑 从此处开始*/
		vTaskDelay(APP_3_UPDATE_TIME_MS);
	}
}

/********************************************此处写入逻辑部分***************************************************/
/*************************************************
* Function: 我的app初始化
* Description: my_app_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
void my_app_init(void)
{
	taskENTER_CRITICAL();
	BaseType_t os_ret;
#if 1
	/*创建app任务*/
	if(NULL == g_app1_handle)
	{
		os_ret = xTaskCreate((TaskFunction_t )my_app_task_1,     	
                             (const char*    )MODULE_NAME,   	
                             (uint16_t       )MY_APP_1_STK, 
                             (void*          )NULL,				
                             (UBaseType_t    )MY_APP_1_PRO,	
                             (TaskHandle_t*  )&g_app1_handle); 
							 
		if(pdPASS != os_ret)
		{
			log_error("Task init filed,ret=%d",(int)os_ret);
			return;
		}
	} 
#endif

#if 0	
	/*创建app任务*/
	if(NULL == g_app2_handle)
	{
		os_ret = xTaskCreate((TaskFunction_t )my_app_task_2,     	
                             (const char*    )MODULE_NAME,   	
                             (uint16_t       )MY_APP_2_STK, 
                             (void*          )NULL,				
                             (UBaseType_t    )MY_APP_2_PRO,	
                             (TaskHandle_t*  )&g_app2_handle); 
							 
		if(pdPASS != os_ret)
		{
			log_error("Task init filed,ret=%d",(int)os_ret);
			return;
		}
	} 
#endif	

#if 0
	/*创建app任务*/
	if(NULL == g_app3_handle)
	{
		os_ret = xTaskCreate((TaskFunction_t )my_app_task_3,     	
                             (const char*    )MODULE_NAME,   	
                             (uint16_t       )MY_APP_3_STK, 
                             (void*          )NULL,				
                             (UBaseType_t    )MY_APP_3_PRO,	
                             (TaskHandle_t*  )&g_app3_handle); 
							 
		if(pdPASS != os_ret)
		{
			log_error("Task init filed,ret=%d",(int)os_ret);
			return;
		}
	} 
#endif    
	   
	vTaskDelete(NULL);
	taskEXIT_CRITICAL();
}
#endif
