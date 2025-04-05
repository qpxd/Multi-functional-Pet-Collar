/**
******************************************************************************
* File Name          : flash.c
* Description        : This file provides code for the configuration
*                      of all used GPIO pins.
******************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "onchip_conf.h"

/* Macros -----------------------------------------------*/
#define R_OK  0
#define R_ERR 1

//#ifdef HELP_LOG_ENABLED
//	#define LOG_OUT				log_debug
//#else
	#define LOG_OUT
//#endif

#ifdef SYSTEM_FREERTOS_ENABLED
	#define N_MALLOC			pvPortMalloc
	#define N_FREE				vPortFree
#else
	#define N_MALLOC			malloc
	#define N_FREE				free
#endif

/* Variables-----------------------------------------------*/


/* Public function prototypes------------------------------*/
int8_t vFlashEraseNPages(uint32_t xStartAddr, uint32_t xPageCnt)
{
    /*检查地址范围有效性*/
    if(IS_FLASH_PROGRAM_ADDRESS(xStartAddr) == 0)
    {
        LOG_OUT("Illegal address is over range: 0x%x", xStartAddr); 
        return R_ERR;   
    }   
    /*检查地址是否按照 AT_AT_FLASH_PAGE_SIZE 对齐*/
    if((xStartAddr%AT_FLASH_PAGE_SIZE) != 0)
    {
        LOG_OUT("Address is not aligned: 0x%x", xStartAddr);
        return R_ERR;   
    }
    /*擦除数据*/
    uint32_t SECTORError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;
    HAL_FLASH_Unlock();   
    EraseInitStruct.TypeErase    = FLASH_TYPEERASE_PAGES;      	/*擦除方式：大量擦除/按页擦除*/
    EraseInitStruct.Banks        = FLASH_BANK_1;                /*选择BANK,L431只有BANK1*/
    EraseInitStruct.NbPages      = 1;                      		/*擦除的页数*/
	/*循环按页擦除*/   
	for(int i = 0; i < xPageCnt; i++)
	{
		EraseInitStruct.PageAddress  = xStartAddr + i * AT_FLASH_PAGE_SIZE;   
		if(HAL_FLASHEx_Erase(&EraseInitStruct, &SECTORError) != HAL_OK)
		{
			HAL_FLASH_Lock();
			LOG_OUT("Flash erase failed !!!");
			return R_ERR;   
		}
	}

    HAL_FLASH_Lock();
    return R_OK;
}


int8_t vFlashWriteNBytes(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize)     
{
    uint32_t xWrAddr = xStartAddr;
    uint16_t *pWrDataBuf = (uint16_t*)pBuffer;
    /*检查地址范围有效性*/
    if(IS_FLASH_PROGRAM_ADDRESS(xStartAddr) == 0)
    {
        LOG_OUT("Illegal address is over range: 0x%x", xStartAddr); 
        return R_ERR;    
    }
    /*写入大小必须4字节对齐*/
    if((xSize%2) != 0)
    {
        LOG_OUT("Size is not aligned in 2 bytes: %d", xSize);
        return R_ERR;   
    }
    /*写入数据*/
    HAL_FLASH_Unlock(); 
    while((xWrAddr - xStartAddr) < xSize)
    {
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, xWrAddr, *(pWrDataBuf)) != HAL_OK)
        {
            HAL_FLASH_Lock();
            LOG_OUT("Flash Write failed !!!");
            return R_ERR;
        }
        xWrAddr += 2;
        pWrDataBuf++;
    }
    HAL_FLASH_Lock();
    return R_OK;
}

int8_t vFlashReadNBytes(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize)       
{
    /*读取数据*/
    for(uint32_t xRdAddr = xStartAddr; xRdAddr < (xStartAddr+xSize); xRdAddr++)
    {
        *pBuffer = *((uint8_t*)xRdAddr); 
        pBuffer++;
    }
    return R_OK;
}

