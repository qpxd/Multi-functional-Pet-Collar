/*****************************************************************************
* Copyright:
* File name: tcs34725.c
* Description: tcs34725颜色传感器函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/01/28
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_TCS34725_ENABLED

#define MODULE_NAME       "tcs34725"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define TCS34725_ADDRESS          (0x29 << 1)
#define TCS34725_IIC_SPEED        I2C_SPEED_DEFAULT

#define TCS34725_COMMAND_BIT      (0x80)

#define TCS34725_ENABLE           (0x00)
#define TCS34725_ENABLE_AIEN      (0x10)    /* RGBC Interrupt Enable */
#define TCS34725_ENABLE_WEN       (0x08)    /* Wait enable - Writing 1 activates the wait timer */
#define TCS34725_ENABLE_AEN       (0x02)    /* RGBC Enable - Writing 1 actives the ADC, 0 disables it */
#define TCS34725_ENABLE_PON       (0x01)    /* Power on - Writing 1 activates the internal oscillator, 0 disables it */
#define TCS34725_ATIME            (0x01)    /* Integration time */
#define TCS34725_WTIME            (0x03)    /* Wait time (if TCS34725_ENABLE_WEN is asserted) */
#define TCS34725_WTIME_2_4MS      (0xFF)    /* WLONG0 = 2.4ms   WLONG1 = 0.029s */
#define TCS34725_WTIME_204MS      (0xAB)    /* WLONG0 = 204ms   WLONG1 = 2.45s  */
#define TCS34725_WTIME_614MS      (0x00)    /* WLONG0 = 614ms   WLONG1 = 7.4s   */
#define TCS34725_AILTL            (0x04)    /* Clear channel lower interrupt threshold */
#define TCS34725_AILTH            (0x05)
#define TCS34725_AIHTL            (0x06)    /* Clear channel upper interrupt threshold */
#define TCS34725_AIHTH            (0x07)
#define TCS34725_PERS             (0x0C)    /* Persistence register - basic SW filtering mechanism for interrupts */
#define TCS34725_PERS_NONE        (0b0000)  /* Every RGBC cycle generates an interrupt                                */
#define TCS34725_PERS_1_CYCLE     (0b0001)  /* 1 clean channel value outside threshold range generates an interrupt   */
#define TCS34725_PERS_2_CYCLE     (0b0010)  /* 2 clean channel values outside threshold range generates an interrupt  */
#define TCS34725_PERS_3_CYCLE     (0b0011)  /* 3 clean channel values outside threshold range generates an interrupt  */
#define TCS34725_PERS_5_CYCLE     (0b0100)  /* 5 clean channel values outside threshold range generates an interrupt  */
#define TCS34725_PERS_10_CYCLE    (0b0101)  /* 10 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_15_CYCLE    (0b0110)  /* 15 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_20_CYCLE    (0b0111)  /* 20 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_25_CYCLE    (0b1000)  /* 25 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_30_CYCLE    (0b1001)  /* 30 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_35_CYCLE    (0b1010)  /* 35 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_40_CYCLE    (0b1011)  /* 40 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_45_CYCLE    (0b1100)  /* 45 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_50_CYCLE    (0b1101)  /* 50 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_55_CYCLE    (0b1110)  /* 55 clean channel values outside threshold range generates an interrupt */
#define TCS34725_PERS_60_CYCLE    (0b1111)  /* 60 clean channel values outside threshold range generates an interrupt */
#define TCS34725_CONFIG           (0x0D)
#define TCS34725_CONFIG_WLONG     (0x02)    /* Choose between short and long (12x) wait times via TCS34725_WTIME */
#define TCS34725_CONTROL          (0x0F)    /* Set the gain level for the sensor */
#define TCS34725_ID               (0x12)    /* 0x44 = TCS34721/TCS34725, 0x4D = TCS34723/TCS34727 */
#define TCS34725_STATUS           (0x13)
#define TCS34725_STATUS_AINT      (0x10)    /* RGBC Clean channel interrupt */
#define TCS34725_STATUS_AVALID    (0x01)    /* Indicates that the RGBC channels have completed an integration cycle */
#define TCS34725_CDATAL           (0x14)    /* Clear channel data */
#define TCS34725_CDATAH           (0x15)
#define TCS34725_RDATAL           (0x16)    /* Red channel data */
#define TCS34725_RDATAH           (0x17)
#define TCS34725_GDATAL           (0x18)    /* Green channel data */
#define TCS34725_GDATAH           (0x19)
#define TCS34725_BDATAL           (0x1A)    /* Blue channel data */
#define TCS34725_BDATAH           (0x1B)

