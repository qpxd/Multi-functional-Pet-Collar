
#include "driver_conf.h"
#include "oledfont.h"

#ifdef DRIVER_OLED_ENABLED

#if 1
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//  文 件 名   : main.c
//  版 本 号   : v2.0
//  作    者   : Evk123
//  生成日期   : 2014-0101
//  最近修改   : 
//  功能描述   : 0.69寸OLED 接口演示例程(STM32F103ZE系列IIC)
//              说明: 
//              ----------------------------------------------------------------
//              GND   电源地
//              VCC   接5V或3.3v电源
//              SCL   接PD6（SCL）
//              SDA   接PD7（SDA）            
//              ----------------------------------------------------------------
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////�

#include "oled.h"
#include "stdlib.h"
#include "oledfont.h"  	 
#include "delay.h"
//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127	
//[1]0 1 2 3 ... 127	
//[2]0 1 2 3 ... 127	
//[3]0 1 2 3 ... 127	
//[4]0 1 2 3 ... 127	
//[5]0 1 2 3 ... 127	
//[6]0 1 2 3 ... 127	
//[7]0 1 2 3 ... 127 			   
/**********************************************
//IIC Start
**********************************************/
/**********************************************
//IIC Start
**********************************************/
void IIC_Start()
{

	OLED_SCLK_Set() ;
	OLED_SDIN_Set();
	OLED_SDIN_Clr();
	OLED_SCLK_Clr();
}

/**********************************************
//IIC Stop
**********************************************/
void IIC_Stop()
{
OLED_SCLK_Set() ;
//	OLED_SCLK_Clr();
	OLED_SDIN_Clr();
	OLED_SDIN_Set();
	
}

void IIC_Wait_Ack()
{

	//GPIOB->CRH &= 0XFFF0FFFF;	//设置PB12为上拉输入模式
	//GPIOB->CRH |= 0x00080000;
//	OLED_SDA = 1;
//	delay_us(1);
	//OLED_SCL = 1;
	//delay_us(50000);
/*	while(1)
	{
		if(!OLED_SDA)				//判断是否接收到OLED 应答信号
		{
			//GPIOB->CRH &= 0XFFF0FFFF;	//设置PB12为通用推免输出模式
			//GPIOB->CRH |= 0x00030000;
			return;
		}
	}
*/
	OLED_SCLK_Set() ;
	OLED_SCLK_Clr();
}
/**********************************************
// IIC Write byte
**********************************************/

void Write_IIC_Byte(unsigned char IIC_Byte)
{
	unsigned char i;
	unsigned char m,da;
	da=IIC_Byte;
	OLED_SCLK_Clr();
	for(i=0;i<8;i++)		
	{
			m=da;
		//	OLED_SCLK_Clr();
		m=m&0x80;
		if(m==0x80)
		{OLED_SDIN_Set();}
		else OLED_SDIN_Clr();
			da=da<<1;
		OLED_SCLK_Set();
		OLED_SCLK_Clr();
		}


}
/**********************************************
// IIC Write Command
**********************************************/
void Write_IIC_Command(unsigned char IIC_Command)
{
   IIC_Start();
   Write_IIC_Byte(0x78);            //Slave address,SA0=0
	IIC_Wait_Ack();	
   Write_IIC_Byte(0x00);			//write command
	IIC_Wait_Ack();	
   Write_IIC_Byte(IIC_Command); 
	IIC_Wait_Ack();	
   IIC_Stop();
}
/**********************************************
// IIC Write Data
**********************************************/
void Write_IIC_Data(unsigned char IIC_Data)
{
   IIC_Start();
   Write_IIC_Byte(0x78);			//D/C#=0; R/W#=0
	IIC_Wait_Ack();	
   Write_IIC_Byte(0x40);			//write data
	IIC_Wait_Ack();	
   Write_IIC_Byte(IIC_Data);
	IIC_Wait_Ack();	
   IIC_Stop();
}
void OLED_WR_Byte(unsigned dat,unsigned cmd)
{
	if(cmd)
			{

   Write_IIC_Data(dat);
   
		}
	else {
   Write_IIC_Command(dat);
		
	}


}


/********************************************
// fill_Picture
********************************************/
void fill_picture(unsigned char fill_Data)
{
	unsigned char m,n;
	for(m=0;m<8;m++)
	{
		OLED_WR_Byte(0xb0+m,0);		//page0-page1
		OLED_WR_Byte(0x00,0);		//low column start address
		OLED_WR_Byte(0x10,0);		//high column start address
		for(n=0;n<128;n++)
			{
				OLED_WR_Byte(fill_Data,1);
			}
	}
}


