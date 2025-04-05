/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : esp.h
* Description: en600/ec800等 支持移远标准通信协议的模块使用的 公开头文件
* Author: Gwen9
* Version: v1.0
* Date: 2022/02/25
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_ESP8266_ENABLED

#define MODULE_NAME       "esp"

#ifndef ESP_TASK_STK
    #define ESP_TASK_STK      (512)
#endif

#ifndef ESP_TASK_PRO
    #define ESP_TASK_PRO      (9)
#endif

#define esp_BAUDRATE                (115200)
#define MSG_QUEUE_MAX_LEN           (5)
#define PUB_STR_MAX_LEN             (256)

#define AT_ACK_OK                  	(0x01 << 1)
#define AT_ACK_FAILED              	(0x01 << 2)

#define MQTT_POST_DATA_RDY          (0x01 << 3)
#define MQTT_POST_DATA_ACK          (0x01 << 4)

#define MAX_RETRY_TIME          (5)


/*数据消息*/
typedef struct __MESG
{
    char pstr_pub[64];     //发布/订阅  消息字符串
}data_msg_t;

typedef struct __M_esp
{
    u8                  m_uart_id;
    u8         			m_state;
    QueueHandle_t       tx_queue;   //发送消息队列
    QueueHandle_t       rx_queue;   //接收消息队列
    TaskHandle_t        m_task;     //esp主线程
    EventGroupHandle_t  m_uart_event; //串口消息同步事件组
    char       *		m_expt_rec;    //期望接收数组

    const char *      	mqtt_key;
    const char *       	mqtt_dev_name;
    const char *        mqtt_dev_secret;
}m_esp;

static char expt_rec_buf[PUB_STR_MAX_LEN];
static char rec_buf[PUB_STR_MAX_LEN];
static uint8_t rec_cnt;
static int m_set(const m_esp* this, const char *tx_data, const char *re_expt, uint32_t time_out);
static void m_task(void* pdata);
static int m_send(const c_esp* this, const char *data);
static int m_state(const c_esp* this);

extern uint8_t open_flag;
int m_callback(void* param, const u8* data, u16 data_len)
{

    BaseType_t  os_ret = 0;
    m_esp* m_this = NULL;
    /*参数检查*/
    if(NULL == param)   {log_error("Param error."); return E_ERROR;} 
    m_this = (m_esp*)param;

 	if(data_len + rec_cnt < PUB_STR_MAX_LEN){
		memcpy(rec_buf + rec_cnt, data, data_len);
		rec_cnt += data_len;
	}else{
		rec_cnt = 0;
		memset(rec_buf, 0, sizeof(rec_buf));
		log_inform("Len:%d, Error", data_len);
	}	

    if(strstr(rec_buf, m_this->m_expt_rec) != NULL){
        xEventGroupSetBits(m_this->m_uart_event, AT_ACK_OK);
        rec_cnt = 0;
        memset(rec_buf, 0, sizeof(rec_buf));
    }else if(strstr(rec_buf, "ERROR") != NULL){
        xEventGroupSetBits(m_this->m_uart_event, AT_ACK_FAILED);
        rec_cnt = 0;
        memset(rec_buf, 0, sizeof(rec_buf));
    }else if(strstr(rec_buf, "+MQTTSUBRECV") != NULL){
		open_flag = 1;
	}
    return E_OK;
}

c_esp esp8266_creat(u8 uart_id, const char *key, const char *dev_name, const char *dev_secret)
{
    int ret = 0;
    BaseType_t  os_ret = 0;
    c_esp    new = {0};
    m_esp*   m_this = NULL;  
    
    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_esp));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_esp));
    m_this = new.this; 

    /*初始化私有成员*/
    m_this->m_uart_id 	= uart_id;
	m_this->m_expt_rec	= expt_rec_buf;
    m_this->m_state   	= ESP_STA_UNINITED;
	m_this->mqtt_key	= key;
	m_this->mqtt_dev_name = dev_name;
	m_this->mqtt_dev_secret = dev_secret;
    m_this->tx_queue 	= xQueueCreate(MSG_QUEUE_MAX_LEN, sizeof(data_msg_t));
    if(NULL == m_this->tx_queue)     {log_error("Queue creat filed."); goto error_handle;}
    m_this->rx_queue = xQueueCreate(MSG_QUEUE_MAX_LEN, sizeof(data_msg_t));
    if(NULL == m_this->rx_queue)     {log_error("Queue creat filed."); goto error_handle;}
    /*创建串口消息同步事件组*/
    m_this->m_uart_event = xEventGroupCreate();
    if(NULL == m_this->m_uart_event)     {log_error("m_uart_sem create failed.");    goto error_handle;}
    /*初始化串口*/
    ret = my_uart.init(uart_id, esp_BAUDRATE, 256, UART_MODE_DMA);
    if(E_OK != ret)     {log_error("Uart init failed.");    goto error_handle;}
    /*注册串口回调*/
    ret = my_uart.set_callback(m_this->m_uart_id, (void*)m_this, m_callback);
    if(E_OK != ret)     {log_error("Callback set failed."); goto error_handle;}
	
	/*初始化公有成员*/
	new.state 			= m_state;
	new.mqtt_send		= m_send;
