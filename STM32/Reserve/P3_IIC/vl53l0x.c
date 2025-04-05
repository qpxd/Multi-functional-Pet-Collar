/***************************************************************************** 
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name  : vl53l0x.h
* Description: vl53l0x tof测距模块 函数实现
* Author: xzh
* Version: v1.0
* Date: 2022/02/10
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_VL53L0X_ENABLED

#define MODULE_NAME       "vl53l0x"

#ifdef  MODE_LOG_TAG
#undef  MODE_LOG_TAG
#endif
#define MODE_LOG_TAG          MODULE_NAME


//vl53l0x模式配置参数集
typedef struct
{
    FixPoint1616_t signalLimit;    //Signal极限数值 
    FixPoint1616_t sigmaLimit;     //Sigmal极限数值
    uint32_t timingBudget;         //采样时间周期
    uint8_t preRangeVcselPeriod ;  //VCSEL脉冲周期
    uint8_t finalRangeVcselPeriod ;//VCSEL脉冲周期范围
}mode_data;

extern mode_data Mode_data[];

//vl53l0x传感器校准信息结构体定义
typedef struct
{
    uint8_t  adjustok;                    //校准成功标志，0XAA，已校准;其他，未校准
    uint8_t  isApertureSpads;             //ApertureSpads值
    uint8_t  VhvSettings;                 //VhvSettings值
    uint8_t  PhaseCal;                    //PhaseCal值
    uint32_t XTalkCalDistance;            //XTalkCalDistance值
    uint32_t XTalkCompensationRateMegaCps;//XTalkCompensationRateMegaCps值
    uint32_t CalDistanceMilliMeter;       //CalDistanceMilliMeter值
    int32_t  OffsetMicroMeter;            //OffsetMicroMeter值
    uint32_t refSpadCount;                //refSpadCount值
    
}_vl53l0x_adjust;

typedef struct __M_VL53L0X
{
    VL53L0X_Dev_t          m_iic      ;  /*iic信息*/
    VL53L0X_DeviceInfo_t   m_info     ;  /*设备id*/
    _vl53l0x_adjust        m_cal_data ;  /*校准数据*/
    c_switch               m_xshut    ;  /*电源开关*/
    bool                   m_ajust_ok ;  /*校准标志*/
}m_vl53l0x;

static int m_get(const c_vl53l0x* this,u16* dis);
static int m_start(const c_vl53l0x* this,u8 measurement_mode, u8 data_mode);
static int m_set_addr(const c_vl53l0x* this,u8 addr);
static int m_reset(const c_vl53l0x* this);
void m_print_error(VL53L0X_Error status);


//VL53L0X各测量模式参数
//0：默认;1:高精度;2:长距离;3:高速
mode_data Mode_data[]= {{(FixPoint1616_t)(0.25 * 65536) , (FixPoint1616_t)(18 * 65536) ,  33000 , 14 , 10},//默认
                        {(FixPoint1616_t)(0.25 * 65536) , (FixPoint1616_t)(18 * 65536) , 200000 , 14 , 10},//高精度
                        {(FixPoint1616_t)(0.1  * 65536) , (FixPoint1616_t)(60 * 65536),   33000,  18 , 14},//长距离
                        {(FixPoint1616_t)(0.25 * 65536) , (FixPoint1616_t)(32 * 65536),   20000,  14 , 10},//高速
};

VL53L0X_Dev_t         vl53l0x_dev      ;   //设备I2C数据参数
VL53L0X_DeviceInfo_t  vl53l0x_dev_info ;   //设备ID版本信息
_vl53l0x_adjust       Vl53l0x_data     ;   //校准数据24c02读缓存区（用于系统初始化时向24C02读取数据）
uint8_t               AjustOK=0        ;   //校准标志位

