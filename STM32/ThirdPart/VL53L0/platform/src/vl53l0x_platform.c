/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : vl53l0x_platform.c
* Description: vl53l0x tof测距模块平台连接实现
* Author: xzh
* Version: v1.0
* Date: 2022/02/13
*****************************************************************************/

#include "vl53l0x_platform.h"


#define MODULE_NAME       "vl53l0x platform"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME

static int VL53L0X_write_byte (const VL53L0X_DEV dev ,u8 reg_addr,u8 data);              //IIC写一个8位数据
static int VL53L0X_write_word (const VL53L0X_DEV dev ,u8 reg_addr,u16 data);             //IIC写一个16位数据
static int VL53L0X_write_dword(const VL53L0X_DEV dev ,u8 reg_addr,u32 data);             //IIC写一个32位数据
static int VL53L0X_write_multi(const VL53L0X_DEV dev ,u8 reg_addr,u8 *pdata,u16 count);  //IIC连续写
                                                 
static int VL53L0X_read_byte  (const VL53L0X_DEV dev , u8 reg_addr,u8 *pdata);            //IIC读一个8位数据
static int VL53L0X_read_word  (const VL53L0X_DEV dev , u8 reg_addr,u16 *pdata);           //IIC读一个16位数据
static int VL53L0X_read_dword (const VL53L0X_DEV dev , u8 reg_addr,u32 *pdata);           //IIC读一个32位数据
static int VL53L0X_read_multi (const VL53L0X_DEV dev , u8 reg_addr,u8 *pdata,u16 count);  //IIC连续读

static int m_write(const VL53L0X_DEV dev, u8 reg_addr,u8 *data,u16 len);
static int m_read (const VL53L0X_DEV dev, u8 reg_addr,u8 *data,u16 len);

//VL53L0X连续写数据
//Dev:设备I2C参数结构体
//index:偏移地址
//pdata:数据指针
//count:长度
//返回值: 0:成功  
//       其他:错误
VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata,uint32_t count)
{
	
	 VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	
	 int32_t status_int = 0;
	
	 uint8_t deviceAddress;
	
	 if(count >=VL53L0X_MAX_I2C_XFER_SIZE)
	 {
		 Status = VL53L0X_ERROR_INVALID_PARAMS;
	 }
	
	 deviceAddress = Dev->I2cDevAddr;
	
	 status_int = VL53L0X_write_multi(Dev, index, pdata, count);
	
	 if(status_int !=0)
	   Status = VL53L0X_ERROR_CONTROL_INTERFACE;
	
	 return Status;
	
}

//VL53L0X连续读数据
//Dev:设备I2C参数结构体
//index:偏移地址
//pdata:数据指针
//count:长度
//返回值: 0:成功  
//       其他:错误
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata,uint32_t count)
{
	
	 VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	
	 int32_t status_int;
	
	 uint8_t deviceAddress;
	
	 if(count >=VL53L0X_MAX_I2C_XFER_SIZE)
	 {
		 Status = VL53L0X_ERROR_INVALID_PARAMS;
	 } 
	 
	 deviceAddress = Dev->I2cDevAddr;
	 
	 status_int = VL53L0X_read_multi(Dev , index, pdata, count);
	 
	 if(status_int!=0)
	   Status = VL53L0X_ERROR_CONTROL_INTERFACE;
	 
	 return Status;
	
}

//VL53L0X 写单字节寄存器
//Dev:设备I2C参数结构体
//index:偏移地址
//pdata:数据指针
//count:长度
//返回值: 0:成功  
//       其他:失败
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
	
	 VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	 int32_t status_int;
	 uint8_t deviceAddress;
	
	 deviceAddress = Dev->I2cDevAddr;
	
	 status_int = VL53L0X_write_byte(Dev,index,data);
	
	 if(status_int!=0)
		Status = VL53L0X_ERROR_CONTROL_INTERFACE;
	
	 return Status;
	 
}

