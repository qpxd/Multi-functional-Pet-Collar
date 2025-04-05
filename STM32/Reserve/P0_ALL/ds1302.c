/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : ds1302.h
* Description: ds1302 驱动模块
* Author: xzh
* Version: v1.0
* Date: 2022/12/07
*****************************************************************************/

#include "driver_conf.h"


#ifdef DRIVER_DS1302_ENABLED

/* Variables -------------------------------------------------------------------*/
typedef struct __m_ds1302
{
    GPIO_TypeDef* clk_gpio; /*时钟脚*/
    uint32_t clk_pin;
    GPIO_TypeDef* dat_gpio;  /*数据脚*/
    uint32_t dat_pin; 
    GPIO_TypeDef* rst_gpio;  /*复位脚*/
    uint32_t rst_pin; 
}m_ds1302;


/* Private function declare-----------------------------------------------------*/   
static int m_init(const c_ds1302* this);
static int m_set_time(const c_ds1302* this, sys_time_t *pTime);
static int m_get_time(const c_ds1302* this, sys_time_t *pTime);
static int m_set_ram(const c_ds1302* this, uint8_t ucAddr, uint8_t ucData);
static int m_get_ram(const c_ds1302* this, uint8_t ucAddr, uint8_t *pcData);

uint8_t prvSwitchDatMode(m_ds1302* m_this, uint8_t ucMode);
void prvSendByte(m_ds1302* m_this, uint8_t ucData);
void prvWriteRegister(m_ds1302* m_this, uint8_t ucAddr, uint8_t ucData);
uint8_t prvReadRegister(m_ds1302* m_this, uint8_t ucAddr);

void mDelayuS(uint16_t xUs);
void mDelaymS(uint16_t xMs);
/* Public  ---------------------------------------------------------------------*/  
c_ds1302 ds1302_creat(GPIO_TypeDef* clk_gpio, uint32_t clk_pin, GPIO_TypeDef* dat_gpio, uint32_t dat_pin, GPIO_TypeDef* rst_gpio, uint32_t rst_pin)
{
    int ret = 0;
	GPIO_InitTypeDef GPIO_Initure;
    c_ds1302    new = {0};
    m_ds1302*   m_this = NULL;  

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_ds1302));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this, 0, sizeof(m_ds1302));
    m_this = new.this; 

    /*保存相关变量*/
    new.init            = m_init;
    new.set_time        = m_set_time;
    new.get_time        = m_get_time;

    new.get_ram        = m_get_ram;
    new.set_ram        = m_set_ram;

    m_this->clk_gpio    = clk_gpio;
    m_this->clk_pin     = clk_pin;
    m_this->dat_gpio    = dat_gpio;
    m_this->dat_pin     = dat_pin;
    m_this->rst_gpio    = rst_gpio;
    m_this->rst_pin     = rst_pin;

    /* 初始化CLK */
    GPIO_Initure.Pin   = m_this->rst_pin          ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP      ;
    GPIO_Initure.Pull  = GPIO_NOPULL              ;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH     ;
    HAL_GPIO_Init(m_this->rst_gpio,&GPIO_Initure) ;  
    /* 初始化CLK */
    GPIO_Initure.Pin   = m_this->clk_pin          ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP      ;
    GPIO_Initure.Pull  = GPIO_NOPULL              ;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH     ;
    HAL_GPIO_Init(m_this->clk_gpio,&GPIO_Initure) ;  
    /*初始化DAT*/
    GPIO_Initure.Pin   = m_this->dat_pin     ;
    GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP ;  //输出
    GPIO_Initure.Pull  = GPIO_PULLUP         ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;  //高速
    HAL_GPIO_Init(m_this->dat_gpio,&GPIO_Initure); 

    uint8_t ucTemp = 0;
    sys_time_t xTime;

    new.get_ram(&new, DS_RAM_MAX_ADDR, &ucTemp);
    if(ucTemp != DS_TIME_VAILID)
    {
        log_error("Time invalid, reset system time.");
        xTime.uxYear = DS_DEF_YEAR;
        xTime.ucMon = DS_DEF_MON;
        xTime.ucDate = DS_DEF_DATE;
        xTime.ucHour = DS_DEF_HOUR;
        xTime.ucMin = DS_DEF_MIN;
        xTime.ucSec = DS_DEF_SEC;
        new.set_time(&new, &xTime);
        new.get_time(&new, &xTime);
        log_error("Reset ok, current time: %04d/%02d/%02d %02d:%02d:%02d",
        xTime.uxYear, xTime.ucMon, xTime.ucDate, xTime.ucHour, xTime.ucMin, xTime.ucSec);
		new.set_ram(&new, DS_RAM_MAX_ADDR, DS_TIME_VAILID);
    }
    else
    {
        new.get_time(&new, &xTime);
        log_error("Current time: %04d/%02d/%02d %02d:%02d:%02d",
        xTime.uxYear, xTime.ucMon, xTime.ucDate, xTime.ucHour, xTime.ucMin, xTime.ucSec);
    }


    return new;
}