/***********************Delay****************************************/
void Delay_50ms(unsigned int Del_50ms)
{
	unsigned int m;
	for(;Del_50ms>0;Del_50ms--)
		for(m=6245;m>0;m--);
}

void Delay_1ms(unsigned int Del_1ms)
{
	unsigned char j;
	while(Del_1ms--)
	{	
		for(j=0;j<123;j++);
	}
}

//坐标设置

	void OLED_Set_Pos(unsigned char x, unsigned char y) 
{ 	OLED_WR_Byte(0xb0+y,OLED_CMD);
	OLED_WR_Byte(((x&0xf0)>>4)|0x10,OLED_CMD);
	OLED_WR_Byte((x&0x0f),OLED_CMD); 
}   	  
//开启OLED显示    
void OLED_Display_On(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X14,OLED_CMD);  //DCDC ON
	OLED_WR_Byte(0XAF,OLED_CMD);  //DISPLAY ON
}
//关闭OLED显示     
void OLED_Display_Off(void)
{
	OLED_WR_Byte(0X8D,OLED_CMD);  //SET DCDC命令
	OLED_WR_Byte(0X10,OLED_CMD);  //DCDC OFF
	OLED_WR_Byte(0XAE,OLED_CMD);  //DISPLAY OFF
}		   			 
//清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!	  
void OLED_Clear(void)  
{  
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		for(n=0;n<128;n++)OLED_WR_Byte(0,OLED_DATA); 
	} //更新显示
}
void OLED_On(void)  
{  
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{  
		OLED_WR_Byte (0xb0+i,OLED_CMD);    //设置页地址（0~7）
		OLED_WR_Byte (0x00,OLED_CMD);      //设置显示位置—列低地址
		OLED_WR_Byte (0x10,OLED_CMD);      //设置显示位置—列高地址   
		for(n=0;n<128;n++)OLED_WR_Byte(1,OLED_DATA); 
	} //更新显示
}
//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示				 
//size:选择字体 16/12 
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 Char_Size)
{      	
	unsigned char c=0,i=0;	
		c=chr-' ';//得到偏移后的值			
		if(x>Max_Column-1){x=0;y=y+2;}
		if(Char_Size ==16)
			{
			OLED_Set_Pos(x,y);	
			for(i=0;i<8;i++)
			OLED_WR_Byte(F8X16[c*16+i],OLED_DATA);
			OLED_Set_Pos(x,y+1);
			for(i=0;i<8;i++)
			OLED_WR_Byte(F8X16[c*16+i+8],OLED_DATA);
			}
			else {	
				OLED_Set_Pos(x,y);
				for(i=0;i<6;i++)
				OLED_WR_Byte(F6x8[c][i],OLED_DATA);
				
			}
}
//m^n函数
u32 oled_pow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}				  
//显示2个数字
//x,y :起点坐标	 
//len :数字的位数
//size:字体大小
//mode:模式	0,填充模式;1,叠加模式
//num:数值(0~4294967295);	 		  
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size2)
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/oled_pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				OLED_ShowChar(x+(size2/2)*t,y,' ',size2);
				continue;
			}else enshow=1; 
		 	 
		}
	 	OLED_ShowChar(x+(size2/2)*t,y,temp+'0',size2); 
	}
} 
//显示一个字符号串
void OLED_ShowString(u8 x,u8 y,u8 *chr,u8 Char_Size)
{
	unsigned char j=0;
	while (chr[j]!='\0')
	{		OLED_ShowChar(x,y,chr[j],Char_Size);
			x+=8;
		if(x>120){x=0;y+=2;}
			j++;
	}
}