//VL53L0X 写字（2字节）寄存器
//Dev:设备I2C参数结构体
//index:偏移地址
//pdata:数据指针
//count:长度
//返回值: 0:成功  
//       其他:失败
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
	
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t deviceAddress;

	deviceAddress = Dev->I2cDevAddr;

	status_int = VL53L0X_write_word(Dev,index,data);

	if(status_int!=0)
	 Status = VL53L0X_ERROR_CONTROL_INTERFACE;

	return Status;
	
}

//VL53L0X 写双字（4字节）寄存器
//Dev:设备I2C参数结构体
//index:偏移地址
//pdata:数据指针
//count:长度
//返回值: 0:成功  
//       其他:失败
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
	
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t deviceAddress;

	deviceAddress = Dev->I2cDevAddr;

	status_int = VL53L0X_write_dword(Dev, index, data);

	if (status_int != 0)
	 Status = VL53L0X_ERROR_CONTROL_INTERFACE;

	return Status;
	
}

//VL53L0X 威胁安全更新(读/修改/写)单字节寄存器
//Dev:设备I2C参数结构体
//index:偏移地址
//AndData:8位与数据
//OrData:8位或数据
//返回值: 0:成功  
//       其他:错误
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData)
{
	 
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t deviceAddress;
	uint8_t data;

	deviceAddress = Dev->I2cDevAddr;

	status_int = VL53L0X_read_byte(Dev,index,&data);

	if(status_int!=0)
	  Status = VL53L0X_ERROR_CONTROL_INTERFACE;

	if(Status == VL53L0X_ERROR_NONE)
	{
	  data = (data & AndData) | OrData;
	  status_int = VL53L0X_write_byte(Dev, index,data);
	 
	 
	  if(status_int !=0)
		 Status = VL53L0X_ERROR_CONTROL_INTERFACE;
	 
	}

	return Status;
	  
}

//VL53L0X 读单字节寄存器
//Dev:设备I2C参数结构体
//index:偏移地址
//pdata:数据指针
//count:长度
//返回值: 0:成功  
//       其他:错误
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
	
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t deviceAddress;

	deviceAddress = Dev->I2cDevAddr;

	status_int = VL53L0X_read_byte(Dev, index, data);

	if(status_int !=0)
	Status = VL53L0X_ERROR_CONTROL_INTERFACE;

	return Status;

}

//VL53L0X 读字（2字节）寄存器
//Dev:设备I2C参数结构体
//index:偏移地址
//pdata:数据指针
//count:长度
//返回值: 0:成功  
//       其他:错误
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
	
	VL53L0X_Error Status = VL53L0X_ERROR_NONE;
	int32_t status_int;
	uint8_t deviceAddress;

	deviceAddress = Dev->I2cDevAddr;

	status_int = VL53L0X_read_word(Dev, index, data);

	if(status_int !=0)
	  Status = VL53L0X_ERROR_CONTROL_INTERFACE;

	return Status;
	  
}

//VL53L0X 读双字（4字节）寄存器
//Dev:设备I2C参数结构体
//index:偏移地址
//pdata:数据指针
//count:长度
//返回值: 0:成功  
//       其他:错误
VL53L0X_Error  VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
	
    VL53L0X_Error Status = VL53L0X_ERROR_NONE;
    int32_t status_int;
    uint8_t deviceAddress;

    deviceAddress = Dev->I2cDevAddr;

    status_int = VL53L0X_read_dword(Dev, index, data);

    if (status_int != 0)
        Status = VL53L0X_ERROR_CONTROL_INTERFACE;

    return Status;
}

//VL53L0X 底层延时函数
//Dev:设备I2C参数结构体
//返回值: 0:成功  
//       其他:错误
#define VL53L0X_POLLINGDELAY_LOOPNB  250
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev)
{
	
    VL53L0X_Error status = VL53L0X_ERROR_NONE;
    volatile uint32_t i;

    for(i=0;i<VL53L0X_POLLINGDELAY_LOOPNB;i++){
        //Do nothing
        
    }

    return status;
}

