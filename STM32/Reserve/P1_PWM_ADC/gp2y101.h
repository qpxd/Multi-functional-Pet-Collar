/*****************************************************************************
* Copyright:
* File name: gp2y101.h
* Description: pm2.5传感器公开头文件
* Author: 许少
* Version: V1.1
* Date: 2022/01/28
* log:  V1.0  2022/01/28
*       发布：
*       V1.1  2022/04/08
*       修复：m_get中 target 和 adc获取判断叠在一起的问题
*****************************************************************************/

#ifndef __GP2Y_H__
#define __GP2Y_H__




typedef struct __GP2Y c_gp2y;

typedef struct __GP2Y
{
    void* this;

    /************************************************* 
    * Function: get 
    * Description: 获取当前pm2.5浓度
    * Input : <this>  c_gp2y对象
    * Output: <pm2_5> 当前pm2.5浓度 单位 ug/m3   最大为500ug/m3
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         float pm2_5 = 0.0f;
    *
    *         ret = gp2y.get(&gp2y,&pm2_5);
    *         if(E_OK != ret)
    *         {
    *             log_error("gp2y get failed.");
    *         }
    *************************************************/  
    int (*get)(const c_gp2y* this,float* pm2_5);
}c_gp2y;

/************************************************* 
* Function: gp2y_create 
* Description: 创建一个gp2y101对象
* Input : <adc_ch>     数据采集的adc通道
*         <trig_gpio>  触发脚GPIO
*         <trig_pin>   触发脚PIN
* Output: 无
* Return: 新的对象拷贝 返回值中，this指针为空 表示创建失败
* Others: 无
* Demo  :
*         c_gp2y  gp2y  = {0};
*         
*         gp2y = gp2y_create(MY_ADC_0,GPIOA,GPIO_PIN_1);
*         if(NULL == gp2y.this)
*         {
*             log_error("gp2y creat failed.");
*         }
*************************************************/
c_gp2y gp2y_create(u8 adc_ch,GPIO_TypeDef* trig_gpio,uint32_t trig_pin);


#endif



