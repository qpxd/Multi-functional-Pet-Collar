/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name: bmp180.h
* Description: 气压传感器驱动模块函数实现
* Author: xzh
* Version: v1.0
* Date: 2022/02/02
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_BMP180_ENABLED

#ifdef MODE_LOG_TAG
#undef MODE_LOG_TAG
#endif
#define MODE_LOG_TAG "bmp180"

#define BMP180_IIC_SPEED   I2C_SPEED_DEFAULT

/*校准系数寄存器  */
/*176 位 E2PROM 被划分为 11 个字，每个字 16 位。 这些包含 11 校准
系数。 每个传感器模块都有单独的系数。 在第一次计算温度和压力之前，主机读取 E2PROM 数据。*/
#define CA_START  0xAA
#define CA_LEN      22
#define CA_AC_1   0xAAAB
#define CA_AC_2   0xACAD
#define CA_AC_3   0xAEAF
#define CA_AC_4   0xB0B1
#define CA_AC_5   0xB2B3
#define CA_AC_6   0xB4B5
#define CA_B_1    0xB6B7
#define CA_B_2    0xB8B9
#define CA_MB     0xBABB
#define CA_MC     0xBCBD
#define CA_MD     0xBEBF

#define BMP180_CHIP_ID_REG              0xD0  /*芯片ID 该值固定为 0x55，可用于检查是否通讯正常*/
#define BMP180_VERSION_REG              0xD1  /*版本号*/

#define BMP180_CTRL_MEAS_REG            0xF4  /*控制测量寄存器*/
#define BMP180_ADC_OUT_MSB_REG          0xF6  /*adc输出 高位*/
#define BMP180_ADC_OUT_LSB_REG          0xF7  /*adc输出 低位*/
#define BMP180_ADC_OUT_XLSB_REG         0xF8  /*adc输出 低低位*/

#define BMP180_SOFT_RESET_REG           0xE0  /*软件复位寄存器*/
#define BMP180_T_MEASURE                0x2E  /* 温度测量 4.5ms*/
#define BMP180_P_MEASURE                0x34  /* 压力测量 */

#define PARAM_MG      3038        /*calibration parameter */
#define PARAM_MH     -7357        /*calibration parameter */
#define PARAM_MI      3791        /*calibration parameter */

#define BMP180_TEMP_CONVERSION_TIME  5 /* TO be spec'd by GL or SB 由 GL 或 SB 指定 */

static int m_get(c_bmp180* this,u8 mode,float* temp,float* altitude,u32* pa);
static int m_read    (const c_bmp180* this,u8 reg_addr,u8* data,u8 len);
static int m_write(const c_bmp180* this,u8 reg_addr,u8* data,u8 len);
static int m_get_ut(const c_bmp180* this,u16* ut);
static int m_get_up(const c_bmp180* this,u8 mode,u32* up);
static int m_get_pressure(const c_bmp180* this,u8 mode,u32* p);
static int m_get_temperature(const c_bmp180* this,float* t);

c_bmp180 bmp180_creat(const c_my_iic* iic);


typedef struct _M_BMP180
{

    short          m_ac1 ;
    short          m_ac2 ;
    short          m_ac3 ;
    unsigned short m_ac4 ;
    unsigned short m_ac5 ;
    unsigned short m_ac6 ;
    short          m_b1  ;
    short          m_b2  ;
    short          m_mb  ;
    short          m_mc  ;
    short          m_md  ;
    int            m_b5  ;

    const c_my_iic*  m_iic    ;   /*所在的IIC总线*/
}m_bmp180;


