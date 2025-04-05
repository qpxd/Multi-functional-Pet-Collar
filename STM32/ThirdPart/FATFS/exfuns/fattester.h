#ifndef __FATTESTER_H
#define __FATTESTER_H 			   
  
#include "ff.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//FATFS ���Դ���	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2014/3/14
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
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





























