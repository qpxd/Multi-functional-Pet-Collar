/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "driver_conf.h"
#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

#define mymalloc(size) pvPortMalloc(size)
#define myfree(ptr)    vPortFree(ptr)

#ifdef DRIVER_FATFS_ENABLED
	extern c_sd_card sd_card;
#endif

/* Definitions of physical drive number for each drive */
#define SD_CARD		0	/* Example: Map Ramdisk to physical drive 0 */
#define EX_FLASH	1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
  	return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{

u8 res=0;

	switch (pdrv) {
	case SD_CARD :
#ifdef DRIVER_FATFS_ENABLED					
//	      res = sd_card.init(&sd_card);//SD_Initialize() SD_Initialize
//		 	if(res)//STM32 SPI��bug,��sd������ʧ�ܵ�ʱ�������ִ����������,���ܵ���SPI��д�쳣
//			{
//				/*����spi�ٶ�*/
//				my_spi.set_speed(MY_SD_SPI,SPI_BAUDRATEPRESCALER_256);
//				my_spi.transmission(MY_SD_SPI,(u8*)0xFF,NULL,1,false,1000);
//				my_spi.set_speed(MY_SD_SPI,SPI_BAUDRATEPRESCALER_4);
//			}
#endif
       res=0;	
       break;
	case EX_FLASH :
		
          res=1;
		      break;
         
	case DEV_USB :

        res=1;
		    break;
	}
	if(res)return  STA_NOINIT;
	else return 0; //��ʼ���ɹ�
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	u8 res=0; 
    if (!count)return RES_PARERR;//count���ܵ���0�����򷵻ز�������		 	 
	switch(pdrv)
	{
		case SD_CARD://SD��
#ifdef DRIVER_FATFS_ENABLED			
			res=sd_card.ReadData(&sd_card,buff,sector,count);	 
		 	if(res)//STM32 SPI��bug,��sd������ʧ�ܵ�ʱ�������ִ����������,���ܵ���SPI��д�쳣
			{
				my_spi.set_speed(MY_SD_SPI,SPI_BAUDRATEPRESCALER_256);
				my_spi.transmission(MY_SD_SPI,(u8*)0xFF,NULL,1,false,1000);
				my_spi.set_speed(MY_SD_SPI,SPI_BAUDRATEPRESCALER_4);
			}
#endif
			break;
		case EX_FLASH://�ⲿflash
//			for(;count>0;count--)
//			{
//				SPI_Flash_Read(buff,sector*FLASH_SECTOR_SIZE,FLASH_SECTOR_SIZE);
//				sector++;
//				buff+=FLASH_SECTOR_SIZE;
//			}
			res=1;
			break;
		default:
			res=1; 
	}
   //������ֵ����SPI_SD_driver.c�ķ���ֵת��ff.c�ķ���ֵ
    if(res==0x00)return RES_OK;	 
    else return RES_ERROR;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	u8 res=0;  
    if (!count)return RES_PARERR;//count���ܵ���0�����򷵻ز�������		 	 
	switch(pdrv)
	{
		case SD_CARD://SD��
#ifdef DRIVER_FATFS_ENABLED				
			res=sd_card.WriteData(&sd_card,(u8*)buff,sector,count);
#endif
			break;
		case EX_FLASH://�ⲿflash
	
			res=1;
			break;
		default:
			res=1; 
	}
    //������ֵ����SPI_SD_driver.c�ķ���ֵת��ff.c�ķ���ֵ
    if(res == 0x00)return RES_OK;	 
    else return RES_ERROR;	
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;						  			     
	if(pdrv==SD_CARD)//SD��
	{
#ifdef DRIVER_FATFS_ENABLE
	    switch(cmd)
	    {
		    case CTRL_SYNC:
				/*ȡ��Ƭѡ*/
				
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_RESET);
				if(sd_card.WaitReady(&sd_card)==E_OK)
//				SD_CS=0;
//		    if(SD_WaitReady()==0) 				
					res = RES_OK; 
				else 
					res = RES_ERROR;	  
				HAL_GPIO_WritePin(GPIOB,GPIO_PIN_12,GPIO_PIN_SET);
		        break;	 
		    case GET_SECTOR_SIZE:
		        *(WORD*)buff = 512;
		        res = RES_OK;
		        break;	 
		    case GET_BLOCK_SIZE:
		        *(WORD*)buff = 8;
		        res = RES_OK;
		        break;	 
		    case GET_SECTOR_COUNT:
						sd_card.GetSectorCount(&sd_card);
		        *(DWORD*)buff = 0;
//		        *(DWORD*)buff = SD_GetSectorCount();				
		        res = RES_OK;
		        break;
		    default:
		        res = RES_PARERR;
		        break;
	    }
#endif
	}
	else if(pdrv==EX_FLASH)	//�ⲿFLASH  
	{
//	    switch(cmd)
//	    {
//		    case CTRL_SYNC:
//				res = RES_OK; 
//		        break;	 
//		    case GET_SECTOR_SIZE:
//		        *(WORD*)buff = FLASH_SECTOR_SIZE;
//		        res = RES_OK;
//		        break;	 
//		    case GET_BLOCK_SIZE:
//		        *(WORD*)buff = FLASH_BLOCK_SIZE;
//		        res = RES_OK;
//		        break;	 
//		    case GET_SECTOR_COUNT:
//		        *(DWORD*)buff = FLASH_SECTOR_COUNT;
//		        res = RES_OK;
//		        break;
//		    default:
//		        res = RES_PARERR;
//		        break;
		res=RES_ERROR; 
	    }
	else res=RES_ERROR;//�����Ĳ�֧��
    return res;
}

//���ʱ��
//User defined function to give a current time to fatfs module      */
//31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */                                                                                                                                                                                                                                          
//15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2) */                                                                                                                                                                                                                                                
DWORD get_fattime (void)
{				 
	return 0;
}			 
//��̬�����ڴ�
void *ff_memalloc (UINT size)			
{
	return (void*)mymalloc(size);
}
//�ͷ��ڴ�
void ff_memfree (void* mf)		 
{
	myfree(mf);
}