c_bmp180 bmp180_creat(const c_my_iic* iic)
{
    c_bmp180 new = {0};
    m_bmp180* m_this  = NULL;
    int ret = 0;
    u8 data[22] = {0};

    /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }
    
    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_bmp180));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_bmp180));
    m_this = new.this;

    /*保存相关变量*/
    m_this->m_iic   = iic   ;
    new.get         = m_get ;

    /*获取校准参数*/
    ret = m_read(&new,CA_START,(u8*)data,CA_LEN);
    if(E_OK != ret)
    {
        log_error("Calibration parameters get failed.");
        vPortFree(m_this);
        new.this = NULL;
        return new;
    }

    /*参数赋值*/
    /*parameters AC1-AC6*/
    m_this->m_ac1 =  (data[0] << 8) | data[1];
    m_this->m_ac2 =  (data[2] << 8) | data[3];
    m_this->m_ac3 =  (data[4] << 8) | data[5];
    m_this->m_ac4 =  (data[6] << 8) | data[7];
    m_this->m_ac5 =  (data[8] << 8) | data[9];
    m_this->m_ac6 = (data[10] << 8) | data[11];
    
    /*parameters B1,B2*/
    m_this->m_b1 =  (data[12] << 8) | data[13];
    m_this->m_b2 =  (data[14] << 8) | data[15];
    
    /*parameters MB,MC,MD*/
    m_this->m_mb =  (data[16] << 8) | data[17];
    m_this->m_mc =  (data[18] << 8) | data[19];
    m_this->m_md =  (data[20] << 8) | data[21];

    /*返回新建的对象*/
    return new;
}

static int m_get(c_bmp180* this,u8 mode,float* temp,float* altitude,u32* pa)
{
    int ret = 0;
    u16 ut = 0;
    u32 up = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == temp || NULL == altitude || NULL == pa)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*获取当前温度*/
    ret = m_get_temperature(this,temp);
    if(E_OK != ret)
    {
        log_error("ut get failed.");
        return E_ERROR;
    }

    /*获取气压*/
    ret = m_get_pressure(this,mode,pa);
    if(E_OK != ret)
    {
        log_error("up get failed.");
        return E_ERROR;
    }

    /*计算海拔*/
    //*altitude = (float)(101325 - *pa) / 100.0f * 9.0f;
    *altitude = 44330.0f * (1.0f - pow(((float)*pa / 101325.0f),(1.0f / 5.255f)));  


    return E_OK;
}
static int m_read    (const c_bmp180* this,u8 reg_addr,u8* data,u8 len)
{
    iic_cmd_handle_t  cmd = NULL;
    const m_bmp180* m_this = NULL;
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
    iic_addr_w = BMP180_ADDR | I2C_MASTER_WRITE;
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

    iic_addr_r = BMP180_ADDR | I2C_MASTER_READ;
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,BMP180_IIC_SPEED,1000);
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

static int m_write(const c_bmp180* this,u8 reg_addr,u8* data,u8 len)
{   
    iic_cmd_handle_t  cmd = NULL;
    const m_bmp180* m_this = NULL;
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
    iic_addr = BMP180_ADDR | I2C_MASTER_WRITE;
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
    ret = m_this->m_iic->begin(m_this->m_iic,cmd,BMP180_IIC_SPEED,1000);
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

/** read out ut for temperature conversion
   \return ut parameter that represents the uncompensated
    temperature sensors conversion value*/
/*读出 ut 进行温度转换  
返回表示未补偿的 ut  参数   uncompensated temperature value
温度传感器转换值 */
static int m_get_ut(const c_bmp180* this,u16* ut)
{
    u8 data[2] = {0};
    u8 temp = 0;
    int ret = 0;

    /*参数检查*/
    if(NULL == this || NULL == this->this || NULL == ut)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*启动温度测量*/
    temp = BMP180_T_MEASURE;
    ret = m_write(this,BMP180_CTRL_MEAS_REG,&temp,1);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }

    vTaskDelay(BMP180_TEMP_CONVERSION_TIME);  /*等待温度转换完成*/

    /*把温度读出来*/
    ret = m_read(this,BMP180_ADC_OUT_MSB_REG,data,2);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        return E_ERROR;
    }

    *ut = (data[0] << 8) | data[1];
    
    return E_OK;
}

