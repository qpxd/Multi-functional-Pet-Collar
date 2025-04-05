/*****************************************************************************
* Copyright:
* File name: my_app.h
* Description: 我的app公开头文件
* Author: 许少
* Version: V1.0
* Date: 2021/12/20
*****************************************************************************/

#ifndef __DIS_DETECTION_H__
#define __DIS_DETECTION_H__

/*************************************************
* Function: 我的app初始化
* Description: my_app_init
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int dis_detection_init(void);

/*************************************************
* Function: 获取当前距离
* Description: dis_detection_get
* Input : 无
* Output: 无
* Return:
* Others: 无
*************************************************/
int dis_detection_get(float* dis);
	
#endif
