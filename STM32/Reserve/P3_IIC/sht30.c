/*****************************************************************************
* Copyright:
* File name: sht30.c
* Description: sht30模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/03/08
* log:  V1.0  2022/03/08
*       发布：
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_SHT30_ENABLED

#ifdef MODE_LOG_TAG
#undef MODE_LOG_TAG
#endif
#define MODE_LOG_TAG "sht30"

#define IIC_SPEED  0

#define SHT30_ADDR  (0x44 << 1)


static int m_get(const c_sht30* this,float* temp,float* humi);
static int m_write(const c_sht30* this,u8 msb,u8 lsb);
static int m_read(const c_sht30* this,u8* buf);

typedef struct _M_SHT30
{
    const c_my_iic*  m_iic    ;   /*所在的IIC总线*/
}m_sht30;

c_sht30 sht30_create(const c_my_iic* iic)
{
    c_sht30 new = {0};
    m_sht30* m_this  = NULL;

    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }    
    
    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_sht30));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_sht30));
    m_this = new.this;
    
    /*保存相关变量*/
    m_this->m_iic   = iic   ;
    new.get         = m_get ;
    
    return new;
}

static int m_get(const c_sht30* this,float* temp,float* humi)
{
    int ret  = 0;
    m_sht30* m_this = NULL;
    u8 buf[6] = {0};
    
    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == temp || NULL == humi)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;
    
    *temp = 0.0f;
    *humi = 0.0f;
    
    /*启动一次采集*/
    ret = m_write(this,0x21,0x26);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    
    vTaskDelay(5);
    
    /*读取数据*/
    ret = m_read(this,buf);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        return E_ERROR;
    }
    
    /*换算数据*/
    *temp = (float)175*((buf[0]<<8)+buf[1])/65535-45;    //左移一位相当于乘2，左移8位*2^8。
    *humi = (float)100*((buf[3]<<8)+buf[4])/65535;
    
    return E_OK;
}


static int m_write(const c_sht30* this,u8 msb,u8 lsb)
{
    iic_cmd_handle_t  cmd = NULL;
    m_sht30* m_this = NULL;
    int ret  = 0;
    u8 iic_addr = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
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
    iic_addr = SHT30_ADDR | I2C_MASTER_WRITE;
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    ret = iic_write_bytes(cmd,&msb,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    ret = iic_write_bytes(cmd,&lsb,1);
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,IIC_SPEED,1000);
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
static int m_read(const c_sht30* this,u8* buf)
{
    iic_cmd_handle_t  cmd = NULL;
    m_sht30* m_this = NULL;
    int ret  = 0;
    u8 iic_addr = 0;
    
    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == buf)
    {
        log_error("Null pointer.");
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
    iic_addr = SHT30_ADDR | I2C_MASTER_READ;
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /*读六个字节*/
    ret = iic_read_bytes(cmd,buf,6);
    if(E_OK != ret)
    {
        log_error("Add iic read byte failed.");
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,IIC_SPEED,1000);
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
#endif