#ifdef ONCHIP_FLASH_NVS_ENABLED
/* Typedef -----------------------------------------------*/
typedef struct
{
	char pcName[12];                //NVS 命名空间名称
	uint32_t uxStart;               //NVS 命名空间起始地址
	uint16_t ucCurItem;             //当前项目索引
	uint32_t sHash[NVS_ITEMS_CNT];  //当前命名空间对应的Hash表
}m_nvs_t;

/* Private function declare-------------------------------------------------------------*/
static char prvCharToLower(char c);
static uint32_t prvHashCode(const char* str);
static uint32_t prvGetNameSpaceAddr(char *pcName);
uint8_t prvGetHashList(const c_nvs_t *this);
uint8_t prvRebuildNameSpace(const c_nvs_t *this);

uint8_t xNvsFindItem(const c_nvs_t *this, char *pcKey, uint16_t *pcIndex);
uint8_t xNvsGetItemValue(const c_nvs_t *this, char *pcKey, void *pcValue);
uint8_t xNvsAddItemValue(const c_nvs_t *this, char *pcKey, void *pcValue, uint16_t ucLen);
/* Variables-----------------------------------------------*/

/* Extern function prototypes-------------------------------------------------------------*/
c_nvs_t nvs_open(char *pcName)
{
	m_nvs_t *m_this;
	c_nvs_t new;
	/*1. 成员申请内存*/
	new.this = N_MALLOC(sizeof(m_nvs_t));
	if(NULL == new.this)
	{
		LOG_OUT("Memmory request failed.");
		return new;
	}
	m_this = new.this;
	memset((uint8_t*)m_this, 0, sizeof(m_nvs_t));
	
	/*2. 私有成员初始化*/
    uint32_t xAddr = prvGetNameSpaceAddr(pcName);	//获取此命名空间对应的地址, 如果命名空间不存在则会直接创建
    if(xAddr != 0)  m_this->uxStart = xAddr;
    else            goto errorHandle;
	strcpy(m_this->pcName, pcName);							//储存命令空间
	m_this->ucCurItem		= 0;							//复位命令空间列表项索引

	/*3. 公有有成员初始化*/
	new.find 				= xNvsFindItem;
	new.get					= xNvsGetItemValue;
	new.add 				= xNvsAddItemValue;

	/*4. 将flash中的hash表读出重新初始化*/
	prvGetHashList(&new);
	return new;

errorHandle:
    N_FREE(new.this);
    new.this = NULL;
    return new;
}


uint8_t nvs_close(c_nvs_t *this)
{
	if(NULL == this)
	{
		LOG_OUT("Null pointor .");
		return R_ERR;
	}
	m_nvs_t *m_this = this->this;
	if(NULL == m_this)
	{
		LOG_OUT("Please open nvs first .");
		return R_ERR;
	}
	N_FREE(this->this);
	this->this = NULL;
	return R_OK;
}

//删除前nvs必须处于关闭状态
uint8_t nvs_delete(char *pcName)
{
	page_item_t xItem;
	uint16_t xOffset = 0;
	for(xOffset = 0; xOffset < AT_FLASH_PAGE_SIZE; xOffset += sizeof(xItem))
    {
		vFlashReadNBytes(NVS_START + xOffset, (uint8_t*)(&xItem), sizeof(xItem));
        if(xItem.arFlag[0] == ITEM_INVALID && xItem.arFlag[1] == ITEM_INVALID){
            continue;
        }
        if(strcmp(pcName, xItem.arData) == 0 && xItem.ucType == ITEM_HEAD){
			xItem.arFlag[0] = ITEM_INVALID;
			xItem.arFlag[1] = ITEM_INVALID;
			vFlashWriteNBytes(NVS_START + xOffset + sizeof(xItem) - 2, xItem.arFlag, 2);
			LOG_OUT("Namespace delete success. ");
			return R_OK;
		}	
    }
	LOG_OUT("Namespace delete failed. ");
	return R_ERR;
}

