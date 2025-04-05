/*****************************************************************************
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name: max30102.c
* Description: max30102模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/03/08
* log:  V1.0  2022/03/08
*       发布：
*****************************************************************************/

#ifndef __SHT30_H__
#define	__SHT30_H__


typedef struct __SHT30 c_sht30;

typedef struct __SHT30
{
    void* this;
    
    int(*get)(const c_sht30* this,float* temp,float* humi);
}c_sht30;

c_sht30 sht30_create(const c_my_iic* iic);

#endif