c_vl53l0x vl53l0x_create(const c_my_iic* iic,u8 addr,GPIO_TypeDef* xshut_gpio,u32 xshut_pin)
{
     c_vl53l0x  new = {0};
     m_vl53l0x* m_this = NULL;
     VL53L0X_Error api_ret = VL53L0X_ERROR_NONE; 
     int ret = 0;

     /*参数检查*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return new;
    }

    /*为新对象申请内存*/
    new.this = pvPortMalloc(sizeof(m_vl53l0x));
    if(NULL == new.this)
    {
        log_error("Out of memory");
        return new;
    }
    memset(new.this,0,sizeof(m_vl53l0x));
    m_this = new.this;

    /*保存相关变量*/
    new.get = m_get;
    new.start = m_start;
    new.reset = m_reset;
    new.set_addr = m_set_addr;
    m_this->m_iic.comms_speed_khz = 400;
    m_this->m_iic.comms_type      =   1;
    m_this->m_iic.m_iic           = iic;
    m_this->m_iic.m_iic_speed     =   2;

    /*初始化电源开关*/
    m_this->m_xshut = switch_create(xshut_gpio,xshut_pin);
    if(NULL == m_this->m_xshut.this)
    {
        log_error("Xshut switch creat failed.");
        goto error_handle;
    }

    /*复位芯片*/
    ret = m_reset(&new);
    if(E_OK != ret)
    {
        log_error("Reset failed.");
        goto error_handle;
    }
    
    /*设置地址*/
    ret = m_set_addr(&new,addr);
    if(E_OK != ret)
    {
        log_error("Addr set failed.");
        goto error_handle;
    }

    /*获取设备ID信息*/
    api_ret = VL53L0X_GetDeviceInfo(&m_this->m_iic,&m_this->m_info);
    if(api_ret!=VL53L0X_ERROR_NONE) 
    {
        log_error("Vl53l0x get device failed.");
        goto error_handle;
    }
    
#if 0
    /*传感器信息打印*/
    log_inform("\r\n-------vl53l0x info-------\r\n\r\n");
    log_inform("  Name:%s\r\n"            ,m_this->m_info.Name);
    log_inform("  ProductId:%s\r\n"       ,m_this->m_info.ProductId);
    log_inform("  RevisionMajor:0x%x\r\n" ,m_this->m_info.ProductRevisionMajor);
    log_inform("  RevisionMinor:0x%x\r\n" ,m_this->m_info.ProductRevisionMinor);
    log_inform("\r\n-----------------------------------\r\n");
#endif

    /*检查校准状态*/
    //AT24CXX_Read(0,(u8*)&Vl53l0x_data,sizeof(_vl53l0x_adjust));//读取24c02保存的校准数据,若已校准 Vl53l0x_data.adjustok==0xAA
    if(m_this->m_cal_data.adjustok==0xAA)
    {
        /*已校准*/
        m_this->m_ajust_ok = true;

        //设定Spads校准值
        api_ret= VL53L0X_SetReferenceSpads(&m_this->m_iic,m_this->m_cal_data.refSpadCount,m_this->m_cal_data.isApertureSpads);
        if(api_ret!=VL53L0X_ERROR_NONE) 
        {
            log_error("Vl53l0x set reference spads failed.");
            goto error_handle;
        }
        vTaskDelay(2);         

        //设定Ref校准值 当温差大于8时 建议从新校准
        api_ret= VL53L0X_SetRefCalibration(&m_this->m_iic,m_this->m_cal_data.VhvSettings,m_this->m_cal_data.PhaseCal);
        if(api_ret!=VL53L0X_ERROR_NONE)
        {
            log_error("Vl53l0x set refcalibration failed.");
            goto error_handle;
        }
        vTaskDelay(2);

        //设定偏移校准值
        api_ret=VL53L0X_SetOffsetCalibrationDataMicroMeter(&m_this->m_iic,m_this->m_cal_data.OffsetMicroMeter);
        if(api_ret!=VL53L0X_ERROR_NONE)
        {
            log_error("Vl53l0x set offset failed.");
            goto error_handle;
        }
        vTaskDelay(2);

        //设定串扰校准值
        api_ret=VL53L0X_SetXTalkCompensationRateMegaCps(&m_this->m_iic,m_this->m_cal_data.XTalkCompensationRateMegaCps);
        if(api_ret!=VL53L0X_ERROR_NONE)
        {
            log_error("Vl53l0x set xtalk failed.");
            goto error_handle;
        }
        vTaskDelay(2);  
    }
    else 
    {
        /*没校准*/
        m_this->m_ajust_ok = false;

        //执行参考SPAD管理  
        api_ret = VL53L0X_PerformRefSpadManagement(&m_this->m_iic, &m_this->m_cal_data.refSpadCount, &m_this->m_cal_data.isApertureSpads);
        if(api_ret!=VL53L0X_ERROR_NONE)
        {
            log_error("Vl53l0x perform refspad failed.");
            goto error_handle;
        }
        vTaskDelay(2);

        //Ref参考校准 温度校准 当温差大于8时 建议从新校准
        api_ret = VL53L0X_PerformRefCalibration(&m_this->m_iic, &m_this->m_cal_data.VhvSettings, &m_this->m_cal_data.PhaseCal);
        if(api_ret!=VL53L0X_ERROR_NONE)
        {
            log_error("Vl53l0x perform refcalibration failed.");
            goto error_handle;
        }
        
        vTaskDelay(2);   
    }
    
    return new;

    error_handle:

    m_print_error(api_ret);//打印错误信息
    vPortFree(new.this);
    new.this = NULL;
    return new;
}