/* Public function prototypes-------------------------------------------------------------*/
uint8_t xNvsFindItem(const c_nvs_t *this, char *pcKey, uint16_t *pcIndex)
{
	if(NULL == this || pcKey == NULL)
	{
		LOG_OUT("Null pointor");
		return R_ERR;
	}
	m_nvs_t *m_this = this->this;
	if(NULL == m_this)
	{
		LOG_OUT("Please open nvs first .");
		return R_ERR;
	}
	
	uint32_t uxHashCode = 0;
	uxHashCode = prvHashCode(pcKey);
	for(uint16_t j = 0; j < NVS_ITEMS_CNT; j++)
	{
		if(uxHashCode == m_this->sHash[j]){
			*pcIndex = j;
			return R_OK;
		}	
	}
	return R_ERR;
}



uint8_t xNvsGetItemValue(const c_nvs_t *this, char *pcKey, void *pcValue)
{
	if(NULL == this || pcKey == NULL)
	{
		LOG_OUT("Null pointor.");
		return R_ERR;
	}
	m_nvs_t *m_this = this->this;
	if(NULL == m_this)
	{
		LOG_OUT("Please open nvs first .");
		return R_ERR;
	}
	
	uint32_t uxHashCode = prvHashCode(pcKey);
	page_item_t xItem;
	
	for(uint16_t j = 0; j < NVS_ITEMS_CNT; j++)
	{
		//找到了hash值对应的节点
		if(uxHashCode == m_this->sHash[j])
		{
			vFlashReadNBytes(m_this->uxStart+j*NVS_ITEMS_SIZE, (uint8_t*)(&xItem), NVS_ITEMS_SIZE);	//检查节点值类型是否正确	
			if(xItem.ucType != ITEM_KEY){
				LOG_OUT("Nvs item type error.");
				return R_ERR;
			}else{
				uint16_t offset = 0;
				for(uint16_t m = j+1; m < NVS_ITEMS_CNT; m++)
				{
					vFlashReadNBytes(m_this->uxStart+m*NVS_ITEMS_SIZE, (uint8_t*)(&xItem), NVS_ITEMS_SIZE);
					if(xItem.ucType == ITEM_VALUE){
						memcpy((char*)pcValue + offset, xItem.arData, xItem.ucLen);
						offset += xItem.ucLen;
					}else{
						LOG_OUT("<%s>: \"%s\" get ok.", m_this->pcName, pcKey);
						return R_OK;
					}							
				}
			}
		}
	}
	return R_ERR;
}



