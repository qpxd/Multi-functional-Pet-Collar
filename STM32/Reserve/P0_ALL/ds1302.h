/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : ds1302.h
* Description: ds1302 驱动模块 公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/25
*****************************************************************************/
#ifndef __DS1302_H__
#define __DS1302_H__

#define DS_CYCLE        5
#define DS_RAM_MAX_ADDR  	0x01
#define DS_TIME_VAILID      0xA5

//转码
#define BCD_TO_DEC(X) ((X>>4)*10 + (X&0X0F))

#define DEC_TO_BCD(X) (((X/10)<<4) | (X%10))

//通信IO 操作宏
#define DS_RST_CTRL(n)  HAL_GPIO_WritePin(m_this->rst_gpio, m_this->rst_pin, n)

#define DS_CLK_CTRL(n)  HAL_GPIO_WritePin(m_this->clk_gpio, m_this->clk_pin, n)

#define DS_DAT_CTRL(n)  HAL_GPIO_WritePin(m_this->dat_gpio, m_this->dat_pin, n)

#define DS_DAT_READ()  HAL_GPIO_ReadPin(m_this->dat_gpio, m_this->dat_pin)

//系统默认时间
#define DS_DEF_YEAR     2022
#define DS_DEF_MON      10
#define DS_DEF_DATE     1
#define DS_DEF_HOUR     12
#define DS_DEF_MIN      30
#define DS_DEF_SEC      50

typedef struct __C_DS1302 c_ds1302;
typedef struct __C_DS1302
{
    void* this;
	
    /************************************************* 
    * Description: 预留，暂未启用
    *************************************************/
    int(*init)          (const struct __C_DS1302* this);
	
	
    /************************************************* 
    * Description: 设置DS1302的时间
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
	*	      sys_time_t sys_time;
    *		  int ret = 0; 
    *         ret = ds1302.set_time(&ds1302, &sys_time);
    *         if(ret != E_OK)
    *         {
    *             log_error("ds1302 set time failed.");
    *         }
    *************************************************/
    int(*set_time)    (const c_ds1302* this, sys_time_t *pTime);
	
	
    /************************************************* 
    * Description: 获取DS1302的时间
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
	*	      sys_time_t sys_time;
    *		  int ret = 0; 
    *         ret = ds1302.get_time(&ds1302, &sys_time);
    *         if(ret != E_OK)
    *         {
    *             log_error("ds1302 get time failed.");
    *         }
    *************************************************/
    int(*get_time) (const c_ds1302* this, sys_time_t *pTime);

    /************************************************* 
    * Description: 获取DS1302 RAM区数据
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
	*		  uint8_t ucTemp = 0;
	*		  uint8_t ucAddr = 0x01;  //ram地址 0 - 30
    *		  int ret = 0; 
    *         ret = ds1302.get_ram(&ds1302, ucAddr, &ucTemp);
    *         if(ret != E_OK)
    *         {
    *             log_error("ds1302 get ram failed.");
    *         }
    *************************************************/
    int(*get_ram)  (const c_ds1302* this,  uint8_t ucAddr, uint8_t *pcData);

    /************************************************* 
    * Description: 设置DS1302 RAM区数据
    * Output: 无
    * Return: <E_OK>     操作成功
    *         <E_NULL>   空指针
    *         <E_ERROR>  操作失败
    * Others: 无
    * Demo  :
	*		  uint8_t ucTemp = 0xA5;
	*		  uint8_t ucAddr = 0x01;  //ram地址 0 - 30
    *		  int ret = 0; 
    *         ret = ds1302.set_ram(&ds1302, ucAddr, ucTemp);
    *         if(ret != E_OK)
    *         {
    *             log_error("ds1302 set ram failed.");
    *         }
    *************************************************/
    int(*set_ram)  (const c_ds1302* this,  uint8_t ucAddr, uint8_t ucData);
	
}c_ds1302;

/************************************************* 
* Function: ds1302_creat 
* Description: 创建 ds1302 实时时钟设备
* Input :  <clk_gpio><clk_pin> 时钟引脚
*          <dat_gpio><dat_pin> 数据引脚
*		   <rst_gpio><rst_pin> 复位引脚
* Output: 无
* Return: <E_OK>     操作成功
*         <E_NULL>   空指针
*         <E_ERROR>  操作失败
* Others: 无
* Demo  :
*		  c_ds1302 ds1302 = {0};
*         ds1302 = ds1302_creat(GPIOA, GPIO_PIN_9, GPIOA, GPIO_PIN_10, GPIOB, GPIO_PIN_11);
*         if(NULL == ds1302.this)
*         {
*             log_error("ds1302 creat failed.");
*         }
*************************************************/
c_ds1302 ds1302_creat(GPIO_TypeDef* clk_gpio, uint32_t clk_pin, GPIO_TypeDef* dat_gpio, uint32_t dat_pin, GPIO_TypeDef* rst_gpio, uint32_t rst_pin);

#endif