////显示汉字
//void OLED_ShowCHinese(u8 x,u8 y,u8 no)
//{      			    
//	u8 t,adder=0;
//	OLED_Set_Pos(x,y);	
//    for(t=0;t<16;t++)
//		{
//				OLED_WR_Byte(Hzk[2*no][t],OLED_DATA);
//				adder+=1;
//     }	
//		OLED_Set_Pos(x,y+1);	
//    for(t=0;t<16;t++)
//			{	
//				OLED_WR_Byte(Hzk[2*no+1][t],OLED_DATA);
//				adder+=1;
//      }					
//}
/***********功能描述：显示显示BMP图片128×64起始点坐标(x,y),x的范围0～127，y为页的范围0～7*****************/
void OLED_DrawBMP(unsigned char x0, unsigned char y0,unsigned char x1, unsigned char y1,unsigned char BMP[])
{ 	
 unsigned int j=0;
 unsigned char x,y;
  
  if(y1%8==0) y=y1/8;      
  else y=y1/8+1;
	for(y=y0;y<y1;y++)
	{
		OLED_Set_Pos(x0,y);
    for(x=x0;x<x1;x++)
	    {      
	    	OLED_WR_Byte(BMP[j++],OLED_DATA);	    	
	    }
	}
} 

//初始化SSD1306					    
void OLED_Init(void)
{ 	
	GPIO_InitTypeDef GPIO_Initure = {0};
    GPIO_Initure.Pin   = GPIO_PIN_6|GPIO_PIN_7 ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP   ;  //推挽输出
    GPIO_Initure.Pull  = GPIO_NOPULL           ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH  ;  //高速
    HAL_GPIO_Init(GPIOB,&GPIO_Initure)     ;  
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);  /*默认输出高电平*/
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);  /*默认输出高电平*/
	
// 	GPIO_InitTypeDef  GPIO_InitStructure;
//	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	 //使能A端口时钟
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_7;	 
// 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
//	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//速度50MHz
// 	GPIO_Init(GPIOA, &GPIO_InitStructure);	  //初始化GPIOD3,6
// 	GPIO_SetBits(GPIOA,GPIO_Pin_5|GPIO_Pin_7);	

	delay_ms(800);
	OLED_WR_Byte(0xAE,OLED_CMD);//--display off
	OLED_WR_Byte(0x00,OLED_CMD);//---set low column address
	OLED_WR_Byte(0x10,OLED_CMD);//---set high column address
	OLED_WR_Byte(0x40,OLED_CMD);//--set start line address  
	OLED_WR_Byte(0xB0,OLED_CMD);//--set page address
	OLED_WR_Byte(0x81,OLED_CMD); // contract control
	OLED_WR_Byte(0xFF,OLED_CMD);//--128   
	OLED_WR_Byte(0xA1,OLED_CMD);//set segment remap 
	OLED_WR_Byte(0xA6,OLED_CMD);//--normal / reverse
	OLED_WR_Byte(0xA8,OLED_CMD);//--set multiplex ratio(1 to 64)
	OLED_WR_Byte(0x3F,OLED_CMD);//--1/32 duty
	OLED_WR_Byte(0xC8,OLED_CMD);//Com scan direction
	OLED_WR_Byte(0xD3,OLED_CMD);//-set display offset
	OLED_WR_Byte(0x00,OLED_CMD);//
	
	OLED_WR_Byte(0xD5,OLED_CMD);//set osc division
	OLED_WR_Byte(0x80,OLED_CMD);//
	
	OLED_WR_Byte(0xD8,OLED_CMD);//set area color mode off
	OLED_WR_Byte(0x05,OLED_CMD);//
	
	OLED_WR_Byte(0xD9,OLED_CMD);//Set Pre-Charge Period
	OLED_WR_Byte(0xF1,OLED_CMD);//
	
	OLED_WR_Byte(0xDA,OLED_CMD);//set com pin configuartion
	OLED_WR_Byte(0x12,OLED_CMD);//
	
	OLED_WR_Byte(0xDB,OLED_CMD);//set Vcomh
	OLED_WR_Byte(0x30,OLED_CMD);//
	
	OLED_WR_Byte(0x8D,OLED_CMD);//set charge pump enable
	OLED_WR_Byte(0x14,OLED_CMD);//
	
	OLED_WR_Byte(0xAF,OLED_CMD);//--turn on oled panel
}  

#endif

#if 0
#define MODULE_NAME       "oled"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define OLED_IIC_SPEED            50

#define OLED_CMD               0x00	//写命令
#define OLED_DATA              0x40	//写数据
#define OLED_DEFAULT_IIC_ADDR  0x78 /*显示屏 地址*/
#define OLED_DISPLAY_ON        0xAF /*开显示*/
#define OLED_DISPLAY_OFF       0xAE /*关显示*/

#define FONT_SIZE    16
#define X_WIDTH 	128  /*宽度*/
#define Y_WIDTH 	 64	 /*高度*/