uint8_t xNvsAddItemValue(const c_nvs_t *this, char *pcKey, void *pcValue, uint16_t ucLen)
{
	if(NULL == this || pcKey == NULL || pcValue == NULL)
	{
		LOG_OUT("Null pointor .");
		return R_ERR;
	}
	
	m_nvs_t *m_this = this->this;
	if(NULL == m_this)
	{
		LOG_OUT("Please open nvs first .");
		return R_ERR;
	}
	
	uint16_t uxIndex;
	page_item_t xItem;
	
	/*hash 表中key已经存在, 原key置无效, ram中hash表和flash中同时置位*/
	if(xNvsFindItem(this, pcKey, &uxIndex) == R_OK)
	{
		m_this->sHash[uxIndex] = (ITEM_INVALID<<24) | (ITEM_INVALID<<16) | (ITEM_INVALID<<8) | ITEM_INVALID;
		xItem.arFlag[0] = ITEM_INVALID;
		xItem.arFlag[1] = ITEM_INVALID;
		vFlashWriteNBytes(m_this->uxStart+(uxIndex+1)*NVS_ITEMS_SIZE-2, xItem.arFlag, 2);
		/*置key对应的后面的value值为无效值*/
		while(uxIndex < NVS_ITEMS_CNT)
		{
			uxIndex++;
			vFlashReadNBytes(m_this->uxStart+uxIndex*NVS_ITEMS_SIZE, (uint8_t*)(&xItem), sizeof(xItem));
			if(xItem.ucType != ITEM_VALUE)	break;
			xItem.arFlag[0] = ITEM_INVALID;
			xItem.arFlag[1] = ITEM_INVALID;
			vFlashWriteNBytes(m_this->uxStart+(uxIndex+1)*NVS_ITEMS_SIZE-2, xItem.arFlag, 2);	
		}
	}
	
	/*新key追加到末尾*/
	uint8_t ucItemCnt =  ucLen / sizeof(xItem.arData);
	if((ucLen%sizeof(xItem.arData)) != 0)	ucItemCnt++;
	
	/*最后一页已存满*/
	if(m_this->ucCurItem + ucItemCnt >= NVS_ITEMS_CNT)
	{
		prvRebuildNameSpace(this);	/*重新调整布局 删除无效的item*/
	}
	
	/*key 值写入flash*/
	m_this->sHash[m_this->ucCurItem] = prvHashCode(pcKey);	//计算hash值
	if(strlen(pcKey) > sizeof(xItem.arData)){
		LOG_OUT("Key is out of lenth.");
		return R_ERR;
	}	

	xItem.ucType = ITEM_KEY;
	memset(xItem.arData, 0, sizeof(xItem.arData));
	strcpy(xItem.arData, pcKey);
	vFlashWriteNBytes(m_this->uxStart+m_this->ucCurItem*NVS_ITEMS_SIZE, (uint8_t*)(&xItem), sizeof(xItem.arData)+2);
	m_this->ucCurItem++;
	/*value 值写入flash*/
	xItem.ucType = ITEM_VALUE;
	for(uint16_t i = 0; i < ucLen; i += sizeof(xItem.arData))
	{
		memset(xItem.arData, 0, sizeof(xItem.arData));
		if(ucLen - i > sizeof(xItem.arData)){
			xItem.ucLen	 = sizeof(xItem.arData);
			memcpy(xItem.arData, (char*)pcValue + i, sizeof(xItem.arData));
		} else{
			xItem.ucLen	 = ucLen - i;
			memcpy(xItem.arData, (char*)pcValue + i, ucLen - i);
		}								 
		m_this->sHash[m_this->ucCurItem] = (ITEM_VALUE<<24)|(ITEM_VALUE<<16)|(ITEM_VALUE<<8)|ITEM_VALUE;
		vFlashWriteNBytes(m_this->uxStart+m_this->ucCurItem*NVS_ITEMS_SIZE, (uint8_t*)(&xItem), sizeof(xItem.arData) + 2);
		m_this->ucCurItem++;
	}
	LOG_OUT("<%s>: \"%s\" add ok.", m_this->pcName, pcKey);
	return R_OK;
}

