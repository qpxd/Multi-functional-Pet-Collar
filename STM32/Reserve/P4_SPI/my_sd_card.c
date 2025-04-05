#include "driver_conf.h"

#ifdef DRIVER_SD_CARD_ENABLED

#define MODULE_NAME       "sd_card"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME		
u8  SD_Type=0;//SD卡的类型 
typedef struct	__M_SD_CARD
{
	c_switch			m_cs;
  u8            m_spi_channal ;  /*spi通道*/	
}m_sd_card;
static int m_WaitReady(const c_sd_card* this);
static int sd_card_SendCmd(const c_sd_card* this,u8 cmd, u32 arg, u8 crc);
static int sd_card_ReadWriteByte(const c_sd_card* this,u8 send_data);

static int SD_Select(const c_sd_card* this);
static int SD_DisSelect(const c_sd_card* this);

static int m_init(const c_sd_card* this);
static int m_GetCSD(const c_sd_card* this,u8 *csd_data);
static int m_GetCID(const c_sd_card* this,u8 *cid_data);
static int m_GetResponse(const c_sd_card* this,u8 Response);

static int m_GetSectorCount(const c_sd_card* this);
static int m_ReadData(const c_sd_card* this,u8*buf,u32 sector,u8 cnt);
static int m_WriteData(const c_sd_card* this,u8*buf,u32 sector,u8 cnt);

static int m_ReadDisk(const c_sd_card* this,u8*buf,u16 len);
static int m_WriteDisk(const c_sd_card* this,u8*buf,u8 cmd);
	
c_sd_card sd_card_create(u8 spi_channal,GPIO_TypeDef* cs_gpio,uint32_t cs_pin)
{
	c_sd_card  new = {0};
	m_sd_card* m_this = NULL; 	
	int ret=0;
	/*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_sd_card));
	if(NULL == new.this)
	{
			log_error("Out of memory");
			return new;
	}
	memset(new.this,0,sizeof(m_sd_card));
	m_this = new.this;

	/*初始化相应的SPI*/
	ret = my_spi.init(spi_channal);
	if(E_OK != ret)
	{
			log_error("Spi init failed.");
			goto error_handle;
	}
	m_this->m_cs = switch_create(cs_gpio,cs_pin);
	if(NULL == m_this->m_cs.this)
	{
			log_error("Switch creat failed.");
			goto error_handle;
	}
	/*保存相关配置*/
	m_this->m_spi_channal = spi_channal;
	new.init=m_init;
	new.GetSectorCount=m_GetSectorCount;
	new.GetCSD=m_GetCSD;
	new.GetCID=m_GetCID;
	new.GetResponse=m_GetResponse;
	new.ReadData=m_ReadData;
	new.WriteData=m_WriteData;
	new.ReadDisk=m_ReadDisk;
	new.WriteDisk=m_WriteDisk;
	new.WaitReady=m_WaitReady;
	 				
	return new;	
	error_handle:

	vPortFree(new.this);
	new.this = NULL;
	return new;	
}

