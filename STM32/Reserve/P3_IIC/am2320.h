/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : am2320.h
* Description: am2320头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/02
*****************************************************************************/

#ifndef __AM2320_H__
#define __AM2320_H__

#define AM2320_ADDR  (0x5C << 1)  /*器件地址*/

typedef struct __AM2320 c_am2320;

typedef struct __AM2320
{
    void* this;
    
    /************************************************* 
    * Function: get 
    * Description: 获取温湿度
    * Input : <this>  am2320对象
    * Output: <temp>   当前温度 单位度 精确到小数点后一位，精度为 +-0.5C
    *         <humi>   当前湿度 百分比 精确到小数点后一位，精度为 +-3.0%
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float temp = 0.0f,humi = 0.0f;
    *
    *         ret = am2320.get(&am2320,&temp,&humi);
    *         if(E_OK != ret)
    *         {
    *             log_error("am2320 get failed.");
    *         }
    *         log_inform("temp:%.2f,humi:%.2f.",temp,humi);
    *************************************************/  
    int (*get)(c_am2320* this,float* temperature,float* humidity);
}c_am2320;

/************************************************* 
* Function: am2320_creat 
* Description: 创建一个am2320对象
* Input : <iic>  am2320所在的iic总线
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_am2320 am2320 = {0};
*         
*	      am2320 = am2320_creat(&iic);
*         if(NULL == am2320.this)
*         {
*             log_error("am2320 creat failed.");
*         }
*************************************************/
c_am2320 am2320_creat(const c_my_iic* iic);

#endif