//	new.set_callback	=  
//	new.mqtt_start		
	
    /*创建任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_task       ,
                     (const char*    )"esp task"     ,
                     (uint16_t       )ESP_TASK_STK   ,
                     (void*          )m_this           ,
                     (UBaseType_t    )ESP_TASK_PRO   ,
                     (TaskHandle_t*  )&m_this->m_task);             
    if(pdPASS != os_ret)    {log_error("Sim800 task creat filed,ret=%d",(int)os_ret);goto error_handle;}
    return new;

error_handle:
    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_state(const c_esp* this)
{
    m_esp* m_this = NULL;
    /*参数检查*/
    if(NULL == this || NULL == this->this)   {log_error("null pointer."); return E_ERROR;} 
	m_this = this->this;
	return m_this->m_state;
}
	
static int m_send(const c_esp* this, const char *data)
{
	data_msg_t new_msg;
	BaseType_t  os_ret = 0 ;
	
    m_esp* m_this = NULL;
    /*参数检查*/
    if(NULL == this || NULL == this->this)   {log_error("null pointer."); return E_ERROR;} 
	m_this = this->this;

	memset(new_msg.pstr_pub, 0, sizeof(new_msg.pstr_pub));
	strcpy(new_msg.pstr_pub, data);
	/*压入消息队列*/
    os_ret = xQueueSend(m_this->tx_queue, &new_msg, 0);
    if(pdTRUE != os_ret)
    {
    	log_error("Queue send message filed.");
    	return E_ERROR;
    }
	return E_OK;
}
	
