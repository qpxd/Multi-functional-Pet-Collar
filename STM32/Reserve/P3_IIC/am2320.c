/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : am2320.c
* Description: am2320函数实现
* Author: xzh
* Version: v1.0
* Date: 2022/02/02
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_AM2320_ENABLED

#ifdef MODE_LOG_TAG
#undef MODE_LOG_TAG
#endif
#define MODE_LOG_TAG "am2320"

#define AM2320_IIC_SPEED              35  /*iic通讯速度*/
#define AM2320_WEAK_UP_TIME_US       850  /*唤醒时间*/
#define AM2320_INTERVAL_TOO_LONG_MS 8000  /*间隔过长时间*/

#define CMD_READ_REG           0x03  /*读寄存器数据命令*/
#define CMD_WRITE_REG          0x10  /*写寄存器数据命令*/

#define REG_HUMI_H             0x00  /*湿度高位*/
#define REG_HUMI_L             0x01  /*湿度低位*/
#define REG_TEMP_H             0x02  /*温度高位*/
#define REG_TEMP_L             0x03  /*温度低位*/

#define REG_DEVICE_MODEL_H     0x08  /*型号高位*/
#define REG_DEVICE_MODEL_L     0x09  /*型号低位*/

#define REG_VERSION_NUMBER     0x0A  /*版本号*/

#define REG_ID_24_31_BIT       0x0B  /*设备ID*/
#define REG_ID_16_23_BIT       0x0C  /*设备ID*/
#define REG_ID__8_15_BIT       0x0D  /*设备ID*/
#define REG_ID__0__7_BIT       0x0E  /*设备ID*/

#define REG_STATE              0x0F  /*设备状态*/


const uint8_t g_read_cmd[] = {0x03,0x00,0x04};  //读温湿度命令（无CRC校验）


static int m_get(c_am2320* this,float* temperature,float* humidity);
static int m_read(const c_am2320* this,u8* data,u8 len);
static int m_write(const c_am2320* this,u8* data,u8 len);
static u16 m_crc(u8* data);

typedef struct _M_AM2320
{
    const c_my_iic*  m_iic   ;   /*所在的IIC总线*/
    uint32_t      m_old_tick ;  /*上一次采集的时间*/
}m_am2320;

c_am2320 am2320_creat(const c_my_iic* iic)
{
    c_am2320  new = {0};
    m_am2320* m_this = NULL;  
    
    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_am2320));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_am2320));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic  = iic  ;
    new.get        = m_get;

    /*返回新建的对象*/
    return new;
}


static int m_get(c_am2320* this,float* temperature,float* humidity)
{
    m_am2320* m_this = NULL;
    u8 buf[8] = {0};
    int ret = 0;
    u16 crc_recv = 0;
    u16 crc_cur  = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == humidity || NULL == temperature)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    angin:
    
    /*唤醒器件*/
    m_this->m_iic->clear(m_this->m_iic,AM2320_ADDR | I2C_MASTER_WRITE,AM2320_WEAK_UP_TIME_US,AM2320_IIC_SPEED);

    /*发送读命令*/
    buf[0] = CMD_READ_REG;
    buf[1] = REG_HUMI_H;
    buf[2] = 4;
    ret = m_write(this,buf,3);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        goto error_handle;
    }

    vTaskDelay(2);  /*最少等待1.5ms*/
    
    /*读数据*/
    ret = m_read(this,buf,8);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        goto error_handle;
    }

    /*如果接收间隔过长 会要求重复接收一次 手册要求*/
    if(HAL_GetTick() - m_this->m_old_tick > AM2320_INTERVAL_TOO_LONG_MS)
    {
        /*如长时间未读传感器，连续读取二次传感器，以第二次
          读回的温湿度为最新的值（连续读取最小间隔为2S）*/
        vTaskDelay(2000);  
        m_this->m_old_tick = HAL_GetTick();
        
        goto angin;
    }
    m_this->m_old_tick = HAL_GetTick();
    
    /*crc校验*/
    crc_recv = (buf[7] << 8 ) + buf[6];
    crc_cur  = m_crc(buf);
    if(crc_recv != crc_cur)
    {
        log_error("Crc error.");
        goto error_handle;
    }
    
    *humidity     = ((buf[2] << 8 ) + buf[3]) / 10.0f;
    *temperature  = ((buf[4] << 8 ) + buf[5]) / 10.0f;

    return E_OK;

    error_handle:

    *humidity = *temperature = 0.0f;
    return E_ERROR;
}
static int m_read(const c_am2320* this,u8* data,u8 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_am2320* m_this = NULL;
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
    iic_addr = AM2320_ADDR | I2C_MASTER_READ;
    ret = iic_write_bytes(cmd,&iic_addr,1);
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,AM2320_IIC_SPEED,1000);
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
static int m_write(const c_am2320* this,u8* data,u8 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_am2320* m_this = NULL;
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
    iic_addr = AM2320_ADDR | I2C_MASTER_WRITE;
    ret = iic_write_bytes(cmd,&iic_addr,1);
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,AM2320_IIC_SPEED,1000);
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

static u16 m_crc(u8* data)
{
    u16 crc=0xFFFF;
    u8 len = 6;
    u8 i = 0;
    
    while(len--)
    {
        crc ^= *data++;
        for(i=0;i<8;i++)
        {
            if(crc & 0x01)
            {
                crc>>=1;
                crc^=0xA001;
            }else
            {
                crc>>=1;
            }
            }
    }
    return crc;
}
#endif