/* Private function prototypes-------------------------------------------------------------*/
static uint32_t prvGetNameSpaceAddr(char *pcName)
{
	page_item_t xItem;
    uint16_t xOffset = 0;
	uint16_t uxItemCnt = 0; 
	uint8_t *pcTemp;
    uint32_t uxAddr;

restart:	
    //查找命令空间是否存在 如果存在直接返回地址
    for(xOffset = 0; xOffset < NVS_SPCAE_CNT*sizeof(xItem); xOffset += sizeof(xItem))
	{
		vFlashReadNBytes(NVS_START + xOffset, (uint8_t*)(&xItem), sizeof(xItem));
		if(xItem.ucType != ITEM_HEAD){
            continue;
		}
        if(xItem.arFlag[0] == ITEM_INVALID && xItem.arFlag[1] == ITEM_INVALID){
            continue;
        }  
        if(strcmp(pcName, xItem.arData) == 0){
			LOG_OUT("Namespace find ok.");
			return (NVS_DATA_START + xOffset/sizeof(xItem)*NVS_SPCAE_SIZE);
		}						
	}

    //如果命令空间不存在，创建并且返回地址，创建成功后会先擦除改命名空间
    for(xOffset = 0; xOffset < NVS_SPCAE_CNT*sizeof(xItem); xOffset += sizeof(xItem))
	{
		vFlashReadNBytes(NVS_START + xOffset, (uint8_t*)(&xItem), sizeof(xItem));
        if(xItem.ucType!=ITEM_UNINITED || xItem.arFlag[0]!=ITEM_UNINITED || xItem.arFlag[1]!=ITEM_UNINITED){
            continue;
        }  
        LOG_OUT("Namespace create ok.");
        xItem.ucLen = 0;
        xItem.ucType = ITEM_HEAD;
        strcpy(xItem.arData, pcName);
        if(vFlashWriteNBytes(NVS_START + xOffset, (uint8_t*)(&xItem), sizeof(xItem.arData) + 2) != R_OK){
            return 0;	
        }else{  //创建成功，擦除该命名空间
            uxAddr = (NVS_DATA_START + xOffset/sizeof(xItem)*NVS_SPCAE_SIZE);
            vFlashEraseNPages(uxAddr, NVS_SPCAE_SIZE/AT_FLASH_PAGE_SIZE);          
            return uxAddr;	
        }			 
	}

    //如果命令空间存满了重置命令空间管理表 
	LOG_OUT("Namespace full, will reset ...");
	pcTemp = (uint8_t*)N_MALLOC(NVS_SPCAE_CNT*sizeof(xItem));
	if(NULL == pcTemp){
		LOG_OUT("Memmory request failed.");
		return 0;
	}else{
        memset(pcTemp, 0, AT_FLASH_PAGE_SIZE);
    }

    vFlashReadNBytes(NVS_START, pcTemp, NVS_SPCAE_CNT*sizeof(xItem));   //读出命名空间管理表
    vFlashEraseNPages(NVS_START, 1);                                    //擦除命名空间管理表     
    //将有效的 命名空间索引头 写入flash ，无效的直接丢掉             
    for(xOffset = 0; xOffset < NVS_SPCAE_CNT*sizeof(xItem); xOffset += sizeof(xItem))    
    {
        memcpy((uint8_t*)(&xItem), pcTemp + xOffset, sizeof(xItem));
        if(xItem.ucType==ITEM_HEAD && xItem.arFlag[0]==ITEM_UNINITED && xItem.arFlag[1]==ITEM_UNINITED)
        {
            vFlashWriteNBytes(NVS_START + xOffset, (uint8_t*)(&xItem), sizeof(xItem));  //写入索引头
            uxItemCnt++;
        }
    }
	N_FREE(pcTemp);
    
    if(uxItemCnt >= NVS_SPCAE_CNT){
	    LOG_OUT("Namespace create failed, memmory is full.");
        return R_ERR;
    }else{
        LOG_OUT("Namespace reset ok . ");
	    goto restart;
    }
}

uint8_t prvGetHashList(const c_nvs_t *this)
{
    if(NULL == this)
	{
		LOG_OUT("Null pointor .");
		return R_ERR;
	}
	m_nvs_t *m_this = this->this;

   	/*重新初始化hash列表*/
	page_item_t	xItem;
	uint32_t 	uxAddr = m_this->uxStart;
	m_this->ucCurItem = 0;
	for(uint16_t j = 0; j < NVS_ITEMS_CNT; j++)
	{
		vFlashReadNBytes(uxAddr+j*sizeof(xItem), (uint8_t*)(&xItem), sizeof(xItem));
		if(xItem.arFlag[0] != ITEM_UNINITED || xItem.arFlag[1] != ITEM_UNINITED){
            m_this->ucCurItem++;
			continue;	//无效item，直接跳过
		}
		if(xItem.ucType == ITEM_KEY){
			m_this->sHash[m_this->ucCurItem] = prvHashCode(xItem.arData);   
		}else if(xItem.ucType == ITEM_VALUE){
			m_this->sHash[m_this->ucCurItem] = (ITEM_VALUE<<24) | (ITEM_VALUE<<16) | (ITEM_VALUE<<8) | ITEM_VALUE;
		}else{
			break;	//当前页所有 item 保存完成
		}
        m_this->ucCurItem++;
	} 
    return R_OK;
}

