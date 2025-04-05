/*****************************************************************************
* Copyright: 2022-2024 版权所有 成都喜戈玛科技有限公司
* File name: max30102.c
* Description: max30102模块 函数实现
* Author: 许少
* Version: V1.0
* Date: 2022/03/08
* log:  V1.0  2022/03/08
*       发布：
*****************************************************************************/

#include "driver_conf.h"

#ifdef DRIVER_MAX30102_ENABLED

#ifdef MODE_LOG_TAG
#undef MODE_LOG_TAG
#endif
#define MODE_LOG_TAG "max30102"

#define MAX_BRIGHTNESS 255

#define IIC_SPEED  0

#define I2C_WR	0		/* 写控制bit */
#define I2C_RD	1		/* 读控制bit */
#define max30102_WR_address 0xAE

#define REG_INTR_STATUS_1 0x00
#define REG_INTR_STATUS_2 0x01
#define REG_INTR_ENABLE_1 0x02
#define REG_INTR_ENABLE_2 0x03
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D
#define REG_PILOT_PA 0x10
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12
#define REG_TEMP_INTR 0x1F
#define REG_TEMP_FRAC 0x20
#define REG_TEMP_CONFIG 0x21
#define REG_PROX_INT_THRESH 0x30
#define REG_REV_ID 0xFE
#define REG_PART_ID 0xFF

static c_my_iic       g_iic;
static TaskHandle_t   g_task  ;  /*任务句柄*/
static int32_t        g_heat  ;//心率
static int32_t        g_o2    ;//血氧浓度

static int m_get(u32* heat,u32* o2);
static int maxim_max30102_init(const c_my_iic* iic);
static int maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led);
static int maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data);
static int maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data);
static void m_task(void* pdata);

const c_max30102 max30102 = {maxim_max30102_init,m_get};

int maxim_max30102_init(const c_my_iic* iic)
/**
* \brief        Initialize the MAX30102
* \par          Details
*               This function initializes the MAX30102
*
* \param        None
*
* \retval       true on success
*/
{
    BaseType_t  os_ret = pdFALSE;
    uint8_t uch_dummy = 0;
    int ret = 0;
    
    /*检查参数*/
    if(NULL == iic || NULL == iic->this)
    {
        log_error("Null pointer.");
        return E_NULL;
    } 
    memcpy((void*)&g_iic,iic,sizeof(c_my_iic));  /*保存iic*/
    
    //resets the MAX30102
    ret = maxim_max30102_write_reg(REG_MODE_CONFIG, 0x40);
    if(E_OK != ret)
    {
        log_error("Write failed");
        return E_ERROR;
    }
    
    //Reads/clears the interrupt status register
    ret = maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_dummy); 
    if(E_OK != ret)
    {
        log_error("Write failed");
        return E_ERROR;
    }

    
    if(maxim_max30102_write_reg(REG_INTR_ENABLE_1, 0xc0)) // INTR setting
    {
        log_error("Write failed");
        return E_ERROR;
    }
    if(maxim_max30102_write_reg(REG_INTR_ENABLE_2, 0x00))
    {
        log_error("Write failed");
        return E_ERROR;
    }
    if(maxim_max30102_write_reg(REG_FIFO_WR_PTR, 0x00)) //FIFO_WR_PTR[4:0]
    {
        log_error("Write failed");
        return E_ERROR;
    }
    if(maxim_max30102_write_reg(REG_OVF_COUNTER, 0x00)) //OVF_COUNTER[4:0]
    {
        log_error("Write failed");
        return E_ERROR;
    }
    if(maxim_max30102_write_reg(REG_FIFO_RD_PTR, 0x00)) //FIFO_RD_PTR[4:0]
    {
        log_error("Write failed");
        return E_ERROR;
    }
    if(maxim_max30102_write_reg(REG_FIFO_CONFIG, 0x6f)) //sample avg = 8, fifo rollover=false, fifo almost full = 17
    {
        log_error("Write failed");
        return E_ERROR;
    }
    if(maxim_max30102_write_reg(REG_MODE_CONFIG, 0x03))  //0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED
    {
        log_error("Write failed");
        return E_ERROR;
    }
    if(maxim_max30102_write_reg(REG_SPO2_CONFIG, 0x2F)) // SPO2_ADC range = 4096nA, SPO2 sample rate (400 Hz), LED pulseWidth (411uS)
    {
        log_error("Write failed");
        return E_ERROR;
    }

    if(maxim_max30102_write_reg(REG_LED1_PA, 0x17))  //Choose value for ~ 4.5mA for LED1
    {
        log_error("Write failed");
        return E_ERROR;
    }
    if(maxim_max30102_write_reg(REG_LED2_PA, 0x17))  // Choose value for ~ 4.5mA for LED2
    {
        log_error("Write failed");
        return E_ERROR;
    }
    if(maxim_max30102_write_reg(REG_PILOT_PA, 0x7f))  // Choose value for ~ 25mA for Pilot LED
    {
        log_error("Write failed");
        return E_ERROR;
    }
    
        /*创建任务*/
    os_ret = xTaskCreate((TaskFunction_t )m_task          ,
                         (const char*    )"max30102 task" ,
                         (uint16_t       )MAX30102_TASK_STK  ,
                         (void*          )NULL          ,
                         (UBaseType_t    )MAX30102_TASK_PRO  ,
                         (TaskHandle_t*  )&g_task);
                             
    if(pdPASS != os_ret)
    {
        log_error("Task creat failed");
        return E_ERROR;
    }
    
    return E_OK;
}

