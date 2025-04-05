/**
******************************************************************************
* File Name          : my_flash.h
* Description        : This file contains all the functions prototypes for 
*                      the fsmc  
******************************************************************************
*/

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __my_flash_H
#define __my_flash_H

#ifdef __cplusplus
extern "C" {
#endif

/*
对于ATF413系列MCU Flash 有如下说明
@note: 	主存储器只有闪存容量为 64K 字节的片 1 闪存， 包含 64 个扇区， 每扇区大小为 1 K 字节。
		外部存储器容量可高达 16M 字节，包含 4096 扇区，每扇区大小为 4K 字节。
		
@note: 	主存储器只有闪存容量为 128K 字节的片 1 闪存， 包含 128 个扇区， 每扇区大小为 1 K 字节。
		外部存储器容量可高达 16M 字节，包含 4096 扇区，每扇区大小为 4K 字节。
		
@note:	主存储器只有片 1 闪存，该片闪存容量为 256K 字节， 包含 128 个扇区，每扇区大小为 2K 字节。
		外部存储器容量可高达 16M 字节，包含 4096 个扇区，每扇区大小为 4K 字节。
		
@note:	外部存储器的操作方法，包括读取、解锁、擦除、编程都跟主存储器相同，唯一区别是外部存储器编程
		只支持 32 位和 16 位操作，不支持 8 位操作
		
@note:	当要写入的地址未被提前擦除时，除非要写入的数据值是全 0，否则编程不被执行，
		并置位闪存状态寄存器（ FLASH_STS） 的 PRGMERR 位来告知编程发生错误。
*/

/* typedef  -------------------------------------------------------------*/
#define NVS_START			0x08018000								//NVS 起始地址 
#define NVS_TOTAL_SIZE		(32 * 1024)								//NVS 总分配内存 
#define NVS_SPCAE_SIZE		(4 * 1024)								//NVS 每个 NAMESPACE 的大小 

#define AT_FLASH_PAGE_SIZE	((uint32_t)0x400)						//Flash 页大小

#define NVS_DATA_START		(NVS_START+AT_FLASH_PAGE_SIZE)			//NVS 数据 起始地址
#define NVS_SPCAE_CNT		(NVS_TOTAL_SIZE/NVS_SPCAE_SIZE)			//最多支持的命名空间
#define NVS_ITEMS_SIZE		(16)									//数据项大小
#define NVS_ITEMS_CNT		(NVS_SPCAE_SIZE/NVS_ITEMS_SIZE)			//命名空间的总数据项大小

#define CMD_HASH 			0xb433e5c6

#define FLASH_VALID			0xfdfe
#define FLASH_INVALID		0xfcfc

#define ITEM_UNINITED		0xff
#define ITEM_HEAD			0xa5
#define ITEM_KEY			0xfe
#define ITEM_VALUE			0xfd
#define ITEM_INVALID		0xc0
/* typedef  -------------------------------------------------------------*/
#ifdef ONCHIP_FLASH_NVS_ENABLED
/* NVS PAGE
[type:1][data:13][flag:2]
[type:1][data:13][flag:2]
...                     
[type:1][data:13][flag:2]
*/

typedef struct
{
	uint32_t uxStatus;
	uint16_t uxVersion;
	char Reserve[8];
	uint16_t uxCrc16;
}page_header_t;

typedef struct
{
	uint8_t ucType;
	char arData[12];
	uint8_t ucLen;
	uint8_t arFlag[2];
}page_item_t;

typedef struct
{
	uint32_t uxHashCode;
	uint8_t uxPageIndex;
	uint8_t uxItemIndex;	
}hash_item_t;

typedef struct xC_NVS_T c_nvs_t;
typedef struct xC_NVS_T
{
	void *this;
	/************************************************* 
	* Function: find 
	* Description: 查找 key 在 namespace 中的位置
	* Input : <this>  	指向对象私有成员
	*         <pcKey> 	键名
	*         <pxIndex> 索引
	* Demo  :	uint16_t index;
	*		 	nvs.find(&nvs, "ADC", &index);
	*************************************************/ 
	uint8_t(*find)(const c_nvs_t*, char*, uint16_t*);
	
	/************************************************* 
	* Function: add 
	* Description: 添加一组键值对
	* Input : <this>  	指向对象私有成员
	*         <pcKey> 	键名
	*         <pcValue> 值
	*         <ucLen> 	值数据长度
	* Demo  :	nvs.add(&nvs, "ADC", "123");
	*************************************************/ 
	uint8_t(*add)(const c_nvs_t*, char*, void*, uint16_t);
	
	/************************************************* 
	* Function: get 
	* Description: 获取键对应的值
	* Input : <this>  	指向对象私有成员
	*         <pcKey> 	键名
	*         <pcValue> 值
	* Demo  :	char *arTemp[32];
	*		 	nvs.get(&nvs, "ADC", arTemp);
	*************************************************/ 
	uint8_t(*get)(const c_nvs_t*, char*, void*);
}c_nvs_t;

	
/* Public interface  ------------------------------------------------*/
/************************************************* 
* Function: nvs_open 
* Description: 打开指定命令空间， 不存在则直接创建
* Input : <pcName> 命令空间的命令，最大长度为12
* Demo  :
*	c_nvs_t nvs 	= {0};
*	nvs = nvs_open("ESP");	
*	if(NULL == nvs.this)	log_error("nvs open failed.");
*************************************************/ 
c_nvs_t nvs_open(char *pcName);

/************************************************* 
* Function: nvs_close 
* Description: 关闭指定命令空间， 操作完成后需要关闭，才能操作其他命名空间
* Input : <pcName> 命令空间的命令，最大长度为12
* Demo  :
*	uint8_t ret;
*	ret = nvs_close(&nvs);	
*	if(0 != ret)	log_error("nvs closed failed.");
*************************************************/ 
uint8_t nvs_close(c_nvs_t *this);

/************************************************* 
* Function: nvs_delete 
* Description: 从Flash中删除命令空间，删除后此命名空间将不存在
* Input : <pcName> 命令空间的命令，最大长度为12
* note: 删除前nvs必须处于关闭状态
* Demo  :
*	uint8_t ret;
*	ret = nvs_delete("ESP");	
*	if(0 != ret)	log_error("nvs elete failed.");
*************************************************/ 
uint8_t nvs_delete(char *pcName);

/* Demo  ------------------------------------------------------------*/
/********************************************** 
Demo:
//	c_nvs_t nvs 	= {0};
//	nvs = nvs_open("ESP");	//打开命名空间, 不存在则创建
//	if(NULL == nvs.this)	log_error("nvs open failed."); 
//	nvs.add(&nvs, "ADC", "ADCdsadads", strlen("ADCdsadads"));				//添加项
//	nvs.add(&nvs, "AbC", "AbC12341111", strlen("AbC12341111"));	
//	nvs.add(&nvs, "AbC", "AbC12342222", strlen("AbC12342222"));	
//	nvs.find(&nvs, "ADC", &index);
//	nvs.find(&nvs, "AbC", &index);
//	memset(temp, 0, sizeof(temp));
//	nvs.get(&nvs, "ADC", temp);
//	memset(temp, 0, sizeof(temp));
//	nvs.get(&nvs, "AbC", temp);
//	//关闭命名空间，释放内存
//	if(nvs_close(&nvs) != 0)	log_error("nvs close failed."); 
//	//删除命名空间
//	nvs_delete("ESP");  //删除前nvs必须处于关闭状态
/*************************************************/

#endif

/* Macros  ------------------------------------------------------------------*/
#ifndef USE_EXTERN_MACROS   //如果不使用外部宏
#endif

/* Private macro -------------------------------------------------------------*/
/* public interface ------------------------------------------------------------------*/
int8_t vFlashEraseNPages(uint32_t xStartAddr, uint32_t xPageCnt);
int8_t vFlashWriteNBytes(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize);
int8_t vFlashReadNBytes(uint32_t xStartAddr, uint8_t *pBuffer, uint32_t xSize);

#ifdef __cplusplus
}
#endif
#endif
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

/*************DEBUG LOG**************
@note1: 代码地地址覆盖到了 nvs的地址，会造成重新烧录代码后之前的NVS数据丢失

@note2: 系统需要添加一个延时，不然有可能初始化擦除后未来得及写入就进调试 （后面需优化，只有写满才重置表格）






************DEBUG LOG END***************/

