/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : spi.h
* Description: spi通讯模块 公开头文件
* Author: xzh
* Version: v1.1
* Date: 2022/04/08
* log:  V1.0  2022/02/06
*       发布：
*
*       V1.1  2022/04/02
*       变更：分配系数从SPI_BAUDRATEPRESCALER_2 更改为SPI_BAUDRATEPRESCALER_4。
*****************************************************************************/

#ifndef __SPI_H__
#define __SPI_H__

#define MY_SPI_1  0
#define MY_SPI_2  1

typedef struct __C_SPI
{
    int (*init)  (u8 spi);
    int (*enable)(u8 spi,bool en);
	int (*set_speed)(u8 spi,u8 speed);
    int (*set_datasize)(u8 spi,u8 datasize);
    int (*transmission)(u8 spi,const u8* send,u8* recv,u16 len,bool incremental,TickType_t time);
}c_spi;

extern const c_spi my_spi;

#endif