static int m_write(const VL53L0X_DEV dev, u8 reg_addr,u8 *data,u16 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const c_my_iic* iic = NULL;
    int ret = 0;
    u8 iic_addr = 0;

    /*检查参数*/
    if(NULL == dev || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(0 == len)
    {
        log_error("Param error.");
        return E_NULL;
    }
    iic = dev->m_iic;

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
    iic_addr = dev->I2cDevAddr | I2C_MASTER_WRITE;
    ret = iic_write_bytes(cmd,&iic_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*寄存器地址*/
    ret = iic_write_bytes(cmd,&reg_addr,1);
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
    ret = iic->begin(iic,cmd,dev->m_iic_speed,1000);
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

static int m_read(const VL53L0X_DEV dev, u8 reg_addr,u8 *data,u16 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const c_my_iic* iic = NULL;
    int ret = 0;
    u8 iic_addr_r = 0;
    u8 iic_addr_w = 0;

    /*检查参数*/
    if(NULL == dev || NULL == data)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(0 == len)
    {
        log_error("Param error.");
        return E_NULL;
    }
    iic = dev->m_iic;

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
    iic_addr_w = dev->I2cDevAddr | I2C_MASTER_WRITE;
    ret = iic_write_bytes(cmd,&iic_addr_w,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /*寄存器地址*/
    ret = iic_write_bytes(cmd,&reg_addr,1);
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

    iic_addr_r = dev->I2cDevAddr | I2C_MASTER_READ;
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
    ret = iic->begin(iic,cmd,dev->m_iic_speed,1000);
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

static int VL53L0X_write_byte (const VL53L0X_DEV dev ,u8 reg_addr,u8 data)
{
    int ret = 0;

    ret = m_write(dev,reg_addr,&data,1);

    return ret;
}

static int VL53L0X_write_word (const VL53L0X_DEV dev ,u8 reg_addr,u16 data)
{
    int ret = 0;
    u8 buffer[2];

    //将16位数据拆分成8位
	buffer[0] = (u8)(data>>8);//高八位
	buffer[1] = (u8)(data&0xff);//低八位

    if(1 == reg_addr %2)
    {
        //串行通信不能处理对非2字节对齐寄存器的字节
		ret = m_write(dev,reg_addr,&buffer[0],1);
        if(E_OK != ret)
        {
            return ret;
        }
		ret = m_write(dev,reg_addr,&buffer[0],1);
    }
    else
    {
        ret = m_write(dev,reg_addr,buffer,2);
    }

    return ret;
}
static int VL53L0X_write_dword(const VL53L0X_DEV dev ,u8 reg_addr,u32 data)
{
    int ret = 0;
    u8 buffer[4];	
	
	//将32位数据拆分成8位
	buffer[0] = (u8)(data>>24);
	buffer[1] = (u8)((data&0xff0000)>>16);
	buffer[2] = (u8)((data&0xff00)>>8);
	buffer[3] = (u8)(data&0xff);
	
	ret = m_write(dev,reg_addr,buffer,4);
	
	return ret;
}
static int VL53L0X_write_multi(const VL53L0X_DEV dev ,u8 reg_addr,u8 *pdata,u16 count)
{
    int ret = 0;

    ret = m_write(dev,reg_addr,pdata,count);

    return ret;
}
                                                 
static int VL53L0X_read_byte  (const VL53L0X_DEV dev , u8 reg_addr,u8 *pdata)
{
    int ret = 0;

    ret = m_read(dev,reg_addr,pdata,1);

    return ret;
}
static int VL53L0X_read_word  (const VL53L0X_DEV dev , u8 reg_addr,u16 *pdata)
{
    int ret = 0;
    u8 buffer[2];

    ret = m_read(dev,reg_addr,buffer,2);
    *pdata = ((u16)buffer[0]<<8)+(u16)buffer[1];

    return ret;
}

static int VL53L0X_read_dword (const VL53L0X_DEV dev , u8 reg_addr,u32 *pdata)
{
    int ret = 0;
    u8 buffer[4];

    ret = m_read(dev,reg_addr,buffer,4);
    *pdata = ((u32)buffer[0]<<24)+((u32)buffer[1]<<16)+((u32)buffer[2]<<8)+((u32)buffer[3]);

    return ret;
}

static int VL53L0X_read_multi (const VL53L0X_DEV dev , u8 reg_addr,u8 *pdata,u16 count)
{
    int ret = 0;

    ret = m_read(dev,reg_addr,pdata,count);

    return ret;
}