/*命名空间 存满后需要擦除命名空间 并重构 并且重建hash表*/
uint8_t prvRebuildNameSpace(const c_nvs_t *this)
{
	if(NULL == this)
	{
		LOG_OUT("Null pointor .");
		return R_ERR;
	}
	m_nvs_t *m_this = this->this;
	

	/*为flash申请内存*/
	uint8_t *pxTemp;
	pxTemp = N_MALLOC(NVS_SPCAE_SIZE);      //先申请一页的内存
	if(NULL == pxTemp)
	{
		LOG_OUT("Mempry request failed .");
		return R_ERR;
	}
	/*所有有效的xItem存入pxTemp中， ucCurItem统计有效的xItem个数， 同时计算 item的hash值*/
	page_item_t	xItem;
	uint32_t 	uxAddr = m_this->uxStart;
	m_this->ucCurItem = 0;
	for(uint16_t j = 0; j < NVS_ITEMS_CNT; j++)
	{
		vFlashReadNBytes(uxAddr+j*sizeof(xItem), (uint8_t*)(&xItem), sizeof(xItem));
		if(xItem.arFlag[0] != ITEM_UNINITED || xItem.arFlag[1] != ITEM_UNINITED){
			continue;	//无效item，直接跳过 (注意这里 ucCurItem 不会加，因为是统计有效 item 的个数)
		}
		
		if(xItem.ucType == ITEM_KEY){
			m_this->sHash[m_this->ucCurItem] = prvHashCode(xItem.arData);
			memcpy(pxTemp + m_this->ucCurItem*NVS_ITEMS_SIZE, (uint8_t*)(&xItem), NVS_ITEMS_SIZE);
			m_this->ucCurItem++;
		}else if(xItem.ucType == ITEM_VALUE){
			m_this->sHash[m_this->ucCurItem] = (ITEM_VALUE<<24) | (ITEM_VALUE<<16) | (ITEM_VALUE<<8) | ITEM_VALUE;
			memcpy(pxTemp+m_this->ucCurItem*NVS_ITEMS_SIZE, (uint8_t*)(&xItem), NVS_ITEMS_SIZE);
			m_this->ucCurItem++;
		}else{
			break;	//当前页所有 item 保存完成
		}
	}
	
	/*擦除后面空闲的所有页*/
	vFlashEraseNPages(m_this->uxStart, (NVS_SPCAE_SIZE/AT_FLASH_PAGE_SIZE));
	/*如果还有未写入的数据项，需要写入进去*/
	if(m_this->ucCurItem != 0)
	{
		vFlashWriteNBytes(m_this->uxStart, pxTemp, m_this->ucCurItem*NVS_ITEMS_SIZE);	
	}
	N_FREE(pxTemp);
	return R_OK;
}

static char prvCharToLower(char c)
{
    if ((c >= 'A') && (c <= 'Z'))
        return c + ('a' - 'A');
    return c;
}

static uint32_t prvHashCode(const char* str)
{
    int tmp, c = *str;
    unsigned int seed = CMD_HASH;  /* 'jiejie' string hash */  
    unsigned int hash = 0;
    
    while(*str) {
        tmp = prvCharToLower(c);
        hash = (hash ^ seed) + tmp;
        str++;
        c = *str;
    }
    return hash;
}

