/*****************************************************************************
* Copyright:
* File name: my_interface.h
* Description: 界面应用 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2021/12/19
*****************************************************************************/
#ifndef __MY_INTERFACE_H__
#define __MY_INTERFACE_H__



/*************************************************
* Function: 界面应用初始化
* Description: my_interface_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int my_interface_init(void);


/*************************************************
* Function: 界面应用初始化
* Description: my_interface_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int my_interface_display(u8 line,char* format,...);

#endif