static void m_task(void* pdata)
{
    uint32_t aun_ir_buffer[150]; //infrared LED sensor data
    uint32_t aun_red_buffer[150];  //red LED sensor data
    int32_t n_ir_buffer_length; //data length
    int32_t n_spo2;  //SPO2 value
    int8_t ch_spo2_valid;  //indicator to show if the SPO2 calculation is valid
    int32_t n_heart_rate; //heart rate value
    int8_t  ch_hr_valid;  //indicator to show if the heart rate calculation is valid
    uint8_t uch_dummy;

    int32_t hr_buf[16];
    int32_t hrSum;
    int32_t hrAvg;//心率
    int32_t spo2_buf[16];
    int32_t spo2Sum;
    int32_t spo2Avg;//血氧浓度
    int32_t spo2BuffFilled;
    int32_t hrBuffFilled;
    int32_t hrValidCnt = 0;
    int32_t spo2ValidCnt = 0;
    int32_t hrThrowOutSamp = 0;
    int32_t spo2ThrowOutSamp = 0;
    int32_t spo2Timeout = 0;
    int32_t hrTimeout = 0;
    
    uint32_t un_min, un_max, un_prev_data, un_brightness;  //variables to calculate the on-board LED brightness that reflects the heartbeats
    int32_t i;
    float f_temp;
    int32_t COUNT = 0;

    un_brightness = 0;
    un_min = 0x3FFFF;
    un_max = 0;

    n_ir_buffer_length = 150; //buffer length of 150 stores 3 seconds of samples running at 50sps

    //read the first 150 samples, and determine the signal range
    for(i = 0; i < n_ir_buffer_length; i++)
    {
        //while(KEY0 == 1); //wait until the interrupt pin asserts
        maxim_max30102_read_fifo((aun_red_buffer + i), (aun_ir_buffer + i)); //read from MAX30102 FIFO

        if(un_min > aun_red_buffer[i])
            un_min = aun_red_buffer[i]; //update signal min
        if(un_max < aun_red_buffer[i])
            un_max = aun_red_buffer[i]; //update signal max
    }
    un_prev_data = aun_red_buffer[i];
    //calculate heart rate and SpO2 after first 150 samples (first 3 seconds of samples)
    maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);

    //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
    
    while(1)
    {

        i = 0;
        un_min = 0x3FFFF;
        un_max = 0;

        //dumping the first 50 sets of samples in the memory and shift the last 100 sets of samples to the top
        for(i = 50; i < 150; i++)
        {
            aun_red_buffer[i - 50] = aun_red_buffer[i];
            aun_ir_buffer[i - 50] = aun_ir_buffer[i];

            //update the signal min and max
            if(un_min > aun_red_buffer[i])
                    un_min = aun_red_buffer[i];
            if(un_max < aun_red_buffer[i])
                    un_max = aun_red_buffer[i];
        }

        //take 50 sets of samples before calculating the heart rate.
        for(i = 100; i < 150; i++)
        {
            un_prev_data = aun_red_buffer[i - 1];
            //while(KEY0 == 1);
            maxim_max30102_read_fifo((aun_red_buffer + i), (aun_ir_buffer + i));

            //calculate the brightness of the LED
            if(aun_red_buffer[i] > un_prev_data)
            {
                    f_temp = aun_red_buffer[i] - un_prev_data;
                    f_temp /= (un_max - un_min);
                    f_temp *= MAX_BRIGHTNESS;
                    f_temp = un_brightness - f_temp;
                    if(f_temp < 0)
                            un_brightness = 0;
                    else
                            un_brightness = (int)f_temp;
            }
            else
            {
                    f_temp = un_prev_data - aun_red_buffer[i];
                    f_temp /= (un_max - un_min);
                    f_temp *= MAX_BRIGHTNESS;
                    un_brightness += (int)f_temp;
                    if(un_brightness > MAX_BRIGHTNESS)
                            un_brightness = MAX_BRIGHTNESS;
            }
        }
        maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
        if(COUNT++ > 5)
        {
            COUNT = 0;
            if ((ch_hr_valid == 1) && (n_heart_rate < 190) && (n_heart_rate > 40))
            {
                hrTimeout = 0;

                // Throw out up to 1 out of every 5 valid samples if wacky
                if (hrValidCnt == 4)
                {
                    hrThrowOutSamp = 1;
                    hrValidCnt = 0;
                    for (i = 12; i < 16; i++)
                    {
                        if (n_heart_rate < hr_buf[i] + 10)
                        {
                                hrThrowOutSamp = 0;
                                hrValidCnt   = 4;
                        }
                    }
                }
                else
                {
                    hrValidCnt = hrValidCnt + 1;
                }

                if (hrThrowOutSamp == 0)
                {

                    // Shift New Sample into buffer
                    for(i = 0; i < 15; i++)
                    {
                            hr_buf[i] = hr_buf[i + 1];
                    }
                    hr_buf[15] = n_heart_rate;

                    // Update buffer fill value
                    if (hrBuffFilled < 16)
                    {
                            hrBuffFilled = hrBuffFilled + 1;
                    }

                    // Take moving average
                    hrSum = 0;
                    if (hrBuffFilled < 2)
                    {
                            hrAvg = 0;
                    }
                    else if (hrBuffFilled < 4)
                    {
                            for(i = 14; i < 16; i++)
                            {
                                    hrSum = hrSum + hr_buf[i];
                            }
                            hrAvg = hrSum >> 1;
                    }
                    else if (hrBuffFilled < 8)
                    {
                            for(i = 12; i < 16; i++)
                            {
                                    hrSum = hrSum + hr_buf[i];
                            }
                            hrAvg = hrSum >> 2;
                    }
                    else if (hrBuffFilled < 16)
                    {
                            for(i = 8; i < 16; i++)
                            {
                                    hrSum = hrSum + hr_buf[i];
                            }
                            hrAvg = hrSum >> 3;
                    }
                    else
                    {
                            for(i = 0; i < 16; i++)
                            {
                                    hrSum = hrSum + hr_buf[i];
                            }
                            hrAvg = hrSum >> 4;
                    }
                }
                            hrThrowOutSamp = 0;
            }
            else
            {
                hrValidCnt = 0;
                if (hrTimeout == 4)
                {
                        hrAvg = 0;
                        hrBuffFilled = 0;
                }
                else
                {
                        hrTimeout++;
                }
            }

            if ((ch_spo2_valid == 1) && (n_spo2 > 59))
            {
                spo2Timeout = 0;

                // Throw out up to 1 out of every 5 valid samples if wacky
                if (spo2ValidCnt == 4)
                {
                        spo2ThrowOutSamp = 1;
                        spo2ValidCnt = 0;
                        for (i = 12; i < 16; i++)
                        {
                                if (n_spo2 > spo2_buf[i] - 10)
                                {
                                        spo2ThrowOutSamp = 0;
                                        spo2ValidCnt   = 4;
                                }
                        }
                }
                else
                {
                        spo2ValidCnt = spo2ValidCnt + 1;
                }

                if (spo2ThrowOutSamp == 0)
                {

                    // Shift New Sample into buffer
                    for(i = 0; i < 15; i++)
                    {
                            spo2_buf[i] = spo2_buf[i + 1];
                    }
                    spo2_buf[15] = n_spo2;

                    // Update buffer fill value
                    if (spo2BuffFilled < 16)
                    {
                            spo2BuffFilled = spo2BuffFilled + 1;
                    }

                    // Take moving average
                    spo2Sum = 0;
                    if (spo2BuffFilled < 2)
                    {
                            spo2Avg = 0;
                    }
                    else if (spo2BuffFilled < 4)
                    {
                            for(i = 14; i < 16; i++)
                            {
                                    spo2Sum = spo2Sum + spo2_buf[i];
                            }
                            spo2Avg = spo2Sum >> 1;
                    }
                    else if (spo2BuffFilled < 8)
                    {
                            for(i = 12; i < 16; i++)
                            {
                                    spo2Sum = spo2Sum + spo2_buf[i];
                            }
                            spo2Avg = spo2Sum >> 2;
                    }
                    else if (spo2BuffFilled < 16)
                    {
                            for(i = 8; i < 16; i++)
                            {
                                    spo2Sum = spo2Sum + spo2_buf[i];
                            }
                            spo2Avg = spo2Sum >> 3;
                    }
                    else
                    {
                            for(i = 0; i < 16; i++)
                            {
                                    spo2Sum = spo2Sum + spo2_buf[i];
                            }
                            spo2Avg = spo2Sum >> 4;
                    }
                }
                spo2ThrowOutSamp = 0;
            }
            else
            {
                    spo2ValidCnt = 0;
                    if (spo2Timeout == 4)
                    {
                            spo2Avg = 0;
                            spo2BuffFilled = 0;
                    }
                    else
                    {
                            spo2Timeout++;
                    }
            }

        }
			
        if(hrAvg >= 0)
        {
            g_heat = hrAvg;
        }
        if(0 != g_heat)
        {
            if(0 < spo2Avg)
            {
                g_o2 = spo2Avg;
            }
        }
        else
        {
            g_o2 = 0;
        }
        //log_inform("Heat:%d,O2:%d",hrAvg,spo2Avg);
        vTaskDelay(10);
    }
}

int maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led)

/**
* \brief        Read a set of samples from the MAX30102 FIFO register
* \par          Details
*               This function reads a set of samples from the MAX30102 FIFO register
*
* \param[out]   *pun_red_led   - pointer that stores the red LED reading data
* \param[out]   *pun_ir_led    - pointer that stores the IR LED reading data
*
* \retval       true on success
*/
{
    uint32_t un_temp;
    uint8_t uch_temp;
    *pun_ir_led = 0;
    *pun_red_led = 0;
    iic_cmd_handle_t  iic_cmd = NULL;
    int ret = 0;
    u8 w_addr = 0,r_addr = 0;
    u8 data1 = REG_FIFO_DATA;
    u8 buf[6] = {0};
    
    /*检查参数*/
    if(NULL == g_iic.this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }    
    
    maxim_max30102_read_reg(REG_INTR_STATUS_1, &uch_temp);
    maxim_max30102_read_reg(REG_INTR_STATUS_2, &uch_temp);

    /*创建新的命令链接*/
    iic_cmd  = iic_cmd_link_create();
    if(NULL == iic_cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }

    /* 第1步：发起I2C总线启动信号 */
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }    

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    w_addr = max30102_WR_address | I2C_WR;
    ret = iic_write_bytes(iic_cmd,&w_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }    

    /* 第4步：发送字节地址， */
    ret = iic_write_bytes(iic_cmd,&data1,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }   

    /* 第6步：重新启动I2C总线。下面开始读取数据 */
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }  

    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    r_addr = max30102_WR_address | I2C_RD;
    ret = iic_write_bytes(iic_cmd,&r_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    ret = iic_read_bytes(iic_cmd,buf,6);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /* 发送I2C总线停止信号 */
    ret = iic_master_stop(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }
    
    /*启动iic 序列*/
    ret = g_iic.begin(&g_iic,iic_cmd,IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        goto error_handle;
    }
    
    /*释放命令*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }
    
    un_temp = buf[0];
    un_temp <<= 16;
    *pun_red_led += un_temp;
    un_temp = buf[1];
    un_temp <<= 8;
    *pun_red_led += un_temp;
    un_temp = buf[2];
    *pun_red_led += un_temp;

    un_temp = buf[3];
    un_temp <<= 16;
    *pun_ir_led += un_temp;
    un_temp = buf[4];
    un_temp <<= 8;
    *pun_ir_led += un_temp;
    un_temp = buf[5];
    *pun_ir_led += un_temp;
    *pun_red_led &= 0x03FFFF; //Mask MSB [23:18]
    *pun_ir_led &= 0x03FFFF; //Mask MSB [23:18]
    
    return E_OK;
    
    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR; 
}

int maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data)
/**
* \brief        Write a value to a MAX30102 register
* \par          Details
*               This function writes a value to a MAX30102 register
*
* \param[in]    uch_addr    - register address
* \param[in]    uch_data    - register data
*
* \retval       true on success
*/
{
    iic_cmd_handle_t  iic_cmd = NULL;
    int ret = 0;
    u8 addr = 0;
    
    /*检查iic 状态*/
    if(NULL == g_iic.this)
    {
        log_error("Need init iic first.");
        return  E_ERROR;
    }
    
    /*创建新的命令链接*/
    iic_cmd  = iic_cmd_link_create();
    if(NULL == iic_cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }
    
    /* 第1步：发起I2C总线启动信号 */
    //i2c_Start();
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    addr = max30102_WR_address | I2C_WR;
    ret = iic_write_bytes(iic_cmd,&addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /* 第4步：发送字节地址 */
    ret = iic_write_bytes(iic_cmd,&uch_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    

    /* 第5步：开始写入数据 */
    ret = iic_write_bytes(iic_cmd,&uch_data,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /* 发送I2C总线停止信号 */
    ret = iic_master_stop(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }
        ret = iic_master_stop(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }
    
    /*启动iic 序列*/
    ret = g_iic.begin(&g_iic,iic_cmd,IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        goto error_handle;
    }
    
    /*释放命令*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }

    return E_OK;

    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR;
}

int maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data)
/**
* \brief        Read a MAX30102 register
* \par          Details
*               This function reads a MAX30102 register
*
* \param[in]    uch_addr    - register address
* \param[out]   puch_data    - pointer that stores the register data
*
* \retval       true on success
*/
{
    iic_cmd_handle_t  iic_cmd = NULL;
    int ret = 0;
    u8 w_addr = 0,r_addr = 0;
    
    /*检查参数*/
    if(NULL == g_iic.this)
    {
        log_error("Null pointer.");
        return E_NULL;
    }

    /*创建新的命令链接*/
    iic_cmd  = iic_cmd_link_create();
    if(NULL == iic_cmd)
    {
        log_error("IIC cmd link create failed.");
        return E_ERROR;
    }
    
    /* 第1步：发起I2C总线启动信号 */
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }
    

    /* 第2步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    w_addr = max30102_WR_address | I2C_WR;
    ret = iic_write_bytes(iic_cmd,&w_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /* 第4步：发送字节地址， */
    ret = iic_write_bytes(iic_cmd,&uch_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }    


    /* 第6步：重新启动I2C总线。下面开始读取数据 */
    ret = iic_master_start(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic start failed.");
        goto error_handle;
    }

    /* 第7步：发起控制字节，高7bit是地址，bit0是读写控制位，0表示写，1表示读 */
    r_addr = max30102_WR_address | I2C_RD;
    ret = iic_write_bytes(iic_cmd,&r_addr,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }

    /* 第9步：读取数据 */
    ret = iic_read_bytes(iic_cmd,puch_data,1);
    if(E_OK != ret)
    {
        log_error("Add iic write byte failed.");
        goto error_handle;
    }
    
    /* 发送I2C总线停止信号 */
    ret = iic_master_stop(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Add iic stop failed.");
        goto error_handle;
    }
    
    /*启动iic 序列*/
    g_iic.clear(&g_iic,w_addr,I2C_ACK_WAIT_DEFAULT_TIME,IIC_SPEED);
    ret = g_iic.begin(&g_iic,iic_cmd,IIC_SPEED,1000);
    if(E_OK != ret)
    {
        log_error("iic start failed.");
        goto error_handle;
    }
    
    /*释放命令*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
        return E_ERROR;
    }
    
    return E_OK;

    error_handle:
    
    /*删除链接*/
    ret = iic_cmd_link_delete(iic_cmd);
    if(E_OK != ret)
    {
        log_error("Delete iic cmd link failed.");
    }
    return E_ERROR; 
}

static int m_get(u32* heat,u32* o2)
{
    if(NULL == heat || NULL == o2)
    {
        log_error("Null pointer.");
        return E_NULL;
    }
    
    *heat = g_heat;
    *o2   = g_o2;
    
    return E_OK;
}
#endif