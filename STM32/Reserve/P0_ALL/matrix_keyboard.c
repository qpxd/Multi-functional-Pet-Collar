/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : matrix_keyboard.h
* Description: matrix_keyboard 驱动模块
* Author: xzh
* Version: v1.0
* Date: 2022/12/07
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_MATRIX_KEYBOARD_ENABLED

typedef struct __M_MATRIX_KEYBD
{
    unsigned char  ucState ;
    matrix_io_t    m_io[8];  		  /*8个IO 对应0-4行 和 0-4列*/
	TaskHandle_t   m_task_handle   ;  /*键盘任务句柄*/
	QueueHandle_t  m_queue_handle  ;  /*键盘消息队列*/
	int (*m_callback)(u8,u8);
}m_matrix;

static void m_matrix_key_board_task(void* pdata);
static int m_set_callback(c_matrix* this, int(*callback)(u8,u8));

c_matrix matrix_keyboard_create(matrix_io_t *io_list)
{
    int ret = 0;
    c_matrix    new = {0};
    m_matrix*   m_this = NULL;  
    BaseType_t  os_ret = 0 ;
	
    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_matrix));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this, 0, sizeof(m_matrix));
    m_this = new.this; 

    /*保存相关变量*/
	new.set_callback = m_set_callback;
	
    /*复位引脚配置*/
	for(uint8_t i = 0; i < 8; i++)
	{
		m_this->m_io[i].gpio = io_list[i].gpio;
		m_this->m_io[i].pin = io_list[i].pin;
	}
	
    /*初始化键盘任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_matrix_key_board_task    ,
                         (const char*    )"matrix key board task"    ,
                         (uint16_t       )MATRIX_KEY_BOARD_TASK_STK  ,
                         (void*          )m_this              ,
                         (UBaseType_t    )MATRIX_KEY_BOARD_TASK_PRO  ,
                         (TaskHandle_t*  )&m_this->m_task_handle);
                                 
    if(pdPASS != os_ret)
    {
        log_error("Key board task creat filed,ret=%d",(int)os_ret);
        goto error_handle;
    }
	
    return new;
    
error_handle:
    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_set_callback(c_matrix* this, int(*callback)(u8,u8))
{
    m_matrix*  m_this = NULL;
    if(NULL == this || NULL == callback)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    m_this->m_callback = callback;
    return E_OK;
}

void io_config(GPIO_TypeDef* gpio, uint32_t pin, uint32_t mode, uint32_t pull)
{
	GPIO_InitTypeDef GPIO_Initure;
	GPIO_Initure.Pin   = pin                 ;
	GPIO_Initure.Mode  = mode     ;  //输入
	GPIO_Initure.Pull  = pull         ;  //浮空
	GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;//高速
	HAL_GPIO_Init(gpio, &GPIO_Initure);  
}

static void m_matrix_key_board_task(void* pdata)
{
    m_matrix* this = NULL;
    int ret = 0;

    /*参数检查*/
    if(NULL == pdata)
    {
        log_error("Param error.");
        vTaskDelete(NULL);   /*删除本任务*/
    }
    this = pdata;
	uint8_t row;
	uint8_t col;
	
    while(1)
    {
		//水平4个脚全部设置为上拉输入
		io_config(this->m_io[0].gpio, this->m_io[0].pin, GPIO_MODE_INPUT, GPIO_PULLUP);
		io_config(this->m_io[1].gpio, this->m_io[1].pin, GPIO_MODE_INPUT, GPIO_PULLUP);
		io_config(this->m_io[2].gpio, this->m_io[2].pin, GPIO_MODE_INPUT, GPIO_PULLUP);
		io_config(this->m_io[3].gpio, this->m_io[3].pin, GPIO_MODE_INPUT, GPIO_PULLUP);
		//竖直4个脚全部设置为推挽输出 低电平
		io_config(this->m_io[4].gpio, this->m_io[4].pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
		io_config(this->m_io[5].gpio, this->m_io[5].pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
		io_config(this->m_io[6].gpio, this->m_io[6].pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
		io_config(this->m_io[7].gpio, this->m_io[7].pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
		HAL_GPIO_WritePin(this->m_io[4].gpio, this->m_io[4].pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(this->m_io[5].gpio, this->m_io[5].pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(this->m_io[6].gpio, this->m_io[6].pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(this->m_io[7].gpio, this->m_io[7].pin, GPIO_PIN_RESET);
		for(row = 0; row < 4; row++)
		{
			if(HAL_GPIO_ReadPin(this->m_io[row].gpio, this->m_io[row].pin) == GPIO_PIN_RESET)
				break;
		}
		
		if(row < 4)
		{
			vTaskDelay(50);	//延时消抖
			io_config(this->m_io[row].gpio, this->m_io[row].pin, GPIO_MODE_OUTPUT_PP, GPIO_PULLDOWN);
			HAL_GPIO_WritePin(this->m_io[row].gpio, this->m_io[row].pin, GPIO_PIN_RESET);
			
			io_config(this->m_io[4].gpio, this->m_io[4].pin, GPIO_MODE_INPUT, GPIO_PULLUP);
			io_config(this->m_io[5].gpio, this->m_io[5].pin, GPIO_MODE_INPUT, GPIO_PULLUP);
			io_config(this->m_io[6].gpio, this->m_io[6].pin, GPIO_MODE_INPUT, GPIO_PULLUP);
			io_config(this->m_io[7].gpio, this->m_io[7].pin, GPIO_MODE_INPUT, GPIO_PULLUP);
			for(col = 0; col < 4; col++)
			{
				if(HAL_GPIO_ReadPin(this->m_io[col+4].gpio, this->m_io[col+4].pin) == GPIO_PIN_RESET)
					break;
			}
			
			if(col < 4 && this->m_callback != NULL)
			{
				//log_inform("row: %d, col: %d", row, col);
				this->m_callback(row, col);
				vTaskDelay(200);
			}
		}
		vTaskDelay(50);
    }

    return;
}

#endif
