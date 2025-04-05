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
    #define ESP_TASK_STK      (256)
#endif

#ifndef ESP_TASK_PRO
    #define ESP_TASK_PRO      (9)
#endif

#define esp_BAUDRATE                (115200)
#define MSG_QUEUE_MAX_LEN           (3)
#define PUB_STR_MAX_LEN             (200)
#define SERVER_MSG_MAX_LEN          (128)

//定义事件标志组
#define AT_ACK_OK                  	(0x01 << 1)
#define AT_ACK_FAILED              	(0x01 << 2)
#define WIFI_AP_CONNCETED           (0x01 << 3)         //wifi连接事件
#define WIFI_AP_DISCONNCETED        (0x01 << 4)         //wifi断连事件
#define WIFI_SERVER_CONNCETED       (0x01 << 5)     //服务器连接事件
#define WIFI_SERVER_DISCONNCETED    (0x01 << 6)     //服务器断开连接事件

#define MQTT_POST_DATA_RDY          (0x01 << 3)
#define MQTT_POST_DATA_ACK          (0x01 << 4)

#define MAX_RETRY_TIME          (3)


/*数据消息*/
typedef struct __MESG
{
    char pstr_pub[SERVER_MSG_MAX_LEN];     //接收/发送  消息字符串
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
    int (*callback)     (const u8*, u16);    //server接收回调函数

}m_esp;

static data_msg_t rx_new_msg;
static data_msg_t tx_new_msg;
static char expt_rec_buf[PUB_STR_MAX_LEN];
static char rec_buf[PUB_STR_MAX_LEN];
static uint8_t rec_cnt;
static int m_set(const m_esp* this, const char *tx_data, const char *re_expt, uint32_t time_out);
static void m_task(void* pdata);
static int m_send(const c_esp* this, const char *data);
static int m_state(const c_esp* this);
static int m_set_callback(const c_esp* this, int (*callback)(const u8* data, u16 data_len));

extern uint8_t open_flag;
int m_callback(void* param, const u8* data, u16 data_len)
{
    BaseType_t  os_ret = 0;
    m_esp* m_this = NULL;
	int ret = E_ERROR;
    /*参数检查*/
    if(NULL == param)   {log_error("Param error."); return E_ERROR;} 
    m_this = (m_esp*)param;

 	if(data_len + rec_cnt < PUB_STR_MAX_LEN){
		memcpy(rec_buf + rec_cnt, data, data_len);
		rec_cnt += data_len;
	}else{
		log_inform("Len:%d, Error", data_len);
		goto end;
	}	

    //AT命令接收处理
    if(strstr(rec_buf, m_this->m_expt_rec) != NULL){
        xEventGroupSetBits(m_this->m_uart_event, AT_ACK_OK);
		ret = E_OK;
		goto end;
    }else if(strstr(rec_buf, "ERROR") != NULL){
        xEventGroupSetBits(m_this->m_uart_event, AT_ACK_FAILED);
		ret = E_OK;
		goto end;
    } 
	
    //wifi连接事件
    if(strstr(rec_buf, "+STA_CONNECTED:") != NULL){
        xEventGroupSetBits(m_this->m_uart_event, WIFI_AP_CONNCETED);
		ret = E_OK;
		goto end;
	}
    //wifi断开连接事件
    if(strstr(rec_buf, "+STA_DISCONNECTED:") != NULL){
        xEventGroupSetBits(m_this->m_uart_event, WIFI_AP_DISCONNCETED);
		ret = E_OK;
		goto end;
	}

    //server连接事件
    if(strstr(rec_buf, "0,CONNECT") != NULL){
        xEventGroupSetBits(m_this->m_uart_event, WIFI_SERVER_CONNCETED);
		ret = E_OK;
		goto end;
	}
    //server断开连接事件
    if(strstr(rec_buf, "0,CLOSED") != NULL){
        xEventGroupSetBits(m_this->m_uart_event, WIFI_SERVER_DISCONNCETED);
		ret = E_OK;
		goto end;
	}

    //收到数据
    if(strstr(rec_buf, "+IPD") != NULL && strstr(rec_buf, "\r\n") != NULL){
        uint8_t *pxIndex = strtok(rec_buf, ",");
        if(pxIndex == NULL)	    goto end;
        pxIndex = strtok(NULL, ",");
        if(pxIndex == NULL)	    goto end;
        pxIndex = strtok(NULL, ":");
        if(pxIndex == NULL)	    goto end;
        uint16_t rec_len = atoi(pxIndex);
		
		pxIndex = strtok(NULL, "\r\n");
        if(pxIndex == NULL)	    goto end;
        if(rec_len > SERVER_MSG_MAX_LEN){           //接收数据超长
            log_error("server recive over lenth.");
            goto end; 
        }else{          //否则压入接收队列
            memset(rx_new_msg.pstr_pub, 0, sizeof(rx_new_msg.pstr_pub));
            strcpy(rx_new_msg.pstr_pub, pxIndex);
			strcat(rx_new_msg.pstr_pub, "\r\n");
            os_ret = xQueueSend(m_this->rx_queue, &rx_new_msg, 0);
            if(pdTRUE != os_ret){
                log_error("Queue send message filed.");
                goto end;
            }
        }
	}
	
    
end:
	rec_cnt = 0;
	memset(rec_buf, 0, sizeof(rec_buf));
	return ret;
}

