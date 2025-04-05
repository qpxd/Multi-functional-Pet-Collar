/*****************************************************************************
* Copyright:
* File name: bh1750.c
* Description: 光照模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/01/11
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_BH1750_ENABLED

#define MODULE_NAME       "bh1750"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

#define BH1750_IIC_SPEED             I2C_SPEED_DEFAULT

/**********************************************************************************************/
//用于测量光强的传感器
//测量范围  1勒克斯~65535勒克斯
#define BH1750_CMD_PowerOn           0x01                //无激活状态
#define BH1750_CMD_PowerDown         0x00                //等待测量指令
#define BH1750_CMD_Reset             0x07                //重置数字寄存器 在断电模式下无效
#define BH1750_CMD_Measure_H         0x10                //连续以1  lux分辨率测量 测量时间120ms
#define BH1750_CMD_Measure_H2        0x11                //连续以0.5lux分辨率测量 测量时间120ms
#define BH1750_CMD_Measure_L         0x13                //连续以41 lux分辨率测量 测量时间16 ms
#define BH1750_CMD_Measure_H_Once    0x20                //以1  lux分辨率测量一次 测量时间120ms
#define BH1750_CMD_Measure_H2_Once   0x21                //以0.5lux分辨率测量一次 测量时间120ms
#define BH1750_CMD_Measure_L_Once    0x23                //以41 lux分辨率测量一次 测量时间16 ms
#define BH1750_CMD_Measure_Time_H(x) (x & 0xE0 | 0x08)   //改变测量时间（灵敏度）高位
#define BH1750_CMD_Measure_Time_L(x) (x & 0x1F | 0x60)   //改变测量时间（灵敏度）低位

#define BH1750_ADDR(x)               (0x46 | x)          //设备地址
#define BH1750_MODE_Write            0x00                //写模式
#define BH1750_MODE_Read             0x01                //读模式
/*=========================================================================*/

int m_start(const c_bh1750* this,u8 mode);
int m_get(const c_bh1750* this,u16* lux);

typedef struct _M_BH1750
{
    const c_my_iic*  m_iic ;   /*所在的IIC总线*/
    u8               m_addr;   /*器件地址*/
}m_bh1750;


c_bh1750 bh1750_creat(const c_my_iic* iic,u8 addr)
{
    c_bh1750 new = {0};
    m_bh1750* m_this = NULL;

    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }
    if(BH1750_ADDR_H != addr && BH1750_ADDR_L != addr)
    {
        log_error("Param error");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_bh1750));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_bh1750));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic   = iic      ;
    m_this->m_addr  = addr     ;
    new.start      = m_start  ;
    new.get        = m_get    ;

    /*返回新建的对象*/
    return new;
}

int m_start(const c_bh1750* this,u8 mode)
{
    iic_cmd_handle_t  cmd = NULL;
    u8   iic_data[2] = {0};
    int ret  = 0;
    const m_bh1750* m_this = NULL;

    /*参数检查*/
    if(NULL == this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    if(BH1750_MODE_HH != mode && BH1750_MODE_H != mode && BH1750_MODE_L != mode)
    {
        log_error("Param error");
        return E_PARAM;
    }
    m_this = this->this;
    if(BH1750_MODE_HH == mode)
    {
        mode = BH1750_CMD_Measure_H2;
    }
    else if(BH1750_MODE_H == mode)
    {
        mode = BH1750_CMD_Measure_H;
    }
    else if(BH1750_MODE_L == mode)
    {
        mode = BH1750_CMD_Measure_L;
    }
    else
    {
        log_error("Param error.");
        return E_PARAM;
    }

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

    /*写入数据*/
    iic_data[0] = m_this->m_addr | I2C_MASTER_WRITE;  /*写地址*/
    iic_data[1] = mode;                               /*写数据*/
    ret = iic_write_bytes(cmd,iic_data,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    ret = iic_write_bytes(cmd,iic_data + 1,1);
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,BH1750_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("Add iic begin failed.");
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
int m_get(const c_bh1750* this,u16* lux)
{
    iic_cmd_handle_t  cmd = NULL;
    u8   iic_addr = 0;
    u8   iic_read_data[2] = {0};
    int ret  = 0;
    const m_bh1750* m_this = NULL;

    /*参数检查*/
    if(NULL == this || NULL == lux)
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

    /*写入地址*/
    iic_addr = m_this->m_addr | I2C_MASTER_READ;  /*读地址*/
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*读取数据*/
    ret = iic_read_bytes(cmd,iic_read_data,2);
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,BH1750_IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("Add iic begin failed.");
        goto error_handle;
    }

    /*释放命令*/
    ret = iic_cmd_link_delete(cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }

    /*换算光照强度*/
    *lux  = 0;
    *lux |= iic_read_data[0] << 8; 
    *lux |= iic_read_data[1];

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
