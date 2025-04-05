/*****************************************************************************
* Copyright:
* File name: track_car.c
* Description: 循迹车 函数实现
* Author: 许少
* Version: V1.0
* Date: 2021/12/20
*****************************************************************************/
#include "app_conf.h"

#ifdef APP_TRACK_CAR_ENABLED

#define MODULE_NAME       "tracking car"

#define L_LIGHT_GPIO      GPIOA
#define L_LIGHT_PIN       GPIO_PIN_12
#define R_LIGHT_GPIO      GPIOA
#define R_LIGHT_PIN       GPIO_PIN_11

#define UPDATE_TIME_MS    20          /*更新时间*/
#define PASS_TIME_MS      250
#define TURN_TIME_MS     1320
#define LEFT_TIME_MS      700
#define RIGHT_TIME_MS     700

#ifdef  LOG_TAGE
#undef  LOG_TAGE
#endif
#define LOG_TAGE          MODULE_NAME

static int m_stop     (void);
static int m_move     (void);
static u8  m_getspeed (void);
static int m_setspeed (u8);
static int tracking_car_init(void);
static int m_move_by_path(u8* path,u16 path_len);
static int m_pause    (void); /*暂停*/
static int m_cont     (void); /*继续*/
static int m_state    (u8* state);

const  c_track_car       track_car = {tracking_car_init,m_pause,m_cont,m_stop,m_move,m_move_by_path,m_state,m_getspeed,m_setspeed};
static bool              g_init_state  ;  /*初始化状态*/
static bool              g_pause_flage ;  /*暂停标志*/
static u8                g_work_type   ;  /*工作状态*/
static u8                g_cur_speed   ;  /*当前速度*/
static u8                g_set_speed   ;  /*设置速度*/
static u8*               g_path        ;
static u16               g_path_len    ;
static u16               g_path_index  ;


static c_tracking_light  g_light_left  ;  /*左循迹灯*/
static c_tracking_light  g_light_right ;  /*右循迹灯*/
static TaskHandle_t      g_task_handle ;  /*任务句柄*/

	   
static void   m_task           (void* pdata);