static int m_start(const c_vl53l0x* this,u8 measurement_mode, u8 data_mode)
{
    m_vl53l0x* m_this = NULL;
    VL53L0X_Error api_ret = VL53L0X_ERROR_NONE; 

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    if(VL53L0X_MEASUREMENT_SINGLE != measurement_mode && VL53L0X_MEASUREMENT_CONTINUOUS != measurement_mode)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    if(VL53L0X_MODE_DEFAULT != data_mode && VL53L0X_MODE_ACCURACY != data_mode && VL53L0X_MODE_LONG != data_mode && VL53L0X_MODE_HIGH != data_mode)
    {
        log_error("Param error.");
        return E_PARAM;
    }
    m_this = this->this;

    /*设置测量模式*/
    api_ret = VL53L0X_SetDeviceMode(&m_this->m_iic,measurement_mode);
    if(api_ret!=VL53L0X_ERROR_NONE)
    {
        log_error("Vl53l0x set device mode failed.");
        goto error_handle;
    }
    vTaskDelay(2);

    //使能SIGMA范围检查
    api_ret = VL53L0X_SetLimitCheckEnable(&m_this->m_iic,VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,1);
    if(api_ret!=VL53L0X_ERROR_NONE)
    {
        log_error("Vl53l0x enable sigma final range failed.");
        goto error_handle;
    }
    vTaskDelay(2);

    //使能信号速率范围检查
    api_ret = VL53L0X_SetLimitCheckEnable(&m_this->m_iic,VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,1);
    if(api_ret!=VL53L0X_ERROR_NONE)
    {
        log_error("Vl53l0x enable signal rete final range failed.");
        goto error_handle;
    }
    vTaskDelay(2);
    
    //设定SIGMA范围
    api_ret = VL53L0X_SetLimitCheckValue(&m_this->m_iic,VL53L0X_CHECKENABLE_SIGMA_FINAL_RANGE,Mode_data[data_mode].sigmaLimit);
    if(api_ret!=VL53L0X_ERROR_NONE)
    {
        log_error("Vl53l0x set sigma final range failed.");
        goto error_handle;
    }
    vTaskDelay(2);

    //设定信号速率范围范围
    api_ret = VL53L0X_SetLimitCheckValue(&m_this->m_iic,VL53L0X_CHECKENABLE_SIGNAL_RATE_FINAL_RANGE,Mode_data[data_mode].signalLimit);
    if(api_ret!=VL53L0X_ERROR_NONE)
    {
        log_error("Vl53l0x set signal rete final range failed.");
        goto error_handle;
    }
    vTaskDelay(2);

    //设定完整测距最长时间
    api_ret = VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&m_this->m_iic,Mode_data[data_mode].timingBudget);
    if(api_ret!=VL53L0X_ERROR_NONE)
    {
        log_error("Vl53l0x set ranging time failed.");
        goto error_handle;
    }
    vTaskDelay(2);

    //设定VCSEL脉冲周期
    api_ret = VL53L0X_SetVcselPulsePeriod(&m_this->m_iic, VL53L0X_VCSEL_PERIOD_PRE_RANGE, Mode_data[data_mode].preRangeVcselPeriod);
    if(api_ret!=VL53L0X_ERROR_NONE)
    {
        log_error("Vl53l0x vcsel pulse failed.");
        goto error_handle;
    }
    vTaskDelay(2);

    //设定VCSEL脉冲周期范围
    api_ret = VL53L0X_SetVcselPulsePeriod(&m_this->m_iic, VL53L0X_VCSEL_PERIOD_FINAL_RANGE, Mode_data[data_mode].finalRangeVcselPeriod);
    if(api_ret!=VL53L0X_ERROR_NONE)
    {
        log_error("Vl53l0x set vcsel scope failed.");
        goto error_handle;
    }

    /*如果是循环测量 则 直接启动测量*/
    if(VL53L0X_DEVICEMODE_CONTINUOUS_RANGING == measurement_mode)
    {
        api_ret = VL53L0X_StartMeasurement(&m_this->m_iic);
        if(api_ret !=VL53L0X_ERROR_NONE)
        {
            log_error("Vl53l0x start failed.");
            goto error_handle;
        }
        
        vTaskDelay(210);  /*等待第一次转换完成*/
    }

    return E_OK;

    error_handle:
        
    m_print_error(api_ret);//打印错误信息
    return E_ERROR;
}

