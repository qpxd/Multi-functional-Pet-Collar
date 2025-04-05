/*****************************************************************************
* Copyright:
* File name: ph.h
* Description: ph传感器公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/02/28
*****************************************************************************/

#ifndef __PH_H__
#define __PH_H__



typedef struct __PH c_ph;

typedef struct __PH
{
    void* this;
    
    /************************************************* 
    * Function: get 
    * Description: 获取当前ph值
    * Input : <this>  c_gp2y对象
    * Output: <ph>    当前ph 0~14
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float ph = 0.0f;
    *
    *         ret = ph.get(&ph,&ph);
    *         if(E_OK != ret)
    *         {
    *             log_error("ph get failed.");
    *         }
    *         log_inform("ph:%.2f.",ph);
    *************************************************/      
    int (*get)(const c_ph* this,float* ph);
}c_ph;

/************************************************* 
* Function: ph_create 
* Description: 创建一个ph对象
* Input : <adc_ch>     数据采集的adc通道
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_ph  ph  = {0};
*         
*         ph = ph_create(MY_ADC_0);
*         if(NULL == ph.this)
*         {
*             log_error("ph creat failed.");
*         }
*************************************************/
c_ph ph_create(u8 adc_ch);


#endif