/*************************************************
* Function: 循迹车初始化
* Description: tracking_car_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
static int tracking_car_init(void)
{
	BaseType_t os_ret;
	
	if(g_init_state)
	{
		return E_OK;
	}
	
	/*创建循迹灯*/
	if(NULL == g_light_left.this)
	{
		g_light_left = tracking_light_init(L_LIGHT_GPIO,L_LIGHT_PIN,0);
		if(NULL == g_light_left.this)
		{
			log_error("Left tracking light creat filed.");
			return E_ERROR;
		}
	}
	if(NULL == g_light_right.this)
	{
		g_light_right = tracking_light_init(R_LIGHT_GPIO,R_LIGHT_PIN,0);
		if(NULL == g_light_right.this)
		{
			log_error("Right tracking light creat filed.");
			return E_ERROR;
		}
	}
	
	/*创建循迹车任务*/
	if(NULL == g_task_handle)
	{
		os_ret = xTaskCreate((TaskFunction_t )m_task,     	
                             (const char*    )MODULE_NAME,   	
                             (uint16_t       )TRACKING_CAR_STK, 
                             (void*          )NULL,				
                             (UBaseType_t    )TRACKING_CAR_PRO,	
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

static void m_follow(void)
{
    if(GPIO_PIN_RESET == g_light_left.get_state(g_light_left.this) && GPIO_PIN_SET == g_light_right.get_state(g_light_right.this))
    {
        chassis_work(CHASSIS_LEFT,g_set_speed);
        g_cur_speed = g_set_speed / 2;
    }
    
    else if(GPIO_PIN_SET == g_light_left.get_state(g_light_left.this) && GPIO_PIN_RESET == g_light_right.get_state(g_light_right.this))
    {
        chassis_work(CHASSIS_RIGHT,g_set_speed);
        g_cur_speed = g_set_speed / 2;
    }
    
    else if(GPIO_PIN_RESET == g_light_left.get_state(g_light_left.this) && GPIO_PIN_RESET == g_light_right.get_state(g_light_right.this))
    {
        chassis_work(CHASSIS_GO,g_set_speed);
        g_cur_speed = g_set_speed;
    }
    
    else
    {
        chassis_work(CHASSIS_STOP,0);
        g_cur_speed = 0;
    }
    
    return;
}

static int m_dir(u8 dir)
{
    switch(dir)
    {
        case CAR_PASS:
        {
            log_inform("Car pass");
            g_cur_speed = g_set_speed;
            chassis_work(CHASSIS_GO,g_set_speed);
            vTaskDelay(PASS_TIME_MS);
            break;
        }
        case CAR_LEFT:
        {
            log_inform("Car left");
            g_cur_speed = g_set_speed / 2;
            chassis_work(CHASSIS_RIGHT,g_set_speed);
            vTaskDelay(LEFT_TIME_MS);
            break;
        }
        case CAR_RIGHT:
        {
            log_inform("Car right");
            g_cur_speed = g_set_speed / 2;
            chassis_work(CHASSIS_LEFT,g_set_speed);
            vTaskDelay(RIGHT_TIME_MS);
            break;
        }
        case CAR_TURN:
        {
            log_inform("Car turn");
            g_cur_speed = g_set_speed / 2;
            chassis_work(CHASSIS_LEFT,g_set_speed);
            vTaskDelay(TURN_TIME_MS);
            break;
        }
        default:
        {
            log_error("Param error");
            return E_PARAM;
        }
    }
    chassis_work(CHASSIS_STOP,g_set_speed);
    g_cur_speed = 0;
    
    return E_OK;
}

static void m_task(void* pdata)
{
    chassis_init();
	chassis_work(CHASSIS_STOP,0);  /*默认先刹车*/	
	
	while(1)
	{
        vTaskDelay(UPDATE_TIME_MS);
        
		/*循迹模式*/
		if(WORK_TYPE_FOLLOW == g_work_type)
		{
            /*暂停*/
            if(g_pause_flage) 
            {
                chassis_work(CHASSIS_STOP,g_set_speed);
                continue;
            }
            
            m_follow();
            
        }
        /*路径模式*/
        else if(WORK_TYPE_PATH == g_work_type)
        { 
            /*暂停*/
            if(g_pause_flage) 
            {
                chassis_work(CHASSIS_STOP,g_set_speed);
                continue;
            }
            
            /*是否遇到了路口*/
            if(GPIO_PIN_SET == g_light_left.get_state(g_light_left.this) && GPIO_PIN_SET == g_light_right.get_state(g_light_right.this))
            {
                /*向前走一点 车中心对准路口*/
                g_cur_speed = g_set_speed;
                chassis_work(CHASSIS_GO,g_set_speed);
                vTaskDelay(650);
                
                m_dir(g_path[g_path_index]);
                ++g_path_index;
                
                /*检查路径是否完成*/
                if(g_path_index >= g_path_len)
                {
                    vPortFree(g_path);
                    g_path = NULL;
                    g_path_len = 0;
                    g_path_index = 0;
                    g_work_type = WORK_TYPE_IDLE;  /*更新为idle*/
                }
                
            }
            
            /*否则 跟随路径*/
            else
            {
                m_follow();
            }
		}
		else
		{
			chassis_work(CHASSIS_STOP,0);
			g_cur_speed = 0;
		}
		
		
    }	
}


static int m_stop     (void)
{
	if(!g_init_state)
	{
		log_error("Need init first.");
		return E_ERROR;
	}
	
	g_work_type = WORK_TYPE_IDLE;
    if(NULL != g_path)
    {
        vPortFree(g_path);
        g_path = NULL;
    }
    g_path_index = 0;
    g_path_len   = 0;
    g_pause_flage = false;
	
	return E_OK;
}

static int m_setspeed (u8 speed)
{
	if(!g_init_state)
	{
		log_error("Need init first.");
		return E_ERROR;
	}
	
	g_set_speed = speed;
	
	return E_OK;    
}

static int m_move     (void)
{
	if(!g_init_state)
	{
		log_error("Need init first.");
		return E_ERROR;
	}
    
    /*检查车状态*/
    if(WORK_TYPE_IDLE != g_work_type)
    {
        log_error("Need stop first.");
        return E_ERROR;
    }
	
	g_work_type = WORK_TYPE_FOLLOW;
	
	return E_OK;
}

static int m_pause    (void)
{
    if(!g_init_state)
	{
		log_error("Need init first.");
		return E_ERROR;
	}
    
    g_pause_flage = true;
    
    return E_OK;
}

static int m_cont     (void)
{
    if(!g_init_state)
	{
		log_error("Need init first.");
		return E_ERROR;
	}
    
    g_pause_flage = false;
    
    return E_OK;    
}

u8  m_getspeed (void)
{
	if(!g_init_state)
	{
		log_error("Need init first.");
		return E_ERROR;
	}
	
	return g_cur_speed;
}

static int m_move_by_path(u8* path,u16 path_len)
{
    /*参数检查*/
    if(NULL == path)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(path_len > CAR_PATH_MAX_LEN)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    
    /*检查初始化状态*/
    if(!g_init_state)
	{
		log_error("Need init first.");
		return E_ERROR;
	}
    
    /*检查车状态*/
    if(WORK_TYPE_IDLE != g_work_type)
    {
        log_error("Need stop first.");
        return E_ERROR;
    }
    
    /*拷贝路径*/
    g_path = pvPortMalloc(path_len);
    memset(g_path,0,path_len);
    memcpy(g_path,path,path_len);
    g_path_len = path_len;
    g_path_index = 0;
    g_work_type = WORK_TYPE_PATH;
    
    return E_OK;
}

static int m_state    (u8* state)
{
    if(NULL == state)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
	if(!g_init_state)
	{
		log_error("Need init first.");
		return E_ERROR;
	}
	
    *state = g_work_type;
    
	return E_OK;
}
#endif