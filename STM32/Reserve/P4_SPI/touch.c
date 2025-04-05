#include "driver_conf.h"

#ifdef DRIVER_TOUCH_ENABLED

#define MODULE_NAME       "touch"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

u8 CMD_RDX_buf=0XD0;
u8 CMD_RDY_buf=0X90;

u8 CMD_RDX;
u8 CMD_RDY;

typedef struct __M_TOUCH
{
    u8            m_spi_channal ;  /*spiͨ��*/
    GPIO_TypeDef* m_pen_gpio    ;  /*gpio*/
		uint32_t      m_pen_pin     ;  /*pin*/
    c_switch      m_cs          ;  /*Ƭѡ��*/
    u16           m_x       		;  /*x����*/
    u16           m_y  					;  /*y����*/
    u8            m_type        ;  /*����������*/
    u8            m_sta 				;  /*�ʵ�״̬*/
		float					m_xfac					;	 /*У׼ֵ*/			
		float					m_yfac					;
		short					m_xoff					;
		short					m_yoff					;	   
}m_touch;
static u8 m_scan(const c_touch* this);
static int m_get_state(const c_touch* this,u8* state);
static int m_get_xy(const c_touch* this,u16* x,u16* y);
static u8 m_read_xy2(const c_touch* this,u16 *x,u16 *y);
static u8 m_read_xy(const c_touch* this,u16 *x,u16 *y);
static u16 m_read_xoy(const c_touch* this,u8 cmd);
static u16 m_read_ad(const c_touch* this,u8 CMD);
c_touch touch_create(u8 spi_channal,u8 type,GPIO_TypeDef* pen_gpio,uint32_t pen_pin,GPIO_TypeDef* cs_gpio,uint32_t cs_pin)
{
    int ret = 0;
    c_touch  new = {0};
    m_touch* m_this = NULL;  
    /*�������*/
    if(TFT18_DIR_V_POSITIVE != type && TFT18_DIR_V_REVERSE != type && TFT18_DIR_H_POSITIVE != type && TFT18_DIR_H_REVERSE != type)
    {
        log_error("Param error.");
        return new;
    }

    /*Ϊ�¶��������ڴ�*/
    new.this = pvPortMalloc(sizeof(m_touch));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_touch));
    m_this = new.this;

    /*��������*/
		m_this->m_pen_gpio=pen_gpio;
		m_this->m_pen_pin=pen_pin;
		
		GPIO_InitTypeDef GPIO_Initure;

    GPIO_Initure.Pin   = m_this->m_pen_pin       ;
    GPIO_Initure.Mode  = GPIO_MODE_INPUT ;  //����
    GPIO_Initure.Pull  = GPIO_PULLUP         ;  //����
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;  //����
    HAL_GPIO_Init(m_this->m_pen_gpio,&GPIO_Initure);  


		
    m_this->m_cs = switch_create(cs_gpio,cs_pin);
    if(NULL == m_this->m_cs.this)
    {
        log_error("Switch creat failed.");
        goto error_handle;
    }

    /*�����������*/
    if(TFT18_DIR_V_POSITIVE == type || TFT18_DIR_V_REVERSE == type)
    {
        CMD_RDX=CMD_RDX_buf;
        CMD_RDY=CMD_RDY_buf;
    }
    else 
    {
        CMD_RDX=CMD_RDY_buf;
        CMD_RDY=CMD_RDX_buf;
    }
    m_this->m_spi_channal = spi_channal;
    m_this->m_type        = type       ;
    new.scan              = m_scan     ;
    new.get_state         = m_get_state;
    new.get_xy            = m_get_xy   ;

		if(type==TFT18_DIR_V_REVERSE)
		{//***���������β�ͬ��ԭ��Ĭ�ϵ�У׼����ֵ���ܻ�������ʶ��׼������У׼����ʹ�ã�������ʹ�ù̶���Ĭ��У׼����
			m_this->m_xfac=-0.035584;
			m_this->m_xoff=136;
			m_this->m_yfac=-0.043796;
			m_this->m_yoff=169;
		}else if(type==TFT18_DIR_V_POSITIVE)
		{//***���������β�ͬ��ԭ��Ĭ�ϵ�У׼����ֵ���ܻ�������ʶ��׼������У׼����ʹ�ã�������ʹ�ù̶���Ĭ��У׼����
			m_this->m_xfac=0.035527;
			m_this->m_xoff=-7;
			m_this->m_yfac=0.042689;
			m_this->m_yoff=-6;
		}else if(type==TFT18_DIR_H_POSITIVE)
		{//***���������β�ͬ��ԭ��Ĭ�ϵ�У׼����ֵ���ܻ�������ʶ��׼������У׼����ʹ�ã�������ʹ�ù̶���Ĭ��У׼����
			m_this->m_xfac=-0.045232;
			m_this->m_xoff=171;
			m_this->m_yfac=0.039321;
			m_this->m_yoff=-13;
		}else
		{//***���������β�ͬ��ԭ��Ĭ�ϵ�У׼����ֵ���ܻ�������ʶ��׼������У׼����ʹ�ã�������ʹ�ù̶���Ĭ��У׼����
			m_this->m_xfac=0.045627;
			m_this->m_xoff=-11;
			m_this->m_yfac=-0.037006;
			m_this->m_yoff=137;
		}
		
		
    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

