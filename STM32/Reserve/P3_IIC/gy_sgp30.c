/*****************************************************************************
* Copyright:
* File name: gy_sgp30.c
* Description: gy_sgp30气体传感器函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/01/30
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_SGP30_ENABLED

#define MODULE_NAME       "sgp30"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define SGP_ADDRESS                         (0x58 << 1) /*器件地址*/
#define SGP30_IIC_SPEED                     I2C_SPEED_DEFAULT

#define SGP30_Feature_Set                   0x0020     /*功能集*/
#define SGP30_Init_air_quality              0x2003     /*初始化空气质量 */
#define SGP30_Measure_air_quality           0x2008     /*测量空气质量 10~12ms*/
#define SGP30_Get_baseline                  0x2015     /*获取基线 */
#define SGP30_Set_baseline                  0x201e     /*设置基线*/
#define SGP30_Set_humidity                  0x2061     /*设置当前湿度 用于湿度补偿*/
#define SGP30_Measure_test                  0x2032     /*命令"Measure_test"包括用于集成和生产线测试的片上自检。如果自检成功，传感器将返回固定数据模式0xD400（具有正确的CRC）*/
#define SGP30_Get_feature_set_version       0x202f     /*获取功能集版本 */
#define SGP30_Measure_raw_signals           0x2050     /*用于部件验证和测试目的。 它返回传感器原始信号，这些信号用作片上校准和基线补偿算法的输入*/

static int m_get (const c_sgp30* this,float humidity,u16* co2,u16* voc);
static int m_start(const c_sgp30* this);
static int m_read    (const c_sgp30* this,u8* data,u8 len);
static int m_write   (const c_sgp30* this,u16 subaddr,u8* data,u8 len);
static u8 m_crc(u8* data);

typedef struct _M_GY_SGP30
{
    const c_my_iic*  m_iic            ;   /*所在的IIC总线*/
    u8               m_cur_humidity[3];   /*当前湿度*/
    bool             m_init_state     ;   /*初始化状态 */
}m_sgp30;

c_sgp30 sgp30_creat(const c_my_iic* iic)
{
    c_sgp30 new = {0};
    m_sgp30* m_this  = NULL;
    int ret = 0;
    
    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_sgp30));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_sgp30));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic   = iic   ;
    new.start       = m_start;
    new.get         = m_get ;
    
    /*返回新建的对象*/
    return new;
}

static int m_get(const c_sgp30* this,float humidity,u16* co2,u16* voc)
{
    m_sgp30* m_this = NULL;
    u8 hum_u8[2] = {0};
    u8 data[6] = {0};
    u16 m_co2 = 0,m_voc = 0;
    int ret = 0;

    /*参数检查*/
    if(NULL == this || NULL == this->this || NULL == co2 || NULL == voc)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(humidity > HUMIDITY_MAX)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*检查初始化状态*/
    if(false == m_this->m_init_state)
    {
        log_error("Need init first.");
        return E_ERROR;
    }

    /*是否需要湿度补偿*/
    if(humidity >= 0.0f)
    {
        /*计算补偿整数部分*/
        hum_u8[1] = humidity;

        /*计算补偿小数部分*/
        hum_u8[0] = (((float)humidity - (float)hum_u8[1]) * 256.0f * 10.0f + 0.5) / 10.0f;  /*四舍五入*/
    }

    /*不需要 写零就是关闭*/
    else
    {
        hum_u8[1] = hum_u8[0] = 0;
    }

    /*如果和上一次补偿的湿度不同 就进行新的湿度补偿*/
    if(hum_u8[1] != m_this->m_cur_humidity[1] || hum_u8[0] != m_this->m_cur_humidity[0])
    {
        memcpy(m_this->m_cur_humidity,hum_u8,2);                     /*保存新的湿度*/
        m_this->m_cur_humidity[2] = m_crc(m_this->m_cur_humidity);  /*计算crc*/

        /*设置湿度补偿*/
        ret = m_write(this,SGP30_Set_humidity,m_this->m_cur_humidity,3);
        if(E_OK != ret)
        {
            log_error("Humidity compensate set failed.");
            goto error_handle;
        }
    }

    /*启动一次转换*/
    ret = m_write(this,SGP30_Measure_air_quality,NULL,0);
    if(E_OK != ret)
    {
        log_error("write failed.");
        goto error_handle;
    }

    /*转换所需要的时间为 10~12ms*/
    vTaskDelay(15);

    /*获取6位数据*/
    ret = m_read(this,data,6);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        goto error_handle;
    }

    m_co2  = data[0] << 8;
    m_co2 += data[1];
    m_voc  = data[3] << 8;
    m_voc += data[4];

    /*co2 crc校验*/
    if(m_crc(&data[0]) != data[2])
    {
        *co2 = 400;
        log_warning("co2 crc error.");
    }
    else
    {
        *co2 = m_co2;
    }
    
    /*voc crc校验*/
    if(m_crc(&data[3]) != data[5])
    {
        *voc = 0;
        log_warning("voc crc error.");
    }
    else
    {
        *voc = m_voc;
    }   

    return E_OK;

    error_handle:

    *co2 = 400;
    *voc = 0;
    return E_ERROR;
}