typedef struct _M_TCS34725
{
    const c_my_iic*  m_iic ;   /*所在的IIC总线*/
    u16              m_r   ;
    u16              m_g   ;
    u16              m_b   ;
    u16              m_c   ;
}m_tcs34725;

static int m_get_rgbc(const c_tcs34725* this,u16* r,u16* g,u16* b,u16* c);
static int m_get_hsl (const c_tcs34725* this,u16* h,u8* s,u8* l);
static int m_start   (const c_tcs34725* this,u8 integral_time,u8 gain);
static int m_read    (const c_tcs34725* this,u8 subaddr,u8* data,u8 len);
static int m_write   (const c_tcs34725* this,u8 subaddr,u8* data,u8 len);


c_tcs34725 tcs34725_creat(const c_my_iic* iic)
{
    c_tcs34725 new = {0};
    m_tcs34725* m_this  = NULL;
    int ret = 0;

    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }
    
    /*为新对象申请内存*/
	new.this = pvPortMalloc(sizeof(m_tcs34725));
	if(NULL == new.this)
	{
		log_error("Out of memory");
		return new;
	}
    memset(new.this,0,sizeof(m_tcs34725));
	m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic   = iic        ;
    new.get_rgbc    = m_get_rgbc ;
    new.get_hsl     = m_get_hsl  ;
    new.start       = m_start;

    /*返回新建的对象*/
    return new;
}


static int m_get_rgbc(const c_tcs34725* this,u16* r,u16* g,u16* b,u16* c)
{
    m_tcs34725* m_this = NULL;
    int ret = 0;
    u8 data = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == r || NULL == g || NULL == b || NULL == c)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    /*检查积分状态*/
    ret = m_read(this,TCS34725_STATUS,&data,1);
    if(E_OK != ret)
    {
        log_error("State read failed.");
        goto error_handle;
    }

    /*如果还没转换完成 就把上一次的值输出出去*/
    if(!(data & TCS34725_STATUS_AVALID))
    {
        *c = m_this->m_c;
        *r = m_this->m_r;
        *g = m_this->m_g;
        *b = m_this->m_b;
        return E_OK;
    }
    
    /*获取白色*/
    ret = m_read(this,TCS34725_CDATAL,(u8*)&m_this->m_c,2);
    if(E_OK != ret)
    {
        log_error("Integral set failed.");
        goto error_handle;
    }
    
    /*获取红色*/
    ret = m_read(this,TCS34725_RDATAL,(u8*)&m_this->m_r,2);
    if(E_OK != ret)
    {
        log_error("Integral set failed.");
        goto error_handle;
    }
    
    /*获取绿色*/
    ret = m_read(this,TCS34725_GDATAL,(u8*)&m_this->m_g,2);
    if(E_OK != ret)
    {
        log_error("Integral set failed.");
        goto error_handle;
    }
    
    /*获取蓝色*/
    ret = m_read(this,TCS34725_BDATAL,(u8*)&m_this->m_b,2);
    if(E_OK != ret)
    {
        log_error("Integral set failed.");
        goto error_handle;
    }

    *c = m_this->m_c;
    *r = m_this->m_r;
    *g = m_this->m_g;
    *b = m_this->m_b;
    
    return E_OK;
    
    error_handle:
    
    *r = *g = *b = *c = 0;
    return E_ERROR;
}

static int m_start(const c_tcs34725* this,u8 integral_time,u8 gain)
{
    int ret = 0;
    u8 data = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(TCS34725_INTEGRATIONTIME_2_4MS != integral_time &&
       TCS34725_INTEGRATIONTIME_24MS  != integral_time &&
       TCS34725_INTEGRATIONTIME_50MS  != integral_time &&
       TCS34725_INTEGRATIONTIME_101MS != integral_time &&
       TCS34725_INTEGRATIONTIME_154MS != integral_time &&
       TCS34725_INTEGRATIONTIME_240MS != integral_time &&
       TCS34725_INTEGRATIONTIME_700MS != integral_time 
      )
    {
        log_error("Param error");
        return E_PARAM;
    }
    if(TCS34725_GAIN_1X  != gain && TCS34725_GAIN_4X  != gain &&
       TCS34725_GAIN_16X != gain && TCS34725_GAIN_60X != gain)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*读ID*/
    ret = m_read(this,TCS34725_ID,&data,1);
    if(E_OK != ret)
    {
        log_error("IIC read failed.");
        return E_ERROR;
    }
    
    /*设置积分时间*/
    ret = m_write(this,TCS34725_ATIME,&integral_time,1);
    if(E_OK != ret)
    {
        log_error("Integral set failed.");
        return E_ERROR;
    }
    
    /*设置增益*/
    ret = m_write(this,TCS34725_CONTROL,&gain,1);
    if(E_OK != ret)
    {
        log_error("Gain set failed.");
        return E_ERROR;
    }
    
    /*使能TCS34725*/    
    data = TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN;
    ret = m_write(this,TCS34725_ENABLE,&data,1);
    if(E_OK != ret)
    {
        log_error("Enable set failed.");
        return E_ERROR;
    }

    return E_OK;
}