/* Private  ------------------------------------------------------------------*/ 
static int m_init(const c_ds1302* this)
{
	int ret = 0;
    m_ds1302* m_this = NULL;
    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_ERROR;
    }
    m_this = this->this;
    return E_OK;
}

static int m_get_time(const c_ds1302* this, sys_time_t *pTime)
{
    m_ds1302* m_this = NULL;
    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == pTime)
    {
        log_error("Null pointer.");
        return E_ERROR;
    }
    m_this = this->this;

    uint8_t ucTemp;
    ucTemp = prvReadRegister(m_this, 0x81);   //读秒
    pTime->ucSec = BCD_TO_DEC(ucTemp);
    ucTemp = prvReadRegister(m_this, 0x83);   //读分
    pTime->ucMin = BCD_TO_DEC(ucTemp);
    ucTemp = prvReadRegister(m_this, 0x85);   //读时
    pTime->ucHour = BCD_TO_DEC(ucTemp);
    ucTemp = prvReadRegister(m_this, 0x87);   //读天
    pTime->ucDate = BCD_TO_DEC(ucTemp);
    ucTemp = prvReadRegister(m_this, 0x89);   //读月
    pTime->ucMon = BCD_TO_DEC(ucTemp);
    ucTemp = prvReadRegister(m_this, 0x8d);   //读年
    pTime->uxYear = BCD_TO_DEC(ucTemp);
    pTime->uxYear += 2000;
    
    // ucTemp = prvReadRegister(m_this, 0x8d);   //读星期
    // pTime->ucReserve = BCD_TO_DEC(ucTemp);
    return E_OK;
}

static int m_set_time(const c_ds1302* this, sys_time_t *pTime)
{
    m_ds1302* m_this = NULL;
    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == pTime)
    {
        log_error("Null pointer.");
        return E_ERROR;
    }
    m_this = this->this;

    if(pTime->uxYear < 2000)
    {
        log_error("Invaild data.");
        return E_ERROR; 
    }

    uint8_t ucTemp = pTime->uxYear - 2000;
    prvWriteRegister(m_this, 0x8e, 0x00); //关闭写保护
    prvWriteRegister(m_this, 0x80, DEC_TO_BCD(pTime->ucSec)); // seconds00秒
    prvWriteRegister(m_this, 0x82, DEC_TO_BCD(pTime->ucMin)); // minutes00分
    prvWriteRegister(m_this, 0x84, DEC_TO_BCD(pTime->ucHour)); // hours9时
    prvWriteRegister(m_this, 0x86, DEC_TO_BCD(pTime->ucDate)); // date17日
    prvWriteRegister(m_this, 0x88, DEC_TO_BCD(pTime->ucMon)); // months6月
    prvWriteRegister(m_this, 0x8c, DEC_TO_BCD(ucTemp)); // year2022年
    prvWriteRegister(m_this, 0x8e, 0x80); //打开写保护

    return E_OK;
}

static int m_set_ram(const c_ds1302* this, uint8_t ucAddr, uint8_t ucData)
{
    m_ds1302* m_this = NULL;
    /*参数检测*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_ERROR;
    }
    if(ucAddr > DS_RAM_MAX_ADDR)
    {
        log_error("Invaild address. ");
        return E_ERROR;
    }
    m_this = this->this;

    uint8_t ucTemp;
	prvWriteRegister(m_this, 0x8e, 0x00); //关闭写保护
    ucTemp = (1<<7) | (1<<6) | (ucAddr<<1) | (0<<0); //默认 | RAM | 地址 | 写
    prvWriteRegister(m_this, ucTemp, ucData);   //写ram
    prvWriteRegister(m_this, 0x8e, 0x80); //打开写保护
    return E_OK;
}


static int m_get_ram(const c_ds1302* this,  uint8_t ucAddr, uint8_t *pcData)
{
    m_ds1302* m_this = NULL;
    /*参数检测*/
    if(NULL == this || NULL == this->this || NULL == pcData)
    {
        log_error("Null pointer.");
        return E_ERROR;
    }
    if(ucAddr > DS_RAM_MAX_ADDR)
    {
        log_error("Invaild address. ");
        return E_ERROR;
    }
    m_this = this->this;

    uint8_t ucTemp;
    ucTemp = (1<<7) | (1<<6) | (ucAddr<<1) | (1<<0); //默认 | RAM | 地址 | 读
    ucTemp = prvReadRegister(m_this, ucTemp);   //ram
    *pcData = ucTemp;

    return E_OK;
}


