#include "FreeRTOS.h"
#include "string.h"
#include "exfuns.h"
#include "fattester.h"	

#define mymalloc(size) pvPortMalloc(size) 
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//FATFS ��չ����	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2014/3/14
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////

 //�ļ������б�
const unsigned char *FILE_TYPE_TBL[6][13]=
{
{"BIN"},			//BIN�ļ�
{"LRC"},			//LRC�ļ�
{"NES"},			//NES�ļ�
{"TXT","C","H"},	//�ı��ļ�
{"MP1","MP2","MP3","MP4","M4A","3GP","3G2","OGG","ACC","WMA","WAV","MID","FLAC"},//�����ļ�
{"BMP","JPG","JPEG","GIF"},//ͼƬ�ļ�
};
///////////////////////////////�����ļ���,ʹ��malloc��ʱ��////////////////////////////////////////////
FATFS *fs[2];  		//�߼����̹�����.	 
FIL *file;	  		//�ļ�1
FIL *ftemp;	  		//�ļ�2.
UINT br,bw;			//��д����
FILINFO fileinfo;	//�ļ���Ϣ
DIR dir;  			//Ŀ¼

unsigned char *fatbuf;			//SD�����ݻ�����
///////////////////////////////////////////////////////////////////////////////////////
//Ϊexfuns�����ڴ�
//����ֵ:0,�ɹ�
//1,ʧ��
unsigned char exfuns_init(void)
{
	fs[0]=(FATFS*)mymalloc(sizeof(FATFS));	//Ϊ����0�����������ڴ�	
	fs[1]=(FATFS*)mymalloc(sizeof(FATFS));	//Ϊ����1�����������ڴ�
	file=(FIL*)mymalloc(sizeof(FIL));		//Ϊfile�����ڴ�
	ftemp=(FIL*)mymalloc(sizeof(FIL));		//Ϊftemp�����ڴ�
	fatbuf=(unsigned char*)mymalloc(512);				//Ϊfatbuf�����ڴ�
	if(fs[0]&&fs[1]&&file&&ftemp&&fatbuf)return 0;  //������һ��ʧ��,��ʧ��.
	else return 1;	
}

//��Сд��ĸתΪ��д��ĸ,���������,�򱣳ֲ���.
unsigned char char_upper(unsigned char c)
{
	if(c<'A')return c;//����,���ֲ���.
	if(c>='a')return c-0x20;//��Ϊ��д.
	else return c;//��д,���ֲ���
}	      
//�����ļ�������
//fname:�ļ���
//����ֵ:0XFF,��ʾ�޷�ʶ����ļ����ͱ��.
//����,����λ��ʾ��������,����λ��ʾ����С��.
unsigned char f_typetell(unsigned char *fname)
{
	unsigned char tbuf[5];
	unsigned char *attr='\0';//��׺��
	unsigned char i=0,j;
	while(i<250)
	{
		i++;
		if(*fname=='\0')break;//ƫ�Ƶ��������.
		fname++;
	}
	if(i==250)return 0XFF;//������ַ���.
 	for(i=0;i<5;i++)//�õ���׺��
	{
		fname--;
		if(*fname=='.')
		{
			fname++;
			attr=fname;
			break;
		}
  	}
	strcpy((char *)tbuf,(const char*)attr);//copy
 	for(i=0;i<4;i++)tbuf[i]=char_upper(tbuf[i]);//ȫ����Ϊ��д 
	for(i=0;i<6;i++)
	{
		for(j=0;j<13;j++)
		{
			if(*FILE_TYPE_TBL[i][j]==0)break;//�����Ѿ�û�пɶԱȵĳ�Ա��.
			if(strcmp((const char *)FILE_TYPE_TBL[i][j],(const char *)tbuf)==0)//�ҵ���
			{
				return (i<<4)|j;
			}
		}
	}
	return 0XFF;//û�ҵ�		 			   
}	 

//�õ�����ʣ������
//drv:���̱��("0:"/"1:")
//total:������	 ����λKB��
//free:ʣ������	 ����λKB��
//����ֵ:0,����.����,�������
unsigned char exf_getfree(unsigned char *drv,unsigned int *total,unsigned int *free)
{
	FATFS *fs1;
	unsigned char res;
    unsigned int fre_clust=0, fre_sect=0, tot_sect=0;
    //�õ�������Ϣ�����д�����
    res =(unsigned int)f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res==0)
	{											   
	    tot_sect=(fs1->n_fatent-2)*fs1->csize;	//�õ���������
	    fre_sect=fre_clust*fs1->csize;			//�õ�����������	   
#if _MAX_SS!=512				  				//������С����512�ֽ�,��ת��Ϊ512�ֽ�
//		tot_sect*=fs1->ssize/512;
//		fre_sect*=fs1->ssize/512;
		
#endif	  
		*total=tot_sect>>1;	//��λΪKB
		*free=fre_sect>>1;	//��λΪKB 
 	}
	return res;
}		   
/////////////////////////////////////////////////////////////////////////////////////////////////////////////




