#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
/*
note:
在参考手册 RM0394 中第3节我们可以看到 STM32L4 系列的 Flsah 介绍
在3.3.1节中查看 L4系列的Flash主要有三种 128K ,256K, 512K,
2K为一页.页为Flash擦除和写入的最小单位
（STM32还有以扇区分的，而且扇区大小不一定相同，此时擦除写入的最小单位就是扇区）

可以根据芯片的名称快速知道 STM32 的 Flash 容量
STM32L4CCT6 第一个C表示引脚数为48脚，第二个C表示 Flash 容量为 256K
另外也可以在.icf连接文件中找到：
define symbol __ICFEDIT_region_ROM_start__    = 0x08000000;
define symbol __ICFEDIT_region_ROM_end__      = 0x0803FFFF;
可以看到 0x08000000 - 0x0803FFFF的 256K 内容就是STM32的Flash大小

note:STM32内部 FLASH介绍
STM32内部 FLASH 包含
1.主存储器
一般说STM32内部 FLASH 时都是指这个主存储器区域，它是存储用户应用程序的空间.
2.系统存储器
系统存储区是用户不能访问的区域，它在芯片出厂时已经固化了启动代码，它负
责实现串口、 USB 以及 CAN 等 ISP 烧录功能。
3.OTP区
OTP(One Time Program)，指的是只能写入一次的存储区域，容量为 512 字节，写
入后数据就无法再更改， OTP 常用于存储应用程序的加密密钥。
4.选项字节区
选项字节用于配置 FLASH 的读写保护、电源管理中的 BOR 级别、软件/硬件看门
狗等功能，这部分共 32 字节。可以通过修改 FLASH 的选项控制寄存器修改。

note:STM32内部FLASH的写入过程：
1. 解锁
芯片复位后默认会结FLASH上锁，这时不允许设置FLASH的控制寄存器，
并且不能修改FLASH中的内容。解锁的操作步骤如下：
(1) 往 Flash 密钥寄存器 FLASH_KEYR 中写入     KEY1 = 0x45670123
(2) 再往 Flash 密钥寄存器 FLASH_KEYR 中写入   KEY2 = 0xCDEF89AB

2. 数据操作位数
在内部FLASH进行擦除及写入操作时，电源电压会影响数据的最大操作位数，该电
源电压可通过配置FLASH_CR寄存器中的PSIZE位改变，
电压范围        2.7 - 3.6 V(使用外部 Vpp)  2.7 - 3.6 V  2.1 – 2.7 V  1.8 – 2.1 V
位数              64                          32              16          8
PSIZE(1:0)配置    11b                         10b             01b         00b
最大操作位数会影响擦除和写入的速度，64位宽度的操作除了配置寄存器位外，
还需要在Vpp引脚外加一个8-9V的电压源，且其供电时间不得超过一小时，否则FLASH
可能损坏，所以64位宽度的操作一般是在量产时对FLASH写入应用程序时才使用，
大部分应用场合都是用32位的宽度

3. 擦除扇区
在写入新的数据前，需要先擦除存储区域， STM32提供了扇区擦除指令和整个
FLASH擦除(批量擦除)的指令，批量擦除指令仅针对主存储区。
扇区擦除的过程如下：
(1) 检查FLASH_SR寄存器中的“忙碌寄存器位 BSY”，确认当前未执行任何Flash操作；
(2) 在FLASH_CR寄存器中，将“激活扇区擦除寄存器位SER”置 1，并设置“扇区编号寄存器位 SNB”，选择要擦除的扇区；
(3) 将 FLASH_CR 寄存器中的“开始擦除寄存器位 STRT ”置 1，开始擦除；
(4) 等待 BSY 位被清零时，表示擦除完成。

4. 写入数据
擦除完毕后即可写入数据，写入数据的过程并不是仅仅使用指针向地址赋值，赋值前
还还需要配置一系列的寄存器，步骤如下：
(1) 检查FLASH_SR中的BSY位，以确认当前未执行任何其它的内部Flash操作；
(2) 将FLASH_CR寄存器中的 “激活编程寄存器位 PG” 置 1；
(3) 针对所需存储器地址（主存储器块或 OTP 区域内）执行数据写入操作；
(4) 等待BSY位被清零时，表示写入完成。

note:查看工程的空间分布
在使用内部FLASH存储其它数据前需要了解哪一些空间已经写入了程序代码，
存储了程序代码的扇区都不应作任何修改。通过查询.map文件可以了解程序存储到了哪些区域
*/
