#include "ws2812/ws2812.h"
#include "ws2812/ws2812font.h"
#include "log/log.h"
#include "FreeRTOS.h"
#include "timers.h"
#include "string.h"

#define MODULE_NAME       "ws2812"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

typedef struct __M_WS2812
{
	GPIO_TypeDef* m_gpio     ;  /*gpio*/
	uint32_t      m_pin      ;  /*pin*/
	int           m_num      ;  /*灯珠数量*/
	int           m_height   ;  /*高*/
  int           m_width    ;  /*宽*/
}m_ws2812;
int ws2812_Write0(const c_ws2812* this) __attribute((section(".ARM.__at_0x8008000")));
int ws2812_Write1(const c_ws2812* this) __attribute((section(".ARM.__at_0x8009000")));
static int reset(const c_ws2812* this);
int write_24Bits(const c_ws2812* this,uint8_t green,uint8_t red,uint8_t blue);
int play_num(const c_ws2812* this,u16 x,u16 y,uint8_t green,uint8_t red,uint8_t blue,float num);
int play_logo(const c_ws2812* this,u16 x,u16 y,uint8_t green,uint8_t red,uint8_t blue);
int play_drop(const c_ws2812* this,u16 x,u16 y,uint8_t green,uint8_t red,uint8_t blue);
int clear(const c_ws2812* this);
/*ws2812 初始化*/
c_ws2812 ws2812_create(GPIO_TypeDef* gpio,uint32_t pin,int light_num)
{
	c_ws2812 new = {0};
	GPIO_InitTypeDef GPIO_Initure;
	
	/*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(c_ws2812));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
	memset(new.this,0,sizeof(c_ws2812));

	 //初始化对应的GPIO
	GPIO_Initure.Pin   = pin                 ;
	GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP ;  //输出
	GPIO_Initure.Pull  = GPIO_PULLUP         ;  //上拉
	GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;  //高速
	HAL_GPIO_Init(gpio,&GPIO_Initure);   
	
	/*保存相关变量*/
	((m_ws2812* )new.this)->m_gpio     = gpio       ;
	((m_ws2812* )new.this)->m_pin      = pin        ;
	((m_ws2812* )new.this)->m_num      = light_num  ;
	((m_ws2812* )new.this)->m_height   = 8          ;
	((m_ws2812* )new.this)->m_width    = light_num/8;
	new.reset=reset;
	new.write_24Bits=write_24Bits;
	new.clear=clear;
	new.play_num=play_num;
	new.play_logo=play_logo;
	new.play_drop=play_drop;
		return new;
}