u8 m_scan(const c_touch* this)
{			   
	m_touch* m_this = NULL;
	int ret = 0;

	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	SWITCH_TYPE pen_state=GPIO_PIN_RESET;
	pen_state = HAL_GPIO_ReadPin(m_this->m_pen_gpio,m_this->m_pen_pin);

	if(pen_state==0)//�а�������
	{
		if(m_read_xy2(this,&m_this->m_x,&m_this->m_y))//��ȡ��Ļ����
		{
	 		m_this->m_x=m_this->m_xfac*m_this->m_x+m_this->m_xoff;//�����ת��Ϊ��Ļ����
			m_this->m_y=m_this->m_yfac*m_this->m_y+m_this->m_yoff;  
	 	} 
				   
	}
	else
	{
		if(m_this->m_sta&TP_PRES_DOWN)//֮ǰ�Ǳ����µ�
		{
			m_this->m_sta&=~(1<<7);//��ǰ����ɿ�
		}else//֮ǰ��û�б�����
		{
			m_this->m_x=0xffff;
			m_this->m_y=0xffff;
		}	    
	}
	return m_this->m_sta&TP_PRES_DOWN;//���ص�ǰ�Ĵ���״̬
}

int m_get_state(const c_touch* this,u8* state)
{
	m_touch* m_this = NULL;
	int ret = 0;

	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	*state=m_this->m_sta;
}

int m_get_xy(const c_touch* this,u16* x,u16* y)
{
m_touch* m_this = NULL;
	int ret = 0;

	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	*x=m_this->m_x;
	*y=m_this->m_y;
}

#define ERR_RANGE 50 //��Χ 
u8 m_read_xy2(const c_touch* this,u16 *x,u16 *y) 
{
	m_touch* m_this = NULL;
	int ret = 0;

	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	u16 x1,y1;
 	u16 x2,y2;
 	u8 flag;    
	flag=m_read_xy(this,&x1,&y1);   
	if(flag!=E_OK)
		return E_ERROR;
	
	flag=m_read_xy(this,&x2,&y2);	   
	if(flag!=E_OK)
		return E_ERROR;
	
	if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//ǰ�����β�����+-50��
		&&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
	{
		*x=(x1+x2)/2;
		*y=(y1+y2)/2;
		return E_OK;
	}
	else 
		return E_ERROR;	  
}  

u8 m_read_xy(const c_touch* this,u16 *x,u16 *y)
{
	m_touch* m_this = NULL;
	int ret = 0;

	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	u16 xtemp,ytemp;
	xtemp=m_read_xoy(this,CMD_RDX);
	ytemp=m_read_xoy(this,CMD_RDY);
	*x=xtemp;
	*y=ytemp;
	return E_OK;//�����ɹ�
}

#define READ_TIMES 5 	//��ȡ����
#define LOST_VAL 1	  	//����ֵ
u16 m_read_xoy(const c_touch* this,u8 cmd)
{
	m_touch* m_this = NULL;
	int ret = 0;

	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	u16 i, j;
	u16 buf[READ_TIMES];
	u16 sum=0;
	u16 temp;
	for(i=0;i<READ_TIMES;i++)buf[i]=m_read_ad(this,cmd);		 		    
	for(i=0;i<READ_TIMES-1; i++)//����
	{
		for(j=i+1;j<READ_TIMES;j++)
		{
			if(buf[i]>buf[j])//��������
			{
				temp=buf[i];
				buf[i]=buf[j];
				buf[j]=temp;
			}
		}
	}	  
	sum=0;
	for(i=LOST_VAL;i<READ_TIMES-LOST_VAL;i++)sum+=buf[i];
	temp=sum/(READ_TIMES-2*LOST_VAL);
	return temp;   
} 

u16 m_read_ad(const c_touch* this,u8 CMD)	  
{ 	 
	m_touch* m_this = NULL;
	int ret = 0;

	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	
	
	u8 count=0; 	  
	u16 Num=0; 
	
	/*ѡ��Ƭѡ*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_RESET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}
	
	/*��������*/
	ret = my_spi.transmission(m_this->m_spi_channal,&CMD,NULL,1,false,3000);
	if(E_OK != ret)
	{
			log_error("Spi transmission failed.");
			return E_ERROR;
	}

	/*��������λ��*/
  ret = my_spi.set_datasize(m_this->m_spi_channal,16);
  if(E_OK != ret)
  {
      log_error("Datasize set failed.");
      return E_ERROR;
  }
	
	ret = my_spi.transmission(m_this->m_spi_channal,NULL,(u8*)&Num,1,false,3000);
	if(E_OK != ret)
	{
			log_error("Spi transmission failed.");
			return E_ERROR;
	}
 	
	Num>>=4;   	//ֻ�и�12λ��Ч.
    
	/*��λƬѡ*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}
	return(Num);   
}
#endif