static int m_init(const c_sd_card* this)
{
	m_sd_card* m_this = NULL;
	int ret = 0;
	u8 recv_data = 0;/*spi返回值*/
	u8 send_data=0;
	u8 i=0;
	u16 retry=20;  /*用来进行超时计数*/
	u8 buf[4]; 
	/*参数检查*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	/*取消片选*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	
//	/*拉低spi速度*/
//	my_spi.set_speed(m_this->m_spi_channal,SPI_BAUDRATEPRESCALER_256);
	

	/*发送最少74个脉冲*/
	send_data=0XFF;
	for(i=0;i<10;i++)
	{
		my_spi.set_datasize(m_this->m_spi_channal, 8);	
		my_spi.transmission(m_this->m_spi_channal,&send_data,&recv_data,1,false,1000);		
	}
	do
	{
		send_data=0x95;
		recv_data=sd_card_SendCmd(this,CMD0,0,send_data);//进入IDLE状态
	}
	while((recv_data!=0X01) && retry--);
	if(recv_data==0X01)
	{
		log_inform("sd_card reset successfully");  
		recv_data=sd_card_SendCmd(this,CMD8,0x1AA,0x87);
		if(recv_data==1)//SD V2.0
		{
			for(i=0;i<4;i++)
			{
				buf[i]=sd_card_ReadWriteByte(this,0XFF);	//Get trailing return value of R7 resp				
			}
			if(buf[2]==0X01&&buf[3]==0XAA)//卡是否支持2.7~3.6V
			{
				retry=0XFFFE;
				do
				{
					sd_card_SendCmd(this,CMD55,0,0X01);	//发送CMD55
					recv_data=sd_card_SendCmd(this,CMD41,0x40000000,0X01);//发送CMD41
				}while(recv_data&&retry--);
				recv_data=sd_card_SendCmd(this,CMD58,0,0X01);
				if(retry&&recv_data==0)//鉴别SD2.0卡版本开始
				{
					for(i=0;i<4;i++)
					{
						buf[i]=sd_card_ReadWriteByte(this,0XFF);//得到OCR值
					}
					if(buf[0]&0x40)	
					{
						SD_Type=SD_TYPE_V2HC;    //检查CCS
						log_inform("SD card is V2HC"); 						
					}											
					else 
					{
						SD_Type=SD_TYPE_V2;
						log_inform("SD card is V2.0"); 						
					}
 
				}
			}
		}else//SD V1.x/ MMC	V3
		{ 		 
			sd_card_SendCmd(this,CMD55,0,0X01);		//发送CMD55
			recv_data=sd_card_SendCmd(this,CMD41,0,0X01);	//发送CMD41
			if(recv_data<=1)
			{		
				SD_Type=SD_TYPE_V1;
				retry=0XFFFE;
				do //等待退出IDLE模式
				{
					sd_card_SendCmd(this,CMD55,0,0X01);	//发送CMD55
					recv_data=sd_card_SendCmd(this,CMD41,0,0X01);//发送CMD41
				}while(recv_data&&retry--);
				log_inform("SD card is V1.0");
			}
			else//MMC卡不支持CMD55+CMD41识别
			{
				SD_Type=SD_TYPE_MMC;//MMC V3
				retry=0XFFFE;
				do //等待退出IDLE模式
				{											    
					recv_data=sd_card_SendCmd(this,CMD1,0,0X01);//发送CMD1
				}while(recv_data&&retry--);
				log_inform("SD card is MMC");				
			}
			recv_data=sd_card_SendCmd(this,CMD16,512,0X01);
			if(retry==0||recv_data!=0)
			{
				SD_Type=SD_TYPE_ERR;//错误的卡		
				log_error("sd_card	error!");
			}
		}
		delay_ms(1000);	
	}
	/*取消片选*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	

//	/*恢复spi速度*/
//	my_spi.set_speed(m_this->m_spi_channal,SPI_BAUDRATEPRESCALER_4);
	if(SD_Type)
		return E_OK;
	else
		return E_ERROR; 
}
/*输入: u8 cmd   命令 */
/*      u32 arg  命令参数*/
/*      u8 crc   crc校验值*/   
/*返回值:SD卡返回的响应*/		
static int sd_card_SendCmd(const c_sd_card* this,u8 cmd, u32 arg, u8 crc)
{
	u8 recv_data=0;
	u8 Retry=0X1F;
	/*参数检查*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	SD_DisSelect(this);
	if(SD_Select(this))
	return E_ERROR;
	sd_card_ReadWriteByte(this,cmd | 0x40);//分别写入命令
	sd_card_ReadWriteByte(this,arg >> 24);
	sd_card_ReadWriteByte(this,arg >> 16);
	sd_card_ReadWriteByte(this,arg >> 8);
	sd_card_ReadWriteByte(this,arg);	  
	sd_card_ReadWriteByte(this,crc);
	if(cmd==CMD12)
		sd_card_ReadWriteByte(this,0xFF);	
	do
	{
		recv_data=sd_card_ReadWriteByte(this,0xFF);
	}while((recv_data & 0X80) && Retry--);	 
	//返回状态值
	return recv_data;
}
/*spi读写sd_card一个字节*/
static int sd_card_ReadWriteByte(const c_sd_card* this,u8 send_data)
{
	m_sd_card* m_this = NULL;
	u8 recv_data=0;
	/*参数检查*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	my_spi.set_datasize(m_this->m_spi_channal, 8);	
	my_spi.transmission(m_this->m_spi_channal,&send_data,&recv_data,1,false,1000);
	return recv_data;
}

//取消选择,释放SPI总线
static int SD_DisSelect(const c_sd_card* this)
{
	m_sd_card* m_this = NULL;
	u8 recv_data=0;
	u8 send_data=0;
	int ret=0;
	/*参数检查*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;	
	/*取消片选*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}		
	send_data=0xFF;
	my_spi.set_datasize(m_this->m_spi_channal, 8);	
	my_spi.transmission(m_this->m_spi_channal,&send_data,&recv_data,1,false,1000);
	return recv_data;
}
//选择sd卡,并且等待卡准备OK
//返回值:0,成功;1,失败;
static int SD_Select(const c_sd_card* this)
{
	m_sd_card* m_this = NULL;
	int ret=0;
	/*参数检查*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;	
	/*取消片选*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_RESET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	
	if(this->WaitReady(this)==E_OK)return 0;//等待成功
	SD_DisSelect(this);
	return 1;//等待失败
}

//等待卡准备好
//返回值:0,准备好了;其他,错误代码
static int  m_WaitReady(const c_sd_card* this)
{
	u32 t=0;
	u8 recv_data;
	/*参数检查*/
	if(NULL == this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}		
	do
	{
		recv_data=sd_card_ReadWriteByte(this,0xff);
		if(recv_data==0XFF)
			return E_OK;//OK
			t++;		  	
	}while(t<0XFF);//等待 
	return E_ERROR;
}

//获取SD卡的CSD信息，包括容量和速度信息
//输入:u8 *cid_data(存放CID的内存，至少16Byte）	    
//返回值:0：NO_ERR
//		 1：错误														   
static int m_GetCSD(const c_sd_card* this,u8 *csd_data)
{
  int ret = 0;
  u8 recv_data;		
	/*参数检查*/
	if(NULL == this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}	 
  recv_data=sd_card_SendCmd(this,CMD9,0,0x01);//发CMD9命令，读CSD
  if(recv_data==0)
	{
    	ret=this->ReadDisk(this,csd_data,16);//接收16个字节的数据 
  }
	/*取消片选*/
	SD_DisSelect(this);
	return E_OK;
} 

//获取SD卡的总扇区数（扇区数）   
//返回值:0： 取容量出错 
//其他:SD卡的容量(扇区数/512字节)
//每扇区的字节数必为512，因为如果不是512，则初始化不能通过.														  
static int m_GetSectorCount(const c_sd_card* this)
{
    u8 csd[16];
    int Capacity;  
    u8 n;
	  u16 csize;	
		/*参数检查*/
		if(NULL == this || NULL == this->this)
		{
				log_error("Null pointer.");
				return E_NULL;
		}
	//取CSD信息，如果期间出错，返回0
    if(this->GetCSD(this,csd)!=E_OK)
			return E_ERROR;	    
    //如果为SDHC卡，按照下面方式计算
    if((csd[0]&0xC0)==0x40)	 //V2.00的卡
    {	
		csize = csd[9] + ((u16)csd[8] << 8) + 1;
		Capacity = (u32)csize << 10;//得到扇区数		
    }else//V1.XX的卡
    {	
			n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;			
			csize = (csd[8] >> 6) + ((u16)csd[7] << 2) + ((u16)(csd[6] & 3) << 10) + 1;		
			Capacity= (u32)csize << (n - 9);//得到扇区数   
    }
    return Capacity;
}


//读SD卡
//buf:数据缓存区
//sector:扇区
//cnt:扇区数
//返回值:0,ok;其他,失败.
static int m_ReadData(const c_sd_card* this,u8*buf,u32 sector,u8 cnt)
{
	m_sd_card* m_this = NULL;
  int ret = 0;
  u8 recv_data;		
	/*参数检查*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;		 	
	if(SD_Type!=SD_TYPE_V2HC)
		sector <<= 9;//转换为字节地址
	if(cnt==1)
	{
		recv_data=sd_card_SendCmd(this,CMD17,sector,0X01);//读命令
		if(recv_data==0)//指令发送成功
		{
			ret=this->ReadDisk(this,buf,512);//接收512个字节	   
		}
	}else
	{
		recv_data=sd_card_SendCmd(this,CMD18,sector,0X01);//连续读命令
		do
		{
			ret=this->ReadDisk(this,buf,512);//接收512个字节	 
			buf+=512;  
		}while(--cnt && ret==E_OK); 	
		sd_card_SendCmd(this,CMD12,0,0X01);	//发送停止命令
	}   
	/*取消片选*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	
	return E_OK;
}
//写SD卡
//buf:数据缓存区
//sector:起始扇区
//cnt:扇区数
//返回值:0,ok;其他,失败.
static int m_WriteData(const c_sd_card* this,u8*buf,u32 sector,u8 cnt)
{
	m_sd_card* m_this = NULL;
  int ret = 0;
  u8 recv_data;		
	/*参数检查*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;		
	if(SD_Type!=SD_TYPE_V2HC)sector *= 512;//转换为字节地址
	if(cnt==1)
	{
		recv_data=sd_card_SendCmd(this,CMD24,sector,0X01);//读命令
		if(recv_data==0)//指令发送成功
		{
			ret=this->WriteDisk(this,buf,0xFE);//写512个字节	
			if(ret!=E_OK)
			{
				log_error("data write faild!");
			}
		}
	}
	else
	{
		if(SD_Type!=SD_TYPE_MMC)
		{
			sd_card_SendCmd(this,CMD55,0,0X01);	
			sd_card_SendCmd(this,CMD23,cnt,0X01);//发送指令	
		}
 		recv_data=sd_card_SendCmd(this,CMD25,sector,0X01);//连续读命令
		if(recv_data==E_OK)
		{
			do
			{
				ret=this->WriteDisk(this,buf,0xFC);//接收512个字节	 
				buf+=512;  
			}while(--cnt && ret==E_OK);
			this->WriteDisk(this,0,0xFD);//接收512个字节 
		}
	}   
	/*取消片选*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	
	return E_OK;
}	

//从sd卡读取一个数据包的内容
//buf:数据缓存区
//len:要读取的数据长度.
//返回值:0,成功;其他,失败;	
static int m_ReadDisk(const c_sd_card* this,u8*buf,u16 len)
{
	/*参数检查*/
	if(NULL == this )
	{
			log_error("Null pointer.");
			return E_NULL;
	}	
	if(this->GetResponse(this,0xFE))
		return 1;//等待SD卡发回数据起始令牌0xFE
	while(len--)//开始接收数据
	{
			*buf=(u8)sd_card_ReadWriteByte(this,0xFF);
			buf++;
	}
	//下面是2个伪CRC（dummy CRC）
	sd_card_ReadWriteByte(this,0xFF);
	sd_card_ReadWriteByte(this,0xFF);									  					    
	return 0;//读取成功
}

//向sd卡写入一个数据包的内容 512字节
//buf:数据缓存区
//cmd:指令
//返回值:0,成功;其他,失败;	
static int m_WriteDisk(const c_sd_card* this,u8*buf,u8 cmd)
{	
	u16 t;
  int ret = 0;
  u8 recv_data;		
	/*参数检查*/
	if(NULL == this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	ret=this->WaitReady(this);
	if(ret!=E_OK)
		return E_ERROR;//等待准备失效
	sd_card_ReadWriteByte(this,cmd);
	if(cmd!=0XFD)//不是结束指令
	{
		for(t=0;t<512;t++)
		{
			sd_card_ReadWriteByte(this,buf[t]);//提高速度,减少函数传参时间
		}
	    sd_card_ReadWriteByte(this,0xFF);//忽略crc
	    sd_card_ReadWriteByte(this,0xFF);
			recv_data=sd_card_ReadWriteByte(this,0xFF);//接收响应
		if((recv_data&0x1F)!=0x05)
			return E_ERROR;//响应错误									  					    
	}						 									  					    
    return E_OK;//写入成功
}

//等待SD卡回应
//Response:要得到的回应值
//返回值:0,成功得到了该回应值
//    其他,得到回应值失败
static int m_GetResponse(const c_sd_card* this,u8 Response)
{
	u16 Count=0xFFFF;//等待次数
  u8 recv_data;		
	/*参数检查*/
	if(NULL == this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}		
	while ((recv_data!=Response)&&Count)
	{
		recv_data=sd_card_ReadWriteByte(this,0XFF);
		Count--;//等待得到准确的回应  			
	}  
	if (Count==0)
		return E_ERROR;//得到回应失败   
	else 
		return E_OK;//正确回应
}

//获取SD卡的CID信息，包括制造商信息
//输入: u8 *cid_data(存放CID的内存，至少16Byte）	  
//返回值:0：NO_ERR
//		 1：错误														   
static int  m_GetCID(const c_sd_card* this,u8 *cid_data)
{
	m_sd_card* m_this = NULL;
	int ret = 0;
	u8 recv_data;		
	/*参数检查*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}	
	//发CMD10命令，读CID
  recv_data=sd_card_SendCmd(this,CMD10,0,0x01);
  if(recv_data==0x00)
	{
		ret=this->WriteDisk(this,cid_data,16);//接收16个字节的数据	 
  }
	/*取消片选*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	
	if(ret!=E_OK)
		return E_ERROR;
	else return E_OK;
}		



#endif

