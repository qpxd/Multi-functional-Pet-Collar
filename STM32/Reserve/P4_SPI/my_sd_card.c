#include "driver_conf.h"

#ifdef DRIVER_SD_CARD_ENABLED

#define MODULE_NAME       "sd_card"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME		
u8  SD_Type=0;//SD�������� 
typedef struct	__M_SD_CARD
{
	c_switch			m_cs;
  u8            m_spi_channal ;  /*spiͨ��*/	
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
	/*Ϊ�¶��������ڴ�*/
	new.this = pvPortMalloc(sizeof(m_sd_card));
	if(NULL == new.this)
	{
			log_error("Out of memory");
			return new;
	}
	memset(new.this,0,sizeof(m_sd_card));
	m_this = new.this;

	/*��ʼ����Ӧ��SPI*/
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
	/*�����������*/
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
	u8 recv_data = 0;/*spi����ֵ*/
	u8 send_data=0;
	u8 i=0;
	u16 retry=20;  /*�������г�ʱ����*/
	u8 buf[4]; 
	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;
	/*ȡ��Ƭѡ*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	
//	/*����spi�ٶ�*/
//	my_spi.set_speed(m_this->m_spi_channal,SPI_BAUDRATEPRESCALER_256);
	

	/*��������74������*/
	send_data=0XFF;
	for(i=0;i<10;i++)
	{
		my_spi.set_datasize(m_this->m_spi_channal, 8);	
		my_spi.transmission(m_this->m_spi_channal,&send_data,&recv_data,1,false,1000);		
	}
	do
	{
		send_data=0x95;
		recv_data=sd_card_SendCmd(this,CMD0,0,send_data);//����IDLE״̬
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
			if(buf[2]==0X01&&buf[3]==0XAA)//���Ƿ�֧��2.7~3.6V
			{
				retry=0XFFFE;
				do
				{
					sd_card_SendCmd(this,CMD55,0,0X01);	//����CMD55
					recv_data=sd_card_SendCmd(this,CMD41,0x40000000,0X01);//����CMD41
				}while(recv_data&&retry--);
				recv_data=sd_card_SendCmd(this,CMD58,0,0X01);
				if(retry&&recv_data==0)//����SD2.0���汾��ʼ
				{
					for(i=0;i<4;i++)
					{
						buf[i]=sd_card_ReadWriteByte(this,0XFF);//�õ�OCRֵ
					}
					if(buf[0]&0x40)	
					{
						SD_Type=SD_TYPE_V2HC;    //���CCS
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
			sd_card_SendCmd(this,CMD55,0,0X01);		//����CMD55
			recv_data=sd_card_SendCmd(this,CMD41,0,0X01);	//����CMD41
			if(recv_data<=1)
			{		
				SD_Type=SD_TYPE_V1;
				retry=0XFFFE;
				do //�ȴ��˳�IDLEģʽ
				{
					sd_card_SendCmd(this,CMD55,0,0X01);	//����CMD55
					recv_data=sd_card_SendCmd(this,CMD41,0,0X01);//����CMD41
				}while(recv_data&&retry--);
				log_inform("SD card is V1.0");
			}
			else//MMC����֧��CMD55+CMD41ʶ��
			{
				SD_Type=SD_TYPE_MMC;//MMC V3
				retry=0XFFFE;
				do //�ȴ��˳�IDLEģʽ
				{											    
					recv_data=sd_card_SendCmd(this,CMD1,0,0X01);//����CMD1
				}while(recv_data&&retry--);
				log_inform("SD card is MMC");				
			}
			recv_data=sd_card_SendCmd(this,CMD16,512,0X01);
			if(retry==0||recv_data!=0)
			{
				SD_Type=SD_TYPE_ERR;//����Ŀ�		
				log_error("sd_card	error!");
			}
		}
		delay_ms(1000);	
	}
	/*ȡ��Ƭѡ*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	

//	/*�ָ�spi�ٶ�*/
//	my_spi.set_speed(m_this->m_spi_channal,SPI_BAUDRATEPRESCALER_4);
	if(SD_Type)
		return E_OK;
	else
		return E_ERROR; 
}
/*����: u8 cmd   ���� */
/*      u32 arg  �������*/
/*      u8 crc   crcУ��ֵ*/   
/*����ֵ:SD�����ص���Ӧ*/		
static int sd_card_SendCmd(const c_sd_card* this,u8 cmd, u32 arg, u8 crc)
{
	u8 recv_data=0;
	u8 Retry=0X1F;
	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	SD_DisSelect(this);
	if(SD_Select(this))
	return E_ERROR;
	sd_card_ReadWriteByte(this,cmd | 0x40);//�ֱ�д������
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
	//����״ֵ̬
	return recv_data;
}
/*spi��дsd_cardһ���ֽ�*/
static int sd_card_ReadWriteByte(const c_sd_card* this,u8 send_data)
{
	m_sd_card* m_this = NULL;
	u8 recv_data=0;
	/*�������*/
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

//ȡ��ѡ��,�ͷ�SPI����
static int SD_DisSelect(const c_sd_card* this)
{
	m_sd_card* m_this = NULL;
	u8 recv_data=0;
	u8 send_data=0;
	int ret=0;
	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;	
	/*ȡ��Ƭѡ*/
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
//ѡ��sd��,���ҵȴ���׼��OK
//����ֵ:0,�ɹ�;1,ʧ��;
static int SD_Select(const c_sd_card* this)
{
	m_sd_card* m_this = NULL;
	int ret=0;
	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;	
	/*ȡ��Ƭѡ*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_RESET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	
	if(this->WaitReady(this)==E_OK)return 0;//�ȴ��ɹ�
	SD_DisSelect(this);
	return 1;//�ȴ�ʧ��
}

//�ȴ���׼����
//����ֵ:0,׼������;����,�������
static int  m_WaitReady(const c_sd_card* this)
{
	u32 t=0;
	u8 recv_data;
	/*�������*/
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
	}while(t<0XFF);//�ȴ� 
	return E_ERROR;
}

//��ȡSD����CSD��Ϣ�������������ٶ���Ϣ
//����:u8 *cid_data(���CID���ڴ棬����16Byte��	    
//����ֵ:0��NO_ERR
//		 1������														   
static int m_GetCSD(const c_sd_card* this,u8 *csd_data)
{
  int ret = 0;
  u8 recv_data;		
	/*�������*/
	if(NULL == this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}	 
  recv_data=sd_card_SendCmd(this,CMD9,0,0x01);//��CMD9�����CSD
  if(recv_data==0)
	{
    	ret=this->ReadDisk(this,csd_data,16);//����16���ֽڵ����� 
  }
	/*ȡ��Ƭѡ*/
	SD_DisSelect(this);
	return E_OK;
} 

//��ȡSD����������������������   
//����ֵ:0�� ȡ�������� 
//����:SD��������(������/512�ֽ�)
//ÿ�������ֽ�����Ϊ512����Ϊ�������512�����ʼ������ͨ��.														  
static int m_GetSectorCount(const c_sd_card* this)
{
    u8 csd[16];
    int Capacity;  
    u8 n;
	  u16 csize;	
		/*�������*/
		if(NULL == this || NULL == this->this)
		{
				log_error("Null pointer.");
				return E_NULL;
		}
	//ȡCSD��Ϣ������ڼ��������0
    if(this->GetCSD(this,csd)!=E_OK)
			return E_ERROR;	    
    //���ΪSDHC�����������淽ʽ����
    if((csd[0]&0xC0)==0x40)	 //V2.00�Ŀ�
    {	
		csize = csd[9] + ((u16)csd[8] << 8) + 1;
		Capacity = (u32)csize << 10;//�õ�������		
    }else//V1.XX�Ŀ�
    {	
			n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;			
			csize = (csd[8] >> 6) + ((u16)csd[7] << 2) + ((u16)(csd[6] & 3) << 10) + 1;		
			Capacity= (u32)csize << (n - 9);//�õ�������   
    }
    return Capacity;
}


//��SD��
//buf:���ݻ�����
//sector:����
//cnt:������
//����ֵ:0,ok;����,ʧ��.
static int m_ReadData(const c_sd_card* this,u8*buf,u32 sector,u8 cnt)
{
	m_sd_card* m_this = NULL;
  int ret = 0;
  u8 recv_data;		
	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;		 	
	if(SD_Type!=SD_TYPE_V2HC)
		sector <<= 9;//ת��Ϊ�ֽڵ�ַ
	if(cnt==1)
	{
		recv_data=sd_card_SendCmd(this,CMD17,sector,0X01);//������
		if(recv_data==0)//ָ��ͳɹ�
		{
			ret=this->ReadDisk(this,buf,512);//����512���ֽ�	   
		}
	}else
	{
		recv_data=sd_card_SendCmd(this,CMD18,sector,0X01);//����������
		do
		{
			ret=this->ReadDisk(this,buf,512);//����512���ֽ�	 
			buf+=512;  
		}while(--cnt && ret==E_OK); 	
		sd_card_SendCmd(this,CMD12,0,0X01);	//����ֹͣ����
	}   
	/*ȡ��Ƭѡ*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	
	return E_OK;
}
//дSD��
//buf:���ݻ�����
//sector:��ʼ����
//cnt:������
//����ֵ:0,ok;����,ʧ��.
static int m_WriteData(const c_sd_card* this,u8*buf,u32 sector,u8 cnt)
{
	m_sd_card* m_this = NULL;
  int ret = 0;
  u8 recv_data;		
	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	m_this = this->this;		
	if(SD_Type!=SD_TYPE_V2HC)sector *= 512;//ת��Ϊ�ֽڵ�ַ
	if(cnt==1)
	{
		recv_data=sd_card_SendCmd(this,CMD24,sector,0X01);//������
		if(recv_data==0)//ָ��ͳɹ�
		{
			ret=this->WriteDisk(this,buf,0xFE);//д512���ֽ�	
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
			sd_card_SendCmd(this,CMD23,cnt,0X01);//����ָ��	
		}
 		recv_data=sd_card_SendCmd(this,CMD25,sector,0X01);//����������
		if(recv_data==E_OK)
		{
			do
			{
				ret=this->WriteDisk(this,buf,0xFC);//����512���ֽ�	 
				buf+=512;  
			}while(--cnt && ret==E_OK);
			this->WriteDisk(this,0,0xFD);//����512���ֽ� 
		}
	}   
	/*ȡ��Ƭѡ*/
	ret = m_this->m_cs.set(&m_this->m_cs,GPIO_PIN_SET);
	if(E_OK != ret)
	{
			log_error("Switch set failed.");
			return E_ERROR;
	}	
	return E_OK;
}	

//��sd����ȡһ�����ݰ�������
//buf:���ݻ�����
//len:Ҫ��ȡ�����ݳ���.
//����ֵ:0,�ɹ�;����,ʧ��;	
static int m_ReadDisk(const c_sd_card* this,u8*buf,u16 len)
{
	/*�������*/
	if(NULL == this )
	{
			log_error("Null pointer.");
			return E_NULL;
	}	
	if(this->GetResponse(this,0xFE))
		return 1;//�ȴ�SD������������ʼ����0xFE
	while(len--)//��ʼ��������
	{
			*buf=(u8)sd_card_ReadWriteByte(this,0xFF);
			buf++;
	}
	//������2��αCRC��dummy CRC��
	sd_card_ReadWriteByte(this,0xFF);
	sd_card_ReadWriteByte(this,0xFF);									  					    
	return 0;//��ȡ�ɹ�
}

//��sd��д��һ�����ݰ������� 512�ֽ�
//buf:���ݻ�����
//cmd:ָ��
//����ֵ:0,�ɹ�;����,ʧ��;	
static int m_WriteDisk(const c_sd_card* this,u8*buf,u8 cmd)
{	
	u16 t;
  int ret = 0;
  u8 recv_data;		
	/*�������*/
	if(NULL == this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}
	ret=this->WaitReady(this);
	if(ret!=E_OK)
		return E_ERROR;//�ȴ�׼��ʧЧ
	sd_card_ReadWriteByte(this,cmd);
	if(cmd!=0XFD)//���ǽ���ָ��
	{
		for(t=0;t<512;t++)
		{
			sd_card_ReadWriteByte(this,buf[t]);//����ٶ�,���ٺ�������ʱ��
		}
	    sd_card_ReadWriteByte(this,0xFF);//����crc
	    sd_card_ReadWriteByte(this,0xFF);
			recv_data=sd_card_ReadWriteByte(this,0xFF);//������Ӧ
		if((recv_data&0x1F)!=0x05)
			return E_ERROR;//��Ӧ����									  					    
	}						 									  					    
    return E_OK;//д��ɹ�
}

//�ȴ�SD����Ӧ
//Response:Ҫ�õ��Ļ�Ӧֵ
//����ֵ:0,�ɹ��õ��˸û�Ӧֵ
//    ����,�õ���Ӧֵʧ��
static int m_GetResponse(const c_sd_card* this,u8 Response)
{
	u16 Count=0xFFFF;//�ȴ�����
  u8 recv_data;		
	/*�������*/
	if(NULL == this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}		
	while ((recv_data!=Response)&&Count)
	{
		recv_data=sd_card_ReadWriteByte(this,0XFF);
		Count--;//�ȴ��õ�׼ȷ�Ļ�Ӧ  			
	}  
	if (Count==0)
		return E_ERROR;//�õ���Ӧʧ��   
	else 
		return E_OK;//��ȷ��Ӧ
}

//��ȡSD����CID��Ϣ��������������Ϣ
//����: u8 *cid_data(���CID���ڴ棬����16Byte��	  
//����ֵ:0��NO_ERR
//		 1������														   
static int  m_GetCID(const c_sd_card* this,u8 *cid_data)
{
	m_sd_card* m_this = NULL;
	int ret = 0;
	u8 recv_data;		
	/*�������*/
	if(NULL == this || NULL == this->this)
	{
			log_error("Null pointer.");
			return E_NULL;
	}	
	//��CMD10�����CID
  recv_data=sd_card_SendCmd(this,CMD10,0,0x01);
  if(recv_data==0x00)
	{
		ret=this->WriteDisk(this,cid_data,16);//����16���ֽڵ�����	 
  }
	/*ȡ��Ƭѡ*/
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