static int m_read(const c_tcs34725* this,u8 subaddr,u8* data,u8 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_tcs34725* m_this = NULL;
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
    iic_addr_w = TCS34725_ADDRESS | I2C_MASTER_WRITE;
    ret = iic_write_bytes(cmd,&iic_addr_w,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    subaddr |= TCS34725_COMMAND_BIT;
    ret = iic_write_bytes(cmd,&subaddr,1);
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

    iic_addr_r = TCS34725_ADDRESS | I2C_MASTER_READ;
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,TCS34725_IIC_SPEED,1000);
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

static int m_write(const c_tcs34725* this,u8 subaddr,u8* data,u8 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_tcs34725* m_this = NULL;
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
    iic_addr = TCS34725_ADDRESS | I2C_MASTER_WRITE;
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    subaddr |= TCS34725_COMMAND_BIT;
    ret = iic_write_bytes(cmd,&subaddr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*写数据*/
    ret = iic_write_bytes(cmd,data,len);           /*20220129 bug 写出了iic_read_bytes  导致两个驱动同一时刻读出来的数据不一致，正常的驱动是50多，这个读出来一百多*/
    if(E_OK != ret)                                /*而且芯片断电后 数据读不出来了，有应答信号 只是一直显示为转换完成，烧正确驱动后 本代码恢复正常 初步断定 读没问题，应该是出在写上*/
    {                                              /*初始化后 只有start用了写， 所以怀疑是使能问题。 后找到写中没有发写入地址 subaddr 添加后 使能正常，不需要烧一遍老驱动也能读出数据 只是数据不对 但能确定使能生效了*/
        log_error("Add iic write byte failed.");   /*芯片能够正常返回数据 只是数值有差异，断定为配置有问题，所以 问题原因还是圈定在写上 最后定位在写数据位置 write写成read 导致配置没有写入进去*/
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,TCS34725_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("Add iic begin failed.");
        goto error_handle;
    }

    /*释放命令*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
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

static int m_get_hsl(const c_tcs34725* this,u16* h,u8* s,u8* l)
{
    u16 r,g,b,c;
    int ret = 0;
    float m_h = 0;
    float maxVal,minVal,difVal;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == h || NULL == s || NULL == l)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*获取rgbc*/
    ret = m_get_rgbc(this,&r,&g,&b,&c);
    if(E_OK != ret)
    {
        log_error("RGBC get failed.");
        *h = *s = *l = 0;
        return E_ERROR;
    }

    /*转换到 0~100*/
    r = r * 100 / c;
    g = g * 100 / c;
    b = b * 100 / c;

    maxVal = (r)<(g)? ((g)<(b)?(b):(g)):((r)<(b)?(b):(r));
    minVal = (r)<(g)? ((g)>(b)?(b):(g)):((r)>(b)?(b):(r));
    difVal = maxVal - minVal;

    /*计算亮度*/
    *l = (maxVal+minVal) / 2.0f;   //[0-100]

    if(maxVal == minVal)  //若r=g=b,灰度
	{
		*h = 0; 
	    *s = 0;
	}
	else
	{
		//计算色调
        if(r == maxVal)
        {
            m_h = (float)(g - b) / (float)(maxVal - minVal);
        }
        else if(g == maxVal)
        {
            m_h = 2.0f + (float)(b - r) / (float)(maxVal - minVal);
        }
        else
        {
            m_h = 4.0f + (float)(r - g) / (float)(maxVal - minVal);
        }
        
        m_h *= 60.0f;
        if(m_h < 0.0f)
        {
            m_h += 360.0f;
        }
        else if(m_h > 360.0f)
        {
            m_h -= 360.0f;
        }
        *h = m_h;
		
		//计算饱和度
		if(*l<=50)
        {
            *s = difVal * 100 / (maxVal + minVal);  //[0-100]
        }
		else
        {
            *s = difVal * 100 / (200 - (maxVal + minVal));
        }
    }
        
    return E_OK;
}

#endif