static int m_get(const c_vl53l0x* this,u16* dis)
{
    m_vl53l0x* m_this = NULL;
    VL53L0X_Error api_ret = VL53L0X_ERROR_NONE; 
    VL53L0X_RangingMeasurementData_t data = {0};
    char buf[VL53L0X_MAX_STRING_LENGTH] = {0};
    VL53L0X_DeviceModes mode = 0;
    u8 i = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this || NULL == dis)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    /*获取设备模式*/
    api_ret = VL53L0X_GetDeviceMode(&m_this->m_iic,&mode);
    if(api_ret !=VL53L0X_ERROR_NONE)
    {
        log_error("Vl53l0x mode get failed.");
        goto error_handle;
    }

    for(i = 0;i < VL53L0X_REACQUIR_TIMES;++i)
    {
        /*如果是单次测量 执行单次测距并获取测距测量数据*/
        if(VL53L0X_DEVICEMODE_SINGLE_RANGING == mode)
        {
            api_ret = VL53L0X_PerformSingleRangingMeasurement(&m_this->m_iic,&data);
            if(api_ret !=VL53L0X_ERROR_NONE)
            {
                log_error("Vl53l0x single measurement failed.");
                goto error_handle;
            }
        }

        /*如果是循环测量 直接读取数据*/
        else if(VL53L0X_DEVICEMODE_CONTINUOUS_RANGING == mode)
        {
            api_ret = VL53L0X_GetRangingMeasurementData(&m_this->m_iic,&data);
            if(api_ret !=VL53L0X_ERROR_NONE)
            {
                log_error("Vl53l0x get measurement data failed.");
                goto error_handle;
            }
        }

        /*否则为错误的状态*/
        else 
        {
            log_error("Error state.");
            return E_ERROR;
        }

        /*检查值是否有效*/
        if(0 != data.RangeStatus)
        {
            if(i != VL53L0X_REACQUIR_TIMES - 1)
            {
                log_inform("Get again,times:%d.",i + 1);
                vTaskDelay(35);
                continue;
            }
            log_error("Invalid distance.");

            /*获取状态字符串*/
            api_ret = VL53L0X_GetRangeStatusString(data.RangeStatus,buf);
            if(api_ret !=VL53L0X_ERROR_NONE)
            {
                log_error("Vl53l0x get range status string failed.");
            }
            else
            {
                *dis = 0;
                log_error("dis:%d,error:%s",data.RangeMilliMeter,buf);
                return E_ERROR;
            }
            goto error_handle;
        }
        else
        {
            break;
        }
    }

    *dis = data.RangeMilliMeter;

    return E_OK;

    error_handle:

    *dis = 0;
    m_print_error(api_ret);//打印错误信息
    return E_ERROR;
}

