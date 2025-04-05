/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : ds3231.c
* Description: ds3231时钟模函数实现
* Author: xzh
* Version: v1.0
* Date: 2022/02/03
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_DS3231_ENABLED

#ifdef MODE_LOG_TAG
#undef MODE_LOG_TAG
#endif
#define MODE_LOG_TAG "ds3231"

#define DS3231_IIC_SPEED   I2C_SPEED_DEFAULT

#define TIM_SEC_REG     ((uint8_t)0x00)   /*秒寄存器           */
#define TIM_MIN_REG     ((uint8_t)0x01)   /*分寄存器           */
#define TIM_HOUR_REG    ((uint8_t)0x02)   /*时寄存器           */
#define TIM_WEEK_REG    ((uint8_t)0x03)   /*星期寄存器         */
#define TIM_DAY_REG     ((uint8_t)0x04)   /*日寄存器           */
#define TIM_MONTH_REG   ((uint8_t)0x05)   /*月寄存器           */
#define TIM_YEAR_REG    ((uint8_t)0x06)   /*年寄存器           */
                                            
#define AIM1_SEC_REG    ((uint8_t)0x07)   /*闹钟1秒寄存器      */
#define AIM1_MIN_REG    ((uint8_t)0x08)   /*闹钟1分寄存器      */
#define AIM1_HOUR_REG   ((uint8_t)0x09)   /*闹钟1时寄存器      */
#define AIM1_DAY_REG    ((uint8_t)0x0A)   /*闹钟1星期/日寄存器 */
                      
#define AIM2_MIN_REG    ((uint8_t)0x0B)   /*闹钟2分寄存器      */
#define AIM2_HOUR_REG   ((uint8_t)0x0C)   /*闹钟2时寄存器      */
#define AIM2_DAY_REG    ((uint8_t)0x0D)   /*闹钟2星期/日寄存器 */
                       
#define EOSC_REG        ((uint8_t)0x0E)   /*控制寄存器         */
#define OSF_REG         ((uint8_t)0x0F)   /*状态寄存器         */
#define CRYSTAL_REG     ((uint8_t)0x10)   /*晶体老化补偿寄存器 */
#define TEMP_MSB_REG    ((uint8_t)0x11)   /*温度寄存器高字节   */
#define TEMP_LSB_REG    ((uint8_t)0x12)   /*温度寄存器低字节   */

#define HOUR_SYS_BIT    ((uint8_t)(0x01 << 6)) /*1:12小时制  0:24小时制                 */

#define AIM_CFG_BIT     ((uint8_t)(0x01 << 7)) /*闹钟的配置位                           */
#define AIM_DY_DT_BIT   ((uint8_t)(0x01 << 6)) /*用于控制存储 于寄存器第0位至第5位的闹钟值是反映星期几还是月份 中的日期*/

#define EOSC_A1IE_BIT   ((uint8_t)(0x01 << 0)) /*闹钟1使能                              */
#define EOSC_A2IE_BIT   ((uint8_t)(0x01 << 1)) /*闹钟2使能                              */
#define EOSC_INTCN_BIT  ((uint8_t)(0x01 << 2)) /*中断控制 0：INT/SQW输出方波 1：输出闹钟中断*/
#define EOSC_RS1_BIT    ((uint8_t)(0x01 << 3)) /*方波使能时的频率选择位                 */
#define EOSC_RS2_BIT    ((uint8_t)(0x01 << 4)) /*方波使能时的频率选择位                 */
#define EOSC_CONV_BIT   ((uint8_t)(0x01 << 5)) /*为1时，将温度值转换为数字码，并执行TCXO算法更新振荡器的电容整列*/
#define EOSC_BBSQW_BIT  ((uint8_t)(0x01 << 6)) /*电池备份的方波使能                     */
#define EOSC_EOSC_BIT   ((uint8_t)(0x01 << 7)) /*使能振荡器，为0时使能                  */

#define OSF_A1F_BIT     ((uint8_t)(0x01 << 0)) /*闹钟1标志                              */
#define OSF_A2F_BIT     ((uint8_t)(0x01 << 1)) /*闹钟2标志                              */
#define OSF_BSY_BIT     ((uint8_t)(0x01 << 2)) /*忙位 器件正在执行TCXO功能              */
#define OSF_EN32KHZ_BIT ((uint8_t)(0x01 << 3)) /*是能32kHz输出                          */
#define OSF_OSF_BIT     ((uint8_t)(0x01 << 7)) /*振荡器停止标志 1标志振荡器曾经停止工作 */

static int m_set(const c_ds3231* this,time_type* time);
static int m_get(const c_ds3231* this,time_type* time);
static int m_read(const c_ds3231* this,u8 reg,u8* data,u8 len);
static int m_write(const c_ds3231* this,u8 reg,const u8* data,u8 len);
static void  bcd_to_hex(time_type* time);
static void  hex_to_bcd(time_type* time);

typedef struct _M_DS3231
{
    const c_my_iic*  m_iic   ;   /*所在的IIC总线*/
}m_ds3231;

/*默认时间 如果ds3231被断电 会写入的默认时间*/
const time_type g_default_time = 
{
    0x00 ,  /* 0秒  */
    0x00 ,  /*00分  */
    0x17 ,  /*17时  */
    0x04 ,  /*星期4 */
    0x03 ,  /*03号  */
    0x02 ,  /*01月  */
    0x22 ,  /*22年  */
};


