/*****************************************************************************
* Copyright:
* File name: rc522.h
* Description: rc522模块 公开头文件
* Author: 许少
* Version: V1.0
* Date: 2022/03/08
* log:  V1.0  2022/03/08
*       发布：
*****************************************************************************/

#ifndef __RC522_H__
#define __RC522_H__



#define RC522_FIND      1   /*找到卡*/
#define RC522_LEAVE     0   /*卡离开*/

typedef struct __RC522_CFG
{
    GPIO_TypeDef*   o_gpio_rst  ; /*复位脚*/
    u32             o_pin_rst   ;
    GPIO_TypeDef*   o_gpio_miso ; /*miso*/
    u32             o_pin_miso  ;
    GPIO_TypeDef*   o_gpio_mosi ;
    u32             o_pin_mosi  ;
    GPIO_TypeDef*   o_gpio_sck  ;
    u32             o_pin_sck   ;
    GPIO_TypeDef*   o_gpio_sda  ;
    u32             o_pin_sda   ;
}rc522_cfg;

typedef struct __C_RC522
{  
    /************************************************* 
    * Function: init 
    * Description: 初始化rc522
    * Input : <cfg>      rc522引脚配置
    * Output: 无
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         rc522_cfg cfg = {GPIOA,GPIO_PIN_7,GPIOA,GPIO_PIN_5,GPIOA,GPIO_PIN_4,GPIOA,GPIO_PIN_3,GPIOA,GPIO_PIN_2};
    *         
    *         ret = rc522.init(&cfg);
    *         if(E_OK != ret )
    *         {
    *             log_error("rc522 init failed.");
    *         }
    *************************************************/  
    int (*init)(const rc522_cfg* cfg);     

    /************************************************* 
    * Function: set_callback 
    * Description: 获取当前GPS信息
    * Input : <param>    回调传递参数
    *         <callback> 回调函数
    * Output: <msg>       nmea gga信息 成员见 struct nmea_gga
    * Return: <E_OK>     获取成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
    *         int rfid_callback(void* param,u8 event,const u8* data)
    *         {
    *             如果找到卡  输出卡sn码
    *             if(RC522_FIND == event)
    *             {
    *                 log_inform("Get card:%#X,%#X,%#X,%#X.",data[0],data[1],data[2],data[3]);
    *             }
    *             
    *             卡离开
    *             else if(RC522_LEAVE == event)
    *             {
    *                 log_inform("Car leved.");
    *             }
    *             
    *             return E_OK;
    *         }
    *         
    *         ret = rc522.set_callback(NULL,rfid_callback);
    *         if(E_OK != ret )
    *         {
    *             log_error("rc522 set callback failed.");
    *         }
    *************************************************/      
    int (*set_callback)   (void* param,int (*callback)(void* param,u8 event,const u8* data));   
}c_rc522;

extern const c_rc522 rc522;


#endif

