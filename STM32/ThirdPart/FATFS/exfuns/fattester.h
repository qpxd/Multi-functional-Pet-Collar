#ifndef __FATTESTER_H
#define __FATTESTER_H 			   
  
#include "ff.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//FATFS 测试代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2014/3/14
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

 
unsigned char mf_mount(unsigned char* path,unsigned char mt);
unsigned char mf_open(unsigned char*path,unsigned char mode);
unsigned char mf_close(void);
unsigned char mf_read(unsigned short len);
unsigned char mf_write(unsigned char*dat,unsigned short len);
unsigned char mf_opendir(unsigned char* path);
unsigned char mf_closedir(void);
unsigned char mf_readdir(void);
unsigned char mf_scan_files(unsigned char * path);
unsigned int mf_showfree(unsigned char *drv);
unsigned char mf_lseek(unsigned int offset);
unsigned int mf_tell(void);
unsigned int mf_size(void);
unsigned char mf_mkdir(unsigned char*pname);
unsigned char mf_fmkfs(unsigned char* path,unsigned char mode,unsigned short au);
unsigned char mf_unlink(unsigned char *pname);
unsigned char mf_rename(unsigned char *oldname,unsigned char* newname);
void mf_getlabel(unsigned char *path);
void mf_setlabel(unsigned char *path); 
void mf_gets(unsigned short size);
unsigned char mf_putc(unsigned char c);
unsigned char mf_puts(unsigned char*c);
 
#endif





