//切换数据引脚的模式 0输出, 1输入
uint8_t prvSwitchDatMode(m_ds1302* m_this, uint8_t ucMode)
{
	GPIO_InitTypeDef GPIO_Initure;
    GPIO_Initure.Pin   = m_this->dat_pin     ;

    if(ucMode == 0) GPIO_Initure.Mode  = GPIO_MODE_OUTPUT_PP ;  
    else            GPIO_Initure.Mode  = GPIO_MODE_INPUT ;  
    
    GPIO_Initure.Pull  = GPIO_PULLUP         ;  //上拉
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;  //高速
    HAL_GPIO_Init(m_this->dat_gpio,&GPIO_Initure); 
}

uint8_t prvReadRegister(m_ds1302* m_this, uint8_t ucAddr)
{
    uint8_t ret = 0;
    DS_CLK_CTRL(0);          //拉低时钟线
    prvSwitchDatMode(m_this, 0);
    DS_RST_CTRL(1);          //拉高复位

    for(uint8_t i = 0; i < 8; i++)
    {
        (ucAddr&0x01) == 0 ? DS_DAT_CTRL(0) : DS_DAT_CTRL(1); //发送一位数据
        ucAddr >>= 1;
        mDelayuS(DS_CYCLE);             //等待数据读取完成
        DS_CLK_CTRL(1);                  //拉高时钟线，通知GM读取数据
        mDelayuS(DS_CYCLE);             //等待数据读取完成
        if(i == 7)  prvSwitchDatMode(m_this, 1);    //在clk拉低之前执行，防止rtc数据传输过来时，引脚仍为输出
        DS_CLK_CTRL(0);                  //拉低时钟线
    }

    for(uint8_t i = 0; i < 8; i++)
    {
        ret = ret >> 1; //读数据变量
        if(DS_DAT_READ() == 0) ret &= 0x7f;
        else                   ret |= 0x80;
        DS_CLK_CTRL(1);                  //拉高时钟线，通知GM读取数据
        mDelayuS(DS_CYCLE);             //等待数据读取完成
        DS_CLK_CTRL(0);                  //拉高时钟线，通知GM读取数据
//        mDelayuS(DS_CYCLE);             //等待数据读取完成
    }

    DS_RST_CTRL(0);          //拉复位
    return ret;
}

void prvWriteRegister(m_ds1302* m_this, uint8_t ucAddr, uint8_t ucData)
{
    DS_CLK_CTRL(0);          //拉低时钟线
    prvSwitchDatMode(m_this, 0);       //IO配置为输出模式
    DS_RST_CTRL(1);          //拉高复位

    prvSendByte(m_this, ucAddr);      //发送地址
    prvSendByte(m_this, ucData);    //发送数据

    DS_DAT_CTRL(0);
    prvSwitchDatMode(m_this, 1);
    DS_RST_CTRL(0);          //拉低复位
}


void prvSendByte(m_ds1302* m_this, uint8_t ucData)
{
    for(uint8_t i = 0; i < 8; i++)
    {
        (ucData&0x01) == 0 ? DS_DAT_CTRL(0) : DS_DAT_CTRL(1); //发送一位数据
        mDelayuS(DS_CYCLE);     //等待数据读取完成
        DS_CLK_CTRL(1);            //拉高时钟线，通知GM读取数据
        mDelayuS(DS_CYCLE);     //等待数据读取完成
        DS_CLK_CTRL(0);            //拉低时钟线，准备发送下一个数据
        ucData >>= 1;
    }
}

void mDelayuS(uint16_t xUs)
{
	delay_us(xUs);
}

void mDelaymS(uint16_t xMs)
{
    do
    {
		mDelayuS(1000);
    }while(--xMs);
}
// /*判断是否为闰年*/
// uint8_t gm1302_judge_leap(uint16_t year)
// {
//     uint8_t ret = 0;
//     if((year%4) == 0){
//         ret = (year%100) == 0 ? !(year%400) : 1;
//     }
//     return ret;
// }

// /*年月日转换为天*/
// uint16_t gm1302_get_day(sys_time_t *pxTime)
// {
//     uint16_t ret = 0;
//     const uint8_t day[11] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30};
//     for (uint16_t i = 0; i < pxTime->mon - 1; i++) {
//         ret += day[i];
//     }
//     if(gm1302_judge_leap(pxTime->year + 2000) != 0 && pxTime->mon > 2){
//         ret += 1;
//     }
//     ret += pxTime->day;
//     return ret;
// }

// /*计算时间差*/
// int16_t gm1302_calculate_day_diff(sys_time_t *pxCurTime, sys_time_t *pxRefTime)
// {
//     int16_t curdays = gm1302_get_day(pxCurTime);
//     int16_t refdays = gm1302_get_day(pxRefTime);
//     return (curdays - refdays);
// }

#endif
