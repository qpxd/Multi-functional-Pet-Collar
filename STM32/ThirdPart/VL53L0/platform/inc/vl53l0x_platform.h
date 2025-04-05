/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : vl53l0x_platform.c
* Description: vl53l0x tof测距模块平台连接公开头文件
* Author: xzh
* Version: v1.0
* Date: 2022/02/13
*****************************************************************************/

#ifndef __VL53L0X_PLATFORM_H
#define __VL53L0X_PLATFORM_H

#include "onchip_conf.h"
#include "vl53l0x_def.h"
#include "vl53l0x_platform_log.h"


//vl53l0x设备I2C信息
typedef struct {
    VL53L0X_DevData_t Data;              /*!< embed ST Ewok Dev  data as "Data"*/
    /*!< user specific field */
    const c_my_iic*  m_iic ;/*所在的IIC总线*/
    uint8_t          m_iic_speed;
    uint8_t   I2cDevAddr;                /*!< i2c device address user specific field */
    uint8_t   comms_type;                /*!< Type of comms : VL53L0X_COMMS_I2C or VL53L0X_COMMS_SPI */
    uint16_t  comms_speed_khz;           /*!< Comms speed [kHz] : typically 400kHz for I2C           */

} VL53L0X_Dev_t;


typedef VL53L0X_Dev_t* VL53L0X_DEV;

#define VL53L0X_MAX_I2C_XFER_SIZE  64 //定义I2C写的最大字节数
#define PALDevDataGet(Dev, field) (Dev->Data.field)
#define PALDevDataSet(Dev, field, data) (Dev->Data.field)=(data)


VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata,uint32_t count);
VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata,uint32_t count);
VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data);
VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data);
VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data);
VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData);
VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data);
VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data);
VL53L0X_Error VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data);
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev);


#endif 