#define IIC_Start()      SMBus_StartBit()
#define IIC_Stop()       SMBus_StopBit()
#define IIC_Wait_Ack()   
#define Write_IIC_Byte(dat) SMBus_SendByte(dat)

typedef struct __M_OLED
{
    const c_my_iic*    m_iic          ;  /*iic对象*/
    u8                 m_iic_addr     ;  /*iic地址*/
    u8                 m_type         ;  /*屏幕类型*/
    u8                 m_font_width   ;  /*字宽*/
    SemaphoreHandle_t  m_semaphore    ;  /*互斥信号量 */
}m_oled;

static int m_display_clear (const c_oled* this);  
static int m_display_str   (const c_oled* this,u8 x,u8 y, char* format,...);
static int m_display_char  (const c_oled* this,u8 x,u8 y, u8 cha);
static int m_display_on    (const c_oled* this);
static int m_display_off   (const c_oled* this);
static int m_set_pos       (const c_oled* this,u8 x,u8 y);

static int m_oled_write    (const c_oled* this,const u8* dat,u8 len,u8 type);\

static const u8 g_cmd_dis_clear  [X_WIDTH] ;
const u8 g_init_cmd[] = {0xAE,0x02,0x10,0x40,0xB0,0x81,0xFF,0xA1,0xA6,
                         0xA8,0x3F,0xC8,0xD3,0x00,0xD5,0x80,0xD8,0x05,
                         0xD9,0xF1,0xDA,0x12,0xDB,0x30,0x8D,0x14,0xAF};

c_oled oled_create(c_my_iic* iic,u8 type)
{
    c_oled new = {0};
    m_oled* m_this = NULL;
    int    ret = 0;
    
    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }

    if(OLED_TYPE_96 != type && OLED_TYPE_91 != type)
    {
        log_error("Param error.");
        return new;
    }

    /*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_oled));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_oled));
	m_this = new.this;

	/*为新对象 创建互斥信号量*/
	m_this->m_semaphore = xSemaphoreCreateMutex();
	if(NULL == m_this->m_semaphore)
	{
		log_error("Mutex semaphore creat failed.");
		goto error_handle;
	}

     /*保存相关变量*/
    new.display_str   = m_display_str   ;
    new.display_clear = m_display_clear ;
    new.display_on    = m_display_on    ;
    new.display_off   = m_display_off   ;
    m_this->m_iic        = iic;
    m_this->m_type       = type;
    m_this->m_font_width = 8;
    m_this->m_iic_addr   = OLED_DEFAULT_IIC_ADDR;
    
    /*oled 初始化命令*/
    ret = m_oled_write(&new,g_init_cmd,sizeof(g_init_cmd)/sizeof(u8),OLED_CMD);
    if(E_OK != ret)
    {
        log_error("OLED cmd init failed.");
        goto error_handle;
    }
   
    return new;
    
    error_handle:
    vPortFree(m_this);
    new.this = NULL;
    
    return new;
}



//OLED的显存
//存放格式如下.
//[0]0 1 2 3 ... 127    
//[1]0 1 2 3 ... 127    
//[2]0 1 2 3 ... 127    
//[3]0 1 2 3 ... 127    
//[4]0 1 2 3 ... 127    
//[5]0 1 2 3 ... 127    
//[6]0 1 2 3 ... 127    
//[7]0 1 2 3 ... 127               

/**********************************************
// IIC Write Command
**********************************************/
static int m_oled_write(const c_oled* this,const u8* dat,u8 len,u8 type)
{
    iic_cmd_handle_t  cmd = NULL;
    int  ret = 0;
    const m_oled* m_this = this->this;
    u8   iic_addr = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*创建新的命令链接*/
    cmd = iic_cmd_link_create();
    if(NULL == cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }

    /*放入启动命令*/
    ret = iic_master_start(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }

    /*写入地址*/
    iic_addr = m_this->m_iic_addr | I2C_MASTER_WRITE;  /*写地址*/
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*写入 数据类型*/
    ret = iic_write_bytes(cmd,&type,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*写入 数据*/
    ret = iic_write_bytes(cmd,dat,len);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*停止命令*/
    ret = iic_master_stop(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }

    /*启动命令序列*/
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,OLED_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("Add iic begin failed.");
        goto error_handle;
    }

    /*释放命令*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }

    return E_OK;
    
    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR;
}

static int m_display_char   (const c_oled* this,u8 x,u8 y, u8 cha)
{       
    u8 c = 0;  
    int ret = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    
    c = cha-' ';//得到偏移后的值

    /*设置显示坐标*/
    ret = m_set_pos(this,x,y);
    if(E_OK != ret)
    {
        log_error("Set pos failed.");
        return E_ERROR;
    }

    /*字体大小 为16*/
    if(FONT_SIZE ==16)
    { 
        /*写入*/
        ret = m_oled_write(this,(u8*)&F8X16[c*16],8,OLED_DATA);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }    
        
        ret = m_set_pos(this,x,y+1);
        if(E_OK != ret)
        {
            log_error("Set pos failed.");
            return E_ERROR;
        }
        
        ret = m_oled_write(this,(u8*)&F8X16[c*16 + 8],8,OLED_DATA);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }
    }
    else 
    {
        ret = m_oled_write(this,(u8*)&F6x8[c][0],8,OLED_DATA);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }
    }
    
    return E_OK;
}

