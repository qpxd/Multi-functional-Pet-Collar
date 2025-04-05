#include "driver_conf.h"

#ifdef DRIVER_INA219_ENABLED

#ifdef MODE_LOG_TAG
#undef MODE_LOG_TAG
#endif
#define MODE_LOG_TAG "ina219"

#define INA219_IIC_SPEED   I2C_SPEED_DEFAULT

typedef struct _M_INA219
{
    const c_my_iic*  m_iic   ;   /*所在的IIC总线*/
}m_ina219;

static void INA_REG_Write(unsigned char reg,unsigned int data);

static int m_write(const c_ina219* this,u8 reg,u16 data);
static int m_read(const c_ina219* this,u8 reg,u8* data);
static int m_get(const c_ina219* this,u32* mv,u32* ma,u32* mw);


c_ina219 ina219_creat(const c_my_iic* iic)
{
    c_ina219  new = {0};
    m_ina219* m_this = NULL;  
    int ret = 0;
    u8  data = 0;
    
    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_ina219));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_ina219));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic  = iic  ;
    new.get        = m_get;

    ret = m_write(&new,INA219_REG_CONFIG,INA219_CONFIG_value);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        goto error_handle;
    }
	ret = m_write(&new,INA219_REG_CALIBRATION,INA_CAL);
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

static int m_get(const c_ina219* this,u32* mv,u32* ma,u32* mw)
{
    const m_ina219* m_this = NULL;
    int ret = 0;
    u8 data_temp[2] = {0};
    
    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == mv || NULL == ma || NULL == mw)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*获取电压*/	
	ret = m_read(this,INA219_REG_BUSVOLTAGE,data_temp);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        return E_ERROR;
    }

    //右移3为去掉：bit2，CNVR，OVF三位，再乘以 4MV (官方文档规定)，得到当前总线的电压值
    *mv = (int)((((data_temp[0]<<8)+data_temp[1]) >> 3)*4);

    /*获取电流*/
    ret = m_write(this,INA219_REG_CONFIG,INA219_CONFIG_value);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }
    ret = m_read(this,INA219_REG_CURRENT,data_temp);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        return E_ERROR;
    }

    //得到寄存器的值在乘以每位对应的值（IAN_I_LSB）得到实际的电流
	*ma = (int)((((data_temp[0]<<8)+data_temp[1]))*IAN_I_LSB);	

    /*获取功率*/
    ret = m_read(this,INA219_REG_POWER,data_temp);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        return E_ERROR;
    }

    //得到寄存器的值在乘以每位对应的值（INA_Power_LSB）得到实际的功率
    *mw = (int)(((data_temp[0]<<8)+data_temp[1])*INA_Power_LSB);  

    return E_OK;
}


static int m_write(const c_ina219* this,u8 reg,u16 data)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_ina219* m_this = NULL;
    int ret = 0;
    u8 iic_addr = 0;
    unsigned char data_temp[2] = {0};
    
	data_temp[0]=(unsigned char )(data>>8);
	data_temp[1]=(unsigned char )(data & 0xFF);

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
    iic_addr = INA219_ADDRESS | I2C_MASTER_WRITE;
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
    ret = iic_write_bytes(cmd,data_temp,2);
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,INA219_IIC_SPEED,1000);
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

static int m_read(const c_ina219* this,u8 reg,u8* data)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_ina219* m_this = NULL;
    int ret = 0;
    u8 iic_addr_r = 0;
    u8 iic_addr_w = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == data)
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
    iic_addr_w = INA219_ADDRESS | I2C_MASTER_WRITE;
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

    iic_addr_r = INA219_ADDRESS | I2C_MASTER_READ;
    ret = iic_write_bytes(cmd,&iic_addr_r,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*读数据*/
    ret = iic_read_bytes(cmd,data,2);
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,INA219_IIC_SPEED,1000);
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


//void INA_Read_Byte_s(unsigned char reg,unsigned char *data)	//读两位数据
//{
//	INA_IIC_Start();
//	INA_IIC_Send_Byte(INA219_ADDRESS);	//发送INA219地址s
//	INA_IIC_Send_Byte(reg);
//	
//	INA_IIC_Start();
//	INA_IIC_Send_Byte(INA219_ADDRESS+0x01);	//设置iic为读模式
//	*data=INA_IIC_Read_Byte(0);
//	data++;
//	//INA_IIC_ACK_Send(1);
//	*data=INA_IIC_Read_Byte(1);
//	//INA_IIC_ACK_Send(0);
//	INA_IIC_Stop();
//}
//void INA_REG_Write(unsigned char reg,unsigned int data)	//写寄存器		测试成功
//{
//	unsigned char data_temp[2];
//	data_temp[0]=(unsigned char )(data>>8);
//	data_temp[1]=(unsigned char )(data & 0xFF);
//	INA_IIC_Start();
//	INA_IIC_Send_Byte(INA219_ADDRESS);	//发送INA219地址
//	INA_IIC_Send_Byte(reg);							//发送寄存器地址
//	INA_IIC_Send_Byte(data_temp[0]);			    //发送高8位数据
//	data++;
//	INA_IIC_Send_Byte(data_temp[1])	;				//发送低8位数据
//	INA_IIC_Stop();
//}

//void INA_Init(void )	
//{
//
//	INA_REG_Write(INA219_REG_CONFIG,INA219_CONFIG_value);
//	INA_REG_Write(INA219_REG_CALIBRATION,INA_CAL);
//}
//unsigned int INA_GET_Voltage_MV(void)	//获取电压（单位：mv）
//{
//	unsigned char data_temp[2];
//	INA_Read_Byte_s(0x02,data_temp);
//	return (int)((((data_temp[0]<<8)+data_temp[1]) >> 3)*4);	//右移3为去掉：bit2，CNVR，OVF三位，再乘以 4MV (官方文档规定)，得到当前总线的电压值
//}
//unsigned int INA_GET_Current_MA(void)		//获取电流（单位：mA）
//{
//	unsigned char data_temp[2];
//	INA_REG_Write(INA219_REG_CONFIG,INA219_CONFIG_value);
//	INA_Read_Byte_s(INA219_REG_CURRENT,data_temp);
//	return (int)((((data_temp[0]<<8)+data_temp[1]))*IAN_I_LSB);		//得到寄存器的值在乘以每位对应的值（IAN_I_LSB）得到实际的电流
//}
//unsigned int INA_GET_Power_MW(void)		//获取当前功率（单位：mw）
//{
//	unsigned char data_temp[2];
//	INA_Read_Byte_s(INA219_REG_POWER,data_temp);
//	return (int)(((data_temp[0]<<8)+data_temp[1])*INA_Power_LSB);	//得到寄存器的值在乘以每位对应的值（INA_Power_LSB）得到实际的功率
//}
#endif