static int m_set_addr(const c_vl53l0x* this,u8 addr)
{
    m_vl53l0x* m_this = NULL;
    VL53L0X_Error api_ret = VL53L0X_ERROR_NONE;
    u16 id = 0,n_id = 0;;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    addr <<= 1;

    /*如果新旧地址一样 就直接返回*/
    if(addr == m_this->m_iic.I2cDevAddr)
    {
        return E_OK;
    }

    /*读取ID*/
    api_ret = VL53L0X_RdWord(&m_this->m_iic, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &id);
    if(api_ret!=VL53L0X_ERROR_NONE) 
    {
        log_error("Vl53l0x get id failed.");
        goto error_handle;
    }

    /*设置新地址*/
    api_ret = VL53L0X_SetDeviceAddress(&m_this->m_iic,addr);
    if(api_ret!=VL53L0X_ERROR_NONE) 
    {
        log_error("Vl53l0x set addr failed.");
        goto error_handle;
    }

    m_this->m_iic.I2cDevAddr = addr;

    /*验证地址*/
    api_ret = VL53L0X_RdWord(&m_this->m_iic, VL53L0X_REG_IDENTIFICATION_MODEL_ID, &n_id);
    if(api_ret!=VL53L0X_ERROR_NONE) 
    {
        log_error("Vl53l0x get id failed.");
        goto error_handle;
    }
    if(id != n_id)
    {
        log_error("Vl53l0x set addr failed.");
        goto error_handle;
    }

    return E_OK;

    error_handle:

    m_print_error(api_ret);//打印错误信息
    return E_ERROR;
}

static int m_reset(const c_vl53l0x* this)
{
    m_vl53l0x* m_this = NULL;
    VL53L0X_Error api_ret = VL53L0X_ERROR_NONE;
    int ret = 0;

    /*检查参数*/
    if(NULL == this || NULL == this->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    m_this = this->this;

    ret = m_this->m_xshut.set(&m_this->m_xshut,GPIO_PIN_RESET);  /*失能*/
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }
    
    vTaskDelay(30);

    /*使能 芯片*/
    ret = m_this->m_xshut.set(&m_this->m_xshut,GPIO_PIN_SET);  /*高电平*/
    if(E_OK != ret)
    {
        log_error("Switch set failed.");
        goto error_handle;
    }
    vTaskDelay(30);

    m_this->m_iic.I2cDevAddr = VL53L0X_ADDR << 1;  /*复位后 地址会复位*/

    /*调用VL53L0X_DataInit()函数一次，设备上电后调用一次。把VL53L0X_State从VL53L0X_STATE_POWERDOWN改为 
    VL53L0X_STATE_WAIT_STATICINIT。VL53L0X_State是初始化状态机，看此变量的值可以就可以知道当前的初始化进度。*/
    api_ret = VL53L0X_DataInit(&m_this->m_iic);
    if(api_ret!=VL53L0X_ERROR_NONE) 
    {
        log_error("Vl53l0x init failed.");
        goto error_handle;
    }

    vTaskDelay(2);

    /*基本初始化*/
    /*调用VL53L0X_StaticInit()函数来做基本的设备初始化。把VL53L0X_State从VL53L0X_STATE_WAIT_STATICINIT改为VL53L0X_STATE_IDLE.*/
    api_ret = VL53L0X_StaticInit(&m_this->m_iic);
    if(api_ret!=VL53L0X_ERROR_NONE) 
    {
        log_error("Vl53l0x static init failed.");
        goto error_handle;
    }
    
    vTaskDelay(2);

    return E_OK;
    
    error_handle:

    m_print_error(api_ret);//打印错误信息
    return E_ERROR;

}


//API错误信息打印
//Status：详情看VL53L0X_Error参数的定义
void m_print_error(VL53L0X_Error status)
{
    
    char buf[VL53L0X_MAX_STRING_LENGTH];
    
    VL53L0X_GetPalErrorString(status,buf);//根据Status状态获取错误信息字符串
    
    log_error("API Status: %i : %s\r\n",status, buf);//打印状态和错误信息
    
}

#endif