static int m_read(const c_sgp30* this,u8* data,u8 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_sgp30* m_this = NULL;
    int ret = 0;
    u8 iic_addr_r = 0;

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

    iic_addr_r = SGP_ADDRESS | I2C_MASTER_READ;
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,SGP30_IIC_SPEED,1000);
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

static int m_write(const c_sgp30* this,u16 subaddr,u8* data,u8 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_sgp30* m_this = NULL;
    int ret = 0;
    u8 iic_addr = 0;
    u8 data_subaddr[2] = {0};

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    m_this = this->this;
    data_subaddr[0] = ((u8*)&subaddr)[1];   /*MSB 高位先出*/
    data_subaddr[1] = ((u8*)&subaddr)[0];

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
    iic_addr = SGP_ADDRESS | I2C_MASTER_WRITE;
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /*目标寄存器地址*/
    ret = iic_write_bytes(cmd,data_subaddr,2);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*写数据*/
    if(NULL != data && 0 != len)
    {
        ret = iic_write_bytes(cmd,data,len);          
        if(E_OK != ret)                               
        {                                             
            log_error("Add iic write byte failed.");  
            goto error_handle;
        }
    }

    /*停止命令*/
    ret = iic_master_stop(cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }

    /*启动命令序列*/
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,SGP30_IIC_SPEED,1000);
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

static u8 m_crc(u8* data)
{
    u8 crc = 0xff;
    u8 i = 0;
    u8 bit = 0;
    
    for(i = 0;i < 2;++i)
    {
        crc ^= data[i];
        
        for(bit = 8;bit > 0;--bit)
        {
            if(crc & 0x80)
            {
                crc = (crc << 1) ^ 0x31;
            }
            else
            {
                crc = (crc << 1);
            }
        }
    }
    
    return crc;
}

static int m_start(const c_sgp30* this)
{
    m_sgp30* m_this = NULL;
    int ret = 0;
    u16 co2 = 0,voc = 0;
    u8 count = 0;
    u8 data[3] = {0};

    /*参数检查*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*如果已经初始化成功 直接返回*/
    if(true == m_this->m_init_state)
    {
        return E_OK;
    }

    /*初始化空气质量*/
    ret = m_write(this,SGP30_Init_air_quality,NULL,0);
    if(E_OK != ret)
    {
        m_this->m_init_state = false;
        log_error("Init air quality failed.");
        return E_ERROR;
    }
    
    /*默认关闭湿度补偿*/
    data[2] = m_crc(data);
    ret = m_write(this,SGP30_Set_humidity,data,3);
    if(E_OK != ret)
    {
        m_this->m_init_state = false;
        log_error("Humidity compensate close failed.");
        return E_ERROR;
    }

    /*获取当前co2 和voc*/
    /*初次上电 sgp30 ack反应会很慢，且会输出一段时间的错误（CRC错误）数值*/
    /*稳定后，会输出至少15s的co2 = 400 voc = 0*/
    m_this->m_init_state = true;
    do
    {
        /*尝试获取*/
        ret = m_get(this,HUMIDITY_UN,&co2,&voc);
        vTaskDelay(1000);
        ++count;
        log_inform("SGP30 is initializing,time:%ds.",count);
    }while((E_OK != ret && count < SGP30_START_MAX_TIME_S) || (E_OK == ret && 400 == co2 && 0 == voc));
    
    /*如果超时 初始化失败*/
    if(count >= SGP30_START_MAX_TIME_S)
    {
        m_this->m_init_state = false;
        log_error("Initialization timeout.");
        return E_ERROR;
    }
    
    m_this->m_init_state = true;  /*标记为初始化成功*/

    return E_OK;
}
#endif