c_esp esp8266_creat(u8 uart_id)
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
    m_this->tx_queue 	= xQueueCreate(MSG_QUEUE_MAX_LEN, sizeof(data_msg_t));
    m_this->callback    = NULL;
    if(NULL == m_this->tx_queue)     {log_error("Queue creat filed."); goto error_handle;}
    m_this->rx_queue    = xQueueCreate(MSG_QUEUE_MAX_LEN, sizeof(data_msg_t));
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
	new.server_send		= m_send;
	new.set_callback    = m_set_callback;

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

static int m_set_callback(const c_esp* this, int (*callback)(const u8* data, u16 data_len))
{
    m_esp* m_this = NULL;
    /*参数检查*/
    if(NULL == this || NULL == this->this)   {log_error("null pointer."); return E_ERROR;} 
	m_this = this->this;
    m_this->callback = callback;
	return E_OK;
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
	BaseType_t  os_ret = 0 ;
	
    m_esp* m_this = NULL;
    /*参数检查*/
    if(NULL == this || NULL == this->this)   {log_error("null pointer."); return E_ERROR;} 
	m_this = this->this;

	if(m_this->m_state != ESP_STA_SERVER_CONNECT){
//		 log_error("server not connect .");
		return E_ERROR;
	}
	
	
    if(strlen(data) > SERVER_MSG_MAX_LEN){
        log_error("server send over lenth.");
        return E_ERROR;
    }else{
        memset(tx_new_msg.pstr_pub, 0, sizeof(tx_new_msg.pstr_pub));
        strcpy(tx_new_msg.pstr_pub, data);
        /*压入消息队列*/
        os_ret = xQueueSend(m_this->tx_queue, &tx_new_msg, 0);
        if(pdTRUE != os_ret)
        {
            log_error("Queue send message filed.");
            return E_ERROR;
        }
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
    static char temp_buf[SERVER_MSG_MAX_LEN];
    /*参数检查*/
    if(NULL == pdata)   {log_error("Param error."); vTaskDelete(NULL);}  /*删除本任务*/
    m_this = (m_esp*)pdata;

restart:
	m_this->m_state = 0;
	rec_cnt = 0;
	memset(rec_buf, 0, sizeof(rec_buf));	//清空接收缓存
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
        case 2: /*设置模式*/
            ret = m_set(m_this, ESP_CMD_CIPMODE, OK_ACK, 1000);
            if(E_OK != ret)	log_error("esp mode set failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 3: /*设置为多连接模式*/
            ret = m_set(m_this, ESP_CMD_CIPMUX, OK_ACK, 1000);
            if(E_OK != ret)	log_error("esp mode set failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 4: /*设置AP模式下的IP地址*/
            ret = m_set(m_this, ESP_CMD_AP_IP, OK_ACK, 1000);
            if(E_OK != ret)	log_error("esp quit ap failed. retry: %d", retry_cnt++);
            else            m_this->m_state++;
            break;
        case 5: /*设置AP的WIFI名称和密码*/
            ret = m_set(m_this, ESP_CMD_AP_PARAM, OK_ACK, 3000);
            if(E_OK != ret)	    log_error("recive mode set failed. retry: %d", retry_cnt++);
            else                m_this->m_state++;
            break;
        default:
			m_this->m_state++;
            break;
        }

        if(retry_cnt >= MAX_RETRY_TIME)
		{
			m_this->m_state = 0;
			retry_cnt = 0;
			log_inform("esp restart.");
			goto restart;
		}			
        if(m_this->m_state > 6)    break;
        vTaskDelay(500);
		memset(rec_buf, 0, sizeof(rec_buf));
		rec_cnt = 0;
    }
	log_inform("wifi ap craete successed.");
	

ap_wait_connect:   /*等待WIFI 连接*/
	m_this->m_state = ESP_STA_SERVER_DISCONN;
	rec_cnt = 0;
	memset(rec_buf, 0, sizeof(rec_buf));	//清空接收缓存
    log_inform("wait for connecting ...");
    EventBits_t r_event1 = xEventGroupWaitBits(m_this->m_uart_event, 
                                                WIFI_AP_CONNCETED,            /*接收任务感兴趣的事件*/ 
                                                pdTRUE,                             /*退出时清除事件位*/
                                                pdFALSE,                            /*满足感兴趣的所有事件*/
                                                portMAX_DELAY);                     /* 指定超时事件,一直等 */
    if((r_event1&WIFI_AP_CONNCETED) == 0){
        log_inform("ap connect failed ...");
        goto restart;
    }
    //启动服务器
    ret = m_set(m_this, ESP_CMD_SERVER_START, OK_ACK, 3000);
    if(E_OK != ret){
        log_error("server start failed, reset !");
        goto restart;
    }	   
	log_inform("ap connected, servr start .");
	
server_wait_connect:   /*等待客户端 连接*/
	m_this->m_state = ESP_STA_SERVER_DISCONN;
	rec_cnt = 0;
	memset(rec_buf, 0, sizeof(rec_buf));	//清空接收缓存
    //等待服务器启动
    EventBits_t r_event2 = xEventGroupWaitBits(m_this->m_uart_event, 
                                                WIFI_SERVER_CONNCETED|WIFI_AP_DISCONNCETED,         /*接收任务感兴趣的事件*/ 
                                                pdTRUE,                             /*退出时清除事件位*/
                                                pdFALSE,                            /*满足感兴趣的所有事件*/
                                                portMAX_DELAY);                     /* 指定超时事件,一直等 */
    if((r_event2&WIFI_SERVER_CONNCETED) == 0){
        log_inform("clinet connect failed, restart .");
        goto restart;
    }
	if((r_event2&WIFI_SERVER_DISCONNCETED) != 0){
		log_inform("wifi disconnected .");
		goto ap_wait_connect;
    }
	
    log_inform("clinet connected .");
	m_this->m_state = ESP_STA_SERVER_CONNECT;
	
    while (1)
    {
        //检查wifi是否断开连接
        EventBits_t r_event3 = xEventGroupWaitBits(m_this->m_uart_event, 
                                                    WIFI_AP_DISCONNCETED,            /*接收任务感兴趣的事件*/ 
                                                    pdTRUE,                             /*退出时清除事件位*/
                                                    pdFALSE,                            /*满足感兴趣的所有事件*/
                                                    0);                     /* 指定超时事件,一直等 */
        if((r_event3&WIFI_AP_DISCONNCETED) != 0){
            log_inform("wifi disconnected .");
            goto ap_wait_connect;
        }
		
        //检查server是否断开连接
        EventBits_t r_event4 = xEventGroupWaitBits(m_this->m_uart_event, 
                                                    WIFI_SERVER_DISCONNCETED,            /*接收任务感兴趣的事件*/ 
                                                    pdTRUE,                             /*退出时清除事件位*/
                                                    pdFALSE,                            /*满足感兴趣的所有事件*/
                                                    0);                     /* 指定超时事件,一直等 */
        if((r_event4&WIFI_SERVER_DISCONNCETED) != 0){
            log_inform("client disconnected .");
            goto server_wait_connect;
        }
		
        /*取出消息发送*/
        os_ret = xQueueReceive(m_this->tx_queue, &new_msg, 0);
        if(pdTRUE == os_ret)
        {
			/*串口发送数据*/
			uint32_t tx_len = strlen(new_msg.pstr_pub);
			memset(temp_buf, 0, sizeof(temp_buf));
			snprintf(temp_buf, sizeof(temp_buf), ESP_CMD_SERVER_SEND, tx_len);
            ret = m_set(m_this, temp_buf, SEND_READY_OK, 1000);
            if(E_OK != ret)     log_error("send failed .");
            else                ret = my_uart.send(m_this->m_uart_id, new_msg.pstr_pub, strlen(new_msg.pstr_pub));//串口发送
        }
		
        //收到客户端发送的数据
        os_ret = xQueueReceive(m_this->rx_queue, &new_msg, 0);
		if(pdTRUE == os_ret)
        {
            if(m_this->callback != NULL){
                m_this->callback(new_msg.pstr_pub, strlen(new_msg.pstr_pub));
            }else{
                log_error("recive callback not defined .");
            }
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
    if(NULL == this || NULL == tx_data || NULL == re_expt)
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