static void m_task(void* pdata)
{
    int ret = 0;
    data_msg_t new_msg;
    m_esp* m_this = NULL;
    BaseType_t  os_ret = 0 ;
    uint8_t retry_cnt = 0;
    static char temp_buf[PUB_STR_MAX_LEN];
    /*参数检查*/
    if(NULL == pdata)   {log_error("Param error."); vTaskDelete(NULL);}  /*删除本任务*/
    m_this = (m_esp*)pdata;

restart:
    while (1)
    {
        switch (m_this->m_state)
        {
        case 0: /*复位*/
            ret = m_set(m_this, ESP_CMD_RST, OK_ACK, 3000);
            if(E_OK != ret)	log_error("esp reset failed. retry: %d", retry_cnt++);
            else{
				m_this->m_state++;
				vTaskDelay(2000);
			}           
            break;
        case 1: /*设置模式*/
            ret = m_set(m_this, ESP_CMD_MODE, OK_ACK, 1000);
            if(E_OK != ret)	log_error("esp mode set failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 2: /*断开连接*/
            ret = m_set(m_this, ESP_CMD_QAP, OK_ACK, 1000);
            if(E_OK != ret)	log_error("esp quit ap failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 3: /*加入wifi*/
            ret = m_set(m_this, ESP_CMD_JAP, "WIFI CONNECTED\r\nWIFI GOT IP\r\n", 3000);
            if(E_OK != ret)	    log_error("recive mode set failed. retry: %d", retry_cnt++);
            else                m_this->m_state++;
            break;
        default:
			m_this->m_state++;
            break;
        }
        if(retry_cnt > MAX_RETRY_TIME)
		{
			m_this->m_state = 0;
			retry_cnt = 0;
			log_inform("esp restart.");
			goto restart;
		}			
        if(m_this->m_state > 4)    break;
        vTaskDelay(1000);
		memset(rec_buf, 0, sizeof(rec_buf));
		rec_cnt = 0;
    }
	log_inform("sta joined ap.");
	
mqtt_connect:
    while (1)
    {
        switch (m_this->m_state)
        {
        case 5: /*配置MQTT参数*/
            memset(temp_buf, 0, sizeof(temp_buf));
            snprintf(temp_buf, PUB_STR_MAX_LEN, ESP_CMD_MQTT_PARAM, m_this->mqtt_dev_name, m_this->mqtt_key, m_this->mqtt_dev_secret);
            ret = m_set(m_this, temp_buf, OK_ACK, 1000);
            if(E_OK != ret)	log_error("mqtt param config failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 6: /*配置客户端ID*/
            memset(temp_buf, 0, sizeof(temp_buf));
            snprintf(temp_buf, PUB_STR_MAX_LEN, ESP_CMD_MQTT_CLIENTID, m_this->mqtt_key, m_this->mqtt_dev_name);
            ret = m_set(m_this, temp_buf, OK_ACK, 1000);
            if(E_OK != ret)	log_error("mqtt client id config failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 7: /*连接mqtt网络*/
            memset(temp_buf, 0, sizeof(temp_buf));
            snprintf(temp_buf, PUB_STR_MAX_LEN, ESP_CMD_MQTT_CON, m_this->mqtt_key);
            ret = m_set(m_this, temp_buf, OK_ACK, 3000);
            if(E_OK != ret)	log_error("mqtt param config failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 8: /*订阅主题*/
            memset(temp_buf, 0, sizeof(temp_buf));
            snprintf(temp_buf, PUB_STR_MAX_LEN, ESP_CMD_MQTT_SUB, ALI_CLOUD_POST);
            ret = m_set(m_this, temp_buf, OK_ACK, 1000);
            if(E_OK != ret)	log_error("mqtt sub post failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 9: /*发布主题*/
            memset(temp_buf, 0, sizeof(temp_buf));
            snprintf(temp_buf, PUB_STR_MAX_LEN, ESP_CMD_MQTT_SUB, ALI_CLOUD_SET);
            ret = m_set(m_this, temp_buf, OK_ACK, 1000);
            if(E_OK != ret)	log_error("mqtt sub set failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 10: /*发布主题*/
            log_inform("mqtt connet ok.");
            m_this->m_state++;
            break; 
        default:
			m_this->m_state++;
            break;
        }

        if(retry_cnt > MAX_RETRY_TIME)
        {
            m_this->m_state = 5;
			retry_cnt = 0;
			log_inform("reset mqtt connet.");
            goto mqtt_connect;
        }  
        if(m_this->m_state > 10)  break;
        vTaskDelay(1000);
    }

    while (1)
    {
        /*取出消息发送*/
        os_ret = xQueueReceive(m_this->tx_queue, &new_msg, 0);
        if( m_this->m_state != ESP_STA_MQTT_CONNECT) continue;
        if(pdTRUE == os_ret)
        {
			/*串口发送数据*/
			uint32_t tx_len = strlen(new_msg.pstr_pub);
			memset(temp_buf, 0, sizeof(temp_buf));
			snprintf(temp_buf, sizeof(temp_buf), ESP_CMD_MQTT_PUB, ALI_CLOUD_POST, new_msg.pstr_pub, tx_len);
			ret = my_uart.send(m_this->m_uart_id, temp_buf, strlen(temp_buf));          //串口发送
			if(E_OK != ret)	    log_error("Uart send failed.");
			EventBits_t r_event = xEventGroupWaitBits(m_this->m_uart_event, 
											MQTT_POST_DATA_RDY,            /*接收任务感兴趣的事件*/ 
											pdTRUE,                             /*退出时清除事件位*/
											pdFALSE,                            /*满足感兴趣的所有事件*/
											1000);                          /* 指定超时事件,一直等 */
			if((r_event&MQTT_POST_DATA_RDY) != 0)
			{
				ret = my_uart.send(m_this->m_uart_id, new_msg.pstr_pub, strlen(new_msg.pstr_pub));          //串口发送
				if(E_OK != ret)	    log_error("Uart send failed.");
			}
        }

        os_ret = xQueueReceive(m_this->rx_queue, &new_msg, 0);
        {

        }

        if(retry_cnt > MAX_RETRY_TIME)
        {
            m_this->m_state = 0;
            goto restart;
        }  
        vTaskDelay(100);
    }
}



static int m_set(const m_esp* this, const char *tx_data, const char *re_expt, uint32_t time_out)
{
    const m_esp* m_this = this;
    int ret = E_OK;
    BaseType_t     os_ret  = 0;
    /*参数检测*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(strlen(re_expt) > PUB_STR_MAX_LEN)    {log_error("expect rec over lenth ."); return E_ERROR;}
    /*设置期望接收数据*/
    memset(m_this->m_expt_rec, 0, sizeof(m_this->m_expt_rec));
    strcpy(m_this->m_expt_rec, re_expt);
    /*串口发送数据*/
    ret = my_uart.send(m_this->m_uart_id, tx_data, strlen(tx_data));          //串口发送
    if(E_OK != ret)	    {log_error("Uart send failed."); return E_ERROR;}
    /*等待事件*/
    EventBits_t r_event = xEventGroupWaitBits(m_this->m_uart_event, 
                                                AT_ACK_OK|AT_ACK_FAILED,            /*接收任务感兴趣的事件*/ 
                                                pdTRUE,                             /*退出时清除事件位*/
                                                pdFALSE,                            /*满足感兴趣的所有事件*/
                                                time_out);                          /* 指定超时事件,一直等 */
    if((r_event&AT_ACK_OK) != 0)    return  E_OK;
    else                            return  E_ERROR;
}

#endif