/*读出压力转换
取决于过采样率设置可以是 16 到 19 位
\return 代表未补偿压力值的参数 */
static int m_get_up(const c_bmp180* this,u8 mode,u32* up)
{
    int ret = 0;
    u8 temp = 0;
    u8 data[3] = {0};

    /*参数检查*/
    if(NULL == this || NULL == this->this || NULL == up)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*启动压力转换*/
    temp = BMP180_P_MEASURE | (mode << 6);
    ret = m_write(this,BMP180_CTRL_MEAS_REG,&temp,1);
    if(E_OK != ret)
    {
        log_error("Write failed.");
        return E_ERROR;
    }

    vTaskDelay(2 + (3 << mode));   /*等待压力转换完成*/

    /*读取压力转换数值*/
    ret = m_read(this,BMP180_ADC_OUT_MSB_REG,data,3);
    if(E_OK != ret)
    {
        log_error("Read failed.");
        return E_ERROR;
    }

    *up = (((u32) data[0] << 16) | ((u32) data[1] << 8) | (u32) data[2]) >> (8-mode);
    
    return E_OK;
}

/** calculate temperature from ut
  ut was read from the device via I2C and fed into the
  right calc path for BMP180
  \param ut parameter ut read from device
  \return temperature in steps of 0.1 deg celsius
  \see bmp180_read_ut()*/
static int m_get_temperature(const c_bmp180* this,float* t)
{
    short temperature = 0;
    long x1 = 0, x2 = 0;
    m_bmp180* m_this = NULL;
    u16 ut = 0;
    int ret = 0;

    /*参数检查*/
    if(NULL == this || NULL == this->this || NULL == t)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*获取未校准的温度值*/
    ret = m_get_ut(this,&ut);
    if(E_OK != ret)
    {
         log_error("Uncompensated temperature get failed.");
         return E_ERROR;
    }
    
    x1 = (((int) ut - (long) m_this->m_ac6) * (int) m_this->m_ac5) >> 15;
    x2 = ((int) m_this->m_mc << 11) / (x1 + m_this->m_md);
    m_this->m_b5 = x1 + x2;

   temperature = ((m_this->m_b5 + 8) >> 4);  /* temperature in 0.1 deg C*/
   *t = (float)temperature / 10.0f;
   
   return E_OK;
}

/** calculate pressure from up
  up was read from the device via I2C and fed into the
  right calc path for BMP180
  In case of BMP180 averaging is done through oversampling by the sensor IC

  \param ut parameter ut read from device
  \return temperature in steps of 1.0 Pa
  \see bmp180_read_up()*/

static int m_get_pressure(const c_bmp180* this,u8 mode,u32* p)
{
    long pressure = 0, x1 = 0, x2 = 0, x3 = 0, b3 = 0, b6 = 0;
    unsigned long b4, b7;
    const m_bmp180* m_this = NULL;
    int ret = 0;
    u32 up = 0;

    /*参数检查*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(BMP180_P_lOW_POWER       != mode && BMP180_P_STANDARD               != mode && 
       BMP180_P_HIGH_RESOLUTION != mode && BMP180_P_ULTRA_HIGHT_RESOLUTION != mode)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*获取未校准的压力值*/
    ret = m_get_up(this,mode,&up);
    if(E_OK != ret)
    {
         log_error("Uncompensated pressure  get failed.");
         return E_ERROR;
    }

    b6 = m_this->m_b5 - 4000;
    
    /*****calculate B3************/
    x1 = (b6 * b6) >> 12;
    x1 *= m_this->m_b2;
    x1 >>= 11;

    x2 = (m_this->m_ac2 * b6);
    x2 >>= 11;

    x3 = x1 + x2;

    b3 = (((((int)m_this->m_ac1)*4 + x3) << mode)+2) >> 2;

    /*****calculate B4************/
    x1 = (m_this->m_ac3 * b6) >> 13;
    x2 = (m_this->m_b1 * ((b6*b6) >> 12)) >> 16;
    x3 = ((x1 + x2) + 2) >> 2;
    b4 = (m_this->m_ac4 * (u32) (x3 + 32768)) >> 15;

    b7 = ((unsigned long)(up - b3) * (50000>>mode));
    if (b7 < 0x80000000)
    {
        pressure = (b7 << 1) / b4;
    }
    else
    {
        pressure = (b7 / b4) << 1;
    
}

    x1 = pressure >> 8;
    x1 *= x1;
    x1 = (x1 * PARAM_MG) >> 16;
    x2 = (pressure * PARAM_MH) >> 16;
    pressure += (x1 + x2 + PARAM_MI) >> 4;/* pressure in Pa*/
    *p = pressure;
    
    return E_OK;
}

#endif
