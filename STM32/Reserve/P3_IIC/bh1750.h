/*****************************************************************************
* Copyright:
* File name: bh1750.h
* Description: 光照模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/01/11
*****************************************************************************/

#ifndef __BH1750FVI_H__
#define __BH1750FVI_H__


#define BH1750_MODE_HH   0  /*0.51lux 精度*/
#define BH1750_MODE_H    1  /*1   lux 精度*/
#define BH1750_MODE_L    2  /*41  lux 精度*/

#define BH1750_ADDR_H   0x46//(0x23 << 1)  /*ADDR 为H时 地址*/
#define BH1750_ADDR_L   0xb8//(0x5c << 1)  /*ADDR 为L时 地址*/

typedef struct __BH1750 c_bh1750;

typedef struct __BH1750
{
    void* this;

    /************************************************* 
    * Function: start 
    * Description: 启动光强转换
    * Input : <this>  1750对象
    *         <mode>  模式
    *                (BH1750_MODE_HH)  以0.51lux 精度连续转换，转换时间为120ms
    *                (BH1750_MODE_H)   以  1 lux 精度连续转换，转换时间为120ms
    *                (BH1750_MODE_L)   以41   lux 精度连续转换，转换时间为16 ms
    * Output: 无
    * Return: <E_OK>     启动成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         ret = bh1750.start(&bh1750,BH1750_MODE_H);
    *         if(E_OK != ret)
    *         {
    *             log_error("bh1750 start failed.");
    *         }
    *************************************************/   
    int (*start)(const c_bh1750* this,u8 mode);

    /************************************************* 
    * Function: get 
    * Description: 获取当前光照强度
    * Input : <this>  1750对象
    * Output: <lux>   当前光照强度 单位lux 勒克斯
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_PARAM>  参数错误
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         u16 lux = 0;
    *
    *         ret = bh1750.get(&bh1750,&lux);
    *         if(E_OK != ret)
    *         {
    *             log_error("bh1750 get failed.");
    *         }
    *************************************************/  
    int (*get)(const c_bh1750* this,u16* lux);
}c_bh1750;

/************************************************* 
* Function: bh1750_creat 
* Description: 创建一个1750对象
* Input : <iic>  1750所在的iic总线
*         <addr> 1750的器件地址  只能为BH1750_ADDR_H 或 BH1750_ADDR_L
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_bh1750 bh1750 = {0};
*         
*	      bh1750 = bh1750_creat(&iic,BH1750_ADDR_H);
*         if(NULL == bh1750.this)
*         {
*             log_error("bh1750 creat failed.");
*         }
*************************************************/
c_bh1750 bh1750_creat(const c_my_iic* iic,u8 addr);


/**********************************************************************************************/
#endif