c_ds3231 ds3231_creat(const c_my_iic* iic)
{
    c_ds3231  new = {0};
    m_ds3231* m_this = NULL;  
    int ret = 0;
    u8  data = 0;
    
    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_ds3231));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_ds3231));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic  = iic  ;
    new.get        = m_get;
    new.set        = m_set;

    /*设定为24小时制*/
    ret = m_read(&new,TIM_HOUR_REG,&data,1);
    if(E_OK != ret)
    {
        log_error("Read cfg failed.");
        goto error_handle;
    }
    if(0 != (data & HOUR_SYS_BIT))
    {
        data &= ~HOUR_SYS_BIT;
        ret = m_write(&new,TIM_HOUR_REG,&data,1);
        if(E_OK != ret)
        {
            log_error("Write failed.");
            goto error_handle;
        }  
    }
    
    /*获取ds3231状态*/
    ret = m_read(&new,OSF_REG,&data,1);
    if(E_OK != ret)
    {
        log_error("Read state failed.");
        goto error_handle;
    }

    /*检查ds3231是否曾经停止工作*/
    if(0 == (data & OSF_OSF_BIT))
    {
        return new;
    }
    
    /*清除停止标志*/
    data &= ~OSF_OSF_BIT;
    ret = m_write(&new,OSF_REG,&data,1);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        goto error_handle;
    } 

    /*写入默认时间*/
    ret = m_write(&new,TIM_SEC_REG,(const u8*)&g_default_time,sizeof(g_default_time));
    if(E_OK != ret)
    {
        log_error("Write failed.");
        goto error_handle;
    } 

    /*返回新建的对象*/
    return new;

    error_handle:

    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_set(const c_ds3231* this,time_type* time)
{
    int ret = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == time)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*hex转bcd*/
    hex_to_bcd(time);
    
    ret = m_write(this,TIM_SEC_REG,(const u8*)time,sizeof(time_type));
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    } 

    return E_OK;
}

static int m_get(const c_ds3231* this,time_type* time)
{
    int ret = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == time)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    /*把时间读出来*/
    ret = m_read(this,TIM_SEC_REG,(u8*)time,sizeof(time_type));
    if(E_OK != ret)
    {
        log_error("Read failed.");
        return E_ERROR;
    } 
    
    /*bcd转换*/
    bcd_to_hex(time);

    return E_OK;

}



static int m_read(const c_ds3231* this,u8 reg,u8* data,u8 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_ds3231* m_this = NULL;
    int ret = 0;
    u8 iic_addr_r = 0;
    u8 iic_addr_w = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(0 == len)
    {
        log_error("Param error.");
        return E_NULL;
    }
    m_this = this->this;

    /*创建新的命令链接*/
    cmd = iic_cmd_link_create();
    if(NULL == cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }

    /*放入启动命令*/
    ret = iic_master_start(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }

    /*写数据*/
    iic_addr_w = DS3231_ADDR | I2C_MASTER_WRITE;
    ret = iic_write_bytes(cmd,&iic_addr_w,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*寄存器地址*/
    ret = iic_write_bytes(cmd,&reg,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*放入启动命令*/
    ret = iic_master_start(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }

    iic_addr_r = DS3231_ADDR | I2C_MASTER_READ;
    ret = iic_write_bytes(cmd,&iic_addr_r,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*读数据*/
    ret = iic_read_bytes(cmd,data,len);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*停止命令*/
    ret = iic_master_stop(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }

    /*启动命令序列*/
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,DS3231_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        goto error_handle;
    }

    /*释放命令*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }

    return E_OK;

    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR; 
}
static int m_write(const c_ds3231* this,u8 reg,const u8* data,u8 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_ds3231* m_this = NULL;
    int ret = 0;
    u8 iic_addr = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(0 == len)
    {
        log_error("Param error.");
        return E_NULL;
    }
    m_this = this->this;

    /*创建新的命令链接*/
    cmd = iic_cmd_link_create();
    if(NULL == cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }

    /*放入启动命令*/
    ret = iic_master_start(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }

    /*写数据*/
    iic_addr = DS3231_ADDR | I2C_MASTER_WRITE;
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*寄存器地址*/
    ret = iic_write_bytes(cmd,&reg,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*写数据*/
    ret = iic_write_bytes(cmd,data,len);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*停止命令*/
    ret = iic_master_stop(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }

    /*启动命令序列*/
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,DS3231_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        goto error_handle;
    }

    /*释放命令*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }

    return E_OK;

    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR; 
}

static void  bcd_to_hex(time_type* time)
{
    time->o_sec    = ((time->o_sec   >> 4) * 10 + (time->o_sec   & 0x0F));
    time->o_min    = ((time->o_min   >> 4) * 10 + (time->o_min   & 0x0F));
    time->o_house  = ((time->o_house >> 4) * 10 + (time->o_house & 0x0F));
    time->o_week   = ((time->o_week  >> 4) * 10 + (time->o_week  & 0x0F));
    time->o_day    = ((time->o_day   >> 4) * 10 + (time->o_day   & 0x0F));
    time->o_month  = ((time->o_month >> 4) * 10 + (time->o_month & 0x0F));
    time->o_year   = ((time->o_year  >> 4) * 10 + (time->o_year  & 0x0F));
}

static void  hex_to_bcd(time_type* time)
{
    time->o_sec    = ((time->o_sec   / 10) * 16 + (time->o_sec   % 10));
    time->o_min    = ((time->o_min   / 10) * 16 + (time->o_min   % 10));
    time->o_house  = ((time->o_house / 10) * 16 + (time->o_house % 10));
    time->o_week   = ((time->o_week  / 10) * 16 + (time->o_week  % 10));
    time->o_day    = ((time->o_day   / 10) * 16 + (time->o_day   % 10));
    time->o_month  = ((time->o_month / 10) * 16 + (time->o_month % 10));
    time->o_year   = ((time->o_year  / 10) * 16 + (time->o_year  % 10));
}
#endif