int ws2812_Write0(const c_ws2812* this) 
{
	m_ws2812* m_this = NULL;
	
	/*参数检测*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;

		HAL_GPIO_WritePin(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
		HAL_GPIO_WritePin(m_this->m_gpio,m_this->m_pin,GPIO_PIN_RESET);
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();

	return  E_OK;
}

int ws2812_Write1(const c_ws2812* this)
{
	m_ws2812* m_this = NULL;
	
	/*参数检测*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;

	HAL_GPIO_WritePin(m_this->m_gpio,m_this->m_pin,GPIO_PIN_SET);
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
	__nop();__nop();__nop();
		HAL_GPIO_WritePin(m_this->m_gpio,m_this->m_pin,GPIO_PIN_RESET);
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();
   
	
	return  E_OK;
}

int reset(const c_ws2812* this)
{
	m_ws2812* m_this = NULL;
	
	/*参数检测*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	HAL_GPIO_WritePin(m_this->m_gpio,m_this->m_pin,GPIO_PIN_RESET);
	vTaskDelay(80);
	
	return  E_OK;
}

int ws2812_Write_Byte(const c_ws2812* this,uint8_t byte)
{
	const m_ws2812* m_this = NULL;
	
	/*参数检测*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	uint8_t i;

	for(i=0;i<8;i++)
		{
			if(byte&0x80)
				{
					ws2812_Write1(this);
			}
			else
				{
					ws2812_Write0(this);
			}
		byte <<= 1;
	}
}

int write_24Bits(const c_ws2812* this,uint8_t green,uint8_t red,uint8_t blue)
{
	ws2812_Write_Byte(this,green);
	ws2812_Write_Byte(this,red);
	ws2812_Write_Byte(this,blue);
	return  E_OK;
}

int clear(const c_ws2812* this)
{
	const m_ws2812* m_this = NULL;
	
	/*参数检测*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");                          
			return E_NULL;
	}
	m_this = this->this;
	

	taskENTER_CRITICAL();   //进入临界区

	for(int i=0;i<m_this->m_num;i++)
	{
		write_24Bits(this,0x00,0x00,0x00);
	}
	reset(this);
	taskEXIT_CRITICAL();            //退出临界区   
	return  E_OK;
}

int play_num(const c_ws2812* this,u16 x,u16 y,uint8_t green,uint8_t red,uint8_t blue,float num)
{
	const m_ws2812* m_this = NULL;
	
	/*参数检测*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	if(m_this->m_width-15 < x || m_this->m_height-7 < y)
	{
			log_error("Param error");
        return E_ERROR;
	}
	int decade,unit,tenth;
	decade=(int)(num/10)%10;
	unit=(int)num%10;
	tenth=(int)(num*10)%10;
	
	uint8_t buf[128*3]={0};
	memset(buf,0,sizeof(buf));

#if 1
	//十位数
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<7;j++)
		{
			if(F4x7[decade][i*7+j]==0)
			{
				buf[(1+j-y+8*(i+x))*3]=0x00;
				buf[(1+j-y+8*(i+x))*3+1]=0x00;
				buf[(1+j-y+8*(i+x))*3+2]=0x00;
			}
			else 
			{
				buf[(1+j-y+8*(i+x))*3]=green;
				buf[(1+j-y+8*(i+x))*3+1]=red;
				buf[(1+j-y+8*(i+x))*3+2]=blue;
			}
		}
	}
	
	//个位数
	for(int i=0;i<4;i++)
	{
		for(int j=0;j<7;j++)
		{
			if(F4x7[unit][i*7+j]==0)
			{
				buf[(1+j-y+8*(i+x+5))*3]=0x00;
				buf[(1+j-y+8*(i+x+5))*3+1]=0x00;
				buf[(1+j-y+8*(i+x+5))*3+2]=0x00;
			}
			else 
			{
				buf[(1+j-y+8*(i+x+5))*3]=green;
				buf[(1+j-y+8*(i+x+5))*3+1]=red;
				buf[(1+j-y+8*(i+x+5))*3+2]=blue;
			}
		}
	}

	//小数点
	for(int i=0;i<3;i++)
	{
		for(int j=0;j<5;j++)
		{
			if(F3x5[10][i*5+j]==0)
			{
				buf[(1+j-y+8*(i+x+9))*3]=0x00;
				buf[(1+j-y+8*(i+x+9))*3+1]=0x00;
				buf[(1+j-y+8*(i+x+9))*3+2]=0x00;
			}
			else 
			{
				buf[(1+j-y+8*(i+x+9))*3]=green;
				buf[(1+j-y+8*(i+x+9))*3+1]=red;
				buf[(1+j-y+8*(i+x+9))*3+2]=blue;
			}          
		}
	}

	//十分位
	for(int i=0;i<3;i++)
	{
		for(int j=0;j<5;j++)
		{
			if(F3x5[tenth][i*5+j]==0)
			{
				buf[(1+j-y+8*(i+x+12))*3]=0x00;
				buf[(1+j-y+8*(i+x+12))*3+1]=0x00;
				buf[(1+j-y+8*(i+x+12))*3+2]=0x00;
			}
			else 
			{
				buf[(1+j-y+8*(i+x+12))*3]=green;
				buf[(1+j-y+8*(i+x+12))*3+1]=red;
				buf[(1+j-y+8*(i+x+12))*3+2]=blue;
			}
		}
	}
#endif
	taskENTER_CRITICAL();   //进入临界区

	for(int i=0;i<m_this->m_num;i++)
	{
		write_24Bits(this,buf[3*i],buf[3*i+1],buf[3*i+2]);
	}
	reset(this);
	taskEXIT_CRITICAL();            //退出临界区   
	return E_OK;
}

int play_logo(const c_ws2812* this,u16 x,u16 y,uint8_t green,uint8_t red,uint8_t blue)
{
	const m_ws2812* m_this = NULL;
	
	/*参数检测*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	if(m_this->m_width-16 < x || m_this->m_height-5 < y)
	{
			log_error("Param error");
        return E_ERROR;
	}
	uint8_t buf[128*3]={0};
	memset(buf,0,sizeof(buf));
	for(int i=0;i<16;i++)
	{
		for(int j=0;j<5;j++)
		{
			if(logo[i*5+j]==0)
			{
				buf[(3+j-y+8*(i+x))*3]=0x00;
				buf[(3+j-y+8*(i+x))*3+1]=0x00;
				buf[(3+j-y+8*(i+x))*3+2]=0x00;
			}
			else 
			{
				buf[(3+j-y+8*(i+x))*3]=green;
				buf[(3+j-y+8*(i+x))*3+1]=red;
				buf[(3+j-y+8*(i+x))*3+2]=blue;
			}
		}
	}
	taskENTER_CRITICAL();   //进入临界区

	for(int i=0;i<m_this->m_num;i++)
	{
		write_24Bits(this,buf[3*i],buf[3*i+1],buf[3*i+2]);
	}
	reset(this);
	taskEXIT_CRITICAL();            //退出临界区   
	return E_OK;
}

int play_drop(const c_ws2812* this,u16 x,u16 y,uint8_t green,uint8_t red,uint8_t blue)
{
	const m_ws2812* m_this = NULL;
	
	/*参数检测*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	if(m_this->m_width-6 < x || m_this->m_height-7 < y)
	{
			log_error("Param error");
        return E_ERROR;
	}
	uint8_t buf[128*3]={0};
	memset(buf,0,sizeof(buf));
	for(int i=0;i<6;i++)
	{
		for(int j=0;j<7;j++)
		{
			if(drop[i*7+j]==0)
			{
				buf[(1+j-y+8*(i+x))*3]=0x00;
				buf[(1+j-y+8*(i+x))*3+1]=0x00;
				buf[(1+j-y+8*(i+x))*3+2]=0x00;
			}
			else 
			{
				buf[(1+j-y+8*(i+x))*3]=green;
				buf[(1+j-y+8*(i+x))*3+1]=red;
				buf[(1+j-y+8*(i+x))*3+2]=blue;
			}
		}
	}
	taskENTER_CRITICAL();   //进入临界区

	for(int i=0;i<m_this->m_num;i++)
	{
		write_24Bits(this,buf[3*i],buf[3*i+1],buf[3*i+2]);
	}
	reset(this);
	taskEXIT_CRITICAL();            //退出临界区   
	return E_OK;
}