static int m_display_str   (const c_oled* this,u8 x,u8 y, char* format,...)
{
    u8 str_index = 0;
    const m_oled*  m_this = NULL;
    va_list   argptr;
    char str_buf[20] = {0};
    
    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    /*检查坐标*/
    if(x >= X_WIDTH || y >= Y_WIDTH / FONT_SIZE)
    {
        log_error("Param error");
        return E_ERROR;
    }
    y *= FONT_SIZE / 8;
    
    va_start( argptr, format );  // 初始化argptr	
	memset(str_buf,0,20);
	vsnprintf(str_buf,20,format,argptr);
	va_end(argptr);
    
    
    for(str_index = 0;'\0' != str_buf[str_index];++str_index)
    {
        m_display_char(this,x + str_index * m_this->m_font_width,y,str_buf[str_index]);
    }
    
    
    return E_OK;
}

static int m_set_pos(const c_oled* this,u8 x,u8 y)
{
    int ret = 0;
    u8 dat[3] = {0};

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
  
    dat[0] = 0xb0+y;                 /*设置y坐标*/
    dat[1] = ((x&0xf0)>>4)|0x10; /*设置x坐标*/
    dat[2] = (x&0x0f);
    ret = m_oled_write(this,dat,3,OLED_CMD);
    if(E_OK != ret)
    {
        log_error("Set pos failed.");
        return E_ERROR;
    }
   
    return E_OK;
}

static int m_display_clear(const c_oled* this)
{
    u16 i = 0;         
    int ret = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    for(i = 0;i<8;i++)
    {
        ret = m_set_pos(this,0,i);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }
        ret = m_oled_write(this,g_cmd_dis_clear,X_WIDTH,OLED_DATA);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            return E_ERROR;
        }
    }

    return E_OK;
}

static int m_display_on   (const c_oled* this)
{
    u8 data = OLED_DISPLAY_ON;
    int ret = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*发送开显示命令*/
    ret = m_oled_write(this,&data,1,OLED_CMD);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    return E_OK;
}

static int m_display_off   (const c_oled* this)
{
    u8 data = OLED_DISPLAY_OFF;
    int ret = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*发送关显示命令*/
    ret = m_oled_write(this,&data,1,OLED_CMD);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    return E_OK;
}

#endif

#endif

/*
SSD1306显存：

		SEG0  SEG1 ... SEG127
PAGE0	
PAGE1
PAGE2
PAGE3
PAGE4
PAGE5
PAGE6
PAGE7

8 PAGE， 每页 128个 SEG，每个SEG为一个字节(8bits)， 显存总大小为 128 * 8 bytes
每个bit标志一个像素点的亮或者灭，一个SEG可以表示8个像素点如下：
SEG0
bit0
bit1
bit2
bit3
bit4
bit5
bit6
bit7
所以X向共128个像素，Y向共 8 * 8 = 64个像素 


SSD1306寻址模式：页地址模式，水平地址模式，竖直地址模式
页地址模式：
页面寻址模式下，读写显示RAM后，列地址指针自动增加1。如果列地址指针到达列结束地址(0 - 127)，
则将列地址指针重置为列起始地址，而不对页面地址指针进行更改（PAGE不会改变）。
用户必须设置新的页和列地址，才能访问下一页的RAM内容。


dat[0] = 0xb0+y;              		//设置页地址     （y: 0 - 7）  
dat[1] = ((x&0xf0)>>4)|0x10; 	    //设置列地址高位
dat[2] = (x&0x0f);					//设置列地址低位  (x: 0 - 8)

*/



























