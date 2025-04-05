#include "ws2812/ws2812_play.h"
#include "string.h"
#include "log/log.h"
#include "FreeRTOS.h"
#include "delay.h"

#include "task.h"

#define MODULE_NAME       "ws2812_play"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

u8 m_luminance=1;
int play_boot_animation(const c_ws2812* this)
{
	int i,ret;
	for(i =0;i<10;i++)
	{
		ret=this->play_logo(this,0,1,i*m_luminance,0x00,i*2*m_luminance);
		if(ret!=E_OK)
			return ret;
		vTaskDelay(20);
	}
	vTaskDelay(1000);
	for(i =10;i>-1;i--)
	{
		ret=this->play_logo(this,0,1,i*m_luminance,0x00,i*2*m_luminance);
		if(ret!=E_OK)
			return ret;
		 vTaskDelay(20);
	}
	return E_OK;
}

int play_current_temp(const c_ws2812* this,float temp,int state)
{
	uint8_t grb[3]={0};
	int ret;
	if(HEATING_MODE==state)
	{
		grb[0]=0x01;
		grb[1]=0x06;
		grb[2]=0x00;
	}
	else if(COOLING_MODE==state)
	{
		grb[0]=0x02;
		grb[1]=0x00;
		grb[2]=0x05;
	}
	else if(NORMAL_MODE==state)
	{
		grb[0]=0x05;
		grb[1]=0x00;
		grb[2]=0x00;
	}
	ret=this->play_num(this,1,1,grb[0]*m_luminance,grb[1]*m_luminance,grb[2],temp*m_luminance);
	return ret;
}

int play_set_temp(const c_ws2812* this,float* target_temp)
{
	int ret;
	static u8 set_temp_time;
	if(set_temp_time!=0)
	{
		set_temp_time=20;
	}
	else
	{
		set_temp_time=20;
		while(set_temp_time!=0)
		{
			if(set_temp_time%2==0)
			{
				ret=this->play_num(this,1,1,0x05*m_luminance,0x01*m_luminance,0x02*m_luminance,*target_temp);
				if(ret!=E_OK)
					return ret;
				vTaskDelay(500);
			}
			else
			{
				ret=this->clear(this);
				if(ret!=E_OK)
					return ret;
				vTaskDelay(500);
			}
			set_temp_time--;
		}
	}
	return E_OK;
}

int lack_warn(const c_ws2812* this)
{
	int ret;

	ret=this->play_drop(this,5,1,0x00,0x05*m_luminance,0x00);
	if(ret!=E_OK)
		return ret;
	vTaskDelay(700);

	ret=this->clear(this);
	if(ret!=E_OK)
		return ret;
	
	return E_OK;
}

int fill_water(const c_ws2812* this)
{
	int ret;
	ret=this->play_drop(this,5,1,0x02*m_luminance,0x00,0x05*m_luminance);
	vTaskDelay(3000);
	return ret;
}
int set_luminance(const c_ws2812* this,u8 luminance)
{
	/*参数检测*/
	if(1 > luminance || 10 < luminance)
	{
			log_error("The luminance range is incorrect.");
			return E_ERROR;
	}
	m_luminance=luminance;
	return E_OK;
}