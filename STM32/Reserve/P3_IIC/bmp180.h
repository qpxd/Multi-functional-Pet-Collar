/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name: bmp180.h
* Description: 气压传感器驱动模块公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/02
*****************************************************************************/

#ifndef __BMP180_H__
#define __BMP180_H__


#define BMP180_ADDR  (0x77 << 1)  /*器件地址*/

#define BMP180_P_lOW_POWER              0  /*压力测量 超低功耗 4.5ms*/
#define BMP180_P_STANDARD               1  /*压力测量 标准 7.5ms*/
#define BMP180_P_HIGH_RESOLUTION        2  /*压力测量 高分辨率 13.5ms*/
#define BMP180_P_ULTRA_HIGHT_RESOLUTION 3  /*压力测量 超高分辨率 12ms*/

typedef struct __BMP180 c_bmp180;

typedef struct __BMP180 
{
    void* this;

    
    /************************************************* 
    * Function: get 
    * Description: 获取当前温度、海拔和气压
    * Input : <this>    bmp180对象
    *         <mode>    转换模式
    *                   (BMP180_P_lOW_POWER             )   压力测量 超低功耗 4.5ms 
    *                   (BMP180_P_STANDARD              )   压力测量 标准 7.5ms 
    *                   (BMP180_P_HIGH_RESOLUTION       )   压力测量 高分辨率 13.5ms 
    *                   (BMP180_P_ULTRA_HIGHT_RESOLUTION)   压力测量 超高分辨率 12ms
    * Output: <temp>     当前温度 单位 摄氏度
    *         <altitude> 当前海拔 单位 米
    *         <pa>       当前气压 单位 帕斯卡
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  : float t = 0.0f,alt = 0.0f;
    *         u32 pa = 0;
    *
    *         ret = bmp180.get(&bmp180,BMP180_P_STANDARD,&t,&alt,&pa);
    *         if(E_OK != ret)
    *         {
    *             log_error("bmp180 get failed.");
    *         }  
    *************************************************/   
    int (*get)(c_bmp180* this,u8 mode,float* temp,float* altitude,u32* pa);
}c_bmp180;

/************************************************* 
* Function: bmp180_creat 
* Description: 创建一个bmp180对象
* Input : <iic>  bmp180所在的iic总线
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_bmp180 bmp180 = {0};
*         
*	      bmp180 = bmp180_creat(&iic);
*         if(NULL == bmp180.this)
*         {
*             log_error("bmp180 creat failed.");
*         }
*************************************************/
c_bmp180 bmp180_creat(const c_my_iic* iic);


#endif



