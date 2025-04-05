/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : ds3231.h
* Description: ds3231时钟模块头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/03
*****************************************************************************/

#ifndef __DS3231_H__
#define __DS3231_H__


#define DS3231_ADDR  (0x68 << 1)  /*器件地址*/

typedef struct __DS3231 c_ds3231;

typedef struct __DS3231
{
    void* this;

    /************************************************* 
    * Function: set 
    * Description: 设置时间
    * Input : <this>  ds3231对象
    *         <time>  需要设置的时间
    * Output: 无
    * Return: <E_OK>     设置成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         time_type time = {0,45,20,4,3,2,22};
    *
    *         ret = ds3231.set(&ds3231,&time);
    *         if(E_OK != ret)
    *         {
    *             log_error("ds3231 set failed.");
    *         }
    *************************************************/  
    int (*set)(const c_ds3231* this,time_type* time);
    
    /************************************************* 
    * Function: get 
    * Description: 获取当前时间
    * Input : <this>  ds3231对象        
    * Output: <time>  当前时间
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         time_type time = {0};
    *
    *         ret = ds3231.get(&ds3231,&time);
    *         if(E_OK != ret)
    *         {
    *             log_error("ds3231 get failed.");
    *         }
    *         log_inform("20%d.%.2d.%.2d.%.2d:%.2d:%.2d",time.o_year,time.o_month,time.o_day,time.o_house,time.o_min,time.o_sec);
    *************************************************/ 
    int (*get)(const c_ds3231* this,time_type* time);
}c_ds3231;

/************************************************* 
* Function: ds3231_creat 
* Description: 创建一个ds3231对象
* Input : <iic>  ds3231所在的iic总线
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_ds3231 ds3231 = {0};
*         
*	      ds3231 = ds3231_creat(&iic);
*         if(NULL == ds3231.this)
*         {
*             log_error("ds3231 creat failed.");
*         }
*************************************************/
c_ds3231 ds3231_creat(const c_my_iic* iic);

#endif


